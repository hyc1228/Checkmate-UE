// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "JudgmentCardWidget.h"

#include "AudioService.h"
#include "CardData.h"
#include "Ch1CardTooltipWidget.h"
#include "Ch1RoundedImageWidget.h"
#include "CardSelectionScreen.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	constexpr int32 HoverZOrderBoost = 1000;

	float CardEaseOutCubic(float T)
	{
		const float Clamped = FMath::Clamp(T, 0.0f, 1.0f);
		return 1.0f - FMath::Pow(1.0f - Clamped, 3.0f);
	}

	float EaseOutBack(float T)
	{
		const float Clamped = FMath::Clamp(T, 0.0f, 1.0f);
		constexpr float C1 = 1.70158f;
		constexpr float C3 = C1 + 1.0f;
		return 1.0f + C3 * FMath::Pow(Clamped - 1.0f, 3.0f) + C1 * FMath::Pow(Clamped - 1.0f, 2.0f);
	}
}

UJudgmentCardWidget::UJudgmentCardWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Visible);
}

void UJudgmentCardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 创建 CardImage 的 dynamic material instance
	if (CardImage)
	{
		EnsureRoundedCardImage();

		UMaterialInterface* BaseMat = CardImage->GetDynamicMaterial();
		if (BaseMat)
		{
			CardMID = Cast<UMaterialInstanceDynamic>(BaseMat);
		}
	}

	// 应用现有 card data（如果已经 SetCardData 过）
	if (CardData)
	{
		SetCardData(CardData);
	}
}

void UJudgmentCardWidget::NativeDestruct()
{
	HideHoverTooltip();
	Super::NativeDestruct();
}

