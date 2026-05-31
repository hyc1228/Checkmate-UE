// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "CardData.h"

namespace
{
	FName NormalizeCh1CardId(FName InCardId)
	{
		FString Id = InCardId.ToString();
		Id.RemoveFromStart(TEXT("DA_Card_"));
		Id.RemoveFromStart(TEXT("Card_"));
		Id.RemoveFromStart(TEXT("T_Card_"));
		Id.RemoveFromStart(TEXT("MI_Card_"));
		Id.RemoveFromStart(TEXT("M_Card_"));
		return FName(*Id);
	}

	struct FCh1CardPreset
	{
		FName CardId;
		const TCHAR* EnglishName;
		const TCHAR* SkillDescription;
		ECh1CardColor Color;
		ECh1CardEffect Effect;
		ECardDimension Dimension;
		float GoodValueMultiplier = 1.0f;
		int32 FlatGoodValueBonus = 0;
		int32 CorrectRejectBonus = 0;
		bool bNegativeLaser = false;
		float NegativeLaserMultiplier = 1.0f;
	};

	const FCh1CardPreset* FindCh1CardPreset(FName InCardId)
	{
		static const FCh1CardPreset Presets[] =
		{
			{ TEXT("Empty"),           TEXT("Card Empty"),        TEXT("Placeholder. Takes a slot, but changes <fade a=1 s=3><red>nothing</red></fade>."), ECh1CardColor::RedBonus,       ECh1CardEffect::None,            ECardDimension::Accessory },
			{ TEXT("PriceTag"),        TEXT("Price Tag"),         TEXT("Correctly accepted dolls sell for <bounce a=3 s=9><red>+20%</red></bounce>."), ECh1CardColor::RedBonus,       ECh1CardEffect::PriceTag,        ECardDimension::Accessory },
			{ TEXT("HighHeels"),       TEXT("High Heels"),        TEXT("High-heel dolls sell for <incr a=3 s=8><red>x1.5</red></incr> when accepted correctly."), ECh1CardColor::RedBonus,       ECh1CardEffect::HighHeelsValue,  ECardDimension::Accessory },
			{ TEXT("BellGirl"),        TEXT("Bell Girl"),         TEXT("Correct streaks grow faster: extra <slideh a=2 s=8><red>+5% per Combo</red></slideh>."), ECh1CardColor::RedBonus,       ECh1CardEffect::ComboDiscipline, ECardDimension::Accessory },
			{ TEXT("RosePremium"),     TEXT("Rose Premium"),      TEXT("Rose-eye dolls sell for <rainb a=2 s=7>x1.5</rainb> when accepted correctly."), ECh1CardColor::RedBonus,       ECh1CardEffect::LockedRose,      ECardDimension::Accessory },
			{ TEXT("HeartGaze"),       TEXT("Heart Gaze"),        TEXT("Black criterion: the doll must have <wave a=2 s=8><red>Heart Eyes</red></wave>. Matching dolls sell for <red>x1.35</red>."), ECh1CardColor::BlackCriterion, ECh1CardEffect::HeartGaze,       ECardDimension::Expression },
			{ TEXT("NoTears"),         TEXT("No Tears"),          TEXT("Black criterion: <shake a=2.2 s=24><red>tearful dolls fail</red></shake>. Correct rejects gain extra value; wrong accepts hurt more."), ECh1CardColor::BlackCriterion, ECh1CardEffect::DangerousTears,  ECardDimension::Expression },
			{ TEXT("RoseGaze"),        TEXT("Rose Gaze"),         TEXT("Black criterion: the doll must have <rainb a=2 s=7>Rose Eyes</rainb>. Matching dolls sell for <red>x1.5</red>."), ECh1CardColor::BlackCriterion, ECh1CardEffect::RoseGaze,        ECardDimension::Expression },
			{ TEXT("BowDiscipline"),   TEXT("Bow Discipline"),    TEXT("Black criterion: the doll must wear a <wiggle a=2 s=10><red>bow</red></wiggle>. Matching dolls sell for <red>x1.3</red>."), ECh1CardColor::BlackCriterion, ECh1CardEffect::BowDiscipline,   ECardDimension::Accessory },
			{ TEXT("SwordAndSerpent"), TEXT("Sword and Serpent"), TEXT("Holographic foil card. Built-in <red>x1.15</red>. At Combo 5, the next correct accept sells for <incr a=4 s=8><red>x2</red></incr>."), ECh1CardColor::Special,        ECh1CardEffect::SwordAndSerpent, ECardDimension::Accessory, 1.0f, 0, 0, true, 1.15f },
			{ TEXT("QueensGambit"),    TEXT("Queen's Gambit"),    TEXT("Holographic foil card. Built-in <red>x1.25</red>. Correct accepts sell for <red>x1.75</red>, but false accepts trigger <pend a=2.4 s=8><red>x1.35 recall penalty</red></pend>."), ECh1CardColor::Special,        ECh1CardEffect::QueenGambit,     ECardDimension::Accessory, 1.0f, 0, 0, true, 1.25f },
			{ TEXT("QueenGambit"),     TEXT("Queen's Gambit"),    TEXT("Holographic foil card. Built-in <red>x1.25</red>. Correct accepts sell for <red>x1.75</red>, but false accepts trigger <pend a=2.4 s=8><red>x1.35 recall penalty</red></pend>."), ECh1CardColor::Special,        ECh1CardEffect::QueenGambit,     ECardDimension::Accessory, 1.0f, 0, 0, true, 1.25f },
			{ TEXT("Checkmate"),       TEXT("Checkmate"),         TEXT("Holographic foil card. Built-in <red>x1.40</red>. Correct rejects gain <red>x1.5</red>; every third reject adds <wave a=3 s=8><red>+40</red></wave>."), ECh1CardColor::Special,        ECh1CardEffect::CheckmateDiscard,ECardDimension::Accessory, 1.0f, 0, 0, true, 1.40f },
		};

		const FName NormalizedId = NormalizeCh1CardId(InCardId);
		for (const FCh1CardPreset& Preset : Presets)
		{
			if (Preset.CardId == NormalizedId)
			{
				return &Preset;
			}
		}
		return nullptr;
	}
}

