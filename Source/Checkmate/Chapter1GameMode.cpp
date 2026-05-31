// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "Chapter1GameMode.h"

#include "AudioService.h"
#include "CardData.h"
#include "CardSelectionScreen.h"
#include "Ch1DeckReplacementWidget.h"
#include "Ch1StarterStandardWidget.h"
#include "Ch1LocSubsystem.h"
#include "DollData.h"
#include "DollDisplay.h"
#include "InspectionScreen.h"
#include "JudgmentEvaluator.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

AChapter1GameMode::AChapter1GameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	// 默认 ADefaultPawn 会绑 mouse-look 让相机跟鼠标转——本游戏是固定视角下的 3D 检验台，
	// 用 bare APawn 替代（无 movement / 无 look 输入）。视角 = 该 pawn 在 PlayerStart 的 transform。
	DefaultPawnClass = APawn::StaticClass();
}

void AChapter1GameMode::BeginPlay()
{
	Super::BeginPlay();

	if (ReplacementQuotaPressure == 35)
	{
		ReplacementQuotaPressure = 20;
	}
	if (QuotaFailurePressure == 45)
	{
		QuotaFailurePressure = 20;
	}
	if (FMath::IsNearlyEqual(MoneyQuotaPressureRate, 0.08f))
	{
		MoneyQuotaPressureRate = 0.04f;
	}
	if (FMath::IsNearlyEqual(EndlessQuotaGrowthRate, 1.16f))
	{
		EndlessQuotaGrowthRate = 1.12f;
	}
	if (EndlessQuotaLinearStep == 55)
	{
		EndlessQuotaLinearStep = 40;
	}
	if (FMath::IsNearlyEqual(DesiredObjectivePassDollRatio, 0.55f))
	{
		DesiredObjectivePassDollRatio = 0.68f;
	}
	if (MinObjectivePassDollsPerDay == 3)
	{
		MinObjectivePassDollsPerDay = 4;
	}
	if (RunMoneyToChapter2Threshold == 900)
	{
		RunMoneyToChapter2Threshold = 700;
	}
	if (FalsePositiveImmediateValue == 12)
	{
		FalsePositiveImmediateValue = 0;
	}
	if (FalsePositiveRecallPenalty == 14)
	{
		FalsePositiveRecallPenalty = 65;
	}

	UAudioService::PlayCueStatic(this, FName("Amb.Ch1"));

	if (Shifts.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Chapter1GameMode: Shifts is empty — BP_Chapter1GameMode 需要填至少 1 条 ShiftConfig"));
		return;
	}

	if (!CardSelectionWidgetClass || !InspectionWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Chapter1GameMode: CardSelectionWidgetClass / InspectionWidgetClass 未填"));
		return;
	}

	// 把本地化字典注入到 subsystem，下游 Screen 都从 subsystem 取 FText
	if (LocStringsAsset)
	{
		if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
		{
			if (UCh1LocSubsystem* Loc = GI->GetSubsystem<UCh1LocSubsystem>())
			{
				Loc->SetStrings(LocStringsAsset);
			}
		}
	}

	// 强制相机：找关卡里的 InspectionCamera (Tag 或者第一个 ACameraActor) → 设为 PC 视口主相机。
	// 否则会用 Pawn 位置（bare APawn 无相机组件，会落回原点视角）。
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		TArray<AActor*> Cameras;
		UGameplayStatics::GetAllActorsWithTag(this, FName(TEXT("InspectionCamera")), Cameras);
		if (Cameras.Num() == 0)
		{
			// 回退：拿第一个 ACameraActor
			UGameplayStatics::GetAllActorsOfClass(this, ACameraActor::StaticClass(), Cameras);
		}
		if (Cameras.Num() > 0)
		{
			ApplyRuntimeCameraFraming(Cameras[0]);
			PC->SetViewTargetWithBlend(Cameras[0], 0.0f);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Chapter1GameMode: 关卡里没有 ACameraActor，相机用 pawn 默认视角"));
		}
	}

	EnsureRuntimeInspectionBackdrop();

	CurrentShiftIdx = 0;
	ShowShiftIntro(CurrentShiftIdx);
}

void AChapter1GameMode::ApplyRuntimeCameraFraming(AActor* CameraActor) const
{
	if (!bOverrideInspectionCameraTransform || !CameraActor)
	{
		return;
	}

	const FVector CameraLocation = bUseFlatFrontInspectionCamera
		? FVector(RuntimeCameraLocation.X, 0.0f, RuntimeCameraLookAt.Z)
		: RuntimeCameraLocation;
	const FVector LookAt = bUseFlatFrontInspectionCamera
		? FVector(0.0f, 0.0f, RuntimeCameraLookAt.Z)
		: RuntimeCameraLookAt;
	const FRotator LookAtRotation = (LookAt - CameraLocation).Rotation();
	CameraActor->SetActorLocationAndRotation(CameraLocation, LookAtRotation);

	if (ACameraActor* InspectionCamera = Cast<ACameraActor>(CameraActor))
	{
		if (UCameraComponent* CameraComponent = InspectionCamera->GetCameraComponent())
		{
			CameraComponent->SetProjectionMode(bUseFlatFrontInspectionCamera
				? ECameraProjectionMode::Orthographic
				: ECameraProjectionMode::Perspective);
			if (bUseFlatFrontInspectionCamera)
			{
				CameraComponent->SetOrthoWidth(RuntimeCameraOrthoWidth);
			}
		}
	}
}

void AChapter1GameMode::EnsureRuntimeInspectionBackdrop()
{
	if (!bSpawnRuntimeInspectionBackdrop || RuntimeInspectionBackdropActors.Num() > 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!CubeMesh)
	{
		return;
	}

	auto SpawnBlock = [this, World, CubeMesh](const FVector& Location, const FVector& Scale, const FVector& Tint)
	{
		AStaticMeshActor* Block = World->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator);
		if (!Block)
		{
			return;
		}

#if WITH_EDITOR
		Block->SetActorLabel(TEXT("Runtime_Ch1_InspectionBackdrop"));
#endif
		Block->SetMobility(EComponentMobility::Movable);
		if (UStaticMeshComponent* MC = Block->GetStaticMeshComponent())
		{
			MC->SetStaticMesh(CubeMesh);
			MC->SetRelativeScale3D(Scale);
			MC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			if (UMaterialInterface* BaseMaterial = MC->GetMaterial(0))
			{
				UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
				if (MID)
				{
					MID->SetVectorParameterValue(TEXT("Color"), Tint);
					MID->SetVectorParameterValue(TEXT("BaseColor"), Tint);
					MC->SetMaterial(0, MID);
				}
			}
		}
		RuntimeInspectionBackdropActors.Add(Block);
	};

	// The flat Ch1 camera looks along +X. These blocks sit behind the doll and below the frame,
	// restoring an Edith-Finch-like inspection nook without reintroducing a tilted camera.
	SpawnBlock(FVector(260.0f, 0.0f, 170.0f), FVector(0.10f, 8.4f, 3.8f), FVector(0.18f, 0.16f, 0.14f));
	SpawnBlock(FVector(70.0f, 0.0f, 20.0f), FVector(3.2f, 8.4f, 0.24f), FVector(0.24f, 0.20f, 0.16f));
}

void AChapter1GameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateProgressPanelPostProcessCue(DeltaSeconds);
}

void AChapter1GameMode::TriggerPresentationCue(FName CueId, int32 ShiftNumber, ECh1ProgressPanelMoment Moment, bool bMajorBeat)
{
	const FName ResolvedCue = CueId.IsNone()
		? (bMajorBeat ? FName("Ch1.PanelMajor") : FName("Ch1.PanelOpen"))
		: CueId;

	OnPresentationCue.Broadcast(ResolvedCue, ShiftNumber, Moment);
	K2_OnPresentationCue(ResolvedCue, ShiftNumber, Moment, bMajorBeat);
	UAudioService::PlayCueStatic(this, ResolvedCue, bMajorBeat ? 0.85f : 0.62f);
	StartProgressPanelPostProcessCue(bMajorBeat);

	UE_LOG(LogTemp, Display, TEXT("CH1_PRESENTATION_CUE Cue=%s Shift=%d Moment=%d Major=%s"),
		*ResolvedCue.ToString(), ShiftNumber, static_cast<int32>(Moment), bMajorBeat ? TEXT("true") : TEXT("false"));
}

