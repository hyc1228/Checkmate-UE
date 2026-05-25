// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AudioService.generated.h"

class UAudioCueTable;
class USoundBase;
class UAudioComponent;
class AActor;

/**
 * 全局音频服务（GameInstanceSubsystem）。
 *
 * 调用点只用 FName key：
 *   GetWorld()->GetGameInstance()->GetSubsystem<UAudioService>()->PlayCue("Ch1.Stamp");
 * 或便利宏 IF_AUDIO(PlayCue, "Ch1.Stamp")。
 *
 * 路由：
 *   1) 装了 FMOD（WITH_FMOD_CHECKMATE=1）且 Table->FmodEventPaths 有此 key → 播 FMOD event
 *   2) 否则查 Table->NativeCues → UGameplayStatics::PlaySound2D（或 SpawnSoundAtLocation/Attached）
 *   3) 都没有 → 静默 log warning（不崩）
 *
 * Ambient：独占一条 UAudioComponent slot，PlayAmbient 会覆盖前一条。
 */
UCLASS()
class CHECKMATE_API UAudioService : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 设计师把 DA_AudioCues 拖这里（项目启动时通过 set_cdo_property 或代码注入）。 */
	UPROPERTY(BlueprintReadWrite, Category="Audio")
	UAudioCueTable* CueTable = nullptr;

	/** 主音量（0..1）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	float MasterVolume = 1.0f;

	/** 主开关——关掉直接静默（debug 用）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio")
	bool bEnabled = true;

	/** 播一次性 cue（UI 点击 / 印章 / 爆炸 …）。 */
	UFUNCTION(BlueprintCallable, Category="Audio")
	void PlayCue(FName Key, float VolumeMul = 1.0f);

	/** 播在世界位置上（带衰减，但当前 placeholder 用 2D，留 hook）。 */
	UFUNCTION(BlueprintCallable, Category="Audio")
	void PlayCueAtLocation(FName Key, FVector Location, float VolumeMul = 1.0f);

	/** 切换 ambient 循环（旧的会 fade out，新的会 fade in）。Key=NAME_None 则停。 */
	UFUNCTION(BlueprintCallable, Category="Audio")
	void PlayAmbient(FName Key, float FadeSeconds = 1.0f);

	UFUNCTION(BlueprintCallable, Category="Audio")
	void StopAmbient(float FadeSeconds = 1.0f);

	/** 显式注入 CueTable（外部 game mode 在 BeginPlay 调）。 */
	UFUNCTION(BlueprintCallable, Category="Audio")
	void SetCueTable(UAudioCueTable* InTable) { CueTable = InTable; }

	/** 便利 static：任何 WorldContextObject 都能一行播 cue。 */
	static void PlayCueStatic(const UObject* WorldContext, FName Key, float VolumeMul = 1.0f);

private:
	UPROPERTY()
	UAudioComponent* AmbientComponent = nullptr;

	/** 当前 ambient key（避免重复触发同一段）。 */
	FName CurrentAmbientKey = NAME_None;

	/** Table lookup helpers。 */
	USoundBase* ResolveNativeCue(FName Key) const;
	bool ResolveFmodEvent(FName Key, FString& OutEventPath) const;
	float ResolveVolume(FName Key, float VolumeMul) const;
};
