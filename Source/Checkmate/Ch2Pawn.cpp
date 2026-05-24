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
	if (BodyMesh)
	{
		const FVector Tint = (NewMode == ECh2Mode::Clown)
			? FVector(0.95f, 0.75f, 0.2f)
			: FVector(0.92f, 0.92f, 0.98f);
		BodyMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), Tint);
	}

	if (ACh2GameMode* GM = GetGM())
	{
		GM->NotifyModeChanged();
	}
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
	ACh2GameMode* GM = GetGM();
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

			const ECh2CellType T = GM ? GM->GetCellType(Probe) : ECh2CellType::Empty;
			// Wall / Destructible：不可进入，滑停在前一格
			if (T == ECh2CellType::Wall || T == ECh2CellType::Destructible) break;

			// 可进入：成为新候选 landing
			LastValid = Probe;

			// Pickup / Exit：滑停 AT 此格（landed 并触发）
			if (T == ECh2CellType::ClownPickup || T == ECh2CellType::Exit) break;
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
	if (bMoving) return false;  // 当前正在 lerp 中，吃掉新点击

	const TArray<FIntPoint> Valid = GetValidMoves();
	if (!Valid.Contains(TargetCell)) return false;

	PendingFromCell = GetCurrentCell();
	PendingTargetCell = TargetCell;
	bPendingClownMove = (CurrentMode == ECh2Mode::Clown);
	MoveStartLoc = GetActorLocation();
	MoveEndLoc = CellToWorld(TargetCell);
	MoveElapsed = 0.0f;
	bMoving = true;
	return true;
}

void ACh2Pawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	// 1) Lerp 中：推进动画，结束时触发 Notify
	if (bMoving)
	{
		MoveElapsed += DeltaSeconds;
		const float T = FMath::Clamp(MoveElapsed / MoveDuration, 0.0f, 1.0f);
		const float Eased = FMath::InterpEaseOut(0.0f, 1.0f, T, 2.0f);
		// 加一点 hop 弧线（中间略抬起）
		FVector L = FMath::Lerp(MoveStartLoc, MoveEndLoc, Eased);
		L.Z += FMath::Sin(T * PI) * 30.0f;
		SetActorLocation(L);

		if (T >= 1.0f)
		{
			bMoving = false;
			SetActorLocation(MoveEndLoc);
			if (ACh2GameMode* GM = GetGM())
			{
				GM->NotifyPawnMoved(PendingFromCell, PendingTargetCell, bPendingClownMove);
				GM->NotifyPawnEnteredCell(PendingTargetCell);
			}
		}
		return;  // lerp 期间不接受新输入
	}

	// 2) R 键 重启关卡
	if (PC->WasInputKeyJustPressed(EKeys::R))
	{
		UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, /*bRemovePrefixString=*/true)));
		return;
	}

	// 3) LMB → 鼠标 deproject → cell → 尝试移动
	if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		FVector WorldOrigin, WorldDir;
		if (PC->DeprojectMousePositionToWorld(WorldOrigin, WorldDir))
		{
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
