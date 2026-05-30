// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "Chapter1GameMode.h"

#include "AudioService.h"
#include "CardData.h"
#include "CardSelectionScreen.h"
#include "Ch1LocSubsystem.h"
#include "DollData.h"
#include "DollDisplay.h"
#include "InspectionScreen.h"

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
	Payload.TotalShifts = FMath::Max(Shifts.Num(), 1);
	Payload.Moment = Moment;

	const FShiftConfig* Cfg = Shifts.IsValidIndex(ShiftIdx) ? &Shifts[ShiftIdx] : nullptr;
	if (Cfg && !Cfg->PanelEyebrow.IsEmpty())
	{
		Payload.Eyebrow = Cfg->PanelEyebrow;
	}
	else
	{
		Payload.Eyebrow = FText::FromString(FString::Printf(TEXT("DAY %02d / FACTORY FLOOR"), Payload.ShiftNumber));
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
			Payload.Title = FText::FromString(FString::Printf(TEXT("SHIFT %02d"), Payload.ShiftNumber));
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
	if (!Shifts.IsValidIndex(ShiftIdx))
	{
		FinishCh1();
		return;
	}

	const FShiftConfig& Cfg = Shifts[ShiftIdx];

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	// 清掉过场（如有）
	if (ActiveTransitionScreen)
	{
		ActiveTransitionScreen->RemoveFromParent();
		ActiveTransitionScreen = nullptr;
	}

	ActiveCardScreen = CreateWidget<UCardSelectionScreen>(PC, CardSelectionWidgetClass);
	if (!ActiveCardScreen)
	{
		UE_LOG(LogTemp, Error, TEXT("Chapter1GameMode: CreateWidget CardSelectionScreen 失败"));
		return;
	}

	ActiveCardScreen->SetShiftConfig(Cfg.PoolCards, Cfg.K, Cfg.AssemblyTimerSec);
	ActiveCardScreen->OnAssemblyComplete.AddDynamic(this, &AChapter1GameMode::HandleCardsAssembled);
	ActiveCardScreen->AddToViewport();

	SetUIInputMode();

	UE_LOG(LogTemp, Display, TEXT("Chapter1: Shift %d 开始 — Pool=%d, K=%d, DollTimeout=%.1fs"),
		ShiftIdx + 1, Cfg.PoolCards.Num(), Cfg.K, Cfg.DollTimeoutSec);
}

void AChapter1GameMode::HandleCardsAssembled(const TArray<UCardData*>& SelectedCards)
{
	const FShiftConfig& Cfg = Shifts[CurrentShiftIdx];

	UE_LOG(LogTemp, Display, TEXT("Chapter1: Shift %d 卡选完成（%d 张）→ 进检验"),
		CurrentShiftIdx + 1, SelectedCards.Num());

	if (ActiveCardScreen)
	{
		ActiveCardScreen->OnAssemblyComplete.RemoveAll(this);
		ActiveCardScreen->RemoveFromParent();
		ActiveCardScreen = nullptr;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	ActiveInspectionScreen = CreateWidget<UInspectionScreen>(PC, InspectionWidgetClass);
	if (!ActiveInspectionScreen) return;

	ActiveInspectionScreen->SetShiftData(SelectedCards, Cfg.DollSequence);
	ActiveInspectionScreen->DollTimeoutSec = Cfg.DollTimeoutSec;
	ActiveInspectionScreen->CorrectGoal = Cfg.CorrectGoal;
	ActiveInspectionScreen->PassQuota = Cfg.PassQuota;
	ActiveInspectionScreen->RejectQuota = Cfg.RejectQuota;
	ActiveInspectionScreen->MaxMisjudgmentsBeforeFail = Cfg.MaxMisjudgmentsBeforeFail;
	ActiveInspectionScreen->OnShiftCompleted.AddDynamic(this, &AChapter1GameMode::HandleShiftCompleted);
	ActiveInspectionScreen->OnMisjudgmentRecorded.AddDynamic(this, &AChapter1GameMode::HandleMisjudgmentRecorded);
	ActiveInspectionScreen->AddToViewport();

	// Spawn 3D 娃娃 actor（一班一只，复用到下一班）
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
			TEXT("Chapter1GameMode: DollActorClass 未填或 spawn 失败——检验屏会缺 3D 交互"));
	}

	// 3D 拖拽需要同时收键鼠和 UI 事件
	APlayerController* PC2 = UGameplayStatics::GetPlayerController(this, 0);
	if (PC2)
	{
		PC2->bShowMouseCursor = true;
		PC2->bEnableClickEvents = true;
		PC2->bEnableMouseOverEvents = true;
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC2->SetInputMode(Mode);
	}
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

	// 班次失败现在进入死亡分支，不再回选卡屏重试。
	if (!Result.bSuccess)
	{
		TriggerDeathCinematic();
		return;
	}

	if (bDeathCinematicTriggered)
	{
		return;
	}

	const int32 NextIdx = CurrentShiftIdx + 1;
	if (Shifts.IsValidIndex(NextIdx))
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

void AChapter1GameMode::FinishCh1()
{
	UE_LOG(LogTemp, Display, TEXT("Chapter1: 全部 %d 班完成 — Ch1 结束"), Shifts.Num());
	// 走完所有班次仍未触发 twist（玩家从未放行 Pearl 完美娃娃）→ 兜底也跳 Ch2
	if (!bTwistTriggered)
	{
		RequestTwist();
	}
}

void AChapter1GameMode::RequestTwist()
{
	if (bTwistTriggered) return;  // 防重入
	bTwistTriggered = true;

	UE_LOG(LogTemp, Display, TEXT("Chapter1: ★ Twist triggered — Ch1→Ch2 翻转拍点"));

	// 音频：翻转专属 cue；没绑 Native/FMOD 时 AudioService 会静默 fallback。
	TriggerPresentationCue(FName("Ch1.PanelTwistLeadIn"), CurrentShiftIdx + 1, ECh1ProgressPanelMoment::TwistLeadIn, true);
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
