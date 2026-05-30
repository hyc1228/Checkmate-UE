// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "InspectionScreen.h"

#include "AudioService.h"
#include "CardData.h"
#include "Ch1LocSubsystem.h"
#include "Chapter1GameMode.h"
#include "DollData.h"
#include "DollDisplay.h"
#include "JudgmentEvaluator.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/Scene.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
	FString DescribeCard(const UCardData* Card)
	{
		if (!Card) return TEXT("(null)");
		const FString Label = Card->DisplayLabel.IsEmpty() ? Card->CardId.ToString() : Card->DisplayLabel.ToString();

		const TCHAR* DimTag = TEXT("");
		switch (Card->Dimension)
		{
			case ECardDimension::Posture:    DimTag = TEXT("姿态"); break;
			case ECardDimension::Hair:       DimTag = TEXT("发型"); break;
			case ECardDimension::Expression: DimTag = TEXT("表情"); break;
			case ECardDimension::Accessory:  DimTag = TEXT("饰物"); break;
		}
		return FString::Printf(TEXT("[%s] %s"), DimTag, *Label);
	}

	FString JoinTraitSet(const TSet<FName>& Traits)
	{
		if (Traits.Num() == 0) return TEXT("(无)");
		TArray<FString> Parts;
		Parts.Reserve(Traits.Num());
		for (const FName& T : Traits) Parts.Add(T.ToString());
		return FString::Join(Parts, TEXT(", "));
	}
}

void UInspectionScreen::SetShiftData(const TArray<UCardData*>& InJudgmentCards, const TArray<UDollData*>& InDollSequence)
{
	JudgmentCards = InJudgmentCards;
	DollSequence = InDollSequence;
	CurrentDollIndex = 0;
	CorrectCount = 0;
	WrongCount = 0;
	TrueAcceptCount = 0;
	TrueRejectCount = 0;
	MisjudgmentCount = 0;
	bHasDriftedFalsePos = false;
	bHasDriftedFalseNeg = false;
	bAwaitingNext = false;
	bShiftEnded = false;
	OpticalInversionMode = ECh1OpticalInversionMode::AmbientButtonEdge;
	OpticalEdgeOpacityT = OpticalTargetEdgeOpacityT = OpticalBaseOpacity;
	OpticalEdgeRadiusT = OpticalTargetEdgeRadiusT = 0.0f;
	OpticalPulseT = OpticalTargetPulseT = 0.0f;
	OpticalInvertT = OpticalTargetInvertT = 0.0f;
	OpticalBurnoutT = OpticalTargetBurnoutT = 0.0f;
	OpticalMechanicalFadeT = OpticalTargetMechanicalFadeT = 0.0f;
	OpticalSurgeRemainingSec = 0.0f;
	OpticalSurgeTotalSec = 0.0f;
	OpticalBurnoutRemainingSec = 0.0f;
	OpticalBurnoutTotalSec = 0.0f;
	OpticalFadeRemainingSec = 0.0f;
	OpticalFadeTotalSec = 0.0f;

	SpawnDeskCards();
}

void UInspectionScreen::SetDollActor(ADollDisplay* InActor)
{
	DollActor = InActor;
	if (DollActor)
	{
		DollActor->SetOwningScreen(this);
		PushCurrentDollToActor();
	}
}

void UInspectionScreen::PushCurrentDollToActor()
{
	if (!DollActor || DollSequence.Num() == 0) return;
	const int32 PoolIdx = CurrentDollIndex % DollSequence.Num();
	DollActor->ApplyDollData(DollSequence[PoolIdx]);
}

void UInspectionScreen::HandleGestureFromDoll(bool bConfirm)
{
	if (bAwaitingNext) return;
	// bConfirm == true → 玩家选择放行；false → 丢弃
	HandlePlayerChoice(/*bPlayerChosePass=*/bConfirm);
}

void UInspectionScreen::OnDollAnimComplete()
{
	// 动画结束 = 推进时刻
	AdvanceToNextDoll();
}

void UInspectionScreen::PlayTossActionFeedback()
{
	// 单纯震屏，不带 flash（flash 由 HandlePlayerChoice 触发，颜色取决于对错）
	ShakeElapsed = 0.0f;
	ShakeTotal = ShakeDurationSec * 0.7f;
	ShakeAmplitude = WrongShakeAmplitude * 0.6f;
}

