// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JudgmentCardSubsystem.h"  // EDollVerdict / FShiftConfig
#include "InspectionStationActor.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UChildActorComponent;
class ASpawnedDoll;
class UDollData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShiftCompleted, int32, ShiftIndex);

/**
 * Ch1 主舞台 Actor：检验台 + 传送带 + 入场出场 + 裁决接线。
 *
 * 组件结构（whitebox 全用 cube placeholder）：
 *   - DeskMesh：桌面平台
 *   - ConveyorMesh：传送带（左→右）
 *   - SpawnAnchor：娃娃入场锚点（传送带右端）
 *   - InspectionAnchor：检验位置（桌面正中央）
 *   - ApproveExitAnchor：放行出场（右侧消失点）
 *   - RejectChuteAnchor：溜槽下落位
 *   - StandardTrayAnchor：左侧木盘（3D 纸卡叠放位）
 *   - ApprovalStamp：印章 cube placeholder
 *   - RejectChain：拉链 cube placeholder
 *
 * 工作流：
 *   1. 编辑器侧把本 Actor 摆进 Level，给 Shift1~4Configs 填好 4 个 FShiftConfig（每个含 DollQueue + Pool）
 *   2. GameState 切到 Ch1_Shift 时 BeginShift 自动启动
 *   3. SpawnNextDoll 拉队列里下一只 → DollData 喂给 ASpawnedDoll
 *   4. 娃娃从 SpawnAnchor 滑到 InspectionAnchor（默认 4s）
 *   5. 玩家按键提交 verdict → JudgmentCardSubsystem::SubmitVerdict
 *   6. Approve → 娃娃滑到 ApproveExitAnchor → destroy；Reject → 下落到 RejectChuteAnchor → destroy
 *   7. 队列空 → OnShiftCompleted → 切 PostShiftFade
 */
UCLASS()
class CHECKMATE_API AInspectionStationActor : public AActor
{
	GENERATED_BODY()

public:
	AInspectionStationActor();

	// ── 组件 ────────────────────────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Station")
	USceneComponent* RootScene = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Station")
	UStaticMeshComponent* DeskMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Station")
	UStaticMeshComponent* ConveyorMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Station|Anchors")
	USceneComponent* SpawnAnchor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Station|Anchors")
	USceneComponent* InspectionAnchor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Station|Anchors")
	USceneComponent* ApproveExitAnchor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Station|Anchors")
	USceneComponent* RejectChuteAnchor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Station|Anchors")
	USceneComponent* StandardTrayAnchor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Station|Props")
	UStaticMeshComponent* ApprovalStamp = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Station|Props")
	UStaticMeshComponent* RejectChain = nullptr;

	// ── 配置 ────────────────────────────────────────────────────────────────

	/** 用什么 Actor 类 spawn 娃娃。默认 ASpawnedDoll，可换 BP 子类。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Station|Config")
	TSubclassOf<ASpawnedDoll> DollActorClass;

	/** 4 个班次的配置。编辑器侧填 13 张卡的 DataAsset 引用 + DollQueue。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Station|Config")
	TArray<FShiftConfig> ShiftConfigs;

	/** 滑入 / 滑出过渡时长（秒）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Station|Tuning", meta=(ClampMin="0.5", ClampMax="10.0"))
	float SlideInSeconds = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Station|Tuning", meta=(ClampMin="0.2", ClampMax="3.0"))
	float SlideOutSeconds = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Station|Tuning", meta=(ClampMin="0.1", ClampMax="2.0"))
	float RejectDropSeconds = 0.5f;

	/** 班次间间隔（秒）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Station|Tuning", meta=(ClampMin="0.0"))
	float InterDollDelay = 0.5f;

	// ── API ──────────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category="Station")
	void BeginShift(int32 ShiftIndex);

	/** 由玩家输入触发（蓝图 / Player Controller 接 IA_Approve / IA_Reject 调）。 */
	UFUNCTION(BlueprintCallable, Category="Station")
	void SubmitPlayerVerdict(EDollVerdict Verdict);

	UPROPERTY(BlueprintAssignable, Category="Station|Events")
	FOnShiftCompleted OnShiftCompleted;

protected:
	virtual void BeginPlay() override;

private:
	int32 ActiveShiftIndex = -1;
	int32 NextDollInQueue = 0;
	bool bAwaitingVerdict = false;

	UPROPERTY()
	ASpawnedDoll* CurrentDoll = nullptr;

	FTimerHandle SlideInTimer;
	FTimerHandle SlideOutTimer;
	FTimerHandle InterDollTimer;
	FTimerHandle VerdictTimeoutTimer;

	float DollTimeoutSecCached = 12.0f;

	void SpawnNextDoll();
	void OnSlideInComplete();
	void OnVerdictTimeout();
	void HandleApproveExit();
	void HandleRejectChute();
	void OnSlideOutComplete();
	void CompleteShift();

	UJudgmentCardSubsystem* GetJudgment() const;

	const FShiftConfig* GetConfigForShift(int32 ShiftIndex) const;
};
