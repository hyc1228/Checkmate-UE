// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "PVShotDirector.h"

#include "AudioService.h"
#include "Ch2Pawn.h"

#include "Camera/CameraActor.h"
#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieSceneSequencePlaybackSettings.h"
#include "TimerManager.h"

namespace
{
	const FName State_ButtonWorld(TEXT("ButtonWorld"));
	const FName State_MechanicalWorld(TEXT("MechanicalWorld"));
	const FName State_ColorBloom(TEXT("ColorBloom"));
}

APVShotDirector::APVShotDirector()
{
	PrimaryActorTick.bCanEverTick = true;
}

bool APVShotDirector::PlayPVSequence(FName ShotId)
{
	TSoftObjectPtr<ULevelSequence>* SoftPtr = PVSequences.Find(ShotId);
	if (!SoftPtr)
	{
		UE_LOG(LogTemp, Warning, TEXT("PVShotDirector: no sequence registered for %s"), *ShotId.ToString());
		return false;
	}

	ULevelSequence* Sequence = SoftPtr->LoadSynchronous();
	if (!Sequence)
	{
		UE_LOG(LogTemp, Warning, TEXT("PVShotDirector: sequence asset for %s failed to load"), *ShotId.ToString());
		return false;
	}

	FMovieSceneSequencePlaybackSettings Settings;
	Settings.bAutoPlay = false;
	ALevelSequenceActor* SequenceActor = nullptr;
	ULevelSequencePlayer* Player = ULevelSequencePlayer::CreateLevelSequencePlayer(
		GetWorld(), Sequence, Settings, SequenceActor);
	if (!Player)
	{
		UE_LOG(LogTemp, Warning, TEXT("PVShotDirector: could not create sequence player for %s"), *ShotId.ToString());
		return false;
	}

	Player->Play();
	UE_LOG(LogTemp, Display, TEXT("PVShotDirector: playing shot sequence %s"), *ShotId.ToString());
	return true;
}

