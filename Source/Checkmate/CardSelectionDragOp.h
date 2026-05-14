// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "CardSelectionDragOp.generated.h"

class UJudgmentCardWidget;

/**
 * UMG drag-drop payload，用于把"被拖动的卡"信息传到 drop target。
 */
UCLASS(BlueprintType)
class CHECKMATE_API UCardSelectionDragOp : public UDragDropOperation
{
	GENERATED_BODY()

public:
	/** 被拖动的源卡 widget。Drop target 用它确定该重新 parent 哪个 widget。 */
	UPROPERTY(BlueprintReadWrite, Category="Drag")
	UJudgmentCardWidget* SourceCard = nullptr;
};
