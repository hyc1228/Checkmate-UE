// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "Ch2GameMode.h"

#include "Ch2HUDWidget.h"
#include "Ch2Pawn.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ACh2GameMode::ACh2GameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;  // 20Hz 够用，节省 cpu
	DefaultPawnClass = APawn::StaticClass();
}

void ACh2GameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!LevelData)
	{
		UE_LOG(LogTemp, Error, TEXT("Ch2GameMode: LevelData 未填 — 关卡建不起来"));
		return;
	}
	if (!PawnClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Ch2GameMode: PawnClass 未填"));
		return;
	}

	BuildLevel();
	SetUpTopDownCamera();
}

ECh2CellType ACh2GameMode::GetCellType(FIntPoint Cell) const
{
	return LevelData ? LevelData->GetCellType(Cell) : ECh2CellType::Empty;
}

bool ACh2GameMode::IsBlocked(FIntPoint Cell) const
{
	const ECh2CellType T = GetCellType(Cell);
	return T == ECh2CellType::Wall || T == ECh2CellType::Destructible;
}

bool ACh2GameMode::IsInBounds(FIntPoint Cell) const
{
	if (!LevelData) return false;
	return Cell.X >= 0 && Cell.X < LevelData->GridWidth
		&& Cell.Y >= 0 && Cell.Y < LevelData->GridHeight;
}

int32 ACh2GameMode::GetGridWidth() const  { return LevelData ? LevelData->GridWidth  : 0; }
int32 ACh2GameMode::GetGridHeight() const { return LevelData ? LevelData->GridHeight : 0; }
float ACh2GameMode::GetCellSize() const   { return LevelData ? LevelData->CellSize   : 200.0f; }

