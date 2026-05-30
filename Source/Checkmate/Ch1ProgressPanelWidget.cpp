// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "Ch1ProgressPanelWidget.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

void UCh1ProgressPanelWidget::ConfigurePanel(const FCh1ProgressPanelPayload& InPayload, float InLifetimeSeconds)
{
	Payload = InPayload;
	LifetimeSeconds = FMath::Max(0.5f, InLifetimeSeconds);
	ElapsedSeconds = 0.0f;
	DismissElapsedSeconds = 0.0f;
	bDismissing = false;
	bRevealBroadcast = false;

	ApplyPayloadToSlate();
}

void UCh1ProgressPanelWidget::StartDismiss()
{
	if (bDismissing)
	{
		return;
	}

	bDismissing = true;
	DismissElapsedSeconds = 0.0f;
	OnDismissCue.Broadcast(Payload);
	K2_PlayDismissEffect(Payload);
}

TSharedRef<SWidget> UCh1ProgressPanelWidget::RebuildWidget()
{
	const FSlateFontInfo EyebrowFont = FCoreStyle::GetDefaultFontStyle("Regular", 15);
	const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 42);
	const FSlateFontInfo BodyFont = FCoreStyle::GetDefaultFontStyle("Regular", 19);

	TSharedRef<SWidget> Root =
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.015f, 0.014f, 0.012f, 0.78f))
			.Padding(0.0f)
		]
		+ SOverlay::Slot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(780.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.050f, 0.045f, 0.038f, 0.92f))
				.Padding(FMargin(36.0f, 28.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(EyebrowTextBlock, STextBlock)
						.Font(EyebrowFont)
						.ColorAndOpacity(FSlateColor(FLinearColor(0.73f, 0.65f, 0.50f, 1.0f)))
						.Justification(ETextJustify::Center)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 12.0f, 0.0f, 10.0f)
					[
						SAssignNew(TitleTextBlock, STextBlock)
						.Font(TitleFont)
						.ColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.92f, 0.82f, 1.0f)))
						.Justification(ETextJustify::Center)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(80.0f, 2.0f, 80.0f, 18.0f)
					[
						SNew(SSeparator)
						.ColorAndOpacity(FLinearColor(0.72f, 0.60f, 0.39f, 0.78f))
						.Thickness(1.0f)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(BodyTextBlock, STextBlock)
						.Font(BodyFont)
						.ColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.75f, 0.68f, 1.0f)))
						.Justification(ETextJustify::Center)
						.AutoWrapText(true)
					]
				]
			]
		];

	ApplyPayloadToSlate();
	return Root;
}

void UCh1ProgressPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!bRevealBroadcast)
	{
		bRevealBroadcast = true;
		OnRevealCue.Broadcast(Payload);
		K2_PlayRevealEffect(Payload);
	}
}

void UCh1ProgressPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	ElapsedSeconds += InDeltaTime;

	float Alpha = 1.0f;
	float Ease = 1.0f;
	if (bDismissing)
	{
		DismissElapsedSeconds += InDeltaTime;
		const float OutT = FMath::Clamp(DismissElapsedSeconds / 0.24f, 0.0f, 1.0f);
		Alpha = 1.0f - OutT;
		Ease = 1.0f - OutT;
	}
	else
	{
		const float InT = FMath::Clamp(ElapsedSeconds / 0.32f, 0.0f, 1.0f);
		Ease = 1.0f - FMath::Pow(1.0f - InT, 3.0f);
		Alpha = Ease;
	}

	SetRenderOpacity(Alpha);

	FWidgetTransform Transform;
	Transform.Translation = FVector2D(0.0f, 18.0f * (1.0f - Ease));
	Transform.Scale = FVector2D(0.965f + 0.035f * Ease, 0.965f + 0.035f * Ease);
	SetRenderTransform(Transform);
}

void UCh1ProgressPanelWidget::ApplyPayloadToSlate()
{
	if (EyebrowTextBlock.IsValid())
	{
		EyebrowTextBlock->SetText(Payload.Eyebrow);
	}
	if (TitleTextBlock.IsValid())
	{
		TitleTextBlock->SetText(Payload.Title);
	}
	if (BodyTextBlock.IsValid())
	{
		BodyTextBlock->SetText(Payload.Body);
	}
}