void UInspectionScreen::PlayStampImpactFeedback()
{
	// 印章砸下：强震，模拟「啪嗒」一下
	ShakeElapsed = 0.0f;
	ShakeTotal = ShakeDurationSec * 1.2f;
	ShakeAmplitude = WrongShakeAmplitude * 1.3f;
}

void UInspectionScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (PassButton)
	{
		PassButton->OnClicked.AddDynamic(this, &UInspectionScreen::OnPassClicked);
	}
	if (RejectButton)
	{
		RejectButton->OnClicked.AddDynamic(this, &UInspectionScreen::OnRejectClicked);
	}

	if (ToastText)
	{
		ToastText->SetText(FText::GetEmpty());
	}

	// FlashOverlay 初始透明
	if (FlashOverlay)
	{
		FlashOverlay->SetBrushColor(FLinearColor(0, 0, 0, 0));
		FlashOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (LangToggleButton)
	{
		LangToggleButton->OnClicked.AddDynamic(this, &UInspectionScreen::OnLangToggleClicked);
	}
	if (UCh1LocSubsystem* Loc = GetLoc())
	{
		LangChangedHandle = Loc->OnLanguageChanged.AddUObject(this, &UInspectionScreen::RefreshLocalizedTexts);
	}

	RenderJudgmentCardsList();
	RenderCurrentDoll();
	RenderProgress();
	RefreshLocalizedTexts();

	// 启动第一只娃娃的 timeout（若配置）
	if (DollTimeoutSec > 0.0f)
	{
		CurrentDollTimeRemaining = DollTimeoutSec;
		GetWorld()->GetTimerManager().SetTimer(
			DollTimeoutHandle,
			FTimerDelegate::CreateUObject(this, &UInspectionScreen::HandleDollTimeout),
			DollTimeoutSec, false);
	}
}

void UInspectionScreen::NativeDestruct()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AdvanceTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(DollTimeoutHandle);
	}

	ClearDeskCards();

	if (MisjudgmentPPVolume)
	{
		MisjudgmentPPVolume->Destroy();
		MisjudgmentPPVolume = nullptr;
		OpticalInversionMID = nullptr;
	}

	if (PassButton)   PassButton->OnClicked.RemoveAll(this);
	if (RejectButton) RejectButton->OnClicked.RemoveAll(this);
	if (LangToggleButton) LangToggleButton->OnClicked.RemoveAll(this);

	if (UCh1LocSubsystem* Loc = GetLoc())
	{
		Loc->OnLanguageChanged.Remove(LangChangedHandle);
	}

	Super::NativeDestruct();
}

UCh1LocSubsystem* UInspectionScreen::GetLoc() const
{
	if (UWorld* W = GetWorld())
	{
		if (UGameInstance* GI = W->GetGameInstance())
		{
			return GI->GetSubsystem<UCh1LocSubsystem>();
		}
	}
	return nullptr;
}

void UInspectionScreen::RefreshLocalizedTexts()
{
	UCh1LocSubsystem* Loc = GetLoc();
	if (!Loc) return;

	// Progress / Stats / Remaining 都重 render（用到 CorrectCount / Goal / WrongCount 等数值）
	RenderProgress();

	if (LangToggleLabel)
	{
		const ECh1Language Next = (Loc->GetLanguage() == ECh1Language::ZhCN)
			? ECh1Language::EnUS : ECh1Language::ZhCN;
		LangToggleLabel->SetText(FText::FromString(Next == ECh1Language::EnUS ? TEXT("EN") : TEXT("中")));
	}
}

void UInspectionScreen::OnLangToggleClicked()
{
	if (UCh1LocSubsystem* Loc = GetLoc())
	{
		Loc->ToggleLanguage();
	}
}

void UInspectionScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 闪屏衰减：sin(π·t/T) 半周期——0 涨到峰值再回 0。用 SetBrushColor 才动 Border 自身颜色。
	if (FlashOverlay && FlashTotal > 0.0f)
	{
		FlashElapsed += InDeltaTime;
		if (FlashElapsed >= FlashTotal)
		{
			FlashTotal = 0.0f;
			FlashOverlay->SetBrushColor(FLinearColor(0, 0, 0, 0));
		}
		else
		{
			const float Phase = FlashElapsed / FlashTotal;
			const float Alpha = FMath::Sin(Phase * PI) * FlashPeakAlpha;
			FLinearColor C = FlashTargetColor;
			C.A = Alpha;
			FlashOverlay->SetBrushColor(C);
		}
	}

	// 震屏：sin 衰减 — 振幅按时间线性递减
	if (ShakeTotal > 0.0f)
	{
		ShakeElapsed += InDeltaTime;
		if (ShakeElapsed >= ShakeTotal)
		{
			ShakeTotal = 0.0f;
			SetRenderTranslation(FVector2D::ZeroVector);
		}
		else
		{
			const float DecayT = 1.0f - (ShakeElapsed / ShakeTotal);
			const float Freq = 35.0f;  // Hz
			const float OffsetX = FMath::Sin(ShakeElapsed * Freq) * ShakeAmplitude * DecayT;
			const float OffsetY = FMath::Cos(ShakeElapsed * (Freq * 1.3f)) * ShakeAmplitude * DecayT * 0.6f;
			SetRenderTranslation(FVector2D(OffsetX, OffsetY));
		}
	}

	// 倒计时数字（Shift 3+）
	if (DollTimerText && DollTimeoutSec > 0.0f && !bAwaitingNext)
	{
		CurrentDollTimeRemaining = FMath::Max(0.0f, CurrentDollTimeRemaining - InDeltaTime);
		DollTimerText->SetText(FText::AsNumber(FMath::CeilToInt(CurrentDollTimeRemaining)));
	}

	UpdateOpticalInversion(InDeltaTime);
}

void UInspectionScreen::OnPassClicked()
{
	if (bAwaitingNext) return;
	HandlePlayerChoice(/*bPlayerChosePass=*/true);
}

void UInspectionScreen::OnRejectClicked()
{
	if (bAwaitingNext) return;
	HandlePlayerChoice(/*bPlayerChosePass=*/false);
}

void UInspectionScreen::HandlePlayerChoice(bool bPlayerChosePass)
{
	if (DollSequence.Num() == 0)
	{
		return;
	}

	const int32 PoolIdx = CurrentDollIndex % DollSequence.Num();
	const UDollData* Doll = DollSequence[PoolIdx];

	FString FailReason;
	const EJudgmentVerdict GroundTruth = UJudgmentEvaluator::EvaluateDoll(JudgmentCards, Doll, FailReason);

	const bool bPlayerCorrect =
		(bPlayerChosePass && GroundTruth == EJudgmentVerdict::Pass) ||
		(!bPlayerChosePass && GroundTruth == EJudgmentVerdict::Reject);

	if (bPlayerCorrect) CorrectCount++;
	else                WrongCount++;

	// 分类 outcome（per judgment-card §3 Rule 7）
	const EOutcomeClass Outcome = UJudgmentEvaluator::ClassifyOutcome(GroundTruth, bPlayerChosePass);
	switch (Outcome)
	{
	case EOutcomeClass::TruePositive:  TrueAcceptCount++; break;
	case EOutcomeClass::TrueNegative:  TrueRejectCount++; break;
	default: break;
	}
	if (UJudgmentEvaluator::IsMisjudgment(Outcome))
	{
		MisjudgmentCount++;
		ApplyMisjudgmentPressure();
	}

	if (ToastText)
	{
		FString Toast;
		if (bPlayerCorrect)
		{
			Toast = FString::Printf(TEXT("✓ 正确（应%s）"),
				GroundTruth == EJudgmentVerdict::Pass ? TEXT("放行") : TEXT("丢弃"));
		}
		else
		{
			// 第一次 FalsePos / FalseNeg 漂一次文案（spec §4 阈下反馈）
			if (Outcome == EOutcomeClass::FalsePositive && !bHasDriftedFalsePos)
			{
				bHasDriftedFalsePos = true;
				Toast = TEXT("……可是她看起来没问题");
			}
			else if (Outcome == EOutcomeClass::FalseNegative && !bHasDriftedFalseNeg)
			{
				bHasDriftedFalseNeg = true;
				Toast = TEXT("……你是不是太严了");
			}
			else
			{
				Toast = FString::Printf(TEXT("✗ 误判（应%s）—— %s"),
					GroundTruth == EJudgmentVerdict::Pass ? TEXT("放行") : TEXT("丢弃"),
					FailReason.IsEmpty() ? TEXT("符合所有判据") : *FailReason);
			}
		}
		ToastText->SetText(FText::FromString(Toast));
	}

	StartFeedback(bPlayerCorrect);

	// 班次终止实时检查（不再等动画完成）
	if (CheckShiftTermination())
	{
		bAwaitingNext = true;
		SetButtonsEnabled(false);
		if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(DollTimeoutHandle);
		return;
	}

	// Twist 触发：玩家放行 + 客观符合 + 该娃娃配了 Pearl trigger
	if (bPlayerChosePass
		&& GroundTruth == EJudgmentVerdict::Pass
		&& Doll && Doll->bIsTwistTrigger)
	{
		if (AChapter1GameMode* GM = Cast<AChapter1GameMode>(UGameplayStatics::GetGameMode(this)))
		{
			GM->RequestTwist();
		}
	}

	bAwaitingNext = true;
	SetButtonsEnabled(false);

	// 取消本只娃娃的 timeout（玩家已做出选择）
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DollTimeoutHandle);
	}

	// 推进路径：
	//   - 有 DollActor → 由其 ApplyDollData/OnDollAnimComplete 驱动推进（动画时长 = 节奏）
	//   - 没 DollActor（旧 UI 按钮 fallback）→ 用 ToastHoldSeconds 定时器
	if (!DollActor && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			AdvanceTimerHandle,
			FTimerDelegate::CreateUObject(this, &UInspectionScreen::AdvanceToNextDoll),
			FMath::Max(0.1f, ToastHoldSeconds), false
		);
	}
}

