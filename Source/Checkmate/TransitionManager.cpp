// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "TransitionManager.h"
#include "JudgmentCardActor.h"
#include "JudgmentCardSubsystem.h"
#include "CheckmateGameStateSubsystem.h"
#include "InspectionStationActor.h"
#include "CardData.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "TimerManager.h"

void UTransitionManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CardActorClass = AJudgmentCardActor::StaticClass();
}

void UTransitionManager::BeginTransition(const TArray<UCardData*>& AssembledCards)
{
	PendingCards = AssembledCards;

	// 1. 喂标准给 JudgmentSubsystem
	if (UJudgmentCardSubsystem* JC = GetWorld()->GetSubsystem<UJudgmentCardSubsystem>())
	{
		JC->SetCurrentStandard(AssembledCards);
	}

	// 2. 推 GameState 进过渡（CameraSystem 自动响应 zoom out）
	if (UGameInstance* GI = GetWorld()->GetGameInstance())
	{
		if (UCheckmateGameStateSubsystem* GS = GI->GetSubsystem<UCheckmateGameStateSubsystem>())
		{
			GS->RequestStateChange(ECheckmateGameState::Ch1_Transition2Dto3D);
		}
	}

	// 3. 在 tray anchor 摆 K 个 3D 纸卡
	SpawnPaperCards(AssembledCards);

	// 4. TransitionSeconds 后切到 Ch1_Shift
	GetWorld()->GetTimerManager().SetTimer(TransitionTimer, this,
		&UTransitionManager::OnTransitionComplete, TransitionSeconds, false);

	UE_LOG(LogTemp, Log, TEXT("TransitionManager: 2D→3D 过渡启动（%d 张卡，%.2fs）"),
		AssembledCards.Num(), TransitionSeconds);
}

void UTransitionManager::SpawnPaperCards(const TArray<UCardData*>& Cards)
{
	for (AJudgmentCardActor* Old : SpawnedPaperCards)
	{
		if (Old) { Old->Destroy(); }
	}
	SpawnedPaperCards.Reset();

	AInspectionStationActor* Station = FindInspectionStation();
	if (!Station || !Station->StandardTrayAnchor)
	{
		UE_LOG(LogTemp, Warning, TEXT("TransitionManager: 找不到 InspectionStation.StandardTrayAnchor，跳过 3D 纸卡 spawn。"));
		return;
	}

	const FVector TrayOrigin = Station->StandardTrayAnchor->GetComponentLocation();
	const FVector TrayRight = Station->StandardTrayAnchor->GetRightVector();
	const int32 NumCards = Cards.Num();
	const float HalfWidth = (NumCards - 1) * CardSpacing * 0.5f;

	for (int32 i = 0; i < NumCards; ++i)
	{
		const FVector Pos = TrayOrigin + TrayRight * (i * CardSpacing - HalfWidth);
		const FRotator Rot = Station->StandardTrayAnchor->GetComponentRotation();

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AJudgmentCardActor* Paper = GetWorld()->SpawnActor<AJudgmentCardActor>(
			CardActorClass ? CardActorClass.Get() : AJudgmentCardActor::StaticClass(),
			Pos, Rot, Params);

		if (Paper)
		{
			Paper->InitializeFromData(Cards[i]);
			SpawnedPaperCards.Add(Paper);
		}
	}

	UE_LOG(LogTemp, Verbose, TEXT("TransitionManager: spawn %d 张 3D paper card on tray."), NumCards);
}

void UTransitionManager::OnTransitionComplete()
{
	if (UGameInstance* GI = GetWorld()->GetGameInstance())
	{
		if (UCheckmateGameStateSubsystem* GS = GI->GetSubsystem<UCheckmateGameStateSubsystem>())
		{
			GS->RequestStateChange(ECheckmateGameState::Ch1_Shift);
		}
	}
}

AInspectionStationActor* UTransitionManager::FindInspectionStation() const
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AInspectionStationActor> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}
