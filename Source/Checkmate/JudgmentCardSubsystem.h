// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EyeStateTypes.h"  // EButtonStyle
#include "JudgmentCardSubsystem.generated.h"

class UCardData;
class UDollData;

/**
 * 玩家裁决类型。
 */
UENUM(BlueprintType)
enum class EDollVerdict : uint8
{
	Approve  UMETA(DisplayName = "Approve 放行"),
	Reject   UMETA(DisplayName = "Reject 丢弃"),
	Timeout  UMETA(DisplayName = "Timeout 超时（默认丢弃）")
};

/**
 * 单次裁决分类。
 * 用于 review tracker / Twist 触发逻辑。
 */
UENUM(BlueprintType)
enum class EVerdictCategory : uint8
{
	TruePositive   UMETA(DisplayName = "TP 正确放行"),
	TrueNegative   UMETA(DisplayName = "TN 正确丢弃"),
	FalsePositive  UMETA(DisplayName = "FP 误放（合规误判为不合规）"),
	FalseNegative  UMETA(DisplayName = "FN 漏放（不合规放行）")
};

/**
 * 一个班次的配置。
 */
USTRUCT(BlueprintType)
struct CHECKMATE_API FShiftConfig
{
	GENERATED_BODY()

	/** 班次编号（1–4）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift")
	int32 ShiftIndex = 1;

	/** 本班可选卡池（N 张，玩家从中选 K 张组装标准）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift")
	TArray<TSoftObjectPtr<UCardData>> CardPool;

	/** 标准必须组装的卡数 K。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift", meta=(ClampMin="1", ClampMax="13"))
	int32 KCount = 3;

	/** 组装阶段倒计时（秒）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift", meta=(ClampMin="5.0"))
	float AssemblyTimerSec = 30.0f;

	/** 单只娃娃裁决超时（秒）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift", meta=(ClampMin="2.0"))
	float DollTimeoutSec = 12.0f;

	/** 本班次的娃娃流水线（按顺序）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift")
	TArray<TSoftObjectPtr<UDollData>> DollQueue;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDollVerdict, UDollData*, Doll, EDollVerdict, Verdict, EVerdictCategory, Category);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTwistConditionMet);

/**
 * Ch1 论点引擎核心。
 *
 * 责任：
 *   1. 持有当前标准（组装好的 K 张卡）
 *   2. EvaluateDoll：把娃娃 13 属性与标准卡逐一对比 → IsConforming
 *   3. 裁决分类（TP / TN / FP / FN）→ 广播 OnDollVerdict
 *   4. 双路径 Twist 检测：
 *      - 主：IsConforming AND ButtonStyle==Pearl AND Verdict==Approve
 *      - 兜底：doll.bIsFallback AND Shift==4 → 任一 verdict 都触发
 *   5. 翻转条件满足 → 广播 OnTwistConditionMet
 *
 * 该 subsystem 是 WorldSubsystem（不是 GameInstance）——保证每场 PIE 是独立实例。
 */
UCLASS()
class CHECKMATE_API UJudgmentCardSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// ── 标准组装 ────────────────────────────────────────────────────────────

	/** 班次开始时由 UCardSelectionScreen::OnAssemblyComplete 喂进来。 */
	UFUNCTION(BlueprintCallable, Category="Judgment")
	void SetCurrentStandard(const TArray<UCardData*>& AssembledCards);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Judgment")
	const TArray<UCardData*>& GetCurrentStandard() const { return CurrentStandard; }

	// ── 评估 + 裁决 ──────────────────────────────────────────────────────

	/** 按 13 属性 + K 张标准卡评估娃娃。返回是否合规（所有标准卡都满足）。 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Judgment")
	bool EvaluateDoll(const UDollData* Doll) const;

	/** 玩家提交裁决。subsystem 完成分类 + 双路径检查 + 广播。 */
	UFUNCTION(BlueprintCallable, Category="Judgment")
	void SubmitVerdict(UDollData* Doll, EDollVerdict Verdict);

	// ── 事件 ─────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category="Judgment|Events")
	FOnDollVerdict OnDollVerdict;

	/** 满足主路径或兜底路径时广播。TwistSequenceManager 订阅。 */
	UPROPERTY(BlueprintAssignable, Category="Judgment|Events")
	FOnTwistConditionMet OnTwistConditionMet;

	// ── 调试 ─────────────────────────────────────────────────────────────────

	UFUNCTION(Exec)
	void JC_PrintStandard();

private:
	UPROPERTY()
	TArray<UCardData*> CurrentStandard;

	EVerdictCategory ClassifyVerdict(bool bIsConforming, EDollVerdict Verdict) const;
	bool CheckTwistConditions(const UDollData* Doll, EDollVerdict Verdict, bool bIsConforming) const;
};