FCh1ProgressPanelPayload AChapter1GameMode::BuildProgressPanelPayload(int32 ShiftIdx, ECh1ProgressPanelMoment Moment) const
{
	FCh1ProgressPanelPayload Payload;
	Payload.ShiftIndex = ShiftIdx;
	Payload.ShiftNumber = ShiftIdx + 1;
	Payload.TotalShifts = bUseEndlessRoguelikeRun ? 0 : FMath::Max(Shifts.Num(), 1);
	Payload.Moment = Moment;

	FShiftConfig RuntimeCfg;
	const FShiftConfig* Cfg = nullptr;
	if (Shifts.Num() > 0)
	{
		RuntimeCfg = BuildRuntimeShiftConfig(ShiftIdx);
		Cfg = &RuntimeCfg;
	}
	if (Cfg && !Cfg->PanelEyebrow.IsEmpty())
	{
		Payload.Eyebrow = Cfg->PanelEyebrow;
	}
	else
	{
		Payload.Eyebrow = FText::FromString(FString::Printf(TEXT("DAY %02d / %s"),
			Payload.ShiftNumber,
			bUseEndlessRoguelikeRun ? TEXT("ENDLESS LINE") : TEXT("FACTORY FLOOR")));
	}

	if (Cfg && !Cfg->PanelTitle.IsEmpty())
	{
		Payload.Title = Cfg->PanelTitle;
	}
	else
	{
		switch (Moment)
		{
		case ECh1ProgressPanelMoment::ShiftRetry:
			Payload.Title = FText::FromString(FString::Printf(TEXT("SHIFT %02d REOPENED"), Payload.ShiftNumber));
			break;
		case ECh1ProgressPanelMoment::ChapterComplete:
			Payload.Title = FText::FromString(TEXT("INSPECTION RECORD SEALED"));
			break;
		case ECh1ProgressPanelMoment::TwistLeadIn:
			Payload.Title = FText::FromString(TEXT("STANDARD ACCEPTED"));
			break;
		default:
			Payload.Title = FText::FromString(FString::Printf(TEXT("DAY %02d"), Payload.ShiftNumber));
			break;
		}
	}

	if (Cfg && !Cfg->PanelBody.IsEmpty() && Moment == ECh1ProgressPanelMoment::ShiftIntro)
	{
		Payload.Body = Cfg->PanelBody;
	}
	else if (Moment == ECh1ProgressPanelMoment::ShiftRetry)
	{
		Payload.Body = FText::FromString(TEXT("The record was rejected. Reassemble the standard and run the line again."));
	}
	else if (Moment == ECh1ProgressPanelMoment::TwistLeadIn)
	{
		Payload.Body = FText::FromString(TEXT("Hold the verdict. The factory is looking back."));
	}
	else
	{
		if (bUseEndlessRoguelikeRun && Cfg)
		{
			Payload.Body = FText::FromString(FString::Printf(
				TEXT("Quota $%d. Deck %d/%d. Replacement pressure %d, earnings pressure $%d."),
				Cfg->DailyQuota,
				CurrentDeckCards.Num(),
				MaxDeckCards,
				RunCardReplacementCount,
				RunTotalMoneyEarned));
		}
		else
		{
			static const TCHAR* ShiftBodies[] =
			{
				TEXT("Assemble the standard. Keep the line clean."),
				TEXT("The standard is now your choice."),
				TEXT("Work faster. Let the edge of the rule show."),
				TEXT("The standard may move while you are holding it.")
			};
			const int32 BodyIdx = FMath::Clamp(ShiftIdx, 0, static_cast<int32>(UE_ARRAY_COUNT(ShiftBodies)) - 1);
			Payload.Body = FText::FromString(ShiftBodies[BodyIdx]);
		}
	}

	if (Cfg && !Cfg->PanelCueId.IsNone())
	{
		Payload.CueId = Cfg->PanelCueId;
	}
	else if (Moment == ECh1ProgressPanelMoment::ShiftRetry)
	{
		Payload.CueId = FName("Ch1.PanelRetry");
	}
	else if (Moment == ECh1ProgressPanelMoment::TwistLeadIn)
	{
		Payload.CueId = FName("Ch1.PanelTwistLeadIn");
	}
	else
	{
		Payload.CueId = Payload.ShiftNumber >= 3 ? FName("Ch1.PanelMajor") : FName("Ch1.PanelOpen");
	}

	Payload.bMajorBeat = (Cfg && Cfg->bMajorPresentationBeat) || Payload.ShiftNumber >= 3
		|| Moment == ECh1ProgressPanelMoment::ChapterComplete
		|| Moment == ECh1ProgressPanelMoment::TwistLeadIn;

	return Payload;
}

void AChapter1GameMode::ShowShiftIntro(int32 ShiftIdx)
{
	const float HoldSeconds = ShiftIdx == 0 ? InitialPanelHoldSeconds : TransitionHoldSeconds;
	ShowProgressPanel(
		ShiftIdx,
		ECh1ProgressPanelMoment::ShiftIntro,
		HoldSeconds,
		FTimerDelegate::CreateLambda([this, ShiftIdx]()
		{
			CurrentShiftIdx = ShiftIdx;
			BeginShift(CurrentShiftIdx);
		}));
}

void AChapter1GameMode::ShowRetryTransition()
{
	ShowProgressPanel(
		CurrentShiftIdx,
		ECh1ProgressPanelMoment::ShiftRetry,
		FMath::Max(1.2f, TransitionHoldSeconds * 0.72f),
		FTimerDelegate::CreateLambda([this]()
		{
			BeginShift(CurrentShiftIdx);
		}));
}

void AChapter1GameMode::ShowProgressPanel(
	int32 ShiftIdx,
	ECh1ProgressPanelMoment Moment,
	float HoldSeconds,
	FTimerDelegate CompletionDelegate)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		CompletionDelegate.ExecuteIfBound();
		return;
	}

	GetWorldTimerManager().ClearTimer(ProgressPanelTimer);

	if (ActiveTransitionScreen)
	{
		ActiveTransitionScreen->RemoveFromParent();
		ActiveTransitionScreen = nullptr;
	}

	const FCh1ProgressPanelPayload Payload = BuildProgressPanelPayload(ShiftIdx, Moment);

	if (bUseNativeProgressPanelFallback || ProgressPanelWidgetClass || !ShiftTransitionWidgetClass)
	{
		TSubclassOf<UCh1ProgressPanelWidget> PanelClass = ProgressPanelWidgetClass;
		if (!PanelClass)
		{
			PanelClass = UCh1ProgressPanelWidget::StaticClass();
		}

		if (UCh1ProgressPanelWidget* ProgressPanel = CreateWidget<UCh1ProgressPanelWidget>(PC, PanelClass))
		{
			ProgressPanel->ConfigurePanel(Payload, HoldSeconds);
			ProgressPanel->AddToViewport(/*ZOrder=*/120);
			ActiveTransitionScreen = ProgressPanel;
		}
	}
	else
	{
		ActiveTransitionScreen = CreateWidget<UUserWidget>(PC, ShiftTransitionWidgetClass);
		if (ActiveTransitionScreen)
		{
			if (UTextBlock* Title = Cast<UTextBlock>(ActiveTransitionScreen->GetWidgetFromName(TEXT("TitleText"))))
			{
				Title->SetText(Payload.Title);
			}
			ActiveTransitionScreen->AddToViewport(/*ZOrder=*/120);
		}
	}

	SetUIInputMode();
	TriggerPresentationCue(Payload.CueId, Payload.ShiftNumber, Payload.Moment, Payload.bMajorBeat);

	GetWorldTimerManager().SetTimer(
		ProgressPanelTimer,
		FTimerDelegate::CreateLambda([this, CompletionDelegate]() mutable
		{
			if (UCh1ProgressPanelWidget* ProgressPanel = Cast<UCh1ProgressPanelWidget>(ActiveTransitionScreen))
			{
				ProgressPanel->StartDismiss();
			}
			if (ActiveTransitionScreen)
			{
				ActiveTransitionScreen->RemoveFromParent();
				ActiveTransitionScreen = nullptr;
			}
			CompletionDelegate.ExecuteIfBound();
		}),
		FMath::Max(0.5f, HoldSeconds),
		false);
}

void AChapter1GameMode::StartProgressPanelPostProcessCue(bool bMajorBeat)
{
	if (ProgressPanelPostProcessDuration <= 0.0f || ProgressPanelPostProcessPeak <= 0.0f)
	{
		return;
	}

	if (!ProgressPanelPostProcessVolume)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ProgressPanelPostProcessVolume = GetWorld()->SpawnActor<APostProcessVolume>(
			APostProcessVolume::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			Params);
		if (ProgressPanelPostProcessVolume)
		{
			ProgressPanelPostProcessVolume->bUnbound = true;
			ProgressPanelPostProcessVolume->Priority = 30.0f;
		}
	}

	ProgressPanelPostProcessElapsed = 0.0f;
	bProgressPanelPostProcessActive = ProgressPanelPostProcessVolume != nullptr;
	bProgressPanelPostProcessMajor = bMajorBeat;
}

