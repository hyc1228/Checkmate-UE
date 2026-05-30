// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "Ch2GameMode.h"

#include "AudioService.h"
#include "Ch2HUDWidget.h"
#include "Ch2Pawn.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/Scene.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequenceActor.h"
#include "LevelSequencePlayer.h"
#include "MovieSceneSequencePlaybackSettings.h"

ACh2GameMode::ACh2GameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
	DefaultPawnClass = APawn::StaticClass();
}

bool ACh2GameMode::TryPlaySequence(FName Key)
{
	TSoftObjectPtr<ULevelSequence>* SoftPtr = NamedSequences.Find(Key);
	if (!SoftPtr) return false;
	ULevelSequence* Seq = SoftPtr->LoadSynchronous();
	if (!Seq) return false;

	FMovieSceneSequencePlaybackSettings Settings;
	Settings.bAutoPlay = false;
	ALevelSequenceActor* SeqActor = nullptr;
	ULevelSequencePlayer* Player = ULevelSequencePlayer::CreateLevelSequencePlayer(
		GetWorld(), Seq, Settings, SeqActor);
	if (!Player) return false;

	Player->Play();
	UE_LOG(LogTemp, Display, TEXT("Ch2 Sequence: 播放 %s"), *Key.ToString());
	return true;
}

void ACh2GameMode::PV_SetSequencerBakeActive(bool bActive)
{
#if UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Warning, TEXT("PV_CH2_BAKE_MODE ignored in shipping build Requested=%s"), bActive ? TEXT("true") : TEXT("false"));
#else
	bPVSequencerBakeActive = bActive;
	if (bPVSequencerBakeActive)
	{
		ResetFloorColorsToBoardPattern();
	}
	UE_LOG(LogTemp, Display, TEXT("PV_CH2_BAKE_MODE Active=%s"), bPVSequencerBakeActive ? TEXT("true") : TEXT("false"));
#endif
}

void ACh2GameMode::PV_SetMoveBudget(int32 MoveBudget)
{
#if UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Warning, TEXT("PV_CH2_MOVE_BUDGET ignored in shipping build Requested=%d"), MoveBudget);
#else
	RuntimeMoveBudgetOverride = MoveBudget > 0 ? MoveBudget : INDEX_NONE;
	UE_LOG(LogTemp, Display, TEXT("PV_CH2_MOVE_BUDGET Effective=%d Source=%s"),
		GetEffectiveMoveBudget(),
		RuntimeMoveBudgetOverride > 0 ? TEXT("RuntimeOverride") : TEXT("LevelData"));
	if (HUDWidget)
	{
		HUDWidget->SetMoveCounter(MoveCount, GetEffectiveMoveBudget());
	}
#endif
}

void ACh2GameMode::PV_ApplyRecordingDefaults()
{
#if UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Warning, TEXT("PV_CH2_RECORDING_DEFAULTS ignored in shipping build"));
#else
	RuntimeMoveBudgetOverride = 99;
	PuppetExplodeAfterTurns = 2;
	if (bPVSequencerBakeActive)
	{
		ResetFloorColorsToBoardPattern();
	}
	if (HUDWidget)
	{
		HUDWidget->SetMoveCounter(MoveCount, GetEffectiveMoveBudget());
		HUDWidget->SetMoveCounterVisible(!bHideMoveCounterForPVRecording);
	}
	UE_LOG(LogTemp, Display, TEXT("PV_CH2_RECORDING_DEFAULTS MoveBudget=%d PuppetExplodeAfterTurns=%d HideMoveCounter=%s"),
		GetEffectiveMoveBudget(),
		PuppetExplodeAfterTurns,
		bHideMoveCounterForPVRecording ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Display, TEXT("PV_CH2_CONFIG MoveBudget=%d PuppetExplodeAfterTurns=%d BakeMode=%s"),
		GetEffectiveMoveBudget(),
		PuppetExplodeAfterTurns,
		bPVSequencerBakeActive ? TEXT("true") : TEXT("false"));
	PV_LogGoldPath();
#endif
}

void ACh2GameMode::PV_Ch2Preset(FName PresetId)
{
#if UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Warning, TEXT("PV_CH2_PRESET ignored in shipping build Preset=%s"), *PresetId.ToString());
#else
	const FString Preset = PresetId.ToString().ToLower();
	if (Preset == TEXT("bake") || Preset == TEXT("c5") || Preset == TEXT("c7") || Preset == TEXT("c10") || Preset == TEXT("c11"))
	{
		PV_SetSequencerBakeActive(true);
		PV_ApplyRecordingDefaults();
		UE_LOG(LogTemp, Display, TEXT("PV_CH2_PRESET Applied=%s Mode=SequencerBake"), *Preset);
	}
	else if (Preset == TEXT("live"))
	{
		PV_SetSequencerBakeActive(false);
		RuntimeMoveBudgetOverride = INDEX_NONE;
		if (HUDWidget)
		{
			HUDWidget->SetMoveCounterVisible(true);
			HUDWidget->SetMoveCounter(MoveCount, GetEffectiveMoveBudget());
		}
		UE_LOG(LogTemp, Display, TEXT("PV_CH2_PRESET Applied=live MoveBudget=%d PuppetExplodeAfterTurns=%d BakeMode=false"),
			GetEffectiveMoveBudget(),
			PuppetExplodeAfterTurns);
	}
	else
	{
		PV_SetSequencerBakeActive(false);
		PV_ApplyRecordingDefaults();
		UE_LOG(LogTemp, Display, TEXT("PV_CH2_PRESET Applied=%s Mode=PVRecord"), Preset.IsEmpty() ? TEXT("record") : *Preset);
	}
#endif
}

void ACh2GameMode::PV_LogGoldPath()
{
#if UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Warning, TEXT("PV_CH2_GOLD_PATH ignored in shipping build"));
#else
	UE_LOG(LogTemp, Display, TEXT("PV_CH2_GOLD_PATH Start=(5,7) Step1=(5,4) Pickup=Clown Step2=(3,3) Step3=(4,1) Exit"));
	UE_LOG(LogTemp, Display, TEXT("PV_CH2_ANCHORS WeddingWreckage=(3,4),(4,4) ClownPickup=(5,4) Destructible=(5,3) Exit=(4,1)"));
#endif
}

