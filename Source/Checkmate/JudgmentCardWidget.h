// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JudgmentCardWidget.generated.h"

class UImage;
class UTextBlock;
class UMaterialInstanceDynamic;
class UCardData;
class UCardSelectionScreen;

/**
 * Balatro 风判据卡 widget 基类。
 *
 * 处理：
 *   - 光标 hover 时的伪 3D 倾斜 + 浮起 + material 参数推送
 *   - 拖拽检测（NativeOnDragDetected → 创建 UCardSelectionDragOp）
 *   - UCardData 绑定（卡 label / icon / Pearl 标志）
 *
 * 视觉布局（哪个 Image 用什么 material）由 WBP 子类决定，本基类不假设具体层级。
 *
 * 用法：
 *   1. 创建 WBP_JudgmentCard 继承自此类
 *   2. 在 WBP 里放一个 Image control 命名为 "CardImage"（必须与 BindWidget 一致）
 *   3. 把 Image 的 Brush Material 设为 M_JudgmentCard_2D 实例
 *   4. 把 Image 设为 Variable（勾上 Is Variable）—— UMG 编译后会暴露给 C++
 *   5. 可选：放一个 TextBlock 命名为 "LabelText" 显示卡字面要求
 *   6. PIE 测试：hover 卡片应看到 tilt + lift 效果；左键按下拖动时应触发 drag
 */
UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API UJudgmentCardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UJudgmentCardWidget(const FObjectInitializer& ObjectInitializer);

	// ── 数据绑定 ────────────────────────────────────────────────────────────

	/** 把 UCardData 应用到本 widget（label / icon / Pearl 视效）。 */
	UFUNCTION(BlueprintCallable, Category="Card|Data")
	void SetCardData(UCardData* InCardData);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Card|Data")
	UCardData* GetCardData() const { return CardData; }

	UFUNCTION(BlueprintCallable, Category="Card|Data")
	void SetOwningScreen(UCardSelectionScreen* InScreen) { OwningScreen = InScreen; }

	// ── 选中状态 ──────────────────────────────────────────────────────────

	/** 由 owning screen 在 click 仲裁后调用。视觉立刻响应（永久浮起 / 落回）。 */
	UFUNCTION(BlueprintCallable, Category="Card|Selection")
	void SetSelected(bool bInSelected);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Card|Selection")
	bool IsSelected() const { return bIsSelected; }

	/** 扇形铺开：每张卡有一个 base 旋转角度（度），叠加在 tilt 之上。Screen 在构造时分配。 */
	UFUNCTION(BlueprintCallable, Category="Card|Layout")
	void SetBaseFanAngle(float DegAngle) { BaseFanAngle = DegAngle; }

	// ── 调参旋钮（在 WBP defaults 或 Editor 里调）──────────────────────────────

	/** 最大倾斜角度（度）。Balatro 默认 ~15。太大会显得卡片像在打转。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Card|Tilt", meta=(ClampMin="0.0", ClampMax="45.0"))
	float MaxTiltAngleDegrees = 15.0f;

	/** 最大剪切量（Shear，制造伪 3D 透视错觉）。0.05-0.15 之间。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Card|Tilt", meta=(ClampMin="0.0", ClampMax="0.5"))
	float MaxShearAmount = 0.10f;

	/** Hover 时上浮像素。10-20 之间感觉自然。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Card|Hover", meta=(ClampMin="0.0"))
	float HoverLiftPixels = 12.0f;

	/** Hover 时缩放增量。1.05 = 放大 5%。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Card|Hover", meta=(ClampMin="1.0", ClampMax="1.5"))
	float HoverScaleMultiplier = 1.05f;

	/** 被选中时永久上浮像素（Balatro：选中后停留在抬起状态）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Card|Selection", meta=(ClampMin="0.0"))
	float SelectedLiftPixels = 32.0f;

	/** 平滑插值速度（越大越快回到目标）。10-20 之间感觉跟手。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Card|Smoothing", meta=(ClampMin="1.0", ClampMax="50.0"))
	float SmoothingSpeed = 14.0f;

	/** Material 参数名（与 M_JudgmentCard_2D 里定义的同名）。可在 WBP 里覆盖。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Card|Material Binding")
	FName MaterialParam_TiltX = TEXT("TiltX");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Card|Material Binding")
	FName MaterialParam_TiltY = TEXT("TiltY");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Card|Material Binding")
	FName MaterialParam_HoverStrength = TEXT("HoverStrength");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Card|Material Binding")
	FName MaterialParam_HoloIntensity = TEXT("HoloIntensity");

	// ── 蓝图 hook（用户可在 WBP 里 override）─────────────────────────────────

	UFUNCTION(BlueprintImplementableEvent, Category="Card|Events")
	void OnTiltUpdated(FVector2D NormalizedTilt, float InCurrentLift);

	UFUNCTION(BlueprintImplementableEvent, Category="Card|Events")
	void OnHoverStart();

	UFUNCTION(BlueprintImplementableEvent, Category="Card|Events")
	void OnHoverEnd();

	/** 卡数据更新后触发（WBP 用来 push label 到 TextBlock 等）。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Card|Events")
	void OnCardDataChanged(UCardData* NewData);

	/** 选中状态切换后触发（WBP 可加发光描边等点缀）。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Card|Events")
	void OnSelectedChanged(bool bSelected);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** WBP 中拖一个 Image 进来命名 "CardImage" 并勾 Is Variable。 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UImage* CardImage = nullptr;

	/** 可选：卡面字面文本（WBP 里命名 "LabelText"）。 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UMG Binding")
	UTextBlock* LabelText = nullptr;

private:
	UPROPERTY()
	UCardData* CardData = nullptr;

	UPROPERTY()
	UCardSelectionScreen* OwningScreen = nullptr;

	FVector2D CurrentTilt = FVector2D::ZeroVector;
	FVector2D TargetTilt = FVector2D::ZeroVector;
	float CurrentLift = 0.0f;
	float TargetLift = 0.0f;
	float CurrentScale = 1.0f;
	float TargetScale = 1.0f;
	bool bIsHovered = false;
	bool bIsSelected = false;
	bool bPressing = false;  // 鼠标按下中——up 时若仍 hovered 视为 click
	float BaseFanAngle = 0.0f;

	UPROPERTY()
	UMaterialInstanceDynamic* CardMID = nullptr;

	void ApplyTiltToTransform();
	FVector2D CalculateNormalizedTilt(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) const;
};
