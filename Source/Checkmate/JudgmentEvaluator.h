// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JudgmentEvaluator.generated.h"

class UCardData;
class UDollData;

/**
 * 判定裁决：玩家组装的判据对该娃娃的客观结果。
 * Pass = 娃娃满足所有判据；Reject = 至少一个判据不满足。
 */
UENUM(BlueprintType)
enum class EJudgmentVerdict : uint8
{
	Pass    UMETA(DisplayName = "Pass (合格)"),
	Reject  UMETA(DisplayName = "Reject (不合格)")
};

/**
 * 玩家选择 vs 客观裁决的 4 象限分类（用于阈下反馈 / 文案漂移 / 翻转触发）。
 * 一句话：玩家「选 Pass / 选 Reject」 × 客观「Pass / Reject」 → 4 outcomes。
 *
 * spec: judgment-card.md §3 Rule 7
 */
UENUM(BlueprintType)
enum class EOutcomeClass : uint8
{
	TruePositive  UMETA(DisplayName="TruePos  放行合格的"),   // 选 Pass，客观 Pass — 正确
	TrueNegative  UMETA(DisplayName="TrueNeg  剔掉不合格的"), // 选 Reject，客观 Reject — 正确
	FalsePositive UMETA(DisplayName="FalsePos 误放行了不合格"), // 选 Pass，客观 Reject — 误判
	FalseNegative UMETA(DisplayName="FalseNeg 误剔掉了合格"),  // 选 Reject，客观 Pass — 误判
};

/**
 * 比对：K 张判据卡 vs 一只娃娃。
 *
 * 规则（slice v0.2 锁定）：
 *   - 「全部必须命中」——任一判据卡不满足 → Reject
 *   - Posture 维度：UCardData::PostureValue == UDollData::Posture
 *   - Hair / Expression / Accessory 维度：UDollData 对应 TSet 包含 UCardData::CardId
 *
 * OutFailReason 在 Reject 时填一行调试文本（灯盒阶段用 toast 显示，便于核对）。
 */
UCLASS()
class CHECKMATE_API UJudgmentEvaluator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Judgment")
	static EJudgmentVerdict EvaluateDoll(
		const TArray<UCardData*>& SelectedCards,
		const UDollData* DollData,
		FString& OutFailReason
	);

	/** 把玩家选择 + 客观结果分类成 4 outcome（FalsePos/FalseNeg 用于翻转 / 阈下反馈）。 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Judgment")
	static EOutcomeClass ClassifyOutcome(
		EJudgmentVerdict ObjectiveVerdict,
		bool bPlayerAcceptedDoll
	);

	/** outcome 是否属于误判（FalsePos / FalseNeg）。 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Judgment")
	static bool IsMisjudgment(EOutcomeClass Outcome);
};
