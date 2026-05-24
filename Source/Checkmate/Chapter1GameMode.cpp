// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "Chapter1GameMode.h"

#include "CardData.h"
#include "CardSelectionScreen.h"
#include "DollData.h"
#include "InspectionScreen.h"

#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AChapter1GameMode::AChapter1GameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AChapter1GameMode::BeginPlay()
{
	Super::BeginPlay();

	if (Shifts.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Chapter1GameMode: Shifts is empty — BP_Chapter1GameMode 需要填至少 1 条 ShiftConfig"));
		return;
	}

	if (!CardSelectionWidgetClass || !InspectionWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Chapter1GameMode: CardSelectionWidgetClass / InspectionWidgetClass 未填"));
		return;
	}

	CurrentShiftIdx = 0;
	BeginShift(CurrentShiftIdx);
}

void AChapter1GameMode::BeginShift(int32 ShiftIdx)
{
	if (!Shifts.IsValidIndex(ShiftIdx))
	{
		FinishCh1();
		return;
	}

	const FShiftConfig& Cfg = Shifts[ShiftIdx];

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	// 清掉过场（如有）
	if (ActiveTransitionScreen)
	{
		ActiveTransitionScreen->RemoveFromParent();
		ActiveTransitionScreen = nullptr;
	}

	ActiveCardScreen = CreateWidget<UCardSelectionScreen>(PC, CardSelectionWidgetClass);
	if (!ActiveCardScreen)
	{
		UE_LOG(LogTemp, Error, TEXT("Chapter1GameMode: CreateWidget CardSelectionScreen 失败"));
		return;
	}

	ActiveCardScreen->SetShiftConfig(Cfg.PoolCards, Cfg.K, Cfg.AssemblyTimerSec);
	ActiveCardScreen->OnAssemblyComplete.AddDynamic(this, &AChapter1GameMode::HandleCardsAssembled);
	ActiveCardScreen->AddToViewport();

	SetUIInputMode();

	UE_LOG(LogTemp, Display, TEXT("Chapter1: Shift %d 开始 — Pool=%d, K=%d, DollTimeout=%.1fs"),
		ShiftIdx + 1, Cfg.PoolCards.Num(), Cfg.K, Cfg.DollTimeoutSec);
}

void AChapter1GameMode::HandleCardsAssembled(const TArray<UCardData*>& SelectedCards)
{
	const FShiftConfig& Cfg = Shifts[CurrentShiftIdx];

	UE_LOG(LogTemp, Display, TEXT("Chapter1: Shift %d 卡选完成（%d 张）→ 进检验"),
		CurrentShiftIdx + 1, SelectedCards.Num());

	if (ActiveCardScreen)
	{
		ActiveCardScreen->OnAssemblyComplete.RemoveAll(this);
		ActiveCardScreen->RemoveFromParent();
		ActiveCardScreen = nullptr;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	ActiveInspectionScreen = CreateWidget<UInspectionScreen>(PC, InspectionWidgetClass);
	if (!ActiveInspectionScreen) return;

	ActiveInspectionScreen->SetShiftData(SelectedCards, Cfg.DollSequence);
	ActiveInspectionScreen->DollTimeoutSec = Cfg.DollTimeoutSec;
	ActiveInspectionScreen->CorrectGoal = Cfg.CorrectGoal;
	ActiveInspectionScreen->OnShiftCompleted.AddDynamic(this, &AChapter1GameMode::HandleShiftCompleted);
	ActiveInspectionScreen->AddToViewport();

	SetUIInputMode();
}

void AChapter1GameMode::HandleShiftCompleted(FShiftResult Result)
{
	UE_LOG(LogTemp, Display, TEXT("Chapter1: Shift %d 完成 — %d / %d 正确，%d 误判"),
		CurrentShiftIdx + 1, Result.CorrectCount, Result.TotalDolls, Result.WrongCount);

	if (ActiveInspectionScreen)
	{
		ActiveInspectionScreen->OnShiftCompleted.RemoveAll(this);
		ActiveInspectionScreen->RemoveFromParent();
		ActiveInspectionScreen = nullptr;
	}

	const int32 NextIdx = CurrentShiftIdx + 1;
	if (Shifts.IsValidIndex(NextIdx))
	{
		ShowShiftTransition(NextIdx);
	}
	else
	{
		FinishCh1();
	}
}

void AChapter1GameMode::ShowShiftTransition(int32 NextShiftIdx)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	if (ShiftTransitionWidgetClass)
	{
		ActiveTransitionScreen = CreateWidget<UUserWidget>(PC, ShiftTransitionWidgetClass);
		if (ActiveTransitionScreen)
		{
			// 找命名 TitleText 设文字（可选 BindWidget，没找到就算了）
			if (UTextBlock* Title = Cast<UTextBlock>(ActiveTransitionScreen->GetWidgetFromName(TEXT("TitleText"))))
			{
				Title->SetText(FText::FromString(FString::Printf(TEXT("班次 %d"), NextShiftIdx + 1)));
			}
			ActiveTransitionScreen->AddToViewport(/*ZOrder=*/100);
			SetUIInputMode();
		}
	}

	// TransitionHoldSeconds 后切下一班
	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(
		TimerHandle,
		FTimerDelegate::CreateLambda([this, NextShiftIdx]()
		{
			CurrentShiftIdx = NextShiftIdx;
			BeginShift(CurrentShiftIdx);
		}),
		FMath::Max(0.5f, TransitionHoldSeconds), false);
}

void AChapter1GameMode::FinishCh1()
{
	UE_LOG(LogTemp, Display, TEXT("Chapter1: 全部 %d 班完成 — Ch1 结束"), Shifts.Num());
	// TODO Tier B/C：触发 twist 演出 / 切 Ch2
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
