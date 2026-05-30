// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "TitleScreenGameMode.h"

#include "TitlePromptWidget.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ATitleScreenGameMode::ATitleScreenGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
	DefaultPawnClass = nullptr;
	PlayerControllerClass = APlayerController::StaticClass();
	PromptWidgetClass = UTitlePromptWidget::StaticClass();

	DropNameHints = {
		TEXT("Button"),
		TEXT("button"),
		TEXT("Btn"),
		TEXT("btn"),
		TEXT("Anniu"),
		TEXT("Kou"),
		TEXT("Pearl"),
		TEXT("pearl")
	};

	TitleShardNameHints = {
		TEXT("Text_cell")
	};

	StaticNameHints = {
		TEXT("Floor"),
		TEXT("Ground"),
		TEXT("Wall"),
		TEXT("Camera"),
		TEXT("Light"),
		TEXT("Sky"),
		TEXT("PostProcess")
	};
}

void ATitleScreenGameMode::BeginPlay()
{
	Super::BeginPlay();

	CollectDropComponents();
	FreezeDropComponents();
	FreezeTitleShardComponents();

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	ActivateTitleCamera(PC);

	PC->bShowMouseCursor = true;
	PC->bEnableClickEvents = true;

	if (PromptWidgetClass)
	{
		PromptWidget = CreateWidget<UTitlePromptWidget>(PC, PromptWidgetClass);
		if (PromptWidget)
		{
			PromptWidget->AddToViewport(100);

			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(PromptWidget->TakeWidget());
			PC->SetInputMode(InputMode);
		}
	}
}

void ATitleScreenGameMode::StartTitleFall()
{
	if (bStarted)
	{
		return;
	}

	bStarted = true;
	if (PromptWidget)
	{
		PromptWidget->SetStarted(true);
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		PC->bShowMouseCursor = false;
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}

	if (UWorld* World = GetWorld())
	{
		if (AWorldSettings* WorldSettings = World->GetWorldSettings())
		{
			WorldSettings->bGlobalGravitySet = true;
			WorldSettings->GlobalGravityZ = FallingGravityZ;
		}
	}

	for (UPrimitiveComponent* Component : DropComponents)
	{
		if (!Component)
		{
			continue;
		}

		Component->SetMobility(EComponentMobility::Movable);
		Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Component->SetEnableGravity(true);
		Component->SetSimulatePhysics(true);
		Component->WakeAllRigidBodies();
	}

	if (UWorld* World = GetWorld())
	{
		if (TitleShardReleaseDelaySeconds <= 0.0f)
		{
			ReleaseTitleShards();
		}
		else
		{
			World->GetTimerManager().SetTimer(
				ReleaseTitleShardsTimerHandle,
				this,
				&ATitleScreenGameMode::ReleaseTitleShards,
				TitleShardReleaseDelaySeconds,
				false);
		}

		const float FadeStartDelaySeconds = FMath::Max(0.0f, OpenFirstMapDelaySeconds - TransitionFadeDurationSeconds);
		if (TransitionFadeDurationSeconds <= 0.0f || FadeStartDelaySeconds <= 0.0f)
		{
			StartFadeToBlack();
		}
		else
		{
			World->GetTimerManager().SetTimer(
				FadeToBlackTimerHandle,
				this,
				&ATitleScreenGameMode::StartFadeToBlack,
				FadeStartDelaySeconds,
				false);
		}

		World->GetTimerManager().SetTimer(
			OpenFirstMapTimerHandle,
			this,
			&ATitleScreenGameMode::OpenFirstMap,
			OpenFirstMapDelaySeconds,
			false);
	}
}

void ATitleScreenGameMode::CollectDropComponents()
{
	DropComponents.Reset();
	TitleShardComponents.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || Actor == this)
		{
			continue;
		}

		TArray<UPrimitiveComponent*> Components;
		Actor->GetComponents<UPrimitiveComponent>(Components);
		for (UPrimitiveComponent* Component : Components)
		{
			if (ShouldTitleShardComponent(Component))
			{
				TitleShardComponents.Add(Component);
			}
			else if (ShouldDropComponent(Component))
			{
				DropComponents.Add(Component);
			}
		}
	}
}

