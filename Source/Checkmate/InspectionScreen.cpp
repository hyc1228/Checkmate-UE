// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "InspectionScreen.h"

#include "CardData.h"
#include "Ch1LocSubsystem.h"
#include "DollData.h"
#include "DollDisplay.h"
#include "JudgmentEvaluator.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
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
	bAwaitingNext = false;
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
			Toast = FString::Printf(TEXT("✗ 误判（应%s）—— %s"),
				GroundTruth == EJudgmentVerdict::Pass ? TEXT("放行") : TEXT("丢弃"),
				FailReason.IsEmpty() ? TEXT("符合所有判据") : *FailReason);
		}
		ToastText->SetText(FText::FromString(Toast));
	}

	StartFeedback(bPlayerCorrect);

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

void UInspectionScreen::StartFeedback(bool bCorrect)
{
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

	// 班次终止条件：正确次数达标（不是过完池）
	if (CorrectCount >= CorrectGoal)
	{
		FShiftResult Result;
		Result.TotalDolls = CurrentDollIndex;  // 总处理数
		Result.CorrectCount = CorrectCount;
		Result.WrongCount = WrongCount;

		SetButtonsEnabled(false);
		OnShiftCompleted.Broadcast(Result);
		return;
	}

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
