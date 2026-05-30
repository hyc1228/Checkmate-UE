// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "Ch1SelectedCardsBoardWidget.h"

#include "CardData.h"
#include "Ch1LocSubsystem.h"
#include "JudgmentCardWidget.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SCanvas.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "UObject/ConstructorHelpers.h"

UCh1SelectedCardsBoardWidget::UCh1SelectedCardsBoardWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FClassFinder<UJudgmentCardWidget> DefaultCardWidgetClass(TEXT("/Game/Ch1/UI/WBP_JudgmentCard"));
	if (DefaultCardWidgetClass.Succeeded())
	{
		CardWidgetClass = DefaultCardWidgetClass.Class;
	}
}

void UCh1SelectedCardsBoardWidget::SetCards(TArray<UCardData*> InCards)
{
	Cards.Reset();
	Cards.Reserve(InCards.Num());
	for (UCardData* Card : InCards)
	{
		if (Card)
		{
			Cards.Add(Card);
		}
	}

	CurrentHoveredCard = nullptr;
	HandleCardUnhovered();
}

void UCh1SelectedCardsBoardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UCh1LocSubsystem* Loc = GetLoc())
	{
		LangChangedHandle = Loc->OnLanguageChanged.AddUObject(this, &UCh1SelectedCardsBoardWidget::RefreshLocalizedTexts);
	}
	RefreshLocalizedTexts();
}

void UCh1SelectedCardsBoardWidget::NativeDestruct()
{
	if (UCh1LocSubsystem* Loc = GetLoc())
	{
		Loc->OnLanguageChanged.Remove(LangChangedHandle);
	}

	Super::NativeDestruct();
}

TSharedRef<SWidget> UCh1SelectedCardsBoardWidget::RebuildWidget()
{
	CardWidgets.Reset();

	return SNew(SOverlay)
	+ SOverlay::Slot()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Top)
	.Padding(FMargin(26.0f, 22.0f, 0.0f, 0.0f))
	[
		SNew(SBox)
		.WidthOverride(330.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.HAlign(HAlign_Left)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("|")))
					.ColorAndOpacity(FLinearColor(0.30f, 0.24f, 0.18f, 1.0f))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.HAlign(HAlign_Right)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("|")))
					.ColorAndOpacity(FLinearColor(0.30f, 0.24f, 0.18f, 1.0f))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.12f, 0.085f, 0.052f, 0.94f))
				.Padding(FMargin(14.0f, 12.0f))
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						SAssignNew(TitleText, STextBlock)
						.Text(LocText(TEXT("SelectedCardsBoard.Title"), TEXT("Selected cards")))
						.ColorAndOpacity(FLinearColor(0.94f, 0.84f, 0.62f, 1.0f))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 17))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						BuildCardTray()
					]
				]
			]
		]
	];
}

TSharedRef<SWidget> UCh1SelectedCardsBoardWidget::BuildCardTray()
{
	const int32 VisibleCardCount = FMath::Min(Cards.Num(), 3);
	TSharedRef<SCanvas> CardCanvas = SNew(SCanvas);

	for (int32 Index = 0; Index < VisibleCardCount; ++Index)
	{
		CardCanvas->AddSlot()
		.Position(FVector2D(1.0f + Index * 82.0f, 0.0f))
		.Size(FVector2D(126.0f, 232.0f))
		[
			BuildCardSlot(Cards[Index], Index, VisibleCardCount)
		];
	}

	return SNew(SBox)
	.WidthOverride(286.0f)
	.HeightOverride(232.0f)
	[
		SNew(SOverlay)

		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.055f, 0.045f, 0.036f, 0.84f))
			.Padding(FMargin(8.0f, 36.0f, 8.0f, 34.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(FMargin(3.0f, 2.0f))
				[
					SNew(SBorder)
					.BorderBackgroundColor(FLinearColor(0.20f, 0.15f, 0.10f, 0.38f))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(FMargin(3.0f, 2.0f))
				[
					SNew(SBorder)
					.BorderBackgroundColor(FLinearColor(0.20f, 0.15f, 0.10f, 0.38f))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(FMargin(3.0f, 2.0f))
				[
					SNew(SBorder)
					.BorderBackgroundColor(FLinearColor(0.20f, 0.15f, 0.10f, 0.38f))
				]
			]
		]

		+ SOverlay::Slot()
		[
			CardCanvas
		]
	];
}

