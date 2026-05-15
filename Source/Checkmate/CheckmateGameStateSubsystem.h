// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CheckmateGameStateSubsystem.generated.h"

/**
 * 全局游戏状态机。
 *
 * 状态推进只走 RequestStateChange()；非法转换会 warning + 拒绝。
 * 所有系统（Camera / UI / Audio / Twist）订阅 OnStateChanged 自动响应。
 */
UENUM(BlueprintType)
enum class ECheckmateGameState : uint8
{
	Boot                  UMETA(DisplayName = "Boot 启动"),
	Ch1_CardSelect        UMETA(DisplayName = "Ch1 卡选阶段（2D 全屏）"),
	Ch1_Transition2Dto3D  UMETA(DisplayName = "Ch1 2D→3D 实体化过渡（1.5s）"),
	Ch1_Shift             UMETA(DisplayName = "Ch1 检验中（3D 桌面）"),
	Ch1_PostShiftFade     UMETA(DisplayName = "Ch1 班次结束淡黑"),
	Ch1_TwistTrigger      UMETA(DisplayName = "Ch1 翻转触发瞬间"),
	Twist_Sequence        UMETA(DisplayName = "Twist 演出 30s"),
	Ch2_Awakening_Stub    UMETA(DisplayName = "Ch2 觉醒（stub 黑屏占位）")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCheckmateGameStateChanged, ECheckmateGameState, OldState, ECheckmateGameState, NewState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShiftStarted, int32, ShiftIndex);

UCLASS()
class CHECKMATE_API UCheckmateGameStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** 请求状态转换。返回 true 表示已转换；false 表示拒绝（看 log）。 */
	UFUNCTION(BlueprintCallable, Category="GameState")
	bool RequestStateChange(ECheckmateGameState NextState);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GameState")
	ECheckmateGameState GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="GameState")
	int32 GetCurrentShiftIndex() const { return CurrentShiftIndex; }

	/** 进入下一班次：递增 ShiftIndex 后切到 Ch1_CardSelect。 */
	UFUNCTION(BlueprintCallable, Category="GameState")
	void AdvanceToNextShift();

	UPROPERTY(BlueprintAssignable, Category="GameState|Events")
	FOnCheckmateGameStateChanged OnStateChanged;

	/** 班次开始（在 RequestStateChange(Ch1_Shift) 时广播一次，传当前 ShiftIndex）。 */
	UPROPERTY(BlueprintAssignable, Category="GameState|Events")
	FOnShiftStarted OnShiftStarted;

	// ── 控制台命令（PIE 调试用）─────────────────────────────────────────

	UFUNCTION(Exec)
	void GS_SetState(int32 StateIndex);

	UFUNCTION(Exec)
	void GS_NextShift();

	UFUNCTION(Exec)
	void GS_PrintState();

private:
	ECheckmateGameState CurrentState = ECheckmateGameState::Boot;
	int32 CurrentShiftIndex = 0;

	bool IsTransitionLegal(ECheckmateGameState From, ECheckmateGameState To) const;
};
