// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "Ch1DeckReplacementWidget.h"

#include "CardData.h"
#include "CheckmateFonts.h"
#include "JudgmentCardWidget.h"

#include "Styling/CoreStyle.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	FString StripInlineMarkupTags(const FString& Source)
	{
		FString Out;
		Out.Reserve(Source.Len());

		bool bInsideTag = false;
		for (int32 Index = 0; Index < Source.Len(); ++Index)
		{
			const TCHAR Ch = Source[Index];
			if (Ch == TEXT('<'))
			{
				bInsideTag = true;
				continue;
			}
			if (Ch == TEXT('>') && bInsideTag)
			{
				bInsideTag = false;
				continue;
			}
			if (!bInsideTag)
			{
				Out.AppendChar(Ch);
			}
		}

		Out.ReplaceInline(TEXT("  "), TEXT(" "));
		return Out.TrimStartAndEnd();
	}
}

UCh1DeckReplacementWidget::UCh1DeckReplacementWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FClassFinder<UJudgmentCardWidget> DefaultCardWidgetClass(TEXT("/Game/Ch1/UI/WBP_JudgmentCard"));
	if (DefaultCardWidgetClass.Succeeded())
	{
		CardWidgetClass = DefaultCardWidgetClass.Class;
	}
}

void UCh1DeckReplacementWidget::ConfigureReplacement(
	UCardData* InNewCard,
	const TArray<UCardData*>& InCurrentDeck,
	const TArray<UCardData*>& InReplacementCandidates,
	int32 InMaxDeckCards,
	int32 InSkipCash)
{
	NewCard = InNewCard;
	CurrentDeck.Reset();
	ReplacementCandidates.Reset();

	for (UCardData* Card : InCurrentDeck)
	{
		if (Card)
		{
			CurrentDeck.Add(Card);
		}
	}
	for (UCardData* Card : InReplacementCandidates)
	{
		if (Card)
		{
			ReplacementCandidates.Add(Card);
		}
	}

	MaxDeckCards = FMath::Max(1, InMaxDeckCards);
	SkipCash = FMath::Max(0, InSkipCash);
}

TSharedRef<SWidget> UCh1DeckReplacementWidget::RebuildWidget()
{
	PreviewWidgets.Reset();

	TSharedRef<SHorizontalBox> DeckRow = SNew(SHorizontalBox);
	for (UCardData* Card : CurrentDeck)
	{
		DeckRow->AddSlot()
		.AutoWidth()
		.Padding(FMargin(8.0f, 0.0f))
		[
			BuildReplacementButton(Card)
		];
	}

	const FString CapacityLine = FString::Printf(
		TEXT("Deck capacity %d / %d. Choose one old card to throw away, or skip this draft."),
		CurrentDeck.Num(),
		MaxDeckCards);

	return SNew(SOverlay)
	+ SOverlay::Slot()
	[
		SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.010f, 0.009f, 0.008f, 0.88f))
	]
	+ SOverlay::Slot()
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(SBox)
		.WidthOverride(980.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.050f, 0.040f, 0.032f, 0.96f))
			.Padding(FMargin(28.0f, 24.0f))
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("REPLACE A STANDARD")))
					.ColorAndOpacity(FLinearColor(0.98f, 0.86f, 0.55f, 1.0f))
					.Font(CheckmateFonts::Title(30))
					.Justification(ETextJustify::Center)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(54.0f, 10.0f, 54.0f, 18.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(CapacityLine))
					.ColorAndOpacity(FLinearColor(0.78f, 0.74f, 0.66f, 1.0f))
					.Font(CheckmateFonts::Body(16))
					.Justification(ETextJustify::Center)
					.AutoWrapText(true)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 18.0f)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 8.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Incoming card")))
							.ColorAndOpacity(FLinearColor(0.94f, 0.82f, 0.54f, 1.0f))
							.Font(CheckmateFonts::BodyBold(15))
							.Justification(ETextJustify::Center)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							BuildCardPreview(NewCard, FVector2D(178.0f, 252.0f), true)
						]
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					.Padding(24.0f, 0.0f)
					[
						SNew(SSeparator)
						.Thickness(1.0f)
						.ColorAndOpacity(FLinearColor(0.56f, 0.44f, 0.28f, 0.65f))
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 0.0f, 0.0f, 8.0f)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("Throw away one current card")))
							.ColorAndOpacity(FLinearColor(0.94f, 0.82f, 0.54f, 1.0f))
							.Font(CheckmateFonts::BodyBold(15))
							.Justification(ETextJustify::Center)
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							DeckRow
						]
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(SButton)
					.ButtonColorAndOpacity(FLinearColor(0.16f, 0.13f, 0.10f, 1.0f))
					.OnClicked(FOnClicked::CreateUObject(this, &UCh1DeckReplacementWidget::ResolveSkip))
					[
						SNew(SBox)
						.WidthOverride(260.0f)
						.HeightOverride(42.0f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(SkipCash > 0
								? FString::Printf(TEXT("Skip card  +$%d buffer"), SkipCash)
								: FString(TEXT("Skip card"))))
							.ColorAndOpacity(FLinearColor(0.92f, 0.87f, 0.75f, 1.0f))
							.Font(CheckmateFonts::BodyBold(15))
							.Justification(ETextJustify::Center)
						]
					]
				]
			]
		]
	];
}

