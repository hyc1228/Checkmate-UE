// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CheckmateGameStateSubsystem.h"  // ECheckmateGameState
#include "CameraSystemComponent.generated.h"

class ACameraActor;
class APlayerController;

/**
 * Camera mode 枚举 — Ch1 用 3 个，Ch2 用 1 个 stub。
 */
UENUM(BlueprintType)
enum class ECameraMode : uint8
{
	None              UMETA(DisplayName = "None"),
	Ch1_CardSelect    UMETA(DisplayName = "Ch1 卡选近距前向（被 UMG 覆盖）"),
	Ch1_Inspection    UMETA(DisplayName = "Ch1 检验台 dolly-back（3D 桌面）"),
	Ch1_PostShiftFade UMETA(DisplayName = "Ch1 班次结束淡黑"),
	Ch2_Topdown_Stub  UMETA(DisplayName = "Ch2 棋盘俯视（stub）")
};

/**
 * 镜头切换系统。
 *
 * 挂在 GameMode / GameState 上即可；BeginPlay 时自动订阅
 * UCheckmateGameStateSubsystem::OnStateChanged 完成状态 → mode 映射。
 *
 * 编辑器侧只需要：在测试关卡里摆 4 个 ACameraActor，details 面板
 * 把 ModeCameras 数组里对应 mode 的项指过去即可。Whitebox 阶段
 * Inspection 镜头摆在检验台正前方 ~120cm；CardSelect 镜头与 Inspection
 * 同位但更靠前 30cm（zoom-out 时从 CardSelect 滑到 Inspection）。
 */
USTRUCT(BlueprintType)
struct CHECKMATE_API FCameraModeBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	ECameraMode Mode = ECameraMode::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	TSoftObjectPtr<ACameraActor> Camera;
};

UCLASS(ClassGroup=(Checkmate), meta=(BlueprintSpawnableComponent))
class CHECKMATE_API UCameraSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCameraSystemComponent();

	/** 编辑器侧把场景里的 4 个 ACameraActor 拖进来；mode 与 camera 一一对应。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera")
	TArray<FCameraModeBinding> ModeCameras;

	/** 2D→3D zoom out 过渡时长（秒）。1.5s 与 Eye state 过渡同步。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Tuning", meta=(ClampMin="0.0", ClampMax="5.0"))
	float TransitionSeconds = 1.5f;

	UFUNCTION(BlueprintCallable, Category="Camera")
	void SwitchToMode(ECameraMode Mode, bool bSmooth = true);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Camera")
	ECameraMode GetCurrentMode() const { return CurrentMode; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	UFUNCTION()
	void HandleStateChanged(ECheckmateGameState OldState, ECheckmateGameState NewState);

	ACameraActor* FindCameraForMode(ECameraMode Mode) const;
	APlayerController* GetPC() const;

	UPROPERTY()
	UCheckmateGameStateSubsystem* CachedSubsystem = nullptr;

	ECameraMode CurrentMode = ECameraMode::None;
};
