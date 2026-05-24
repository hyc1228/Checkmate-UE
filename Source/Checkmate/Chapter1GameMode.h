// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "InspectionScreen.h"  // for FShiftResult / FOnShiftCompleted signature
#include "Chapter1GameMode.generated.h"

class UCardSelectionScreen;
class UCardData;
class UDollData;

/**
 * Ch1 slice 灯盒版 GameMode：把「选卡屏幕 → 检验屏幕 → 班次结束」串成一个完整循环。
 *
 * 灯盒范围（slice v0.2 锁定）：
 *   - 1 班次
 *   - 1 次选卡（玩家从 PoolCards 选 K 张）
 *   - K 张判据卡检验 DollSequence 中的 3 只娃娃
 *   - 班次结束在 log 打统计；不进入下一班
 *
 * 在 Editor 里建一个 BP_Chapter1GameMode（继承本类），填以下字段：
 *   - CardSelectionWidgetClass = WBP_CardSelectionScreen
 *   - InspectionWidgetClass    = WBP_InspectionScreen
 *   - ShiftPoolCards           = N 张 UCardData asset（实验性 slice 给 6-8 张即可）
 *   - ShiftDollSequence        = 3 个 UDollData asset
 *   - K                        = 3
 *   - AssemblyTimerSec         = 30.0
 *
 * 然后把这个 BP 设为测试 Level 的 GameMode Override（World Settings）。
 */
UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API AChapter1GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AChapter1GameMode();

	UPROPERTY(EditDefaultsOnly, Category="Ch1|Slice")
	TSubclassOf<UCardSelectionScreen> CardSelectionWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Ch1|Slice")
	TSubclassOf<UInspectionScreen> InspectionWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Ch1|Slice|Data")
	TArray<UCardData*> ShiftPoolCards;

	UPROPERTY(EditDefaultsOnly, Category="Ch1|Slice|Data")
	TArray<UDollData*> ShiftDollSequence;

	UPROPERTY(EditDefaultsOnly, Category="Ch1|Slice|Tuning", meta=(ClampMin="1"))
	int32 K = 3;

	UPROPERTY(EditDefaultsOnly, Category="Ch1|Slice|Tuning", meta=(ClampMin="1.0"))
	float AssemblyTimerSec = 30.0f;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	UCardSelectionScreen* ActiveCardScreen = nullptr;

	UPROPERTY()
	UInspectionScreen* ActiveInspectionScreen = nullptr;

	UFUNCTION()
	void HandleCardsAssembled(const TArray<UCardData*>& SelectedCards);

	UFUNCTION()
	void HandleShiftCompleted(FShiftResult Result);

	void SetUIInputMode();
};
