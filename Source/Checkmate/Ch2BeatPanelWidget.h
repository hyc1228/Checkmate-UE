// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Ch2BeatPanelWidget.generated.h"

UENUM(BlueprintType)
enum class ECh2BeatPanelMoment : uint8
{
	ChapterIntro,
	ModeRitual,
	PuppetArmed,
	DestructibleBroken,
	ExitReached
};

USTRUCT(BlueprintType)
struct FCh2BeatPanelPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch2|Beat")
	ECh2BeatPanelMoment Moment = ECh2BeatPanelMoment::ChapterIntro;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch2|Beat")
	FText Eyebrow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch2|Beat")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch2|Beat")
	FText Body;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch2|Beat")
	FName CueId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch2|Beat")
	bool bMajorBeat = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCh2BeatPanelPayload, FCh2BeatPanelPayload, Payload);

UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API UCh2BeatPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Ch2|Beat")
	void ConfigurePanel(const FCh2BeatPanelPayload& InPayload, float InLifetimeSeconds);

	UFUNCTION(BlueprintCallable, Category="Ch2|Beat")
	void StartDismiss();

	UPROPERTY(BlueprintAssignable, Category="Ch2|Beat")
	FOnCh2BeatPanelPayload OnRevealCue;

	UPROPERTY(BlueprintAssignable, Category="Ch2|Beat")
	FOnCh2BeatPanelPayload OnDismissCue;

	UFUNCTION(BlueprintImplementableEvent, Category="Ch2|Beat")
	void K2_PlayRevealEffect(const FCh2BeatPanelPayload& InPayload);

	UFUNCTION(BlueprintImplementableEvent, Category="Ch2|Beat")
	void K2_PlayDismissEffect(const FCh2BeatPanelPayload& InPayload);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void ApplyPayloadToSlate();

	FCh2BeatPanelPayload Payload;
	float LifetimeSeconds = 1.8f;
	float ElapsedSeconds = 0.0f;
	float DismissElapsedSeconds = 0.0f;
	bool bDismissing = false;
	bool bRevealBroadcast = false;

	TSharedPtr<class STextBlock> EyebrowTextBlock;
	TSharedPtr<class STextBlock> TitleTextBlock;
	TSharedPtr<class STextBlock> BodyTextBlock;
};