void ACh2GameMode::BeginPlay()
{
	Super::BeginPlay();
	BellRungAtCells.Reset();

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

	UE_LOG(LogTemp, Display, TEXT("PV_CH2_CONFIG MoveBudget=%d PuppetExplodeAfterTurns=%d BakeMode=%s"),
		GetEffectiveMoveBudget(),
		PuppetExplodeAfterTurns,
		bPVSequencerBakeActive ? TEXT("true") : TEXT("false"));

	BuildLevel();
	SetUpTopDownCamera();
	ShowBeatPanel(ECh2BeatPanelMoment::ChapterIntro, BeatPanelHoldSeconds + 0.35f);

	UAudioService::PlayCueStatic(this, FName("Amb.Ch2"));  // 进 Ch2 开 ambient（如 CueTable 里有）
}

ECh2CellType ACh2GameMode::GetCellType(FIntPoint Cell) const
{
	// 优先读 RuntimeCells（运行时副本）；fallback 到 LevelData
	if (const ECh2CellType* T = RuntimeCells.Find(Cell)) return *T;
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

int32 ACh2GameMode::GetEffectiveMoveBudget() const
{
	if (RuntimeMoveBudgetOverride > 0)
	{
		return RuntimeMoveBudgetOverride;
	}
	return LevelData ? LevelData->MoveBudget : 0;
}

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

	// 拷贝 cells 到 runtime 副本（重要：gameplay mutate 不污染源 asset）
	RuntimeCells = LevelData->Cells;

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
		{
			// Exit 是高灯柱 + 强 emissive，远处也能一眼看见
			DecoMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
			DecoScale = FVector(CS / 100.0f * 0.55f, CS / 100.0f * 0.55f, 6.0f);  // 高 600 unit 灯柱
			TintColor = FVector(0.35f, 1.4f, 0.7f);  // 抢眼青绿
			DecoActor->SetActorLocation(FVector(C.X * CS, C.Y * CS, CS * 3.0f));  // 抬到柱中心
			if (EmissiveMaterial) DecoMC->SetMaterial(0, EmissiveMaterial);
			DecoMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ExitActor = DecoActor;
			break;
		}
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
		if (Type == ECh2CellType::Destructible)
		{
			UTextRenderComponent* JiangLabel = NewObject<UTextRenderComponent>(DecoActor);
			if (JiangLabel)
			{
				JiangLabel->SetupAttachment(DecoActor->GetRootComponent());
				JiangLabel->RegisterComponent();
				JiangLabel->SetText(FText::FromString(TEXT("\u5C06")));
				JiangLabel->SetHorizontalAlignment(EHTA_Center);
				JiangLabel->SetVerticalAlignment(EVRTA_TextCenter);
				JiangLabel->SetWorldSize(CS * 0.32f);
				JiangLabel->SetTextRenderColor(FColor(248, 248, 250));
				JiangLabel->SetRelativeLocation(FVector(0.0f, 0.0f, CS * 0.45f));
				JiangLabel->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
			}
		}
		CellActors.Add(C, DecoActor);
	}

	// 3) spawn 玩家 pawn 到 Start cell
	SpawnTraceDecalFallback();

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
				HUDWidget->SetMinimalHUDMode(bUseMinimalHUD);
				if (ActivePawn) HUDWidget->SetMode(ActivePawn->CurrentMode);
				HUDWidget->SetMoveCounter(MoveCount, GetEffectiveMoveBudget());
				if (bHideMoveCounterForPVRecording && GetEffectiveMoveBudget() >= 99)
				{
					HUDWidget->SetMoveCounterVisible(false);
				}

				// 出口方向 hint（屏幕方向相对玩家描述）
				const FIntPoint ExitCell = LevelData->FindCellOfType(ECh2CellType::Exit);
				const FIntPoint StartCellHint = LevelData->FindCellOfType(ECh2CellType::Start);
				if (ExitCell.X >= 0 && StartCellHint.X >= 0)
				{
					const int32 DX = ExitCell.X - StartCellHint.X;
					const int32 DY = ExitCell.Y - StartCellHint.Y;
					FString Dir;
					if (DY > 2)  Dir = TEXT("北");   else if (DY < -2) Dir = TEXT("南");
					if (DX > 2)  Dir += TEXT("东");  else if (DX < -2) Dir += TEXT("西");
					if (Dir.IsEmpty()) Dir = TEXT("中央");
					HUDWidget->SetHintMessage(FString::Printf(TEXT("出口（青绿灯柱）在 %s — 找到爆炸玩偶用法绕过 destructible"), *Dir));
				}
			}
		}
	}

	RefreshHighlights();
}

void ACh2GameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	WorldElapsed += DeltaSeconds;
	UpdateBeatPostProcessCue(DeltaSeconds);

	// Camera shake：sin 振幅衰减
	if (CameraActorRef && ShakeTotal > 0.0f)
	{
		ShakeElapsed += DeltaSeconds;
		if (ShakeElapsed >= ShakeTotal)
		{
			ShakeTotal = 0.0f;
			ActiveShakeMagnitude = 0.0f;
			CameraActorRef->SetActorLocation(CamBaseLoc);
		}
		else
		{
			const float Decay = 1.0f - (ShakeElapsed / ShakeTotal);
			const float Magnitude = ActiveShakeMagnitude > 0.0f ? ActiveShakeMagnitude : ExplosionShakeMagnitude;
			const float OffsetZ = FMath::Sin(ShakeElapsed * 80.0f) * Magnitude * Decay;
			const float OffsetX = FMath::Cos(ShakeElapsed * 65.0f) * Magnitude * 0.6f * Decay;
			CameraActorRef->SetActorLocation(CamBaseLoc + FVector(OffsetX, 0, OffsetZ));
		}
	}

	// sensorium 扩张：出口附近地板渐变成彩色（曼哈顿距离 ≤ ColorRadius）
	const FIntPoint ExitCell = LevelData ? LevelData->FindCellOfType(ECh2CellType::Exit) : FIntPoint(-1, -1);
	if (!bPVSequencerBakeActive && ExitCell.X >= 0)
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

	// 出口 pulse：高灯柱 sin 强呼吸（青绿 ↔ 亮白绿）
	if (!bPVSequencerBakeActive && ExitActor)
	{
		if (AStaticMeshActor* SMA = Cast<AStaticMeshActor>(ExitActor))
		{
			if (UStaticMeshComponent* MC = SMA->GetStaticMeshComponent())
			{
				const float Pulse = 0.5f + 0.5f * FMath::Sin(WorldElapsed * 2.5f);
				const FVector C = FMath::Lerp(
					FVector(0.35f, 1.4f, 0.7f),
					FVector(0.6f, 2.0f, 1.0f),
					Pulse);
				MC->SetVectorParameterValueOnMaterials(TEXT("Color"), C);
				// 同步抖一点亮度（Intensity 参数让 emissive 起伏）
				MC->SetScalarParameterValueOnMaterials(TEXT("Intensity"), 6.0f + Pulse * 4.0f);
			}
			// 柱子轻轻上下浮动（呼吸感）
			FVector L = SMA->GetActorLocation();
			L.Z = LevelData->CellSize * 3.0f + FMath::Sin(WorldElapsed * 1.5f) * 8.0f;
			SMA->SetActorLocation(L);
		}
	}

	// Highlight breathing pulse（Timelie 风：valid cell 软呼吸）
	for (AActor* M : HighlightMarkers)
	{
		if (!M) continue;
		AStaticMeshActor* SMA = Cast<AStaticMeshActor>(M);
		if (!SMA) continue;
		UStaticMeshComponent* MC = SMA->GetStaticMeshComponent();
		if (!MC) continue;
		const float Pulse = 0.6f + 0.4f * FMath::Sin(WorldElapsed * 3.0f);
		const FVector C(HighlightColor.R * (1.5f + Pulse), HighlightColor.G * (1.5f + Pulse), HighlightColor.B * (1.5f + Pulse));
		MC->SetVectorParameterValueOnMaterials(TEXT("Color"), C);
		MC->SetVectorParameterValueOnMaterials(TEXT("Emissive Color"), C);
	}

	// 鼠标悬停预览：跟随光标到合法 cell 时显示更亮 marker
	UpdateHoverMarker();

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