void AChapter1GameMode::UpdateProgressPanelPostProcessCue(float DeltaSeconds)
{
	if (!bProgressPanelPostProcessActive || !ProgressPanelPostProcessVolume)
	{
		return;
	}

	ProgressPanelPostProcessElapsed += DeltaSeconds;
	const float T = FMath::Clamp(ProgressPanelPostProcessElapsed / FMath::Max(0.05f, ProgressPanelPostProcessDuration), 0.0f, 1.0f);
	const float Pulse = FMath::Sin(T * PI) * ProgressPanelPostProcessPeak * (bProgressPanelPostProcessMajor ? 1.0f : 0.58f);

	FPostProcessSettings& PP = ProgressPanelPostProcessVolume->Settings;
	ProgressPanelPostProcessVolume->BlendWeight = FMath::Clamp(Pulse, 0.0f, 1.0f);

	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = 0.28f + Pulse * (bProgressPanelPostProcessMajor ? 0.45f : 0.25f);

	PP.bOverride_SceneFringeIntensity = true;
	PP.SceneFringeIntensity = Pulse * (bProgressPanelPostProcessMajor ? 2.4f : 1.1f);

	PP.bOverride_BloomIntensity = true;
	PP.BloomIntensity = 0.22f + Pulse * (bProgressPanelPostProcessMajor ? 1.2f : 0.55f);

	PP.bOverride_AutoExposureBias = true;
	PP.AutoExposureBias = Pulse * (bProgressPanelPostProcessMajor ? 0.22f : 0.12f);

	if (ProgressPanelPostProcessElapsed >= ProgressPanelPostProcessDuration)
	{
		ProgressPanelPostProcessVolume->BlendWeight = 0.0f;
		bProgressPanelPostProcessActive = false;
	}
}

void AChapter1GameMode::BeginShift(int32 ShiftIdx)
{
	bInspectionLaunchInProgress = false;

	if (!Shifts.IsValidIndex(ShiftIdx) && !bUseEndlessRoguelikeRun)
	{
		FinishCh1();
		return;
	}

	ActiveRuntimeShiftConfig = BuildRuntimeShiftConfig(ShiftIdx);
	const FShiftConfig& Cfg = ActiveRuntimeShiftConfig;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		bInspectionLaunchInProgress = false;
		return;
	}

	// 清掉过场（如有）
	if (ActiveTransitionScreen)
	{
		ActiveTransitionScreen->RemoveFromParent();
		ActiveTransitionScreen = nullptr;
	}

	if (Cfg.K <= 0)
	{
		if (!bStarterStandardSeen && CurrentDeckCards.Num() == 0)
		{
			ShowStarterStandardPanel();
			return;
		}

		TArray<UCardData*> EmptySelection;
		HandleCardsAssembled(EmptySelection);
		return;
	}

	ActiveCardScreen = CreateWidget<UCardSelectionScreen>(PC, CardSelectionWidgetClass);
	if (!ActiveCardScreen)
	{
		UE_LOG(LogTemp, Error, TEXT("Chapter1GameMode: CreateWidget CardSelectionScreen 失败"));
		return;
	}

	const TArray<UCardData*> DraftPool = BuildDraftPoolForShift(Cfg);
	ActiveCardScreen->SetShiftConfig(DraftPool, Cfg.K, Cfg.AssemblyTimerSec);
	ActiveCardScreen->OnAssemblyComplete.AddDynamic(this, &AChapter1GameMode::HandleCardsAssembled);
	ActiveCardScreen->AddToViewport();

	SetUIInputMode();

	UE_LOG(LogTemp, Display, TEXT("Chapter1: Shift %d 开始 — Pool=%d, DraftPool=%d, Deck=%d, K=%d, DollTimeout=%.1fs"),
		ShiftIdx + 1, Cfg.PoolCards.Num(), DraftPool.Num(), CurrentDeckCards.Num(), Cfg.K, Cfg.DollTimeoutSec);
}

void AChapter1GameMode::ShowStarterStandardPanel()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		TArray<UCardData*> EmptySelection;
		HandleCardsAssembled(EmptySelection);
		return;
	}

	if (ActiveStarterStandardScreen)
	{
		ActiveStarterStandardScreen->RemoveFromParent();
		ActiveStarterStandardScreen = nullptr;
	}

	TSubclassOf<UCh1StarterStandardWidget> PanelClass = StarterStandardWidgetClass;
	if (!PanelClass)
	{
		PanelClass = UCh1StarterStandardWidget::StaticClass();
	}

	ActiveStarterStandardScreen = CreateWidget<UCh1StarterStandardWidget>(PC, PanelClass);
	if (!ActiveStarterStandardScreen)
	{
		TArray<UCardData*> EmptySelection;
		HandleCardsAssembled(EmptySelection);
		return;
	}

	ActiveStarterStandardScreen->OnConfirmed.AddDynamic(this, &AChapter1GameMode::HandleStarterStandardConfirmed);
	ActiveStarterStandardScreen->AddToViewport(/*ZOrder=*/125);
	SetUIInputMode();
}

void AChapter1GameMode::HandleStarterStandardConfirmed()
{
	if (bStarterStandardSeen || bInspectionLaunchInProgress || ActiveInspectionScreen)
	{
		return;
	}

	bStarterStandardSeen = true;

	if (ActiveStarterStandardScreen)
	{
		ActiveStarterStandardScreen->OnConfirmed.RemoveAll(this);
		ActiveStarterStandardScreen->RemoveFromParent();
		ActiveStarterStandardScreen = nullptr;
	}

	TArray<UCardData*> EmptySelection;
	HandleCardsAssembled(EmptySelection);
}

void AChapter1GameMode::HandleCardsAssembled(const TArray<UCardData*>& SelectedCards)
{
	if (bInspectionLaunchInProgress || ActiveInspectionScreen)
	{
		UE_LOG(LogTemp, Warning, TEXT("Chapter1: duplicate inspection launch ignored for shift %d"), CurrentShiftIdx + 1);
		return;
	}
	bInspectionLaunchInProgress = true;

	if (ActiveCardScreen)
	{
		ActiveCardScreen->OnAssemblyComplete.RemoveAll(this);
		ActiveCardScreen->RemoveFromParent();
		ActiveCardScreen = nullptr;
	}

	if (!ApplyDraftSelectionOrRequestReplacement(SelectedCards))
	{
		return;
	}

	LaunchInspectionForCurrentShift();
	return;
}

bool AChapter1GameMode::ApplyDraftSelectionOrRequestReplacement(const TArray<UCardData*>& SelectedCards)
{
	PendingDraftCard = nullptr;
	if (SelectedCards.Num() == 0)
	{
		LastSelectedCards = CurrentDeckCards;
		return true;
	}

	UCardData* PickedCard = SelectedCards[0];
	if (!PickedCard || CurrentDeckCards.Contains(PickedCard))
	{
		++RunDraftSkipCount;
		LastSelectedCards = CurrentDeckCards;
		return true;
	}

	if (CanAddCardToDeck(PickedCard))
	{
		AddCardToDeck(PickedCard);
		LastSelectedCards = CurrentDeckCards;
		return true;
	}

	const TArray<UCardData*> ReplacementCandidates = GetReplacementCandidates(PickedCard);
	if (ReplacementCandidates.Num() == 0)
	{
		++RunDraftSkipCount;
		UE_LOG(LogTemp, Warning, TEXT("Chapter1: picked card %s cannot fit and has no legal replacement; skipping."),
			*PickedCard->GetName());
		LastSelectedCards = CurrentDeckCards;
		return true;
	}

	PendingDraftCard = PickedCard;
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		bInspectionLaunchInProgress = false;
		return false;
	}

	ClearDeckReplacementScreen();
	TSubclassOf<UCh1DeckReplacementWidget> ReplacementClass = DeckReplacementWidgetClass;
	if (!ReplacementClass)
	{
		ReplacementClass = UCh1DeckReplacementWidget::StaticClass();
	}

	ActiveDeckReplacementScreen = CreateWidget<UCh1DeckReplacementWidget>(PC, ReplacementClass);
	if (!ActiveDeckReplacementScreen)
	{
		bInspectionLaunchInProgress = false;
		return false;
	}

	ActiveDeckReplacementScreen->ConfigureReplacement(
		PickedCard,
		CurrentDeckCards,
		ReplacementCandidates,
		MaxDeckCards,
		0);
	ActiveDeckReplacementScreen->OnResolved.AddDynamic(this, &AChapter1GameMode::HandleDeckReplacementResolved);
	ActiveDeckReplacementScreen->AddToViewport(/*ZOrder=*/130);
	SetUIInputMode();

	UE_LOG(LogTemp, Display, TEXT("Chapter1: deck full; waiting for replacement before shift %d."),
		CurrentShiftIdx + 1);
	return false;
}

void AChapter1GameMode::HandleDeckReplacementResolved(UCardData* NewCard, UCardData* ReplacedCard)
{
	ClearDeckReplacementScreen();

	if (NewCard && ReplacedCard && CurrentDeckCards.RemoveSingle(ReplacedCard) > 0)
	{
		CurrentDeckCards.AddUnique(NewCard);
		++RunCardReplacementCount;
		UE_LOG(LogTemp, Display, TEXT("Chapter1: replaced deck card %s -> %s; replacements=%d"),
			*ReplacedCard->GetName(), *NewCard->GetName(), RunCardReplacementCount);
	}
	else
	{
		++RunDraftSkipCount;
		UE_LOG(LogTemp, Display, TEXT("Chapter1: replacement skipped; skips=%d"), RunDraftSkipCount);
	}

	PendingDraftCard = nullptr;
	LaunchInspectionForCurrentShift();
}

