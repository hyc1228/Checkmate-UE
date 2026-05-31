// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "DollDisplay.h"

#include "AudioService.h"
#include "DollData.h"
#include "InspectionScreen.h"
#include "JudgmentEvaluator.h"

#include "Animation/AnimSequence.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RandomStream.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
struct FHairRampPalette
{
	FLinearColor Root;
	FLinearColor Mid;
	FLinearColor Tip;
	float Roughness = 0.62f;
	float Specular = 0.28f;
};

const FHairRampPalette DarkNaturalHairPalettes[] =
{
	{ FLinearColor(0.08f, 0.045f, 0.025f, 1.0f), FLinearColor(0.18f, 0.10f, 0.05f, 1.0f), FLinearColor(0.34f, 0.20f, 0.10f, 1.0f), 0.66f, 0.24f },
	{ FLinearColor(0.035f, 0.032f, 0.030f, 1.0f), FLinearColor(0.11f, 0.095f, 0.080f, 1.0f), FLinearColor(0.28f, 0.22f, 0.16f, 1.0f), 0.70f, 0.20f },
	{ FLinearColor(0.13f, 0.055f, 0.035f, 1.0f), FLinearColor(0.28f, 0.13f, 0.08f, 1.0f), FLinearColor(0.46f, 0.25f, 0.15f, 1.0f), 0.64f, 0.25f },
};

const FHairRampPalette LightNaturalHairPalettes[] =
{
	{ FLinearColor(0.38f, 0.24f, 0.10f, 1.0f), FLinearColor(0.76f, 0.55f, 0.25f, 1.0f), FLinearColor(1.00f, 0.83f, 0.46f, 1.0f), 0.58f, 0.30f },
	{ FLinearColor(0.50f, 0.34f, 0.18f, 1.0f), FLinearColor(0.86f, 0.66f, 0.38f, 1.0f), FLinearColor(1.00f, 0.88f, 0.62f, 1.0f), 0.56f, 0.32f },
	{ FLinearColor(0.42f, 0.20f, 0.12f, 1.0f), FLinearColor(0.78f, 0.36f, 0.22f, 1.0f), FLinearColor(1.00f, 0.58f, 0.36f, 1.0f), 0.60f, 0.30f },
};

FLinearColor MakeCreativeHairColor(float Hue, float Saturation, float Value)
{
	return FLinearColor::MakeFromHSV8(
		static_cast<uint8>(FMath::RoundToInt(FMath::Fmod(Hue, 1.0f) * 255.0f)),
		static_cast<uint8>(FMath::RoundToInt(FMath::Clamp(Saturation, 0.0f, 1.0f) * 255.0f)),
		static_cast<uint8>(FMath::RoundToInt(FMath::Clamp(Value, 0.0f, 1.0f) * 255.0f)));
}

FHairRampPalette MakeCreativeHairPalette(int32 Seed)
{
	FRandomStream Stream(Seed != 0 ? Seed : 9137);
	const float BaseHue = Stream.FRand();
	const float MidHue = FMath::Fmod(BaseHue + Stream.FRandRange(0.08f, 0.28f), 1.0f);
	const float TipHue = FMath::Fmod(BaseHue + Stream.FRandRange(0.32f, 0.62f), 1.0f);

	FHairRampPalette Palette;
	Palette.Root = MakeCreativeHairColor(BaseHue, Stream.FRandRange(0.68f, 0.92f), Stream.FRandRange(0.52f, 0.72f));
	Palette.Mid = MakeCreativeHairColor(MidHue, Stream.FRandRange(0.58f, 0.88f), Stream.FRandRange(0.72f, 0.94f));
	Palette.Tip = MakeCreativeHairColor(TipHue, Stream.FRandRange(0.48f, 0.78f), Stream.FRandRange(0.78f, 1.0f));
	Palette.Roughness = Stream.FRandRange(0.52f, 0.66f);
	Palette.Specular = Stream.FRandRange(0.26f, 0.36f);
	return Palette;
}
}

