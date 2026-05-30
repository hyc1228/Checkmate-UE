// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "DollDisplay.h"

#include "AudioService.h"
#include "DollData.h"
#include "InspectionScreen.h"

#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ADollDisplay::ADollDisplay()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));

	DollMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DollMesh"));
	DollMesh->SetupAttachment(SceneRoot);
	DollMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DollMesh->SetCollisionResponseToAllChannels(ECR_Block);
	DollMesh->SetGenerateOverlapEvents(false);
	if (CubeMeshAsset.Succeeded()) DollMesh->SetStaticMesh(CubeMeshAsset.Object);
	DollMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.4f));

	BoxCoverMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxCoverMesh"));
	BoxCoverMesh->SetupAttachment(SceneRoot);
	BoxCoverMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BoxCoverMesh->SetCollisionResponseToAllChannels(ECR_Block);
	BoxCoverMesh->SetGenerateOverlapEvents(false);
	if (CubeMeshAsset.Succeeded()) BoxCoverMesh->SetStaticMesh(CubeMeshAsset.Object);
	BoxCoverMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.2f));
	// 印章默认可见、悬停在头顶（具体 Z 在 BeginPlay 用 StampHoverZ 设）
}

void ADollDisplay::BeginPlay()
{
	Super::BeginPlay();

	InspectionLocation = GetActorLocation();
	InspectionRotation = GetActorRotation();
	StampCurrentZ = StampHoverZ;
	BoxCoverMesh->SetRelativeLocation(FVector(0.0f, 0.0f, StampHoverZ));

	// v0.8：印章完全独立于 actor —— 玩家拖/扔娃娃时印章绝不动。
	// Confirming 时由 TickConfirming 主动 SetWorldLocation 跟随 actor。
	BoxCoverMesh->SetAbsolute(/*bLocation=*/true, /*bRotation=*/true, /*bScale=*/false);
	BoxCoverMesh->SetWorldLocation(GetActorLocation() + FVector(0.0f, 0.0f, StampHoverZ));

	if (APlayerController* PC = GetPC())
	{
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;
	}
}

APlayerController* ADollDisplay::GetPC() const
{
	return UGameplayStatics::GetPlayerController(this, 0);
}

UPrimitiveComponent* ADollDisplay::TraceUnderCursor() const
{
	APlayerController* PC = GetPC();
	if (!PC) return nullptr;
	FHitResult Hit;
	if (!PC->GetHitResultUnderCursor(ECC_Visibility, /*bTraceComplex=*/false, Hit)) return nullptr;
	if (Hit.GetActor() != this) return nullptr;
	return Hit.GetComponent();
}

void ADollDisplay::ApplyDollData(UDollData* InDoll)
{
	CurrentDoll = InDoll;
	BeginSlideIn();
}

void ADollDisplay::SnapToIdle()
{
	State = EState::Idle;
	SetActorLocation(InspectionLocation);
	SetActorRotation(InspectionRotation);
	DollMesh->SetVisibility(true);
	DollMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.4f));
	DollMesh->SetRelativeRotation(FRotator::ZeroRotator);
	AccumYaw = 0.0f;
	AccumPitch = 0.0f;
	BoxCoverMesh->SetVisibility(true);
	StampCurrentZ = StampHoverZ;
	BoxCoverMesh->SetWorldLocation(InspectionLocation + FVector(0.0f, 0.0f, StampHoverZ));
}

void ADollDisplay::BeginSlideIn()
{
	State = EState::SlidingIn;
	AnimElapsed = 0.0f;
	AnimStartLoc = InspectionLocation + FVector(0.0f, EntryOffsetY, 0.0f);
	AnimEndLoc = InspectionLocation;
	SetActorLocation(AnimStartLoc);
	SetActorRotation(InspectionRotation);

	DollMesh->SetVisibility(true);
	DollMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.4f));
	DollMesh->SetRelativeRotation(FRotator::ZeroRotator);
	AccumYaw = 0.0f;
	AccumPitch = 0.0f;
	BoxCoverMesh->SetVisibility(true);
	StampCurrentZ = StampHoverZ;
	BoxCoverMesh->SetWorldLocation(InspectionLocation + FVector(0.0f, 0.0f, StampHoverZ));
}

