// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "Ch1StarterStandardWidget.h"

#include "CheckmateFonts.h"

#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

UCh1StarterStandardWidget::UCh1StarterStandardWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlaceholderImage = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/Ch1/Art/Cards/T_Card_Empty.T_Card_Empty")));
}

TSharedRef<SWidget> UCh1StarterStandardWidget::RebuildWidget()
{
	UTexture2D* Texture = PlaceholderImage.LoadSynchronous();
	if (Texture)
	{
		PlaceholderBrush.SetResourceObject(Texture);
		PlaceholderBrush.ImageSize = FVector2D(420.0f, 560.0f);
	}

	return SNew(SOverlay)
	+ SOverlay::Slot()
	[
		SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.015f, 0.012f, 0.010f, 0.86f))
	]

	+ SOverlay::Slot()
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(SButton)
		.ButtonStyle(FCoreStyle::Get(), "NoBorder")
		.OnClicked_UObject(this, &UCh1StarterStandardWidget::HandleClicked)
		[
			SNew(SBox)
			.WidthOverride(520.0f)
			.HeightOverride(640.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.10f, 0.070f, 0.045f, 0.98f))
				.Padding(FMargin(22.0f))
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 14.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("STANDARD ISSUE")))
						.ColorAndOpacity(FLinearColor(0.98f, 0.86f, 0.58f, 1.0f))
						.Font(CheckmateFonts::Title(22))
						.Justification(ETextJustify::Center)
					]

					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SNew(SBorder)
						.BorderBackgroundColor(FLinearColor(0.030f, 0.026f, 0.022f, 1.0f))
						.Padding(FMargin(10.0f))
						[
							SNew(SScaleBox)
							.Stretch(EStretch::ScaleToFit)
							.StretchDirection(EStretchDirection::Both)
							[
								SNew(SImage)
								.Image(&PlaceholderBrush)
							]
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 16.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Smile  /  Graceful pose  /  Natural hair")))
						.ColorAndOpacity(FLinearColor(0.92f, 0.86f, 0.74f, 1.0f))
						.Font(CheckmateFonts::Body(16))
						.Justification(ETextJustify::Center)
					]
				]
			]
		]
	];
}

FReply UCh1StarterStandardWidget::HandleClicked()
{
	OnConfirmed.Broadcast();
	return FReply::Handled();
}
