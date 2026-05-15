// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "JudgmentCardSubsystem.h"  // EVerdictCategory
#include "SubliminalLayerSubsystem.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class APostProcessVolume;

/**
 * 按扣切边 post-process 驱动。
 *
 * 订阅 OnDollVerdict，按公式累积 EdgeOpacity：
 *   EdgeOpacity = clamp(MisjudgmentCount * 0.15 + 0.05, 0.05, 0.60)
 *
 * Twist 演出期间被 ForceOpacityTo(1.0) 覆盖（一次性，不再受累计影响）。
 *
 * 编辑器侧需要：
 *   1. 创建 PostProcessVolume（unbounded）放进 Level
 *   2. 创建 M_SubliminalButtonEdge post-process material
 *      参数：EdgeOpacity (Scalar 0-1, default 0.05)
 *      效果：屏幕边缘按扣形状 vignette
 *   3. PostProcessVolume → Settings → PostProcess Materials 加入 M_SubliminalButtonEdge
 *   4. 运行时调 RegisterSubliminalMaterial(M_SubliminalButtonEdge) 启动 MID 接管
 */
UCLASS()
class CHECKMATE_API USubliminalLayerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="Subliminal")
	void RegisterSubliminalMaterial(UMaterialInterface* InMaterial);

	UFUNCTION(BlueprintCallable, Category="Subliminal")
	void ForceOpacityTo(float Opacity);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Subliminal")
	float GetCurrentOpacity() const { return CurrentOpacity; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Subliminal")
	int32 GetMisjudgmentCount() const { return MisjudgmentCount; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Subliminal")
	FName OpacityParamName = TEXT("EdgeOpacity");

private:
	UFUNCTION()
	void HandleDollVerdict(class UDollData* Doll, EDollVerdict Verdict, EVerdictCategory Category);

	void RefreshMID();
	void PushOpacity(float Opacity);

	UPROPERTY()
	UMaterialInstanceDynamic* SubliminalMID = nullptr;

	UPROPERTY()
	UMaterialInterface* SourceMaterial = nullptr;

	float CurrentOpacity = 0.05f;
	int32 MisjudgmentCount = 0;
};
