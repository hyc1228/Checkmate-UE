// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "CheckmateGameStateSubsystem.h"

void UCheckmateGameStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CurrentState = ECheckmateGameState::Boot;
	CurrentShiftIndex = 0;
	UE_LOG(LogTemp, Log, TEXT("CheckmateGameStateSubsystem: Initialized (Boot)."));
}

bool UCheckmateGameStateSubsystem::RequestStateChange(ECheckmateGameState NextState)
{
	if (NextState == CurrentState)
	{
		return false;
	}

	if (!IsTransitionLegal(CurrentState, NextState))
	{
		UE_LOG(LogTemp, Warning, TEXT("GameState: 非法转换 %s → %s（拒绝）。"),
			*UEnum::GetValueAsString(CurrentState), *UEnum::GetValueAsString(NextState));
		return false;
	}

	const ECheckmateGameState Old = CurrentState;
	CurrentState = NextState;

	UE_LOG(LogTemp, Log, TEXT("GameState: %s → %s"),
		*UEnum::GetValueAsString(Old), *UEnum::GetValueAsString(NextState));

	OnStateChanged.Broadcast(Old, NextState);

	if (NextState == ECheckmateGameState::Ch1_Shift)
	{
		OnShiftStarted.Broadcast(CurrentShiftIndex);
	}

	return true;
}

void UCheckmateGameStateSubsystem::AdvanceToNextShift()
{
	CurrentShiftIndex = FMath::Clamp(CurrentShiftIndex + 1, 1, 4);
	RequestStateChange(ECheckmateGameState::Ch1_CardSelect);
}

bool UCheckmateGameStateSubsystem::IsTransitionLegal(ECheckmateGameState From, ECheckmateGameState To) const
{
	using S = ECheckmateGameState;

	// Boot → CardSelect 启动
	if (From == S::Boot && To == S::Ch1_CardSelect)        { return true; }

	// CardSelect → 2D→3D 过渡
	if (From == S::Ch1_CardSelect && To == S::Ch1_Transition2Dto3D)        { return true; }

	// 过渡 → Shift（检验中）
	if (From == S::Ch1_Transition2Dto3D && To == S::Ch1_Shift)             { return true; }

	// Shift → PostShiftFade 班次结束
	if (From == S::Ch1_Shift && To == S::Ch1_PostShiftFade)                { return true; }

	// PostShiftFade → CardSelect 下一班次
	if (From == S::Ch1_PostShiftFade && To == S::Ch1_CardSelect)           { return true; }

	// Shift → TwistTrigger（双路径满足时由 JudgmentCardSubsystem 推）
	if (From == S::Ch1_Shift && To == S::Ch1_TwistTrigger)                 { return true; }

	// TwistTrigger → Twist_Sequence
	if (From == S::Ch1_TwistTrigger && To == S::Twist_Sequence)            { return true; }

	// Twist_Sequence → Ch2_Awakening_Stub
	if (From == S::Twist_Sequence && To == S::Ch2_Awakening_Stub)          { return true; }

	return false;
}

void UCheckmateGameStateSubsystem::GS_SetState(int32 StateIndex)
{
	const int32 NumStates = static_cast<int32>(ECheckmateGameState::Ch2_Awakening_Stub) + 1;
	if (StateIndex < 0 || StateIndex >= NumStates)
	{
		UE_LOG(LogTemp, Warning, TEXT("GS_SetState: 越界 %d（合法 0..%d）"), StateIndex, NumStates - 1);
		return;
	}
	const ECheckmateGameState Target = static_cast<ECheckmateGameState>(StateIndex);

	// 调试 force-set：跳过合法性检查，但仍走广播
	const ECheckmateGameState Old = CurrentState;
	CurrentState = Target;
	UE_LOG(LogTemp, Warning, TEXT("[DEBUG] GameState force-set %s → %s"),
		*UEnum::GetValueAsString(Old), *UEnum::GetValueAsString(Target));
	OnStateChanged.Broadcast(Old, Target);
}

void UCheckmateGameStateSubsystem::GS_NextShift()
{
	AdvanceToNextShift();
}

void UCheckmateGameStateSubsystem::GS_PrintState()
{
	UE_LOG(LogTemp, Warning, TEXT("[STATE] Current=%s, Shift=%d"),
		*UEnum::GetValueAsString(CurrentState), CurrentShiftIndex);
}