bool UInspectionScreen::CheckShiftTermination()
{
	if (bShiftEnded) return true;

	const bool bUseQuota = (PassQuota > 0 || RejectQuota > 0);

	// 失败：误判超上限
	if (MaxMisjudgmentsBeforeFail > 0 && MisjudgmentCount >= MaxMisjudgmentsBeforeFail)
	{
		bShiftEnded = true;
		UE_LOG(LogTemp, Display, TEXT("[InspectionScreen] ★ 班次失败：误判 %d/%d"), MisjudgmentCount, MaxMisjudgmentsBeforeFail);

		if (ToastText)
		{
			ToastText->SetText(FText::FromString(FString::Printf(
				TEXT("★ 班次失败 — 误判 %d / %d 次\n重新组装判据"),
				MisjudgmentCount, MaxMisjudgmentsBeforeFail)));
		}

		FShiftResult R;
		R.TotalDolls = CurrentDollIndex + 1;
		R.CorrectCount = CorrectCount;
		R.WrongCount = WrongCount;
		R.TrueAcceptCount = TrueAcceptCount;
		R.TrueRejectCount = TrueRejectCount;
		R.bSuccess = false;
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(ShiftEndTimer,
				FTimerDelegate::CreateLambda([this, R]() {
					OnShiftCompleted.Broadcast(R);
				}), 1.8f, false);
		}
		return true;
	}

	// 成功
	const bool bDone = bUseQuota
		? (TrueAcceptCount >= PassQuota && TrueRejectCount >= RejectQuota)
		: (CorrectCount >= CorrectGoal);
	if (bDone)
	{
		bShiftEnded = true;
		UE_LOG(LogTemp, Display, TEXT("[InspectionScreen] ★ 班次完成：TP=%d TN=%d"), TrueAcceptCount, TrueRejectCount);

		if (ToastText)
		{
			ToastText->SetText(FText::FromString(FString::Printf(
				TEXT("✓ 班次完成 — 放行 %d / 丢弃 %d"),
				TrueAcceptCount, TrueRejectCount)));
		}

		FShiftResult R;
		R.TotalDolls = CurrentDollIndex + 1;
		R.CorrectCount = CorrectCount;
		R.WrongCount = WrongCount;
		R.TrueAcceptCount = TrueAcceptCount;
		R.TrueRejectCount = TrueRejectCount;
		R.bSuccess = true;
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(ShiftEndTimer,
				FTimerDelegate::CreateLambda([this, R]() {
					OnShiftCompleted.Broadcast(R);
				}), 1.8f, false);
		}
		return true;
	}

	return false;
}