bool UCardData::ApplyCh1SlicePreset()
{
	const FName SourceId = CardId.IsNone() ? FName(*GetName()) : CardId;
	const FName NormalizedId = NormalizeCh1CardId(SourceId);
	const FCh1CardPreset* Preset = FindCh1CardPreset(NormalizedId);
	if (!Preset)
	{
		return false;
	}

	Modify();
	CardId = Preset->CardId == TEXT("QueenGambit") ? FName(TEXT("QueensGambit")) : Preset->CardId;
	Dimension = Preset->Dimension;
	DisplayLabel = FText::FromString(Preset->EnglishName);
	EnglishDisplayName = FText::FromString(Preset->EnglishName);
	SkillDescriptionMarkup = FText::FromString(Preset->SkillDescription);
	CardColor = Preset->Color;
	PieceworkEffect = Preset->Effect;
	GoodValueMultiplier = Preset->GoodValueMultiplier;
	FlatGoodValueBonus = Preset->FlatGoodValueBonus;
	CorrectRejectBonus = Preset->CorrectRejectBonus;
	bUseNegativeLaserFoil = Preset->bNegativeLaser;
	NegativeLaserScoreMultiplier = Preset->NegativeLaserMultiplier;
	NegativeLaserVisualStrength = Preset->bNegativeLaser ? 0.92f : 0.0f;
	PostureValue = NAME_None;
	bIsPearlCompatible = false;
	MarkPackageDirty();
	return true;
}

bool UCardData::GetCh1SlicePreset(FName InCardId, FText& OutEnglishName, ECh1CardColor& OutColor, ECh1CardEffect& OutEffect)
{
	const FCh1CardPreset* Preset = FindCh1CardPreset(InCardId);
	if (!Preset)
	{
		OutEnglishName = FText::GetEmpty();
		OutColor = ECh1CardColor::BlackCriterion;
		OutEffect = ECh1CardEffect::None;
		return false;
	}

	OutEnglishName = FText::FromString(Preset->EnglishName);
	OutColor = Preset->Color;
	OutEffect = Preset->Effect;
	return true;
}
