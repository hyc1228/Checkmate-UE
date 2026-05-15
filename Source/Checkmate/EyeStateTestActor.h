// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EyeStateTypes.h"
#include "EyeStateTestActor.generated.h"

class UStaticMeshComponent;
class UEyeStateComponent;

/**
 * Substrate 眼睛尖刺测试 Actor。
 *
 * 只服务于 5/15 这次 spike：摆进 L_Spike_EyeState.umap，跑 PIE，
 * 用控制台命令验证 M_Eye_Substrate 材质能不能跑出按扣 ↔ 机械的过渡。
 *
 * PIE 控制台命令（Actor 实例在世界里即可识别）：
 *   - Eye Toggle            → 1.5s 平滑切换
 *   - Eye SetBlend 0.5      → 强制中间帧，手动 scrub 验证
 *   - Eye SetStyle Brass    → 切按扣染色（Pearl/Bone/Brass/Resin）
 *
 * 尖刺验过即可，整合进玩家 Pawn 是 Whitebox Phase 3.3 的事。
 */
UCLASS()
class CHECKMATE_API AEyeStateTestActor : public AActor
{
	GENERATED_BODY()

public:
	AEyeStateTestActor();

	/** 球体或临时玩偶 mesh。编辑器里在 details 面板指 SM_Sphere 即可。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="EyeSpike")
	UStaticMeshComponent* EyeMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="EyeSpike")
	UEyeStateComponent* EyeState = nullptr;

	// ── PIE 控制台命令（UFUNCTION(Exec)） ────────────────────────────────────

	/** 1.5s 平滑切换到反面。 */
	UFUNCTION(Exec, Category="EyeSpike")
	void EyeToggle();

	/** 立即把混合值钉到 [0..1] 之间，手动 scrub。 */
	UFUNCTION(Exec, Category="EyeSpike")
	void EyeSetBlend(float Alpha);

	/** 切按扣染色，参数取 Pearl / Bone / Brass / Resin（大小写不敏感）。 */
	UFUNCTION(Exec, Category="EyeSpike")
	void EyeSetStyle(const FString& StyleName);

	/** 立即切换（不过渡），用于排查"过渡曲线 vs 终态外观"两件事谁出问题。 */
	UFUNCTION(Exec, Category="EyeSpike")
	void EyeSnap(const FString& StateName);

protected:
	virtual void BeginPlay() override;
};