void ACh2GameMode::ResetFloorColorsToBoardPattern()
{
	for (const TPair<FIntPoint, AActor*>& P : FloorActors)
	{
		AStaticMeshActor* SMA = Cast<AStaticMeshActor>(P.Value);
		if (!SMA) continue;
		UStaticMeshComponent* MC = SMA->GetStaticMeshComponent();
		if (!MC) continue;

		const bool bWhite = ((P.Key.X + P.Key.Y) % 2) == 0;
		MC->SetVectorParameterValueOnMaterials(TEXT("Color"),
			bWhite ? FVector(0.85f, 0.85f, 0.85f) : FVector(0.15f, 0.15f, 0.15f));
	}
}

FCh2BeatPanelPayload ACh2GameMode::BuildBeatPanelPayload(ECh2BeatPanelMoment Moment) const
{
	FCh2BeatPanelPayload Payload;
	Payload.Moment = Moment;

	switch (Moment)
	{
	case ECh2BeatPanelMoment::ChapterIntro:
		Payload.Eyebrow = FText::FromString(TEXT("CHAPTER II / WASTE BOARD"));
		Payload.Title = FText::FromString(TEXT("THE GRID WAKES"));
		Payload.Body = FText::FromString(TEXT("Pearl body. Mechanical eye. Follow the gold path."));
		Payload.CueId = FName("Ch2.PanelIntro");
		Payload.bMajorBeat = true;
		break;
	case ECh2BeatPanelMoment::ModeRitual:
		Payload.Eyebrow = FText::FromString(TEXT("RITUAL / BUTTON EYE"));
		Payload.Title = FText::FromString(TEXT("CLOWN MOVE UNLOCKED"));
		Payload.Body = FText::FromString(TEXT("The line breaks into an L."));
		Payload.CueId = FName("Ch2.PanelRitual");
		Payload.bMajorBeat = true;
		break;
	case ECh2BeatPanelMoment::PuppetArmed:
		Payload.Eyebrow = FText::FromString(TEXT("DELAYED BLAST"));
		Payload.Title = FText::FromString(TEXT("PUPPET ARMED"));
		Payload.Body = FText::FromString(TEXT("A button-eye copy waits behind you."));
		Payload.CueId = FName("Ch2.PanelPuppet");
		Payload.bMajorBeat = false;
		break;
	case ECh2BeatPanelMoment::DestructibleBroken:
		Payload.Eyebrow = FText::FromString(TEXT("BREACH"));
		Payload.Title = FText::FromString(TEXT("KING DOOR SPLIT"));
		Payload.Body = FText::FromString(TEXT("The blocked file opens."));
		Payload.CueId = FName("Ch2.PanelBreach");
		Payload.bMajorBeat = true;
		break;
	case ECh2BeatPanelMoment::ExitReached:
		Payload.Eyebrow = FText::FromString(TEXT("EXIT SIGNAL"));
		Payload.Title = FText::FromString(TEXT("COLOR RETURNS"));
		Payload.Body = FText::FromString(TEXT("Leave before the board remembers you."));
		Payload.CueId = FName("Ch2.PanelExit");
		Payload.bMajorBeat = true;
		break;
	}

	return Payload;
}

void ACh2GameMode::ShowBeatPanel(ECh2BeatPanelMoment Moment, float HoldSeconds)
{
	if (bPVSequencerBakeActive)
	{
		return;
	}

	const FCh2BeatPanelPayload Payload = BuildBeatPanelPayload(Moment);
	const float Lifetime = HoldSeconds > 0.0f ? HoldSeconds : BeatPanelHoldSeconds;
	StartBeatPostProcessCue(Payload.bMajorBeat);
	UAudioService::PlayCueStatic(this, Payload.bMajorBeat ? FName("Ch1.PanelMajor") : FName("Ch1.PanelOpen"));

	if (!bShowTextBeatPanels)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC)
	{
		return;
	}

	TSubclassOf<UCh2BeatPanelWidget> PanelClass = BeatPanelWidgetClass;
	if (!PanelClass)
	{
		PanelClass = UCh2BeatPanelWidget::StaticClass();
	}

	if (ActiveBeatPanel)
	{
		ActiveBeatPanel->RemoveFromParent();
		ActiveBeatPanel = nullptr;
	}

	ActiveBeatPanel = CreateWidget<UCh2BeatPanelWidget>(PC, PanelClass);
	if (!ActiveBeatPanel)
	{
		return;
	}

	ActiveBeatPanel->ConfigurePanel(Payload, Lifetime);
	ActiveBeatPanel->AddToViewport(/*ZOrder=*/118);

	FTimerHandle DismissTimer;
	GetWorldTimerManager().SetTimer(DismissTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (ActiveBeatPanel)
		{
			ActiveBeatPanel->StartDismiss();
		}
	}), Lifetime, false);
}