void ADollDisplay::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	MouseFrameDt = FMath::Max(0.001f, DeltaSeconds);

	switch (State)
	{
	case EState::SlidingIn:
		TickSlideIn(DeltaSeconds);
		break;

	case EState::Idle:
		TryStartAnyDrag();
		break;

	case EState::MovingDoll:
		TickMovingDoll(DeltaSeconds);
		break;

	case EState::RotatingDoll:
		TickRotatingDoll(DeltaSeconds);
		break;

	case EState::DraggingStamp:
		TickDraggingStamp(DeltaSeconds);
		break;

	case EState::Tossing:
		TickTossing(DeltaSeconds);
		break;

	case EState::Confirming:
		TickConfirming(DeltaSeconds);
		break;
	}
}

void ADollDisplay::TickSlideIn(float Dt)
{
	AnimElapsed += Dt;
	const float T = FMath::Clamp(AnimElapsed / EntrySlideDurationSec, 0.0f, 1.0f);
	const float Eased = FMath::InterpEaseOut(0.0f, 1.0f, T, 2.0f);
	SetActorLocation(FMath::Lerp(AnimStartLoc, AnimEndLoc, Eased));

	if (T >= 1.0f)
	{
		SetActorLocation(AnimEndLoc);
		State = EState::Idle;
	}
}

bool ADollDisplay::TryStartAnyDrag()
{
	APlayerController* PC = GetPC();
	if (!PC) return false;

	const bool bLMB = PC->WasInputKeyJustPressed(EKeys::LeftMouseButton);
	const bool bRMB = PC->WasInputKeyJustPressed(EKeys::RightMouseButton);
	if (!bLMB && !bRMB) return false;

	UPrimitiveComponent* Hit = TraceUnderCursor();
	if (!Hit) return false;

	float MX = 0.0f, MY = 0.0f;
	PC->GetMousePosition(MX, MY);
	LastMousePos = FVector2D(MX, MY);
	LastMouseDelta = FVector2D::ZeroVector;

	if (bLMB && Hit == DollMesh)
	{
		State = EState::MovingDoll;
		return true;
	}
	if (bLMB && Hit == BoxCoverMesh)
	{
		State = EState::DraggingStamp;
		StampDragStartZ = StampCurrentZ;
		return true;
	}
	if (bRMB && Hit == DollMesh)
	{
		State = EState::RotatingDoll;
		return true;
	}
	return false;
}

void ADollDisplay::TickMovingDoll(float Dt)
{
	APlayerController* PC = GetPC();
	if (!PC) { ReleaseMovingDoll(); return; }

	float MX = 0.0f, MY = 0.0f;
	if (!PC->GetMousePosition(MX, MY)) { ReleaseMovingDoll(); return; }

	const FVector2D Cur(MX, MY);
	const FVector2D Delta = Cur - LastMousePos;
	LastMousePos = Cur;
	LastMouseDelta = Delta;  // 留给松手判 toss 速度

	// 跟随鼠标在屏幕平面拖移：屏幕 X delta → 世界 Y；屏幕 Y delta → 世界 Z
	FVector L = GetActorLocation();
	L.Y += Delta.X * DragMoveSensitivity;
	L.Z -= Delta.Y * DragMoveSensitivity;
	SetActorLocation(L);

	if (PC->WasInputKeyJustReleased(EKeys::LeftMouseButton))
	{
		ReleaseMovingDoll();
	}
}

