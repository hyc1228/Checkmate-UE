// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EyeStateTypes.h"  // EButtonStyle
#include "CardData.h"       // ECardDimension（虽然此处不直接用，但概念耦合）
#include "DollData.generated.h"

class UStaticMesh;
class USkeletalMesh;
class UAnimationAsset;
class UMaterialInterface;

/**
 * 姿态 Posture 维度的 4 个枚举值。
 * 必须与 13 张卡里 4 张 Posture 卡的 PostureValue 一致：
 *   BalletPose / Bowing / Upright / SeatedProper
 */
UENUM(BlueprintType)
enum class EDollPosture : uint8
{
	BalletPose      UMETA(DisplayName = "BalletPose 芭蕾舞姿"),
	Bowing          UMETA(DisplayName = "Bowing 鞠躬"),
	Upright         UMETA(DisplayName = "Upright 直立"),
	SeatedProper    UMETA(DisplayName = "SeatedProper 端坐")
};

/**
 * 单只娃娃的属性档案（13 属性 = 1 Posture + 9 bool）。
 *
 * Pearl-compatible 完整画像（v1.2）的娃娃应满足：
 *   Posture=BalletPose, bLongHair=true, bNaturalColor=true, bStyled=true,
 *   bSmile=true, bPearlNecklace=true, ButtonStyle=Pearl
 *
 * 创建方式：编辑器 Content Browser 右键 → Miscellaneous → Data Asset → UDollData。
 * 命名约定：`DA_Doll_Shift{N}_{ID}` 例如 `DA_Doll_Shift1_01`。
 *
 * Whitebox 阶段：
 *   - SkeletalMesh / 各 Accessory mesh 可留空，运行时落 placeholder
 *   - bIsFallback：仅 Shift 4 最末位的兜底娃娃设 true（双路径触发的 fallback 路径）
 *   - bIsPearlPortrait：标记 Pearl 完美画像的娃娃（设计审查用，运行时按属性集自行评估）
 */
UCLASS(BlueprintType)
class CHECKMATE_API UDollData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// ── 属性集（13 项；与判据卡 CardId 严格对应）─────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Attributes")
	EDollPosture Posture = EDollPosture::Upright;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Attributes")
	bool bLongHair = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Attributes")
	bool bNaturalColor = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Attributes")
	bool bStyled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Attributes")
	bool bSmile = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Attributes")
	bool bPearlNecklace = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Attributes")
	bool bVeil = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Attributes")
	bool bApron = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Attributes")
	bool bRibbon = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Attributes")
	bool bWhiteGloves = false;

	// ── 眼睛 / 视觉签名 ─────────────────────────────────────────────────

	/** 按扣形制（控制 ButtonTint 与 Twist 翻转资格）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Visual")
	EButtonStyle ButtonStyle = EButtonStyle::Bone;

	// ── 流程 / 翻转触发 ─────────────────────────────────────────────────

	/** Shift 4 末位兜底娃娃。Twist 双路径触发用。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Flow")
	bool bIsFallback = false;

	/** 设计标记：本娃是 Pearl 完美画像（运行时不读，仅用于设计 review 时一眼可见）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Flow")
	bool bIsPearlPortrait = false;

	// ── 美术挂点（whitebox 全留空，编辑器侧填）─────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Visual")
	TSoftObjectPtr<USkeletalMesh> BodyMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Visual")
	TSoftObjectPtr<UAnimationAsset> PostureAnim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Visual")
	TSoftObjectPtr<UStaticMesh> HairMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Visual")
	TSoftObjectPtr<UMaterialInterface> HairMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Accessories")
	TSoftObjectPtr<UStaticMesh> PearlNecklaceMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Accessories")
	TSoftObjectPtr<UStaticMesh> VeilMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Accessories")
	TSoftObjectPtr<UStaticMesh> ApronMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Accessories")
	TSoftObjectPtr<UStaticMesh> RibbonMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Doll|Accessories")
	TSoftObjectPtr<UStaticMesh> WhiteGlovesMesh;

	/** 按 CardId 取本娃属性的 bool/enum 值。EvaluateDoll 用。 */
	bool GetAttributeByCardId(FName CardId) const;
};
