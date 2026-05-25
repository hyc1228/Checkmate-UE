// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "MainMenuGameMode.h"

#include "MainMenuWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

AMainMenuGameMode::AMainMenuGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!MenuWidgetClass) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	MenuInstance = CreateWidget<UMainMenuWidget>(PC, MenuWidgetClass);
	if (MenuInstance)
	{
		MenuInstance->AddToViewport(100);
	}
}