void ACh2GameMode::StartBeatPostProcessCue(bool bMajorBeat)
{
	if (BeatPostProcessDuration <= 0.0f || BeatPostProcessPeak <= 0.0f || bPVSequencerBakeActive)
	{
		return;
	}

	if (!BeatPostProcessVolume)
	{
		BeatPostProcessVolume = GetWorld()->SpawnActor<APostProcessVolume>(
			APostProcessVolume::StaticClass(),
			FVector::ZeroVector,
			FRotator::ZeroRotator);
		if (BeatPostProcessVolume)
		{
			BeatPostProcessVolume->bUnbound = true;
			BeatPostProcessVolume->Priority = 32.0f;
			BeatPostProcessVolume->BlendWeight = 0.0f;
		}
	}

	BeatPostProcessElapsed = 0.0f;
	bBeatPostProcessActive = BeatPostProcessVolume != nullptr;
	bBeatPostProcessMajor = bMajorBeat;
}

void ACh2GameMode::UpdateBeatPostProcessCue(float DeltaSeconds)
{
	if (!bBeatPostProcessActive || !BeatPostProcessVolume)
	{
		return;
	}

	BeatPostProcessElapsed += DeltaSeconds;
	const float T = FMath::Clamp(BeatPostProcessElapsed / FMath::Max(0.05f, BeatPostProcessDuration), 0.0f, 1.0f);
	const float Pulse = FMath::Sin(T * PI) * BeatPostProcessPeak * (bBeatPostProcessMajor ? 1.0f : 0.55f);

	FPostProcessSettings& PP = BeatPostProcessVolume->Settings;
	BeatPostProcessVolume->BlendWeight = FMath::Clamp(Pulse, 0.0f, 1.0f);
	PP.bOverride_VignetteIntensity = true;
	PP.bOverride_SceneFringeIntensity = true;
	PP.bOverride_BloomIntensity = true;
	PP.bOverride_AutoExposureBias = true;
	PP.VignetteIntensity = 0.22f + Pulse * (bBeatPostProcessMajor ? 0.38f : 0.18f);
	PP.SceneFringeIntensity = Pulse * (bBeatPostProcessMajor ? 1.8f : 0.75f);
	PP.BloomIntensity = 0.28f + Pulse * (bBeatPostProcessMajor ? 1.05f : 0.45f);
	PP.AutoExposureBias = Pulse * (bBeatPostProcessMajor ? 0.18f : 0.08f);

	if (BeatPostProcessElapsed >= BeatPostProcessDuration)
	{
		BeatPostProcessVolume->BlendWeight = 0.0f;
		bBeatPostProcessActive = false;
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

	// 高亮用 cube（有厚度，避免和地板 plane 重叠 z-fight；emissive tint 醒目）
	UStaticMesh* M = HighlightMesh
		? HighlightMesh : LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	const float CS = LevelData->CellSize;

	const TArray<FIntPoint> Valid = ActivePawn->GetValidMoves();
	for (const FIntPoint& Cell : Valid)
	{
		const FVector Loc(Cell.X * CS, Cell.Y * CS, 15.0f);  // 抬高一些
		AStaticMeshActor* A = World->SpawnActor<AStaticMeshActor>(Loc, FRotator::ZeroRotator);
		if (!A) continue;
		A->SetMobility(EComponentMobility::Movable);
		if (UStaticMeshComponent* MC = A->GetStaticMeshComponent())
		{
			MC->SetStaticMesh(M);
			// 厚度 0.15 = 15unit，可见的薄板
			MC->SetRelativeScale3D(FVector(CS / 100.0f * 0.85f, CS / 100.0f * 0.85f, 0.15f));
			MC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			// 配了 M_Highlight 就 swap，参数 "Color" 直接控发光
			if (EmissiveMaterial)
			{
				MC->SetMaterial(0, EmissiveMaterial);
			}
			const FVector EmColor(HighlightColor.R, HighlightColor.G, HighlightColor.B);
			MC->SetVectorParameterValueOnMaterials(TEXT("Color"), EmColor);
			MC->SetVectorParameterValueOnMaterials(TEXT("Emissive Color"), EmColor);
			MC->SetVectorParameterValueOnMaterials(TEXT("BaseColor"), EmColor);
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
		CameraActorRef = Cam;
		CamBaseLoc = CamLoc;
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

void ACh2GameMode::RestartCh2Level()
{
	UE_LOG(LogTemp, Display, TEXT("Ch2: 步数超预算 — 关卡重启"));
	UAudioService::PlayCueStatic(this, FName("Ch1.Wrong"));
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

void ACh2GameMode::NotifyPawnMoved(FIntPoint FromCell, FIntPoint ToCell, bool bWasClownMove)
{
	SpawnCellPulse(ToCell, bWasClownMove ? FVector(1.45f, 0.85f, 0.2f) : FVector(0.75f, 0.95f, 1.4f), bWasClownMove ? 1.55f : 1.25f, 0.28f);

	// 小丑日字跳后在前一 cell 留一个玩偶（如果该 cell 没特殊类型）
	if (bWasClownMove && LevelData && GetCellType(FromCell) == ECh2CellType::Empty)
	{
		SpawnCellPulse(FromCell, FVector(1.0f, 0.35f, 0.08f), 1.2f, 0.26f);
		SpawnPuppetAt(FromCell);
	}
	// 任何一步都推进所有玩偶倒计时
	TickPuppets();

	// 移动后重算合法 cell 高亮
	RefreshHighlights();

	// 步数预算检查（关卡 MoveBudget > 0 时启用）
	MoveCount++;
	const int32 EffectiveMoveBudget = GetEffectiveMoveBudget();
	if (HUDWidget) HUDWidget->SetMoveCounter(MoveCount, EffectiveMoveBudget);
	if (EffectiveMoveBudget > 0 && MoveCount > EffectiveMoveBudget)
	{
		// 短延迟后重启（让玩家看清楚最后一步落点）
		FTimerHandle FailTimer;
		GetWorld()->GetTimerManager().SetTimer(FailTimer,
			FTimerDelegate::CreateUObject(this, &ACh2GameMode::RestartCh2Level),
			0.6f, false);
	}
}

void ACh2GameMode::UpdateHoverMarker()
{
	UWorld* World = GetWorld();
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!World || !PC || !LevelData) return;

	FVector WorldOrigin, WorldDir;
	if (!PC->DeprojectMousePositionToWorld(WorldOrigin, WorldDir)) return;
	if (FMath::Abs(WorldDir.Z) < KINDA_SMALL_NUMBER) return;

	const float Tval = -WorldOrigin.Z / WorldDir.Z;
	if (Tval <= 0.0f) return;
	const FVector HitWorld = WorldOrigin + WorldDir * Tval;
	const FIntPoint HoverCell(
		FMath::RoundToInt(HitWorld.X / LevelData->CellSize),
		FMath::RoundToInt(HitWorld.Y / LevelData->CellSize));

	if (!IsInBounds(HoverCell))
	{
		if (HoverMarker) HoverMarker->SetActorHiddenInGame(true);
		return;
	}

	// Lazy spawn
	if (!HoverMarker)
	{
		UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		AStaticMeshActor* A = World->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator);
		if (A)
		{
			A->SetMobility(EComponentMobility::Movable);
			if (UStaticMeshComponent* MC = A->GetStaticMeshComponent())
			{
				MC->SetStaticMesh(M);
				MC->SetRelativeScale3D(FVector(LevelData->CellSize / 100.0f * 0.93f,
					LevelData->CellSize / 100.0f * 0.93f, 0.05f));
				MC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
			HoverMarker = A;
		}
	}
	if (!HoverMarker) return;

	HoverMarker->SetActorHiddenInGame(false);
	HoverMarker->SetActorLocation(FVector(
		HoverCell.X * LevelData->CellSize,
		HoverCell.Y * LevelData->CellSize,
		8.0f));

	// 区分：合法 → 亮白；非法 → 暗红
	const bool bValid = ActivePawn ? ActivePawn->GetValidMoves().Contains(HoverCell) : false;
	if (AStaticMeshActor* SMA = Cast<AStaticMeshActor>(HoverMarker))
	{
		if (UStaticMeshComponent* MC = SMA->GetStaticMeshComponent())
		{
			const FVector C = bValid ? FVector(1.5f, 1.5f, 1.6f) : FVector(0.4f, 0.1f, 0.12f);
			MC->SetVectorParameterValueOnMaterials(TEXT("Color"), C);
			MC->SetVectorParameterValueOnMaterials(TEXT("Emissive Color"), C);
		}
	}

	// Ghost pawn 预览（仅 valid cell）
	if (bValid)
	{
		if (!GhostPawn)
		{
			UStaticMesh* CubeM = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
			AStaticMeshActor* G = World->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator);
			if (G)
			{
				G->SetMobility(EComponentMobility::Movable);
				if (UStaticMeshComponent* MC = G->GetStaticMeshComponent())
				{
					MC->SetStaticMesh(CubeM);
					MC->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.9f));
					MC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					if (EmissiveMaterial) MC->SetMaterial(0, EmissiveMaterial);
					MC->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.85f, 0.85f, 0.95f));
					MC->SetScalarParameterValueOnMaterials(TEXT("Intensity"), 2.5f);
				}
				GhostPawn = G;
			}
		}
		if (GhostPawn)
		{
			GhostPawn->SetActorHiddenInGame(false);
			const float Pulse = 0.7f + 0.3f * FMath::Sin(WorldElapsed * 5.0f);
			GhostPawn->SetActorLocation(FVector(
				HoverCell.X * LevelData->CellSize,
				HoverCell.Y * LevelData->CellSize,
				LevelData->CellSize * 0.5f + Pulse * 5.0f));
		}
	}
	else if (GhostPawn)
	{
		GhostPawn->SetActorHiddenInGame(true);
	}

	// 路径预览
	UpdatePathPreview(HoverCell, bValid);
}

