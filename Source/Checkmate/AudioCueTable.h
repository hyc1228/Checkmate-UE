// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AudioCueTable.generated.h"

class USoundBase;

/**
 * 音频 cue 表（设计师在此编辑）。
 *
 * 两条并行的 lookup：
 *   - NativeCues   : FName -> USoundBase*（原生 USoundCue / USoundWave）
 *   - FmodEventPaths: FName -> "event:/Path/In/FMOD"
 *
 * AudioService 优先用 FMOD，没装/没配的 key 退到 NativeCues。
 * 这样：1) PV 前先丢原生占位听响；2) 装好 FMOD 把 key 写入 FmodEventPaths，无须改调用点。
 *
 * 推荐 key 命名（见 docs/Audio-Spec.md）：
 *   UI.Hover / UI.Click
 *   Ch1.Stamp / Ch1.Toss / Ch1.Correct / Ch1.Wrong
 *   Ch2.Move / Ch2.Explode / Ch2.Ritual / Ch2.Victory
 *   Amb.Ch1 / Amb.Ch2 (长循环 ambient)
 */
UCLASS(BlueprintType)
class CHECKMATE_API UAudioCueTable : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 原生 cue（兜底层）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio", meta=(ForceInlineRow))
	TMap<FName, USoundBase*> NativeCues;

	/** FMOD event path（"event:/UI/Stamp"）。AudioService 优先查这个。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio", meta=(ForceInlineRow))
	TMap<FName, FString> FmodEventPaths;

	/** 默认音量倍率（per-key）。1.0 = 原值。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio", meta=(ForceInlineRow))
	TMap<FName, float> VolumeOverrides;
};
