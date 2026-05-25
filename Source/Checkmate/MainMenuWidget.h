// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UButton;
class UTextBlock;

/**
 * 主菜单。三个 button：Ch1 / Ch2 / Quit。
 * 设计师只需在 WBP 里绑同名 widget，C++ 自动接 hover scale + click ripple feel。
 */
UCLASS()
class CHECKMATE_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MainMenu")
	FName Ch1MapName = TEXT("Ch1Test");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MainMenu")
	FName Ch2MapName = TEXT("Ch2Test");

	UPROPERTY(meta=(BindWidgetOptional))
	UButton* Ch1Button = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UButton* Ch2Button = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UButton* QuitButton = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	UTextBlock* TitleText = nullptr;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION() void OnCh1Clicked();
	UFUNCTION() void OnCh2Clicked();
	UFUNCTION() void OnQuitClicked();

	UFUNCTION() void OnCh1Hovered();
	UFUNCTION() void OnCh1Unhovered();
	UFUNCTION() void OnCh2Hovered();
	UFUNCTION() void OnCh2Unhovered();
	UFUNCTION() void OnQuitHovered();
	UFUNCTION() void OnQuitUnhovered();

private:
	// 三个 button 的悬停 0~1 状态 + 当前 scale（NativeTick 平滑插值）
	float Ch1Hover = 0.0f, Ch1Scale = 1.0f;
	float Ch2Hover = 0.0f, Ch2Scale = 1.0f;
	float QuitHover = 0.0f, QuitScale = 1.0f;

	// 标题的 idle pulse 时间
	float Elapsed = 0.0f;

	void ApplyButtonScale(UButton* Btn, float Scale);
};