void UJudgmentCardWidget::SetCardData(UCardData* InCardData)
{
	CardData = InCardData;
	if (!CardData) return;

	if (LabelText)
	{
		LabelText->SetVisibility(ESlateVisibility::Collapsed);
		FString CompactLabel = CardData->DisplayLabel.ToString();
		if (bUseCompactCardLabel)
		{
			CompactLabel.RemoveFromStart(TEXT("必须"));
			CompactLabel.RemoveFromStart(TEXT("Must "));
			CompactLabel.RemoveFromStart(TEXT("must "));
			CompactLabel = CompactLabel.TrimStartAndEnd();
		}
		LabelText->SetText(FText::FromString(CompactLabel));
		LabelText->SetAutoWrapText(true);
	}

	if (CardImage)
	{
		CardImage->SetDesiredSizeOverride(FallbackCardImageSize);
		// 优先用 CardData.IconTexture（玩家拖图进去就自动接管）；没图回退维度色块。
		UTexture2D* IconTex = CardData->IconTexture.IsNull() ? nullptr : CardData->IconTexture.LoadSynchronous();
		if (IconTex)
		{
			if (RoundedCardImage)
			{
				RoundedCardImage->SetTexture(IconTex);
				RoundedCardImage->SetTintColor(FLinearColor::White);
				RoundedCardImage->SetCornerRadius(CardCornerRadiusPx);
			}
			else
			{
				CardImage->SetBrushFromTexture(IconTex, /*bMatchSize=*/false);
				CardImage->SetColorAndOpacity(FLinearColor::White);
			}
		}
		else
		{
			// 灰盒占位：按维度配深色 tint。
			FLinearColor Tint = FLinearColor(0.10f, 0.08f, 0.07f, 1.0f);
			switch (CardData->Dimension)
			{
				case ECardDimension::Posture:    Tint = FLinearColor(0.32f, 0.10f, 0.10f, 1.0f); break;
				case ECardDimension::Hair:       Tint = FLinearColor(0.20f, 0.13f, 0.08f, 1.0f); break;
				case ECardDimension::Expression: Tint = FLinearColor(0.30f, 0.24f, 0.10f, 1.0f); break;
				case ECardDimension::Accessory:  Tint = FLinearColor(0.12f, 0.18f, 0.22f, 1.0f); break;
			}
			if (CardData->bIsPearlCompatible)
			{
				Tint *= 1.25f;
				Tint.A = 1.0f;
			}
			if (RoundedCardImage)
			{
				RoundedCardImage->SetTexture(nullptr);
				RoundedCardImage->SetTintColor(Tint);
				RoundedCardImage->SetCornerRadius(CardCornerRadiusPx);
			}
			else
			{
				CardImage->SetColorAndOpacity(Tint);
			}
		}
	}

	if (CardMID)
	{
		const bool bHolographicFoil = CardData->bUseNegativeLaserFoil || CardData->CardColor == ECh1CardColor::Special;
		const float FoilStrength = bHolographicFoil
			? FMath::Max(0.86f, CardData->NegativeLaserVisualStrength)
			: 0.0f;
		CardMID->SetScalarParameterValue(
			MaterialParam_HoloIntensity,
			FMath::Max(CardData->bIsPearlCompatible ? 1.0f : 0.0f, FoilStrength)
		);
	}

	if (RoundedCardImage)
	{
		const bool bHolographicFoil = CardData->bUseNegativeLaserFoil || CardData->CardColor == ECh1CardColor::Special;
		const float FoilStrength = bHolographicFoil
			? FMath::Max(0.86f, CardData->NegativeLaserVisualStrength)
			: 0.0f;
		RoundedCardImage->SetNegativeLaserEffect(FoilStrength, 0.0f);
	}

	OnCardDataChanged(CardData);

	if (LabelText)
	{
		LabelText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UJudgmentCardWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 平滑插值到目标
	CurrentTilt = FMath::Vector2DInterpTo(CurrentTilt, TargetTilt, InDeltaTime, SmoothingSpeed);
	CurrentLift = FMath::FInterpTo(CurrentLift, TargetLift, InDeltaTime, SmoothingSpeed);
	CurrentScale = FMath::FInterpTo(CurrentScale, TargetScale, InDeltaTime, SmoothingSpeed);
	TickOutro(InDeltaTime);
	if (ScorePulseElapsed < ScorePulseDuration)
	{
		ScorePulseElapsed += InDeltaTime;
	}

	ApplyTiltToTransform();
	UpdateHoverTooltipPosition();

	// 推送参数到 material
	if (CardMID)
	{
		CardMID->SetScalarParameterValue(MaterialParam_TiltX, CurrentTilt.X);
		CardMID->SetScalarParameterValue(MaterialParam_TiltY, CurrentTilt.Y);
		CardMID->SetScalarParameterValue(MaterialParam_HoverStrength, bIsHovered ? 1.0f : 0.0f);
	}
	if (RoundedCardImage)
	{
		RoundedCardImage->SetLaserTilt(CurrentTilt);
	}

	OnTiltUpdated(CurrentTilt, CurrentLift);
}

namespace
{
	float ComputeTargetLift(bool bSelected, bool bHovered, float HoverLift, float SelectLift)
	{
		// 选中是 floor lift（永久浮起），hover 可以再多抬一点；取 max 而非相加避免飞太高
		const float SelLift = bSelected ? SelectLift : 0.0f;
		const float HovLift = bHovered ? HoverLift : 0.0f;
		return FMath::Max(SelLift, HovLift);
	}
}

void UJudgmentCardWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (bInteractionLocked) return;
	if (bIsHovered) return;  // 防重入（drag visual 等 corner case）

	bIsHovered = true;
	TargetLift = ComputeTargetLift(bIsSelected, true, HoverLiftPixels, SelectedLiftPixels);
	TargetScale = HoverScaleMultiplier;
	// 卡 hover 不出声 — 13 张密排扫过会乱，留 click 音即可（参考 Balatro/Slay the Spire）

	// 把 hover 中的卡顶到最上层
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasSlot->SetZOrder(CanvasSlot->GetZOrder() + HoverZOrderBoost);
	}

	OnHoverStart();
	ShowHoverTooltip();
	OnCardHoverStarted.Broadcast(this);
}

void UJudgmentCardWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (!bIsHovered) return;

	bIsHovered = false;
	bPressing = false;
	TargetTilt = FVector2D::ZeroVector;
	TargetLift = ComputeTargetLift(bIsSelected, false, HoverLiftPixels, SelectedLiftPixels);
	TargetScale = 1.0f;

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasSlot->SetZOrder(CanvasSlot->GetZOrder() - HoverZOrderBoost);
	}

	OnHoverEnd();
	HideHoverTooltip();
	OnCardHoverEnded.Broadcast(this);
}