void APVShotDirector::PlayShot(FName ShotId)
{
	if (PlayPVSequence(ShotId))
	{
		return;
	}

	const FString Id = ShotId.ToString().ToUpper();
	if (Id == TEXT("B2"))
	{
		TriggerButtonFall();
		UAudioService::PlayCueStatic(this, FName("Twist.PearlEye"));
	}
	else if (Id == TEXT("B3"))
	{
		ForceEyeState(/*bMechanical=*/true);
		ApplyPVSceneState(State_MechanicalWorld);
		UAudioService::PlayCueStatic(this, FName("Twist.MechanicalEye"));
	}
	else if (Id == TEXT("C11"))
	{
		TriggerPuppetExplosionAt(FIntPoint(5, 4));
	}
	else if (Id == TEXT("C12"))
	{
		TriggerSensoriumBloom(SensoriumDefaultDuration);
	}
	else if (Id == TEXT("C7"))
	{
		ForceEyeState(/*bMechanical=*/true);
		UAudioService::PlayCueStatic(this, FName("Ch2.Ritual"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PVShotDirector: no fallback behavior for shot %s"), *ShotId.ToString());
	}
}

void APVShotDirector::ApplyPVSceneState(FName StateId)
{
	if (StateId == State_ButtonWorld)
	{
		SetSceneTagVisibility(ButtonWorldTag);
		ForceEyeState(/*bMechanical=*/false);
		ApplyColorToActorsWithTag(ButtonWorldTag, ButtonWorldColor, 0.5f);
	}
	else if (StateId == State_MechanicalWorld)
	{
		SetSceneTagVisibility(MechanicalWorldTag);
		ForceEyeState(/*bMechanical=*/true);
		ApplyColorToActorsWithTag(MechanicalWorldTag, MechanicalWorldColor, 3.0f);
	}
	else if (StateId == State_ColorBloom)
	{
		SetSceneTagVisibility(ColorWorldTag);
		TriggerSensoriumBloom(SensoriumDefaultDuration);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PVShotDirector: unknown scene state %s"), *StateId.ToString());
	}
}

void APVShotDirector::ForceEyeState(bool bMechanical, bool bAlsoSetCh2Mode)
{
	ACh2Pawn* Pawn = FindCh2Pawn();
	if (!Pawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("PVShotDirector: no Ch2 pawn found for ForceEyeState"));
		return;
	}

	if (bAlsoSetCh2Mode)
	{
		Pawn->SetMode(bMechanical ? ECh2Mode::Clown : ECh2Mode::Ballet);
	}
	Pawn->SetEyeStyle(bMechanical ? EButtonEyeStyle::Standard : EButtonEyeStyle::Pearl);

	if (Pawn->EyeMarker)
	{
		if (UMaterialInterface* Override = bMechanical ? MechanicalEyeMaterial : ButtonEyeMaterial)
		{
			Pawn->EyeMarker->SetMaterial(0, Override);
		}
		Pawn->EyeMarker->SetVisibility(true);
		const FVector Color = bMechanical
			? FVector(MechanicalWorldColor.R, MechanicalWorldColor.G, MechanicalWorldColor.B)
			: FVector(ButtonWorldColor.R, ButtonWorldColor.G, ButtonWorldColor.B);
		Pawn->EyeMarker->SetVectorParameterValueOnMaterials(ColorParameterName, Color);
		Pawn->EyeMarker->SetVectorParameterValueOnMaterials(EmissiveColorParameterName, Color);
		Pawn->EyeMarker->SetScalarParameterValueOnMaterials(IntensityParameterName, bMechanical ? 8.0f : 1.0f);
	}
}

void APVShotDirector::TriggerButtonFall()
{
	ForceEyeState(/*bMechanical=*/false, /*bAlsoSetCh2Mode=*/false);
	SetActorsWithTagHidden(ButtonWorldTag, true);
	SetActorsWithTagHidden(MechanicalWorldTag, false);
	UAudioService::PlayCueStatic(this, FName("Twist.PearlEye"));
	UE_LOG(LogTemp, Display, TEXT("PVShotDirector: TriggerButtonFall"));
}

void APVShotDirector::TriggerSensoriumBloom(float Duration)
{
	SensoriumBloomDuration = Duration > 0.0f ? Duration : SensoriumDefaultDuration;
	SensoriumBloomElapsed = 0.0f;
	bSensoriumBloomActive = true;
	SetActorsWithTagHidden(ColorWorldTag, false);
	UAudioService::PlayCueStatic(this, FName("Ch2.Victory"), 0.7f);
}

void APVShotDirector::SpawnAndExplodePuppetForPV(FIntPoint Cell, float Delay)
{
	UWorld* World = GetWorld();
	if (!World) return;

	UStaticMesh* Mesh = PuppetMesh ? PuppetMesh : LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	AStaticMeshActor* Puppet = World->SpawnActor<AStaticMeshActor>(CellToWorld(Cell), FRotator::ZeroRotator);
	if (Puppet)
	{
		Puppet->SetMobility(EComponentMobility::Movable);
		if (UStaticMeshComponent* MeshComp = Puppet->GetStaticMeshComponent())
		{
			MeshComp->SetStaticMesh(Mesh);
			MeshComp->SetRelativeScale3D(FVector(0.65f));
			MeshComp->SetVectorParameterValueOnMaterials(ColorParameterName, FVector(1.0f, 0.45f, 0.08f));
		}
		Puppet->SetLifeSpan(FMath::Max(Delay + 0.1f, 0.2f));
	}

	FTimerHandle TimerHandle;
	World->GetTimerManager().SetTimer(
		TimerHandle,
		FTimerDelegate::CreateUObject(this, &APVShotDirector::TriggerPuppetExplosionAt, Cell),
		FMath::Max(0.0f, Delay),
		false);
}

void APVShotDirector::TriggerPuppetExplosionAt(FIntPoint Cell)
{
	UWorld* World = GetWorld();
	if (!World) return;

	UStaticMesh* Mesh = ExplosionFlashMesh ? ExplosionFlashMesh : LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	AStaticMeshActor* Flash = World->SpawnActor<AStaticMeshActor>(CellToWorld(Cell), FRotator::ZeroRotator);
	if (Flash)
	{
		Flash->SetMobility(EComponentMobility::Movable);
		if (UStaticMeshComponent* MeshComp = Flash->GetStaticMeshComponent())
		{
			MeshComp->SetStaticMesh(Mesh);
			MeshComp->SetRelativeScale3D(FVector(ExplosionFlashScale));
			MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			MeshComp->SetVectorParameterValueOnMaterials(ColorParameterName, FVector(2.0f, 1.4f, 0.55f));
			MeshComp->SetVectorParameterValueOnMaterials(EmissiveColorParameterName, FVector(2.0f, 1.4f, 0.55f));
			MeshComp->SetScalarParameterValueOnMaterials(IntensityParameterName, 10.0f);
		}
		Flash->SetLifeSpan(ExplosionFlashLifeSeconds);
	}

	// Hide hand-placed breakable PV props near the blast if designers tag them.
	TArray<AActor*> Breakables;
	UGameplayStatics::GetAllActorsWithTag(this, TEXT("PV_Destructible"), Breakables);
	const FVector BlastLoc = CellToWorld(Cell);
	for (AActor* Actor : Breakables)
	{
		if (Actor && FVector::Dist2D(Actor->GetActorLocation(), BlastLoc) <= DefaultCellSize * 1.5f)
		{
			SetActorRenderHidden(Actor, true);
		}
	}

	UAudioService::PlayCueStatic(this, FName("Ch2.Explode"));
	TriggerPVShake();
}

void APVShotDirector::HideGameplayHUD(bool bHidden)
{
	SetActorsWithTagHidden(GameplayHUDTag, bHidden);
}

ACh2Pawn* APVShotDirector::FindCh2Pawn() const
{
	TArray<AActor*> Pawns;
	UGameplayStatics::GetAllActorsOfClass(this, ACh2Pawn::StaticClass(), Pawns);
	return Pawns.Num() > 0 ? Cast<ACh2Pawn>(Pawns[0]) : nullptr;
}

void APVShotDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bSensoriumBloomActive)
	{
		SensoriumBloomElapsed += DeltaSeconds;
		const float T = FMath::Clamp(SensoriumBloomElapsed / FMath::Max(0.01f, SensoriumBloomDuration), 0.0f, 1.0f);
		const float Eased = FMath::InterpEaseInOut(0.0f, 1.0f, T, 2.0f);
		ApplyColorToActorsWithTag(ColorWorldTag, FMath::Lerp(FLinearColor::White, ColorBloomColor, Eased), 1.0f + Eased * 5.0f);
		if (T >= 1.0f)
		{
			bSensoriumBloomActive = false;
		}
	}

	if (bPVShakeActive && RuntimeShakeTarget)
	{
		PVShakeElapsed += DeltaSeconds;
		if (PVShakeElapsed >= PVShakeDuration)
		{
			RuntimeShakeTarget->SetActorLocation(PVShakeBaseLocation);
			RuntimeShakeTarget = nullptr;
			bPVShakeActive = false;
		}
		else
		{
			const float Decay = 1.0f - (PVShakeElapsed / PVShakeDuration);
			const FVector Offset(
				FMath::Sin(PVShakeElapsed * 75.0f) * PVShakeMagnitude * Decay,
				FMath::Cos(PVShakeElapsed * 61.0f) * PVShakeMagnitude * 0.5f * Decay,
				FMath::Sin(PVShakeElapsed * 91.0f) * PVShakeMagnitude * 0.7f * Decay);
			RuntimeShakeTarget->SetActorLocation(PVShakeBaseLocation + Offset);
		}
	}
}

