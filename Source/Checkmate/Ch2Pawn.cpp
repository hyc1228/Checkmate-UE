// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "Ch2Pawn.h"

#include "Ch2GameMode.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ACh2Pawn::ACh2Pawn()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(SceneRoot);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BodyMesh->SetCollisionResponseToAllChannels(ECR_Block);
	BodyMesh->SetGenerateOverlapEvents(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeAsset.Succeeded())
	{
		BodyMesh->SetStaticMesh(CubeAsset.Object);
	}
	BodyMesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.0f));
}

void ACh2Pawn::BeginPlay()
{
	Super::BeginPlay();
	SetMode(CurrentMode);  // 应用初始 tint
}

void ACh2Pawn::SetMode(ECh2Mode NewMode)
{
	CurrentMode = NewMode;
	if (!BodyMesh) return;

	const FVector Tint = (NewMode == ECh2Mode::Clown)
		? FVector(0.95f, 0.75f, 0.2f)   // 小丑金
		: FVector(0.92f, 0.92f, 0.98f); // 芭蕾白
	BodyMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), Tint);
}

ACh2GameMode* ACh2Pawn::GetGM() const
{
	return Cast<ACh2GameMode>(UGameplayStatics::GetGameMode(this));
}

FVector ACh2Pawn::CellToWorld(FIntPoint Cell) const
{
	return FVector(Cell.X * CellSize, Cell.Y * CellSize, CellSize * 0.5f);
}

FIntPoint ACh2Pawn::WorldToCell(FVector World) const
{
	return FIntPoint(
		FMath::RoundToInt(World.X / CellSize),
		FMath::RoundToInt(World.Y / CellSize));
}

FIntPoint ACh2Pawn::GetCurrentCell() const
{
	return WorldToCell(GetActorLocation());
}

bool ACh2Pawn::IsInBounds(FIntPoint Cell) const
{
	if (ACh2GameMode* GM = GetGM())
	{
		return GM->IsInBounds(Cell);
	}
	return Cell.X >= 0 && Cell.X < GridSize && Cell.Y >= 0 && Cell.Y < GridSize;
}

bool ACh2Pawn::IsWall(FIntPoint Cell) const
{
	if (ACh2GameMode* GM = GetGM())
	{
		return GM->IsBlocked(Cell);
	}
	return false;
}

void ACh2Pawn::ComputeBalletMoves(TArray<FIntPoint>& OutMoves) const
{
	const FIntPoint Cur = GetCurrentCell();
	const FIntPoint Dirs[4] = { {1,0}, {-1,0}, {0,1}, {0,-1} };

	for (const FIntPoint& D : Dirs)
	{
		FIntPoint Probe = Cur;
		FIntPoint LastValid = Cur;
		for (int32 Step = 0; Step < 64; ++Step)
		{
			Probe.X += D.X;
			Probe.Y += D.Y;
			if (!IsInBounds(Probe)) break;
			if (IsWall(Probe)) break;
			LastValid = Probe;
		}
		if (LastValid != Cur)
		{
			OutMoves.Add(LastValid);
		}
	}
}

void ACh2Pawn::ComputeClownMoves(TArray<FIntPoint>& OutMoves) const
{
	const FIntPoint Cur = GetCurrentCell();
	static const FIntPoint Knights[8] = {
		{2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}
	};
	for (const FIntPoint& K : Knights)
	{
		const FIntPoint T(Cur.X + K.X, Cur.Y + K.Y);
		if (!IsInBounds(T)) continue;
		if (IsWall(T)) continue;
		OutMoves.Add(T);
	}
}

TArray<FIntPoint> ACh2Pawn::GetValidMoves() const
{
	TArray<FIntPoint> Out;
	if (CurrentMode == ECh2Mode::Ballet) ComputeBalletMoves(Out);
	else                                  ComputeClownMoves(Out);
	return Out;
}

bool ACh2Pawn::TryMoveTo(FIntPoint TargetCell)
{
	const TArray<FIntPoint> Valid = GetValidMoves();
	if (!Valid.Contains(TargetCell)) return false;

	const FIntPoint FromCell = GetCurrentCell();
	const bool bClownMove = (CurrentMode == ECh2Mode::Clown);

	SetActorLocation(CellToWorld(TargetCell));

	if (ACh2GameMode* GM = GetGM())
	{
		GM->NotifyPawnMoved(FromCell, TargetCell, bClownMove);
		GM->NotifyPawnEnteredCell(TargetCell);  // 拾取 / 胜利检查
	}
	return true;
}

void ACh2Pawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		FVector WorldOrigin, WorldDir;
		if (PC->DeprojectMousePositionToWorld(WorldOrigin, WorldDir))
		{
			// 与 Z=0 平面相交（地板高度）
			if (FMath::Abs(WorldDir.Z) > KINDA_SMALL_NUMBER)
			{
				const float T = -WorldOrigin.Z / WorldDir.Z;
				if (T > 0.0f)
				{
					const FVector HitWorld = WorldOrigin + WorldDir * T;
					const FIntPoint Target = WorldToCell(HitWorld);
					TryMoveTo(Target);
				}
			}
		}
	}
}

FText ACh2Pawn::GetModeDisplayName() const
{
	return FText::FromString(CurrentMode == ECh2Mode::Clown ? TEXT("小丑") : TEXT("芭蕾"));
}
