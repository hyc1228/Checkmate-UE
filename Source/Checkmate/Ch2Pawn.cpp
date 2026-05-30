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
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (CubeAsset.Succeeded()) BodyMesh->SetStaticMesh(CubeAsset.Object);
	BodyMesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.0f));

	// 机械眼 hint：pawn 顶部小球（spec：Ch2 玩家 = 机械眼）
	EyeMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EyeMarker"));
	EyeMarker->SetupAttachment(BodyMesh);
	EyeMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (SphereAsset.Succeeded()) EyeMarker->SetStaticMesh(SphereAsset.Object);
	EyeMarker->SetRelativeLocation(FVector(60.0f, 0.0f, 50.0f));  // 向 +X 突出一点像"看的方向"
	EyeMarker->SetRelativeScale3D(FVector(0.25f));
}

void ACh2Pawn::BeginPlay()
{
	Super::BeginPlay();
	SetMode(CurrentMode);  // 应用初始 tint
	// Ch2 玩家默认机械眼（spec 锁定）
	SetEyeStyle(EButtonEyeStyle::Standard);
}

void ACh2Pawn::SetEyeStyle(EButtonEyeStyle NewStyle)
{
	CurrentEyeStyle = NewStyle;
	if (!EyeMarker) return;

	UMaterialInterface* Mat = nullptr;
	switch (NewStyle)
	{
	case EButtonEyeStyle::Pearl:    Mat = EyeMaterial_Pearl; break;
	case EButtonEyeStyle::VeilPin:  // 没建 MI，复用 Pearl
	case EButtonEyeStyle::Bell:
	case EButtonEyeStyle::Standard: Mat = EyeMaterial_Pearl; break;
	}
	// Ch2 默认机械（spec：进 Ch2 玩家已是机械眼）。SetMode(Clown) 时也走 Mechanical。
	if (CurrentMode == ECh2Mode::Clown || NewStyle == EButtonEyeStyle::Standard)
	{
		Mat = EyeMaterial_Mechanical ? EyeMaterial_Mechanical : Mat;
	}
	if (Mat)
	{
		EyeMarker->SetMaterial(0, Mat);
	}
}

void ACh2Pawn::SetMode(ECh2Mode NewMode)
{
	CurrentMode = NewMode;
	ModePulseElapsed = 0.0f;
	if (BodyMesh)
	{
		const FVector Tint = (NewMode == ECh2Mode::Clown)
			? FVector(0.95f, 0.75f, 0.2f)
			: FVector(0.92f, 0.92f, 0.98f);
		BodyMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), Tint);
	}
	if (EyeMarker)
	{
		// 机械眼 cyan—两种模式都是 Ch2 玩家，眼睛都是机械眼
		EyeMarker->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.2f, 0.95f, 1.4f));
	}

	if (ACh2GameMode* GM = GetGM())
	{
		GM->NotifyModeChanged();
	}

	// 切 mode 同步切眼睛材质（spec：切换 ritual 取扣 → 缝扣的 Glimpse 段视效）
	SetEyeStyle(CurrentEyeStyle);
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
	if (!Valid.Contains(TargetCell))
	{
		if (ACh2GameMode* GM = GetGM())
		{
			GM->NotifyInvalidMove(TargetCell);
		}
		return false;
	}

	PendingFromCell = GetCurrentCell();
	PendingTargetCell = TargetCell;
	bPendingClownMove = (CurrentMode == ECh2Mode::Clown);
	MoveStartLoc = GetActorLocation();
	MoveEndLoc = CellToWorld(TargetCell);
	MoveElapsed = 0.0f;
	bMoving = true;
	// 通知 GameMode 触发 click ripple + 清路径预览
	if (ACh2GameMode* GM = GetGM())
	{
		GM->NotifyMoveCommitted(TargetCell);
	}
	return true;
}

// 注：MoveDuration 软化（走近 Exit 时变慢）在 NativeTick 里按 EffectiveDuration 计算。

