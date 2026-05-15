// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EyeStateTypes.generated.h"

/**
 * 眼睛状态机的三态。
 *
 * - ButtonEyed：按扣眼（玩偶 / 觉醒前的玩家）
 * - Transitioning：过渡中（Twist Sequence 的 EvtEyeFall → EvtIrisBloom 窗口）
 * - MechanicalEyed：机械眼（觉醒后 / NPC 监工）
 */
UENUM(BlueprintType)
enum class EEyeState : uint8
{
	ButtonEyed       UMETA(DisplayName = "按扣眼 Button"),
	Transitioning    UMETA(DisplayName = "过渡中 Transitioning"),
	MechanicalEyed   UMETA(DisplayName = "机械眼 Mechanical")
};

/**
 * 按扣材质风格（不同玩偶可以挂不同款）。
 * 仅影响 ButtonTint 向量参数。
 */
UENUM(BlueprintType)
enum class EButtonStyle : uint8
{
	Pearl   UMETA(DisplayName = "珍珠白 Pearl"),
	Bone    UMETA(DisplayName = "象牙骨 Bone"),
	Brass   UMETA(DisplayName = "黄铜 Brass"),
	Resin   UMETA(DisplayName = "树脂 Resin")
};

/**
 * Substrate 材质参数名契约。
 *
 * 默认值与 M_Eye_Substrate（编辑器侧手工创建的主材质）必须**逐字符**对齐。
 * 如果编辑器侧改名，覆盖这里的默认即可，不必改 C++ 逻辑。
 */
USTRUCT(BlueprintType)
struct CHECKMATE_API FEyeMaterialParamNames
{
	GENERATED_BODY()

	/** 标量 0–1：按扣 ↔ 机械 主混合权重（驱动 Substrate Horizontal Blend）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye|Material")
	FName BlendAlpha = TEXT("EyeBlendAlpha");

	/** 标量 0–1：机械眼瞳孔扩张 / 虹膜辉光。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye|Material")
	FName IrisBloom = TEXT("IrisBloom");

	/** 向量：按扣分支染色（每个玩偶可独立配色）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye|Material")
	FName ButtonTint = TEXT("ButtonTint");

	/** 标量 0–1：机械分支金属度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye|Material")
	FName MechanicalMetallic = TEXT("MechanicalMetallic");

	/** 标量 0–1：觉醒辉光强度（emissive）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Eye|Material")
	FName EmissiveStrength = TEXT("EmissiveStrength");
};

/**
 * 把 EButtonStyle 解析为 LinearColor。
 * 美术后续可以挪到 DataAsset；尖刺阶段先硬编码。
 */
struct CHECKMATE_API FButtonStylePalette
{
	static FLinearColor Resolve(EButtonStyle Style)
	{
		switch (Style)
		{
		case EButtonStyle::Pearl: return FLinearColor(0.96f, 0.94f, 0.90f, 1.0f);
		case EButtonStyle::Bone:  return FLinearColor(0.88f, 0.82f, 0.70f, 1.0f);
		case EButtonStyle::Brass: return FLinearColor(0.72f, 0.55f, 0.20f, 1.0f);
		case EButtonStyle::Resin: return FLinearColor(0.20f, 0.18f, 0.16f, 1.0f);
		default:                  return FLinearColor::White;
		}
	}
};
