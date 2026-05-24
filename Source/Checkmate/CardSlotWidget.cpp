// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "CardSlotWidget.h"
#include "JudgmentCardWidget.h"
#include "CardSelectionDragOp.h"
#include "CardSelectionScreen.h"
#include "Components/PanelWidget.h"

bool UCardSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	// 已弃用：v0.3 起卡选采用点选 + 浮起模式（CardSelectionScreen::HandleCardClicked），不再走 drag-drop。
	// 此函数保留为 no-op 以便保留 WBP_CardSlot asset 兼容性，后续清理时连同 .h/.cpp 一并删除。
	return false;
}

void UCardSlotWidget::AttachCard(UJudgmentCardWidget* InCard)
{
	if (!InCard || !ContentHost)
	{
		return;
	}

	HeldCard = InCard;
	ContentHost->AddChild(InCard);
}

UJudgmentCardWidget* UCardSlotWidget::DetachCard()
{
	if (!HeldCard || !ContentHost)
	{
		return nullptr;
	}

	UJudgmentCardWidget* OutCard = HeldCard;
	ContentHost->RemoveChild(OutCard);
	HeldCard = nullptr;
	return OutCard;
}
