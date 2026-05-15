// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TwistSequenceManager.generated.h"

class ULevelSequence;
class ULevelSequencePlayer;
class ALevelSequenceActor;
class UJudgmentCardSubsystem;

/**
 * Twist 演出阶段事件名（Level Sequence 的 Event Track 上 dispatch 这些）。
 *
 * Sequence 资产里的 Event Track 把 OnSequenceEvent 委托接到这里的 HandleSequenceEvent，
 * payload 用 FName 传 EventName。
 */
namespace TwistEvents
{
	const FName SubliminalSurge = TEXT("EvtSubliminalSurge");  // t=0.5s
	const FName EyeFall          = TEXT("EvtEyeFall");          // t=4.0s
	const FName IrisBloom        = TEXT("EvtIrisBloom");        // t=5.0s
	const FName UIBreakdown      = TEXT("EvtUIBreakdown");      // t=6.0s
	const FName Complete         = TEXT("EvtComplete");         // t=30.0s
}

/**
 * Twist Sequence 协调者。
 *
 * 触发：订阅 UJudgmentCardSubsystem::OnTwistConditionMet
 *   → 推 GameState 进 Ch1_TwistTrigger 然后 Twist_Sequence
 *   → Play LS_Twist_Sequence Level Sequence asset
 *   → Sequence 内 Event Track 在精确 t 时刻调 HandleSequenceEvent(EventName)
 *   → 本类 dispatch 到下游 subsystem（Subliminal / Eye / UI / Audio）
 *   → Sequence 结束 → 推 GameState 进 Ch2_Awakening_Stub（黑屏 + "Ch2 Coming Soon"）
 *
 * 编辑器侧需要：
 *   1. 创建 LS_Twist_Sequence (Level Sequence asset) in Content/Twist/
 *   2. Sequence 内加 Event Track，5 个 trigger 分别 t=0.5/4.0/5.0/6.0/30.0
 *   3. 每个 trigger 的 callback bind 到本 manager 的 HandleSequenceEvent(EventName=...)
 *   4. 把 sequence asset 路径设到 TwistSequenceAsset
 */
UCLASS()
class CHECKMATE_API UTwistSequenceManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 编辑器侧填入 LS_Twist_Sequence 资产。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Twist")
	TSoftObjectPtr<ULevelSequence> TwistSequenceAsset;

	UFUNCTION(BlueprintCallable, Category="Twist")
	void StartTwistSequence();

	UFUNCTION(BlueprintCallable, Category="Twist")
	void HandleSequenceEvent(FName EventName);

	UFUNCTION(Exec)
	void TS_Trigger();

private:
	UFUNCTION()
	void OnTwistConditionMet();

	UPROPERTY()
	UJudgmentCardSubsystem* CachedJC = nullptr;

	UPROPERTY()
	ULevelSequencePlayer* ActivePlayer = nullptr;

	UPROPERTY()
	ALevelSequenceActor* ActorSequenceActor = nullptr;
};
