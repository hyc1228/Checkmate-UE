// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "JudgmentEvaluator.h"

#include "CardData.h"
#include "DollData.h"

#include <initializer_list>

namespace
{
	bool ContainsAnyToken(const FString& Source, std::initializer_list<const TCHAR*> Tokens)
	{
		for (const TCHAR* Token : Tokens)
		{
			if (Source.Contains(Token, ESearchCase::IgnoreCase))
			{
				return true;
			}
		}
		return false;
	}

	FString BuildDollIdentityString(const UDollData* DollData)
	{
		if (!DollData)
		{
			return FString();
		}

		FString Identity = DollData->GetName();
		if (!DollData->DollId.IsNone())
		{
			Identity += TEXT(" ");
			Identity += DollData->DollId.ToString();
		}
		if (!DollData->DisplayName.IsEmpty())
		{
			Identity += TEXT(" ");
			Identity += DollData->DisplayName.ToString();
		}
		return Identity;
	}

	bool TryInferBaseTraitFromDollName(const UDollData* DollData, FName TraitId, bool& bOutValue)
	{
		const FString Identity = BuildDollIdentityString(DollData);
		if (Identity.IsEmpty())
		{
			return false;
		}

		if (TraitId == TEXT("Smile"))
		{
			if (ContainsAnyToken(Identity, { TEXT("NoSmile"), TEXT("Sad") }))
			{
				bOutValue = false;
				return true;
			}
			if (ContainsAnyToken(Identity, { TEXT("Smile"), TEXT("Perfect") }))
			{
				bOutValue = true;
				return true;
			}
		}

		if (TraitId == TEXT("PoseConforming"))
		{
			if (ContainsAnyToken(Identity, { TEXT("Bad"), TEXT("Flawed"), TEXT("Failing") }))
			{
				bOutValue = false;
				return true;
			}
			if (ContainsAnyToken(Identity, { TEXT("Dance"), TEXT("Ballet"), TEXT("Upright"), TEXT("Perfect") }))
			{
				bOutValue = true;
				return true;
			}
		}

		if (TraitId == TEXT("NaturalColor"))
		{
			if (ContainsAnyToken(Identity, { TEXT("Unnatural"), TEXT("ColorBad"), TEXT("HairBad") }))
			{
				bOutValue = false;
				return true;
			}
			if (DollData->HairTraits.Num() == 0)
			{
				bOutValue = true;
				return true;
			}
		}

		return false;
	}
}

EJudgmentVerdict UJudgmentEvaluator::EvaluateDoll(
	const TArray<UCardData*>& SelectedCards,
	const UDollData* DollData,
	FString& OutFailReason)
{
	OutFailReason.Reset();

	if (!DollPassesBaseStandard(DollData, OutFailReason))
	{
		return EJudgmentVerdict::Reject;
	}

	for (const UCardData* Card : SelectedCards)
	{
		if (!Card || Card->CardColor == ECh1CardColor::RedBonus)
		{
			continue;
		}

		if (!DollMatchesCardCriterion(DollData, Card, OutFailReason))
		{
			return EJudgmentVerdict::Reject;
		}
	}

	return EJudgmentVerdict::Pass;
}

bool UJudgmentEvaluator::DollPassesBaseStandard(const UDollData* DollData, FString& OutFailReason)
{
	OutFailReason.Reset();
	if (!DollData)
	{
		OutFailReason = TEXT("No doll data");
		return false;
	}

	if (!DollHasPieceworkTrait(DollData, TEXT("Smile")))
	{
		OutFailReason = TEXT("Base standard failed: Smile");
		return false;
	}
	if (!DollHasPieceworkTrait(DollData, TEXT("PoseConforming")))
	{
		OutFailReason = TEXT("Base standard failed: Pose");
		return false;
	}
	if (!DollHasPieceworkTrait(DollData, TEXT("NaturalColor")))
	{
		OutFailReason = TEXT("Base standard failed: Natural hair color");
		return false;
	}

	return true;
}