void ACh2GameMode::BuildLevel()
{
	UWorld* World = GetWorld();
	if (!World || !LevelData) return;

	// 默认 mesh：fall back 到 engine cube / plane
	UStaticMesh* FloorM = FloorMesh
		? FloorMesh : LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	UStaticMesh* WallM = WallMesh
		? WallMesh : LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* PickupM = PickupMesh
		? PickupMesh : LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* ExitM = ExitMesh
		? ExitMesh : LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));

	const float CS = LevelData->CellSize;
	const int32 W = LevelData->GridWidth;
	const int32 H = LevelData->GridHeight;

	// 1) 全格地板（黑白交替）
	for (int32 X = 0; X < W; ++X)
	for (int32 Y = 0; Y < H; ++Y)
	{
		const FVector Loc(X * CS, Y * CS, 0.0f);
		AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(Loc, FRotator::ZeroRotator);
		if (!Floor) continue;
		Floor->SetMobility(EComponentMobility::Movable);
		if (UStaticMeshComponent* MC = Floor->GetStaticMeshComponent())
		{
			MC->SetStaticMesh(FloorM);
			MC->SetRelativeScale3D(FVector(CS / 100.0f, CS / 100.0f, 1.0f));
			const bool bWhite = ((X + Y) % 2) == 0;
			MC->SetVectorParameterValueOnMaterials(TEXT("Color"),
				bWhite ? FVector(0.85f, 0.85f, 0.85f) : FVector(0.15f, 0.15f, 0.15f));
		}
		FloorActors.Add(FIntPoint(X, Y), Floor);
	}

	// 2) 特殊 cell 上的 decoration actors
	for (const TPair<FIntPoint, ECh2CellType>& P : LevelData->Cells)
	{
		const FIntPoint& C = P.Key;
		const ECh2CellType Type = P.Value;
		if (Type == ECh2CellType::Empty || Type == ECh2CellType::Start) continue;

		const FVector Loc(C.X * CS, C.Y * CS, CS * 0.5f);  // 立方体抬一半
		AStaticMeshActor* DecoActor = World->SpawnActor<AStaticMeshActor>(Loc, FRotator::ZeroRotator);
		if (!DecoActor) continue;
		DecoActor->SetMobility(EComponentMobility::Movable);
		UStaticMeshComponent* DecoMC = DecoActor->GetStaticMeshComponent();
		if (!DecoMC) continue;

		FVector TintColor(0.5f, 0.5f, 0.5f);
		FVector DecoScale(1.0f, 1.0f, 1.0f);
		UStaticMesh* DecoMesh = WallM;

		switch (Type)
		{
		case ECh2CellType::Wall:
			DecoMesh = WallM;
			DecoScale = FVector(CS / 100.0f, CS / 100.0f, CS / 100.0f);
			TintColor = FVector(0.3f, 0.3f, 0.35f);
			break;
		case ECh2CellType::Destructible:
			DecoMesh = WallM;
			DecoScale = FVector(CS / 100.0f * 0.8f, CS / 100.0f * 0.8f, CS / 100.0f * 0.7f);
			TintColor = FVector(0.6f, 0.45f, 0.3f);  // 木箱色
			break;
		case ECh2CellType::ClownPickup:
			DecoMesh = PickupM;
			DecoScale = FVector(CS / 100.0f * 0.4f, CS / 100.0f * 0.4f, CS / 100.0f * 0.4f);
			TintColor = FVector(0.95f, 0.7f, 0.2f);  // 小丑金
			break;
		case ECh2CellType::Exit:
			DecoMesh = ExitM;
			DecoScale = FVector(CS / 100.0f, CS / 100.0f, 1.0f);
			TintColor = FVector(0.2f, 0.9f, 0.5f);
			DecoActor->SetActorLocation(FVector(C.X * CS, C.Y * CS, 5.0f));
			ExitActor = DecoActor;
			break;
		case ECh2CellType::WeddingWreckage:
			DecoMesh = WallM;
			DecoScale = FVector(CS / 100.0f * 0.45f, CS / 100.0f * 0.45f, CS / 100.0f * 0.25f);
			TintColor = FVector(0.85f, 0.85f, 0.9f);  // 偏白
			// 倒地姿态：rotation pitch -65 + 随机 yaw
			DecoActor->SetActorLocation(FVector(C.X * CS, C.Y * CS, CS * 0.12f));
			DecoActor->SetActorRotation(FRotator(-65.0f, FMath::FRandRange(-30.0f, 30.0f), 0.0f));
			break;
		default: break;
		}

		DecoMC->SetStaticMesh(DecoMesh);
		DecoMC->SetRelativeScale3D(DecoScale);
		DecoMC->SetVectorParameterValueOnMaterials(TEXT("Color"), TintColor);
		CellActors.Add(C, DecoActor);
	}

	// 3) spawn 玩家 pawn 到 Start cell
	const FIntPoint StartCell = LevelData->FindCellOfType(ECh2CellType::Start);
	const FVector StartLoc(StartCell.X * CS, StartCell.Y * CS, CS * 0.5f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActivePawn = World->SpawnActor<ACh2Pawn>(PawnClass, StartLoc, FRotator::ZeroRotator, Params);

	if (ActivePawn)
	{
		ActivePawn->CellSize = CS;
		ActivePawn->GridSize = FMath::Max(W, H);

		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			PC->Possess(ActivePawn);
		}
	}

	// HUD
	if (Ch2HUDClass)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			HUDWidget = CreateWidget<UCh2HUDWidget>(PC, Ch2HUDClass);
			if (HUDWidget)
			{
				HUDWidget->AddToViewport(/*ZOrder=*/10);
				if (ActivePawn) HUDWidget->SetMode(ActivePawn->CurrentMode);
			}
		}
	}

	RefreshHighlights();
}

