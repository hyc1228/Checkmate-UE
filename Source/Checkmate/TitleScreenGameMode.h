// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TitleScreenGameMode.generated.h"

class UPrimitiveComponent;
class UTitlePromptWidget;
class ACameraActor;

UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API ATitleScreenGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATitleScreenGameMode();

	UFUNCTION(BlueprintCallable, Category="Title")
	void StartTitleFall();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditDefaultsOnly, Category="Title")
	FName FirstGameMapName = TEXT("Ch1Test");

	UPROPERTY(EditDefaultsOnly, Category="Title", meta=(ClampMin="0.0"))
	float OpenFirstMapDelaySeconds = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category="Title", meta=(ClampMin="0.0"))
	float TransitionFadeDurationSeconds = 1.25f;

	UPROPERTY(EditDefaultsOnly, Category="Title")
	float FallingGravityZ = -980.0f;

	UPROPERTY(EditDefaultsOnly, Category="Title", meta=(ClampMin="0.0"))
	float TitleShardReleaseDelaySeconds = 0.75f;

	UPROPERTY(EditDefaultsOnly, Category="Title")
	TSubclassOf<UTitlePromptWidget> PromptWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Title")
	TArray<FString> DropNameHints;

	UPROPERTY(EditDefaultsOnly, Category="Title")
	TArray<FString> TitleShardNameHints;

	UPROPERTY(EditDefaultsOnly, Category="Title")
	TArray<FString> StaticNameHints;

	UPROPERTY(EditDefaultsOnly, Category="Title|Idle Shake")
	bool bEnableTitleIdleShake = true;

	UPROPERTY(EditDefaultsOnly, Category="Title|Idle Shake", meta=(ClampMin="0.0"))
	float TitleIdleShakeAmplitude = 2.4f;

	UPROPERTY(EditDefaultsOnly, Category="Title|Idle Shake", meta=(ClampMin="0.0"))
	float TitleIdleShakeFrequency = 1.8f;

	UPROPERTY(EditDefaultsOnly, Category="Title|Idle Shake", meta=(ClampMin="0.0"))
	float TitleIdleShakeRotationDegrees = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category="Title|Camera")
	FVector TitleCameraLocation = FVector(-61.0f, 700.0f, 200.0f);

	UPROPERTY(EditDefaultsOnly, Category="Title|Camera")
	FRotator TitleCameraRotation = FRotator(0.0f, -90.0f, 0.0f);

	UPROPERTY(EditDefaultsOnly, Category="Title|Camera", meta=(ClampMin="5.0", ClampMax="170.0"))
	float TitleCameraFOV = 55.0f;

private:
	UPROPERTY()
	UTitlePromptWidget* PromptWidget = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UPrimitiveComponent>> DropComponents;

	UPROPERTY()
	TArray<TObjectPtr<UPrimitiveComponent>> TitleShardComponents;

	UPROPERTY()
	TArray<FTransform> TitleShardBaseWorldTransforms;

	bool bStarted = false;
	float TitleIdleShakeElapsedSeconds = 0.0f;
	FVector TitleShardShakePivot = FVector::ZeroVector;
	FTimerHandle OpenFirstMapTimerHandle;
	FTimerHandle ReleaseTitleShardsTimerHandle;
	FTimerHandle FadeToBlackTimerHandle;

	void CollectDropComponents();
	bool ShouldDropComponent(const UPrimitiveComponent* Component) const;
	bool ShouldTitleShardComponent(const UPrimitiveComponent* Component) const;
	bool TextContainsAnyHint(const FString& Text, const TArray<FString>& Hints) const;
	void FreezeDropComponents();
	void FreezeTitleShardComponents();
	void CacheTitleShardBaseTransforms();
	void RestoreTitleShardBaseTransforms();
	void UpdateTitleIdleShake(float DeltaSeconds);
	void ReleaseTitleShards();
	void StartFadeToBlack();
	void ActivateTitleCamera(APlayerController* PC);
	ACameraActor* FindOrSpawnTitleCamera() const;
	void OpenFirstMap();
};