FReply UJudgmentCardWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bInteractionLocked)
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}

	if (bIsHovered)
	{
		TargetTilt = CalculateNormalizedTilt(InGeometry, InMouseEvent);
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UJudgmentCardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bInteractionLocked)
	{
		return FReply::Handled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bPressing = true;
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UJudgmentCardWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bInteractionLocked)
	{
		bPressing = false;
		return FReply::Handled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bPressing)
	{
		bPressing = false;
		UAudioService::PlayCueStatic(this, FName("UI.Click"), 0.6f);
		if (OwningScreen)
		{
			OwningScreen->HandleCardClicked(this);
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UJudgmentCardWidget::SetSelected(bool bInSelected)
{
	if (bIsSelected == bInSelected) return;
	bIsSelected = bInSelected;
	TargetLift = ComputeTargetLift(bIsSelected, bIsHovered, HoverLiftPixels, SelectedLiftPixels);
	OnSelectedChanged(bIsSelected);
}

void UJudgmentCardWidget::SetInteractionLocked(bool bLocked)
{
	if (bInteractionLocked == bLocked) return;
	bInteractionLocked = bLocked;
	bPressing = false;

	if (bInteractionLocked)
	{
		if (bIsHovered)
		{
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
			{
				CanvasSlot->SetZOrder(CanvasSlot->GetZOrder() - HoverZOrderBoost);
			}
			bIsHovered = false;
			OnHoverEnd();
			HideHoverTooltip();
		}
		TargetTilt = FVector2D::ZeroVector;
		TargetLift = ComputeTargetLift(bIsSelected, false, HoverLiftPixels, SelectedLiftPixels);
		TargetScale = 1.0f;
	}
}

void UJudgmentCardWidget::PlayScorePulse(float Strength)
{
	ScorePulseStrength = FMath::Clamp(Strength, 0.25f, 2.5f);
	ScorePulseElapsed = 0.0f;
}

void UJudgmentCardWidget::PlaySelectionCommitDrop(int32 SelectionIndex, int32 TotalSelected, float DelaySeconds)
{
	SetInteractionLocked(true);

	const int32 SafeTotal = FMath::Max(1, TotalSelected);
	const float CenteredIndex = static_cast<float>(SelectionIndex) - (static_cast<float>(SafeTotal - 1) * 0.5f);
	const float TableSpread = 74.0f;
	const float SettleAngle = CenteredIndex * 3.5f;
	FVector2D TargetOffset(CenteredIndex * TableSpread, CommitDropDistancePixels);

	if (const UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		TargetOffset.X -= CanvasSlot->GetPosition().X * 0.35f;
	}

	StartOutro(
		TargetOffset,
		-BaseFanAngle + SettleAngle,
		1.04f,
		1.0f,
		CommitDropDurationSec,
		DelaySeconds);
}

void UJudgmentCardWidget::PlaySelectionDiscardFly(int32 CardIndex, int32 TotalCards, float DirectionSign, float DelaySeconds)
{
	SetInteractionLocked(true);

	const float SafeSign = DirectionSign >= 0.0f ? 1.0f : -1.0f;
	const float VerticalLift = -170.0f - (FMath::Max(0, CardIndex) % 5) * 18.0f;
	const float ArcOffset = (TotalCards > 0 ? static_cast<float>(CardIndex) / static_cast<float>(TotalCards) : 0.0f) * 80.0f;
	const FVector2D TargetOffset(
		SafeSign * (DiscardFlyDistancePixels + ArcOffset),
		VerticalLift);

	StartOutro(
		TargetOffset,
		SafeSign * (32.0f + (CardIndex % 3) * 6.0f),
		0.74f,
		0.0f,
		DiscardFlyDurationSec,
		DelaySeconds);
}

void UJudgmentCardWidget::StartOutro(const FVector2D& TargetOffset, float TargetAngleAdd, float InTargetScale, float InTargetOpacity, float DurationSec, float DelaySeconds)
{
	bOutroActive = true;
	OutroDelayRemaining = FMath::Max(0.0f, DelaySeconds);
	OutroElapsed = 0.0f;
	OutroDuration = FMath::Max(0.05f, DurationSec);

	OutroStartOffset = CurrentOutroOffset;
	OutroTargetOffset = TargetOffset;
	OutroStartAngleAdd = CurrentOutroAngleAdd;
	OutroTargetAngleAdd = TargetAngleAdd;
	OutroStartScale = CurrentOutroScale;
	OutroTargetScale = InTargetScale;
	OutroStartOpacity = CurrentOutroOpacity;
	OutroTargetOpacity = FMath::Clamp(InTargetOpacity, 0.0f, 1.0f);
}

void UJudgmentCardWidget::TickOutro(float InDeltaTime)
{
	if (!bOutroActive)
	{
		return;
	}

	if (OutroDelayRemaining > 0.0f)
	{
		OutroDelayRemaining -= InDeltaTime;
		return;
	}

	OutroElapsed = FMath::Min(OutroElapsed + InDeltaTime, OutroDuration);
	const float T = OutroElapsed / OutroDuration;
	const float MoveT = EaseOutBack(T);
	const float FadeT = CardEaseOutCubic(T);

	CurrentOutroOffset = FMath::Lerp(OutroStartOffset, OutroTargetOffset, MoveT);
	CurrentOutroAngleAdd = FMath::Lerp(OutroStartAngleAdd, OutroTargetAngleAdd, MoveT);
	CurrentOutroScale = FMath::Lerp(OutroStartScale, OutroTargetScale, MoveT);
	CurrentOutroOpacity = FMath::Lerp(OutroStartOpacity, OutroTargetOpacity, FadeT);

	if (OutroElapsed >= OutroDuration)
	{
		bOutroActive = false;
	}
}

FVector2D UJudgmentCardWidget::CalculateNormalizedTilt(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) const
{
	const FVector2D LocalMouse = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	const FVector2D WidgetSize = InGeometry.GetLocalSize();

	if (WidgetSize.X <= KINDA_SMALL_NUMBER || WidgetSize.Y <= KINDA_SMALL_NUMBER)
	{
		return FVector2D::ZeroVector;
	}

	const FVector2D Center = WidgetSize * 0.5f;
	const FVector2D Normalized = (LocalMouse - Center) / Center;

	return FVector2D(
		FMath::Clamp(Normalized.X, -1.0f, 1.0f),
		FMath::Clamp(Normalized.Y, -1.0f, 1.0f)
	);
}

void UJudgmentCardWidget::ApplyTiltToTransform()
{
	FWidgetTransform Transform;
	float PulseScale = 1.0f;
	float PulseLift = 0.0f;
	float PulseAngle = 0.0f;
	if (ScorePulseElapsed < ScorePulseDuration)
	{
		const float T = FMath::Clamp(ScorePulseElapsed / FMath::Max(0.01f, ScorePulseDuration), 0.0f, 1.0f);
		const float Pop = FMath::Sin(T * PI);
		const float Shake = FMath::Sin(T * PI * 10.0f) * (1.0f - T);
		PulseScale += Pop * 0.18f * ScorePulseStrength + Shake * 0.035f;
		PulseLift = Pop * 28.0f * ScorePulseStrength;
		PulseAngle = Shake * 7.0f * ScorePulseStrength;
	}

	Transform.Angle = BaseFanAngle + CurrentOutroAngleAdd + PulseAngle + (-CurrentTilt.X * (MaxTiltAngleDegrees * 0.3f));
	Transform.Shear = FVector2D(
		-CurrentTilt.Y * MaxShearAmount,
		-CurrentTilt.X * MaxShearAmount
	);
	Transform.Translation = FVector2D(0.0f, -CurrentLift - PulseLift) + CurrentOutroOffset;
	Transform.Scale = FVector2D(CurrentScale * CurrentOutroScale * PulseScale, CurrentScale * CurrentOutroScale * PulseScale);

	SetRenderTransform(Transform);
	SetRenderOpacity(CurrentOutroOpacity);
	// 旋转 pivot 设在底部中心——扇形铺开时卡只绕底边旋转，所有底边保持对齐
	SetRenderTransformPivot(FVector2D(0.5f, 1.0f));
}

void UJudgmentCardWidget::EnsureRoundedCardImage()
{
	if (RoundedCardImage || !CardImage)
	{
		return;
	}

	UOverlay* CardOverlay = Cast<UOverlay>(CardImage->GetParent());
	if (!CardOverlay)
	{
		return;
	}

	RoundedCardImage = NewObject<UCh1RoundedImageWidget>(this);
	if (!RoundedCardImage)
	{
		return;
	}

	RoundedCardImage->SetCornerRadius(CardCornerRadiusPx);
	RoundedCardImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UOverlaySlot* RoundedSlot = CardOverlay->AddChildToOverlay(RoundedCardImage))
	{
		RoundedSlot->SetHorizontalAlignment(HAlign_Fill);
		RoundedSlot->SetVerticalAlignment(VAlign_Fill);
	}
	CardImage->SetVisibility(ESlateVisibility::Collapsed);
}

void UJudgmentCardWidget::ShowHoverTooltip()
{
	if (!bShowNativeHoverTooltip || !CardData || ActiveTooltip)
	{
		return;
	}

	ActiveTooltip = CreateWidget<UCh1CardTooltipWidget>(GetOwningPlayer(), UCh1CardTooltipWidget::StaticClass());
	if (!ActiveTooltip)
	{
		return;
	}

	ActiveTooltip->SetCardData(CardData);
	ActiveTooltip->AddToViewport(/*ZOrder=*/5000);
	UpdateHoverTooltipPosition();
}

void UJudgmentCardWidget::HideHoverTooltip()
{
	if (ActiveTooltip)
	{
		ActiveTooltip->RemoveFromParent();
		ActiveTooltip = nullptr;
	}
}

void UJudgmentCardWidget::UpdateHoverTooltipPosition()
{
	if (!ActiveTooltip)
	{
		return;
	}

	FVector2D PixelPosition;
	FVector2D ViewportPosition;
	USlateBlueprintLibrary::AbsoluteToViewport(
		this,
		GetCachedGeometry().GetAbsolutePosition(),
		PixelPosition,
		ViewportPosition);
	ActiveTooltip->SetPositionInViewport(ViewportPosition + TooltipViewportOffset, false);
}
