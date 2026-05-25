// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InspectionScreen.generated.h"

class UCardData;
class UDollData;
class UTextBlock;
class UButton;
class UBorder;
class ADollDisplay;
class UCh1LocSubsystem;

/**
 * 单班检验结果。班次结束时通过 OnShiftCompleted 广播给 GameMode。
 */
USTRUCT(BlueprintType)
struct FShiftResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Shift")
	int32 TotalDolls = 0;

	UPROPERTY(BlueprintReadOnly, Category="Shift")
	int32 CorrectCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Shift")
	int32 WrongCount = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShiftCompleted, FShiftResult, Result);

/**
 * Ch1 班次中的娃娃检验界面（灯盒版）。
 *
 * 工作流：
 *   1. GameMode 调 SetShiftData(K 张判据卡, 3 只娃娃)
 *   2. AddToViewport → NativeConstruct 渲染第 1 只娃娃 + K 张判据卡 label
 *   3. 玩家审视 → 按 PassButton 或 RejectButton
 *   4. UJudgmentEvaluator 算出客观裁决 → 与玩家选择对比 → toast「正确 / 误判」
 *   5. 1.0s 后推进到下一只
 *   6. 全部 3 只过完 → 广播 OnShiftCompleted(FShiftResult)
 *
 * WBP_InspectionScreen 必须提供以下命名 widget：
 *   - JudgmentCardListText (TextBlock) — 显示当班 K 张卡的字面要求
 *   - DollAttributeText    (TextBlock) — 显示当前娃娃 4 维属性
 *   - ProgressText         (TextBlock) — "1 / 3"
 *   - ToastText            (TextBlock) — "正确" / "误判：xxx"
 *   - PassButton           (Button)
 *   - RejectButton         (Button)
 */
UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API UInspectionScreen : public UUserWidget
{
	GENERATED_BODY()

public:
	/** GameMode 在 AddToViewport 前调用。 */
	UFUNCTION(BlueprintCallable, Category="Inspection")
	void SetShiftData(const TArray<UCardData*>& InJudgmentCards, const TArray<UDollData*>& InDollSequence);

	/** GameMode 在 spawn 完 3D 娃娃 actor 后调用，把 actor 引用交给本 screen。 */
	UFUNCTION(BlueprintCallable, Category="Inspection")
	void SetDollActor(ADollDisplay* InActor);

	/** ADollDisplay 在手势确认/丢弃瞬间调（早事件，立即触发评估 + 反馈）。 */
	void HandleGestureFromDoll(bool bConfirm);

	/** ADollDisplay 动画播完时调（晚事件，触发推进下一只或下班）。 */
	void OnDollAnimComplete();

	/** ADollDisplay 在玩家做出「扔」动作的瞬间调（独立于评估）—— 触发轻微震 + 风声占位。 */
	void PlayTossActionFeedback();

	/** ADollDisplay 在印章砸到娃娃的瞬间调 —— 触发强震 + 冲击。 */
	void PlayStampImpactFeedback();

	/** 班次结束时广播。 */
	UPROPERTY(BlueprintAssignable, Category="Inspection")
	FOnShiftCompleted OnShiftCompleted;

	/** toast 停留时间（秒），到点后自动推进下一只。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inspection|Tuning", meta=(ClampMin="0.1"))
	float ToastHoldSeconds = 1.0f;

	/** 每只娃娃超时（秒）。<=0 表示无超时（Shift 1-2）。GameMode 按班次配置改写。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inspection|Tuning", meta=(ClampMin="0.0"))
	float DollTimeoutSec = 0.0f;

	/** 本班需要正确判定多少次才下班。GameMode 按班次配置改写。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inspection|Tuning", meta=(ClampMin="1"))
	int32 CorrectGoal = 3;

	// ── 反馈 juice 参数 ────────────────────────────────────────────────────

	/** 正确闪屏颜色（绿调）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback")
	FLinearColor CorrectFlashColor = FLinearColor(0.2f, 0.9f, 0.4f, 1.0f);

	/** 错误闪屏颜色（红调）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback")
	FLinearColor WrongFlashColor = FLinearColor(0.95f, 0.25f, 0.25f, 1.0f);

	/** 闪屏峰值不透明度（0-1）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback", meta=(ClampMin="0.0", ClampMax="1.0"))
	float FlashPeakAlpha = 0.45f;

	/** 闪屏总时长（秒）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback", meta=(ClampMin="0.05"))
	float FlashDurationSec = 0.35f;

	/** 错误震屏振幅（像素），正确震屏用 1/3。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback", meta=(ClampMin="0.0"))
	float WrongShakeAmplitude = 14.0f;

	/** 震屏时长（秒）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback", meta=(ClampMin="0.05"))
	float ShakeDurationSec = 0.30f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UTextBlock* JudgmentCardListText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UTextBlock* DollAttributeText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UTextBlock* ProgressText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UTextBlock* ToastText = nullptr;

	/** v0.5：UI 按钮废弃，留 BindWidgetOptional 兼容旧 WBP；3D 手势已替代。 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UMG Binding")
	UButton* PassButton = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UMG Binding")
	UButton* RejectButton = nullptr;

	/** 全屏闪屏 overlay（Border，覆盖整屏）。WBP 里命名 FlashOverlay，可选。 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UMG Binding")
	UBorder* FlashOverlay = nullptr;

	/** 倒计时显示（用于 Shift 3+ 的 DollTimeoutSec）。可选。 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UMG Binding")
	UTextBlock* DollTimerText = nullptr;

	/** 显示「还需正确 N 只」等班次目标信息。 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UMG Binding")
	UTextBlock* RemainingText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UMG Binding")
	UButton* LangToggleButton = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UMG Binding")
	UTextBlock* LangToggleLabel = nullptr;

	UFUNCTION()
	void OnLangToggleClicked();

	UFUNCTION()
	void OnPassClicked();

	UFUNCTION()
	void OnRejectClicked();

private:
	UPROPERTY()
	TArray<UCardData*> JudgmentCards;

	UPROPERTY()
	TArray<UDollData*> DollSequence;

	UPROPERTY()
	ADollDisplay* DollActor = nullptr;

	int32 CurrentDollIndex = 0;
	int32 CorrectCount = 0;
	int32 WrongCount = 0;
	bool bAwaitingNext = false;

	// 误判统计 + 文案漂移 state
	int32 MisjudgmentCount = 0;
	bool bHasDriftedFalsePos = false;  // 一次性：第一次 FalsePos 时 toast 漂一次
	bool bHasDriftedFalseNeg = false;  // 一次性：第一次 FalseNeg 时 toast 漂一次

	FTimerHandle AdvanceTimerHandle;
	FTimerHandle DollTimeoutHandle;
	float CurrentDollTimeRemaining = 0.0f;

	// 反馈状态（NativeTick 驱动衰减）
	float FlashElapsed = 0.0f;
	float FlashTotal = 0.0f;
	FLinearColor FlashTargetColor = FLinearColor::White;
	float ShakeElapsed = 0.0f;
	float ShakeTotal = 0.0f;
	float ShakeAmplitude = 0.0f;

	void RenderJudgmentCardsList();
	void RenderCurrentDoll();
	void RenderProgress();
	void HandlePlayerChoice(bool bPlayerChosePass);
	void AdvanceToNextDoll();
	void SetButtonsEnabled(bool bEnabled);

	void StartFeedback(bool bCorrect);
	void HandleDollTimeout();
	void PushCurrentDollToActor();

	void RefreshLocalizedTexts();
	UCh1LocSubsystem* GetLoc() const;

	FDelegateHandle LangChangedHandle;
};