ADollDisplay::ADollDisplay()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	VisualPivot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualPivot"));
	VisualPivot->SetupAttachment(SceneRoot);
	VisualPivot->SetRelativeLocation(RotationPivotRelativeLocation);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));

	DollMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DollMesh"));
	DollMesh->SetupAttachment(VisualPivot);
	DollMesh->SetRelativeLocation(-RotationPivotRelativeLocation);
	DollMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DollMesh->SetCollisionResponseToAllChannels(ECR_Block);
	DollMesh->SetGenerateOverlapEvents(false);
	if (CubeMeshAsset.Succeeded()) DollMesh->SetStaticMesh(CubeMeshAsset.Object);
	DollMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.4f));

	PoseMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PoseMesh"));
	PoseMesh->SetupAttachment(VisualPivot);
	PoseMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PoseMesh->SetCollisionResponseToAllChannels(ECR_Block);
	PoseMesh->SetGenerateOverlapEvents(false);
	PoseMesh->SetVisibility(false);
	PoseMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);

	HairMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HairMesh"));
	HairMesh->SetupAttachment(VisualPivot);
	HairMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HairMesh->SetCollisionResponseToAllChannels(ECR_Block);
	HairMesh->SetGenerateOverlapEvents(false);
	HairMesh->SetVisibility(false);
	HairMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> DefaultPoseMeshAsset(TEXT("/Game/Art/Characters/Ch1Poses/SK_Character_A_BadandSad2.SK_Character_A_BadandSad2"));
	if (DefaultPoseMeshAsset.Succeeded())
	{
		DefaultPoseMesh = DefaultPoseMeshAsset.Object;
		PoseMesh->SetSkeletalMesh(DefaultPoseMeshAsset.Object);
		PoseMesh->SetRelativeLocation(PoseMeshRelativeLocation - RotationPivotRelativeLocation);
		PoseMesh->SetRelativeRotation(PoseMeshRelativeRotation);
		PoseMesh->SetRelativeScale3D(FVector(PoseMeshUniformScale));
		PoseMesh->SetVisibility(true);
		DollMesh->SetVisibility(false);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> HairMaterialAsset(TEXT("/Game/Materials/MI_Hair_ColorRamp_Default.MI_Hair_ColorRamp_Default"));
	if (HairMaterialAsset.Succeeded())
	{
		HairBaseMaterialOverride = HairMaterialAsset.Object;
	}
	static ConstructorHelpers::FObjectFinder<UAnimSequence> SmilePoseAnimAsset(TEXT("/Game/Art/Characters/Ch1Poses/SK_Character_A_DanceSmile_Anim.SK_Character_A_DanceSmile_Anim"));
	if (SmilePoseAnimAsset.Succeeded())
	{
		SmilePoseAnim = SmilePoseAnimAsset.Object;
	}
	static ConstructorHelpers::FObjectFinder<UAnimSequence> SadPoseAnimAsset(TEXT("/Game/Art/Characters/Ch1Poses/SK_Character_A_DanceSad_Anim.SK_Character_A_DanceSad_Anim"));
	if (SadPoseAnimAsset.Succeeded())
	{
		SadPoseAnim = SadPoseAnimAsset.Object;
	}
	static ConstructorHelpers::FObjectFinder<UAnimSequence> BadPoseAnimAsset(TEXT("/Game/Art/Characters/Ch1Poses/SK_Character_A_DanceSad2_Anim.SK_Character_A_DanceSad2_Anim"));
	if (BadPoseAnimAsset.Succeeded())
	{
		BadPoseAnim = BadPoseAnimAsset.Object;
	}

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
	CurrentViewScaleMultiplier = FMath::Clamp(DefaultViewScaleMultiplier, MinViewScaleMultiplier, MaxViewScaleMultiplier);
	StampCurrentZ = StampHoverZ;
	ApplyVisualPivotOffset();
	BoxCoverMesh->SetRelativeLocation(FVector(0.0f, 0.0f, StampHoverZ));

	// v0.8：印章完全独立于 actor —— 玩家拖/扔娃娃时印章绝不动。
	// Confirming 时由 TickConfirming 主动 SetWorldLocation 跟随 actor。
	BoxCoverMesh->SetAbsolute(/*bLocation=*/true, /*bRotation=*/true, /*bScale=*/false);
	BoxCoverMesh->SetWorldLocation(GetStampWorldLocation(StampHoverZ));

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
	HairColorSeed = FMath::Rand();
	ApplyPoseVisualFromDoll();
	BeginSlideIn();
}