void UInspectionScreen::ApplyMisjudgmentPressure()
{
	EnsureOpticalInversionPostProcess();
	if (!MisjudgmentPPVolume) return;

	// 累积压力：vignette / chromatic / desaturation 渐进，5 次封顶。
	// PP material 存在时额外推 EdgeOpacityT；不存在时这些内置 PP 仍可作为可见 fallback。
	const float Pressure = FMath::Clamp(MisjudgmentCount / 5.0f, 0.0f, 1.0f);
	OpticalInversionMode = ECh1OpticalInversionMode::AmbientButtonEdge;
	OpticalTargetEdgeOpacityT = ComputeOpticalEdgeOpacityFromMisjudgments();
	OpticalTargetEdgeRadiusT = FMath::Lerp(0.0f, 0.12f, Pressure);
	OpticalTargetPulseT = FMath::Lerp(0.05f, 0.35f, Pressure);
	OpticalTargetInvertT = 0.0f;
	OpticalTargetBurnoutT = 0.0f;
	OpticalTargetMechanicalFadeT = 0.0f;

	FPostProcessSettings& PP = MisjudgmentPPVolume->Settings;
	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = FMath::Lerp(0.35f, 1.10f, OpticalTargetEdgeOpacityT);

	PP.bOverride_SceneFringeIntensity = true;
	PP.SceneFringeIntensity = FMath::Lerp(0.0f, 2.5f, Pressure);

	PP.bOverride_ColorSaturation = true;
	PP.ColorSaturation = FVector4(
		FMath::Lerp(1.0f, 0.78f, Pressure),
		FMath::Lerp(1.0f, 0.78f, Pressure),
		FMath::Lerp(1.0f, 0.78f, Pressure),
		1.0f);

	UE_LOG(LogTemp, Verbose, TEXT("[InspectionScreen] 误判压力 %d/5 → Vignette=%.2f Saturation=%.2f"),
		MisjudgmentCount, PP.VignetteIntensity, PP.ColorSaturation.X);
}

void UInspectionScreen::TriggerOpticalInversionSurge(float DurationSec)
{
	EnsureOpticalInversionPostProcess();
	OpticalInversionMode = ECh1OpticalInversionMode::SurgeInversion;
	OpticalSurgeTotalSec = FMath::Max(0.05f, DurationSec);
	OpticalSurgeRemainingSec = OpticalSurgeTotalSec;

	OpticalTargetEdgeOpacityT = 1.0f;
	OpticalTargetEdgeRadiusT = SurgeEdgeRadius;
	OpticalTargetPulseT = 1.0f;
	OpticalTargetInvertT = 0.12f;
	OpticalTargetBurnoutT = 0.0f;
	OpticalTargetMechanicalFadeT = 0.0f;

	if (MisjudgmentPPVolume)
	{
		FPostProcessSettings& PP = MisjudgmentPPVolume->Settings;
		PP.bOverride_VignetteIntensity = true;
		PP.VignetteIntensity = 1.25f;
		PP.bOverride_SceneFringeIntensity = true;
		PP.SceneFringeIntensity = 1.5f;
	}
}

void UInspectionScreen::PlayTwistOpticalInversionBurnout(float DurationSec)
{
	EnsureOpticalInversionPostProcess();
	OpticalInversionMode = ECh1OpticalInversionMode::TwistBurnout;
	OpticalBurnoutTotalSec = FMath::Max(0.05f, DurationSec);
	OpticalBurnoutRemainingSec = OpticalBurnoutTotalSec;

	OpticalTargetEdgeOpacityT = 1.0f;
	OpticalTargetEdgeRadiusT = FMath::Max(SurgeEdgeRadius, 0.35f);
	OpticalTargetPulseT = 1.0f;
	OpticalTargetInvertT = BurnoutInvertPeak;
	OpticalTargetBurnoutT = 1.0f;
	OpticalTargetMechanicalFadeT = 0.0f;

	if (MisjudgmentPPVolume)
	{
		FPostProcessSettings& PP = MisjudgmentPPVolume->Settings;
		PP.bOverride_VignetteIntensity = true;
		PP.VignetteIntensity = 1.35f;
		PP.bOverride_SceneFringeIntensity = true;
		PP.SceneFringeIntensity = 3.0f;
		PP.bOverride_AutoExposureBias = true;
		PP.AutoExposureBias = 2.0f;
	}
}

void UInspectionScreen::FadeOpticalInversionForMechanicalEye(float DurationSec)
{
	EnsureOpticalInversionPostProcess();
	OpticalFadeTotalSec = FMath::Max(0.05f, DurationSec);
	OpticalFadeRemainingSec = OpticalFadeTotalSec;
	OpticalTargetEdgeOpacityT = 0.0f;
	OpticalTargetEdgeRadiusT = 0.0f;
	OpticalTargetPulseT = 0.0f;
	OpticalTargetInvertT = 0.0f;
	OpticalTargetBurnoutT = 0.0f;
	OpticalTargetMechanicalFadeT = 1.0f;
}

