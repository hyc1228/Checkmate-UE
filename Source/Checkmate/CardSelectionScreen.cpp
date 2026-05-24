// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "CardSelectionScreen.h"

#include "CardData.h"
#include "JudgmentCardWidget.h"

#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

void UCardSelectionScreen::SetShiftConfig(const TArray<UCardData*>& InPoolCards, int32 InK, float InAssemblyTimerSec)
{
	PoolCards = InPoolCards;
	K = FMath::Max(1, InK);
	AssemblyTimerSec = FMath::Max(1.0f, InAssemblyTimerSec);
	TimeRemaining = AssemblyTimerSec;
}

void UCardSelectionScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (!CardPoolContainer)
	{
		UE_LOG(LogTemp, Error, TEXT("CardSelectionScreen: missing required CardPoolContainer BindWidget"));
		return;
	}

	if (CardWidgetClass && PoolCards.Num() > 0)
	{
		// 先 spawn 所有卡
		for (UCardData* CardData : PoolCards)
		{
			if (!CardData) continue;

			UJudgmentCardWidget* NewCard = CreateWidget<UJudgmentCardWidget>(this, CardWidgetClass);
			if (NewCard)
			{
				NewCard->SetCardData(CardData);
				NewCard->SetOwningScreen(this);
				CardPoolContainer->AddChild(NewCard);
				AllCardWidgets.Add(NewCard);
			}
		}

		// 底部对齐扇形：所有卡的底边在同一水平线，只用旋转角度展开（Balatro 手牌式）。
		// z-order 中央卡最高，边缘卡递减——视觉上中心卡盖在最上面。
		const int32 N = AllCardWidgets.Num();
		if (N > 1)
		{
			const int32 CenterIdx = N / 2;

			for (int32 i = 0; i < N; ++i)
			{
				const float t = (static_cast<float>(i) / static_cast<float>(N - 1)) - 0.5f;
				const float AngleDeg = t * FanSpreadDegrees;

				UJudgmentCardWidget* Card = AllCardWidgets[i];
				Card->SetBaseFanAngle(AngleDeg);

				if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Card->Slot))
				{
					CanvasSlot->SetAnchors(FAnchors(0.5f, 1.0f));    // panel 底部中央
					CanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f)); // Position 指向 widget 底部中央
					CanvasSlot->SetSize(FVector2D(CardWidth, CardHeight));

					const float Offset = (i - (N - 1) * 0.5f) * CardSpacingPx;
					CanvasSlot->SetPosition(FVector2D(Offset, 0.0f));

					// 中央 z-order 最高，向两边递减
					CanvasSlot->SetZOrder(N - FMath::Abs(i - CenterIdx));
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("CardSelectionScreen: CardWidgetClass not set or PoolCards empty"));
	}

	if (BeginShiftButton)
	{
		BeginShiftButton->OnClicked.AddDynamic(this, &UCardSelectionScreen::OnBeginShiftClicked);
		BeginShiftButton->SetIsEnabled(false);
		OnBeginShiftButtonEnableChanged(false);
	}

	if (GetWorld())
	{
		TimeRemaining = AssemblyTimerSec;
		GetWorld()->GetTimerManager().SetTimer(
			CountdownTimerHandle,
			FTimerDelegate::CreateUObject(this, &UCardSelectionScreen::TickCountdown),
			0.1f, true
		);
	}

	RefreshConfirmEnabled();
}

void UCardSelectionScreen::NativeDestruct()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);
	}

	if (BeginShiftButton)
	{
		BeginShiftButton->OnClicked.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UCardSelectionScreen::TickCountdown()
{
	TimeRemaining -= 0.1f;
	if (TimeRemaining < 0.0f)
	{
		TimeRemaining = 0.0f;
	}

	if (TimerLabel)
	{
		const int32 Seconds = FMath::CeilToInt(TimeRemaining);
		TimerLabel->SetText(FText::AsNumber(Seconds));
	}

	if (TimeRemaining <= 0.0f)
	{
		GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);

		// 30s 到 → 用未选卡顺序补齐到 K
		for (UJudgmentCardWidget* Card : AllCardWidgets)
		{
			if (SelectedCards.Num() >= K) break;
			if (Card && !SelectedCards.Contains(Card))
			{
				SelectedCards.Add(Card);
				Card->SetSelected(true);
			}
		}

		RefreshConfirmEnabled();
		OnBeginShiftClicked();
	}
}

void UCardSelectionScreen::HandleCardClicked(UJudgmentCardWidget* ClickedCard)
{
	if (!ClickedCard) return;

	const int32 ExistingIdx = SelectedCards.IndexOfByKey(ClickedCard);
	if (ExistingIdx != INDEX_NONE)
	{
		// 已选 → 取消
		SelectedCards.RemoveAt(ExistingIdx);
		ClickedCard->SetSelected(false);
	}
	else
	{
		// 未选 → 加入（若未到 K）
		if (SelectedCards.Num() >= K)
		{
			// 满了：忽略（也可考虑挤掉最早一张——Balatro 是直接拒绝，这里跟 Balatro）
			return;
		}
		SelectedCards.Add(ClickedCard);
		ClickedCard->SetSelected(true);
	}

	RefreshConfirmEnabled();
}

void UCardSelectionScreen::RefreshConfirmEnabled()
{
	const bool bShouldEnable = (SelectedCards.Num() >= K);
	if (BeginShiftButton)
	{
		BeginShiftButton->SetIsEnabled(bShouldEnable);
		OnBeginShiftButtonEnableChanged(bShouldEnable);
	}
}

void UCardSelectionScreen::OnBeginShiftClicked()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);
	}

	TArray<UCardData*> SelectedData = GetAssembledCardData();
	OnAssemblyComplete.Broadcast(SelectedData);

	if (BeginShiftButton)
	{
		BeginShiftButton->SetIsEnabled(false);
	}
}

TArray<UCardData*> UCardSelectionScreen::GetAssembledCardData() const
{
	TArray<UCardData*> Out;
	Out.Reserve(SelectedCards.Num());
	for (UJudgmentCardWidget* Card : SelectedCards)
	{
		if (Card)
		{
			if (UCardData* Data = Card->GetCardData())
			{
				Out.Add(Data);
			}
		}
	}
	return Out;
}
