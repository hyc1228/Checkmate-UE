// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DollDisplay.generated.h"

class UDollData;
class UInspectionScreen;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class USkeletalMesh;
class UAnimSequence;
class USceneComponent;
class APlayerController;
class UPrimitiveComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * 检验台上的 3D 娃娃 + 印章 actor。
 *
 * 流水线流程（v0.5）：
 *   1. ApplyDollData(doll) → SlidingIn：娃娃从左方滑入到检验位
 *   2. Idle：玩家可拖娃娃旋转 / 可拖印章下拉
 *   3a. 拖娃娃上甩 → Tossing：飞起翻滚 + 缩小 = 丢弃
 *   3b. 拖印章往下 → 拉过阈值松手 → Confirming：盒盖落定 + 整体右移 = 确认
 *   3c. 拖动不满足任一阈值 → 弹回 Idle
 *   4. Tossing / Confirming 动画结束 → 通知 screen 推进，screen 调 ApplyDollData(下一只)
 */
UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API ADollDisplay : public AActor
{
	GENERATED_BODY()

public:
	ADollDisplay();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	USceneComponent* SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	USceneComponent* VisualPivot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UStaticMeshComponent* DollMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	USkeletalMeshComponent* PoseMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	USkeletalMeshComponent* HairMesh = nullptr;

	/** 头顶印章（盒盖）—— 玩家拖它往下触发确认。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UStaticMeshComponent* BoxCoverMesh = nullptr;

	// ── 入场（流水线滑入）────────────────────────────────────────────────

	/** 滑入起点相对检验位的 Y 偏移（负值 = 从左侧滑入）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Conveyor")
	float EntryOffsetY = -600.0f;

	/** 滑入动画时长。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Conveyor", meta=(ClampMin="0.05"))
	float EntrySlideDurationSec = 0.6f;

	// ── 输入模型（v0.7）────────────────────────────────────────────────
	// 左键拖 = 拖移娃娃（鼠标 X→世界 Y，鼠标 Y→世界 Z），松手上甩 = 扔
	// 右键拖 = 旋转娃娃（鼠标 X→Yaw，鼠标 Y→Pitch，带 clamp）
	// 左键拖印章下拉 = 确认

	/** 拖移灵敏度：鼠标像素 → 世界单位。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Input", meta=(ClampMin="0.05"))
	float DragMoveSensitivity = 0.6f;

	/** 旋转灵敏度——长边方向（Yaw / 绕立柱长轴）幅度大。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Input", meta=(ClampMin="0.01"))
	float YawSensitivity = 0.8f;

	/** 旋转灵敏度——短边方向（Pitch）幅度小。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Input", meta=(ClampMin="0.01"))
	float PitchSensitivity = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Input", meta=(ClampMin="5.0", ClampMax="180.0"))
	float YawMaxDegrees = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Input", meta=(ClampMin="5.0", ClampMax="80.0"))
	float PitchMaxDegrees = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Input")
	FVector RotationPivotRelativeLocation = FVector(0.0f, 0.0f, -85.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Input|Gamepad", meta=(ClampMin="0.0", ClampMax="0.95"))
	float GamepadStickDeadZone = 0.18f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Input|Gamepad", meta=(ClampMin="1.0"))
	float GamepadYawDegreesPerSecond = 95.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Input|Gamepad", meta=(ClampMin="1.0"))
	float GamepadPitchDegreesPerSecond = 45.0f;

	// ── 上甩丢弃 ────────────────────────────────────────────────────────

	/** 松手瞬间鼠标速度（pixel/sec 大小，方向不限）超过该值 → 触发 toss。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Gesture", meta=(ClampMin="50.0"))
	float TossSpeedThreshold = 700.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Anim", meta=(ClampMin="0.1"))
	float TossDurationSec = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Anim", meta=(ClampMin="50.0"))
	float TossUpwardDistance = 400.0f;

	// ── 印章拖下来 ──────────────────────────────────────────────────────

	/** 印章在 Idle 时悬停高度（相对 actor root）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Stamp", meta=(ClampMin="50.0"))
	float StampHoverZ = 220.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Stamp")
	float StampWorldZOffset = 45.0f;

	/** 印章触发确认所需的下拉幅度（相对 hover Z 的距离，向下 = 正值）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Stamp", meta=(ClampMin="20.0"))
	float StampPullDownThreshold = 140.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Gesture", meta=(ClampMin="1.0"))
	float TossMinDragDistancePixels = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Gesture")
	bool bRequireUpwardTossGesture = true;

	/** 鼠标 Y 像素 → 印章 Z 单位的换算系数。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Stamp", meta=(ClampMin="0.1"))
	float StampDragSensitivity = 0.8f;

	// ── 确认动画 ────────────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Anim", meta=(ClampMin="0.1"))
	float ConfirmDurationSec = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Pose Assets")
	USkeletalMesh* DefaultPoseMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Pose Assets")
	UAnimSequence* SmilePoseAnim = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Pose Assets")
	UAnimSequence* SadPoseAnim = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Pose Assets")
	UAnimSequence* BadPoseAnim = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Pose Assets")
	FVector PoseMeshRelativeLocation = FVector(0.0f, 0.0f, -60.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Pose Assets")
	FRotator PoseMeshRelativeRotation = FRotator(0.0f, 90.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Pose Assets", meta=(ClampMin="0.01"))
	float PoseMeshUniformScale = 1.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Hair")
	UMaterialInterface* HairBaseMaterialOverride = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|View", meta=(ClampMin="0.1"))
	float DefaultViewScaleMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|View", meta=(ClampMin="0.1"))
	float MinViewScaleMultiplier = 0.85f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|View", meta=(ClampMin="0.1"))
	float MaxViewScaleMultiplier = 2.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|View", meta=(ClampMin="0.01"))
	float MouseWheelZoomStep = 0.14f;

	/** 确认时整体向右滑出的 Y 距离（正值 = 右）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Doll|Anim", meta=(ClampMin="50.0"))
	float ConfirmExitDistanceY = 500.0f;

	// ── Public API ──────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category="Doll")
	void SetOwningScreen(UInspectionScreen* InScreen) { OwningScreen = InScreen; }

	/** 加载下一只娃娃数据 + 触发滑入动画。 */
	UFUNCTION(BlueprintCallable, Category="Doll")
	void ApplyDollData(UDollData* InDoll);

	/** 重置（位置 / 印章 hover Z）—— 一般不直接调，让动画自然流转。 */
	UFUNCTION(BlueprintCallable, Category="Doll")
	void SnapToIdle();

	/** 外部（如超时）强制丢弃。 */
	UFUNCTION(BlueprintCallable, Category="Doll")
	void ForceToss();

	/** PV / Sequencer seam: hold a direct look back to the active camera, then restore. */
	UFUNCTION(BlueprintCallable, Category="Doll|PV")
	void TriggerLookAtCamera(float HoldSeconds = 0.5f);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	enum class EState : uint8
	{
		SlidingIn,
		Idle,
		MovingDoll,      // LMB 拖 → 跟鼠标移动；松手上甩 = 扔
		RotatingDoll,    // RMB 拖 → 旋转（Yaw/Pitch，带 clamp）
		DraggingStamp,   // LMB 拖印章 → 往下拉触发确认
		Tossing,
		Confirming,
	};
	EState State = EState::Idle;

	UPROPERTY()
	UDollData* CurrentDoll = nullptr;

	UPROPERTY()
	UInspectionScreen* OwningScreen = nullptr;

	FVector InspectionLocation = FVector::ZeroVector;
	FRotator InspectionRotation = FRotator::ZeroRotator;

	// 拖娃娃
	FVector2D LastMousePos = FVector2D::ZeroVector;
	FVector2D DragStartMousePos = FVector2D::ZeroVector;
	float DragDistancePixels = 0.0f;
	FVector2D LastMouseDelta = FVector2D::ZeroVector;  // 最后一帧像素位移（XY 都用，判 toss 速度）
	float MouseFrameDt = 0.016f;
	float CurrentViewScaleMultiplier = 1.0f;

	// 累积旋转（只施加在 DollMesh 上，不污染 actor 自身 transform）
	float AccumYaw = 0.0f;
	float AccumPitch = 0.0f;

	// 拖印章
	float StampDragStartZ = 0.0f;
	float StampCurrentZ = 0.0f;

	// 各种动画通用
	float AnimElapsed = 0.0f;
	FVector AnimStartLoc = FVector::ZeroVector;
	FVector AnimEndLoc = FVector::ZeroVector;
	FRotator PVLookAtOriginalRelativeRotation = FRotator::ZeroRotator;
	FTimerHandle PVLookAtTimerHandle;

	APlayerController* GetPC() const;
	UPrimitiveComponent* TraceUnderCursor() const;

	void BeginSlideIn();
	void TickSlideIn(float Dt);

	bool TryStartAnyDrag();
	void TickMovingDoll(float Dt);
	void TickRotatingDoll(float Dt);
	void TickDraggingStamp(float Dt);
	void ReleaseMovingDoll();
	void ReleaseRotatingDoll();
	void ReleaseStampDrag();
	bool IsCurrentDragATossGesture() const;
	void HandleZoomInput();
	void HandleGamepadRotationInput(float Dt);
	FVector GetStampWorldLocation(float LocalStampZ) const;

	void EnterTossing();
	void EnterConfirming();
	void TickTossing(float Dt);
	void TickConfirming(float Dt);
	void RestorePVLookAtRotation();
	void ApplyPoseVisualFromDoll();
	void ApplyHairVisualFromDoll();
	void ApplyHairMaterialOverrides();
	bool ShouldOverrideHairMaterialSlot(int32 MaterialIndex) const;
	int32 HairColorSeed = 0;
	bool IsPoseVisualActive() const;
	void ApplyVisualPivotOffset();
	void SetDollVisualVisibility(bool bVisible);
	void SetDollVisualRelativeScale(float ScaleMultiplier);
	void SetDollVisualRelativeRotation(const FRotator& Rotation);
	void SetDollVisualWorldRotation(const FRotator& Rotation);
};
