// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "TitlePromptWidget.h"

#include "CheckmateFonts.h"
#include "TitleScreenGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

void UTitlePromptWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Visible);
	UpdateFade(0.0f);
}

void UTitlePromptWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bFadingToBlack)
	{
		return;
	}

	FadeElapsedSeconds += InDeltaTime;
	const float Alpha = FadeDurationSeconds > 0.0f
		? FMath::Clamp(FadeElapsedSeconds / FadeDurationSeconds, 0.0f, 1.0f)
		: 1.0f;
	UpdateFade(Alpha);

	if (Alpha >= 1.0f)
	{
		bFadingToBlack = false;
	}
}

TSharedRef<SWidget> UTitlePromptWidget::RebuildWidget()
{
	return SNew(SOverlay)

	+ SOverlay::Slot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f))
	]

	+ SOverlay::Slot()
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Bottom)
	.Padding(FMargin(0.0f, 0.0f, 0.0f, 52.0f))
	[
		SNew(SBorder)
		.BorderBackgroundColor(FLinearColor(0.02f, 0.018f, 0.014f, 0.56f))
		.Padding(FMargin(22.0f, 9.0f))
		[
			SAssignNew(PromptText, STextBlock)
			.Text(FText::FromString(TEXT("Press any button to start")))
			.ColorAndOpacity(FLinearColor(0.94f, 0.90f, 0.80f, 1.0f))
			.Font(CheckmateFonts::BodyBold(22))
			.Justification(ETextJustify::Center)
		]
	]

	+ SOverlay::Slot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Fill)
	[
		SAssignNew(FadeBorder, SBorder)
		.Visibility(EVisibility::HitTestInvisible)
		.BorderBackgroundColor(FLinearColor::Transparent)
	];
}

FReply UTitlePromptWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	RequestStart();
	return FReply::Handled();
}

FReply UTitlePromptWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	RequestStart();
	return FReply::Handled();
}

void UTitlePromptWidget::SetStarted(bool bInStarted)
{
	bStarted = bInStarted;
	if (PromptText)
	{
		PromptText->SetVisibility(bStarted ? EVisibility::Collapsed : EVisibility::HitTestInvisible);
	}
}

void UTitlePromptWidget::StartFadeToBlack(float DurationSeconds)
{
	bFadingToBlack = true;
	FadeDurationSeconds = FMath::Max(0.0f, DurationSeconds);
	FadeElapsedSeconds = 0.0f;
	UpdateFade(FadeDurationSeconds <= 0.0f ? 1.0f : 0.0f);
}

void UTitlePromptWidget::RequestStart()
{
	if (bStarted)
	{
		return;
	}

	if (ATitleScreenGameMode* TitleGameMode = Cast<ATitleScreenGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		TitleGameMode->StartTitleFall();
	}
}

void UTitlePromptWidget::UpdateFade(float Alpha)
{
	if (FadeBorder)
	{
		FadeBorder->SetBorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, Alpha));
	}
}
