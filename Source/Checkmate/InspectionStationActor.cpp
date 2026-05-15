// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "InspectionStationActor.h"
#include "SpawnedDoll.h"
#include "DollData.h"
#include "CheckmateGameStateSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AInspectionStationActor::AInspectionStationActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	DeskMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DeskMesh"));
	DeskMesh->SetupAttachment(RootScene);
	DeskMesh->SetRelativeScale3D(FVector(3.0f, 1.5f, 0.1f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded()) { DeskMesh->SetStaticMesh(CubeFinder.Object); }

	ConveyorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ConveyorMesh"));
	ConveyorMesh->SetupAttachment(RootScene);
	ConveyorMesh->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
	ConveyorMesh->SetRelativeScale3D(FVector(5.0f, 0.8f, 0.05f));
	if (CubeFinder.Succeeded()) { ConveyorMesh->SetStaticMesh(CubeFinder.Object); }

	SpawnAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("SpawnAnchor"));
	SpawnAnchor->SetupAttachment(RootScene);
	SpawnAnchor->SetRelativeLocation(FVector(250.f, 0.f, 60.f));

	InspectionAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("InspectionAnchor"));
	InspectionAnchor->SetupAttachment(RootScene);
	InspectionAnchor->SetRelativeLocation(FVector(0.f, 0.f, 60.f));

	ApproveExitAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("ApproveExitAnchor"));
	ApproveExitAnchor->SetupAttachment(RootScene);
	ApproveExitAnchor->SetRelativeLocation(FVector(-250.f, 0.f, 60.f));

	RejectChuteAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("RejectChuteAnchor"));
	RejectChuteAnchor->SetupAttachment(RootScene);
	RejectChuteAnchor->SetRelativeLocation(FVector(0.f, 80.f, -100.f));

	StandardTrayAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("StandardTrayAnchor"));
	StandardTrayAnchor->SetupAttachment(RootScene);
	StandardTrayAnchor->SetRelativeLocation(FVector(0.f, -120.f, 10.f));

	ApprovalStamp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ApprovalStamp"));
	ApprovalStamp->SetupAttachment(RootScene);
	ApprovalStamp->SetRelativeLocation(FVector(120.f, 60.f, 15.f));
	ApprovalStamp->SetRelativeScale3D(FVector(0.3f, 0.3f, 0.4f));
	if (CubeFinder.Succeeded()) { ApprovalStamp->SetStaticMesh(CubeFinder.Object); }

	RejectChain = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RejectChain"));
	RejectChain->SetupAttachment(RootScene);
	RejectChain->SetRelativeLocation(FVector(120.f, -60.f, 15.f));
	RejectChain->SetRelativeScale3D(FVector(0.3f, 0.3f, 0.5f));
	if (CubeFinder.Succeeded()) { RejectChain->SetStaticMesh(CubeFinder.Object); }

	DollActorClass = ASpawnedDoll::StaticClass();
}

void AInspectionStationActor::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UCheckmateGameStateSubsystem* GS = GI->GetSubsystem<UCheckmateGameStateSubsystem>())
			{
				GS->OnShiftStarted.AddDynamic(this, &AInspectionStationActor::BeginShift);
			}
		}
	}
}

void AInspectionStationActor::BeginShift(int32 ShiftIndex)
{
	const FShiftConfig* Cfg = GetConfigForShift(ShiftIndex);
	if (!Cfg)
	{
		UE_LOG(LogTemp, Warning, TEXT("InspectionStation: 找不到 Shift %d 的配置。检查 ShiftConfigs 数组。"), ShiftIndex);
		return;
	}

	ActiveShiftIndex = ShiftIndex;
	NextDollInQueue = 0;
	DollTimeoutSecCached = Cfg->DollTimeoutSec;

	UE_LOG(LogTemp, Log, TEXT("InspectionStation: Shift %d 开始，娃娃队列 %d 只。"),
		ShiftIndex, Cfg->DollQueue.Num());

	SpawnNextDoll();
}