void AChapter1GameMode::ClearDeckReplacementScreen()
{
	if (ActiveDeckReplacementScreen)
	{
		ActiveDeckReplacementScreen->OnResolved.RemoveAll(this);
		ActiveDeckReplacementScreen->RemoveFromParent();
		ActiveDeckReplacementScreen = nullptr;
	}
}

bool AChapter1GameMode::CanAddCardToDeck(UCardData* Card) const
{
	if (!Card)
	{
		return false;
	}

	const ECh1DeckSlot Slot = GetDeckSlot(Card);
	if (CurrentDeckCards.Num() >= MaxDeckCards)
	{
		return false;
	}

	return CountDeckSlot(Slot) < GetUnlockedSlotCap(Slot, CurrentShiftIdx);
}

void AChapter1GameMode::AddCardToDeck(UCardData* Card)
{
	if (Card)
	{
		CurrentDeckCards.AddUnique(Card);
	}
}

TArray<UCardData*> AChapter1GameMode::GetReplacementCandidates(UCardData* NewCard) const
{
	TArray<UCardData*> Candidates;
	if (!NewCard)
	{
		return Candidates;
	}

	for (UCardData* ExistingCard : CurrentDeckCards)
	{
		if (!ExistingCard)
		{
			continue;
		}

		TArray<UCardData*> SimulatedDeck = CurrentDeckCards;
		SimulatedDeck.RemoveSingle(ExistingCard);

		int32 SimTotal = SimulatedDeck.Num() + 1;
		int32 SimRed = 0;
		int32 SimBlack = 0;
		for (const UCardData* Card : SimulatedDeck)
		{
			if (GetDeckSlot(Card) == ECh1DeckSlot::Red)
			{
				++SimRed;
			}
			else if (GetDeckSlot(Card) == ECh1DeckSlot::Black)
			{
				++SimBlack;
			}
		}
		if (GetDeckSlot(NewCard) == ECh1DeckSlot::Red)
		{
			++SimRed;
		}
		else if (GetDeckSlot(NewCard) == ECh1DeckSlot::Black)
		{
			++SimBlack;
		}

		const bool bFitsTotal = SimTotal <= MaxDeckCards;
		const bool bFitsRed = SimRed <= GetUnlockedSlotCap(ECh1DeckSlot::Red, CurrentShiftIdx);
		const bool bFitsBlack = SimBlack <= GetUnlockedSlotCap(ECh1DeckSlot::Black, CurrentShiftIdx);
		if (bFitsTotal && bFitsRed && bFitsBlack)
		{
			Candidates.Add(ExistingCard);
		}
	}

	return Candidates;
}

ECh1DeckSlot AChapter1GameMode::GetDeckSlot(const UCardData* Card) const
{
	if (!Card)
	{
		return ECh1DeckSlot::Neutral;
	}

	if (Card->PieceworkEffect == ECh1CardEffect::SwordAndSerpent)
	{
		return ECh1DeckSlot::Red;
	}
	if (Card->PieceworkEffect == ECh1CardEffect::QueenGambit
		|| Card->PieceworkEffect == ECh1CardEffect::CheckmateDiscard)
	{
		return ECh1DeckSlot::Black;
	}

	if (Card->CardColor == ECh1CardColor::RedBonus)
	{
		return ECh1DeckSlot::Red;
	}
	if (Card->CardColor == ECh1CardColor::BlackCriterion)
	{
		return ECh1DeckSlot::Black;
	}
	return ECh1DeckSlot::Neutral;
}

int32 AChapter1GameMode::CountDeckSlot(ECh1DeckSlot Slot) const
{
	if (Slot == ECh1DeckSlot::Neutral)
	{
		return 0;
	}

	int32 Count = 0;
	for (const UCardData* Card : CurrentDeckCards)
	{
		if (GetDeckSlot(Card) == Slot)
		{
			++Count;
		}
	}
	return Count;
}

int32 AChapter1GameMode::GetUnlockedSlotCap(ECh1DeckSlot Slot, int32 ShiftIdx) const
{
	if (Slot == ECh1DeckSlot::Neutral)
	{
		return MaxDeckCards;
	}

	if (!bUseEndlessRoguelikeRun)
	{
		return Slot == ECh1DeckSlot::Red ? MaxRedCards : MaxBlackCards;
	}

	const int32 DayNumber = ShiftIdx + 1;
	if (DayNumber <= 1)
	{
		return 0;
	}
	if (DayNumber == 2)
	{
		return 1;
	}
	if (DayNumber == 3)
	{
		return Slot == ECh1DeckSlot::Red ? FMath::Min(2, MaxRedCards) : FMath::Min(1, MaxBlackCards);
	}
	return Slot == ECh1DeckSlot::Red ? MaxRedCards : MaxBlackCards;
}

void AChapter1GameMode::LaunchInspectionForCurrentShift()
{
	const FShiftConfig& Cfg = ActiveRuntimeShiftConfig;
	LastSelectedCards = CurrentDeckCards;

	UE_LOG(LogTemp, Display, TEXT("Chapter1: Shift %d deck locked (%d cards) -> inspection"),
		CurrentShiftIdx + 1, LastSelectedCards.Num());

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		bInspectionLaunchInProgress = false;
		return;
	}

	ActiveInspectionScreen = CreateWidget<UInspectionScreen>(PC, InspectionWidgetClass);
	if (!ActiveInspectionScreen)
	{
		bInspectionLaunchInProgress = false;
		return;
	}

	const int32 TargetDollCount = Cfg.DayDollTarget > 0 ? Cfg.DayDollTarget : Cfg.DollSequence.Num();
	const TArray<UDollData*> InspectionDollSequence = BuildBalancedInspectionDollSequence(
		Cfg.DollSequence,
		LastSelectedCards,
		TargetDollCount);

	ActiveInspectionScreen->SetShiftData(LastSelectedCards, InspectionDollSequence);
	ActiveInspectionScreen->DollTimeoutSec = Cfg.DollTimeoutSec;
	ActiveInspectionScreen->CorrectGoal = Cfg.CorrectGoal;
	ActiveInspectionScreen->PassQuota = Cfg.PassQuota;
	ActiveInspectionScreen->RejectQuota = Cfg.RejectQuota;
	ActiveInspectionScreen->MaxMisjudgmentsBeforeFail = 0;
	ActiveInspectionScreen->bUsePieceworkEconomy = bUsePieceworkEconomyFlow && Cfg.bUsePieceworkEconomy;
	ActiveInspectionScreen->ShiftNumber = CurrentShiftIdx + 1;
	ActiveInspectionScreen->DailyQuota = Cfg.DailyQuota;
	ActiveInspectionScreen->DayDollTarget = TargetDollCount > 0 ? TargetDollCount : InspectionDollSequence.Num();
	ActiveInspectionScreen->BaseGoodPrice = Cfg.BaseGoodPrice;
	ActiveInspectionScreen->BaseRejectValue = Cfg.BaseRejectValue;
	ActiveInspectionScreen->FalsePositiveImmediateValue = FalsePositiveImmediateValue;
	ActiveInspectionScreen->FalsePositiveRecallPenalty = FalsePositiveRecallPenalty;
	ActiveInspectionScreen->FalseNegativeDiscardPenalty = FalseNegativeDiscardPenalty;
	ActiveInspectionScreen->OnShiftCompleted.AddDynamic(this, &AChapter1GameMode::HandleShiftCompleted);
	ActiveInspectionScreen->OnMisjudgmentRecorded.AddDynamic(this, &AChapter1GameMode::HandleMisjudgmentRecorded);
	ActiveInspectionScreen->AddToViewport();

	if (DollActorClass && !ActiveDollActor)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ActiveDollActor = GetWorld()->SpawnActor<ADollDisplay>(
			DollActorClass, DollSpawnTransform, Params);
	}
	if (ActiveDollActor)
	{
		ActiveInspectionScreen->SetDollActor(ActiveDollActor);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Chapter1GameMode: DollActorClass is unset or failed to spawn; inspection falls back to UI-only."));
	}

	PC->bShowMouseCursor = true;
	PC->bEnableClickEvents = true;
	PC->bEnableMouseOverEvents = true;
	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(Mode);
}

