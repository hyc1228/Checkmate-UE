// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CheckmateAudioSubsystem.generated.h"

class USoundBase;

/**
 * Cue 注册表条目。
 */
USTRUCT(BlueprintType)
struct FAudioCueEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	FName CueName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	TSoftObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0.0", ClampMax="2.0"))
	float Volume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ClampMin="0.5", ClampMax="2.0"))
	float Pitch = 1.0f;
};

/**
 * Audio cue 注册表 + dispatch。
 *
 * Whitebox：所有 cue 调用都 print log；周末音轨做完后，
 *   把 USoundCue 引用填进编辑器侧创建的 UDataAsset（CueRegistry）即可。
 *
 * 已注册的 cue name（其它 subsystem 调 PlayCue 时用这些）：
 *   SFX_StampApprove / SFX_StampReject / SFX_DollSlideIn / SFX_DollDropChute /
 *   SFX_CardPickUp / SFX_CardSnap / SFX_ShiftBell / SFX_SubliminalSurge /
 *   SFX_EyeFall / SFX_IrisBloom / SFX_UIBreakdown / SFX_TwistComplete /
 *   AMB_FactoryLoop / AMB_TwistDrone
 */
UCLASS()
class CHECKMATE_API UCheckmateAudioSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Audio")
	void PlayCue(FName CueName);

	UFUNCTION(BlueprintCallable, Category="Audio")
	void RegisterCue(const FAudioCueEntry& Entry);

	UFUNCTION(BlueprintCallable, Category="Audio")
	void RegisterCues(const TArray<FAudioCueEntry>& Entries);

private:
	UPROPERTY()
	TMap<FName, FAudioCueEntry> Registry;
};
