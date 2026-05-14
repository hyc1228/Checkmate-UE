// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "CardSlotWidget.h"
#include "JudgmentCardWidget.h"
#include "CardSelectionDragOp.h"
#include "CardSelectionScreen.h"
#include "Components/PanelWidget.h"

bool UCardSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UCardSelectionDragOp* CardOp = Cast<UCardSelectionDragOp>(InOperation);
	if (!CardOp || !CardOp->SourceCard)
	{
		return false;
	}

	if (OwningScreen)
	{
		OwningScreen->HandleCardDroppedOnSlot(CardOp->SourceCard, this);
	}

	OnDropHighlightEnd();
	return true;
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