void UInspectionScreen::EnsureOpticalInversionPostProcess()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (!MisjudgmentPPVolume)
	{
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		MisjudgmentPPVolume = World->SpawnActor<APostProcessVolume>(
			FVector::ZeroVector, FRotator::ZeroRotator, SP);
		if (MisjudgmentPPVolume)
		{
			MisjudgmentPPVolume->bUnbound = true;
			MisjudgmentPPVolume->BlendWeight = 1.0f;
		}
	}

	if (MisjudgmentPPVolume && OpticalInversionMaterial && !OpticalInversionMID)
	{
		OpticalInversionMID = UMaterialInstanceDynamic::Create(OpticalInversionMaterial, this);
		if (OpticalInversionMID)
		{
			MisjudgmentPPVolume->Settings.WeightedBlendables.Array.Add(
				FWeightedBlendable(1.0f, OpticalInversionMID));
			PushOpticalInversionParameters();
		}
	}
}

void UInspectionScreen::UpdateOpticalInversion(float DeltaSeconds)
{
	if (!MisjudgmentPPVolume && !OpticalInversionMaterial)
	{
		return;
	}

	EnsureOpticalInversionPostProcess();

	const float InterpSpeed = 6.0f;
	OpticalEdgeOpacityT = FMath::FInterpTo(OpticalEdgeOpacityT, OpticalTargetEdgeOpacityT, DeltaSeconds, InterpSpeed);
	OpticalEdgeRadiusT = FMath::FInterpTo(OpticalEdgeRadiusT, OpticalTargetEdgeRadiusT, DeltaSeconds, InterpSpeed);
	OpticalPulseT = FMath::FInterpTo(OpticalPulseT, OpticalTargetPulseT, DeltaSeconds, InterpSpeed);
	OpticalInvertT = FMath::FInterpTo(OpticalInvertT, OpticalTargetInvertT, DeltaSeconds, InterpSpeed * 2.0f);
	OpticalBurnoutT = FMath::FInterpTo(OpticalBurnoutT, OpticalTargetBurnoutT, DeltaSeconds, InterpSpeed * 2.0f);
	OpticalMechanicalFadeT = FMath::FInterpTo(OpticalMechanicalFadeT, OpticalTargetMechanicalFadeT, DeltaSeconds, InterpSpeed);

	if (OpticalSurgeRemainingSec > 0.0f)
	{
		OpticalSurgeRemainingSec -= DeltaSeconds;
		if (OpticalSurgeRemainingSec <= 0.0f && OpticalInversionMode == ECh1OpticalInversionMode::SurgeInversion)
		{
			OpticalInversionMode = ECh1OpticalInversionMode::AmbientButtonEdge;
			OpticalTargetEdgeOpacityT = ComputeOpticalEdgeOpacityFromMisjudgments();
			OpticalTargetEdgeRadiusT = 0.0f;
			OpticalTargetPulseT = 0.15f;
			OpticalTargetInvertT = 0.0f;
		}
	}

	if (OpticalBurnoutRemainingSec > 0.0f)
	{
		OpticalBurnoutRemainingSec -= DeltaSeconds;
		const float BurnoutPhase = 1.0f - FMath::Clamp(OpticalBurnoutRemainingSec / FMath::Max(0.05f, OpticalBurnoutTotalSec), 0.0f, 1.0f);
		OpticalTargetBurnoutT = FMath::Sin(BurnoutPhase * PI);
		if (OpticalBurnoutRemainingSec <= 0.0f)
		{
			FadeOpticalInversionForMechanicalEye(1.5f);
		}
	}

	if (OpticalFadeRemainingSec > 0.0f)
	{
		OpticalFadeRemainingSec -= DeltaSeconds;
		if (MisjudgmentPPVolume)
		{
			const float FadeAlpha = FMath::Clamp(OpticalFadeRemainingSec / FMath::Max(0.05f, OpticalFadeTotalSec), 0.0f, 1.0f);
			FPostProcessSettings& PP = MisjudgmentPPVolume->Settings;
			PP.VignetteIntensity = FMath::Lerp(0.35f, 1.35f, FadeAlpha);
			PP.SceneFringeIntensity = FMath::Lerp(0.0f, 3.0f, FadeAlpha);
			PP.AutoExposureBias = FMath::Lerp(0.0f, 2.0f, FadeAlpha);
		}
	}

	PushOpticalInversionParameters();
}

