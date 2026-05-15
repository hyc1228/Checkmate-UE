// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "TwistSequenceManager.h"
#include "JudgmentCardSubsystem.h"
#include "CheckmateGameStateSubsystem.h"
#include "CheckmateAudioSubsystem.h"
#include "SubliminalLayerSubsystem.h"
#include "EyeStateComponent.h"
#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

void UTwistSequenceManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UWorld* World = GetWorld())
	{
		CachedJC = World->GetSubsystem<UJudgmentCardSubsystem>();
		if (CachedJC)
		{
			CachedJC->OnTwistConditionMet.AddDynamic(this, &UTwistSequenceManager::OnTwistConditionMet);
		}
	}
}

void UTwistSequenceManager::Deinitialize()
{
	if (CachedJC)
	{
		CachedJC->OnTwistConditionMet.RemoveDynamic(this, &UTwistSequenceManager::OnTwistConditionMet);
	}
	Super::Deinitialize();
}

void UTwistSequenceManager::OnTwistConditionMet()
{
	StartTwistSequence();
}

void UTwistSequenceManager::StartTwistSequence()
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	UCheckmateGameStateSubsystem* GS = nullptr;
	if (UGameInstance* GI = World->GetGameInstance())
	{
		GS = GI->GetSubsystem<UCheckmateGameStateSubsystem>();
	}
	if (GS)
	{
		GS->RequestStateChange(ECheckmateGameState::Ch1_TwistTrigger);
		GS->RequestStateChange(ECheckmateGameState::Twist_Sequence);
	}

	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		PC->DisableInput(PC);
	}

	ULevelSequence* SeqAsset = TwistSequenceAsset.LoadSynchronous();
	if (!SeqAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("TwistSequenceManager: TwistSequenceAsset 为空。Whitebox 模式：模拟 30s 后直接进 Ch2 stub。"));
		FTimerHandle T1, T2, T3, T4, T5;
		World->GetTimerManager().SetTimer(T1, FTimerDelegate::CreateUObject(this, &UTwistSequenceManager::HandleSequenceEvent, TwistEvents::SubliminalSurge), 0.5f, false);
		World->GetTimerManager().SetTimer(T2, FTimerDelegate::CreateUObject(this, &UTwistSequenceManager::HandleSequenceEvent, TwistEvents::EyeFall),         4.0f, false);
		World->GetTimerManager().SetTimer(T3, FTimerDelegate::CreateUObject(this, &UTwistSequenceManager::HandleSequenceEvent, TwistEvents::IrisBloom),       5.0f, false);
		World->GetTimerManager().SetTimer(T4, FTimerDelegate::CreateUObject(this, &UTwistSequenceManager::HandleSequenceEvent, TwistEvents::UIBreakdown),     6.0f, false);
		World->GetTimerManager().SetTimer(T5, FTimerDelegate::CreateUObject(this, &UTwistSequenceManager::HandleSequenceEvent, TwistEvents::Complete),       30.0f, false);
		return;
	}

	FMovieSceneSequencePlaybackSettings Settings;
	Settings.bDisableMovementInput = true;
	ActivePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(World, SeqAsset, Settings, ActorSequenceActor);
	if (ActivePlayer)
	{
		ActivePlayer->Play();
		UE_LOG(LogTemp, Log, TEXT("TwistSequenceManager: 启动 LS_Twist_Sequence 演出。"));
	}
}

void UTwistSequenceManager::HandleSequenceEvent(FName EventName)
{
	UWorld* World = GetWorld();
	if (!World) { return; }

	UE_LOG(LogTemp, Log, TEXT("Twist Event: %s"), *EventName.ToString());

	if (EventName == TwistEvents::SubliminalSurge)
	{
		if (USubliminalLayerSubsystem* SL = World->GetSubsystem<USubliminalLayerSubsystem>())
		{
			SL->ForceOpacityTo(1.0f);
		}
		if (UCheckmateAudioSubsystem* AS = World->GetSubsystem<UCheckmateAudioSubsystem>())
		{
			AS->PlayCue(TEXT("SFX_SubliminalSurge"));
		}
		return;
	}

	if (EventName == TwistEvents::EyeFall)
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				if (UEyeStateComponent* Eye = Pawn->FindComponentByClass<UEyeStateComponent>())
				{
					Eye->RequestStateTransition(EEyeState::Transitioning, false);
				}
			}
		}
		if (UCheckmateAudioSubsystem* AS = World->GetSubsystem<UCheckmateAudioSubsystem>())
		{
			AS->PlayCue(TEXT("SFX_EyeFall"));
		}
		return;
	}

	if (EventName == TwistEvents::IrisBloom)
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				if (UEyeStateComponent* Eye = Pawn->FindComponentByClass<UEyeStateComponent>())
				{
					Eye->RequestStateTransition(EEyeState::MechanicalEyed, false);
				}
			}
		}
		if (UCheckmateAudioSubsystem* AS = World->GetSubsystem<UCheckmateAudioSubsystem>())
		{
			AS->PlayCue(TEXT("SFX_IrisBloom"));
		}
		return;
	}

	if (EventName == TwistEvents::UIBreakdown)
	{
		if (UCheckmateAudioSubsystem* AS = World->GetSubsystem<UCheckmateAudioSubsystem>())
		{
			AS->PlayCue(TEXT("SFX_UIBreakdown"));
		}
		return;
	}

	if (EventName == TwistEvents::Complete)
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UCheckmateGameStateSubsystem* GS = GI->GetSubsystem<UCheckmateGameStateSubsystem>())
			{
				GS->RequestStateChange(ECheckmateGameState::Ch2_Awakening_Stub);
			}
		}
		if (UCheckmateAudioSubsystem* AS = World->GetSubsystem<UCheckmateAudioSubsystem>())
		{
			AS->PlayCue(TEXT("SFX_TwistComplete"));
		}
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("TwistSequenceManager: 未知 event '%s'。"), *EventName.ToString());
}

void UTwistSequenceManager::TS_Trigger()
{
	StartTwistSequence();
}
