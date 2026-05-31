// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Ch1DeckReplacementWidget.generated.h"

class UCardData;
class UJudgmentCardWidget;
class SWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCh1DeckReplacementResolved, UCardData*, NewCard, UCardData*, ReplacedCard);

UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API UCh1DeckReplacementWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCh1DeckReplacementWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category="Ch1|Deck")
	void ConfigureReplacement(
		UCardData* InNewCard,
		const TArray<UCardData*>& InCurrentDeck,
		const TArray<UCardData*>& InReplacementCandidates,
		int32 InMaxDeckCards,
		int32 InSkipCash);

	UPROPERTY(BlueprintAssignable, Category="Ch1|Deck")
	FOnCh1DeckReplacementResolved OnResolved;

	UPROPERTY(EditDefaultsOnly, Category="Ch1|Deck")
	TSubclassOf<UJudgmentCardWidget> CardWidgetClass;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY()
	TObjectPtr<UCardData> NewCard = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UCardData>> CurrentDeck;

	UPROPERTY()
	TArray<TObjectPtr<UCardData>> ReplacementCandidates;

	UPROPERTY()
	TArray<TObjectPtr<UJudgmentCardWidget>> PreviewWidgets;

	int32 MaxDeckCards = 4;
	int32 SkipCash = 0;

	TSharedRef<SWidget> BuildCardPreview(UCardData* Card, FVector2D Size, bool bLarge);
	TSharedRef<SWidget> BuildReplacementButton(UCardData* Card);
	FText BuildCardName(UCardData* Card) const;
	FText BuildCardRule(UCardData* Card) const;
	FText BuildPlainRuleText(const FText& MarkupText) const;
	bool IsReplacementCandidate(UCardData* Card) const;
	FReply ResolveWithReplacement(UCardData* ReplacedCard);
	FReply ResolveSkip();
};