void UInspectionScreen::PushOpticalInversionParameters()
{
	if (!OpticalInversionMID)
	{
		return;
	}

	OpticalInversionMID->SetScalarParameterValue(TEXT("EdgeOpacityT"), OpticalEdgeOpacityT);
	OpticalInversionMID->SetScalarParameterValue(TEXT("EdgeRadiusT"), OpticalEdgeRadiusT);
	OpticalInversionMID->SetScalarParameterValue(TEXT("PulseT"), OpticalPulseT);
	OpticalInversionMID->SetScalarParameterValue(TEXT("InvertT"), OpticalInvertT);
	OpticalInversionMID->SetScalarParameterValue(TEXT("BurnoutT"), OpticalBurnoutT);
	OpticalInversionMID->SetScalarParameterValue(TEXT("MechanicalFadeT"), OpticalMechanicalFadeT);
	OpticalInversionMID->SetScalarParameterValue(TEXT("ModeT"), static_cast<float>(OpticalInversionMode));
}

float UInspectionScreen::ComputeOpticalEdgeOpacityFromMisjudgments() const
{
	return FMath::Clamp(
		MisjudgmentCount * OpticalMisjudgmentStep + OpticalBaseOpacity,
		OpticalBaseOpacity,
		OpticalMaxOpacity);
}

void UInspectionScreen::StartFeedback(bool bCorrect)
{
	UAudioService::PlayCueStatic(this, bCorrect ? FName("Ch1.Correct") : FName("Ch1.Wrong"));

	// 闪屏
	FlashElapsed = 0.0f;
	FlashTotal = FlashDurationSec;
	FlashTargetColor = bCorrect ? CorrectFlashColor : WrongFlashColor;

	// 震屏（正确弱 / 错误强）
	ShakeElapsed = 0.0f;
	ShakeTotal = ShakeDurationSec;
	ShakeAmplitude = bCorrect ? (WrongShakeAmplitude * 0.33f) : WrongShakeAmplitude;
}

void UInspectionScreen::HandleDollTimeout()
{
	if (bAwaitingNext) return;
	// 超时 = 强制丢弃（GDD：超时 = 系统默认怀疑）
	if (DollActor)
	{
		DollActor->ForceToss();  // 走动画路径，会回调 HandleGestureFromDoll(false)
	}
	else
	{
		HandlePlayerChoice(/*bPlayerChosePass=*/false);
	}
}

void UInspectionScreen::AdvanceToNextDoll()
{
	CurrentDollIndex++;  // 处理过的总数（含错误 + 超时）
	bAwaitingNext = false;

	if (ToastText)
	{
		ToastText->SetText(FText::GetEmpty());
	}

	// 班次终止已由 HandlePlayerChoice 即时检测；这里不再 broadcast，避免双触发
	if (bShiftEnded) { return; }

	SetButtonsEnabled(true);
	RenderCurrentDoll();
	RenderProgress();
	PushCurrentDollToActor();  // 把下一只娃娃 push 给 3D actor，actor 内部 SnapToIdle

	// 启动下一只娃娃的 timeout（若有）
	if (DollTimeoutSec > 0.0f && GetWorld())
	{
		CurrentDollTimeRemaining = DollTimeoutSec;
		GetWorld()->GetTimerManager().SetTimer(
			DollTimeoutHandle,
			FTimerDelegate::CreateUObject(this, &UInspectionScreen::HandleDollTimeout),
			DollTimeoutSec, false);
	}
}

void UInspectionScreen::RenderJudgmentCardsList()
{
	if (!JudgmentCardListText) return;

	TArray<FString> Lines;
	Lines.Reserve(JudgmentCards.Num());
	for (const UCardData* Card : JudgmentCards)
	{
		Lines.Add(DescribeCard(Card));
	}

	const FString Joined = Lines.Num() > 0
		? FString::Join(Lines, TEXT("\n"))
		: TEXT("(无当班判据)");

	JudgmentCardListText->SetText(FText::FromString(Joined));
}

void UInspectionScreen::RenderCurrentDoll()
{
	if (!DollAttributeText) return;

	if (DollSequence.Num() == 0)
	{
		DollAttributeText->SetText(FText::FromString(TEXT("(无娃娃池)")));
		return;
	}

	// 池循环：从 CurrentDollIndex 取余
	const int32 PoolIdx = CurrentDollIndex % DollSequence.Num();
	const UDollData* Doll = DollSequence[PoolIdx];
	if (!Doll)
	{
		DollAttributeText->SetText(FText::FromString(TEXT("(空 doll data)")));
		return;
	}

	const FString DisplayName = Doll->DisplayName.IsEmpty() ? Doll->DollId.ToString() : Doll->DisplayName.ToString();
	const FString Body = FString::Printf(
		TEXT("【%s】\n姿态：%s\n发型：%s\n表情：%s\n饰物：%s"),
		*DisplayName,
		*Doll->Posture.ToString(),
		*JoinTraitSet(Doll->HairTraits),
		*JoinTraitSet(Doll->ExpressionTraits),
		*JoinTraitSet(Doll->AccessoryTraits)
	);

	DollAttributeText->SetText(FText::FromString(Body));
}