void ACh2GameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	WorldElapsed += DeltaSeconds;

	// sensorium 扩张：出口附近地板渐变成彩色（曼哈顿距离 ≤ ColorRadius）
	const FIntPoint ExitCell = LevelData ? LevelData->FindCellOfType(ECh2CellType::Exit) : FIntPoint(-1, -1);
	if (ExitCell.X >= 0)
	{
		const int32 ColorRadius = 3;
		const float ExitPulse = 0.5f + 0.5f * FMath::Sin(WorldElapsed * 2.0f);
		for (const TPair<FIntPoint, AActor*>& P : FloorActors)
		{
			if (!P.Value) continue;
			const int32 Dist = FMath::Abs(P.Key.X - ExitCell.X) + FMath::Abs(P.Key.Y - ExitCell.Y);
			if (Dist > ColorRadius) continue;
			AStaticMeshActor* SMA = Cast<AStaticMeshActor>(P.Value);
			if (!SMA) continue;
			UStaticMeshComponent* MC = SMA->GetStaticMeshComponent();
			if (!MC) continue;

			const float Strength = (1.0f - static_cast<float>(Dist) / ColorRadius) * 0.7f * ExitPulse;
			const bool bWhite = ((P.Key.X + P.Key.Y) % 2) == 0;
			const FVector Base = bWhite ? FVector(0.85f) : FVector(0.15f);
			const FVector Colored = FMath::Lerp(Base,
				FVector(0.4f + ExitPulse * 0.3f, 0.85f, 0.55f + ExitPulse * 0.2f),
				Strength);
			MC->SetVectorParameterValueOnMaterials(TEXT("Color"), Colored);
		}
	}

	// 出口 pulse：基色 + sin 节拍亮起（spec「一抹彩色」）
	if (ExitActor)
	{
		if (AStaticMeshActor* SMA = Cast<AStaticMeshActor>(ExitActor))
		{
			if (UStaticMeshComponent* MC = SMA->GetStaticMeshComponent())
			{
				const float Pulse = 0.5f + 0.5f * FMath::Sin(WorldElapsed * 2.0f);
				const FVector C = FMath::Lerp(
					FVector(0.2f, 0.9f, 0.5f),
					FVector(0.7f, 1.0f, 0.85f),
					Pulse);
				MC->SetVectorParameterValueOnMaterials(TEXT("Color"), C);
			}
		}
	}

	// 玩偶 scale pulse：越临近爆炸抖得越快
	for (FCh2PuppetState& P : Puppets)
	{
		if (!P.RuntimeActor) continue;
		AStaticMeshActor* SMA = Cast<AStaticMeshActor>(P.RuntimeActor);
		if (!SMA) continue;
		UStaticMeshComponent* MC = SMA->GetStaticMeshComponent();
		if (!MC) continue;

		const float Urgency = 1.0f - (static_cast<float>(P.TurnsRemaining) / FMath::Max(1, PuppetExplodeAfterTurns));
		const float Freq = 3.0f + Urgency * 12.0f;
		const float ScaleBase = 0.6f + 0.15f * FMath::Sin(WorldElapsed * Freq);
		MC->SetRelativeScale3D(FVector(ScaleBase, ScaleBase, ScaleBase));
	}
}

void ACh2GameMode::NotifyModeChanged()
{
	if (HUDWidget && ActivePawn)
	{
		HUDWidget->SetMode(ActivePawn->CurrentMode);
	}
	RefreshHighlights();
}

void ACh2GameMode::RefreshHighlights()
{
	// 清旧 markers
	for (AActor* M : HighlightMarkers)
	{
		if (M) M->Destroy();
	}
	HighlightMarkers.Reset();

	if (!ActivePawn || !LevelData) return;
	UWorld* World = GetWorld();
	if (!World) return;

	UStaticMesh* M = HighlightMesh
		? HighlightMesh : LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	const float CS = LevelData->CellSize;

	const TArray<FIntPoint> Valid = ActivePawn->GetValidMoves();
	for (const FIntPoint& Cell : Valid)
	{
		const FVector Loc(Cell.X * CS, Cell.Y * CS, 5.0f);  // 高于地板一点防 z-fighting
		AStaticMeshActor* A = World->SpawnActor<AStaticMeshActor>(Loc, FRotator::ZeroRotator);
		if (!A) continue;
		A->SetMobility(EComponentMobility::Movable);
		if (UStaticMeshComponent* MC = A->GetStaticMeshComponent())
		{
			MC->SetStaticMesh(M);
			MC->SetRelativeScale3D(FVector(CS / 100.0f * 0.85f, CS / 100.0f * 0.85f, 1.0f));
			MC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			MC->SetVectorParameterValueOnMaterials(TEXT("Color"),
				FVector(HighlightColor.R, HighlightColor.G, HighlightColor.B));
		}
		HighlightMarkers.Add(A);
	}
}

