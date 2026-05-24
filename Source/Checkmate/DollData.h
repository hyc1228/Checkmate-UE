// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DollData.generated.h"

/**
 * 单只娃娃的属性集 DataAsset（灰盒：纯数据，灯盒阶段无 mesh / 无表情贴图）。
 *
 * 与 UCardData 的对应关系：
 *   - Posture (FName)：精确等值匹配 UCardData::PostureValue
 *   - HairTraits / ExpressionTraits / AccessoryTraits (TSet<FName>)：
 *     包含对应 UCardData::CardId 即视为该 trait 存在
 *
 * 设计意图（slice spec / judgment-card.md）：
 *   每只娃娃可以同时拥有多个非姿态特征（如同时 LongHair + NaturalColor + Styled），
 *   而姿态在同一时刻只能是一种。
 *
 * Pearl-compatible 完美娃娃示例（用于翻转拍点前的"完美贴合标准"那一只）：
 *   Posture=BalletPose, Hair={LongHair, NaturalColor, Styled},
 *   Expression={Smile}, Accessory={PearlNecklace}
 */
UCLASS(BlueprintType)
class CHECKMATE_API UDollData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 娃娃 ID（如 "Ballet_Perfect_01"）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll")
	FName DollId;

	/** 灰盒显示名（如 "1 号娃娃"）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll")
	FText DisplayName;

	// ── 4 维度属性 ──────────────────────────────────────────────────────────

	/** 单值：芭蕾 / 鞠躬 / 端立 / 端坐 之一（FName 须匹配 UCardData::PostureValue）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Attributes")
	FName Posture = NAME_None;

	/** 多值：长发 / 自然色 / 整洁 等的子集（FName 须匹配 UCardData::CardId）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Attributes")
	TSet<FName> HairTraits;

	/** 多值：微笑 等。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Attributes")
	TSet<FName> ExpressionTraits;

	/** 多值：珍珠项链 / 头纱 / 围裙 / 缎带 / 白手套 的子集。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Attributes")
	TSet<FName> AccessoryTraits;
};
