// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CardData.generated.h"

class UTexture2D;

/**
 * 判据卡的维度（v1.2：4 维度）。
 * 用来在判定时定位娃娃属性集中的哪个字段。
 */
UENUM(BlueprintType)
enum class ECardDimension : uint8
{
	Posture     UMETA(DisplayName = "姿态 Posture"),
	Hair        UMETA(DisplayName = "发型 Hairstyle"),
	Expression  UMETA(DisplayName = "表情 Expression"),
	Accessory   UMETA(DisplayName = "饰物 Accessory")
};

/**
 * 单张判据卡的元数据 DataAsset。
 *
 * Slice 内总共 13 张这类 asset：
 *   - Posture 4 张（BalletPose / Bowing / Upright / SeatedProper）
 *   - Hair 3 张（LongHair / NaturalColor / Styled）
 *   - Expression 1 张（Smile）
 *   - Accessory 5 张（PearlNecklace / Veil / Apron / Ribbon / WhiteGloves）
 *
 * Pearl-compatible（玩家自己 = 完美芭蕾舞者 silhouette）的 6 张应设 bIsPearlCompatible=true：
 *   BalletPose, LongHair, NaturalColor, Styled, Smile, PearlNecklace
 */
UCLASS(BlueprintType)
class CHECKMATE_API UCardData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 卡 ID（唯一标识符，如 "BalletPose"、"LongHair"）。匹配 UDollData 字段名时用这个。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	FName CardId;

	/** 维度归类。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	ECardDimension Dimension = ECardDimension::Posture;

	/** 卡面字面要求（中文显示），如"必须微笑"、"必须留长发"。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	FText DisplayLabel;

	/** 卡面 icon（木刻风格小图）。Whitebox 阶段可空。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	TSoftObjectPtr<UTexture2D> IconTexture;

	/** 是否 Pearl-compatible（构成玩家自己画像的卡）。决定是否启用 holographic foil 视效。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	bool bIsPearlCompatible = false;

	/** 维度内的具体值（如 "BalletPose"）。Posture 用 enum，其他用 bool。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card", meta=(EditCondition="Dimension==ECardDimension::Posture"))
	FName PostureValue;
};