void ACh2Pawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	// 1) Lerp 中：推进动画，结束时触发 Notify
	if (bMoving)
	{
		// sensorium 软化：走近 Exit 时移动变慢（spec 「控制响应轻微软化」）
		float EffectiveDuration = MoveDuration;
		if (ACh2GameMode* GM = GetGM())
		{
			if (GM->LevelData)
			{
				const FIntPoint ExitCell = GM->LevelData->FindCellOfType(ECh2CellType::Exit);
				if (ExitCell.X >= 0)
				{
					const FIntPoint Cur = WorldToCell(MoveEndLoc);
					const int32 Dist = FMath::Abs(Cur.X - ExitCell.X) + FMath::Abs(Cur.Y - ExitCell.Y);
					if (Dist <= 3)
					{
						const float Soften = 1.0f + (1.0f - Dist / 3.0f) * 0.6f;  // 最近 ×1.6
						EffectiveDuration = MoveDuration * Soften;
					}
				}
			}
		}
		MoveElapsed += DeltaSeconds;
		const float T = FMath::Clamp(MoveElapsed / EffectiveDuration, 0.0f, 1.0f);
		const float Eased = FMath::InterpEaseOut(0.0f, 1.0f, T, 2.0f);
		// 加一点 hop 弧线（中间略抬起）
		FVector L = FMath::Lerp(MoveStartLoc, MoveEndLoc, Eased);
		const float Arc = FMath::Sin(T * PI);
		const float HopHeight = bPendingClownMove ? ClownHopHeight : MoveHopHeight;
		L.Z += Arc * HopHeight;
		SetActorLocation(L);

		if (BodyMesh)
		{
			const FVector MoveDir = (MoveEndLoc - MoveStartLoc).GetSafeNormal2D();
			const float Squash = Arc * MoveSquashAmount;
			BodyMesh->SetRelativeScale3D(FVector(
				0.6f + Squash * 0.35f,
				0.6f + Squash * 0.35f,
				1.0f - Squash));
			BodyMesh->SetRelativeRotation(FRotator(
				-MoveDir.X * MoveLeanDegrees * Arc,
				0.0f,
				MoveDir.Y * MoveLeanDegrees * Arc));
		}
		if (EyeMarker)
		{
			const float EyePulse = 1.0f + Arc * 0.35f;
			EyeMarker->SetRelativeScale3D(FVector(0.25f * EyePulse));
		}

		if (T >= 1.0f)
		{
			bMoving = false;
			SetActorLocation(MoveEndLoc);
			if (BodyMesh)
			{
				BodyMesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.0f));
				BodyMesh->SetRelativeRotation(FRotator::ZeroRotator);
			}
			if (EyeMarker)
			{
				EyeMarker->SetRelativeScale3D(FVector(0.25f));
			}
			if (ACh2GameMode* GM = GetGM())
			{
				GM->NotifyPawnMoved(PendingFromCell, PendingTargetCell, bPendingClownMove);
				GM->NotifyPawnEnteredCell(PendingTargetCell);
			}
		}
		return;  // lerp 期间不接受新输入
	}

	// 2) Idle bob：站着时身体微微浮动呼吸（机械眼隐喻：未睡死的人偶）
	{
		static float IdleElapsed = 0.0f;
		IdleElapsed += DeltaSeconds;
		ModePulseElapsed += DeltaSeconds;
		const float Bob = FMath::Sin(IdleElapsed * 2.5f) * 4.0f;
		const float ModePulse = ModePulseElapsed < 0.35f
			? FMath::Sin((ModePulseElapsed / 0.35f) * PI) * 0.18f
			: 0.0f;
		if (BodyMesh)
		{
			FVector RelLoc = BodyMesh->GetRelativeLocation();
			RelLoc.Z = Bob;
			BodyMesh->SetRelativeLocation(RelLoc);
			BodyMesh->SetRelativeScale3D(FVector(0.6f + ModePulse, 0.6f + ModePulse, 1.0f + ModePulse * 0.5f));
			BodyMesh->SetRelativeRotation(FRotator::ZeroRotator);
		}
		if (EyeMarker)
		{
			const float EyePulse = 1.0f + ModePulse * 1.2f + FMath::Max(0.0f, FMath::Sin(IdleElapsed * 5.0f)) * 0.06f;
			EyeMarker->SetRelativeScale3D(FVector(0.25f * EyePulse));
		}
	}

	// 3) R 键 重启关卡
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
