// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Ch1SelectedCardsBoardWidget.generated.h"

class UCardData;
class UCh1LocSubsystem;
class UJudgmentCardWidget;
class STextBlock;
class SWidget;

/**
 * Ch1 selected-card hanging board.
 *
 * Native top-left inspection board. It reuses WBP_JudgmentCard from card
 * selection, arranged as three fixed cards in one Balatro-like tray.
 */
UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API UCh1SelectedCardsBoardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCh1SelectedCardsBoardWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category="Ch1|Selected Cards")
	void SetCards(TArray<UCardData*> InCards);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UPROPERTY()
	TArray<TObjectPtr<UCardData>> Cards;

	UPROPERTY()
	TObjectPtr<UCardData> CurrentHoveredCard = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Ch1|Selected Cards")
	TSubclassOf<UJudgmentCardWidget> CardWidgetClass;

	UPROPERTY()
	TArray<TObjectPtr<UJudgmentCardWidget>> CardWidgets;

	TSharedPtr<STextBlock> TitleText;
	FDelegateHandle LangChangedHandle;

	TSharedRef<SWidget> BuildCardTray();
	TSharedRef<SWidget> BuildCardSlot(UCardData* Card, int32 CardIndex, int32 VisibleCardCount);
	FText BuildCardDetailText(const UCardData* Card) const;
	FText BuildCardNameText(const UCardData* Card) const;
	FText BuildCardHoverDescriptionText(const UCardData* Card) const;
	FText BuildDefaultDetailText() const;
	FText LocText(FName Key, const TCHAR* Fallback) const;
	FText LocTextByString(const FString& Key, const FString& Fallback) const;
	FString HumanizeCardId(FName CardId) const;
	UCh1LocSubsystem* GetLoc() const;
	void RefreshLocalizedTexts();
	UFUNCTION()
	void HandleCardWidgetHovered(UJudgmentCardWidget* CardWidget);
	UFUNCTION()
	void HandleCardWidgetUnhovered(UJudgmentCardWidget* CardWidget);
	void HandleCardHovered(UCardData* Card);
	void HandleCardUnhovered();
};
