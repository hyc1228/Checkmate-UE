// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Ch1ProgressPanelWidget.generated.h"

UENUM(BlueprintType)
enum class ECh1ProgressPanelMoment : uint8
{
	ShiftIntro,
	ShiftComplete,
	ShiftRetry,
	ChapterComplete,
	TwistLeadIn
};

USTRUCT(BlueprintType)
struct FCh1ProgressPanelPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Progress")
	int32 ShiftIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Progress")
	int32 ShiftNumber = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Progress")
	int32 TotalShifts = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Progress")
	ECh1ProgressPanelMoment Moment = ECh1ProgressPanelMoment::ShiftIntro;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Progress")
	FText Eyebrow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Progress")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Progress")
	FText Body;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Progress")
	FName CueId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Progress")
	bool bMajorBeat = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCh1ProgressPanelPayload, FCh1ProgressPanelPayload, Payload);

/**
 * Native fallback panel for Ch1 day/shift beats.
 *
 * A WBP can subclass this later for art-directed layouts and bind to the cue events.
 * When no WBP is assigned, RebuildWidget creates a full-screen Slate panel so the
 * progression beats still read in the graybox build.
 */
UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API UCh1ProgressPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Ch1|Progress")
	void ConfigurePanel(const FCh1ProgressPanelPayload& InPayload, float InLifetimeSeconds);

	UFUNCTION(BlueprintCallable, Category="Ch1|Progress")
	void StartDismiss();

	UPROPERTY(BlueprintAssignable, Category="Ch1|Progress")
	FOnCh1ProgressPanelPayload OnRevealCue;

	UPROPERTY(BlueprintAssignable, Category="Ch1|Progress")
	FOnCh1ProgressPanelPayload OnDismissCue;

	UFUNCTION(BlueprintImplementableEvent, Category="Ch1|Progress")
	void K2_PlayRevealEffect(const FCh1ProgressPanelPayload& InPayload);

	UFUNCTION(BlueprintImplementableEvent, Category="Ch1|Progress")
	void K2_PlayDismissEffect(const FCh1ProgressPanelPayload& InPayload);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void ApplyPayloadToSlate();

	FCh1ProgressPanelPayload Payload;
	float LifetimeSeconds = 2.5f;
	float ElapsedSeconds = 0.0f;
	float DismissElapsedSeconds = 0.0f;
	bool bDismissing = false;
	bool bRevealBroadcast = false;

	TSharedPtr<class STextBlock> EyebrowTextBlock;
	TSharedPtr<class STextBlock> TitleTextBlock;
	TSharedPtr<class STextBlock> BodyTextBlock;
};
