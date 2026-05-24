// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "Chapter1GameMode.h"

#include "CardData.h"
#include "CardSelectionScreen.h"
#include "Ch1LocSubsystem.h"
#include "DollData.h"
#include "DollDisplay.h"
#include "InspectionScreen.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AChapter1GameMode::AChapter1GameMode()
{
	PrimaryActorTick.bCanEverTick = false;

	// 默认 ADefaultPawn 会绑 mouse-look 让相机跟鼠标转——本游戏是固定视角下的 3D 检验台，
	// 用 bare APawn 替代（无 movement / 无 look 输入）。视角 = 该 pawn 在 PlayerStart 的 transform。
	DefaultPawnClass = APawn::StaticClass();
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

	// 把本地化字典注入到 subsystem，下游 Screen 都从 subsystem 取 FText
	if (LocStringsAsset)
	{
		if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
		{
			if (UCh1LocSubsystem* Loc = GI->GetSubsystem<UCh1LocSubsystem>())
			{
				Loc->SetStrings(LocStringsAsset);
			}
		}
	}

	// 强制相机：找关卡里的 InspectionCamera (Tag 或者第一个 ACameraActor) → 设为 PC 视口主相机。
	// 否则会用 Pawn 位置（bare APawn 无相机组件，会落回原点视角）。
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		TArray<AActor*> Cameras;
		UGameplayStatics::GetAllActorsWithTag(this, FName(TEXT("InspectionCamera")), Cameras);
		if (Cameras.Num() == 0)
		{
			// 回退：拿第一个 ACameraActor
			UGameplayStatics::GetAllActorsOfClass(this, ACameraActor::StaticClass(), Cameras);
		}
		if (Cameras.Num() > 0)
		{
			PC->SetViewTargetWithBlend(Cameras[0], 0.0f);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Chapter1GameMode: 关卡里没有 ACameraActor，相机用 pawn 默认视角"));
		}
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

	// Spawn 3D 娃娃 actor（一班一只，复用到下一班）
	if (DollActorClass && !ActiveDollActor)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ActiveDollActor = GetWorld()->SpawnActor<ADollDisplay>(
			DollActorClass, DollSpawnTransform, Params);
	}
	if (ActiveDollActor)
	{
		ActiveInspectionScreen->SetDollActor(ActiveDollActor);
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Chapter1GameMode: DollActorClass 未填或 spawn 失败——检验屏会缺 3D 交互"));
	}

	// 3D 拖拽需要同时收键鼠和 UI 事件
	APlayerController* PC2 = UGameplayStatics::GetPlayerController(this, 0);
	if (PC2)
	{
		PC2->bShowMouseCursor = true;
		PC2->bEnableClickEvents = true;
		PC2->bEnableMouseOverEvents = true;
		FInputModeGameAndUI Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC2->SetInputMode(Mode);
	}
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
			if (UTextBlock* Title = Cast<UTextBlock>(ActiveTransitionScreen->GetWidgetFromName(TEXT("TitleText"))))
			{
				FText TitleText;
				if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
				{
					if (UCh1LocSubsystem* Loc = GI->GetSubsystem<UCh1LocSubsystem>())
					{
						TitleText = FText::Format(Loc->Get(TEXT("Shift.Title")), FText::AsNumber(NextShiftIdx + 1));
					}
				}
				if (TitleText.IsEmpty())
				{
					TitleText = FText::FromString(FString::Printf(TEXT("班次 %d"), NextShiftIdx + 1));
				}
				Title->SetText(TitleText);
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
