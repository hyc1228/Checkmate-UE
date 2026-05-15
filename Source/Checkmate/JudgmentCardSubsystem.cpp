// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "JudgmentCardSubsystem.h"
#include "CardData.h"
#include "DollData.h"
#include "CheckmateGameStateSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void UJudgmentCardSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CurrentStandard.Reset();
}

void UJudgmentCardSubsystem::SetCurrentStandard(const TArray<UCardData*>& AssembledCards)
{
	CurrentStandard.Reset();
	for (UCardData* Card : AssembledCards)
	{
		if (Card) { CurrentStandard.Add(Card); }
	}
	UE_LOG(LogTemp, Log, TEXT("Judgment: 标准已组装 %d 张卡。"), CurrentStandard.Num());
	for (const UCardData* C : CurrentStandard)
	{
		UE_LOG(LogTemp, Verbose, TEXT("  - %s (Pearl=%s)"), *C->CardId.ToString(), C->bIsPearlCompatible ? TEXT("true") : TEXT("false"));
	}
}

bool UJudgmentCardSubsystem::EvaluateDoll(const UDollData* Doll) const
{
	if (!Doll)
	{
		UE_LOG(LogTemp, Warning, TEXT("EvaluateDoll: Doll == nullptr。"));
		return false;
	}

	if (CurrentStandard.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("EvaluateDoll: 当前标准为空（先 SetCurrentStandard）。默认判定为不合规。"));
		return false;
	}

	for (const UCardData* Card : CurrentStandard)
	{
		if (!Card) { continue; }
		const bool bAttrTrue = Doll->GetAttributeByCardId(Card->CardId);
		if (!bAttrTrue)
		{
			// 任一标准未满足 → 不合规
			return false;
		}
	}
	return true;
}

void UJudgmentCardSubsystem::SubmitVerdict(UDollData* Doll, EDollVerdict Verdict)
{
	if (!Doll)
	{
		UE_LOG(LogTemp, Warning, TEXT("SubmitVerdict: Doll == nullptr。"));
		return;
	}

	const bool bIsConforming = EvaluateDoll(Doll);
	const EVerdictCategory Category = ClassifyVerdict(bIsConforming, Verdict);

	UE_LOG(LogTemp, Log, TEXT("Verdict: %s → %s (Conforming=%s, Category=%s)"),
		*Doll->GetName(),
		*UEnum::GetValueAsString(Verdict),
		bIsConforming ? TEXT("true") : TEXT("false"),
		*UEnum::GetValueAsString(Category));

	OnDollVerdict.Broadcast(Doll, Verdict, Category);

	if (CheckTwistConditions(Doll, Verdict, bIsConforming))
	{
		UE_LOG(LogTemp, Warning, TEXT("Twist condition met! (Doll=%s, Verdict=%s, Pearl=%s, Fallback=%s)"),
			*Doll->GetName(),
			*UEnum::GetValueAsString(Verdict),
			Doll->ButtonStyle == EButtonStyle::Pearl ? TEXT("YES") : TEXT("no"),
			Doll->bIsFallback ? TEXT("YES") : TEXT("no"));
		OnTwistConditionMet.Broadcast();
	}
}

EVerdictCategory UJudgmentCardSubsystem::ClassifyVerdict(bool bIsConforming, EDollVerdict Verdict) const
{
	const bool bApproved = (Verdict == EDollVerdict::Approve);

	if (bIsConforming && bApproved)   { return EVerdictCategory::TruePositive; }
	if (!bIsConforming && !bApproved) { return EVerdictCategory::TrueNegative; }
	if (!bIsConforming && bApproved)  { return EVerdictCategory::FalseNegative; }  // 漏放
	/* bIsConforming && !bApproved */ return EVerdictCategory::FalsePositive;       // 误丢（合规误判）
}

bool UJudgmentCardSubsystem::CheckTwistConditions(const UDollData* Doll, EDollVerdict Verdict, bool bIsConforming) const
{
	// 主路径：合规 + Pearl + 放行
	if (bIsConforming
		&& Doll->ButtonStyle == EButtonStyle::Pearl
		&& Verdict == EDollVerdict::Approve)
	{
		return true;
	}

	// 兜底路径：仅 Shift 4 的 fallback 娃娃，任一 verdict 都触发
	UCheckmateGameStateSubsystem* GS = nullptr;
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			GS = GI->GetSubsystem<UCheckmateGameStateSubsystem>();
		}
	}
	if (Doll->bIsFallback && GS && GS->GetCurrentShiftIndex() == 4)
	{
		return true;
	}

	return false;
}

void UJudgmentCardSubsystem::JC_PrintStandard()
{
	UE_LOG(LogTemp, Warning, TEXT("[STANDARD] %d cards:"), CurrentStandard.Num());
	for (const UCardData* C : CurrentStandard)
	{
		if (C)
		{
			UE_LOG(LogTemp, Warning, TEXT("  - %s (%s) Pearl=%s"),
				*C->CardId.ToString(),
				*UEnum::GetValueAsString(C->Dimension),
				C->bIsPearlCompatible ? TEXT("true") : TEXT("false"));
		}
	}
}