TSharedRef<SWidget> UCh1SelectedCardsBoardWidget::BuildCardSlot(UCardData* Card, int32 CardIndex, int32 VisibleCardCount)
{
	const TWeakObjectPtr<UCh1SelectedCardsBoardWidget> WeakThis(this);
	const TWeakObjectPtr<UCardData> WeakCard(Card);
	TSharedRef<SWidget> CardFace = SNew(SBorder)
	.BorderBackgroundColor(FLinearColor(0.12f, 0.085f, 0.052f, 0.92f))
	.Padding(FMargin(8.0f))
	[
		SNew(STextBlock)
		.Text(Card && !Card->DisplayLabel.IsEmpty() ? Card->DisplayLabel : FText::FromName(Card ? Card->CardId : NAME_None))
		.ColorAndOpacity(FLinearColor(0.96f, 0.92f, 0.82f, 1.0f))
		.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
		.WrapTextAt(96.0f)
	];

	if (CardWidgetClass)
	{
		if (UJudgmentCardWidget* CardWidget = CreateWidget<UJudgmentCardWidget>(this, CardWidgetClass))
		{
			CardWidget->FallbackCardImageSize = FVector2D(112.0f, 154.0f);
			CardWidget->HoverLiftPixels = 10.0f;
			CardWidget->HoverScaleMultiplier = 1.06f;
			CardWidget->SelectedLiftPixels = 0.0f;
			CardWidget->SetCardData(Card);
			CardWidget->SetBaseFanAngle((CardIndex - (VisibleCardCount - 1) * 0.5f) * 2.5f);
			CardWidget->OnCardHoverStarted.AddDynamic(this, &UCh1SelectedCardsBoardWidget::HandleCardWidgetHovered);
			CardWidget->OnCardHoverEnded.AddDynamic(this, &UCh1SelectedCardsBoardWidget::HandleCardWidgetUnhovered);
			CardWidgets.Add(CardWidget);
			CardFace = SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFit)
			.StretchDirection(EStretchDirection::Both)
			[
				CardWidget->TakeWidget()
			];
		}
	}

	return SNew(SOverlay)

	+ SOverlay::Slot()
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Top)
	.Padding(FMargin(0.0f, 0.0f, 0.0f, 0.0f))
	[
		SNew(SBorder)
		.Visibility_Lambda([WeakThis, WeakCard]()
		{
			const UCh1SelectedCardsBoardWidget* Self = WeakThis.Get();
			return Self && Self->CurrentHoveredCard == WeakCard.Get()
				? EVisibility::HitTestInvisible
				: EVisibility::Hidden;
		})
		.BorderBackgroundColor(FLinearColor(0.02f, 0.018f, 0.014f, 0.88f))
		.Padding(FMargin(6.0f, 3.0f))
		[
			SNew(STextBlock)
			.Text_Lambda([WeakThis, WeakCard]()
			{
				if (const UCh1SelectedCardsBoardWidget* Self = WeakThis.Get())
				{
					return Self->BuildCardNameText(WeakCard.Get());
				}
				return FText::GetEmpty();
			})
			.ColorAndOpacity(FLinearColor(0.98f, 0.88f, 0.58f, 1.0f))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
			.Justification(ETextJustify::Center)
		]
	]

	+ SOverlay::Slot()
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Top)
	.Padding(FMargin(0.0f, 30.0f, 0.0f, 0.0f))
	[
		SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.015f, 0.012f, 0.01f, 0.48f))
		.Padding(FMargin(3.0f))
		[
			SNew(SBox)
			.WidthOverride(104.0f)
			.HeightOverride(146.0f)
			[
				CardFace
			]
		]
	]

	+ SOverlay::Slot()
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Top)
	.Padding(FMargin(0.0f, 182.0f, 0.0f, 0.0f))
	[
		SNew(SBorder)
		.Visibility_Lambda([WeakThis, WeakCard]()
		{
			const UCh1SelectedCardsBoardWidget* Self = WeakThis.Get();
			return Self && Self->CurrentHoveredCard == WeakCard.Get()
				? EVisibility::HitTestInvisible
				: EVisibility::Hidden;
		})
		.BorderBackgroundColor(FLinearColor(0.02f, 0.018f, 0.014f, 0.90f))
		.Padding(FMargin(7.0f, 5.0f))
		[
			SNew(SBox)
			.WidthOverride(114.0f)
			[
				SNew(STextBlock)
				.Text_Lambda([WeakThis, WeakCard]()
				{
					if (const UCh1SelectedCardsBoardWidget* Self = WeakThis.Get())
					{
						return Self->BuildCardHoverDescriptionText(WeakCard.Get());
					}
					return FText::GetEmpty();
				})
				.ColorAndOpacity(FLinearColor(0.90f, 0.86f, 0.76f, 1.0f))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
				.Justification(ETextJustify::Center)
				.WrapTextAt(108.0f)
			]
		]
	];
}