void ACh2GameMode::SetUpTopDownCamera()
{
	UWorld* World = GetWorld();
	if (!World || !LevelData) return;

	const float CS = LevelData->CellSize;
	const FVector Center((LevelData->GridWidth - 1) * CS * 0.5f, (LevelData->GridHeight - 1) * CS * 0.5f, 0.0f);

	// 斜俯视相机：从 Center 向 (yaw + 180°) 方向后退 D*cos(pitch) + 上升 D*sin(pitch)
	const float PitchRad = FMath::DegreesToRadians(CameraTiltDegrees);
	const float YawRad = FMath::DegreesToRadians(CameraYawDegrees);
	const float Back = CameraDistance * FMath::Cos(PitchRad);
	const float Up = CameraDistance * FMath::Sin(PitchRad);

	const FVector Offset(-Back * FMath::Cos(YawRad), -Back * FMath::Sin(YawRad), Up);
	const FVector CamLoc = Center + Offset;
	const FRotator CamRot(-CameraTiltDegrees, CameraYawDegrees, 0.0f);

	ACameraActor* Cam = World->SpawnActor<ACameraActor>(CamLoc, CamRot);
	if (Cam)
	{
		Cam->Tags.Add(TEXT("Ch2_TopDownCam"));
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			PC->SetViewTargetWithBlend(Cam, 0.0f);
			PC->bShowMouseCursor = true;
			PC->bEnableClickEvents = true;
			PC->bEnableMouseOverEvents = true;
			FInputModeGameAndUI Mode;
			Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			Mode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(Mode);
		}
	}
}

void ACh2GameMode::NotifyPawnMoved(FIntPoint FromCell, FIntPoint ToCell, bool bWasClownMove)
{
	// 小丑日字跳后在前一 cell 留一个玩偶（如果该 cell 没特殊类型）
	if (bWasClownMove && LevelData && GetCellType(FromCell) == ECh2CellType::Empty)
	{
		SpawnPuppetAt(FromCell);
	}
	// 任何一步都推进所有玩偶倒计时
	TickPuppets();

	// 移动后重算合法 cell 高亮
	RefreshHighlights();
}

void ACh2GameMode::SpawnPuppetAt(FIntPoint Cell)
{
	UWorld* World = GetWorld();
	if (!World || !LevelData) return;

	UStaticMesh* M = PuppetMesh
		? PuppetMesh : LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	const float CS = LevelData->CellSize;
	const FVector Loc(Cell.X * CS, Cell.Y * CS, CS * 0.3f);

	AStaticMeshActor* A = World->SpawnActor<AStaticMeshActor>(Loc, FRotator::ZeroRotator);
	if (!A) return;
	A->SetMobility(EComponentMobility::Movable);
	if (UStaticMeshComponent* MC = A->GetStaticMeshComponent())
	{
		MC->SetStaticMesh(M);
		MC->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.6f));
		MC->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(1.0f, 0.5f, 0.1f));
	}

	FCh2PuppetState P;
	P.Cell = Cell;
	P.TurnsRemaining = PuppetExplodeAfterTurns;
	P.RuntimeActor = A;
	Puppets.Add(P);

	UE_LOG(LogTemp, Display, TEXT("Ch2: 爆炸玩偶 spawn @ (%d,%d), %d 回合后爆炸"),
		Cell.X, Cell.Y, P.TurnsRemaining);
}

void ACh2GameMode::TickPuppets()
{
	for (int32 i = Puppets.Num() - 1; i >= 0; --i)
	{
		Puppets[i].TurnsRemaining--;
		RefreshPuppetVisual(Puppets[i]);
		if (Puppets[i].TurnsRemaining <= 0)
		{
			ExplodePuppet(i);
		}
	}
}

