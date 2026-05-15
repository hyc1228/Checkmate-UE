// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "JudgmentCardActor.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UCardData;
class UTexture2D;

/**
 * 3D 纸卡 actor，摆在检验台左侧木盘上的标准卡。
 *
 * 由 UTransitionManager 在 2D→3D 过渡完成时 spawn K 个，
 * 每个绑定一张 UCardData。纸卡正面用 CardData 的 IconTexture 作为
 * material parameter（M_PaperCard 主材质需要一个 BaseColorTexture 参数 + Tint）。
 *
 * Whitebox：CardMaterial 留空时用引擎默认 material，IconTexture 仍可写
 * 进 MID 但因为材质没有读取槽位，玩家看到的就是纯白 quad。这是 OK 的，
 * 因为 whitebox 验证的是 spawn / 摆放 / 数量逻辑，不是纸卡视觉。
 */
UCLASS()
class CHECKMATE_API AJudgmentCardActor : public AActor
{
	GENERATED_BODY()

public:
	AJudgmentCardActor();

	/** 纸卡 quad mesh（whitebox = 引擎默认 Plane，scale 拉成卡片比例）。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Card")
	UStaticMeshComponent* CardMesh = nullptr;

	/** 主材质：M_PaperCard。编辑器侧创建，参数：BaseColorTexture (Texture2D), Tint (Vector3)。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Card")
	UMaterialInterface* CardMaterial = nullptr;

	/** Material 参数名（编辑器侧改名时这里改默认值即可）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Card|Material")
	FName BaseColorParamName = TEXT("BaseColorTexture");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Card|Material")
	FName TintParamName = TEXT("Tint");

	UFUNCTION(BlueprintCallable, Category="Card")
	void InitializeFromData(UCardData* InData);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Card")
	UCardData* GetData() const { return CardData; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	UCardData* CardData = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* CardMID = nullptr;

	void ApplyVisualFromData();
};
