// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CardSelectionScreen.generated.h"

class UCardData;
class UJudgmentCardWidget;
class UCardSlotWidget;
class UPanelWidget;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssemblyComplete, const TArray<UCardData*>&, SelectedCards);

/**
 * 班次开始时的卡选界面。
 *
 * 工作流：
 *   1. NativeConstruct 时按 PoolCards 数组 spawn N 个 UJudgmentCardWidget 到 CardPoolContainer
 *   2. spawn K 个 UCardSlotWidget 到 AssemblyContainer
 *   3. 启动 30s 倒计时
 *   4. 玩家拖卡到 slot，slot 通过 HandleCardDroppedOnSlot 把请求回传给本 screen
 *   5. AssemblyContainer 全填满 OR 30s 到 → BeginShiftButton 可点
 *   6. 玩家按 BeginShiftButton → 触发 OnAssemblyComplete delegate（携带 K 张选中卡），开始 2D→3D 过渡
 *
 * WBP 子类 (`WBP_CardSelectionScreen`) 必须提供这些命名 widget：
 *   - CardPoolContainer (UPanelWidget) —— 池区，N 张可选卡
 *   - AssemblyContainer (UPanelWidget) —— 组装区，K 个槽位
 *   - TimerLabel (UTextBlock) —— 倒计时显示
 *   - BeginShiftButton (UButton) —— 开始班次按钮
 *
 * 调用方（GameMode / level BP）应该：
 *   1. CreateWidget<UCardSelectionScreen>
 *   2. SetShiftConfig(PoolCards, K, AssemblyTimerSec)
 *   3. AddToViewport
 *   4. Bind OnAssemblyComplete
 */
UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API UCardSelectionScreen : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 班次开始前由调用方设置。N = PoolCards.Num()。 */
	UFUNCTION(BlueprintCallable, Category="CardSelection")
	void SetShiftConfig(const TArray<UCardData*>& InPoolCards, int32 InK, float InAssemblyTimerSec);

	/** Slot 在 drop 时调用此函数，通知 screen 处理 re-parent + 状态更新。 */
	void HandleCardDroppedOnSlot(UJudgmentCardWidget* DroppedCard, UCardSlotWidget* TargetSlot);

	/** 卡 widget 在 drag from slot 时调用此函数（玩家从 slot 把卡拖出来）。 */
	void HandleCardDraggedFromSlot(UJudgmentCardWidget* DraggedCard);

	/** 选好的卡（K 张）。BeginShift 按下时通过 delegate 传出。 */
	UPROPERTY(BlueprintAssignable, Category="CardSelection")
	FOnAssemblyComplete OnAssemblyComplete;

	/** 可选：暴露当前激活的卡（调试用）。 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="CardSelection")
	TArray<UCardData*> GetAssembledCardData() const;

	/** Designer 可调：UJudgmentCardWidget 的 widget class（WBP_JudgmentCard）。WBP 子类应填这个。 */
	UPROPERTY(EditDefaultsOnly, Category="CardSelection|Classes")
	TSubclassOf<UJudgmentCardWidget> CardWidgetClass;

	/** Designer 可调：UCardSlotWidget 的 widget class（WBP_CardSlot）。 */
	UPROPERTY(EditDefaultsOnly, Category="CardSelection|Classes")
	TSubclassOf<UCardSlotWidget> SlotWidgetClass;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UPanelWidget* CardPoolContainer = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UPanelWidget* AssemblyContainer = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UTextBlock* TimerLabel = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UButton* BeginShiftButton = nullptr;

	UFUNCTION()
	void OnBeginShiftClicked();

	/** WBP 可 override：班次开始按钮的 enable/disable 视觉反馈。 */
	UFUNCTION(BlueprintImplementableEvent, Category="CardSelection|Events")
	void OnBeginShiftButtonEnableChanged(bool bEnabled);

private:
	/** 班次配置数据。 */
	UPROPERTY()
	TArray<UCardData*> PoolCards;

	int32 K = 3;
	float AssemblyTimerSec = 30.0f;
	float TimeRemaining = 30.0f;

	/** Spawn 的 K 个 slot widgets。 */
	UPROPERTY()
	TArray<UCardSlotWidget*> Slots;

	/** Spawn 的 N 张 card widgets。 */
	UPROPERTY()
	TArray<UJudgmentCardWidget*> AllCardWidgets;

	FTimerHandle CountdownTimerHandle;

	/** 每秒 tick 倒计时。 */
	void TickCountdown();

	/** 重新检查是否所有 slot 都填满，更新 BeginShift 按钮可点状态。 */
	void RefreshBeginShiftEnabled();

	/** 把卡返回到 pool（移除任何 slot 的引用，re-parent 回 pool container）。 */
	void ReturnCardToPool(UJudgmentCardWidget* Card);

	/** 找到卡当前所在的 slot（如有）。 */
	UCardSlotWidget* FindSlotHoldingCard(UJudgmentCardWidget* Card) const;
};
