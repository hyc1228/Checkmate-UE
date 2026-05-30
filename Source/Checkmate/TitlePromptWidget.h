// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TitlePromptWidget.generated.h"

class STextBlock;
class SBorder;

UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API UTitlePromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Title")
	void SetStarted(bool bInStarted);

	UFUNCTION(BlueprintCallable, Category="Title")
	void StartFadeToBlack(float DurationSeconds);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	TSharedPtr<STextBlock> PromptText;
	TSharedPtr<SBorder> FadeBorder;
	bool bStarted = false;
	bool bFadingToBlack = false;
	float FadeDurationSeconds = 0.0f;
	float FadeElapsedSeconds = 0.0f;

	void RequestStart();
	void UpdateFade(float Alpha);
};
