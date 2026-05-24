// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "InspectionScreen.h"

#include "CardData.h"
#include "DollData.h"
#include "JudgmentEvaluator.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
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

	// FlashOverlay 初始不可见
	if (FlashOverlay)
	{
		FLinearColor C = FlashOverlay->GetContentColorAndOpacity();
		C.A = 0.0f;
		FlashOverlay->SetContentColorAndOpacity(C);
		FlashOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	RenderJudgmentCardsList();
	RenderCurrentDoll();
	RenderProgress();

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

	Super::NativeDestruct();
}

void UInspectionScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 闪屏衰减：sin(π·t/T) 半周期——0 涨到峰值再回 0
	if (FlashOverlay && FlashTotal > 0.0f)
	{
		FlashElapsed += InDeltaTime;
		if (FlashElapsed >= FlashTotal)
		{
			FlashTotal = 0.0f;
			FLinearColor C = FlashTargetColor;
			C.A = 0.0f;
			FlashOverlay->SetContentColorAndOpacity(C);
		}
		else
		{
			const float Phase = FlashElapsed / FlashTotal;
			const float Alpha = FMath::Sin(Phase * PI) * FlashPeakAlpha;
			FLinearColor C = FlashTargetColor;
			C.A = Alpha;
			FlashOverlay->SetContentColorAndOpacity(C);
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

	if (GetWorld())
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
	// 超时 = 强制丢弃（与 GDD 一致：FalseNeg 若合规 / TrueNeg 若不合规）
	HandlePlayerChoice(/*bPlayerChosePass=*/false);
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
	if (!ProgressText) return;

	// 进度 = 「正确 / 目标」，强调玩家要凑正确数而非过完池
	ProgressText->SetText(FText::FromString(
		FString::Printf(TEXT("正确 %d / %d   （处理 %d 只，丢错 %d）"),
			CorrectCount, CorrectGoal, CurrentDollIndex, WrongCount)
	));
}

void UInspectionScreen::SetButtonsEnabled(bool bEnabled)
{
	if (PassButton)   PassButton->SetIsEnabled(bEnabled);
	if (RejectButton) RejectButton->SetIsEnabled(bEnabled);
}
