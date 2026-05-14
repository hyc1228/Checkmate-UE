// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CardSlotWidget.generated.h"

class UJudgmentCardWidget;
class UCardSelectionScreen;
class UPanelWidget;

/**
 * 组装区里的单个空槽位 widget。
 * 接受卡 drop，把卡 re-parent 到自己；同时通知 owning screen。
 *
 * 用法：
 *   1. WBP_CardSlot 继承自此类
 *   2. 里面放一个 PanelWidget（如 SizeBox 或 Border）命名为 "ContentHost"（必须）
 *   3. CardSelectionScreen 在 spawn K 个 slot 时调用 SetOwningScreen()
 */
UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API UCardSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 槽位索引（0 到 K-1），由 owning screen 设置。 */
	UPROPERTY(BlueprintReadWrite, Category="Slot")
	int32 SlotIndex = -1;

	/** 当前持有的卡（null 表示空）。 */
	UPROPERTY(BlueprintReadOnly, Category="Slot")
	UJudgmentCardWidget* HeldCard = nullptr;

	UFUNCTION(BlueprintCallable, Category="Slot")
	void SetOwningScreen(UCardSelectionScreen* InOwner) { OwningScreen = InOwner; }

	UFUNCTION(BlueprintCallable, Category="Slot")
	bool IsEmpty() const { return HeldCard == nullptr; }

	/** 把卡放进槽（由 screen 调用，处理 re-parent）。 */
	void AttachCard(UJudgmentCardWidget* InCard);

	/** 从槽里拿出卡（不销毁，仅 detach）。 */
	UJudgmentCardWidget* DetachCard();

protected:
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	/** 视觉容器——卡放进来时 attach 到这里。WBP 子类的 panel 必须命名为 "ContentHost"。 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Slot|UMG Binding")
	UPanelWidget* ContentHost = nullptr;

	/** 视觉反馈：drop 接受时高亮（WBP 可 override）。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Slot|Events")
	void OnDropHighlightStart();

	UFUNCTION(BlueprintImplementableEvent, Category="Slot|Events")
	void OnDropHighlightEnd();

private:
	UPROPERTY()
	UCardSelectionScreen* OwningScreen = nullptr;
};
