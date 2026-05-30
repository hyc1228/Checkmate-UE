// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Ch2Pawn.h"
#include "Ch2HUDWidget.generated.h"

class UTextBlock;
class UBorder;

/**
 * Ch2 顶部 HUD：显示当前模式（芭蕾 / 小丑）。
 * WBP_Ch2HUD 必须提供 BindWidget UTextBlock "ModeText" 和 "HintText"。
 */
UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API UCh2HUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Ch2 HUD")
	void SetMode(ECh2Mode NewMode);

	UFUNCTION(BlueprintCallable, Category="Ch2 HUD")
	void SetHintMessage(const FString& Hint);

	/** 切换娃娃 ritual：屏幕短暂 fade（"取扣 / 缝扣" 占位）。 */
	UFUNCTION(BlueprintCallable, Category="Ch2 HUD")
	void PlayPickupRitual();

	/** 胜利屏（达到 Exit 时）。 */
	UFUNCTION(BlueprintCallable, Category="Ch2 HUD")
	void ShowVictory();

	/** 移动步数 HUD 更新（GameMode 每步调一次）。Budget=0 时只显示当前步数。 */
	UFUNCTION(BlueprintCallable, Category="Ch2 HUD")
	void SetMoveCounter(int32 Current, int32 Budget);

	UFUNCTION(BlueprintCallable, Category="Ch2 HUD")
	void SetMoveCounterVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category="Ch2 HUD")
	void SetMinimalHUDMode(bool bMinimal);

	/** Ritual fade 总时长。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch2 HUD")
	float RitualDuration = 0.7f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UTextBlock* ModeText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UMG Binding")
	UTextBlock* HintText = nullptr;

	/** 全屏 fade overlay（拾取 ritual 用）。 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UMG Binding")
	UBorder* FadeOverlay = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UMG Binding")
	UTextBlock* RitualText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UMG Binding")
	UTextBlock* VictoryText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UMG Binding")
	UTextBlock* MoveCounterText = nullptr;

private:
	float RitualElapsed = 0.0f;
	bool bRitualActive = false;
	float ModePulseElapsed = 99.0f;
	float HintPulseElapsed = 99.0f;
	bool bMinimalHUDMode = false;
	bool bMoveCounterRequestedVisible = true;
};
