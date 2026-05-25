// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "JudgmentCardWidget.h"

#include "AudioService.h"
#include "CardData.h"
#include "CardSelectionScreen.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	constexpr int32 HoverZOrderBoost = 1000;
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

void UJudgmentCardWidget::SetCardData(UCardData* InCardData)
{
	CardData = InCardData;
	if (!CardData) return;

	if (LabelText)
	{
		LabelText->SetText(CardData->DisplayLabel);
	}

	if (CardImage)
	{
		// 优先用 CardData.IconTexture（玩家拖图进去就自动接管）；没图回退维度色块。
		UTexture2D* IconTex = CardData->IconTexture.IsNull() ? nullptr : CardData->IconTexture.LoadSynchronous();
		if (IconTex)
		{
			CardImage->SetBrushFromTexture(IconTex, /*bMatchSize=*/false);
			CardImage->SetColorAndOpacity(FLinearColor::White);
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
			CardImage->SetColorAndOpacity(Tint);
		}
	}

	if (CardMID)
	{
		CardMID->SetScalarParameterValue(
			MaterialParam_HoloIntensity,
			CardData->bIsPearlCompatible ? 1.0f : 0.0f
		);
	}

	OnCardDataChanged(CardData);
}

void UJudgmentCardWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 平滑插值到目标
	CurrentTilt = FMath::Vector2DInterpTo(CurrentTilt, TargetTilt, InDeltaTime, SmoothingSpeed);
	CurrentLift = FMath::FInterpTo(CurrentLift, TargetLift, InDeltaTime, SmoothingSpeed);
	CurrentScale = FMath::FInterpTo(CurrentScale, TargetScale, InDeltaTime, SmoothingSpeed);

	ApplyTiltToTransform();

	// 推送参数到 material
	if (CardMID)
	{
		CardMID->SetScalarParameterValue(MaterialParam_TiltX, CurrentTilt.X);
		CardMID->SetScalarParameterValue(MaterialParam_TiltY, CurrentTilt.Y);
		CardMID->SetScalarParameterValue(MaterialParam_HoverStrength, bIsHovered ? 1.0f : 0.0f);
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

	if (bIsHovered) return;  // 防重入（drag visual 等 corner case）

	bIsHovered = true;
	TargetLift = ComputeTargetLift(bIsSelected, true, HoverLiftPixels, SelectedLiftPixels);
	TargetScale = HoverScaleMultiplier;

	UAudioService::PlayCueStatic(this, FName("UI.Hover"), 0.4f);

	// 把 hover 中的卡顶到最上层
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasSlot->SetZOrder(CanvasSlot->GetZOrder() + HoverZOrderBoost);
	}

	OnHoverStart();
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
}

FReply UJudgmentCardWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsHovered)
	{
		TargetTilt = CalculateNormalizedTilt(InGeometry, InMouseEvent);
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UJudgmentCardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bPressing = true;
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UJudgmentCardWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
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

	Transform.Angle = BaseFanAngle + (-CurrentTilt.X * (MaxTiltAngleDegrees * 0.3f));
	Transform.Shear = FVector2D(
		-CurrentTilt.Y * MaxShearAmount,
		-CurrentTilt.X * MaxShearAmount
	);
	Transform.Translation = FVector2D(0.0f, -CurrentLift);
	Transform.Scale = FVector2D(CurrentScale, CurrentScale);

	SetRenderTransform(Transform);
	// 旋转 pivot 设在底部中心——扇形铺开时卡只绕底边旋转，所有底边保持对齐
	SetRenderTransformPivot(FVector2D(0.5f, 1.0f));
}
