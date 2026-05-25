// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "AudioService.h"

#include "AudioCueTable.h"
#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "UObject/UObjectGlobals.h"

// 装好 FMOD plugin 后，在 Checkmate.Build.cs 里：
//   PrivateDependencyModuleNames.Add("FMODStudio");
//   PublicDefinitions.Add("WITH_FMOD_CHECKMATE=1");
// 然后下面的 include + 调用路径会被启用。
#if WITH_FMOD_CHECKMATE
	#include "FMODBlueprintStatics.h"
	#include "FMODEvent.h"
#endif

namespace
{
	constexpr TCHAR DefaultTablePath[] = TEXT("/Game/Audio/DA_AudioCues.DA_AudioCues");
}

void UAudioService::PlayCueStatic(const UObject* WorldContext, FName Key, float VolumeMul)
{
	if (!WorldContext) return;
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull))
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UAudioService* Svc = GI->GetSubsystem<UAudioService>())
			{
				Svc->PlayCue(Key, VolumeMul);
			}
		}
	}
}

void UAudioService::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 自动加载默认 cue table（设计师可以 Override）
	if (!CueTable)
	{
		CueTable = LoadObject<UAudioCueTable>(nullptr, DefaultTablePath);
		if (CueTable)
		{
			UE_LOG(LogTemp, Log, TEXT("[AudioService] Loaded default CueTable %s"), DefaultTablePath);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[AudioService] No CueTable at %s — call SetCueTable() or create the asset."), DefaultTablePath);
		}
	}
}

void UAudioService::Deinitialize()
{
	if (AmbientComponent)
	{
		AmbientComponent->Stop();
		AmbientComponent = nullptr;
	}
	Super::Deinitialize();
}

USoundBase* UAudioService::ResolveNativeCue(FName Key) const
{
	if (!CueTable) return nullptr;
	USoundBase* const* Found = CueTable->NativeCues.Find(Key);
	return Found ? *Found : nullptr;
}

bool UAudioService::ResolveFmodEvent(FName Key, FString& OutEventPath) const
{
	if (!CueTable) return false;
	const FString* Found = CueTable->FmodEventPaths.Find(Key);
	if (!Found || Found->IsEmpty()) return false;
	OutEventPath = *Found;
	return true;
}

float UAudioService::ResolveVolume(FName Key, float VolumeMul) const
{
	float Base = 1.0f;
	if (CueTable)
	{
		if (const float* Override = CueTable->VolumeOverrides.Find(Key))
		{
			Base = *Override;
		}
	}
	return FMath::Clamp(Base * VolumeMul * MasterVolume, 0.0f, 4.0f);
}

void UAudioService::PlayCue(FName Key, float VolumeMul)
{
	if (!bEnabled || Key == NAME_None) return;

	const float Volume = ResolveVolume(Key, VolumeMul);

#if WITH_FMOD_CHECKMATE
	FString FmodPath;
	if (ResolveFmodEvent(Key, FmodPath))
	{
		UWorld* World = GetWorld();
		// FMOD 2D one-shot
		if (UFMODEvent* Event = LoadObject<UFMODEvent>(nullptr, *FmodPath))
		{
			UFMODBlueprintStatics::PlayEvent2D(World, Event, true /*bAutoPlay*/);
			return;
		}
		UE_LOG(LogTemp, Warning, TEXT("[AudioService] FMOD event not found: %s (key %s)"), *FmodPath, *Key.ToString());
		// fall through to native
	}
#endif

	if (USoundBase* Cue = ResolveNativeCue(Key))
	{
		UGameplayStatics::PlaySound2D(this, Cue, Volume);
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("[AudioService] No cue for key '%s'"), *Key.ToString());
	}
}

void UAudioService::PlayCueAtLocation(FName Key, FVector Location, float VolumeMul)
{
	if (!bEnabled || Key == NAME_None) return;
	const float Volume = ResolveVolume(Key, VolumeMul);

#if WITH_FMOD_CHECKMATE
	FString FmodPath;
	if (ResolveFmodEvent(Key, FmodPath))
	{
		UWorld* World = GetWorld();
		if (UFMODEvent* Event = LoadObject<UFMODEvent>(nullptr, *FmodPath))
		{
			UFMODBlueprintStatics::PlayEventAtLocation(World, Event, FTransform(Location), true);
			return;
		}
	}
#endif

	if (USoundBase* Cue = ResolveNativeCue(Key))
	{
		UGameplayStatics::PlaySoundAtLocation(this, Cue, Location, Volume);
	}
}

void UAudioService::PlayAmbient(FName Key, float FadeSeconds)
{
	if (!bEnabled || Key == NAME_None)
	{
		StopAmbient(FadeSeconds);
		return;
	}
	if (CurrentAmbientKey == Key && AmbientComponent && AmbientComponent->IsPlaying())
	{
		return;  // 同一首已经在放
	}

	// 停旧
	if (AmbientComponent)
	{
		AmbientComponent->FadeOut(FadeSeconds, 0.0f);
		AmbientComponent = nullptr;
	}

	const float Volume = ResolveVolume(Key, 1.0f);
	USoundBase* Cue = ResolveNativeCue(Key);
	// 注：FMOD ambient 走单独 event 系统，这里先用原生 ambient；FMOD 集成后再扩。
	if (Cue)
	{
		AmbientComponent = UGameplayStatics::SpawnSound2D(this, Cue, Volume);
		if (AmbientComponent)
		{
			AmbientComponent->bAutoDestroy = false;
			AmbientComponent->FadeIn(FadeSeconds, Volume);
			CurrentAmbientKey = Key;
		}
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("[AudioService] No ambient cue for key '%s'"), *Key.ToString());
	}
}

void UAudioService::StopAmbient(float FadeSeconds)
{
	if (AmbientComponent)
	{
		AmbientComponent->FadeOut(FadeSeconds, 0.0f);
		AmbientComponent = nullptr;
	}
	CurrentAmbientKey = NAME_None;
}