void ACh2GameMode::RefreshPuppetVisual(FCh2PuppetState& P)
{
	if (!P.RuntimeActor) return;
	AStaticMeshActor* A = Cast<AStaticMeshActor>(P.RuntimeActor);
	if (!A) return;
	UStaticMeshComponent* MC = A->GetStaticMeshComponent();
	if (!MC) return;

	// 颜色随倒计时变红（橘 → 红）
	const float T = FMath::Clamp(
		1.0f - static_cast<float>(P.TurnsRemaining) / FMath::Max(1, PuppetExplodeAfterTurns),
		0.0f, 1.0f);
	const FVector Color = FMath::Lerp(FVector(1.0f, 0.5f, 0.1f), FVector(1.0f, 0.05f, 0.05f), T);
	MC->SetVectorParameterValueOnMaterials(TEXT("Color"), Color);
}

void ACh2GameMode::ExplodePuppet(int32 PuppetIdx)
{
	if (!Puppets.IsValidIndex(PuppetIdx) || !LevelData) return;
	FCh2PuppetState& P = Puppets[PuppetIdx];

	UE_LOG(LogTemp, Display, TEXT("Ch2: 爆炸玩偶 BOOM @ (%d,%d)"), P.Cell.X, P.Cell.Y);

	// Flash 占位：在爆炸位 spawn 一颗亮黄白球，0.3s 后自销毁
	if (UWorld* W = GetWorld())
	{
		UStaticMesh* SphereM = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		const FVector FlashLoc(P.Cell.X * LevelData->CellSize, P.Cell.Y * LevelData->CellSize, LevelData->CellSize * 0.5f);
		AStaticMeshActor* Flash = W->SpawnActor<AStaticMeshActor>(FlashLoc, FRotator::ZeroRotator);
		if (Flash)
		{
			Flash->SetMobility(EComponentMobility::Movable);
			if (UStaticMeshComponent* MC = Flash->GetStaticMeshComponent())
			{
				MC->SetStaticMesh(SphereM);
				MC->SetRelativeScale3D(FVector(1.8f));
				MC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				MC->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(2.0f, 1.6f, 0.6f));
			}
			Flash->SetLifeSpan(0.35f);
		}
	}

	// 邻 4 方向：清掉 Destructible
	const FIntPoint Dirs[4] = { {1,0}, {-1,0}, {0,1}, {0,-1} };
	for (const FIntPoint& D : Dirs)
	{
		const FIntPoint N(P.Cell.X + D.X, P.Cell.Y + D.Y);
		if (!IsInBounds(N)) continue;
		if (GetCellType(N) == ECh2CellType::Destructible)
		{
			LevelData->Cells.Remove(N);
			if (AActor** Found = CellActors.Find(N))
			{
				if (*Found) (*Found)->Destroy();
				CellActors.Remove(N);
			}
			UE_LOG(LogTemp, Display, TEXT("Ch2:   炸开 (%d,%d)"), N.X, N.Y);
		}
	}

	// 销毁玩偶 actor + 从列表移除
	if (P.RuntimeActor) P.RuntimeActor->Destroy();
	Puppets.RemoveAt(PuppetIdx);
}

void ACh2GameMode::NotifyPawnEnteredCell(FIntPoint Cell)
{
	if (!LevelData) return;

	const ECh2CellType Type = GetCellType(Cell);

	if (Type == ECh2CellType::ClownPickup)
	{
		if (HUDWidget) HUDWidget->PlayPickupRitual();
		if (ActivePawn)
		{
			ActivePawn->SetMode(ECh2Mode::Clown);
		}
		if (AActor** Found = CellActors.Find(Cell))
		{
			if (*Found) (*Found)->Destroy();
			CellActors.Remove(Cell);
		}
		LevelData->Cells.Remove(Cell);
		UE_LOG(LogTemp, Display, TEXT("Ch2: 拾取小丑残骸 → 切换 Mode::Clown"));
	}
	else if (Type == ECh2CellType::Exit)
	{
		UE_LOG(LogTemp, Display, TEXT("Ch2: 抵达出口 — Ch2 Complete"));
		if (HUDWidget) HUDWidget->ShowVictory();
	}
}
