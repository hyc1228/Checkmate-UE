// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "EyeStateTestActor.h"
#include "EyeStateComponent.h"
#include "Components/StaticMeshComponent.h"

AEyeStateTestActor::AEyeStateTestActor()
{
	PrimaryActorTick.bCanEverTick = false;

	EyeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EyeMesh"));
	RootComponent = EyeMesh;

	EyeState = CreateDefaultSubobject<UEyeStateComponent>(TEXT("EyeState"));
}

void AEyeStateTestActor::BeginPlay()
{
	Super::BeginPlay();

	if (EyeState && EyeMesh)
	{
		EyeState->SetMeshComponent(EyeMesh);
	}
}

void AEyeStateTestActor::EyeToggle()
{
	if (!EyeState)
	{
		return;
	}

	const EEyeState Current = EyeState->GetCurrentState();
	const EEyeState Target = (Current == EEyeState::MechanicalEyed)
		? EEyeState::ButtonEyed
		: EEyeState::MechanicalEyed;

	EyeState->RequestStateTransition(Target, /*bInstant=*/false);
}

void AEyeStateTestActor::EyeSetBlend(float Alpha)
{
	if (!EyeState)
	{
		return;
	}
	EyeState->DebugSetBlendAlpha(Alpha);
}

void AEyeStateTestActor::EyeSetStyle(const FString& StyleName)
{
	if (!EyeState)
	{
		return;
	}

	const FString Lower = StyleName.ToLower();
	EButtonStyle Style = EButtonStyle::Pearl;

	if (Lower == TEXT("pearl"))      { Style = EButtonStyle::Pearl; }
	else if (Lower == TEXT("bone"))  { Style = EButtonStyle::Bone; }
	else if (Lower == TEXT("brass")) { Style = EButtonStyle::Brass; }
	else if (Lower == TEXT("resin")) { Style = EButtonStyle::Resin; }
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("EyeSetStyle: unknown style '%s' — expected Pearl|Bone|Brass|Resin."), *StyleName);
		return;
	}

	EyeState->SetButtonStyle(Style);
}

void AEyeStateTestActor::EyeSnap(const FString& StateName)
{
	if (!EyeState)
	{
		return;
	}

	const FString Lower = StateName.ToLower();
	EEyeState Target = EEyeState::ButtonEyed;

	if (Lower == TEXT("button") || Lower == TEXT("buttoneyed"))
	{
		Target = EEyeState::ButtonEyed;
	}
	else if (Lower == TEXT("mechanical") || Lower == TEXT("mechanicaleyed") || Lower == TEXT("mech"))
	{
		Target = EEyeState::MechanicalEyed;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("EyeSnap: unknown state '%s' — expected Button|Mechanical."), *StateName);
		return;
	}

	EyeState->RequestStateTransition(Target, /*bInstant=*/true);
}