FShiftConfig AChapter1GameMode::BuildRuntimeShiftConfig(int32 ShiftIdx) const
{
	FShiftConfig RuntimeCfg;
	if (Shifts.Num() == 0)
	{
		return RuntimeCfg;
	}

	const int32 SourceIdx = Shifts.IsValidIndex(ShiftIdx)
		? ShiftIdx
		: FMath::Max(0, Shifts.Num() - 1);
	RuntimeCfg = Shifts[SourceIdx];

	if (bUseEndlessRoguelikeRun)
	{
		RuntimeCfg.PoolCards = BuildRoguelikeCardPool(ShiftIdx);
		RuntimeCfg.K = ShiftIdx <= 0 ? RuntimeCfg.K : 1;
		RuntimeCfg.PassQuota = 0;
		RuntimeCfg.RejectQuota = 0;
		RuntimeCfg.MaxMisjudgmentsBeforeFail = 0;
		RuntimeCfg.bUsePieceworkEconomy = true;
		RuntimeCfg.DailyQuota = ComputeDailyQuota(RuntimeCfg, ShiftIdx);
		RuntimeCfg.DayDollTarget = ComputeDayDollTarget(RuntimeCfg, ShiftIdx);

		const int32 ExtraDays = FMath::Max(0, ShiftIdx - (Shifts.Num() - 1));
		if (ExtraDays > 0 && RuntimeCfg.DollTimeoutSec > 0.0f)
		{
			RuntimeCfg.DollTimeoutSec = FMath::Max(4.5f, RuntimeCfg.DollTimeoutSec - ExtraDays * 0.15f);
		}
	}
	else
	{
		RuntimeCfg.DailyQuota = RuntimeCfg.DailyQuota > 0
			? RuntimeCfg.DailyQuota
			: ComputeDailyQuota(RuntimeCfg, ShiftIdx);
		RuntimeCfg.DayDollTarget = RuntimeCfg.DayDollTarget > 0
			? RuntimeCfg.DayDollTarget
			: ComputeDayDollTarget(RuntimeCfg, ShiftIdx);
	}

	return RuntimeCfg;
}

TArray<UCardData*> AChapter1GameMode::BuildRoguelikeCardPool(int32 ShiftIdx) const
{
	TArray<UCardData*> Pool;
	const int32 MaxSourceShift = FMath::Min(ShiftIdx, Shifts.Num() - 1);
	for (int32 Index = 0; Index <= MaxSourceShift; ++Index)
	{
		if (!Shifts.IsValidIndex(Index))
		{
			continue;
		}
		for (UCardData* Card : Shifts[Index].PoolCards)
		{
			if (Card)
			{
				Pool.AddUnique(Card);
			}
		}
	}

	if (bUseEndlessRoguelikeRun && ShiftIdx >= Shifts.Num())
	{
		for (const FShiftConfig& Shift : Shifts)
		{
			for (UCardData* Card : Shift.PoolCards)
			{
				if (Card)
				{
					Pool.AddUnique(Card);
				}
			}
		}
	}

	if (Pool.Num() == 0)
	{
		for (const FShiftConfig& Shift : Shifts)
		{
			for (UCardData* Card : Shift.PoolCards)
			{
				if (Card)
				{
					Pool.AddUnique(Card);
				}
			}
		}
	}

	return Pool;
}

bool AChapter1GameMode::DoesDollPassActiveCards(const UDollData* Doll, const TArray<UCardData*>& ActiveCards) const
{
	if (!Doll)
	{
		return false;
	}

	FString FailReason;
	return UJudgmentEvaluator::EvaluateDoll(ActiveCards, Doll, FailReason) == EJudgmentVerdict::Pass;
}

TArray<UDollData*> AChapter1GameMode::BuildBalancedInspectionDollSequence(
	const TArray<UDollData*>& SourceSequence,
	const TArray<UCardData*>& ActiveCards,
	int32 TargetCount) const
{
	TArray<UDollData*> CleanSource;
	CleanSource.Reserve(SourceSequence.Num());
	for (UDollData* Doll : SourceSequence)
	{
		if (Doll)
		{
			CleanSource.Add(Doll);
		}
	}

	const int32 ResolvedTargetCount = TargetCount > 0 ? TargetCount : CleanSource.Num();
	if (!bUsePieceworkEconomyFlow
		|| !bRebalanceDollSequenceForPiecework
		|| CleanSource.Num() == 0
		|| ResolvedTargetCount <= 0)
	{
		return CleanSource;
	}

	TArray<UDollData*> ObjectivePassDolls;
	TArray<UDollData*> ObjectiveRejectDolls;
	for (UDollData* Doll : CleanSource)
	{
		if (DoesDollPassActiveCards(Doll, ActiveCards))
		{
			ObjectivePassDolls.Add(Doll);
		}
		else
		{
			ObjectiveRejectDolls.Add(Doll);
		}
	}

	if (ObjectivePassDolls.Num() == 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Chapter1: cannot rebalance dolls for day %d; no source doll passes the active criteria."),
			CurrentShiftIdx + 1);
		return CleanSource;
	}

	int32 DesiredPassCount = FMath::RoundToInt(ResolvedTargetCount * DesiredObjectivePassDollRatio);
	DesiredPassCount = FMath::Max(DesiredPassCount, FMath::Min(MinObjectivePassDollsPerDay, ResolvedTargetCount));
	if (ObjectiveRejectDolls.Num() > 0)
	{
		const int32 DesiredRejectFloor = FMath::Min(MinObjectiveRejectDollsPerDay, FMath::Max(0, ResolvedTargetCount - 1));
		DesiredPassCount = FMath::Min(DesiredPassCount, ResolvedTargetCount - DesiredRejectFloor);
	}
	DesiredPassCount = FMath::Clamp(DesiredPassCount, 1, ResolvedTargetCount);
	const int32 DesiredRejectCount = ResolvedTargetCount - DesiredPassCount;

	TArray<UDollData*> Result;
	Result.Reserve(ResolvedTargetCount);
	int32 PassCursor = CurrentShiftIdx % ObjectivePassDolls.Num();
	int32 RejectCursor = ObjectiveRejectDolls.Num() > 0 ? CurrentShiftIdx % ObjectiveRejectDolls.Num() : 0;
	int32 PassUsed = 0;
	int32 RejectUsed = 0;

	while (Result.Num() < ResolvedTargetCount)
	{
		const int32 PassRemaining = DesiredPassCount - PassUsed;
		const int32 RejectRemaining = DesiredRejectCount - RejectUsed;

		bool bTakePass = PassRemaining > 0;
		if (ObjectiveRejectDolls.Num() > 0 && PassRemaining > 0 && RejectRemaining > 0)
		{
			const float PassProgress = DesiredPassCount > 0
				? static_cast<float>(PassUsed) / static_cast<float>(DesiredPassCount)
				: 1.0f;
			const float RejectProgress = DesiredRejectCount > 0
				? static_cast<float>(RejectUsed) / static_cast<float>(DesiredRejectCount)
				: 1.0f;
			bTakePass = PassProgress <= RejectProgress;
		}

		if (bTakePass)
		{
			Result.Add(ObjectivePassDolls[PassCursor % ObjectivePassDolls.Num()]);
			++PassCursor;
			++PassUsed;
		}
		else if (ObjectiveRejectDolls.Num() > 0)
		{
			Result.Add(ObjectiveRejectDolls[RejectCursor % ObjectiveRejectDolls.Num()]);
			++RejectCursor;
			++RejectUsed;
		}
		else
		{
			Result.Add(ObjectivePassDolls[PassCursor % ObjectivePassDolls.Num()]);
			++PassCursor;
			++PassUsed;
		}
	}

	UE_LOG(LogTemp, Display,
		TEXT("Chapter1: balanced dolls day %d sourcePass=%d/%d finalPass=%d/%d targetRatio=%.2f"),
		CurrentShiftIdx + 1,
		ObjectivePassDolls.Num(),
		CleanSource.Num(),
		DesiredPassCount,
		ResolvedTargetCount,
		DesiredObjectivePassDollRatio);

	return Result;
}

int32 AChapter1GameMode::ComputeDailyQuota(const FShiftConfig& BaseCfg, int32 ShiftIdx) const
{
	if (BaseCfg.DailyQuota > 0 && (!bUseEndlessRoguelikeRun || Shifts.IsValidIndex(ShiftIdx)))
	{
		return BaseCfg.DailyQuota
			+ RunCardReplacementCount * ReplacementQuotaPressure
			+ RunQuotaFailureCount * QuotaFailurePressure
			+ FMath::RoundToInt(FMath::Max(0, RunTotalMoneyEarned) * MoneyQuotaPressureRate);
	}

	static const int32 BaseQuotas[] = { 0, 100, 180, 320, 460, 620, 820, 1050 };
	const int32 CurveIdx = FMath::Clamp(ShiftIdx, 0, static_cast<int32>(UE_ARRAY_COUNT(BaseQuotas)) - 1);
	int32 Quota = BaseQuotas[CurveIdx];
	if (ShiftIdx >= static_cast<int32>(UE_ARRAY_COUNT(BaseQuotas)))
	{
		const int32 ExtraDays = ShiftIdx - static_cast<int32>(UE_ARRAY_COUNT(BaseQuotas)) + 1;
		Quota = FMath::RoundToInt(BaseQuotas[UE_ARRAY_COUNT(BaseQuotas) - 1] * FMath::Pow(EndlessQuotaGrowthRate, ExtraDays))
			+ ExtraDays * EndlessQuotaLinearStep;
	}

	const int32 ReplacementPressure = RunCardReplacementCount * ReplacementQuotaPressure;
	const int32 FailurePressure = RunQuotaFailureCount * QuotaFailurePressure;
	const int32 MoneyPressure = FMath::RoundToInt(FMath::Max(0, RunTotalMoneyEarned) * MoneyQuotaPressureRate);
	return FMath::Max(0, Quota + ReplacementPressure + FailurePressure + MoneyPressure);
}