bool UJudgmentEvaluator::DollMatchesCardCriterion(const UDollData* DollData, const UCardData* Card, FString& OutFailReason)
{
	OutFailReason.Reset();
	if (!DollData || !Card)
	{
		OutFailReason = TEXT("Missing criterion data");
		return false;
	}

	switch (Card->PieceworkEffect)
	{
	case ECh1CardEffect::BowDiscipline:
		if (!DollHasPieceworkTrait(DollData, TEXT("Ribbon")))
		{
			OutFailReason = TEXT("Black card failed: Bow Discipline");
			return false;
		}
		return true;

	case ECh1CardEffect::HeartGaze:
		if (!DollHasPieceworkTrait(DollData, TEXT("HeartEye")))
		{
			OutFailReason = TEXT("Black card failed: Heart Gaze");
			return false;
		}
		return true;

	case ECh1CardEffect::RoseGaze:
		if (!DollHasPieceworkTrait(DollData, TEXT("RoseEye")))
		{
			OutFailReason = TEXT("Black card failed: Rose Gaze");
			return false;
		}
		return true;

	case ECh1CardEffect::TheGaze:
		if (!DollHasPieceworkTrait(DollData, TEXT("PoseConforming")))
		{
			OutFailReason = TEXT("Black card failed: The Gaze");
			return false;
		}
		return true;

	case ECh1CardEffect::DangerousTears:
		if (DollHasPieceworkTrait(DollData, TEXT("Tearful")))
		{
			OutFailReason = TEXT("Black card failed: Dangerous Tears");
			return false;
		}
		return true;

	case ECh1CardEffect::SmilingThroughTears:
		if (DollHasPieceworkTrait(DollData, TEXT("Smile")) && DollHasPieceworkTrait(DollData, TEXT("Tearful")))
		{
			OutFailReason = TEXT("Black card failed: Smiling Through Tears");
			return false;
		}
		return true;

	case ECh1CardEffect::WhiteGloves:
		if (!DollHasPieceworkTrait(DollData, TEXT("WhiteGloves")))
		{
			OutFailReason = TEXT("Black card failed: White Gloves");
			return false;
		}
		return true;

	case ECh1CardEffect::PearlStandard:
		if (!DollHasPieceworkTrait(DollData, TEXT("PearlNecklace")))
		{
			OutFailReason = TEXT("Black card failed: Pearl Standard");
			return false;
		}
		return true;

	case ECh1CardEffect::ApronDuty:
		if (!DollHasPieceworkTrait(DollData, TEXT("Apron")))
		{
			OutFailReason = TEXT("Black card failed: Apron Duty");
			return false;
		}
		return true;

	case ECh1CardEffect::QueenGambit:
	case ECh1CardEffect::SwordAndSerpent:
	case ECh1CardEffect::CheckmateDiscard:
		return true;

	default:
		break;
	}

	switch (Card->Dimension)
	{
	case ECardDimension::Posture:
		if (DollData->Posture != Card->PostureValue)
		{
			OutFailReason = FString::Printf(TEXT("Criterion failed: Posture %s requires %s"),
				*DollData->Posture.ToString(),
				*Card->PostureValue.ToString());
			return false;
		}
		return true;

	case ECardDimension::Hair:
	case ECardDimension::Expression:
	case ECardDimension::Accessory:
		if (!DollHasPieceworkTrait(DollData, Card->CardId))
		{
			OutFailReason = FString::Printf(TEXT("Criterion failed: %s"), *Card->CardId.ToString());
			return false;
		}
		return true;
	}

	return true;
}

bool UJudgmentEvaluator::DollHasPieceworkTrait(const UDollData* DollData, FName TraitId)
{
	if (!DollData || TraitId.IsNone())
	{
		return false;
	}

	if (DollData->bUseExplicitPieceworkTraits)
	{
		if (TraitId == TEXT("Smile")) return DollData->bSmile;
		if (TraitId == TEXT("PoseConforming")) return DollData->bPoseConforming;
		if (TraitId == TEXT("NaturalColor")) return DollData->bNaturalColor;
		if (TraitId == TEXT("LongHair")) return DollData->bLongHair;
		if (TraitId == TEXT("Styled")) return DollData->bStyled;
		if (TraitId == TEXT("PearlNecklace")) return DollData->bPearlNecklace;
		if (TraitId == TEXT("Veil")) return DollData->bVeil;
		if (TraitId == TEXT("Apron")) return DollData->bApron;
		if (TraitId == TEXT("Ribbon")) return DollData->bRibbon;
		if (TraitId == TEXT("WhiteGloves")) return DollData->bWhiteGloves;
		if (TraitId == TEXT("Tearful")) return DollData->bTearful;
		if (TraitId == TEXT("RoseAdornment")) return DollData->bRoseAdornment;
		if (TraitId == TEXT("HighHeels")) return DollData->bHighHeels;
		if (TraitId == TEXT("HeartEye")) return DollData->ButtonStyle == EButtonEyeStyle::Heart;
		if (TraitId == TEXT("RoseEye")) return DollData->ButtonStyle == EButtonEyeStyle::Rose;
		if (TraitId == TEXT("PearlEye")) return DollData->ButtonStyle == EButtonEyeStyle::Pearl;
	}

	bool bInferredValue = false;
	if (TryInferBaseTraitFromDollName(DollData, TraitId, bInferredValue))
	{
		return bInferredValue;
	}

	if (TraitId == TEXT("Smile")) return DollData->ExpressionTraits.Contains(TEXT("Smile"));
	if (TraitId == TEXT("PoseConforming"))
	{
		return DollData->Posture == TEXT("BalletPose")
			|| DollData->Posture == TEXT("Upright")
			|| DollData->Posture == TEXT("SeatedProper");
	}
	if (TraitId == TEXT("NaturalColor")) return DollData->HairTraits.Contains(TEXT("NaturalColor"));
	if (TraitId == TEXT("HeartEye")) return DollData->ButtonStyle == EButtonEyeStyle::Heart || DollData->AccessoryTraits.Contains(TEXT("HeartEye"));
	if (TraitId == TEXT("RoseEye")) return DollData->ButtonStyle == EButtonEyeStyle::Rose || DollData->AccessoryTraits.Contains(TEXT("RoseEye"));
	if (TraitId == TEXT("PearlEye")) return DollData->ButtonStyle == EButtonEyeStyle::Pearl || DollData->AccessoryTraits.Contains(TEXT("PearlEye"));

	return DollData->HairTraits.Contains(TraitId)
		|| DollData->ExpressionTraits.Contains(TraitId)
		|| DollData->AccessoryTraits.Contains(TraitId);
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
