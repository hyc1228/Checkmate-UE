// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InspectionScreen.generated.h"

class UCardData;
class UDollData;
class UTextBlock;
class UButton;

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

	/** 班次结束时广播。 */
	UPROPERTY(BlueprintAssignable, Category="Inspection")
	FOnShiftCompleted OnShiftCompleted;

	/** toast 停留时间（秒），到点后自动推进下一只。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inspection|Tuning", meta=(ClampMin="0.1"))
	float ToastHoldSeconds = 1.0f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UTextBlock* JudgmentCardListText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UTextBlock* DollAttributeText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UTextBlock* ProgressText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UTextBlock* ToastText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UButton* PassButton = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UButton* RejectButton = nullptr;

	UFUNCTION()
	void OnPassClicked();

	UFUNCTION()
	void OnRejectClicked();

private:
	UPROPERTY()
	TArray<UCardData*> JudgmentCards;

	UPROPERTY()
	TArray<UDollData*> DollSequence;

	int32 CurrentDollIndex = 0;
	int32 CorrectCount = 0;
	int32 WrongCount = 0;
	bool bAwaitingNext = false;

	FTimerHandle AdvanceTimerHandle;

	void RenderJudgmentCardsList();
	void RenderCurrentDoll();
	void RenderProgress();
	void HandlePlayerChoice(bool bPlayerChosePass);
	void AdvanceToNextDoll();
	void SetButtonsEnabled(bool bEnabled);
};