int32 AChapter1GameMode::ComputeDayDollTarget(const FShiftConfig& BaseCfg, int32 ShiftIdx) const
{
	if (BaseCfg.DayDollTarget > 0 && (!bUseEndlessRoguelikeRun || Shifts.IsValidIndex(ShiftIdx)))
	{
		return BaseCfg.DayDollTarget;
	}

	static const int32 BaseTargets[] = { 6, 8, 10, 12, 14, 15 };
	const int32 CurveIdx = FMath::Clamp(ShiftIdx, 0, static_cast<int32>(UE_ARRAY_COUNT(BaseTargets)) - 1);
	int32 Target = BaseTargets[CurveIdx];
	if (ShiftIdx >= static_cast<int32>(UE_ARRAY_COUNT(BaseTargets)))
	{
		const int32 ExtraDays = ShiftIdx - static_cast<int32>(UE_ARRAY_COUNT(BaseTargets)) + 1;
		Target += ExtraDays / 2;
	}
	return FMath::Clamp(Target, 1, MaxEndlessDollTarget);
}

TArray<UCardData*> AChapter1GameMode::BuildDraftPoolForShift(const FShiftConfig& Cfg) const
{
	TArray<UCardData*> DraftPool;
	for (UCardData* Card : Cfg.PoolCards)
	{
		if (Card && !CurrentDeckCards.Contains(Card))
		{
			DraftPool.Add(Card);
		}
	}

	if (DraftPool.Num() == 0)
	{
		for (UCardData* Card : Cfg.PoolCards)
		{
			if (Card)
			{
				DraftPool.Add(Card);
			}
		}
	}

	return DraftPool;
}

void AChapter1GameMode::HandleShiftCompleted(FShiftResult Result)
{
	UE_LOG(LogTemp, Display, TEXT("Chapter1: Shift %d %s — TP=%d TN=%d 误判=%d 总=%d"),
		CurrentShiftIdx + 1, Result.bSuccess ? TEXT("成功") : TEXT("失败"),
		Result.TrueAcceptCount, Result.TrueRejectCount, Result.WrongCount, Result.TotalDolls);

	if (ActiveInspectionScreen)
	{
		ActiveInspectionScreen->OnShiftCompleted.RemoveAll(this);
		ActiveInspectionScreen->OnMisjudgmentRecorded.RemoveAll(this);
		ActiveInspectionScreen->RemoveFromParent();
		ActiveInspectionScreen = nullptr;
	}
	bInspectionLaunchInProgress = false;
	RunTotalMoneyEarned += FMath::Max(0, Result.MoneyEarned);
	if (!Result.bSuccess)
	{
		++RunQuotaFailureCount;
	}
	const bool bRunMoneyGoalMet = bAdvanceToChapter2OnMoneyGoal
		&& RunMoneyToChapter2Threshold > 0
		&& RunTotalMoneyEarned >= RunMoneyToChapter2Threshold;
	const bool bCheckmateBurstMet = bAdvanceToChapter2OnCheckmateBurst
		&& HasSelectedCheckmateCard()
		&& Result.MoneyEarned >= CheckmateBurstDayMoneyThreshold;

	// 班次失败现在进入死亡分支，不再回选卡屏重试。
	if (!Result.bSuccess)
	if (!Result.bSuccess)
	{
		if (bUsePieceworkEconomyFlow && bCheckmateBurstMet)
		{
			UE_LOG(LogTemp, Display,
				TEXT("Chapter1: checkmate burst advances to Ch2 after quota miss. DayMoney=%d Threshold=%d RunMoney=%d"),
				Result.MoneyEarned, CheckmateBurstDayMoneyThreshold, RunTotalMoneyEarned);
			TriggerChapter2Transition(FName("Checkmate"), /*bQuotaMet=*/false);
			return;
		}

		if (bUsePieceworkEconomyFlow && bAdvanceToChapter2OnQuotaMiss)
		{
			UE_LOG(LogTemp, Display,
				TEXT("Chapter1: quota miss advances to Ch2. DayMoney=%d Quota=%d RunMoney=%d Goal=%d"),
				Result.MoneyEarned, Result.DailyQuota, RunTotalMoneyEarned, RunMoneyToChapter2Threshold);
			TriggerChapter2Transition(FName("QuotaMiss"), /*bQuotaMet=*/false);
			return;
		}

		if (bUseEndlessRoguelikeRun)
		{
			const int32 NextIdx = CurrentShiftIdx + 1;
			UE_LOG(LogTemp, Display,
				TEXT("Chapter1: quota missed $%d / $%d; endless run continues to day %d. failurePressure=%d"),
				Result.MoneyEarned, Result.DailyQuota, NextIdx + 1, RunQuotaFailureCount);
			UAudioService::PlayCueStatic(this, FName("Ch1.Wrong"), 0.55f);
			ShowShiftTransition(NextIdx);
			return;
		}

		if (bUsePieceworkEconomyFlow && !bQuotaCollapseActive)
		{
			if (bRequireBridgeCardBeforeCollapse && !HasSelectedBridgeCard())
			{
				const int32 NextIdx = CurrentShiftIdx + 1;
				UE_LOG(LogTemp, Warning,
					TEXT("Chapter1: quota failed before any bridge card was selected; continuing to shift %d instead of entering Ch2."),
					NextIdx + 1);
				if (Shifts.IsValidIndex(NextIdx))
				{
					UAudioService::PlayCueStatic(this, FName("Ch1.ShiftPass"));
					ShowShiftTransition(NextIdx);
					return;
				}
			}

			UE_LOG(LogTemp, Display, TEXT("Chapter1: quota collapse $%d / $%d; forcing D05 verdict"),
				Result.MoneyEarned, Result.DailyQuota);
			StartQuotaCollapseInspection();
			return;
		}

		TriggerDeathCinematic();
		return;
	}

	if (bDeathCinematicTriggered)
	{
		return;
	}

	if (bUsePieceworkEconomyFlow && bRunMoneyGoalMet)
	{
		UE_LOG(LogTemp, Display,
			TEXT("Chapter1: money goal advances to Ch2. RunMoney=%d Goal=%d"),
			RunTotalMoneyEarned, RunMoneyToChapter2Threshold);
		TriggerChapter2Transition(FName("MoneyGoal"), /*bQuotaMet=*/true);
		return;
	}
	if (bUsePieceworkEconomyFlow && bCheckmateBurstMet)
	{
		UE_LOG(LogTemp, Display,
			TEXT("Chapter1: checkmate burst advances to Ch2. DayMoney=%d Threshold=%d RunMoney=%d"),
			Result.MoneyEarned, CheckmateBurstDayMoneyThreshold, RunTotalMoneyEarned);
		TriggerChapter2Transition(FName("Checkmate"), /*bQuotaMet=*/Result.bQuotaMet);
		return;
	}

	const int32 NextIdx = CurrentShiftIdx + 1;
	if (Shifts.IsValidIndex(NextIdx) || bUseEndlessRoguelikeRun)
	{
		UAudioService::PlayCueStatic(this, FName("Ch1.ShiftPass"));
		ShowShiftTransition(NextIdx);
	}
	else
	{
		FinishCh1();
	}
}

void AChapter1GameMode::HandleMisjudgmentRecorded(int32 ShiftMisjudgments, int32 ShiftWrongCount)
{
	if (bDeathCinematicTriggered || bTwistTriggered)
	{
		return;
	}

	TotalMisjudgmentsThisChapter++;
	UE_LOG(LogTemp, Display, TEXT("Chapter1: Misjudgment total=%d shift=%d wrong=%d threshold=%d"),
		TotalMisjudgmentsThisChapter, ShiftMisjudgments, ShiftWrongCount, ChapterDeathMisjudgmentThreshold);

	if (bUsePieceworkEconomyFlow)
	{
		return;
	}

	if (ChapterDeathMisjudgmentThreshold > 0
		&& TotalMisjudgmentsThisChapter >= ChapterDeathMisjudgmentThreshold)
	{
		TriggerDeathCinematic();
	}
}

