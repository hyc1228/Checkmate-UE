// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

class UMainMenuWidget;

/**
 * 主菜单专用 GameMode。BeginPlay 时 spawn 配的 MenuWidgetClass，AddToViewport。
 * 通常配合 MainMenuMap（空 level + 这个 GameMode）作为项目启动地图。
 */
UCLASS()
class CHECKMATE_API AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMainMenuGameMode();

	UPROPERTY(EditDefaultsOnly, Category="MainMenu")
	TSubclassOf<UMainMenuWidget> MenuWidgetClass;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	UMainMenuWidget* MenuInstance = nullptr;
};
