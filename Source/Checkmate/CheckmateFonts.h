// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"

enum class ECheckmateFontWeight : uint8
{
	Light,
	Regular,
	Bold
};

namespace CheckmateFonts
{
	FSlateFontInfo Title(float Size);
	FSlateFontInfo Body(float Size, ECheckmateFontWeight Weight = ECheckmateFontWeight::Regular);
	FSlateFontInfo BodyLight(float Size);
	FSlateFontInfo BodyBold(float Size);
}