void APVShotDirector::SetActorsWithTagHidden(FName Tag, bool bHidden)
{
	if (Tag == NAME_None) return;
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsWithTag(this, Tag, Actors);
	for (AActor* Actor : Actors)
	{
		SetActorRenderHidden(Actor, bHidden);
	}
}

void APVShotDirector::SetSceneTagVisibility(FName VisibleStateTag)
{
	SetActorsWithTagHidden(ButtonWorldTag, VisibleStateTag != ButtonWorldTag);
	SetActorsWithTagHidden(MechanicalWorldTag, VisibleStateTag != MechanicalWorldTag);
	SetActorsWithTagHidden(ColorWorldTag, VisibleStateTag != ColorWorldTag);
	SetActorsWithTagHidden(SharedTag, false);
}

void APVShotDirector::ApplyColorToActorsWithTag(FName Tag, FLinearColor Color, float Intensity)
{
	if (Tag == NAME_None) return;

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsWithTag(this, Tag, Actors);
	for (AActor* Actor : Actors)
	{
		if (!Actor) continue;
		TArray<UMeshComponent*> MeshComponents;
		Actor->GetComponents<UMeshComponent>(MeshComponents);
		for (UMeshComponent* MeshComp : MeshComponents)
		{
			if (!MeshComp) continue;
			const FVector Vec(Color.R, Color.G, Color.B);
			MeshComp->SetVectorParameterValueOnMaterials(ColorParameterName, Vec);
			MeshComp->SetVectorParameterValueOnMaterials(EmissiveColorParameterName, Vec);
			MeshComp->SetScalarParameterValueOnMaterials(IntensityParameterName, Intensity);
		}
	}
}

void APVShotDirector::SetActorRenderHidden(AActor* Actor, bool bHidden) const
{
	if (!Actor) return;
	Actor->SetActorHiddenInGame(bHidden);
	Actor->SetActorEnableCollision(!bHidden);

	TArray<UPrimitiveComponent*> Primitives;
	Actor->GetComponents<UPrimitiveComponent>(Primitives);
	for (UPrimitiveComponent* Prim : Primitives)
	{
		if (Prim)
		{
			Prim->SetVisibility(!bHidden, true);
			Prim->SetHiddenInGame(bHidden, true);
			Prim->SetCollisionEnabled(bHidden ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
		}
	}
}

FVector APVShotDirector::CellToWorld(FIntPoint Cell) const
{
	return FVector(Cell.X * DefaultCellSize, Cell.Y * DefaultCellSize, DefaultCellSize * 0.5f);
}

void APVShotDirector::TriggerPVShake()
{
	RuntimeShakeTarget = ResolveShakeTarget();
	if (!RuntimeShakeTarget) return;

	PVShakeBaseLocation = RuntimeShakeTarget->GetActorLocation();
	PVShakeElapsed = 0.0f;
	bPVShakeActive = true;
}

AActor* APVShotDirector::ResolveShakeTarget() const
{
	if (CameraShakeTarget) return CameraShakeTarget;

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (AActor* ViewTarget = PC->GetViewTarget())
		{
			return ViewTarget;
		}
	}

	TArray<AActor*> Cameras;
	UGameplayStatics::GetAllActorsOfClass(this, ACameraActor::StaticClass(), Cameras);
	return Cameras.Num() > 0 ? Cameras[0] : nullptr;
}
