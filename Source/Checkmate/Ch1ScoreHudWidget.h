// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Ch1ScoreHudWidget.generated.h"

class STextBlock;
class SWidget;

UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API UCh1ScoreHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Ch1|Score")
	void SetScoreState(int32 InDayNumber, int32 InMoney, int32 InQuota, int32 InProcessed, int32 InTarget);

	UFUNCTION(BlueprintCallable, Category="Ch1|Score")
	void PlayScoreEvent(int32 InDelta, int32 InMoney, int32 InQuota, int32 InCombo, float InMultiplier = 1.0f);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	int32 DayNumber = 1;
	int32 Money = 0;
	int32 Quota = 0;
	int32 Processed = 0;
	int32 Target = 0;
	int32 LastDelta = 0;
	int32 Combo = 0;
	float Multiplier = 1.0f;

	float PulseElapsed = 999.0f;
	float PulseDuration = 0.72f;

	TSharedPtr<STextBlock> DayText;
	TSharedPtr<STextBlock> MoneyText;
	TSharedPtr<STextBlock> QuotaText;
	TSharedPtr<STextBlock> DeltaText;
	TSharedPtr<STextBlock> ComboText;

	FVector2D GetPulseScale() const;
	FVector2D GetPulseTranslation() const;
	float GetPulseOpacity() const;
	EVisibility GetDeltaVisibility() const;
	void RefreshTexts();
};