void ACh2GameMode::UpdatePathPreview(FIntPoint HoverCell, bool bValid)
{
	ClearPathPreview();
	if (!bValid || !ActivePawn || !LevelData) return;

	UWorld* World = GetWorld();
	if (!World) return;

	UStaticMesh* DotM = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	const float CS = LevelData->CellSize;
	const FIntPoint From = ActivePawn->GetCurrentCell();

	// 收集路径点
	TArray<FVector> Points;
	if (ActivePawn->CurrentMode == ECh2Mode::Ballet)
	{
		// 直线均匀采样
		const int32 Steps = FMath::Max(FMath::Abs(HoverCell.X - From.X), FMath::Abs(HoverCell.Y - From.Y));
		for (int32 i = 1; i < Steps; ++i)
		{
			const float T = static_cast<float>(i) / Steps;
			Points.Add(FVector(
				FMath::Lerp((float)From.X, (float)HoverCell.X, T) * CS,
				FMath::Lerp((float)From.Y, (float)HoverCell.Y, T) * CS,
				12.0f));
		}
	}
	else
	{
		// L-跳：一个 mid 点（沿 L 的拐角）+ 抛物线高度
		const int32 Dx = HoverCell.X - From.X;
		const int32 Dy = HoverCell.Y - From.Y;
		const bool bLongOnX = FMath::Abs(Dx) > FMath::Abs(Dy);
		const FIntPoint Mid = bLongOnX
			? FIntPoint(HoverCell.X, From.Y)
			: FIntPoint(From.X, HoverCell.Y);
		// 4 个点 + 抛物线高度
		for (int32 i = 1; i < 5; ++i)
		{
			const float T = static_cast<float>(i) / 5.0f;
			const FVector A(From.X * CS, From.Y * CS, 12.0f);
			const FVector M(Mid.X * CS, Mid.Y * CS, 12.0f);
			const FVector B(HoverCell.X * CS, HoverCell.Y * CS, 12.0f);
			// quadratic bezier through A→M→B
			const float Inv = 1.0f - T;
			FVector P = Inv * Inv * A + 2.0f * Inv * T * M + T * T * B;
			P.Z += FMath::Sin(T * PI) * 60.0f;
			Points.Add(P);
		}
	}

	// Spawn dot actors
	for (const FVector& P : Points)
	{
		AStaticMeshActor* D = World->SpawnActor<AStaticMeshActor>(P, FRotator::ZeroRotator);
		if (!D) continue;
		D->SetMobility(EComponentMobility::Movable);
		if (UStaticMeshComponent* MC = D->GetStaticMeshComponent())
		{
			MC->SetStaticMesh(DotM);
			MC->SetRelativeScale3D(FVector(0.15f));
			MC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			if (EmissiveMaterial) MC->SetMaterial(0, EmissiveMaterial);
			MC->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(HighlightColor.R, HighlightColor.G, HighlightColor.B));
			MC->SetScalarParameterValueOnMaterials(TEXT("Intensity"), 5.0f);
		}
		PathDots.Add(D);
	}
}

void ACh2GameMode::ClearPathPreview()
{
	for (AActor* A : PathDots) { if (A) A->Destroy(); }
	PathDots.Reset();
}

