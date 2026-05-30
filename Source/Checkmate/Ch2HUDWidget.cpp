// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "Ch2HUDWidget.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"

void UCh2HUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetMode(ECh2Mode::Ballet);
	SetHintMessage(TEXT("LMB 点亮起的格子移动；高亮格 = 合法目标"));

	if (FadeOverlay) FadeOverlay->SetBrushColor(FLinearColor(0, 0, 0, 0));
	if (RitualText)  RitualText->SetVisibility(ESlateVisibility::Hidden);
	if (VictoryText) VictoryText->SetVisibility(ESlateVisibility::Hidden);
}

void UCh2HUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bRitualActive)
	{
		RitualElapsed += InDeltaTime;
		const float T = FMath::Clamp(RitualElapsed / RitualDuration, 0.0f, 1.0f);
		const float Alpha = FMath::Sin(T * PI);  // 0→1→0

		if (FadeOverlay) FadeOverlay->SetBrushColor(FLinearColor(0, 0, 0, Alpha));
		if (RitualText)
		{
			RitualText->SetVisibility(Alpha > 0.5f ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		}

		if (T >= 1.0f)
		{
			bRitualActive = false;
			if (FadeOverlay) FadeOverlay->SetBrushColor(FLinearColor(0, 0, 0, 0));
			if (RitualText)  RitualText->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	ModePulseElapsed += InDeltaTime;
	HintPulseElapsed += InDeltaTime;

	if (ModeText)
	{
		const float T = FMath::Clamp(ModePulseElapsed / 0.28f, 0.0f, 1.0f);
		const float Pulse = FMath::Sin(T * PI);
		FWidgetTransform Transform;
		Transform.Scale = FVector2D(1.0f + Pulse * 0.08f, 1.0f + Pulse * 0.08f);
		Transform.Translation = FVector2D(0.0f, -4.0f * Pulse);
		ModeText->SetRenderTransform(Transform);
		ModeText->SetRenderOpacity(0.82f + Pulse * 0.18f);
	}
	if (HintText)
	{
		const float T = FMath::Clamp(HintPulseElapsed / 0.32f, 0.0f, 1.0f);
		const float Pulse = FMath::Sin(T * PI);
		HintText->SetRenderOpacity(0.66f + Pulse * 0.34f);
	}
}

void UCh2HUDWidget::PlayPickupRitual()
{
	bRitualActive = true;
	RitualElapsed = 0.0f;
	if (RitualText)
	{
		RitualText->SetText(FText::FromString(TEXT("—— 取扣 / 缝扣 ——")));
		RitualText->SetVisibility(ESlateVisibility::Visible);
	}
}

void UCh2HUDWidget::ShowVictory()
{
	if (VictoryText)
	{
		VictoryText->SetText(FText::FromString(TEXT("Ch2 Complete\nR 重启关卡")));
		VictoryText->SetVisibility(ESlateVisibility::Visible);
	}
}

void UCh2HUDWidget::SetMoveCounter(int32 Current, int32 Budget)
{
	if (!MoveCounterText) return;
	FString Msg;
	if (Budget > 0)
	{
		const int32 Remaining = FMath::Max(0, Budget - Current);
		Msg = FString::Printf(TEXT("剩余步数 %d / %d"), Remaining, Budget);
	}
	else
	{
		Msg = FString::Printf(TEXT("步数 %d"), Current);
	}
	MoveCounterText->SetText(FText::FromString(Msg));
}

void UCh2HUDWidget::SetMoveCounterVisible(bool bVisible)
{
	bMoveCounterRequestedVisible = bVisible;
	if (MoveCounterText)
	{
		MoveCounterText->SetVisibility((bVisible && !bMinimalHUDMode) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UCh2HUDWidget::SetMinimalHUDMode(bool bMinimal)
{
	bMinimalHUDMode = bMinimal;
	if (ModeText)
	{
		ModeText->SetVisibility(bMinimal ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (HintText)
	{
		HintText->SetVisibility(bMinimal ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (MoveCounterText)
	{
		MoveCounterText->SetVisibility((!bMinimal && bMoveCounterRequestedVisible) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UCh2HUDWidget::SetMode(ECh2Mode NewMode)
{
	if (!ModeText) return;
	if (bMinimalHUDMode)
	{
		ModeText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	ModeText->SetVisibility(ESlateVisibility::Visible);
	ModePulseElapsed = 0.0f;
	const FString JuicyModeName = (NewMode == ECh2Mode::Clown) ? TEXT("CLOWN / L-JUMP") : TEXT("BALLET / SLIDE");
	ModeText->SetText(FText::FromString(JuicyModeName));
	ModeText->SetColorAndOpacity(FSlateColor(NewMode == ECh2Mode::Clown
		? FLinearColor(1.0f, 0.78f, 0.22f, 1.0f)
		: FLinearColor(0.72f, 0.94f, 1.0f, 1.0f)));
}

void UCh2HUDWidget::SetHintMessage(const FString& Hint)
{
	if (HintText)
	{
		if (bMinimalHUDMode)
		{
			HintText->SetVisibility(ESlateVisibility::Collapsed);
			return;
		}

		HintText->SetVisibility(ESlateVisibility::Visible);
		HintPulseElapsed = 0.0f;
		HintText->SetText(FText::FromString(Hint));
	}
}