bool AChapter1GameMode::TryPlaySequence(FName Key)
{
	if (Key.IsNone())
	{
		return false;
	}

	const TSoftObjectPtr<ULevelSequence>* SequencePtr = NamedSequences.Find(Key);
	if (!SequencePtr || SequencePtr->IsNull())
	{
		return false;
	}

	ULevelSequence* Sequence = SequencePtr->LoadSynchronous();
	if (!Sequence || !GetWorld())
	{
		return false;
	}

	FMovieSceneSequencePlaybackSettings Settings;
	Settings.bAutoPlay = false;
	ActiveSequenceActor = nullptr;
	ActiveSequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
		GetWorld(),
		Sequence,
		Settings,
		ActiveSequenceActor);

	if (!ActiveSequencePlayer)
	{
		return false;
	}

	ActiveSequencePlayer->Play();
	UE_LOG(LogTemp, Display, TEXT("Chapter1: Playing sequence key=%s asset=%s"),
		*Key.ToString(), *Sequence->GetPathName());
	return true;
}

void AChapter1GameMode::TriggerDeathCinematic()
{
	if (bDeathCinematicTriggered || bTwistTriggered)
	{
		return;
	}

	bDeathCinematicTriggered = true;
	UE_LOG(LogTemp, Display, TEXT("Chapter1: ★ Death cinematic triggered after %d misjudgments"),
		TotalMisjudgmentsThisChapter);

	GetWorldTimerManager().ClearTimer(ProgressPanelTimer);
	GetWorldTimerManager().ClearTimer(TwistHoldTimer);
	GetWorldTimerManager().ClearTimer(TwistOpticalBurnoutTimer);

	if (ActiveCardScreen)
	{
		ActiveCardScreen->OnAssemblyComplete.RemoveAll(this);
		ActiveCardScreen->RemoveFromParent();
		ActiveCardScreen = nullptr;
	}

	if (ActiveInspectionScreen)
	{
		ActiveInspectionScreen->OnShiftCompleted.RemoveAll(this);
		ActiveInspectionScreen->OnMisjudgmentRecorded.RemoveAll(this);
		ActiveInspectionScreen->TriggerOpticalInversionSurge(DeathHandCoverSeconds);
	}

	if (ActiveTransitionScreen)
	{
		ActiveTransitionScreen->RemoveFromParent();
		ActiveTransitionScreen = nullptr;
	}

	TriggerPresentationCue(FName("Ch1.Death"), CurrentShiftIdx + 1, ECh1ProgressPanelMoment::TwistLeadIn, true);
	UAudioService::PlayCueStatic(this, FName("Ch1.Wrong"), 1.15f);
	UAudioService::PlayCueStatic(this, FName("Ch1.Toss"), 0.95f);
	K2_OnDeathCinematicRequested(TotalMisjudgmentsThisChapter);

	const bool bPlayedSequence = TryPlaySequence(DeathSequenceKey);
	if (!bPlayedSequence)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			PC->bShowMouseCursor = false;
			PC->SetIgnoreMoveInput(true);
			PC->SetIgnoreLookInput(true);
			PC->PlayerCameraManager->StartCameraFade(
				0.0f, 1.0f, DeathHandCoverSeconds, FLinearColor::Black,
				/*bShouldFadeAudio=*/false, /*bHoldWhenFinished=*/true);
		}

		GetWorldTimerManager().SetTimer(
			DeathFallbackTimer,
			FTimerDelegate::CreateUObject(this, &AChapter1GameMode::PlayDeathButtonDropFallback),
			FMath::Max(0.05f, DeathHandCoverSeconds),
			false);
	}
}

void AChapter1GameMode::PlayDeathButtonDropFallback()
{
	UAudioService::PlayCueStatic(this, FName("Twist.PearlEye"), 0.8f);
	if (ActiveInspectionScreen)
	{
		ActiveInspectionScreen->PlayTwistOpticalInversionBurnout(0.85f);
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->PlayerCameraManager->StartCameraFade(
			1.0f, 0.35f, 0.35f, FLinearColor::Black,
			/*bShouldFadeAudio=*/false, /*bHoldWhenFinished=*/true);
	}

	UE_LOG(LogTemp, Display,
		TEXT("Chapter1: Death fallback beat reached. Bind NamedSequences[Ch1.Death] for hand cover, button drop, conveyor disposal."));
}

void AChapter1GameMode::ShowShiftTransition(int32 NextShiftIdx)
{
	if (bUseNativeProgressPanelFallback || ProgressPanelWidgetClass || !ShiftTransitionWidgetClass)
	{
		ShowShiftIntro(NextShiftIdx);
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	if (ShiftTransitionWidgetClass)
	{
		ActiveTransitionScreen = CreateWidget<UUserWidget>(PC, ShiftTransitionWidgetClass);
		if (ActiveTransitionScreen)
		{
			if (UTextBlock* Title = Cast<UTextBlock>(ActiveTransitionScreen->GetWidgetFromName(TEXT("TitleText"))))
			{
				FText TitleText;
				if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
				{
					if (UCh1LocSubsystem* Loc = GI->GetSubsystem<UCh1LocSubsystem>())
					{
						TitleText = FText::Format(Loc->Get(TEXT("Shift.Title")), FText::AsNumber(NextShiftIdx + 1));
					}
				}
				if (TitleText.IsEmpty())
				{
					TitleText = FText::FromString(FString::Printf(TEXT("班次 %d"), NextShiftIdx + 1));
				}
				Title->SetText(TitleText);
			}
			ActiveTransitionScreen->AddToViewport(/*ZOrder=*/100);
			SetUIInputMode();
		}
	}

	// TransitionHoldSeconds 后切下一班
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(
		TimerHandle,
		FTimerDelegate::CreateLambda([this, NextShiftIdx]()
		{
			CurrentShiftIdx = NextShiftIdx;
			BeginShift(CurrentShiftIdx);
		}),
		FMath::Max(0.5f, TransitionHoldSeconds), false);
}

void AChapter1GameMode::StartQuotaCollapseInspection()
{
	if (bTwistTriggered)
	{
		return;
	}

	bQuotaCollapseActive = true;
	UDollData* CollapseDoll = ForcedCollapseDoll ? ForcedCollapseDoll : FindFallbackCollapseDoll();
	if (!CollapseDoll)
	{
		UE_LOG(LogTemp, Warning, TEXT("Chapter1: no collapse doll configured; triggering twist directly"));
		RequestTwist();
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !InspectionWidgetClass)
	{
		RequestTwist();
		return;
	}

	TriggerPresentationCue(FName("Ch1.PanelTwistLeadIn"), CurrentShiftIdx + 1, ECh1ProgressPanelMoment::TwistLeadIn, true);

	TArray<UDollData*> CollapseSequence;
	CollapseSequence.Add(CollapseDoll);

	ActiveInspectionScreen = CreateWidget<UInspectionScreen>(PC, InspectionWidgetClass);
	if (!ActiveInspectionScreen)
	{
		RequestTwist();
		return;
	}

	ActiveInspectionScreen->SetShiftData(LastSelectedCards, CollapseSequence);
	ActiveInspectionScreen->DollTimeoutSec = 0.0f;
	ActiveInspectionScreen->CorrectGoal = 1;
	ActiveInspectionScreen->PassQuota = 0;
	ActiveInspectionScreen->RejectQuota = 0;
	ActiveInspectionScreen->MaxMisjudgmentsBeforeFail = 0;
	ActiveInspectionScreen->bUsePieceworkEconomy = false;
	ActiveInspectionScreen->ShiftNumber = CurrentShiftIdx + 1;
	ActiveInspectionScreen->bTwistOnAnyVerdictForLastDoll = true;
	ActiveInspectionScreen->OnShiftCompleted.AddDynamic(this, &AChapter1GameMode::HandleShiftCompleted);
	ActiveInspectionScreen->OnMisjudgmentRecorded.AddDynamic(this, &AChapter1GameMode::HandleMisjudgmentRecorded);
	ActiveInspectionScreen->AddToViewport();

	if (DollActorClass && !ActiveDollActor)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ActiveDollActor = GetWorld()->SpawnActor<ADollDisplay>(
			DollActorClass, DollSpawnTransform, Params);
	}
	if (ActiveDollActor)
	{
		ActiveInspectionScreen->SetDollActor(ActiveDollActor);
	}

	PC->bShowMouseCursor = true;
	PC->bEnableClickEvents = true;
	PC->bEnableMouseOverEvents = true;
	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(Mode);
}

UDollData* AChapter1GameMode::FindFallbackCollapseDoll() const
{
	for (const FShiftConfig& Shift : Shifts)
	{
		for (UDollData* Doll : Shift.DollSequence)
		{
			if (Doll && Doll->bIsTwistTrigger)
			{
				return Doll;
			}
		}
	}

	for (const FShiftConfig& Shift : Shifts)
	{
		for (UDollData* Doll : Shift.DollSequence)
		{
			if (Doll && Doll->ButtonStyle == EButtonEyeStyle::Pearl)
			{
				return Doll;
			}
		}
	}

	return nullptr;
}

bool AChapter1GameMode::HasSelectedBridgeCard() const
{
	for (const UCardData* Card : LastSelectedCards)
	{
		if (!Card)
		{
			continue;
		}

		if (Card->PieceworkEffect == ECh1CardEffect::SwordAndSerpent
			|| Card->PieceworkEffect == ECh1CardEffect::QueenGambit
			|| Card->PieceworkEffect == ECh1CardEffect::CheckmateDiscard)
		{
			return true;
		}
	}
	return false;
}