void ACh2GameMode::SpawnClickRipple(const FVector& WorldPos)
{
	UWorld* World = GetWorld();
	if (!World) return;
	UStaticMesh* CubeM = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	AStaticMeshActor* R = World->SpawnActor<AStaticMeshActor>(WorldPos + FVector(0, 0, 10), FRotator::ZeroRotator);
	if (!R) return;
	R->SetMobility(EComponentMobility::Movable);
	if (UStaticMeshComponent* MC = R->GetStaticMeshComponent())
	{
		MC->SetStaticMesh(CubeM);
		MC->SetRelativeScale3D(FVector(2.5f, 2.5f, 0.03f));
		MC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (EmissiveMaterial) MC->SetMaterial(0, EmissiveMaterial);
		MC->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(HighlightColor.R, HighlightColor.G, HighlightColor.B));
		MC->SetScalarParameterValueOnMaterials(TEXT("Intensity"), 8.0f);
	}
	R->SetLifeSpan(0.4f);
}

void ACh2GameMode::TriggerCameraKick(float Magnitude, float Duration)
{
	if (bPVSequencerBakeActive)
	{
		return;
	}
	ShakeElapsed = 0.0f;
	ShakeTotal = FMath::Max(ShakeTotal, Duration);
	ActiveShakeMagnitude = FMath::Max(ActiveShakeMagnitude, Magnitude);
}

void ACh2GameMode::SpawnJuiceMarker(const FVector& WorldPos, const FVector& Color, float UniformScale, float LifeSpan, bool bSphere)
{
	UWorld* World = GetWorld();
	if (!World) return;

	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, bSphere
		? TEXT("/Engine/BasicShapes/Sphere.Sphere")
		: TEXT("/Engine/BasicShapes/Cube.Cube"));
	AStaticMeshActor* Marker = World->SpawnActor<AStaticMeshActor>(WorldPos, FRotator::ZeroRotator);
	if (!Marker) return;

	Marker->SetMobility(EComponentMobility::Movable);
	if (UStaticMeshComponent* MC = Marker->GetStaticMeshComponent())
	{
		MC->SetStaticMesh(Mesh);
		MC->SetRelativeScale3D(FVector(UniformScale));
		MC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (EmissiveMaterial)
		{
			MC->SetMaterial(0, EmissiveMaterial);
		}
		MC->SetVectorParameterValueOnMaterials(TEXT("Color"), Color);
		MC->SetVectorParameterValueOnMaterials(TEXT("Emissive Color"), Color);
		MC->SetScalarParameterValueOnMaterials(TEXT("Intensity"), 8.0f);
	}
	Marker->SetLifeSpan(LifeSpan);
}

void ACh2GameMode::SpawnCellPulse(FIntPoint Cell, const FVector& Color, float RadiusScale, float LifeSpan)
{
	if (!LevelData) return;
	const FVector Center(Cell.X * LevelData->CellSize, Cell.Y * LevelData->CellSize, 18.0f);
	SpawnJuiceMarker(Center, Color, RadiusScale, LifeSpan, false);
}

void ACh2GameMode::SpawnTraceDecalFallback()
{
	if (!bSpawnTraceDecalFallback || !LevelData)
	{
		return;
	}

	const FIntPoint TraceCell(3, 3);
	if (!IsInBounds(TraceCell))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float CS = LevelData->CellSize;
	AActor* TraceActor = World->SpawnActor<AActor>(
		FVector(TraceCell.X * CS, TraceCell.Y * CS, 8.0f),
		FRotator::ZeroRotator);
	if (!TraceActor)
	{
		return;
	}

	USceneComponent* Root = NewObject<USceneComponent>(TraceActor);
	TraceActor->SetRootComponent(Root);
	Root->RegisterComponent();
	TraceActor->Tags.Add(TEXT("Ch2_TraceDecalFallback"));

	UTextRenderComponent* TraceText = NewObject<UTextRenderComponent>(TraceActor);
	if (!TraceText)
	{
		return;
	}

	TraceText->SetupAttachment(Root);
	TraceText->RegisterComponent();
	TraceText->SetText(FText::FromString(TEXT("I  V  X")));
	TraceText->SetHorizontalAlignment(EHTA_Center);
	TraceText->SetVerticalAlignment(EVRTA_TextCenter);
	TraceText->SetWorldSize(CS * 0.18f);
	TraceText->SetTextRenderColor(FColor(218, 218, 224));
	TraceText->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	TraceText->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));

	UE_LOG(LogTemp, Display, TEXT("CH2_TRACE_DECAL_FALLBACK Cell=(%d,%d)"), TraceCell.X, TraceCell.Y);
}

void ACh2GameMode::SpawnJiangDebrisBurst(FIntPoint Cell)
{
	if (!LevelData)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float CS = LevelData->CellSize;
	const FVector Center(Cell.X * CS, Cell.Y * CS, CS * 0.62f);
	for (int32 i = 0; i < 6; ++i)
	{
		const float Angle = (static_cast<float>(i) / 6.0f) * 360.0f + FMath::FRandRange(-12.0f, 12.0f);
		const float Radius = FMath::FRandRange(CS * 0.08f, CS * 0.38f);
		const FVector Offset(FMath::Cos(FMath::DegreesToRadians(Angle)) * Radius,
			FMath::Sin(FMath::DegreesToRadians(Angle)) * Radius,
			FMath::FRandRange(-CS * 0.06f, CS * 0.16f));

		AActor* Fragment = World->SpawnActor<AActor>(
			Center + Offset,
			FRotator(0.0f, Angle, FMath::FRandRange(-28.0f, 28.0f)));
		if (!Fragment)
		{
			continue;
		}

		USceneComponent* Root = NewObject<USceneComponent>(Fragment);
		Fragment->SetRootComponent(Root);
		Root->RegisterComponent();

		UTextRenderComponent* Text = NewObject<UTextRenderComponent>(Fragment);
		if (!Text)
		{
			Fragment->Destroy();
			continue;
		}

		Text->SetupAttachment(Root);
		Text->RegisterComponent();
		Text->SetText(FText::FromString(TEXT("\u5C06")));
		Text->SetHorizontalAlignment(EHTA_Center);
		Text->SetVerticalAlignment(EVRTA_TextCenter);
		Text->SetWorldSize(CS * FMath::FRandRange(0.12f, 0.2f));
		Text->SetTextRenderColor(FColor(248, 248, 250));
		Text->SetRelativeRotation(FRotator(75.0f, 0.0f, FMath::FRandRange(-24.0f, 24.0f)));

		Fragment->SetLifeSpan(FMath::FRandRange(0.45f, 0.8f));
	}
}