void UCh1DeckReplacementWidget::NativeDestruct()
{
	OnResolved.Clear();
	Super::NativeDestruct();
}

TSharedRef<SWidget> UCh1DeckReplacementWidget::BuildCardPreview(UCardData* Card, FVector2D Size, bool bLarge)
{
	if (CardWidgetClass && Card)
	{
		if (UJudgmentCardWidget* CardWidget = CreateWidget<UJudgmentCardWidget>(this, CardWidgetClass))
		{
			CardWidget->FallbackCardImageSize = Size;
			CardWidget->HoverLiftPixels = bLarge ? 14.0f : 8.0f;
			CardWidget->HoverScaleMultiplier = bLarge ? 1.06f : 1.04f;
			CardWidget->SelectedLiftPixels = 0.0f;
			CardWidget->SetCardData(Card);
			PreviewWidgets.Add(CardWidget);
			return SNew(SBox)
			.WidthOverride(Size.X)
			.HeightOverride(Size.Y)
			[
				SNew(SScaleBox)
				.Stretch(EStretch::ScaleToFit)
				[
					CardWidget->TakeWidget()
				]
			];
		}
	}

	return SNew(SBox)
	.WidthOverride(Size.X)
	.HeightOverride(Size.Y)
	[
		SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.12f, 0.09f, 0.06f, 1.0f))
		.Padding(FMargin(10.0f))
		[
			SNew(STextBlock)
			.Text(BuildCardName(Card))
			.ColorAndOpacity(FLinearColor(0.95f, 0.89f, 0.76f, 1.0f))
			.Font(CheckmateFonts::BodyBold(bLarge ? 18 : 13))
			.Justification(ETextJustify::Center)
			.AutoWrapText(true)
		]
	];
}

TSharedRef<SWidget> UCh1DeckReplacementWidget::BuildReplacementButton(UCardData* Card)
{
	const bool bCandidate = IsReplacementCandidate(Card);
	const FLinearColor ButtonTint = bCandidate
		? FLinearColor(0.20f, 0.16f, 0.12f, 1.0f)
		: FLinearColor(0.06f, 0.05f, 0.045f, 0.75f);

	return SNew(SButton)
	.IsEnabled(bCandidate)
	.ButtonColorAndOpacity(ButtonTint)
	.OnClicked(FOnClicked::CreateUObject(this, &UCh1DeckReplacementWidget::ResolveWithReplacement, Card))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		[
			BuildCardPreview(Card, FVector2D(116.0f, 162.0f), false)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 7.0f, 0.0f, 0.0f)
		[
			SNew(SBox)
			.WidthOverride(132.0f)
			[
				SNew(STextBlock)
				.Text(BuildCardRule(Card))
				.ColorAndOpacity(bCandidate
					? FLinearColor(0.88f, 0.83f, 0.72f, 1.0f)
					: FLinearColor(0.45f, 0.42f, 0.37f, 1.0f))
				.Font(CheckmateFonts::BodyLight(11))
				.Justification(ETextJustify::Center)
				.AutoWrapText(true)
			]
		]
	];
}

FText UCh1DeckReplacementWidget::BuildCardName(UCardData* Card) const
{
	if (!Card)
	{
		return FText::FromString(TEXT("Empty"));
	}
	if (!Card->EnglishDisplayName.IsEmpty())
	{
		return Card->EnglishDisplayName;
	}
	if (!Card->DisplayLabel.IsEmpty())
	{
		return Card->DisplayLabel;
	}
	return FText::FromName(Card->CardId);
}

FText UCh1DeckReplacementWidget::BuildCardRule(UCardData* Card) const
{
	if (!Card)
	{
		return FText::GetEmpty();
	}
	if (!Card->SkillDescriptionMarkup.IsEmpty())
	{
		return BuildPlainRuleText(Card->SkillDescriptionMarkup);
	}
	return BuildCardName(Card);
}

FText UCh1DeckReplacementWidget::BuildPlainRuleText(const FText& MarkupText) const
{
	return FText::FromString(StripInlineMarkupTags(MarkupText.ToString()));
}

bool UCh1DeckReplacementWidget::IsReplacementCandidate(UCardData* Card) const
{
	return Card && ReplacementCandidates.Contains(Card);
}

FReply UCh1DeckReplacementWidget::ResolveWithReplacement(UCardData* ReplacedCard)
{
	if (IsReplacementCandidate(ReplacedCard))
	{
		OnResolved.Broadcast(NewCard, ReplacedCard);
	}
	return FReply::Handled();
}

FReply UCh1DeckReplacementWidget::ResolveSkip()
{
	OnResolved.Broadcast(NewCard, nullptr);
	return FReply::Handled();
}
