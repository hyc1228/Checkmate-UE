// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Ch1CardTooltipWidget.generated.h"

class UCardData;

UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API UCh1CardTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Ch1|Card Tooltip")
	void SetCardData(const UCardData* InCardData);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Card Tooltip")
	FSlateFontInfo TitleFont;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Card Tooltip")
	FSlateFontInfo BodyFont;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Card Tooltip")
	FLinearColor TitleColor = FLinearColor(0.96f, 0.90f, 0.78f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Card Tooltip")
	FLinearColor BodyColor = FLinearColor(0.82f, 0.78f, 0.70f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Card Tooltip")
	FLinearColor EmphasisColor = FLinearColor(0.95f, 0.17f, 0.12f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Card Tooltip", meta=(ClampMin="120.0"))
	float BodyWrapWidth = 330.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Card Tooltip", meta=(ClampMin="0.0"))
	float ShakeAmplitude = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Card Tooltip", meta=(ClampMin="0.1"))
	float ShakeSpeed = 23.0f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;

private:
	FText TitleText;
	FText BodyMarkupText;

	TSharedPtr<class STextBlock> SlateTitleText;
	TSharedPtr<class SCh1AnimatedMarkupText> SlateBodyText;
};