void UInspectionScreen::RenderProgress()
{
	UCh1LocSubsystem* Loc = GetLoc();
	const int32 Remaining = FMath::Max(0, CorrectGoal - CorrectCount);

	if (ProgressText && Loc)
	{
		ProgressText->SetText(FText::Format(
			Loc->Get(TEXT("Inspection.Progress")),
			FText::AsNumber(CorrectCount), FText::AsNumber(CorrectGoal)));
	}
	else if (ProgressText)
	{
		// fallback：没有 Loc subsystem 也要能跑
		ProgressText->SetText(FText::FromString(
			FString::Printf(TEXT("%d / %d"), CorrectCount, CorrectGoal)));
	}

	if (RemainingText && Loc)
	{
		RemainingText->SetText(FText::Format(
			Loc->Get(TEXT("Inspection.Remaining")),
			FText::AsNumber(Remaining), FText::AsNumber(CurrentDollIndex), FText::AsNumber(WrongCount)));
	}
}

void UInspectionScreen::SetButtonsEnabled(bool bEnabled)
{
	if (PassButton)   PassButton->SetIsEnabled(bEnabled);
	if (RejectButton) RejectButton->SetIsEnabled(bEnabled);
}

// ── 3D 桌面纸卡（diegetic 标准）─────────────────────────────────────────────

void UInspectionScreen::ClearDeskCards()
{
	for (AActor* A : DeskCardActors)
	{
		if (A) A->Destroy();
	}
	DeskCardActors.Reset();
}

void UInspectionScreen::SpawnDeskCards()
{
	ClearDeskCards();
	UWorld* World = GetWorld();
	if (!World || JudgmentCards.Num() == 0) return;

	UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (!PlaneMesh) return;

	// 维度配色（论点：玩家看见自己选的 4 类标准，从颜色可分辨）
	auto ColorForDim = [](ECardDimension D) -> FVector
	{
		switch (D)
		{
		case ECardDimension::Posture:    return FVector(0.95f, 0.85f, 0.45f); // 暖黄（姿态）
		case ECardDimension::Hair:       return FVector(0.55f, 0.40f, 0.30f); // 棕（发型）
		case ECardDimension::Expression: return FVector(0.85f, 0.45f, 0.45f); // 红（表情）
		case ECardDimension::Accessory:  return FVector(0.55f, 0.65f, 0.85f); // 蓝（饰物）
		}
		return FVector(0.9f);
	};

	for (int32 i = 0; i < JudgmentCards.Num(); ++i)
	{
		const UCardData* Card = JudgmentCards[i];
		if (!Card) continue;

		// 在 doll 前方排开（X≈-30），Y 方向 spread 让相机视野能看见
		const FVector Loc = DeskCardOrigin + FVector(0.0f, i * DeskCardSpacing, 0.0f);

		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Card3D = World->SpawnActor<AStaticMeshActor>(Loc, DeskCardRotation, SP);
		if (!Card3D) continue;

		Card3D->SetMobility(EComponentMobility::Movable);
		if (UStaticMeshComponent* MC = Card3D->GetStaticMeshComponent())
		{
			MC->SetStaticMesh(PlaneMesh);
			// 立起来的纸卡：80×60 unit（高×宽）
			MC->SetRelativeScale3D(FVector(0.6f, 0.85f, 1.0f));
			MC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			// emissive material 让卡发光显眼（M_Highlight 已带 Color + Intensity 参数）
			if (DeskCardMaterial)
			{
				MC->SetMaterial(0, DeskCardMaterial);
			}
			MC->SetVectorParameterValueOnMaterials(TEXT("Color"), ColorForDim(Card->Dimension));
			MC->SetVectorParameterValueOnMaterials(TEXT("BaseColor"), ColorForDim(Card->Dimension));
			MC->SetScalarParameterValueOnMaterials(TEXT("Intensity"), 2.5f);
		}
		DeskCardActors.Add(Card3D);
	}

	UE_LOG(LogTemp, Verbose, TEXT("[InspectionScreen] 桌面 spawn 了 %d 张 3D 纸卡"), DeskCardActors.Num());
}