void AInspectionStationActor::SpawnNextDoll()
{
	const FShiftConfig* Cfg = GetConfigForShift(ActiveShiftIndex);
	if (!Cfg || NextDollInQueue >= Cfg->DollQueue.Num())
	{
		CompleteShift();
		return;
	}

	UDollData* DollData = Cfg->DollQueue[NextDollInQueue].LoadSynchronous();
	NextDollInQueue++;

	if (!DollData)
	{
		UE_LOG(LogTemp, Warning, TEXT("InspectionStation: DollQueue 第 %d 项加载失败，跳过。"), NextDollInQueue - 1);
		SpawnNextDoll();
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CurrentDoll = GetWorld()->SpawnActor<ASpawnedDoll>(
		DollActorClass ? DollActorClass.Get() : ASpawnedDoll::StaticClass(),
		SpawnAnchor->GetComponentTransform(),
		Params);

	if (!CurrentDoll)
	{
		UE_LOG(LogTemp, Error, TEXT("InspectionStation: spawn 娃娃 actor 失败。"));
		return;
	}

	CurrentDoll->InitializeFromData(DollData);

	const FVector StartLoc = SpawnAnchor->GetComponentLocation();
	const FVector EndLoc = InspectionAnchor->GetComponentLocation();
	CurrentDoll->SetActorLocation(StartLoc);

	GetWorld()->GetTimerManager().SetTimer(SlideInTimer, this, &AInspectionStationActor::OnSlideInComplete,
		SlideInSeconds, false);

	const int32 NumSteps = 30;
	for (int32 i = 1; i <= NumSteps; ++i)
	{
		const float T = static_cast<float>(i) / NumSteps;
		const float DelaySec = SlideInSeconds * T;
		const FVector Lerp = FMath::Lerp(StartLoc, EndLoc, T);
		FTimerHandle TmpHandle;
		GetWorld()->GetTimerManager().SetTimer(TmpHandle,
			FTimerDelegate::CreateLambda([WeakDoll = TWeakObjectPtr<ASpawnedDoll>(CurrentDoll), Lerp]() {
				if (WeakDoll.IsValid()) { WeakDoll->SetActorLocation(Lerp); }
			}),
			DelaySec, false);
	}
}

void AInspectionStationActor::OnSlideInComplete()
{
	bAwaitingVerdict = true;
	GetWorld()->GetTimerManager().SetTimer(VerdictTimeoutTimer, this,
		&AInspectionStationActor::OnVerdictTimeout, DollTimeoutSecCached, false);
	UE_LOG(LogTemp, Verbose, TEXT("InspectionStation: 娃娃到位，等待裁决（超时 %.1fs）。"), DollTimeoutSecCached);
}

void AInspectionStationActor::OnVerdictTimeout()
{
	if (bAwaitingVerdict)
	{
		UE_LOG(LogTemp, Log, TEXT("InspectionStation: 超时 → 自动 Reject。"));
		SubmitPlayerVerdict(EDollVerdict::Timeout);
	}
}

void AInspectionStationActor::SubmitPlayerVerdict(EDollVerdict Verdict)
{
	if (!bAwaitingVerdict || !CurrentDoll)
	{
		UE_LOG(LogTemp, Verbose, TEXT("InspectionStation: SubmitPlayerVerdict 被忽略（未在等待状态）。"));
		return;
	}
	bAwaitingVerdict = false;
	GetWorld()->GetTimerManager().ClearTimer(VerdictTimeoutTimer);

	if (UJudgmentCardSubsystem* JC = GetJudgment())
	{
		JC->SubmitVerdict(CurrentDoll->GetData(), Verdict);
	}

	if (Verdict == EDollVerdict::Approve)
	{
		HandleApproveExit();
	}
	else
	{
		HandleRejectChute();
	}
}

void AInspectionStationActor::HandleApproveExit()
{
	if (!CurrentDoll) { return; }

	const FVector StartLoc = CurrentDoll->GetActorLocation();
	const FVector EndLoc = ApproveExitAnchor->GetComponentLocation();
	const int32 NumSteps = 20;
	for (int32 i = 1; i <= NumSteps; ++i)
	{
		const float T = static_cast<float>(i) / NumSteps;
		const FVector Lerp = FMath::Lerp(StartLoc, EndLoc, T);
		FTimerHandle TmpHandle;
		GetWorld()->GetTimerManager().SetTimer(TmpHandle,
			FTimerDelegate::CreateLambda([WeakDoll = TWeakObjectPtr<ASpawnedDoll>(CurrentDoll), Lerp]() {
				if (WeakDoll.IsValid()) { WeakDoll->SetActorLocation(Lerp); }
			}),
			SlideOutSeconds * T, false);
	}

	GetWorld()->GetTimerManager().SetTimer(SlideOutTimer, this,
		&AInspectionStationActor::OnSlideOutComplete, SlideOutSeconds, false);
}

void AInspectionStationActor::HandleRejectChute()
{
	if (!CurrentDoll) { return; }

	const FVector StartLoc = CurrentDoll->GetActorLocation();
	const FVector EndLoc = RejectChuteAnchor->GetComponentLocation();
	const int32 NumSteps = 10;
	for (int32 i = 1; i <= NumSteps; ++i)
	{
		const float T = static_cast<float>(i) / NumSteps;
		const FVector Lerp = FMath::Lerp(StartLoc, EndLoc, T);
		FTimerHandle TmpHandle;
		GetWorld()->GetTimerManager().SetTimer(TmpHandle,
			FTimerDelegate::CreateLambda([WeakDoll = TWeakObjectPtr<ASpawnedDoll>(CurrentDoll), Lerp]() {
				if (WeakDoll.IsValid()) { WeakDoll->SetActorLocation(Lerp); }
			}),
			RejectDropSeconds * T, false);
	}

	GetWorld()->GetTimerManager().SetTimer(SlideOutTimer, this,
		&AInspectionStationActor::OnSlideOutComplete, RejectDropSeconds, false);
}

void AInspectionStationActor::OnSlideOutComplete()
{
	if (CurrentDoll)
	{
		CurrentDoll->Destroy();
		CurrentDoll = nullptr;
	}

	GetWorld()->GetTimerManager().SetTimer(InterDollTimer, this,
		&AInspectionStationActor::SpawnNextDoll, FMath::Max(0.01f, InterDollDelay), false);
}

void AInspectionStationActor::CompleteShift()
{
	UE_LOG(LogTemp, Log, TEXT("InspectionStation: Shift %d 完成。"), ActiveShiftIndex);
	OnShiftCompleted.Broadcast(ActiveShiftIndex);

	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UCheckmateGameStateSubsystem* GS = GI->GetSubsystem<UCheckmateGameStateSubsystem>())
			{
				GS->RequestStateChange(ECheckmateGameState::Ch1_PostShiftFade);
			}
		}
	}

	ActiveShiftIndex = -1;
}

UJudgmentCardSubsystem* AInspectionStationActor::GetJudgment() const
{
	if (UWorld* World = GetWorld())
	{
		return World->GetSubsystem<UJudgmentCardSubsystem>();
	}
	return nullptr;
}

const FShiftConfig* AInspectionStationActor::GetConfigForShift(int32 ShiftIndex) const
{
	for (const FShiftConfig& Cfg : ShiftConfigs)
	{
		if (Cfg.ShiftIndex == ShiftIndex) { return &Cfg; }
	}
	return nullptr;
}
