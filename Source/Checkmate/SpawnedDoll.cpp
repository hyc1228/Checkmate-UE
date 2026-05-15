// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "SpawnedDoll.h"
#include "DollData.h"
#include "EyeStateComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ASpawnedDoll::ASpawnedDoll()
{
	PrimaryActorTick.bCanEverTick = false;

	BodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BodyMesh"));
	RootComponent = BodyMesh;

	PlaceholderCube = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderCube"));
	PlaceholderCube->SetupAttachment(RootComponent);
	PlaceholderCube->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 默认引擎自带 cube
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		PlaceholderCube->SetStaticMesh(CubeFinder.Object);
		PlaceholderCube->SetRelativeScale3D(FVector(0.5f, 0.5f, 1.5f));  // 拉成竖长体
	}

	EyeState = CreateDefaultSubobject<UEyeStateComponent>(TEXT("EyeState"));
}

void ASpawnedDoll::BeginPlay()
{
	Super::BeginPlay();
	ApplyVisualFromData();
}

void ASpawnedDoll::InitializeFromData(UDollData* InData)
{
	DollData = InData;
	if (HasActorBegunPlay())
	{
		// 已经 BeginPlay 之后再 init，立即应用
		ApplyVisualFromData();
	}
}

void ASpawnedDoll::ApplyVisualFromData()
{
	if (!DollData)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnedDoll: DollData == nullptr，保持 placeholder cube。"));
		return;
	}

	// BodyMesh：whitebox 阶段如为空就保持 cube
	if (USkeletalMesh* Body = DollData->BodyMesh.LoadSynchronous())
	{
		BodyMesh->SetSkeletalMesh(Body);
		PlaceholderCube->SetVisibility(false);
	}
	else
	{
		// 用 placeholder cube；颜色按 ButtonStyle 染色（debug 用）
		PlaceholderCube->SetVisibility(true);
	}

	// 头发
	if (DollData->bLongHair || DollData->bStyled)
	{
		if (UStaticMesh* Hair = DollData->HairMesh.LoadSynchronous())
		{
			AttachAccessoryToSocket(Hair, TEXT("HairSocket"));
		}
	}

	// 饰物：按 13 属性逐个 attach
	if (DollData->bPearlNecklace) { if (UStaticMesh* M = DollData->PearlNecklaceMesh.LoadSynchronous()) AttachAccessoryToSocket(M, TEXT("NecklaceSocket")); }
	if (DollData->bVeil)          { if (UStaticMesh* M = DollData->VeilMesh.LoadSynchronous())          AttachAccessoryToSocket(M, TEXT("VeilSocket")); }
	if (DollData->bApron)         { if (UStaticMesh* M = DollData->ApronMesh.LoadSynchronous())         AttachAccessoryToSocket(M, TEXT("ApronSocket")); }
	if (DollData->bRibbon)        { if (UStaticMesh* M = DollData->RibbonMesh.LoadSynchronous())        AttachAccessoryToSocket(M, TEXT("RibbonSocket")); }
	if (DollData->bWhiteGloves)   { if (UStaticMesh* M = DollData->WhiteGlovesMesh.LoadSynchronous())   AttachAccessoryToSocket(M, TEXT("GlovesSocket")); }

	// 同步眼睛初始状态：所有娃娃默认按扣眼
	if (EyeState)
	{
		EyeState->SetButtonStyle(DollData->ButtonStyle);
	}

	UE_LOG(LogTemp, Log, TEXT("SpawnedDoll(%s): visual applied (BodyMesh=%s, ButtonStyle=%s)"),
		*GetName(),
		BodyMesh->GetSkeletalMeshAsset() ? TEXT("set") : TEXT("placeholder"),
		*UEnum::GetValueAsString(DollData->ButtonStyle));
}

void ASpawnedDoll::AttachAccessoryToSocket(UStaticMesh* Mesh, FName SocketName)
{
	if (!Mesh || !BodyMesh) { return; }

	UStaticMeshComponent* NewComp = NewObject<UStaticMeshComponent>(this);
	NewComp->SetStaticMesh(Mesh);
	NewComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NewComp->RegisterComponent();
	NewComp->AttachToComponent(BodyMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, SocketName);
	AccessoryMeshes.Add(NewComp);
}
