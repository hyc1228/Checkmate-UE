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

UENUM(BlueprintType)
enum class ECh1CardColor : uint8
{
	BlackCriterion UMETA(DisplayName = "Black / Criterion"),
	RedBonus       UMETA(DisplayName = "Red / Bonus"),
	Special        UMETA(DisplayName = "Special")
};

UENUM(BlueprintType)
enum class ECh1CardEffect : uint8
{
	None                 UMETA(DisplayName = "None"),
	PriceTag             UMETA(DisplayName = "Price Tag"),
	HighHeelsValue       UMETA(DisplayName = "High Heels Value"),
	ComboDiscipline      UMETA(DisplayName = "Combo Discipline"),
	LockedRose           UMETA(DisplayName = "Locked Rose"),
	BowDiscipline        UMETA(DisplayName = "Bow Discipline"),
	HeartGaze            UMETA(DisplayName = "Heart Gaze"),
	RoseGaze             UMETA(DisplayName = "Rose Gaze"),
	TheGaze              UMETA(DisplayName = "The Gaze"),
	DangerousTears       UMETA(DisplayName = "Dangerous Tears"),
	SmilingThroughTears  UMETA(DisplayName = "Smiling Through Tears"),
	WhiteGloves          UMETA(DisplayName = "White Gloves"),
	PearlStandard        UMETA(DisplayName = "Pearl Standard"),
	ApronDuty            UMETA(DisplayName = "Apron Duty"),
	RecallExemption      UMETA(DisplayName = "Recall Exemption"),
	SwordAndSerpent      UMETA(DisplayName = "Sword and Serpent"),
	QueenGambit          UMETA(DisplayName = "Queen's Gambit"),
	CheckmateDiscard      UMETA(DisplayName = "Checkmate / Discard")
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	FText EnglishDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Tooltip", meta=(MultiLine="true"))
	FText SkillDescriptionMarkup;

	/** 卡面 icon（木刻风格小图）。Whitebox 阶段可空。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	TSoftObjectPtr<UTexture2D> IconTexture;

	/** 是否 Pearl-compatible（构成玩家自己画像的卡）。决定是否启用 holographic foil 视效。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card")
	bool bIsPearlCompatible = false;

	/** 维度内的具体值（如 "BalletPose"）。Posture 用 enum，其他用 bool。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card", meta=(EditCondition="Dimension==ECardDimension::Posture"))
	FName PostureValue;

	/** New Ch1 piecework identity: black cards add criteria; red cards only change value/combo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Piecework")
	ECh1CardColor CardColor = ECh1CardColor::BlackCriterion;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Piecework")
	ECh1CardEffect PieceworkEffect = ECh1CardEffect::None;

	/** Additive value bonus for simple red-card tuning. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Piecework", meta=(ClampMin="0"))
	int32 FlatGoodValueBonus = 0;

	/** Multiplicative value bonus. 1.0 means no change. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Piecework", meta=(ClampMin="0.0"))
	float GoodValueMultiplier = 1.0f;

	/** Extra money for correct rejects enabled by discard-oriented black cards. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Piecework", meta=(ClampMin="0"))
	int32 CorrectRejectBonus = 0;

	/** Holographic mirror-foil treatment for bridge or rare cards. Kept under the old variable name for asset compatibility. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Holographic Foil")
	bool bUseNegativeLaserFoil = false;

	/** Built-in score multiplier granted while this foil card is in the active deck. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Holographic Foil", meta=(ClampMin="1.0"))
	float NegativeLaserScoreMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Card|Holographic Foil", meta=(ClampMin="0.0", ClampMax="1.0"))
	float NegativeLaserVisualStrength = 0.0f;

	UFUNCTION(CallInEditor, BlueprintCallable, Category="Card|Piecework")
	bool ApplyCh1SlicePreset();

	UFUNCTION(BlueprintCallable, Category="Card|Piecework")
	static bool GetCh1SlicePreset(FName InCardId, FText& OutEnglishName, ECh1CardColor& OutColor, ECh1CardEffect& OutEffect);
};