void ADollDisplay::SnapToIdle()
{
	State = EState::Idle;
	SetActorLocation(InspectionLocation);
	SetActorRotation(InspectionRotation);
	ApplyPoseVisualFromDoll();
	SetDollVisualVisibility(true);
	SetDollVisualRelativeScale(1.0f);
	SetDollVisualRelativeRotation(FRotator::ZeroRotator);
	AccumYaw = 0.0f;
	AccumPitch = 0.0f;
	BoxCoverMesh->SetVisibility(true);
	StampCurrentZ = StampHoverZ;
	BoxCoverMesh->SetWorldLocation(GetStampWorldLocation(StampHoverZ));
}

void ADollDisplay::BeginSlideIn()
{
	State = EState::SlidingIn;
	AnimElapsed = 0.0f;
	CurrentViewScaleMultiplier = FMath::Clamp(DefaultViewScaleMultiplier, MinViewScaleMultiplier, MaxViewScaleMultiplier);
	AnimStartLoc = InspectionLocation + FVector(0.0f, EntryOffsetY, 0.0f);
	AnimEndLoc = InspectionLocation;
	SetActorLocation(AnimStartLoc);
	SetActorRotation(InspectionRotation);

	ApplyPoseVisualFromDoll();
	SetDollVisualVisibility(true);
	SetDollVisualRelativeScale(1.0f);
	SetDollVisualRelativeRotation(FRotator::ZeroRotator);
	AccumYaw = 0.0f;
	AccumPitch = 0.0f;
	BoxCoverMesh->SetVisibility(true);
	StampCurrentZ = StampHoverZ;
	BoxCoverMesh->SetWorldLocation(GetStampWorldLocation(StampHoverZ));
}

void ADollDisplay::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	MouseFrameDt = FMath::Max(0.001f, DeltaSeconds);

	if (State == EState::Idle || State == EState::RotatingDoll)
	{
		HandleZoomInput();
		HandleGamepadRotationInput(DeltaSeconds);
	}

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
	DragStartMousePos = LastMousePos;
	DragDistancePixels = 0.0f;
	LastMouseDelta = FVector2D::ZeroVector;

	if (bLMB && Hit == BoxCoverMesh)
	{
		State = EState::DraggingStamp;
		StampDragStartZ = StampCurrentZ;
		return true;
	}
	if (bLMB && (Hit == DollMesh || Hit == PoseMesh))
	{
		State = EState::MovingDoll;
		return true;
	}
	if (bLMB && Hit == HairMesh)
	{
		State = EState::MovingDoll;
		return true;
	}
	if (bRMB && (Hit == DollMesh || Hit == PoseMesh || Hit == HairMesh))
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
	DragDistancePixels += Delta.Size();

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
	DragDistancePixels += Delta.Size();

	AccumYaw   = FMath::Clamp(AccumYaw   + Delta.X * YawSensitivity,   -YawMaxDegrees,   YawMaxDegrees);
	AccumPitch = FMath::Clamp(AccumPitch + Delta.Y * PitchSensitivity, -PitchMaxDegrees, PitchMaxDegrees);
	SetDollVisualRelativeRotation(FRotator(AccumPitch, AccumYaw, 0.0f));

	if (PC->WasInputKeyJustReleased(EKeys::LeftMouseButton) || PC->WasInputKeyJustReleased(EKeys::RightMouseButton))
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
	BoxCoverMesh->SetWorldLocation(GetStampWorldLocation(StampCurrentZ));

	if (PC->WasInputKeyJustReleased(EKeys::LeftMouseButton))
	{
		ReleaseStampDrag();
	}
}

