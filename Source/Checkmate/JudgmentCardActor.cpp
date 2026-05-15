// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "JudgmentCardActor.h"
#include "CardData.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

AJudgmentCardActor::AJudgmentCardActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CardMesh"));
	RootComponent = CardMesh;
	CardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CardMesh->SetRelativeScale3D(FVector(0.5f, 0.7f, 0.02f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneFinder.Succeeded())
	{
		CardMesh->SetStaticMesh(PlaneFinder.Object);
	}
}

void AJudgmentCardActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyVisualFromData();
}

void AJudgmentCardActor::InitializeFromData(UCardData* InData)
{
	CardData = InData;
	if (HasActorBegunPlay())
	{
		ApplyVisualFromData();
	}
}

void AJudgmentCardActor::ApplyVisualFromData()
{
	if (!CardData) { return; }

	if (CardMaterial)
	{
		CardMID = CardMesh->CreateDynamicMaterialInstance(0, CardMaterial);
		if (CardMID)
		{
			if (UTexture2D* Icon = CardData->IconTexture.LoadSynchronous())
			{
				CardMID->SetTextureParameterValue(BaseColorParamName, Icon);
			}
			CardMID->SetVectorParameterValue(TintParamName, CardData->bIsPearlCompatible
				? FLinearColor(1.0f, 0.95f, 0.85f, 1.0f)
				: FLinearColor::White);
		}
	}
}
