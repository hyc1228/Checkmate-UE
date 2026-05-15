// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EyeStateTypes.h"
#include "EyeStateComponent.generated.h"

class UMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEyeStateChanged, EEyeState, OldState, EEyeState, NewState);

/**
 * 按扣眼 ↔ 机械眼状态机组件。
 *
 * 挂在任何带 MeshComponent 的 Actor 上：玩家 Pawn / NPC 玩偶 / 道具机械眼。
 * 通过推 Substrate 主材质（M_Eye_Substrate）的参数实现视觉过渡。
 *
 * 用法：
 *   1. 编辑器侧手工创建 M_Eye_Substrate（参数名见 FEyeMaterialParamNames）
 *   2. Actor 蓝图里挂本组件，把 EyeMaterialTemplate 指向 M_Eye_Substrate
 *   3. BeginPlay 自动 CreateDynamicMaterialInstance 接管目标 mesh 的指定 slot
 *   4. 调用 RequestStateTransition 触发过渡
 *
 * 性能说明：未在过渡时 Tick 被禁用，零开销；过渡期间才打开。
 */
UCLASS(ClassGroup=(Checkmate), meta=(BlueprintSpawnableComponent))
class CHECKMATE_API UEyeStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEyeStateComponent();

	// ── 配置 ────────────────────────────────────────────────────────────────

	/** Substrate 主材质（编辑器侧创建的 M_Eye_Substrate）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye|Setup")
	UMaterialInterface* EyeMaterialTemplate = nullptr;

	/** 目标 mesh 的材质槽索引。多数情况是 0。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye|Setup", meta=(ClampMin="0"))
	int32 MaterialSlotIndex = 0;

	/** 过渡时长（秒）。1.5 与 Camera fade 节奏一致；Twist 高潮可以临时拉长。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye|Tuning", meta=(ClampMin="0.0", ClampMax="10.0"))
	float TransitionSeconds = 1.5f;

	/** 材质参数名契约。若编辑器侧改了参数名，在这里覆盖。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye|Material")
	FEyeMaterialParamNames ParamNames;

	/** 初始按扣染色（每个玩偶可独立）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye|Setup")
	EButtonStyle InitialButtonStyle = EButtonStyle::Pearl;

	/** 初始眼睛状态。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye|Setup")
	EEyeState InitialEyeState = EEyeState::ButtonEyed;

	// ── API ────────────────────────────────────────────────────────────────

	/**
	 * 请求过渡到目标状态。
	 * @param Target  目标状态（传 Transitioning 无意义，会被忽略）
	 * @param bInstant true = 立即写参数，false = 在 TransitionSeconds 内 lerp
	 */
	UFUNCTION(BlueprintCallable, Category="Eye")
	void RequestStateTransition(EEyeState Target, bool bInstant = false);

	/** 更换按扣染色（瞬时）。 */
	UFUNCTION(BlueprintCallable, Category="Eye")
	void SetButtonStyle(EButtonStyle Style);

	/** 手动指定要驱动的 MeshComponent；不调用时 BeginPlay 会自动找 owner 的第一个。 */
	UFUNCTION(BlueprintCallable, Category="Eye|Setup")
	void SetMeshComponent(UMeshComponent* InMesh);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Eye")
	EEyeState GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Eye")
	float GetCurrentBlendAlpha() const { return CurrentBlendAlpha; }

	/**
	 * 调试用：直接把 BlendAlpha 钉到指定值并推材质参数。
	 * 不改变 CurrentState（保留调用前的状态），仅用于尖刺阶段手动 scrub。
	 * 非尖刺路径不要用——逻辑应当走 RequestStateTransition。
	 */
	UFUNCTION(BlueprintCallable, Category="Eye|Debug")
	void DebugSetBlendAlpha(float Alpha);

	/** 状态变更广播（过渡开始时 New=Transitioning，结束时 New=最终态）。 */
	UPROPERTY(BlueprintAssignable, Category="Eye|Events")
	FOnEyeStateChanged OnEyeStateChanged;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	UMeshComponent* TargetMesh = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* EyeMID = nullptr;

	EEyeState CurrentState = EEyeState::ButtonEyed;

	/** 0 = 完全按扣，1 = 完全机械。Tick 期间从 BlendStart → BlendTarget 插值。 */
	float CurrentBlendAlpha = 0.0f;
	float BlendStart = 0.0f;
	float BlendTarget = 0.0f;
	float BlendElapsed = 0.0f;
	bool bIsBlending = false;

	/** 过渡结束时进入这个最终态。 */
	EEyeState PendingFinalState = EEyeState::ButtonEyed;

	void PushBlendAlphaToMID(float Alpha);
	void FinishBlend();

	static float BlendAlphaForState(EEyeState State);
};
