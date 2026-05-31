// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "CheckmateFonts.h"

#include "Fonts/CompositeFont.h"
#include "Misc/Paths.h"

namespace
{
	const FName FontRegularName(TEXT("Regular"));
	const FName FontLightName(TEXT("Light"));
	const FName FontBoldName(TEXT("Bold"));

	FString FontPath(const TCHAR* FileName)
	{
		return FPaths::ConvertRelativePathToFull(
			FPaths::ProjectDir() / TEXT("Source/Checkmate/Fonts") / FileName);
	}

	TSharedRef<const FCompositeFont> MakeSingleFaceFont(const TCHAR* FileName)
	{
		TSharedRef<FStandaloneCompositeFont> Font = MakeShared<FStandaloneCompositeFont>();
		Font->DefaultTypeface.AppendFont(
			FontRegularName,
			FontPath(FileName),
			EFontHinting::Default,
			EFontLoadingPolicy::LazyLoad);
		return StaticCastSharedRef<const FCompositeFont>(Font);
	}

	TSharedRef<const FCompositeFont> MakeSourceSansFont()
	{
		TSharedRef<FStandaloneCompositeFont> Font = MakeShared<FStandaloneCompositeFont>();
		Font->DefaultTypeface
			.AppendFont(FontLightName, FontPath(TEXT("SourceSans3-Light.ttf")), EFontHinting::Default, EFontLoadingPolicy::LazyLoad)
			.AppendFont(FontRegularName, FontPath(TEXT("SourceSans3-Regular.ttf")), EFontHinting::Default, EFontLoadingPolicy::LazyLoad)
			.AppendFont(FontBoldName, FontPath(TEXT("SourceSans3-Bold.ttf")), EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
		return StaticCastSharedRef<const FCompositeFont>(Font);
	}

	TSharedRef<const FCompositeFont> TitleFont()
	{
		static TSharedRef<const FCompositeFont> Font = MakeSingleFaceFont(TEXT("PermanentMarker-Regular.ttf"));
		return Font;
	}

	TSharedRef<const FCompositeFont> BodyFont()
	{
		static TSharedRef<const FCompositeFont> Font = MakeSourceSansFont();
		return Font;
	}

	FName TypefaceName(ECheckmateFontWeight Weight)
	{
		switch (Weight)
		{
		case ECheckmateFontWeight::Light:
			return FontLightName;
		case ECheckmateFontWeight::Bold:
			return FontBoldName;
		default:
			return FontRegularName;
		}
	}
}

FSlateFontInfo CheckmateFonts::Title(float Size)
{
	return FSlateFontInfo(TitleFont(), Size, FontRegularName);
}

FSlateFontInfo CheckmateFonts::Body(float Size, ECheckmateFontWeight Weight)
{
	return FSlateFontInfo(BodyFont(), Size, TypefaceName(Weight));
}

FSlateFontInfo CheckmateFonts::BodyLight(float Size)
{
	return Body(Size, ECheckmateFontWeight::Light);
}

FSlateFontInfo CheckmateFonts::BodyBold(float Size)
{
	return Body(Size, ECheckmateFontWeight::Bold);
}
