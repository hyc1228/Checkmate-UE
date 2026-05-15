// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpawnedDoll.generated.h"

class UDollData;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UEyeStateComponent;
class AInspectionStationActor;

/**
 * 检验台上的单只娃娃 actor。
 *
 * 由 AInspectionStationActor::SpawnNextDoll 创建：
 *   - 传送带上从右滑入到中央检验位
 *   - 等待玩家裁决
 *   - Approve → 继续右移出场 → destroy
 *   - Reject → 下落到 RejectChute → destroy
 *
 * Whitebox 阶段视觉：
 *   - BodyMesh 槽位为空时挂个默认 cube placeholder
 *   - Accessory mesh 全部 attach 到 socket（编辑器侧给 BodyMesh 加 socket：HairSocket / NecklaceSocket / VeilSocket / ApronSocket / RibbonSocket / GlovesSocket）
 *
 * 美术做完后直接替换 UDollData 里的 mesh 软指针即可，本类无需改。
 */
UCLASS()
class CHECKMATE_API ASpawnedDoll : public AActor
{
	GENERATED_BODY()

public:
	ASpawnedDoll();

	/** 主体 mesh（whitebox = cube；美术 slot 进来后 = SkeletalMesh）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Doll")
	USkeletalMeshComponent* BodyMesh = nullptr;

	/** Placeholder cube（BodyMesh 为空时显示）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Doll")
	UStaticMeshComponent* PlaceholderCube = nullptr;

	/** 眼睛状态组件（默认 ButtonEyed）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Doll")
	UEyeStateComponent* EyeState = nullptr;

	UFUNCTION(BlueprintCallable, Category="Doll")
	void InitializeFromData(UDollData* InData);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Doll")
	UDollData* GetData() const { return DollData; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	UDollData* DollData = nullptr;

	/** Accessory mesh 容器（attach 到 socket 上的临时 UStaticMeshComponent）。 */
	UPROPERTY()
	TArray<UStaticMeshComponent*> AccessoryMeshes;

	void ApplyVisualFromData();
	void AttachAccessoryToSocket(UStaticMesh* Mesh, FName SocketName);
};
