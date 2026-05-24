// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "Components/Button.h"
#include "Ch2LevelData.h"
#include "Ch2LevelEditorWidget.generated.h"

class UCh2LevelEditorWidget;

/** 自带 cell 坐标的按钮，避免给 64 个 UButton 各写一个 UFUNCTION 回调。 */
UCLASS()
class CHECKMATE_API UCh2CellButton : public UButton
{
	GENERATED_BODY()
public:
	FIntPoint Cell = FIntPoint::ZeroValue;

	UPROPERTY()
	UCh2LevelEditorWidget* Owner = nullptr;

	UFUNCTION()
	void OnInternalClicked();
};

class UUniformGridPanel;
class UButton;
class UTextBlock;

/**
 * Ch2 关卡编辑器（Editor Utility Widget）。
 *
 * 用法：
 *   1. Content Browser 右键 WBP_Ch2LevelEditor → Run Editor Utility Widget
 *   2. 在 TargetAsset 槽拖一个 UCh2LevelData asset（如 DA_Ch2Level_01）
 *   3. 工具栏选 brush（Empty / Wall / Destructible / Pickup / Exit / Start）
 *   4. 在 8×8 网格按钮上点选 → cell 类型按 brush 涂
 *   5. 按 SaveButton → 写回 TargetAsset 并 Save 到磁盘
 *
 * WBP 子类必须提供这些命名 widget：
 *   - GridPanel (UUniformGridPanel) —— 8×8 按钮容器（C++ 自己 spawn）
 *   - BrushBtn_Empty / Wall / Destructible / Pickup / Exit / Start (UButton)
 *   - BrushLabel (UTextBlock) —— 显示当前 brush 名
 *   - SaveButton (UButton)
 *   - StatusText (UTextBlock) —— 显示 save/load 状态
 */
UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API UCh2LevelEditorWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	/** 编辑目标 DataAsset。属性面板拖进来，或 Pick 按钮选。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch2 Editor")
	UCh2LevelData* TargetAsset = nullptr;

	/** 单 cell 按钮像素大小。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch2 Editor")
	float CellButtonSize = 48.0f;

	UFUNCTION(BlueprintCallable, Category="Ch2 Editor")
	void RebuildGrid();

	UFUNCTION(BlueprintCallable, Category="Ch2 Editor")
	void SaveToAsset();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UUniformGridPanel* GridPanel = nullptr;

	// 6 个 brush 按钮（点选当前画笔类型）
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UButton* BrushBtn_Empty = nullptr;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UButton* BrushBtn_Wall = nullptr;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UButton* BrushBtn_Destructible = nullptr;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UButton* BrushBtn_Pickup = nullptr;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UButton* BrushBtn_Exit = nullptr;
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UButton* BrushBtn_Start = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UTextBlock* BrushLabel = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UButton* SaveButton = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UTextBlock* StatusText = nullptr;

	UFUNCTION() void OnBrushEmptyClicked();
	UFUNCTION() void OnBrushWallClicked();
	UFUNCTION() void OnBrushDestructibleClicked();
	UFUNCTION() void OnBrushPickupClicked();
	UFUNCTION() void OnBrushExitClicked();
	UFUNCTION() void OnBrushStartClicked();
	UFUNCTION() void OnSaveClicked();

private:
	ECh2CellType CurrentBrush = ECh2CellType::Wall;

	/** Runtime spawn 的 cell 按钮（按 FIntPoint 索引）。 */
	UPROPERTY()
	TMap<FIntPoint, UCh2CellButton*> CellButtons;

public:
	/** UCh2CellButton 回调用。 */
	void HandleCellButtonClicked(FIntPoint Cell);

private:

	void SpawnGridButtons();
	void RefreshAllCellVisuals();
	void RefreshCellVisual(FIntPoint Cell);
	void OnCellClicked_Internal(FIntPoint Cell);
	void SetBrush(ECh2CellType NewBrush);

	static FLinearColor ColorForType(ECh2CellType Type);
	static FString LabelForType(ECh2CellType Type);
};