bool AChapter1GameMode::HasSelectedCheckmateCard() const
{
	for (const UCardData* Card : LastSelectedCards)
	{
		if (Card && Card->PieceworkEffect == ECh1CardEffect::CheckmateDiscard)
		{
			return true;
		}
	}
	return false;
}

void AChapter1GameMode::FinishCh1()
{
	UE_LOG(LogTemp, Display, TEXT("Chapter1: 全部 %d 班完成 — Ch1 结束"), Shifts.Num());
	// 走完所有班次仍未触发 twist（玩家从未放行 Pearl 完美娃娃）→ 兜底也跳 Ch2
	if (!bTwistTriggered && (!bRequireBridgeCardBeforeCollapse || HasSelectedBridgeCard()))
	{
		RequestTwist();
	}
	else if (!bTwistTriggered)
	{
		UE_LOG(LogTemp, Warning, TEXT("Chapter1: finished without a selected bridge card; Ch2 transition is blocked by bRequireBridgeCardBeforeCollapse."));
	}
}

void AChapter1GameMode::TriggerChapter2Transition(FName Reason, bool bQuotaMet)
{
	if (bTwistTriggered)
	{
		return;
	}

	PendingChapter2TransitionReason = Reason.IsNone() ? FName("Chapter2") : Reason;
	K2_OnChapter2TransitionReason(PendingChapter2TransitionReason, RunTotalMoneyEarned, RunMoneyToChapter2Threshold, bQuotaMet);

	bForceAllowChapter2Transition = true;
	RequestTwist();
	bForceAllowChapter2Transition = false;
	PendingChapter2TransitionReason = NAME_None;
}

void AChapter1GameMode::RequestTwist()
{
	if (bTwistTriggered)
	{
		return;
	}
	if (!bForceAllowChapter2Transition && bUseEndlessRoguelikeRun && bBlockAutomaticTwistInEndlessRun)
	{
		UE_LOG(LogTemp, Display, TEXT("Chapter1: automatic Ch2 twist blocked by endless roguelike run mode."));
		return;
	}
	if (!bForceAllowChapter2Transition && bRequireBridgeCardBeforeCollapse && !HasSelectedBridgeCard())
	{
		UE_LOG(LogTemp, Warning, TEXT("Chapter1: Ch2 transition blocked; no bridge card has been selected yet."));
		return;
	}

	bTwistTriggered = true;

	UE_LOG(LogTemp, Display, TEXT("Chapter1: ★ Twist triggered — Ch1→Ch2 reason=%s"),
		PendingChapter2TransitionReason.IsNone() ? TEXT("Default") : *PendingChapter2TransitionReason.ToString());

	// 音频：翻转专属 cue；没绑 Native/FMOD 时 AudioService 会静默 fallback。
	FName LeadInCue = FName("Ch1.PanelTwistLeadIn");
	if (PendingChapter2TransitionReason == FName("QuotaMiss"))
	{
		LeadInCue = FName("Ch1.QuotaMissToCh2");
	}
	else if (PendingChapter2TransitionReason == FName("MoneyGoal"))
	{
		LeadInCue = FName("Ch1.MoneyGoalToCh2");
	}
	TriggerPresentationCue(LeadInCue, CurrentShiftIdx + 1, ECh1ProgressPanelMoment::TwistLeadIn, true);
	UAudioService::PlayCueStatic(this, FName("Twist.DroneAscent"), 0.8f);
	UAudioService::PlayCueStatic(this, FName("Twist.PearlEye"));

	// Ch1 optical inversion：先让按扣边缘变实，再进入 EyeFall burnout。
	if (ActiveInspectionScreen)
	{
		ActiveInspectionScreen->TriggerOpticalInversionSurge(1.0f);
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				TwistOpticalBurnoutTimer,
				FTimerDelegate::CreateUObject(this, &AChapter1GameMode::PlayTwistOpticalBurnout),
				1.0f, false);
		}
	}

	// Fade-to-white：用 PlayerController CameraFade
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->PlayerCameraManager->StartCameraFade(
			0.0f, 1.0f, TwistFadeSeconds, FLinearColor::White,
			/*bShouldFadeAudio=*/false, /*bHoldWhenFinished=*/true);
	}

	// 优先播 Ch2.Intro sequence（如果 BP 里配了）
	// 不阻塞——HoldTimer 到时直接 OpenLevel，无 sequence 也 OK

	// HoldTimer：fade + hold 后跳 Ch2
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			TwistHoldTimer,
			FTimerDelegate::CreateUObject(this, &AChapter1GameMode::OpenCh2Map),
			TwistFadeSeconds + TwistHoldSeconds, false);
	}
}

void AChapter1GameMode::PlayTwistOpticalBurnout()
{
	UAudioService::PlayCueStatic(this, FName("Twist.MechanicalEye"));
	if (ActiveInspectionScreen)
	{
		ActiveInspectionScreen->PlayTwistOpticalInversionBurnout(TwistFadeSeconds);
	}
}

void AChapter1GameMode::OpenCh2Map()
{
	UE_LOG(LogTemp, Display, TEXT("Chapter1: OpenLevel(%s)"), *Ch2MapName.ToString());
	UGameplayStatics::OpenLevel(this, Ch2MapName);
}

void AChapter1GameMode::PV_Ch1Preset(FName PresetId)
{
#if UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Warning, TEXT("PV_CH1_PRESET ignored in shipping build"));
#else
	const FString Id = PresetId.ToString().ToLower();
	UE_LOG(LogTemp, Display, TEXT("PV_CH1_PRESET PresetId=%s Shift=%d"), *PresetId.ToString(), CurrentShiftIdx + 1);

	if (Id == TEXT("a7") || Id == TEXT("dolllook") || Id == TEXT("lookat"))
	{
		PV_TriggerDollLookAtCamera(0.5f);
		return;
	}

	if (Id == TEXT("a8") || Id == TEXT("buttonedge") || Id == TEXT("edge"))
	{
		PV_SetEdgeOpacity(0.5f, 0.5f);
		return;
	}

	if (Id == TEXT("a13") || Id == TEXT("finalpearl") || Id == TEXT("fallback"))
	{
		PV_TriggerFinalPearl(true);
		return;
	}

	if (Id == TEXT("twist") || Id == TEXT("b1"))
	{
		PV_TriggerTwistLeadIn();
		return;
	}

	if (Id == TEXT("shift2transition") || Id == TEXT("a9"))
	{
		ShowShiftTransition(/*NextShiftIdx=*/1);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("PV_CH1_PRESET unknown PresetId=%s"), *PresetId.ToString());
#endif
}

void AChapter1GameMode::PV_SetEdgeOpacity(float Opacity, float DurationSec)
{
#if UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Warning, TEXT("PV_CH1_EDGE_OVERRIDE ignored in shipping build"));
#else
	if (!ActiveInspectionScreen)
	{
		UE_LOG(LogTemp, Warning, TEXT("PV_CH1_EDGE_OVERRIDE failed: no active inspection screen"));
		return;
	}

	ActiveInspectionScreen->SetOpticalInversionEdgeOverride(Opacity, DurationSec, 0.20f);
#endif
}

void AChapter1GameMode::PV_TriggerDollLookAtCamera(float HoldSeconds)
{
#if UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Warning, TEXT("PV_CH1_A7_LOOKAT ignored in shipping build"));
#else
	if (!ActiveDollActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("PV_CH1_A7_LOOKAT failed: no active doll actor"));
		return;
	}

	ActiveDollActor->TriggerLookAtCamera(HoldSeconds);
#endif
}

void AChapter1GameMode::PV_TriggerFinalPearl(bool bFallback)
{
#if UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Warning, TEXT("PV_CH1_FINAL_PEARL ignored in shipping build"));
#else
	if (!ActiveInspectionScreen)
	{
		UE_LOG(LogTemp, Warning, TEXT("PV_CH1_FINAL_PEARL failed: no active inspection screen"));
		return;
	}

	const bool bSet = ActiveInspectionScreen->PV_SetCurrentDollToTwistTrigger(bFallback);
	UE_LOG(LogTemp, Display, TEXT("PV_CH1_FINAL_PEARL Fallback=%s Result=%s"),
		bFallback ? TEXT("true") : TEXT("false"),
		bSet ? TEXT("ok") : TEXT("failed"));
#endif
}

void AChapter1GameMode::PV_TriggerTwistLeadIn()
{
#if UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Warning, TEXT("PV_CH1_TWIST ignored in shipping build"));
#else
	UE_LOG(LogTemp, Display, TEXT("PV_CH1_TWIST_LEADIN"));
	RequestTwist();
#endif
}

void AChapter1GameMode::SetUIInputMode()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	PC->bShowMouseCursor = true;
	PC->bEnableClickEvents = true;
	PC->bEnableMouseOverEvents = true;

	FInputModeUIOnly Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(Mode);
}