void ADollDisplay::TickRotatingDoll(float Dt)
{
	APlayerController* PC = GetPC();
	if (!PC) { ReleaseRotatingDoll(); return; }

	float MX = 0.0f, MY = 0.0f;
	if (!PC->GetMousePosition(MX, MY)) { ReleaseRotatingDoll(); return; }

	const FVector2D Cur(MX, MY);
	const FVector2D Delta = Cur - LastMousePos;
	LastMousePos = Cur;
	LastMouseDelta = Delta;

	AccumYaw   = FMath::Clamp(AccumYaw   + Delta.X * YawSensitivity,   -YawMaxDegrees,   YawMaxDegrees);
	AccumPitch = FMath::Clamp(AccumPitch + Delta.Y * PitchSensitivity, -PitchMaxDegrees, PitchMaxDegrees);
	DollMesh->SetRelativeRotation(FRotator(AccumPitch, AccumYaw, 0.0f));

	if (PC->WasInputKeyJustReleased(EKeys::RightMouseButton))
	{
		ReleaseRotatingDoll();
	}
}

void ADollDisplay::TickDraggingStamp(float Dt)
{
	APlayerController* PC = GetPC();
	if (!PC) { ReleaseStampDrag(); return; }

	float MX = 0.0f, MY = 0.0f;
	if (!PC->GetMousePosition(MX, MY)) { ReleaseStampDrag(); return; }

	const FVector2D Cur(MX, MY);
	const float DeltaY = Cur.Y - LastMousePos.Y;
	LastMousePos = Cur;

	StampCurrentZ -= DeltaY * StampDragSensitivity;
	const float StampMinZ = StampHoverZ - (StampPullDownThreshold * 1.2f);
	StampCurrentZ = FMath::Clamp(StampCurrentZ, StampMinZ, StampHoverZ);
	BoxCoverMesh->SetWorldLocation(InspectionLocation + FVector(0.0f, 0.0f, StampCurrentZ));

	if (PC->WasInputKeyJustReleased(EKeys::LeftMouseButton))
	{
		ReleaseStampDrag();
	}
}

void ADollDisplay::ReleaseMovingDoll()
{
	const float Speed = LastMouseDelta.Size() / MouseFrameDt;
	if (Speed >= TossSpeedThreshold)
	{
		EnterTossing();
	}
	else
	{
		// 力度不够 → 弹回原位
		SetActorLocation(InspectionLocation);
		State = EState::Idle;
	}
}

void ADollDisplay::ReleaseRotatingDoll()
{
	// 旋转松手不触发任何手势，只回到 Idle 保留当前角度
	State = EState::Idle;
}

void ADollDisplay::ReleaseStampDrag()
{
	const float PullDown = StampHoverZ - StampCurrentZ;
	if (PullDown >= StampPullDownThreshold)
	{
		EnterConfirming();
	}
	else
	{
		StampCurrentZ = StampHoverZ;
		BoxCoverMesh->SetWorldLocation(InspectionLocation + FVector(0.0f, 0.0f, StampHoverZ));
		State = EState::Idle;
	}
}

void ADollDisplay::ForceToss()
{
	if (State == EState::Idle || State == EState::MovingDoll || State == EState::RotatingDoll
		|| State == EState::DraggingStamp || State == EState::SlidingIn)
	{
		EnterTossing();
	}
}

void ADollDisplay::TriggerLookAtCamera(float HoldSeconds)
{
	if (!DollMesh)
	{
		return;
	}

	APlayerController* PC = GetPC();
	if (!PC || !PC->PlayerCameraManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("PV_CH1_A7_LOOKAT skipped: no PlayerCameraManager"));
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PVLookAtTimerHandle);
	}

	PVLookAtOriginalRelativeRotation = DollMesh->GetRelativeRotation();

	const FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();
	const FVector EyeLocation = DollMesh->GetComponentLocation();
	const FRotator LookAtRotation = (CameraLocation - EyeLocation).Rotation();
	DollMesh->SetWorldRotation(LookAtRotation);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PVLookAtTimerHandle,
			FTimerDelegate::CreateUObject(this, &ADollDisplay::RestorePVLookAtRotation),
			FMath::Max(0.05f, HoldSeconds), false);
	}

	UE_LOG(LogTemp, Display, TEXT("PV_CH1_A7_LOOKAT Hold=%.2f Doll=%s"),
		HoldSeconds,
		CurrentDoll ? *CurrentDoll->DollId.ToString() : TEXT("None"));
}

