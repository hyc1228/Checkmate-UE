// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Ch2LevelData.h"
#include "Ch2GameMode.generated.h"

class ACh2Pawn;
class UStaticMesh;
class UMaterialInterface;

/** 一个爆炸玩偶的运行时状态（小丑日字跳后留在前一 cell 上）。 */
USTRUCT(BlueprintType)
struct FCh2PuppetState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FIntPoint Cell = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly)
	int32 TurnsRemaining = 0;

	UPROPERTY(BlueprintReadOnly)
	AActor* RuntimeActor = nullptr;
};

/**
 * Ch2 GameMode（v0.1 原型）。
 *
 * BeginPlay：
 *   1. 从 LevelData 读棋盘 → BuildLevel：spawn 地板格子 / 墙 / 拾取 / 出口 actors
 *   2. spawn 玩家 pawn 到 Start cell
 *   3. 设固定俯视相机
 *
 * 运行时职责：
 *   - 提供 IsWall(Cell) / IsInBounds(Cell) 给 Pawn 查
 *   - Pawn 移动后通知 → NotifyPawnEnteredCell：检查拾取 / 胜利
 *   - 回合计数 + 爆炸玩偶推进（Tier B）
 */
UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API ACh2GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACh2GameMode();

	/** 关卡数据（必填）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ch2")
	UCh2LevelData* LevelData = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Ch2|Classes")
	TSubclassOf<ACh2Pawn> PawnClass;

	/** 各类 cell 的占位 mesh。默认用 Engine/BasicShapes/Cube。 */
	UPROPERTY(EditDefaultsOnly, Category="Ch2|Visual")
	UStaticMesh* FloorMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Ch2|Visual")
	UStaticMesh* WallMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Ch2|Visual")
	UStaticMesh* PickupMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Ch2|Visual")
	UStaticMesh* ExitMesh = nullptr;

	/** 相机到棋盘中心的距离（unit）。8×8 200-cell 推荐 2000。 */
	UPROPERTY(EditDefaultsOnly, Category="Ch2|Camera", meta=(ClampMin="200"))
	float CameraDistance = 2000.0f;

	/** 相机俯角（度，0 = 水平，90 = 正俯视）。45 = 斜俯视 isometric 风。 */
	UPROPERTY(EditDefaultsOnly, Category="Ch2|Camera", meta=(ClampMin="10.0", ClampMax="89.0"))
	float CameraTiltDegrees = 50.0f;

	/** 相机绕棋盘 yaw 角（0 = 从 -X 方向看 +X）。 */
	UPROPERTY(EditDefaultsOnly, Category="Ch2|Camera", meta=(ClampMin="-180.0", ClampMax="180.0"))
	float CameraYawDegrees = 0.0f;

	/** 查 cell 类型（Pawn 查 wall / pickup / exit 用）。 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ch2")
	ECh2CellType GetCellType(FIntPoint Cell) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ch2")
	bool IsBlocked(FIntPoint Cell) const;  // Wall or Destructible

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ch2")
	bool IsInBounds(FIntPoint Cell) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ch2")
	int32 GetGridWidth() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ch2")
	int32 GetGridHeight() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ch2")
	float GetCellSize() const;

	/** Pawn 进入新 cell 后通知，触发拾取 / 胜利等。 */
	UFUNCTION(BlueprintCallable, Category="Ch2")
	void NotifyPawnEnteredCell(FIntPoint Cell);

	/** Pawn 完成一次移动（包含 from / to / 是否 Clown 模式）。GameMode 用这个推进回合 + spawn puppet。 */
	UFUNCTION(BlueprintCallable, Category="Ch2")
	void NotifyPawnMoved(FIntPoint FromCell, FIntPoint ToCell, bool bWasClownMove);

	/** 爆炸玩偶倒计时：小丑跳后第 N 个回合爆炸（默认 2）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch2|Tier B", meta=(ClampMin="1"))
	int32 PuppetExplodeAfterTurns = 2;

	/** 爆炸玩偶占位 mesh（默认 cube）。 */
	UPROPERTY(EditDefaultsOnly, Category="Ch2|Visual")
	UStaticMesh* PuppetMesh = nullptr;

protected:
	virtual void BeginPlay() override;

	void BuildLevel();
	void SetUpTopDownCamera();

	UPROPERTY()
	ACh2Pawn* ActivePawn = nullptr;

	/** runtime 实例化的 cell-decoration actors（按 cell 索引）。 */
	UPROPERTY()
	TMap<FIntPoint, AActor*> CellActors;

	/** 当前活跃的爆炸玩偶。 */
	UPROPERTY()
	TArray<FCh2PuppetState> Puppets;

	void SpawnPuppetAt(FIntPoint Cell);
	void TickPuppets();      // 每次玩家移动后调，倒计时 -1
	void ExplodePuppet(int32 PuppetIdx);  // 爆炸：清掉相邻 Destructible
	void RefreshPuppetVisual(FCh2PuppetState& P);
};