bool ATitleScreenGameMode::ShouldDropComponent(const UPrimitiveComponent* Component) const
{
	if (!Component)
	{
		return false;
	}

	const AActor* OwningActor = Component->GetOwner();
	const FString ActorName = OwningActor ? OwningActor->GetName() : FString();
	const FString ComponentName = Component->GetName();
	FString MeshName;
	if (const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Component))
	{
		if (const UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh())
		{
			MeshName = StaticMesh->GetName();
		}
	}

	const FString Combined = ActorName + TEXT(" ") + ComponentName + TEXT(" ") + MeshName;
	if (TextContainsAnyHint(Combined, StaticNameHints))
	{
		return false;
	}

	if (TextContainsAnyHint(Combined, DropNameHints))
	{
		return true;
	}

	return Component->IsSimulatingPhysics();
}

bool ATitleScreenGameMode::ShouldTitleShardComponent(const UPrimitiveComponent* Component) const
{
	if (!Component)
	{
		return false;
	}

	const AActor* OwningActor = Component->GetOwner();
	const FString ActorName = OwningActor ? OwningActor->GetName() : FString();
	const FString ComponentName = Component->GetName();
	FString MeshName;
	if (const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Component))
	{
		if (const UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh())
		{
			MeshName = StaticMesh->GetName();
		}
	}

	const FString Combined = ActorName + TEXT(" ") + ComponentName + TEXT(" ") + MeshName;
	return TextContainsAnyHint(Combined, TitleShardNameHints);
}

bool ATitleScreenGameMode::TextContainsAnyHint(const FString& Text, const TArray<FString>& Hints) const
{
	for (const FString& Hint : Hints)
	{
		if (!Hint.IsEmpty() && Text.Contains(Hint, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}
	return false;
}

void ATitleScreenGameMode::FreezeDropComponents()
{
	for (UPrimitiveComponent* Component : DropComponents)
	{
		if (!Component)
		{
			continue;
		}

		Component->SetSimulatePhysics(false);
		Component->SetEnableGravity(false);
		Component->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Component->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		Component->PutAllRigidBodiesToSleep();
	}
}

void ATitleScreenGameMode::FreezeTitleShardComponents()
{
	for (UPrimitiveComponent* Component : TitleShardComponents)
	{
		if (!Component)
		{
			continue;
		}

		Component->SetSimulatePhysics(false);
		Component->SetEnableGravity(false);
		Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Component->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Component->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		Component->PutAllRigidBodiesToSleep();
	}
}

void ATitleScreenGameMode::ReleaseTitleShards()
{
	for (UPrimitiveComponent* Component : TitleShardComponents)
	{
		if (!Component)
		{
			continue;
		}

		Component->SetMobility(EComponentMobility::Movable);
		Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Component->SetEnableGravity(false);
		Component->SetSimulatePhysics(true);
		Component->WakeAllRigidBodies();
	}
}

void ATitleScreenGameMode::StartFadeToBlack()
{
	if (PromptWidget)
	{
		PromptWidget->StartFadeToBlack(TransitionFadeDurationSeconds);
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(
				0.0f,
				1.0f,
				TransitionFadeDurationSeconds,
				FLinearColor::Black,
				false,
				true);
		}
	}
}

void ATitleScreenGameMode::ActivateTitleCamera(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}

	if (ACameraActor* CameraActor = FindOrSpawnTitleCamera())
	{
		CameraActor->SetActorLocationAndRotation(TitleCameraLocation, TitleCameraRotation);
		if (UCameraComponent* CameraComponent = CameraActor->GetCameraComponent())
		{
			CameraComponent->SetFieldOfView(TitleCameraFOV);
		}
		PC->SetViewTarget(CameraActor);
	}
}

ACameraActor* ATitleScreenGameMode::FindOrSpawnTitleCamera() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ACameraActor> It(World); It; ++It)
	{
		ACameraActor* CameraActor = *It;
		if (CameraActor && CameraActor->GetName().Contains(TEXT("Title"), ESearchCase::IgnoreCase))
		{
			return CameraActor;
		}
	}

	for (TActorIterator<ACameraActor> It(World); It; ++It)
	{
		if (*It)
		{
			return *It;
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = TEXT("RuntimeTitleCamera");
	ACameraActor* CameraActor = World->SpawnActor<ACameraActor>(TitleCameraLocation, TitleCameraRotation, SpawnParams);
	if (CameraActor)
	{
#if WITH_EDITOR
		CameraActor->SetActorLabel(TEXT("RuntimeTitleCamera"));
#endif
		if (UCameraComponent* CameraComponent = CameraActor->GetCameraComponent())
		{
			CameraComponent->SetFieldOfView(TitleCameraFOV);
		}
	}
	return CameraActor;
}

void ATitleScreenGameMode::OpenFirstMap()
{
	UGameplayStatics::OpenLevel(this, FirstGameMapName);
}