void ACh2GameMode::NotifyMoveCommitted(FIntPoint TargetCell)
{
	if (!LevelData) return;
	UAudioService::PlayCueStatic(this, FName("Ch2.Move"));
	const FVector P(TargetCell.X * LevelData->CellSize, TargetCell.Y * LevelData->CellSize, 5.0f);
	SpawnClickRipple(P);
	SpawnCellPulse(TargetCell, FVector(0.3f, 1.25f, 1.6f), 1.15f, 0.22f);
	ClearPathPreview();
	if (GhostPawn) GhostPawn->SetActorHiddenInGame(true);
}

void ACh2GameMode::NotifyInvalidMove(FIntPoint TargetCell)
{
	if (!LevelData || !IsInBounds(TargetCell)) return;
	UAudioService::PlayCueStatic(this, FName("Ch2.Invalid"), 0.65f);
	SpawnCellPulse(TargetCell, FVector(1.4f, 0.05f, 0.08f), 1.1f, 0.22f);
	TriggerCameraKick(3.5f, 0.12f);
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

	// 加按扣眼 marker（spec：爆炸玩偶眼睛 = 按扣眼 = 未觉醒的自己）
	// 在玩偶球顶上 attach 一个小立方体，暗色
	UStaticMesh* ButtonM = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (ButtonM)
	{
		UStaticMeshComponent* EyeBtn = NewObject<UStaticMeshComponent>(A);
		EyeBtn->SetupAttachment(A->GetRootComponent());
		EyeBtn->RegisterComponent();
		EyeBtn->SetStaticMesh(ButtonM);
		EyeBtn->SetRelativeLocation(FVector(40.0f, 0.0f, 20.0f));
		EyeBtn->SetRelativeScale3D(FVector(0.15f, 0.15f, 0.15f));
		EyeBtn->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		EyeBtn->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.05f, 0.05f, 0.08f));
	}

	FCh2PuppetState P;
	P.Cell = Cell;
	P.TurnsRemaining = PuppetExplodeAfterTurns;
	P.RuntimeActor = A;
	Puppets.Add(P);
	SpawnCellPulse(Cell, FVector(1.4f, 0.45f, 0.08f), 1.45f, 0.32f);
	SpawnJuiceMarker(Loc + FVector(0.0f, 0.0f, CS * 0.45f), FVector(1.5f, 0.55f, 0.1f), 0.45f, 0.38f);
	TriggerCameraKick(2.0f, 0.10f);
	ShowBeatPanel(ECh2BeatPanelMoment::PuppetArmed, 0.95f);
	UE_LOG(LogTemp, Display, TEXT("CH2_PUPPET_SPAWN Cell=(%d,%d) TurnsRemaining=%d ActivePuppets=%d"),
		Cell.X, Cell.Y, P.TurnsRemaining, Puppets.Num());

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
		else
		{
			UE_LOG(LogTemp, Display, TEXT("CH2_PUPPET_TICK Cell=(%d,%d) TurnsRemaining=%d"),
				Puppets[i].Cell.X, Puppets[i].Cell.Y, Puppets[i].TurnsRemaining);
			UAudioService::PlayCueStatic(this, FName("Ch2.PuppetTick"), 0.7f);
			SpawnCellPulse(Puppets[i].Cell, FVector(1.2f, 0.12f, 0.08f), 0.85f, 0.18f);
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

	UE_LOG(LogTemp, Display, TEXT("CH2_PUPPET_BOOM Cell=(%d,%d)"), P.Cell.X, P.Cell.Y);
	UE_LOG(LogTemp, Display, TEXT("Ch2: 爆炸玩偶 BOOM @ (%d,%d)"), P.Cell.X, P.Cell.Y);

	UAudioService::PlayCueStatic(this, FName("Ch2.Explode"));
	SpawnCellPulse(P.Cell, FVector(2.0f, 1.35f, 0.25f), 2.6f, 0.42f);
	SpawnJuiceMarker(FVector(P.Cell.X * LevelData->CellSize, P.Cell.Y * LevelData->CellSize, LevelData->CellSize * 0.7f), FVector(2.0f, 1.6f, 0.6f), 1.2f, 0.32f);

	// 优先播 Ch2.Explosion sequence；总是叠加 camera shake + flash 球（即时反馈）
	TryPlaySequence(TEXT("Ch2.Explosion"));
	ShakeElapsed = 0.0f;
	ShakeTotal = ExplosionShakeDuration;
	ActiveShakeMagnitude = ExplosionShakeMagnitude;

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
			RuntimeCells.Remove(N);
			if (AActor** Found = CellActors.Find(N))
			{
				if (*Found) (*Found)->Destroy();
				CellActors.Remove(N);
			}
			SpawnCellPulse(N, FVector(1.8f, 1.25f, 0.45f), 1.75f, 0.35f);
			SpawnJuiceMarker(FVector(N.X * LevelData->CellSize, N.Y * LevelData->CellSize, LevelData->CellSize * 0.55f), FVector(1.6f, 1.0f, 0.35f), 0.55f, 0.5f);
			SpawnJiangDebrisBurst(N);
			ShowBeatPanel(ECh2BeatPanelMoment::DestructibleBroken, 1.2f);
			UE_LOG(LogTemp, Display, TEXT("CH2_DESTRUCTIBLE_REMOVED Cell=(%d,%d) TriggerPuppet=(%d,%d)"),
				N.X, N.Y, P.Cell.X, P.Cell.Y);
			UE_LOG(LogTemp, Display, TEXT("Ch2:   炸开 (%d,%d)"), N.X, N.Y);
		}
	}

	// 销毁玩偶 actor + 从列表移除
	if (P.RuntimeActor) P.RuntimeActor->Destroy();
	Puppets.RemoveAt(PuppetIdx);

	// bug fix：炸完高亮要重算（破坏的 cell 现在可通行）
	RefreshHighlights();
}