void ADollDisplay::ReleaseMovingDoll()
{
	if (IsCurrentDragATossGesture())
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
	// 旋转松手默认只回到 Idle，保留当前角度。
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
		BoxCoverMesh->SetWorldLocation(GetStampWorldLocation(StampHoverZ));
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

bool ADollDisplay::IsCurrentDragATossGesture() const
{
	if (DragDistancePixels < TossMinDragDistancePixels)
	{
		return false;
	}

	const float Speed = LastMouseDelta.Size() / FMath::Max(0.001f, MouseFrameDt);
	if (Speed < TossSpeedThreshold)
	{
		return false;
	}

	if (bRequireUpwardTossGesture)
	{
		const FVector2D TotalDelta = LastMousePos - DragStartMousePos;
		if (TotalDelta.Y > -TossMinDragDistancePixels * 0.35f)
		{
			return false;
		}
	}

	return true;
}

void ADollDisplay::HandleZoomInput()
{
	APlayerController* PC = GetPC();
	if (!PC)
	{
		return;
	}

	float ZoomDelta = 0.0f;
	if (PC->WasInputKeyJustPressed(EKeys::MouseScrollUp))
	{
		ZoomDelta += MouseWheelZoomStep;
	}
	if (PC->WasInputKeyJustPressed(EKeys::MouseScrollDown))
	{
		ZoomDelta -= MouseWheelZoomStep;
	}

	if (FMath::IsNearlyZero(ZoomDelta))
	{
		return;
	}

	CurrentViewScaleMultiplier = FMath::Clamp(
		CurrentViewScaleMultiplier + ZoomDelta,
		MinViewScaleMultiplier,
		MaxViewScaleMultiplier);
	SetDollVisualRelativeScale(1.0f);
}

void ADollDisplay::HandleGamepadRotationInput(float Dt)
{
	APlayerController* PC = GetPC();
	if (!PC)
	{
		return;
	}

	const FVector2D LeftStick(
		PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX),
		PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY));
	const FVector2D RightStick(
		PC->GetInputAnalogKeyState(EKeys::Gamepad_RightX),
		PC->GetInputAnalogKeyState(EKeys::Gamepad_RightY));

	const FVector2D Stick = RightStick.SizeSquared() > LeftStick.SizeSquared() ? RightStick : LeftStick;
	if (Stick.SizeSquared() < FMath::Square(GamepadStickDeadZone))
	{
		return;
	}

	AccumYaw = FMath::Clamp(
		AccumYaw + Stick.X * GamepadYawDegreesPerSecond * Dt,
		-YawMaxDegrees,
		YawMaxDegrees);
	AccumPitch = FMath::Clamp(
		AccumPitch - Stick.Y * GamepadPitchDegreesPerSecond * Dt,
		-PitchMaxDegrees,
		PitchMaxDegrees);
	SetDollVisualRelativeRotation(FRotator(AccumPitch, AccumYaw, 0.0f));
}

FVector ADollDisplay::GetStampWorldLocation(float LocalStampZ) const
{
	return InspectionLocation + FVector(0.0f, 0.0f, LocalStampZ + StampWorldZOffset);
}

