// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DollData.generated.h"

class USkeletalMesh;

/**
 * 娃娃眼睛 / 按扣的视觉签名（视觉锚点；eye-state GDD §8）。
 *   Pearl   : 经典珍珠按扣眼（默认）
 *   Bell    : 铃铛眼（少见）
 *   VeilPin : 头纱别针眼（最稀有）
 *   Standard: 标准眼（白底）
 */
UENUM(BlueprintType)
enum class EButtonEyeStyle : uint8
{
	Standard UMETA(DisplayName="Standard"),
	Pearl    UMETA(DisplayName="Pearl"),
	Bell     UMETA(DisplayName="Bell"),
	VeilPin  UMETA(DisplayName="VeilPin"),
	Heart    UMETA(DisplayName="Heart"),
	Rose     UMETA(DisplayName="Rose"),
};

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

	/** Use these explicit fields for the Ch1 piecework rewrite. When false, code derives them from the legacy trait sets. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Piecework")
	bool bUseExplicitPieceworkTraits = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Piecework", meta=(EditCondition="bUseExplicitPieceworkTraits"))
	bool bSmile = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Piecework", meta=(EditCondition="bUseExplicitPieceworkTraits"))
	bool bPoseConforming = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Piecework", meta=(EditCondition="bUseExplicitPieceworkTraits"))
	bool bNaturalColor = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Piecework", meta=(EditCondition="bUseExplicitPieceworkTraits"))
	bool bLongHair = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Piecework", meta=(EditCondition="bUseExplicitPieceworkTraits"))
	bool bStyled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Piecework", meta=(EditCondition="bUseExplicitPieceworkTraits"))
	bool bPearlNecklace = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Piecework", meta=(EditCondition="bUseExplicitPieceworkTraits"))
	bool bVeil = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Piecework", meta=(EditCondition="bUseExplicitPieceworkTraits"))
	bool bApron = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Piecework", meta=(EditCondition="bUseExplicitPieceworkTraits"))
	bool bRibbon = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Piecework", meta=(EditCondition="bUseExplicitPieceworkTraits"))
	bool bWhiteGloves = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Piecework", meta=(EditCondition="bUseExplicitPieceworkTraits"))
	bool bTearful = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Piecework", meta=(EditCondition="bUseExplicitPieceworkTraits"))
	bool bRoseAdornment = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Piecework", meta=(EditCondition="bUseExplicitPieceworkTraits"))
	bool bHighHeels = false;

	/** Optional bound hair skeletal mesh. Uses the same skeleton as the Ch1 doll body; hair color is driven at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Visual")
	USkeletalMesh* HairMesh = nullptr;

	// ── 视觉签名 + 翻转触发 ─────────────────────────────────────────────────

	/** 按扣眼视觉风格（Pearl / Bell / VeilPin / Standard）。Pearl 是翻转拍点的"完美贴合"主路径。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|EyeMotif")
	EButtonEyeStyle ButtonStyle = EButtonEyeStyle::Standard;

	/** 翻转触发：放行该娃娃 + 客观 IsConforming=true 即 fire Ch1→Ch2 twist。
	 *  per judgment-card.md §3 Rule 9：通常仅 Shift 4 的 Pearl-perfect 那一只配 true。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|EyeMotif")
	bool bIsTwistTrigger = false;
};