FText UCh1SelectedCardsBoardWidget::BuildCardDetailText(const UCardData* Card) const
{
	if (!Card)
	{
		return BuildDefaultDetailText();
	}

	FString DimensionText;
	FString MatchText;
	switch (Card->Dimension)
	{
	case ECardDimension::Posture:
		DimensionText = TEXT("Posture");
		MatchText = FText::Format(
			LocText(TEXT("SelectedCardsBoard.Requires.Posture"), TEXT("Requires posture value: {0}")),
			FText::FromName(Card->PostureValue)).ToString();
		break;
	case ECardDimension::Hair:
		DimensionText = TEXT("Hair");
		MatchText = FText::Format(
			LocText(TEXT("SelectedCardsBoard.Requires.Hair"), TEXT("Requires hair trait: {0}")),
			FText::FromName(Card->CardId)).ToString();
		break;
	case ECardDimension::Expression:
		DimensionText = TEXT("Expression");
		MatchText = FText::Format(
			LocText(TEXT("SelectedCardsBoard.Requires.Expression"), TEXT("Requires expression trait: {0}")),
			FText::FromName(Card->CardId)).ToString();
		break;
	case ECardDimension::Accessory:
		DimensionText = TEXT("Accessory");
		MatchText = FText::Format(
			LocText(TEXT("SelectedCardsBoard.Requires.Accessory"), TEXT("Requires accessory trait: {0}")),
			FText::FromName(Card->CardId)).ToString();
		break;
	}

	const FString Label = Card->DisplayLabel.IsEmpty()
		? Card->CardId.ToString()
		: Card->DisplayLabel.ToString();
	const FString PearlLine = Card->bIsPearlCompatible
		? FString::Printf(TEXT("\n%s"), *LocText(TEXT("SelectedCardsBoard.PearlCompatible"), TEXT("Pearl-compatible.")).ToString())
		: TEXT("");

	return FText::Format(
		LocText(TEXT("SelectedCardsBoard.Detail"), TEXT("{0}\nType: {1}\n{2}{3}")),
		FText::FromString(Label),
		FText::FromString(DimensionText),
		FText::FromString(MatchText),
		FText::FromString(PearlLine));
}

FText UCh1SelectedCardsBoardWidget::BuildDefaultDetailText() const
{
	return LocText(TEXT("SelectedCardsBoard.HoverPrompt"), TEXT("Hover a stacked card to inspect its rule."));
}

FText UCh1SelectedCardsBoardWidget::BuildCardNameText(const UCardData* Card) const
{
	if (!Card)
	{
		return FText::GetEmpty();
	}

	const FString Fallback = HumanizeCardId(Card->CardId);
	return LocTextByString(FString::Printf(TEXT("Card.%s.Name"), *Card->CardId.ToString()), Fallback);
}

FText UCh1SelectedCardsBoardWidget::BuildCardHoverDescriptionText(const UCardData* Card) const
{
	if (!Card)
	{
		return FText::GetEmpty();
	}

	const FString Fallback = Card->DisplayLabel.IsEmpty()
		? BuildCardDetailText(Card).ToString()
		: Card->DisplayLabel.ToString();
	return LocTextByString(FString::Printf(TEXT("Card.%s.Description"), *Card->CardId.ToString()), Fallback);
}

FText UCh1SelectedCardsBoardWidget::LocText(FName Key, const TCHAR* Fallback) const
{
	if (UCh1LocSubsystem* Loc = GetLoc())
	{
		const FText Text = Loc->Get(Key);
		if (!Text.IsEmpty() && !Text.ToString().Equals(Key.ToString(), ESearchCase::CaseSensitive))
		{
			return Text;
		}
	}
	return FText::FromString(Fallback);
}

FText UCh1SelectedCardsBoardWidget::LocTextByString(const FString& Key, const FString& Fallback) const
{
	if (UCh1LocSubsystem* Loc = GetLoc())
	{
		const FName KeyName(*Key);
		const FText Text = Loc->Get(KeyName);
		if (!Text.IsEmpty() && !Text.ToString().Equals(Key, ESearchCase::CaseSensitive))
		{
			return Text;
		}
	}
	return FText::FromString(Fallback);
}

FString UCh1SelectedCardsBoardWidget::HumanizeCardId(FName CardId) const
{
	FString Source = CardId.ToString();
	FString Out;
	Out.Reserve(Source.Len() + 4);

	for (int32 Index = 0; Index < Source.Len(); ++Index)
	{
		const TCHAR Ch = Source[Index];
		const bool bInsertSpace = Index > 0
			&& FChar::IsUpper(Ch)
			&& (FChar::IsLower(Source[Index - 1]) || (Index + 1 < Source.Len() && FChar::IsLower(Source[Index + 1])));
		if (bInsertSpace)
		{
			Out.AppendChar(' ');
		}
		Out.AppendChar(Ch);
	}

	return Out.IsEmpty() ? Source : Out;
}

UCh1LocSubsystem* UCh1SelectedCardsBoardWidget::GetLoc() const
{
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSubsystem<UCh1LocSubsystem>();
		}
	}
	return nullptr;
}

void UCh1SelectedCardsBoardWidget::RefreshLocalizedTexts()
{
	if (TitleText)
	{
		TitleText->SetText(LocText(TEXT("SelectedCardsBoard.Title"), TEXT("Selected cards")));
	}
}

void UCh1SelectedCardsBoardWidget::HandleCardWidgetHovered(UJudgmentCardWidget* CardWidget)
{
	HandleCardHovered(CardWidget ? CardWidget->GetCardData() : nullptr);
}

void UCh1SelectedCardsBoardWidget::HandleCardWidgetUnhovered(UJudgmentCardWidget* CardWidget)
{
	HandleCardUnhovered();
}

void UCh1SelectedCardsBoardWidget::HandleCardHovered(UCardData* Card)
{
	CurrentHoveredCard = Card;
	InvalidateLayoutAndVolatility();
}

void UCh1SelectedCardsBoardWidget::HandleCardUnhovered()
{
	CurrentHoveredCard = nullptr;
	InvalidateLayoutAndVolatility();
}
