// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "EyeStateComponent.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/Actor.h"

UEyeStateComponent::UEyeStateComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UEyeStateComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!TargetMesh)
	{
		if (AActor* Owner = GetOwner())
		{
			TargetMesh = Owner->FindComponentByClass<UMeshComponent>();
		}
	}

	if (!ensureMsgf(TargetMesh, TEXT("UEyeStateComponent on %s: no MeshComponent found on owner."),
		*GetNameSafe(GetOwner())))
	{
		return;
	}

	if (!ensureMsgf(EyeMaterialTemplate, TEXT("UEyeStateComponent on %s: EyeMaterialTemplate is null."),
		*GetNameSafe(GetOwner())))
	{
		return;
	}

	EyeMID = TargetMesh->CreateDynamicMaterialInstance(MaterialSlotIndex, EyeMaterialTemplate);

	if (!ensureMsgf(EyeMID, TEXT("UEyeStateComponent on %s: CreateDynamicMaterialInstance returned null (slot %d)."),
		*GetNameSafe(GetOwner()), MaterialSlotIndex))
	{
		return;
	}

	SetButtonStyle(InitialButtonStyle);

	CurrentState = InitialEyeState;
	CurrentBlendAlpha = BlendAlphaForState(InitialEyeState);
	PushBlendAlphaToMID(CurrentBlendAlpha);
}

void UEyeStateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsBlending)
	{
		return;
	}

	BlendElapsed += DeltaTime;
	const float T = TransitionSeconds > KINDA_SMALL_NUMBER
		? FMath::Clamp(BlendElapsed / TransitionSeconds, 0.0f, 1.0f)
		: 1.0f;

	CurrentBlendAlpha = FMath::Lerp(BlendStart, BlendTarget, T);
	PushBlendAlphaToMID(CurrentBlendAlpha);

	if (T >= 1.0f)
	{
		FinishBlend();
	}
}

void UEyeStateComponent::RequestStateTransition(EEyeState Target, bool bInstant)
{
	if (Target == EEyeState::Transitioning)
	{
		return;
	}

	if (!EyeMID)
	{
		return;
	}

	const float TargetAlpha = BlendAlphaForState(Target);

	if (bInstant || TransitionSeconds <= KINDA_SMALL_NUMBER)
	{
		const EEyeState OldState = CurrentState;
		CurrentBlendAlpha = TargetAlpha;
		PushBlendAlphaToMID(CurrentBlendAlpha);
		CurrentState = Target;
		bIsBlending = false;
		SetComponentTickEnabled(false);
		OnEyeStateChanged.Broadcast(OldState, Target);
		return;
	}

	const EEyeState OldState = CurrentState;
	BlendStart = CurrentBlendAlpha;
	BlendTarget = TargetAlpha;
	BlendElapsed = 0.0f;
	bIsBlending = true;
	PendingFinalState = Target;
	CurrentState = EEyeState::Transitioning;
	SetComponentTickEnabled(true);

	OnEyeStateChanged.Broadcast(OldState, EEyeState::Transitioning);
}

void UEyeStateComponent::SetButtonStyle(EButtonStyle Style)
{
	if (!EyeMID)
	{
		return;
	}
	EyeMID->SetVectorParameterValue(ParamNames.ButtonTint, FButtonStylePalette::Resolve(Style));
}

void UEyeStateComponent::SetMeshComponent(UMeshComponent* InMesh)
{
	TargetMesh = InMesh;
}

void UEyeStateComponent::DebugSetBlendAlpha(float Alpha)
{
	if (!EyeMID)
	{
		return;
	}
	bIsBlending = false;
	SetComponentTickEnabled(false);
	CurrentBlendAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	PushBlendAlphaToMID(CurrentBlendAlpha);
}

void UEyeStateComponent::PushBlendAlphaToMID(float Alpha)
{
	if (!EyeMID)
	{
		return;
	}
	EyeMID->SetScalarParameterValue(ParamNames.BlendAlpha, Alpha);
	EyeMID->SetScalarParameterValue(ParamNames.MechanicalMetallic, Alpha);
	EyeMID->SetScalarParameterValue(ParamNames.IrisBloom, Alpha);
	EyeMID->SetScalarParameterValue(ParamNames.EmissiveStrength, Alpha);
}

void UEyeStateComponent::FinishBlend()
{
	bIsBlending = false;
	SetComponentTickEnabled(false);

	const EEyeState OldTransitionState = CurrentState;
	CurrentState = PendingFinalState;
	CurrentBlendAlpha = BlendAlphaForState(PendingFinalState);
	PushBlendAlphaToMID(CurrentBlendAlpha);

	OnEyeStateChanged.Broadcast(OldTransitionState, CurrentState);
}

float UEyeStateComponent::BlendAlphaForState(EEyeState State)
{
	switch (State)
	{
	case EEyeState::ButtonEyed:     return 0.0f;
	case EEyeState::MechanicalEyed: return 1.0f;
	case EEyeState::Transitioning:  return 0.5f;
	default:                         return 0.0f;
	}
}
