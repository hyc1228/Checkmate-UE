// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "SubliminalLayerSubsystem.h"
#include "JudgmentCardSubsystem.h"
#include "DollData.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/PostProcessVolume.h"
#include "EngineUtils.h"
#include "Engine/World.h"

void USubliminalLayerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UWorld* World = GetWorld())
	{
		if (UJudgmentCardSubsystem* JC = World->GetSubsystem<UJudgmentCardSubsystem>())
		{
			JC->OnDollVerdict.AddDynamic(this, &USubliminalLayerSubsystem::HandleDollVerdict);
		}
	}
}

void USubliminalLayerSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		if (UJudgmentCardSubsystem* JC = World->GetSubsystem<UJudgmentCardSubsystem>())
		{
			JC->OnDollVerdict.RemoveDynamic(this, &USubliminalLayerSubsystem::HandleDollVerdict);
		}
	}
	Super::Deinitialize();
}

void USubliminalLayerSubsystem::RegisterSubliminalMaterial(UMaterialInterface* InMaterial)
{
	SourceMaterial = InMaterial;
	RefreshMID();
}

void USubliminalLayerSubsystem::RefreshMID()
{
	if (!SourceMaterial) { return; }

	APostProcessVolume* UnboundedVolume = nullptr;
	for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
	{
		if (It->bUnbound) { UnboundedVolume = *It; break; }
	}
	if (!UnboundedVolume)
	{
		UE_LOG(LogTemp, Warning, TEXT("SubliminalLayer: 找不到 unbounded PostProcessVolume。请在 Level 里加一个。"));
		return;
	}

	SubliminalMID = UMaterialInstanceDynamic::Create(SourceMaterial, this);
	if (!SubliminalMID) { return; }

	FWeightedBlendable Blendable;
	Blendable.Weight = 1.0f;
	Blendable.Object = SubliminalMID;

	UnboundedVolume->Settings.WeightedBlendables.Array.Empty();
	UnboundedVolume->Settings.WeightedBlendables.Array.Add(Blendable);

	PushOpacity(CurrentOpacity);
}

void USubliminalLayerSubsystem::HandleDollVerdict(UDollData* Doll, EDollVerdict Verdict, EVerdictCategory Category)
{
	const bool bMisjudgment = (Category == EVerdictCategory::FalsePositive
		|| Category == EVerdictCategory::FalseNegative);
	if (bMisjudgment)
	{
		MisjudgmentCount++;
		const float NewOpacity = FMath::Clamp(MisjudgmentCount * 0.15f + 0.05f, 0.05f, 0.60f);
		PushOpacity(NewOpacity);
		UE_LOG(LogTemp, Log, TEXT("Subliminal: 误判 #%d → opacity=%.2f"), MisjudgmentCount, NewOpacity);
	}
}

void USubliminalLayerSubsystem::ForceOpacityTo(float Opacity)
{
	PushOpacity(FMath::Clamp(Opacity, 0.0f, 1.0f));
	UE_LOG(LogTemp, Log, TEXT("Subliminal: force opacity → %.2f"), CurrentOpacity);
}

void USubliminalLayerSubsystem::PushOpacity(float Opacity)
{
	CurrentOpacity = Opacity;
	if (SubliminalMID)
	{
		SubliminalMID->SetScalarParameterValue(OpacityParamName, Opacity);
	}
}
