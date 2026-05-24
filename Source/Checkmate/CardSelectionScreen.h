// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CardSelectionScreen.generated.h"

class UCardData;
class UJudgmentCardWidget;
class UPanelWidget;
class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssemblyComplete, const TArray<UCardData*>&, SelectedCards);

/**
 * 班次开始时的卡选界面（Balatro 风：扇形铺开，点选 K 张，Confirm）。
 *
 * 工作流：
 *   1. NativeConstruct 时按 PoolCards 数组 spawn N 个 UJudgmentCardWidget 到 CardPoolContainer
 *   2. 启动 30s 倒计时
 *   3. 玩家点卡 → card 调 HandleCardClicked → 切换选中（永久浮起）
 *   4. Selected.Num() == K → ConfirmButton 可点
 *   5. 30s 到 → 用未选卡顺序补齐到 K，强制开始
 *   6. 玩家按 ConfirmButton → 触发 OnAssemblyComplete delegate（携带 K 张选中卡）
 *
 * WBP 子类 (`WBP_CardSelectionScreen`) 必须提供这些命名 widget：
 *   - CardPoolContainer (UPanelWidget) —— 卡铺开的容器
 *   - TimerLabel (UTextBlock) —— 倒计时显示
 *   - BeginShiftButton (UButton) —— 确定按钮（名字 legacy，文案改"确定"即可）
 *
 * 调用方（GameMode）：
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

	/** 卡 widget 在被点击时调用此函数，由 screen 仲裁是否切换选中。 */
	void HandleCardClicked(UJudgmentCardWidget* ClickedCard);

	/** 选好的卡（K 张）。Confirm 按下时通过 delegate 传出。 */
	UPROPERTY(BlueprintAssignable, Category="CardSelection")
	FOnAssemblyComplete OnAssemblyComplete;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="CardSelection")
	TArray<UCardData*> GetAssembledCardData() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="CardSelection")
	int32 GetSelectedCount() const { return SelectedCards.Num(); }

	/** Designer 可调：UJudgmentCardWidget 的 widget class（WBP_JudgmentCard）。 */
	UPROPERTY(EditDefaultsOnly, Category="CardSelection|Classes")
	TSubclassOf<UJudgmentCardWidget> CardWidgetClass;

	// ── 扇形布局参数 ──────────────────────────────────────────────────────
	// 在 WBP_CardSelectionScreen 的 Class Defaults 里调，立即生效（不用 C++ 重编）

	/** 扇形总展开角度（度）。值越大扇越宽。13 张卡推荐 30-50。 */
	UPROPERTY(EditDefaultsOnly, Category="CardSelection|Fan", meta=(ClampMin="0.0", ClampMax="180.0"))
	float FanSpreadDegrees = 36.0f;

	/** 相邻两张卡的水平间距（像素）。< CardWidth 即有重叠。 */
	UPROPERTY(EditDefaultsOnly, Category="CardSelection|Fan", meta=(ClampMin="10.0"))
	float CardSpacingPx = 65.0f;

	/** 单张卡渲染宽度（像素）。同时设 WBP_JudgmentCard 里 SizeBox 一致。 */
	UPROPERTY(EditDefaultsOnly, Category="CardSelection|Fan", meta=(ClampMin="40.0"))
	float CardWidth = 120.0f;

	/** 单张卡渲染高度（像素）。同时设 WBP_JudgmentCard 里 SizeBox 一致。 */
	UPROPERTY(EditDefaultsOnly, Category="CardSelection|Fan", meta=(ClampMin="60.0"))
	float CardHeight = 180.0f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UPanelWidget* CardPoolContainer = nullptr;

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
	UPROPERTY()
	TArray<UCardData*> PoolCards;

	int32 K = 3;
	float AssemblyTimerSec = 30.0f;
	float TimeRemaining = 30.0f;

	/** 当前选中的卡 widget（顺序 = 玩家点击顺序）。最长 K。 */
	UPROPERTY()
	TArray<UJudgmentCardWidget*> SelectedCards;

	UPROPERTY()
	TArray<UJudgmentCardWidget*> AllCardWidgets;

	FTimerHandle CountdownTimerHandle;

	void TickCountdown();

	/** 根据 SelectedCards.Num() 刷新 Confirm 按钮可点状态。 */
	void RefreshConfirmEnabled();
};
