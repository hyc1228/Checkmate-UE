// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "Ch1ScoreHudWidget.h"

#include "CheckmateFonts.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

void UCh1ScoreHudWidget::SetScoreState(int32 InDayNumber, int32 InMoney, int32 InQuota, int32 InProcessed, int32 InTarget)
{
	DayNumber = FMath::Max(1, InDayNumber);
	Money = InMoney;
	Quota = InQuota;
	Processed = InProcessed;
	Target = InTarget;
	RefreshTexts();
}

void UCh1ScoreHudWidget::PlayScoreEvent(int32 InDelta, int32 InMoney, int32 InQuota, int32 InCombo, float InMultiplier)
{
	LastDelta = InDelta;
	Money = InMoney;
	Quota = InQuota;
	Combo = InCombo;
	Multiplier = FMath::Max(1.0f, InMultiplier);
	PulseElapsed = 0.0f;
	RefreshTexts();
}

TSharedRef<SWidget> UCh1ScoreHudWidget::RebuildWidget()
{
	return SNew(SOverlay)
	.Visibility(EVisibility::HitTestInvisible)

	+ SOverlay::Slot()
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Top)
	.Padding(FMargin(0.0f, 18.0f, 0.0f, 0.0f))
	[
		SNew(SBox)
		.WidthOverride(420.0f)
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor(0.010f, 0.008f, 0.010f, 0.74f))
			.Padding(FMargin(18.0f, 8.0f))
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(DayText, STextBlock)
					.ColorAndOpacity(FLinearColor(0.86f, 0.12f, 0.16f, 1.0f))
					.Font(CheckmateFonts::Title(20))
					.Justification(ETextJustify::Center)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 3.0f, 0.0f, 0.0f)
				[
					SAssignNew(MoneyText, STextBlock)
					.ColorAndOpacity(FLinearColor(0.92f, 0.90f, 0.86f, 0.94f))
					.Font(CheckmateFonts::BodyBold(17))
					.Justification(ETextJustify::Center)
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 1.0f, 0.0f, 0.0f)
				[
					SAssignNew(QuotaText, STextBlock)
					.ColorAndOpacity(FLinearColor(0.56f, 0.60f, 0.62f, 0.88f))
					.Font(CheckmateFonts::BodyLight(12))
					.Justification(ETextJustify::Center)
				]
			]
		]
	]

	+ SOverlay::Slot()
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(SBox)
		.WidthOverride(560.0f)
		.Visibility_Lambda([this]() { return GetDeltaVisibility(); })
		.RenderTransform_Lambda([this]()
		{
			return FSlateRenderTransform(FScale2D(GetPulseScale()), GetPulseTranslation());
		})
		.RenderTransformPivot(FVector2D(0.5f, 0.5f))
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(DeltaText, STextBlock)
				.ColorAndOpacity_Lambda([this]()
				{
					FLinearColor Color = LastDelta >= 0
						? FLinearColor(0.96f, 0.06f, 0.12f, 1.0f)
						: FLinearColor(0.58f, 0.04f, 0.07f, 1.0f);
					Color.A = GetPulseOpacity();
					return Color;
				})
				.Font(CheckmateFonts::BodyBold(76))
				.ShadowOffset(FVector2D(4.0f, 5.0f))
				.ShadowColorAndOpacity(FLinearColor(0.02f, 0.0f, 0.0f, 0.90f))
				.Justification(ETextJustify::Center)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, -6.0f, 0.0f, 0.0f)
			[
				SAssignNew(ComboText, STextBlock)
				.ColorAndOpacity_Lambda([this]()
				{
					return FLinearColor(0.92f, 0.90f, 0.86f, GetPulseOpacity());
				})
				.Font(CheckmateFonts::BodyBold(28))
				.ShadowOffset(FVector2D(2.0f, 3.0f))
				.ShadowColorAndOpacity(FLinearColor(0.02f, 0.0f, 0.0f, 0.78f))
				.Justification(ETextJustify::Center)
			]
		]
	];
}

void UCh1ScoreHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (PulseElapsed < PulseDuration)
	{
		PulseElapsed += InDeltaTime;
		InvalidateLayoutAndVolatility();
	}
}

FVector2D UCh1ScoreHudWidget::GetPulseScale() const
{
	if (PulseElapsed >= PulseDuration)
	{
		return FVector2D(1.0f, 1.0f);
	}

	const float T = FMath::Clamp(PulseElapsed / FMath::Max(0.01f, PulseDuration), 0.0f, 1.0f);
	const float Pop = FMath::Sin(T * PI) * (LastDelta >= 0 ? 0.42f : 0.20f);
	const float Kick = FMath::Sin(T * PI * 11.0f) * (1.0f - T) * 0.075f;
	return FVector2D(1.0f + Pop + Kick, 1.0f + Pop * 0.72f - Kick * 0.45f);
}

FVector2D UCh1ScoreHudWidget::GetPulseTranslation() const
{
	if (PulseElapsed >= PulseDuration)
	{
		return FVector2D::ZeroVector;
	}

	const float T = FMath::Clamp(PulseElapsed / FMath::Max(0.01f, PulseDuration), 0.0f, 1.0f);
	return FVector2D(
		FMath::Sin(T * PI * 22.0f) * (1.0f - T) * 18.0f,
		-FMath::Sin(T * PI) * 34.0f);
}

float UCh1ScoreHudWidget::GetPulseOpacity() const
{
	if (PulseElapsed >= PulseDuration)
	{
		return 0.0f;
	}

	const float T = FMath::Clamp(PulseElapsed / FMath::Max(0.01f, PulseDuration), 0.0f, 1.0f);
	if (T < 0.82f)
	{
		return 1.0f;
	}
	return 1.0f - FMath::Clamp((T - 0.82f) / 0.18f, 0.0f, 1.0f);
}

EVisibility UCh1ScoreHudWidget::GetDeltaVisibility() const
{
	return PulseElapsed < PulseDuration ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
}

void UCh1ScoreHudWidget::RefreshTexts()
{
	if (DayText)
	{
		DayText->SetText(FText::FromString(FString::Printf(TEXT("DAY %02d"), DayNumber)));
	}
	if (MoneyText)
	{
		MoneyText->SetText(FText::FromString(FString::Printf(TEXT("TOTAL $%d"), Money)));
	}
	if (QuotaText)
	{
		const FString QuotaLine = Target > 0
			? FString::Printf(TEXT("quota $%d  |  %d / %d dolls"), Quota, Processed, Target)
			: FString::Printf(TEXT("quota $%d"), Quota);
		QuotaText->SetText(FText::FromString(QuotaLine));
	}
	if (DeltaText)
	{
		DeltaText->SetText(FText::FromString(FString::Printf(TEXT("%s $%d"),
			LastDelta >= 0 ? TEXT("+") : TEXT("-"),
			FMath::Abs(LastDelta))));
	}
	if (ComboText)
	{
		ComboText->SetText(Multiplier >= 1.10f
			? FText::FromString(FString::Printf(TEXT("x%.2f"), Multiplier))
			: FText::GetEmpty());
	}
}
