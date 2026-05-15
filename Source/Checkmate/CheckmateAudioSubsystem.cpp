// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "CheckmateAudioSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

void UCheckmateAudioSubsystem::PlayCue(FName CueName)
{
	const FAudioCueEntry* Entry = Registry.Find(CueName);
	if (!Entry || Entry->Sound.IsNull())
	{
		UE_LOG(LogTemp, Verbose, TEXT("Audio: PlayCue('%s')（未注册 / 未填 Sound，stub log）"), *CueName.ToString());
		return;
	}

	USoundBase* Sound = Entry->Sound.LoadSynchronous();
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound, Entry->Volume, Entry->Pitch);
	}
}

void UCheckmateAudioSubsystem::RegisterCue(const FAudioCueEntry& Entry)
{
	Registry.Add(Entry.CueName, Entry);
}

void UCheckmateAudioSubsystem::RegisterCues(const TArray<FAudioCueEntry>& Entries)
{
	for (const FAudioCueEntry& E : Entries)
	{
		Registry.Add(E.CueName, E);
	}
}
