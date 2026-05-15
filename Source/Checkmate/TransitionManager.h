// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TransitionManager.generated.h"

class UCardData;
class AJudgmentCardActor;
class AInspectionStationActor;

/**
 * 2D → 3D 实体化过渡协调者。
 *
 * 工作流：
 *   1. UCardSelectionScreen::OnAssemblyComplete 触发时调 BeginTransition(K张卡)
 *   2. 推 GameState 进 Ch1_Transition2Dto3D（CameraSystem 自动 zoom out）
 *   3. 把 K 张选定卡塑给 JudgmentCardSubsystem::SetCurrentStandard
 *   4. 在 InspectionStation.StandardTrayAnchor 摆 K 个 AJudgmentCardActor（横向排开）
 *   5. 1.5s 后推 GameState 进 Ch1_Shift（InspectionStation 收到 OnShiftStarted 自动 BeginShift）
 *
 * Whitebox：2D 卡 fade-out anim 由 UMG WBP 自己做（NativeOnAssemblyComplete 里 PlayAnimation）。
 * 本类只管 3D 侧的 spawn + 状态推进。
 */
UCLASS()
class CHECKMATE_API UTransitionManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * 由 UCardSelectionScreen 在玩家按"开始班次"后调用。
	 * @param AssembledCards K 张组装好的卡
	 */
	UFUNCTION(BlueprintCallable, Category="Transition")
	void BeginTransition(const TArray<UCardData*>& AssembledCards);

	/** 3D 纸卡 actor 类（编辑器侧可换 BP 子类）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition")
	TSubclassOf<AJudgmentCardActor> CardActorClass;

	/** 纸卡之间的横向间距（cm）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition", meta=(ClampMin="5.0"))
	float CardSpacing = 25.0f;

	/** 过渡时长（与 CameraSystem.TransitionSeconds 同步）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Transition")
	float TransitionSeconds = 1.5f;

private:
	void OnTransitionComplete();
	void SpawnPaperCards(const TArray<UCardData*>& Cards);
	AInspectionStationActor* FindInspectionStation() const;

	UPROPERTY()
	TArray<AJudgmentCardActor*> SpawnedPaperCards;

	FTimerHandle TransitionTimer;

	UPROPERTY()
	TArray<UCardData*> PendingCards;
};
