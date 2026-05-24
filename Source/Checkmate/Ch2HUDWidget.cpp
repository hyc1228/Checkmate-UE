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

void UCh2HUDWidget::SetMode(ECh2Mode NewMode)
{
	if (!ModeText) return;
	const FString Name = (NewMode == ECh2Mode::Clown) ? TEXT("小丑 (knight L-跳)") : TEXT("芭蕾 (直线滑到墙)");
	ModeText->SetText(FText::FromString(FString::Printf(TEXT("当前角色：%s"), *Name)));
}

void UCh2HUDWidget::SetHintMessage(const FString& Hint)
{
	if (HintText) HintText->SetText(FText::FromString(Hint));
}
