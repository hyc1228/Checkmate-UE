// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "Ch2BeatPanelWidget.h"

#include "CheckmateFonts.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

void UCh2BeatPanelWidget::ConfigurePanel(const FCh2BeatPanelPayload& InPayload, float InLifetimeSeconds)
{
	Payload = InPayload;
	LifetimeSeconds = FMath::Max(0.35f, InLifetimeSeconds);
	ElapsedSeconds = 0.0f;
	DismissElapsedSeconds = 0.0f;
	bDismissing = false;
	bRevealBroadcast = false;

	ApplyPayloadToSlate();
}

void UCh2BeatPanelWidget::StartDismiss()
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

TSharedRef<SWidget> UCh2BeatPanelWidget::RebuildWidget()
{
	const FSlateFontInfo EyebrowFont = CheckmateFonts::BodyLight(14);
	const FSlateFontInfo TitleFont = CheckmateFonts::Title(38);
	const FSlateFontInfo BodyFont = CheckmateFonts::Body(18);

	TSharedRef<SWidget> Root =
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.01f, 0.012f, 0.016f, 0.58f))
			.Padding(0.0f)
		]
		+ SOverlay::Slot()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(720.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.025f, 0.030f, 0.036f, 0.92f))
				.Padding(FMargin(34.0f, 24.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(EyebrowTextBlock, STextBlock)
						.Font(EyebrowFont)
						.ColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.92f, 1.0f, 1.0f)))
						.Justification(ETextJustify::Center)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 10.0f, 0.0f, 9.0f)
					[
						SAssignNew(TitleTextBlock, STextBlock)
						.Font(TitleFont)
						.ColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.88f, 0.58f, 1.0f)))
						.Justification(ETextJustify::Center)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(96.0f, 2.0f, 96.0f, 16.0f)
					[
						SNew(SSeparator)
						.ColorAndOpacity(FLinearColor(0.32f, 0.88f, 1.0f, 0.82f))
						.Thickness(1.0f)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(BodyTextBlock, STextBlock)
						.Font(BodyFont)
						.ColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.84f, 0.86f, 1.0f)))
						.Justification(ETextJustify::Center)
						.AutoWrapText(true)
					]
				]
			]
		];

	ApplyPayloadToSlate();
	return Root;
}

void UCh2BeatPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!bRevealBroadcast)
	{
		bRevealBroadcast = true;
		OnRevealCue.Broadcast(Payload);
		K2_PlayRevealEffect(Payload);
	}
}

void UCh2BeatPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	ElapsedSeconds += InDeltaTime;

	float Alpha = 1.0f;
	float Ease = 1.0f;
	if (bDismissing)
	{
		DismissElapsedSeconds += InDeltaTime;
		const float OutT = FMath::Clamp(DismissElapsedSeconds / 0.20f, 0.0f, 1.0f);
		Alpha = 1.0f - OutT;
		Ease = 1.0f - OutT;
	}
	else
	{
		const float InT = FMath::Clamp(ElapsedSeconds / 0.26f, 0.0f, 1.0f);
		Ease = 1.0f - FMath::Pow(1.0f - InT, 3.0f);
		Alpha = Ease;
	}

	SetRenderOpacity(Alpha);

	FWidgetTransform Transform;
	Transform.Translation = FVector2D(0.0f, 14.0f * (1.0f - Ease));
	Transform.Scale = FVector2D(0.955f + 0.045f * Ease, 0.955f + 0.045f * Ease);
	SetRenderTransform(Transform);

	if (bDismissing && DismissElapsedSeconds >= 0.22f)
	{
		RemoveFromParent();
	}
}

void UCh2BeatPanelWidget::ApplyPayloadToSlate()
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
