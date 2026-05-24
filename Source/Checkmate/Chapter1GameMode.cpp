// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "Chapter1GameMode.h"

#include "CardData.h"
#include "CardSelectionScreen.h"
#include "DollData.h"
#include "InspectionScreen.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AChapter1GameMode::AChapter1GameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AChapter1GameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!CardSelectionWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Chapter1GameMode: CardSelectionWidgetClass not set in BP defaults"));
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("Chapter1GameMode: no PlayerController 0"));
		return;
	}

	ActiveCardScreen = CreateWidget<UCardSelectionScreen>(PC, CardSelectionWidgetClass);
	if (!ActiveCardScreen)
	{
		UE_LOG(LogTemp, Error, TEXT("Chapter1GameMode: failed to create CardSelectionScreen"));
		return;
	}

	ActiveCardScreen->SetShiftConfig(ShiftPoolCards, K, AssemblyTimerSec);
	ActiveCardScreen->OnAssemblyComplete.AddDynamic(this, &AChapter1GameMode::HandleCardsAssembled);
	ActiveCardScreen->AddToViewport();

	SetUIInputMode();
}

void AChapter1GameMode::HandleCardsAssembled(const TArray<UCardData*>& SelectedCards)
{
	UE_LOG(LogTemp, Display, TEXT("Chapter1: cards assembled (%d), entering inspection loop"), SelectedCards.Num());

	if (ActiveCardScreen)
	{
		ActiveCardScreen->OnAssemblyComplete.RemoveAll(this);
		ActiveCardScreen->RemoveFromParent();
		ActiveCardScreen = nullptr;
	}

	if (!InspectionWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Chapter1GameMode: InspectionWidgetClass not set"));
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	ActiveInspectionScreen = CreateWidget<UInspectionScreen>(PC, InspectionWidgetClass);
	if (!ActiveInspectionScreen)
	{
		UE_LOG(LogTemp, Error, TEXT("Chapter1GameMode: failed to create InspectionScreen"));
		return;
	}

	ActiveInspectionScreen->SetShiftData(SelectedCards, ShiftDollSequence);
	ActiveInspectionScreen->OnShiftCompleted.AddDynamic(this, &AChapter1GameMode::HandleShiftCompleted);
	ActiveInspectionScreen->AddToViewport();

	SetUIInputMode();
}

void AChapter1GameMode::HandleShiftCompleted(FShiftResult Result)
{
	UE_LOG(LogTemp, Display, TEXT("Chapter1: shift completed — %d / %d correct, %d wrong"),
		Result.CorrectCount, Result.TotalDolls, Result.WrongCount);

	if (ActiveInspectionScreen)
	{
		ActiveInspectionScreen->OnShiftCompleted.RemoveAll(this);
		// 灯盒：保留检验屏幕在画面里（让玩家看到 toast 残留 + 最终统计）
		// 后续 spec 接入「班次结束」过场时在此 RemoveFromParent
	}
}

void AChapter1GameMode::SetUIInputMode()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	PC->bShowMouseCursor = true;
	PC->bEnableClickEvents = true;
	PC->bEnableMouseOverEvents = true;

	FInputModeUIOnly Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(Mode);
}
