// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Ch2LevelData.h"
#include "DollData.h"
#include "GameFramework/Actor.h"
#include "PVShotDirector.generated.h"

class ACameraActor;
class ACh2Pawn;
class ULevelSequence;
class UMaterialInterface;
class UStaticMesh;

/**
 * PV-only shot director.
 *
 * Drop this actor into a PV capture map and trigger its BlueprintCallable
 * functions from Details panel buttons, Level Blueprint, or Sequencer Event
 * Tracks. It intentionally avoids owning gameplay state; it nudges existing
 * actors/materials into clear, recordable shot states.
 */
UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API APVShotDirector : public AActor
{
	GENERATED_BODY()

public:
	APVShotDirector();

	// ── Actor/tag conventions ────────────────────────────────────────────

	/** Actors with this tag are visible in every PV world state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Tags")
	FName SharedTag = TEXT("PV_Shared");

	/** Visible in the button-eye / deprived world state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Tags")
	FName ButtonWorldTag = TEXT("PV_ButtonWorld");

	/** Visible in the mechanical-eye world state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Tags")
	FName MechanicalWorldTag = TEXT("PV_MechanicalWorld");

	/** Visible in the color/sensorium world state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Tags")
	FName ColorWorldTag = TEXT("PV_ColorWorld");

	/** Actors with this tag are hidden by HideGameplayHUD(true). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Tags")
	FName GameplayHUDTag = TEXT("PV_GameplayHUD");

	// ── Scene-state visuals ──────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Color")
	FLinearColor ButtonWorldColor = FLinearColor(0.12f, 0.12f, 0.13f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Color")
	FLinearColor MechanicalWorldColor = FLinearColor(0.22f, 0.32f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Color")
	FLinearColor ColorBloomColor = FLinearColor(1.0f, 0.72f, 0.52f, 1.0f);

	/** Material parameter names used by existing placeholder materials. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Color")
	FName ColorParameterName = TEXT("Color");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Color")
	FName EmissiveColorParameterName = TEXT("Emissive Color");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Color")
	FName IntensityParameterName = TEXT("Intensity");

	/** Optional override materials for eye close-up props / Ch2 pawn marker. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Eye")
	UMaterialInterface* ButtonEyeMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Eye")
	UMaterialInterface* MechanicalEyeMaterial = nullptr;

	// ── Manual PV effects ────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Explosion")
	UStaticMesh* PuppetMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Explosion")
	UStaticMesh* ExplosionFlashMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Explosion")
	float DefaultCellSize = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Explosion", meta=(ClampMin="0.05"))
	float ExplosionFlashLifeSeconds = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Explosion")
	float ExplosionFlashScale = 2.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Sensorium", meta=(ClampMin="0.05"))
	float SensoriumDefaultDuration = 2.0f;

	/** Optional actor to shake during PV explosions. Defaults to player view target/camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Camera")
	AActor* CameraShakeTarget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Camera", meta=(ClampMin="0.0"))
	float PVShakeMagnitude = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Camera", meta=(ClampMin="0.05"))
	float PVShakeDuration = 0.45f;

	// ── Sequence playback ────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PV|Sequences", meta=(ForceInlineRow))
	TMap<FName, TSoftObjectPtr<ULevelSequence>> PVSequences;

	/** Play a Level Sequence registered in PVSequences. Returns false if missing. */
	UFUNCTION(BlueprintCallable, Category="PV|Sequences")
	bool PlayPVSequence(FName ShotId);

	// ── Blueprint/Sequencer trigger surface ──────────────────────────────

	/** Convenience dispatcher for common shot ids, e.g. B2, B3, C11. */
	UFUNCTION(BlueprintCallable, Category="PV|Shots")
	void PlayShot(FName ShotId);

	/** StateId: ButtonWorld, MechanicalWorld, ColorBloom. Unknown ids only log. */
	UFUNCTION(BlueprintCallable, Category="PV|World")
	void ApplyPVSceneState(FName StateId);

	/** Forces the active Ch2 pawn eye marker to button/Pearl or mechanical. */
	UFUNCTION(BlueprintCallable, Category="PV|Eye")
	void ForceEyeState(bool bMechanical, bool bAlsoSetCh2Mode = true);

	/** A simple button-eye-fall beat: hides tagged button actors and shows tagged mechanical actors. */
	UFUNCTION(BlueprintCallable, Category="PV|Eye")
	void TriggerButtonFall();

	/** Gradually pushes tagged ColorWorld actors toward ColorBloomColor. */
	UFUNCTION(BlueprintCallable, Category="PV|Sensorium")
	void TriggerSensoriumBloom(float Duration = 0.0f);

	/** Spawns a PV puppet prop and explodes it after Delay seconds. */
	UFUNCTION(BlueprintCallable, Category="PV|Explosion")
	void SpawnAndExplodePuppetForPV(FIntPoint Cell, float Delay = 0.35f);

	/** Instantly emits an explosion flash at a grid cell and hides nearby destructible-tag actors. */
	UFUNCTION(BlueprintCallable, Category="PV|Explosion")
	void TriggerPuppetExplosionAt(FIntPoint Cell);

	/** Hides/shows actors tagged with GameplayHUDTag. */
	UFUNCTION(BlueprintCallable, Category="PV|Utility")
	void HideGameplayHUD(bool bHidden);

	/** Returns the first Ch2 pawn in the world, or nullptr. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="PV|Utility")
	ACh2Pawn* FindCh2Pawn() const;

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	bool bSensoriumBloomActive = false;
	float SensoriumBloomElapsed = 0.0f;
	float SensoriumBloomDuration = 0.0f;

	bool bPVShakeActive = false;
	float PVShakeElapsed = 0.0f;
	FVector PVShakeBaseLocation = FVector::ZeroVector;
	UPROPERTY()
	AActor* RuntimeShakeTarget = nullptr;

	void SetActorsWithTagHidden(FName Tag, bool bHidden);
	void SetSceneTagVisibility(FName VisibleStateTag);
	void ApplyColorToActorsWithTag(FName Tag, FLinearColor Color, float Intensity = 1.0f);
	void SetActorRenderHidden(AActor* Actor, bool bHidden) const;
	FVector CellToWorld(FIntPoint Cell) const;
	void TriggerPVShake();
	AActor* ResolveShakeTarget() const;
};

