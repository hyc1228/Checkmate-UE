// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Ch2Pawn.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class ACh2GameMode;

UENUM(BlueprintType)
enum class ECh2Mode : uint8
{
	Ballet UMETA(DisplayName="芭蕾 (slide-til-wall)"),
	Clown  UMETA(DisplayName="小丑 (knight L-jump)"),
};

/**
 * Ch2 玩家娃娃 Pawn（v0.1 原型）。
 *
 * 棋盘格离散移动：
 *   - 输入：鼠标左键点目标格
 *   - 合法性由当前 Mode 决定：
 *       Ballet：从当前 cell 沿 4 方向滑到撞墙/边界为止，目标必须是该方向的「滑停点」
 *       Clown ：knight 的 8 个 L-offset 之一，且目标 cell 未被 Wall 占
 *   - 合法则 SetActorLocation 到目标 cell 中心；视当前 cell 检查是否拾取小丑残骸 / 到出口
 *
 * 与 ACh2GameMode 协作：
 *   - 查 wall / pickup / exit 列表（GameMode 在 BeginPlay 用 tag 扫关卡）
 *   - 移动后通知 GameMode 检查触发事件（拾取 / 胜利）
 */
UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API ACh2Pawn : public APawn
{
	GENERATED_BODY()

public:
	ACh2Pawn();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	USceneComponent* SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UStaticMeshComponent* BodyMesh = nullptr;

	/** 机械眼 hint：spec 锁定 Ch2 玩家 = 机械眼。Pawn 顶部小球，cyan tint。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	UStaticMeshComponent* EyeMarker = nullptr;

	/** 单 cell 边长（unit）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch2|Grid", meta=(ClampMin="50"))
	float CellSize = 200.0f;

	/** 棋盘 X/Y 边数（0..N-1）。GameMode 也可改写。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch2|Grid", meta=(ClampMin="3", ClampMax="32"))
	int32 GridSize = 8;

	UPROPERTY(BlueprintReadOnly, Category="Ch2|State")
	ECh2Mode CurrentMode = ECh2Mode::Ballet;

	/** 切换娃娃模式（外部调，如拾取小丑残骸时 GameMode 调）。 */
	UFUNCTION(BlueprintCallable, Category="Ch2")
	void SetMode(ECh2Mode NewMode);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ch2|Grid")
	FIntPoint GetCurrentCell() const;

	/** 当前模式下合法的所有目标 cell。 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ch2")
	TArray<FIntPoint> GetValidMoves() const;

	/** 尝试移动到 Target cell。返回是否成功。 */
	UFUNCTION(BlueprintCallable, Category="Ch2")
	bool TryMoveTo(FIntPoint TargetCell);

	/** Grid <-> world 工具。 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ch2|Grid")
	FVector CellToWorld(FIntPoint Cell) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ch2|Grid")
	FIntPoint WorldToCell(FVector World) const;

	/** 当前模式名（UI 用）。 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Ch2")
	FText GetModeDisplayName() const;

	/** 一次移动的 lerp 时长（秒）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch2|Feel", meta=(ClampMin="0.05"))
	float MoveDuration = 0.22f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	ACh2GameMode* GetGM() const;
	bool IsWall(FIntPoint Cell) const;
	bool IsInBounds(FIntPoint Cell) const;

	void ComputeBalletMoves(TArray<FIntPoint>& OutMoves) const;
	void ComputeClownMoves(TArray<FIntPoint>& OutMoves) const;

	// 移动 lerp 状态
	bool bMoving = false;
	FVector MoveStartLoc = FVector::ZeroVector;
	FVector MoveEndLoc = FVector::ZeroVector;
	float MoveElapsed = 0.0f;
	FIntPoint PendingTargetCell = FIntPoint::ZeroValue;
	bool bPendingClownMove = false;
	FIntPoint PendingFromCell = FIntPoint::ZeroValue;
};