void ADollDisplay::EnterTossing()
{
	State = EState::Tossing;
	AnimElapsed = 0.0f;
	AnimStartLoc = GetActorLocation();
	const float LateralY = FMath::FRandRange(-1.0f, 1.0f) * (TossUpwardDistance * 0.6f);
	const float BackX = -TossUpwardDistance * 0.4f;
	AnimEndLoc = AnimStartLoc + FVector(BackX, LateralY, TossUpwardDistance);

	UAudioService::PlayCueStatic(this, FName("Ch1.Toss"));

	// 印章留在原地：记录当前世界位（toss 期间会保持在此）
	if (OwningScreen)
	{
		OwningScreen->PlayTossActionFeedback();        // 扔动作瞬间的弱震
		OwningScreen->HandleGestureFromDoll(false);    // 触发评估 → 红 flash 跟上
	}
}

void ADollDisplay::EnterConfirming()
{
	State = EState::Confirming;
	AnimElapsed = 0.0f;
	AnimStartLoc = GetActorLocation();
	AnimEndLoc = AnimStartLoc + FVector(0.0f, ConfirmExitDistanceY, 0.0f);

	UAudioService::PlayCueStatic(this, FName("Ch1.Stamp"));

	if (OwningScreen)
	{
		OwningScreen->PlayStampImpactFeedback();      // 印章砸下的强震
		OwningScreen->HandleGestureFromDoll(true);    // 评估 → 绿 flash 跟上
	}
}

void ADollDisplay::TickTossing(float Dt)
{
	AnimElapsed += Dt;
	const float T = FMath::Clamp(AnimElapsed / TossDurationSec, 0.0f, 1.0f);

	const float Eased = FMath::InterpEaseOut(0.0f, 1.0f, T, 1.6f);
	FVector L = FMath::Lerp(AnimStartLoc, AnimEndLoc, Eased);
	const float ArcDip = -120.0f * T * T;
	L.Z += ArcDip;
	SetActorLocation(L);

	const float Scale = FMath::Lerp(1.0f, 0.0f, T);
	DollMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.4f) * Scale);

	FRotator R = GetActorRotation();
	R.Roll  += 900.0f * Dt;
	R.Pitch += 720.0f * Dt;
	R.Yaw   += 360.0f * Dt;
	SetActorRotation(R);

	// 印章不跟扔：用绝对世界坐标 pin 在检验位的 hover Z
	BoxCoverMesh->SetWorldLocation(InspectionLocation + FVector(0.0f, 0.0f, StampHoverZ));
	BoxCoverMesh->SetWorldRotation(FRotator::ZeroRotator);

	if (T >= 1.0f)
	{
		DollMesh->SetVisibility(false);
		if (OwningScreen) OwningScreen->OnDollAnimComplete();
	}
}

void ADollDisplay::TickConfirming(float Dt)
{
	AnimElapsed += Dt;
	const float T = FMath::Clamp(AnimElapsed / ConfirmDurationSec, 0.0f, 1.0f);
	const float Eased = FMath::InterpEaseInOut(0.0f, 1.0f, T, 2.0f);
	const FVector ActorPos = FMath::Lerp(AnimStartLoc, AnimEndLoc, Eased);
	SetActorLocation(ActorPos);

	// 印章罩在娃娃头顶（landedZ = StampHoverZ - StampPullDownThreshold），跟随 actor 右移
	const float LandedZ = StampHoverZ - StampPullDownThreshold;
	BoxCoverMesh->SetWorldLocation(ActorPos + FVector(0.0f, 0.0f, LandedZ));

	if (T >= 1.0f)
	{
		DollMesh->SetVisibility(false);
		BoxCoverMesh->SetVisibility(false);
		if (OwningScreen) OwningScreen->OnDollAnimComplete();
	}
}

void ADollDisplay::RestorePVLookAtRotation()
{
	if (DollMesh)
	{
		DollMesh->SetRelativeRotation(PVLookAtOriginalRelativeRotation);
	}
}
