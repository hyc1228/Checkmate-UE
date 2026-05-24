// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Ch2LevelData.generated.h"

UENUM(BlueprintType)
enum class ECh2CellType : uint8
{
	Empty       UMETA(DisplayName="空"),
	Wall        UMETA(DisplayName="墙"),
	Destructible UMETA(DisplayName="可破坏"),
	ClownPickup UMETA(DisplayName="小丑残骸（拾取）"),
	Exit        UMETA(DisplayName="出口"),
	Start       UMETA(DisplayName="起点"),
};

/**
 * Ch2 关卡数据（v0.1 原型）。
 *
 * 数据驱动：关卡的所有 cell（墙 / 拾取 / 出口 / 起点）都存在这里，runtime 由
 * ACh2GameMode 读取并 spawn 对应 actor。设计师只编辑这个 asset，不放 actor。
 *
 * 后续编辑器 UI（Editor Utility Widget）会提供 8×8 按钮点选 cell 类型。
 */
UCLASS(BlueprintType)
class CHECKMATE_API UCh2LevelData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 棋盘宽（cell 数）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ch2|Grid", meta=(ClampMin="3", ClampMax="32"))
	int32 GridWidth = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ch2|Grid", meta=(ClampMin="3", ClampMax="32"))
	int32 GridHeight = 8;

	/** 单 cell 边长（unit）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ch2|Grid", meta=(ClampMin="50"))
	float CellSize = 200.0f;

	/** 非空格 → 类型。未列出的 cell 视为 Empty。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ch2|Cells", meta=(ForceInlineRow))
	TMap<FIntPoint, ECh2CellType> Cells;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ch2")
	ECh2CellType GetCellType(FIntPoint Cell) const
	{
		if (const ECh2CellType* T = Cells.Find(Cell)) return *T;
		return ECh2CellType::Empty;
	}

	/** 找首个匹配类型的 cell（Start / Exit / ClownPickup 通常只有一个）。 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ch2")
	FIntPoint FindCellOfType(ECh2CellType Type) const
	{
		for (const TPair<FIntPoint, ECh2CellType>& P : Cells)
		{
			if (P.Value == Type) return P.Key;
		}
		return FIntPoint(-1, -1);
	}
};