void ADollDisplay::TriggerLookAtCamera(float HoldSeconds)
{
	if (!DollMesh && !PoseMesh)
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

	PVLookAtOriginalRelativeRotation = IsPoseVisualActive()
		? PoseMesh->GetRelativeRotation()
		: DollMesh->GetRelativeRotation();

	const FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();
	const FVector EyeLocation = IsPoseVisualActive()
		? PoseMesh->GetComponentLocation()
		: DollMesh->GetComponentLocation();
	const FRotator LookAtRotation = (CameraLocation - EyeLocation).Rotation();
	SetDollVisualWorldRotation(LookAtRotation);

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
	SetDollVisualRelativeScale(Scale);

	FRotator R = GetActorRotation();
	R.Roll  += 900.0f * Dt;
	R.Pitch += 720.0f * Dt;
	R.Yaw   += 360.0f * Dt;
	SetActorRotation(R);

	// 印章不跟扔：用绝对世界坐标 pin 在检验位的 hover Z
	BoxCoverMesh->SetWorldLocation(GetStampWorldLocation(StampHoverZ));
	BoxCoverMesh->SetWorldRotation(FRotator::ZeroRotator);

	if (T >= 1.0f)
	{
		SetDollVisualVisibility(false);
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
	BoxCoverMesh->SetWorldLocation(ActorPos + FVector(0.0f, 0.0f, LandedZ + StampWorldZOffset));

	if (T >= 1.0f)
	{
		SetDollVisualVisibility(false);
		BoxCoverMesh->SetVisibility(false);
		if (OwningScreen) OwningScreen->OnDollAnimComplete();
	}
}

void ADollDisplay::RestorePVLookAtRotation()
{
	if (IsPoseVisualActive())
	{
		PoseMesh->SetRelativeRotation(PVLookAtOriginalRelativeRotation);
	}
	else if (DollMesh)
	{
		DollMesh->SetRelativeRotation(PVLookAtOriginalRelativeRotation);
	}
}

void ADollDisplay::ApplyPoseVisualFromDoll()
{
	if (!PoseMesh || !DefaultPoseMesh)
	{
		if (DollMesh)
		{
			DollMesh->SetVisibility(true);
		}
		return;
	}

	PoseMesh->SetSkeletalMesh(DefaultPoseMesh);
	ApplyVisualPivotOffset();
	PoseMesh->SetRelativeRotation(PoseMeshRelativeRotation);
	PoseMesh->SetRelativeScale3D(FVector(PoseMeshUniformScale));

	UAnimSequence* ChosenAnim = SmilePoseAnim;
	if (CurrentDoll)
	{
		if (!UJudgmentEvaluator::DollHasPieceworkTrait(CurrentDoll, TEXT("PoseConforming")))
		{
			ChosenAnim = BadPoseAnim ? BadPoseAnim : SadPoseAnim;
		}
		else if (!UJudgmentEvaluator::DollHasPieceworkTrait(CurrentDoll, TEXT("Smile")))
		{
			ChosenAnim = SadPoseAnim ? SadPoseAnim : SmilePoseAnim;
		}
	}

	if (ChosenAnim)
	{
		PoseMesh->PlayAnimation(ChosenAnim, true);
	}

	PoseMesh->SetVisibility(true);
	if (DollMesh)
	{
		DollMesh->SetVisibility(false);
	}

	ApplyHairVisualFromDoll();
}

void ADollDisplay::ApplyHairVisualFromDoll()
{
	if (!HairMesh)
	{
		return;
	}

	USkeletalMesh* ChosenHairMesh = CurrentDoll ? CurrentDoll->HairMesh : nullptr;
	HairMesh->SetSkeletalMesh(ChosenHairMesh);
	HairMesh->SetVisibility(ChosenHairMesh != nullptr && PoseMesh && PoseMesh->IsVisible());
	HairMesh->SetCollisionEnabled(ChosenHairMesh ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	HairMesh->SetRelativeLocation(PoseMeshRelativeLocation - RotationPivotRelativeLocation);
	HairMesh->SetRelativeRotation(PoseMeshRelativeRotation);
	HairMesh->SetRelativeScale3D(FVector(PoseMeshUniformScale * CurrentViewScaleMultiplier));

	if (ChosenHairMesh && PoseMesh)
	{
		HairMesh->SetLeaderPoseComponent(PoseMesh, true);
	}

	ApplyHairMaterialOverrides();
}

void ADollDisplay::ApplyHairMaterialOverrides()
{
	if (!HairMesh || !HairMesh->GetSkeletalMeshAsset() || !HairBaseMaterialOverride)
	{
		return;
	}

	const bool bNaturalColor = CurrentDoll
		? UJudgmentEvaluator::DollHasPieceworkTrait(CurrentDoll, TEXT("NaturalColor"))
		: true;
	const bool bLongHair = CurrentDoll
		? UJudgmentEvaluator::DollHasPieceworkTrait(CurrentDoll, TEXT("LongHair"))
		: true;
	const bool bStyled = CurrentDoll
		? UJudgmentEvaluator::DollHasPieceworkTrait(CurrentDoll, TEXT("Styled"))
		: false;

	FHairRampPalette ChosenPalette;
	if (!bNaturalColor)
	{
		ChosenPalette = MakeCreativeHairPalette(HairColorSeed);
	}
	else
	{
		const uint32 PaletteIndex = (CurrentDoll ? GetTypeHash(CurrentDoll->DollId) : 0)
			+ (bLongHair ? 1 : 0)
			+ (bStyled ? 2 : 0);
		const bool bUseLightNatural = (PaletteIndex % 2) == 0;
		ChosenPalette = bUseLightNatural
			? LightNaturalHairPalettes[PaletteIndex % UE_ARRAY_COUNT(LightNaturalHairPalettes)]
			: DarkNaturalHairPalettes[PaletteIndex % UE_ARRAY_COUNT(DarkNaturalHairPalettes)];
	}

	const int32 MaterialCount = HairMesh->GetNumMaterials();
	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		if (!ShouldOverrideHairMaterialSlot(MaterialIndex))
		{
			continue;
		}

		UMaterialInstanceDynamic* HairMID = HairMesh->CreateDynamicMaterialInstance(MaterialIndex, HairBaseMaterialOverride);
		if (!HairMID)
		{
			continue;
		}

		HairMID->SetVectorParameterValue(TEXT("ColorRoot"), ChosenPalette.Root);
		HairMID->SetVectorParameterValue(TEXT("ColorMid"), ChosenPalette.Mid);
		HairMID->SetVectorParameterValue(TEXT("ColorTip"), ChosenPalette.Tip);
		HairMID->SetVectorParameterValue(TEXT("RampAxis"), FLinearColor(0.0f, 0.0f, 1.0f, 0.0f));
		HairMID->SetScalarParameterValue(TEXT("StopA"), 0.38f);
		HairMID->SetScalarParameterValue(TEXT("StopB"), 0.82f);
		HairMID->SetScalarParameterValue(TEXT("RampOffset"), 0.0f);
		HairMID->SetScalarParameterValue(TEXT("RampContrast"), 1.08f);
		HairMID->SetScalarParameterValue(TEXT("Roughness"), ChosenPalette.Roughness);
		HairMID->SetScalarParameterValue(TEXT("Specular"), ChosenPalette.Specular);
	}
}

bool ADollDisplay::ShouldOverrideHairMaterialSlot(int32 MaterialIndex) const
{
	if (!HairMesh || !HairMesh->GetSkeletalMeshAsset())
	{
		return false;
	}

	const USkeletalMesh* SkeletalMeshAsset = HairMesh->GetSkeletalMeshAsset();
	const TArray<FSkeletalMaterial>& Materials = SkeletalMeshAsset->GetMaterials();
	if (Materials.IsValidIndex(MaterialIndex) && Materials[MaterialIndex].MaterialSlotName == TEXT("M_HairBase"))
	{
		return true;
	}

	const UMaterialInterface* CurrentMaterial = HairMesh->GetMaterial(MaterialIndex);
	return CurrentMaterial && CurrentMaterial->GetName().Contains(TEXT("M_HairBase"));
}

bool ADollDisplay::IsPoseVisualActive() const
{
	return PoseMesh && PoseMesh->GetSkeletalMeshAsset() && PoseMesh->IsVisible();
}

void ADollDisplay::ApplyVisualPivotOffset()
{
	if (VisualPivot)
	{
		VisualPivot->SetRelativeLocation(RotationPivotRelativeLocation);
	}
	if (DollMesh)
	{
		DollMesh->SetRelativeLocation(-RotationPivotRelativeLocation);
	}
	if (PoseMesh)
	{
		PoseMesh->SetRelativeLocation(PoseMeshRelativeLocation - RotationPivotRelativeLocation);
	}
	if (HairMesh)
	{
		HairMesh->SetRelativeLocation(PoseMeshRelativeLocation - RotationPivotRelativeLocation);
	}
}

void ADollDisplay::SetDollVisualVisibility(bool bVisible)
{
	if (IsPoseVisualActive() || (PoseMesh && PoseMesh->GetSkeletalMeshAsset()))
	{
		PoseMesh->SetVisibility(bVisible);
		if (HairMesh && HairMesh->GetSkeletalMeshAsset())
		{
			HairMesh->SetVisibility(bVisible);
		}
		if (DollMesh)
		{
			DollMesh->SetVisibility(false);
		}
		return;
	}

	if (DollMesh)
	{
		DollMesh->SetVisibility(bVisible);
	}
	if (HairMesh)
	{
		HairMesh->SetVisibility(false);
	}
}

void ADollDisplay::SetDollVisualRelativeScale(float ScaleMultiplier)
{
	if (PoseMesh && PoseMesh->GetSkeletalMeshAsset())
	{
		PoseMesh->SetRelativeScale3D(FVector(PoseMeshUniformScale * CurrentViewScaleMultiplier * ScaleMultiplier));
	}
	if (HairMesh && HairMesh->GetSkeletalMeshAsset())
	{
		HairMesh->SetRelativeScale3D(FVector(PoseMeshUniformScale * CurrentViewScaleMultiplier * ScaleMultiplier));
	}
	if (DollMesh)
	{
		DollMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.4f) * CurrentViewScaleMultiplier * ScaleMultiplier);
	}
}

void ADollDisplay::SetDollVisualRelativeRotation(const FRotator& Rotation)
{
	if (VisualPivot)
	{
		VisualPivot->SetRelativeRotation(Rotation);
		return;
	}

	if (PoseMesh && PoseMesh->GetSkeletalMeshAsset())
	{
		PoseMesh->SetRelativeRotation(PoseMeshRelativeRotation + Rotation);
	}
	if (DollMesh)
	{
		DollMesh->SetRelativeRotation(Rotation);
	}
}

void ADollDisplay::SetDollVisualWorldRotation(const FRotator& Rotation)
{
	if (IsPoseVisualActive())
	{
		PoseMesh->SetWorldRotation(Rotation);
		return;
	}
	if (DollMesh)
	{
		DollMesh->SetWorldRotation(Rotation);
	}
}
