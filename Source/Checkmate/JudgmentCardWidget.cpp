// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "JudgmentCardWidget.h"

#include "CardData.h"
#include "CardSelectionDragOp.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

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

void UJudgmentCardWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	bIsHovered = true;
	TargetLift = HoverLiftPixels;
	TargetScale = HoverScaleMultiplier;

	OnHoverStart();
}

void UJudgmentCardWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	bIsHovered = false;
	TargetTilt = FVector2D::ZeroVector;
	TargetLift = 0.0f;
	TargetScale = 1.0f;

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
	// 标记此 widget 可以触发 drag —— UE 会在玩家移动鼠标超过阈值后调用 NativeOnDragDetected
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UJudgmentCardWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	UCardSelectionDragOp* DragOp = NewObject<UCardSelectionDragOp>(this);
	DragOp->SourceCard = this;
	DragOp->Payload = this;        // UDragDropOperation 自带的 Payload 字段
	DragOp->Pivot = EDragPivot::CenterCenter;

	// DefaultDragVisual = 拖拽时跟随光标的视觉。先用本卡 widget 作为 visual（光标会拖着一张卡的副本）
	DragOp->DefaultDragVisual = this;

	OutOperation = DragOp;

	// hover 状态在 drag 期间保持视觉（卡片浮起）
	bIsHovered = false;
	TargetLift = 0.0f;
	TargetScale = 1.0f;
}

void UJudgmentCardWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);

	// 拖到无效位置 → 视觉回弹（slot/pool 没有 accept，UE 会自动把 widget 还原 parent）
	bIsHovered = false;
	TargetTilt = FVector2D::ZeroVector;
	TargetLift = 0.0f;
	TargetScale = 1.0f;
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

	Transform.Angle = -CurrentTilt.X * (MaxTiltAngleDegrees * 0.3f);
	Transform.Shear = FVector2D(
		-CurrentTilt.Y * MaxShearAmount,
		-CurrentTilt.X * MaxShearAmount
	);
	Transform.Translation = FVector2D(0.0f, -CurrentLift);
	Transform.Scale = FVector2D(CurrentScale, CurrentScale);

	SetRenderTransform(Transform);
	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
}
