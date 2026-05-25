// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "JudgmentEvaluator.h"

#include "CardData.h"
#include "DollData.h"

EJudgmentVerdict UJudgmentEvaluator::EvaluateDoll(
	const TArray<UCardData*>& SelectedCards,
	const UDollData* DollData,
	FString& OutFailReason)
{
	OutFailReason.Reset();

	if (!DollData)
	{
		OutFailReason = TEXT("No doll data");
		return EJudgmentVerdict::Reject;
	}

	for (const UCardData* Card : SelectedCards)
	{
		if (!Card) continue;

		switch (Card->Dimension)
		{
		case ECardDimension::Posture:
			if (DollData->Posture != Card->PostureValue)
			{
				OutFailReason = FString::Printf(
					TEXT("姿态不符：娃娃=%s / 要求=%s"),
					*DollData->Posture.ToString(),
					*Card->PostureValue.ToString()
				);
				return EJudgmentVerdict::Reject;
			}
			break;

		case ECardDimension::Hair:
			if (!DollData->HairTraits.Contains(Card->CardId))
			{
				OutFailReason = FString::Printf(
					TEXT("缺发型特征：%s"),
					*Card->CardId.ToString()
				);
				return EJudgmentVerdict::Reject;
			}
			break;

		case ECardDimension::Expression:
			if (!DollData->ExpressionTraits.Contains(Card->CardId))
			{
				OutFailReason = FString::Printf(
					TEXT("缺表情特征：%s"),
					*Card->CardId.ToString()
				);
				return EJudgmentVerdict::Reject;
			}
			break;

		case ECardDimension::Accessory:
			if (!DollData->AccessoryTraits.Contains(Card->CardId))
			{
				OutFailReason = FString::Printf(
					TEXT("缺饰物特征：%s"),
					*Card->CardId.ToString()
				);
				return EJudgmentVerdict::Reject;
			}
			break;
		}
	}

	return EJudgmentVerdict::Pass;
}

EOutcomeClass UJudgmentEvaluator::ClassifyOutcome(EJudgmentVerdict ObjectiveVerdict, bool bPlayerAcceptedDoll)
{
	const bool bObjectivePass = (ObjectiveVerdict == EJudgmentVerdict::Pass);
	if (bPlayerAcceptedDoll && bObjectivePass)   return EOutcomeClass::TruePositive;
	if (!bPlayerAcceptedDoll && !bObjectivePass) return EOutcomeClass::TrueNegative;
	if (bPlayerAcceptedDoll && !bObjectivePass)  return EOutcomeClass::FalsePositive;
	return EOutcomeClass::FalseNegative;
}

bool UJudgmentEvaluator::IsMisjudgment(EOutcomeClass Outcome)
{
	return Outcome == EOutcomeClass::FalsePositive || Outcome == EOutcomeClass::FalseNegative;
}
