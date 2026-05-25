// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "MainMenuWidget.h"

#include "AudioService.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Ch1Button)
	{
		Ch1Button->OnClicked.AddDynamic(this, &UMainMenuWidget::OnCh1Clicked);
		Ch1Button->OnHovered.AddDynamic(this, &UMainMenuWidget::OnCh1Hovered);
		Ch1Button->OnUnhovered.AddDynamic(this, &UMainMenuWidget::OnCh1Unhovered);
	}
	if (Ch2Button)
	{
		Ch2Button->OnClicked.AddDynamic(this, &UMainMenuWidget::OnCh2Clicked);
		Ch2Button->OnHovered.AddDynamic(this, &UMainMenuWidget::OnCh2Hovered);
		Ch2Button->OnUnhovered.AddDynamic(this, &UMainMenuWidget::OnCh2Unhovered);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnQuitClicked);
		QuitButton->OnHovered.AddDynamic(this, &UMainMenuWidget::OnQuitHovered);
		QuitButton->OnUnhovered.AddDynamic(this, &UMainMenuWidget::OnQuitUnhovered);
	}

	// 显示鼠标 + 让 menu 接收输入
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->bShowMouseCursor = true;
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(TakeWidget());
		PC->SetInputMode(Mode);
	}
}

void UMainMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	Elapsed += InDeltaTime;

	// 悬停 scale 平滑插值（hover=1 时朝 1.08 拉，hover=0 时回到 1.0）
	const float Speed = 12.0f;
	auto Lerp = [&](float Cur, float Target) {
		return FMath::FInterpTo(Cur, Target, InDeltaTime, Speed);
	};

	Ch1Scale = Lerp(Ch1Scale, FMath::Lerp(1.0f, 1.08f, Ch1Hover));
	Ch2Scale = Lerp(Ch2Scale, FMath::Lerp(1.0f, 1.08f, Ch2Hover));
	QuitScale = Lerp(QuitScale, FMath::Lerp(1.0f, 1.08f, QuitHover));

	ApplyButtonScale(Ch1Button, Ch1Scale);
	ApplyButtonScale(Ch2Button, Ch2Scale);
	ApplyButtonScale(QuitButton, QuitScale);

	// Title idle 呼吸：alpha + 轻微 Y 漂浮
	if (TitleText)
	{
		const float A = 0.85f + 0.15f * FMath::Sin(Elapsed * 1.8f);
		FLinearColor C = TitleText->GetColorAndOpacity().GetSpecifiedColor();
		C.A = A;
		TitleText->SetColorAndOpacity(FSlateColor(C));

		FWidgetTransform Xf = TitleText->GetRenderTransform();
		Xf.Translation.Y = FMath::Sin(Elapsed * 1.2f) * 3.0f;
		TitleText->SetRenderTransform(Xf);
	}
}

void UMainMenuWidget::ApplyButtonScale(UButton* Btn, float Scale)
{
	if (!Btn) return;
	FWidgetTransform Xf = Btn->GetRenderTransform();
	Xf.Scale = FVector2D(Scale, Scale);
	Btn->SetRenderTransform(Xf);
}

void UMainMenuWidget::OnCh1Clicked()
{
	UAudioService::PlayCueStatic(this, FName("UI.Click"));
	UGameplayStatics::OpenLevel(this, Ch1MapName);
}

void UMainMenuWidget::OnCh2Clicked()
{
	UAudioService::PlayCueStatic(this, FName("UI.Click"));
	UGameplayStatics::OpenLevel(this, Ch2MapName);
}

void UMainMenuWidget::OnQuitClicked()
{
	UAudioService::PlayCueStatic(this, FName("UI.Click"));
	if (APlayerController* PC = GetOwningPlayer())
	{
		UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
	}
}

void UMainMenuWidget::OnCh1Hovered()
{
	Ch1Hover = 1.0f;
	UAudioService::PlayCueStatic(this, FName("UI.Hover"), 0.5f);
}
void UMainMenuWidget::OnCh1Unhovered() { Ch1Hover = 0.0f; }
void UMainMenuWidget::OnCh2Hovered()
{
	Ch2Hover = 1.0f;
	UAudioService::PlayCueStatic(this, FName("UI.Hover"), 0.5f);
}
void UMainMenuWidget::OnCh2Unhovered() { Ch2Hover = 0.0f; }
void UMainMenuWidget::OnQuitHovered()
{
	QuitHover = 1.0f;
	UAudioService::PlayCueStatic(this, FName("UI.Hover"), 0.5f);
}
void UMainMenuWidget::OnQuitUnhovered(){ QuitHover = 0.0f; }