void ACh2GameMode::NotifyPawnEnteredCell(FIntPoint Cell)
{
	if (!LevelData) return;

	// 婚纱铃场：玩家停在 WeddingWreckage 4-邻格时响一次（每格仅一次）。
	// spec ch2-slice-space.md §5 锚点 1：接不上的铃 = 一响即止。
	if (!BellRungAtCells.Contains(Cell))
	{
		const FIntPoint NDirs[4] = { {1,0}, {-1,0}, {0,1}, {0,-1} };
		for (const FIntPoint& D : NDirs)
		{
			if (GetCellType(FIntPoint(Cell.X + D.X, Cell.Y + D.Y)) == ECh2CellType::WeddingWreckage)
			{
				UE_LOG(LogTemp, Display, TEXT("SC_Ch2.Bell Cell=(%d,%d) WeddingNeighbor=(%d,%d)"),
					Cell.X, Cell.Y, Cell.X + D.X, Cell.Y + D.Y);
				UAudioService::PlayCueStatic(this, FName("Ch2.Bell"));
				SpawnCellPulse(Cell, FVector(1.25f, 1.25f, 1.45f), 1.45f, 0.42f);
				SpawnCellPulse(FIntPoint(Cell.X + D.X, Cell.Y + D.Y), FVector(1.1f, 1.1f, 1.35f), 1.15f, 0.42f);
				BellRungAtCells.Add(Cell);
				break;
			}
		}
	}

	const ECh2CellType Type = GetCellType(Cell);

	if (Type == ECh2CellType::ClownPickup)
	{
		UAudioService::PlayCueStatic(this, FName("Ch2.Ritual"));
		SpawnCellPulse(Cell, FVector(1.7f, 1.0f, 0.2f), 2.3f, 0.55f);
		SpawnJuiceMarker(FVector(Cell.X * LevelData->CellSize, Cell.Y * LevelData->CellSize, LevelData->CellSize * 0.95f), FVector(0.3f, 1.4f, 1.8f), 0.9f, 0.55f);
		TriggerCameraKick(5.0f, 0.20f);
		ShowBeatPanel(ECh2BeatPanelMoment::ModeRitual, 1.25f);
		// 优先播 Ch2.Pickup sequence；fallback：UMG fade + close-up glimpse
		const bool bPlayedSeq = TryPlaySequence(TEXT("Ch2.Pickup"));
		if (!bPlayedSeq)
		{
			if (HUDWidget) HUDWidget->PlayPickupRitual();
			PlayRitualGlimpse();
		}
		// SetMode 留给 timer 内 RestorePearl 阶段做（spec §3 缝扣后切 Clown 走法）
		// 这里 deferred 是为了 glimpse 时眼睛先回 Pearl 再 SetMode 切 Clown 才显得 ritual
		if (AActor** Found = CellActors.Find(Cell))
		{
			if (*Found) (*Found)->Destroy();
			CellActors.Remove(Cell);
		}
		RuntimeCells.Remove(Cell);
		UE_LOG(LogTemp, Display, TEXT("Ch2: 拾取小丑残骸 → 切换 Mode::Clown"));
	}
	else if (Type == ECh2CellType::Exit)
	{
		UE_LOG(LogTemp, Display, TEXT("Ch2: 抵达出口 — Ch2 Complete"));
		if (bPVSequencerBakeActive)
		{
			UE_LOG(LogTemp, Display, TEXT("PV_CH2_BAKE_SUPPRESS VictoryUIAudio"));
			return;
		}
		if (ActivePawn)
		{
			ActivePawn->RevealHumanForm();
		}
		UAudioService::PlayCueStatic(this, FName("Ch2.Victory"));
		SpawnCellPulse(Cell, FVector(0.35f, 2.0f, 0.8f), 2.8f, 0.7f);
		SpawnJuiceMarker(FVector(Cell.X * LevelData->CellSize, Cell.Y * LevelData->CellSize, LevelData->CellSize * 1.2f), FVector(0.45f, 2.0f, 1.0f), 1.1f, 0.8f);
		TriggerCameraKick(4.0f, 0.25f);
		ShowBeatPanel(ECh2BeatPanelMoment::ExitReached, 1.45f);
		// 优先播 Ch2.Victory sequence；fallback UMG victory screen
		if (!TryPlaySequence(TEXT("Ch2.Victory")) && HUDWidget)
		{
			HUDWidget->ShowVictory();
		}
	}
}

void ACh2GameMode::PlayRitualGlimpse()
{
	UWorld* World = GetWorld();
	if (!World || !ActivePawn) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	// 1) Spawn close-up camera near pawn 头顶斜前方
	const FVector PawnLoc = ActivePawn->GetActorLocation();
	const FVector CamLoc = PawnLoc + FVector(-180.0f, 0.0f, 220.0f);
	const FRotator CamRot = (PawnLoc - CamLoc).Rotation();

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	RitualCloseUpCam = World->SpawnActor<ACameraActor>(CamLoc, CamRot, SP);
	if (RitualCloseUpCam)
	{
		PC->SetViewTargetWithBlend(RitualCloseUpCam, 0.25f, EViewTargetBlendFunction::VTBlend_Cubic);
	}

	auto& TM = World->GetTimerManager();

	// T=0.0：取扣（藏 EyeMarker）
	if (ActivePawn->EyeMarker) ActivePawn->EyeMarker->SetVisibility(false);

	// T=0.45：Glimpse — show 机械眼一闪
	TM.SetTimer(RitualTimer_ShowMechanical, FTimerDelegate::CreateLambda([this]() {
		if (!ActivePawn || !ActivePawn->EyeMarker) return;
		ActivePawn->EyeMarker->SetVisibility(true);
		if (ActivePawn->EyeMaterial_Mechanical)
		{
			ActivePawn->EyeMarker->SetMaterial(0, ActivePawn->EyeMaterial_Mechanical);
		}
	}), 0.45f, false);

	// T=0.85：缝扣 — 切回 Pearl 暖白；同步 SetMode(Clown) 让走法切换
	TM.SetTimer(RitualTimer_RestorePearl, FTimerDelegate::CreateLambda([this]() {
		if (!ActivePawn) return;
		if (ActivePawn->EyeMarker && ActivePawn->EyeMaterial_Pearl)
		{
			ActivePawn->EyeMarker->SetMaterial(0, ActivePawn->EyeMaterial_Pearl);
		}
		ActivePawn->SetMode(ECh2Mode::Clown);
	}), 0.85f, false);

	// T=1.4：blend 回俯视 camera
	TM.SetTimer(RitualTimer_ReturnCam, FTimerDelegate::CreateLambda([this, PC]() {
		if (CameraActorRef && PC)
		{
			PC->SetViewTargetWithBlend(CameraActorRef, 0.35f, EViewTargetBlendFunction::VTBlend_Cubic);
		}
		if (RitualCloseUpCam)
		{
			RitualCloseUpCam->Destroy();
			RitualCloseUpCam = nullptr;
		}
	}), 1.4f, false);
}
