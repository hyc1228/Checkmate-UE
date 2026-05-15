// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "DollData.h"

bool UDollData::GetAttributeByCardId(FName CardId) const
{
	// Posture 维度：CardId 与 EDollPosture 名称匹配
	if (CardId == TEXT("BalletPose"))    { return Posture == EDollPosture::BalletPose; }
	if (CardId == TEXT("Bowing"))        { return Posture == EDollPosture::Bowing; }
	if (CardId == TEXT("Upright"))       { return Posture == EDollPosture::Upright; }
	if (CardId == TEXT("SeatedProper"))  { return Posture == EDollPosture::SeatedProper; }

	// Hair / Expression / Accessory 维度
	if (CardId == TEXT("LongHair"))       { return bLongHair; }
	if (CardId == TEXT("NaturalColor"))   { return bNaturalColor; }
	if (CardId == TEXT("Styled"))         { return bStyled; }
	if (CardId == TEXT("Smile"))          { return bSmile; }
	if (CardId == TEXT("PearlNecklace"))  { return bPearlNecklace; }
	if (CardId == TEXT("Veil"))           { return bVeil; }
	if (CardId == TEXT("Apron"))          { return bApron; }
	if (CardId == TEXT("Ribbon"))         { return bRibbon; }
	if (CardId == TEXT("WhiteGloves"))    { return bWhiteGloves; }

	UE_LOG(LogTemp, Warning, TEXT("UDollData::GetAttributeByCardId: 未识别 CardId '%s'。检查卡定义与 13 属性对齐。"), *CardId.ToString());
	return false;
}
