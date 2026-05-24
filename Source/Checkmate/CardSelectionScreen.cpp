// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "CardSelectionScreen.h"

#include "CardData.h"
#include "CardSlotWidget.h"
#include "JudgmentCardWidget.h"

#include "Components/Button.h"
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

	// 防御性检查
	if (!CardPoolContainer || !AssemblyContainer)
	{
		UE_LOG(LogTemp, Error, TEXT("CardSelectionScreen: missing required BindWidget panels"));
		return;
	}

	// Spawn N 张可选卡到 Pool
	if (CardWidgetClass && PoolCards.Num() > 0)
	{
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
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("CardSelectionScreen: CardWidgetClass not set or PoolCards empty — running in placeholder mode"));
	}

	// Spawn K 个空 slot 到组装区
	if (SlotWidgetClass)
	{
		for (int32 i = 0; i < K; ++i)
		{
			UCardSlotWidget* NewSlot = CreateWidget<UCardSlotWidget>(this, SlotWidgetClass);
			if (NewSlot)
			{
				NewSlot->SlotIndex = i;
				NewSlot->SetOwningScreen(this);
				AssemblyContainer->AddChild(NewSlot);
				Slots.Add(NewSlot);
			}
		}
	}

	// 绑定 Begin Shift 按钮
	if (BeginShiftButton)
	{
		BeginShiftButton->OnClicked.AddDynamic(this, &UCardSelectionScreen::OnBeginShiftClicked);
		BeginShiftButton->SetIsEnabled(false);
		OnBeginShiftButtonEnableChanged(false);
	}

	// 启动倒计时（每 0.5s tick 一次以更新 label）
	if (GetWorld())
	{
		TimeRemaining = AssemblyTimerSec;
		GetWorld()->GetTimerManager().SetTimer(
			CountdownTimerHandle,
			FTimerDelegate::CreateUObject(this, &UCardSelectionScreen::TickCountdown),
			0.1f, true
		);
	}

	RefreshBeginShiftEnabled();
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

	// 30s 到 → 自动填充剩余 slot（如未填满）+ 强制开始
	if (TimeRemaining <= 0.0f)
	{
		GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);

		// 自动填充：把池里剩余的卡顺序塞进空 slot（slice spec: 随机自动填充，先用顺序代替）
		for (UCardSlotWidget* SlotWidget : Slots)
		{
			if (SlotWidget && SlotWidget->IsEmpty())
			{
				// 找一个还在 Pool 里（即不在任何 slot 里）的卡
				for (UJudgmentCardWidget* Card : AllCardWidgets)
				{
					if (Card && !FindSlotHoldingCard(Card))
					{
						// 此卡当前 parent 是 CardPoolContainer；从 pool 移到 slot
						if (CardPoolContainer)
						{
							CardPoolContainer->RemoveChild(Card);
						}
						SlotWidget->AttachCard(Card);
						break;
					}
				}
			}
		}

		RefreshBeginShiftEnabled();

		// 强制触发：到点了立刻开始（slice spec: 30s 到强制开始）
		OnBeginShiftClicked();
	}
}

void UCardSelectionScreen::HandleCardDroppedOnSlot(UJudgmentCardWidget* DroppedCard, UCardSlotWidget* TargetSlot)
{
	if (!DroppedCard || !TargetSlot)
	{
		return;
	}

	// 1. 从原位置移除（pool 或别的 slot）
	UCardSlotWidget* OldSlot = FindSlotHoldingCard(DroppedCard);
	if (OldSlot && OldSlot != TargetSlot)
	{
		OldSlot->DetachCard();
	}
	else if (!OldSlot && CardPoolContainer)
	{
		CardPoolContainer->RemoveChild(DroppedCard);
	}

	// 2. 处理 target slot 当前持有的卡（如有 → 交换到 OldSlot 或回 Pool）
	if (!TargetSlot->IsEmpty())
	{
		UJudgmentCardWidget* DisplacedCard = TargetSlot->DetachCard();
		if (DisplacedCard)
		{
			if (OldSlot)
			{
				// 交换
				OldSlot->AttachCard(DisplacedCard);
			}
			else
			{
				// 拖入卡来自 pool，被替出的卡回 pool
				ReturnCardToPool(DisplacedCard);
			}
		}
	}

	// 3. 把新卡放进 target slot
	TargetSlot->AttachCard(DroppedCard);

	// 4. 重新检查"开始班次"可点状态
	RefreshBeginShiftEnabled();
}

void UCardSelectionScreen::HandleCardDraggedFromSlot(UJudgmentCardWidget* DraggedCard)
{
	// 玩家从 slot 把卡拖到 pool 区域时调用（如果想让 pool 区域也能 accept drop，
	// 可以在 WBP_CardSelectionScreen 的 pool panel 上重写 NativeOnDrop 调本函数）。
	if (!DraggedCard)
	{
		return;
	}

	UCardSlotWidget* OldSlot = FindSlotHoldingCard(DraggedCard);
	if (OldSlot)
	{
		OldSlot->DetachCard();
		ReturnCardToPool(DraggedCard);
		RefreshBeginShiftEnabled();
	}
}

void UCardSelectionScreen::ReturnCardToPool(UJudgmentCardWidget* Card)
{
	if (Card && CardPoolContainer)
	{
		// 如果当前还有 parent，先 remove
		if (Card->GetParent())
		{
			Card->RemoveFromParent();
		}
		CardPoolContainer->AddChild(Card);
	}
}

UCardSlotWidget* UCardSelectionScreen::FindSlotHoldingCard(UJudgmentCardWidget* Card) const
{
	if (!Card) return nullptr;

	for (UCardSlotWidget* SlotWidget : Slots)
	{
		if (SlotWidget && SlotWidget->HeldCard == Card)
		{
			return SlotWidget;
		}
	}
	return nullptr;
}

void UCardSelectionScreen::RefreshBeginShiftEnabled()
{
	int32 FilledSlots = 0;
	for (UCardSlotWidget* SlotWidget : Slots)
	{
		if (SlotWidget && !SlotWidget->IsEmpty()) FilledSlots++;
	}

	const bool bShouldEnable = (FilledSlots >= K);
	if (BeginShiftButton)
	{
		BeginShiftButton->SetIsEnabled(bShouldEnable);
		OnBeginShiftButtonEnableChanged(bShouldEnable);
	}
}

void UCardSelectionScreen::OnBeginShiftClicked()
{
	// 停倒计时
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);
	}

	// 收集 K 张选中卡的 UCardData
	TArray<UCardData*> SelectedData = GetAssembledCardData();

	// 广播 → game mode / level BP 处理 2D→3D 过渡
	OnAssemblyComplete.Broadcast(SelectedData);

	// 视觉：禁用所有交互
	if (BeginShiftButton)
	{
		BeginShiftButton->SetIsEnabled(false);
	}
}

TArray<UCardData*> UCardSelectionScreen::GetAssembledCardData() const
{
	TArray<UCardData*> Out;
	for (UCardSlotWidget* SlotWidget : Slots)
	{
		if (SlotWidget && SlotWidget->HeldCard)
		{
			UCardData* CardData = SlotWidget->HeldCard->GetCardData();
			if (CardData)
			{
				Out.Add(CardData);
			}
		}
	}
	return Out;
}
