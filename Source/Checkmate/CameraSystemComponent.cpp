// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "CameraSystemComponent.h"
#include "Camera/CameraActor.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

UCameraSystemComponent::UCameraSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCameraSystemComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			CachedSubsystem = GI->GetSubsystem<UCheckmateGameStateSubsystem>();
		}
	}

	if (CachedSubsystem)
	{
		CachedSubsystem->OnStateChanged.AddDynamic(this, &UCameraSystemComponent::HandleStateChanged);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("CameraSystemComponent: GameStateSubsystem 拿不到，订阅失败。"));
	}
}

void UCameraSystemComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (CachedSubsystem)
	{
		CachedSubsystem->OnStateChanged.RemoveDynamic(this, &UCameraSystemComponent::HandleStateChanged);
	}
	Super::EndPlay(Reason);
}

void UCameraSystemComponent::HandleStateChanged(ECheckmateGameState OldState, ECheckmateGameState NewState)
{
	using S = ECheckmateGameState;
	ECameraMode Target = CurrentMode;

	switch (NewState)
	{
	case S::Ch1_CardSelect:        Target = ECameraMode::Ch1_CardSelect;    break;
	case S::Ch1_Transition2Dto3D:  Target = ECameraMode::Ch1_Inspection;    break;  // 用 smooth 过渡
	case S::Ch1_Shift:             Target = ECameraMode::Ch1_Inspection;    break;
	case S::Ch1_PostShiftFade:     Target = ECameraMode::Ch1_PostShiftFade; break;
	case S::Twist_Sequence:        return;  // 由 LS_Twist_Sequence 接管
	case S::Ch2_Awakening_Stub:    Target = ECameraMode::Ch2_Topdown_Stub;  break;
	default: return;
	}

	const bool bSmooth = (NewState == S::Ch1_Transition2Dto3D);
	SwitchToMode(Target, bSmooth);
}

void UCameraSystemComponent::SwitchToMode(ECameraMode Mode, bool bSmooth)
{
	if (Mode == CurrentMode) { return; }

	ACameraActor* TargetCamera = FindCameraForMode(Mode);
	APlayerController* PC = GetPC();

	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("CameraSystemComponent: PlayerController == nullptr，无法切镜头。"));
		return;
	}

	if (!TargetCamera)
	{
		UE_LOG(LogTemp, Warning, TEXT("CameraSystemComponent: mode %s 没有绑定 ACameraActor。请在 Level 里摆 ACameraActor 并填到 ModeCameras。"),
			*UEnum::GetValueAsString(Mode));
		return;
	}

	const float BlendTime = bSmooth ? TransitionSeconds : 0.0f;
	PC->SetViewTargetWithBlend(TargetCamera, BlendTime, VTBlend_EaseInOut, 2.0f, false);

	CurrentMode = Mode;

	UE_LOG(LogTemp, Log, TEXT("Camera: → %s (blend=%.2fs)"),
		*UEnum::GetValueAsString(Mode), BlendTime);
}

ACameraActor* UCameraSystemComponent::FindCameraForMode(ECameraMode Mode) const
{
	for (const FCameraModeBinding& Binding : ModeCameras)
	{
		if (Binding.Mode == Mode)
		{
			return Binding.Camera.LoadSynchronous();
		}
	}
	return nullptr;
}

APlayerController* UCameraSystemComponent::GetPC() const
{
	if (UWorld* World = GetWorld())
	{
		return World->GetFirstPlayerController();
	}
	return nullptr;
}
