// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "Ch1CardTooltipWidget.h"

#include "CardData.h"
#include "CheckmateFonts.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "SlateOptMacros.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	enum class ECh1TextEffect : uint8
	{
		None,
		Pendulum,
		Dangle,
		Fade,
		Rainbow,
		Rotate,
		Bounce,
		SlideHorizontal,
		Swing,
		Wave,
		Increase,
		Shake,
		Wiggle
	};

	struct FCh1AnimatedGlyph
	{
		FString Text;
		FLinearColor Color;
		ECh1TextEffect Effect = ECh1TextEffect::None;
		float Amplitude = 1.0f;
		float Speed = 1.0f;
	};

	struct FCh1LaidOutGlyph
	{
		FString Text;
		FVector2D Position = FVector2D::ZeroVector;
		FLinearColor Color;
		ECh1TextEffect Effect = ECh1TextEffect::None;
		float Amplitude = 1.0f;
		float Speed = 1.0f;
	};

	bool IsEffectTag(const FString& TagName, ECh1TextEffect& OutEffect)
	{
		if (TagName.Equals(TEXT("pend"), ESearchCase::IgnoreCase) || TagName.Equals(TEXT("pendulum"), ESearchCase::IgnoreCase))
		{
			OutEffect = ECh1TextEffect::Pendulum;
			return true;
		}
		if (TagName.Equals(TEXT("dangle"), ESearchCase::IgnoreCase))
		{
			OutEffect = ECh1TextEffect::Dangle;
			return true;
		}
		if (TagName.Equals(TEXT("fade"), ESearchCase::IgnoreCase))
		{
			OutEffect = ECh1TextEffect::Fade;
			return true;
		}
		if (TagName.Equals(TEXT("rainb"), ESearchCase::IgnoreCase) || TagName.Equals(TEXT("rainbow"), ESearchCase::IgnoreCase))
		{
			OutEffect = ECh1TextEffect::Rainbow;
			return true;
		}
		if (TagName.Equals(TEXT("rot"), ESearchCase::IgnoreCase) || TagName.Equals(TEXT("rotate"), ESearchCase::IgnoreCase))
		{
			OutEffect = ECh1TextEffect::Rotate;
			return true;
		}
		if (TagName.Equals(TEXT("bounce"), ESearchCase::IgnoreCase))
		{
			OutEffect = ECh1TextEffect::Bounce;
			return true;
		}
		if (TagName.Equals(TEXT("slideh"), ESearchCase::IgnoreCase) || TagName.Equals(TEXT("slide"), ESearchCase::IgnoreCase))
		{
			OutEffect = ECh1TextEffect::SlideHorizontal;
			return true;
		}
		if (TagName.Equals(TEXT("swing"), ESearchCase::IgnoreCase))
		{
			OutEffect = ECh1TextEffect::Swing;
			return true;
		}
		if (TagName.Equals(TEXT("wave"), ESearchCase::IgnoreCase))
		{
			OutEffect = ECh1TextEffect::Wave;
			return true;
		}
		if (TagName.Equals(TEXT("incr"), ESearchCase::IgnoreCase) || TagName.Equals(TEXT("increase"), ESearchCase::IgnoreCase))
		{
			OutEffect = ECh1TextEffect::Increase;
			return true;
		}
		if (TagName.Equals(TEXT("shake"), ESearchCase::IgnoreCase))
		{
			OutEffect = ECh1TextEffect::Shake;
			return true;
		}
		if (TagName.Equals(TEXT("wiggle"), ESearchCase::IgnoreCase))
		{
			OutEffect = ECh1TextEffect::Wiggle;
			return true;
		}
		return false;
	}

	float GetTagFloatValue(const TArray<FString>& Parts, const TCHAR* Key, float DefaultValue)
	{
		const FString Prefix = FString::Printf(TEXT("%s="), Key);
		for (const FString& Part : Parts)
		{
			if (Part.StartsWith(Prefix, ESearchCase::IgnoreCase))
			{
				return FCString::Atof(*Part.Mid(Prefix.Len()));
			}
		}
		return DefaultValue;
	}
}

class SCh1AnimatedMarkupText : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SCh1AnimatedMarkupText) {}
		SLATE_ARGUMENT(FText, Text)
		SLATE_ARGUMENT(FSlateFontInfo, Font)
		SLATE_ARGUMENT(FLinearColor, DefaultColor)
		SLATE_ARGUMENT(FLinearColor, EmphasisColor)
		SLATE_ARGUMENT(float, WrapWidth)
		SLATE_ARGUMENT(float, ShakeAmplitude)
		SLATE_ARGUMENT(float, ShakeSpeed)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Text = InArgs._Text;
		Font = InArgs._Font;
		DefaultColor = InArgs._DefaultColor;
		EmphasisColor = InArgs._EmphasisColor;
		WrapWidth = InArgs._WrapWidth;
		ShakeAmplitude = InArgs._ShakeAmplitude;
		ShakeSpeed = InArgs._ShakeSpeed;
		StartTime = FSlateApplication::Get().GetCurrentTime();

		RegisterActiveTimer(0.0f, FWidgetActiveTimerDelegate::CreateSP(this, &SCh1AnimatedMarkupText::TickAnimation));
	}

	void SetText(FText InText)
	{
		Text = InText;
		Invalidate(EInvalidateWidgetReason::PaintAndVolatility);
	}

	void SetStyle(FSlateFontInfo InFont, FLinearColor InDefaultColor, FLinearColor InEmphasisColor, float InWrapWidth, float InShakeAmplitude, float InShakeSpeed)
	{
		Font = InFont;
		DefaultColor = InDefaultColor;
		EmphasisColor = InEmphasisColor;
		WrapWidth = InWrapWidth;
		ShakeAmplitude = InShakeAmplitude;
		ShakeSpeed = InShakeSpeed;
		Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		TArray<FCh1LaidOutGlyph> Glyphs;
		float DesiredHeight = 0.0f;
		BuildLayout(AllottedGeometry.GetLocalSize().X, Glyphs, DesiredHeight);

		const float Now = static_cast<float>(FSlateApplication::Get().GetCurrentTime() - StartTime);
		int32 GlyphIndex = 0;
		for (const FCh1LaidOutGlyph& Glyph : Glyphs)
		{
			FVector2D DrawPosition = Glyph.Position;
			FLinearColor DrawColor = Glyph.Color;
			FSlateFontInfo DrawFont = Font;
			const float Phase = Now * Glyph.Speed + GlyphIndex * 0.73f;

			switch (Glyph.Effect)
			{
			case ECh1TextEffect::Pendulum:
				DrawPosition.X += FMath::Sin(Phase) * Glyph.Amplitude;
				DrawPosition.Y += FMath::Cos(Phase * 0.82f) * Glyph.Amplitude * 0.35f;
				break;
			case ECh1TextEffect::Dangle:
				DrawPosition.X += FMath::Sin(Phase * 0.72f) * Glyph.Amplitude * 0.55f;
				DrawPosition.Y += FMath::Abs(FMath::Sin(Phase)) * Glyph.Amplitude * 0.75f;
				break;
			case ECh1TextEffect::Fade:
				DrawColor.A *= FMath::Lerp(0.32f, 1.0f, 0.5f + 0.5f * FMath::Sin(Phase));
				break;
			case ECh1TextEffect::Rainbow:
				DrawColor = FLinearColor::MakeFromHSV8(static_cast<uint8>(FMath::Fmod(Now * Glyph.Speed * 18.0f + GlyphIndex * 16.0f, 255.0f)), 190, 255);
				DrawColor.A = Glyph.Color.A;
				break;
			case ECh1TextEffect::Rotate:
				DrawPosition.X += FMath::Sin(Phase) * Glyph.Amplitude * 0.35f;
				DrawPosition.Y += FMath::Cos(Phase) * Glyph.Amplitude * 0.35f;
				break;
			case ECh1TextEffect::Bounce:
				DrawPosition.Y -= FMath::Abs(FMath::Sin(Phase)) * Glyph.Amplitude;
				break;
			case ECh1TextEffect::SlideHorizontal:
				DrawPosition.X += FMath::Sin(Phase) * Glyph.Amplitude;
				break;
			case ECh1TextEffect::Swing:
				DrawPosition.X += FMath::Sin(Phase) * Glyph.Amplitude * 0.75f;
				DrawPosition.Y += FMath::Sin(Phase * 0.5f) * Glyph.Amplitude * 0.25f;
				break;
			case ECh1TextEffect::Wave:
				DrawPosition.Y += FMath::Sin(Phase) * Glyph.Amplitude;
				break;
			case ECh1TextEffect::Increase:
				DrawFont.Size = FMath::Max(1, FMath::RoundToInt(static_cast<float>(Font.Size) * (1.0f + FMath::Max(0.0f, FMath::Sin(Phase)) * Glyph.Amplitude * 0.035f)));
				break;
			case ECh1TextEffect::Shake:
				DrawPosition.X += FMath::Sin(Phase) * Glyph.Amplitude;
				DrawPosition.Y += FMath::Cos(Phase * 1.31f) * Glyph.Amplitude;
				break;
			case ECh1TextEffect::Wiggle:
				DrawPosition.X += FMath::Sin(Phase * 1.6f) * Glyph.Amplitude * 0.65f;
				DrawPosition.Y += FMath::Cos(Phase * 1.15f) * Glyph.Amplitude * 0.65f;
				break;
			case ECh1TextEffect::None:
			default:
				break;
			}

			FSlateDrawElement::MakeText(
				OutDrawElements,
				LayerId,
				AllottedGeometry.ToPaintGeometry(Measure(Glyph.Text), FSlateLayoutTransform(DrawPosition)),
				Glyph.Text,
				DrawFont,
				ESlateDrawEffect::None,
				DrawColor);
			++GlyphIndex;
		}

		return LayerId + 1;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		TArray<FCh1LaidOutGlyph> Glyphs;
		float DesiredHeight = 0.0f;
		BuildLayout(WrapWidth, Glyphs, DesiredHeight);
		return FVector2D(WrapWidth, DesiredHeight);
	}

	virtual bool ComputeVolatility() const override
	{
		return true;
	}

private:
	FText Text;
	FSlateFontInfo Font;
	FLinearColor DefaultColor = FLinearColor::White;
	FLinearColor EmphasisColor = FLinearColor::Red;
	float WrapWidth = 330.0f;
	float ShakeAmplitude = 1.8f;
	float ShakeSpeed = 23.0f;
	double StartTime = 0.0;

	EActiveTimerReturnType TickAnimation(double, float)
	{
		Invalidate(EInvalidateWidgetReason::Paint);
		return EActiveTimerReturnType::Continue;
	}

	FVector2D Measure(const FString& InText) const
	{
		const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		return FontMeasure->Measure(InText, Font);
	}

	TArray<FCh1AnimatedGlyph> ParseMarkup() const
	{
		TArray<FCh1AnimatedGlyph> Result;
		const FString Source = Text.ToString();
		bool bRed = false;
		ECh1TextEffect ActiveEffect = ECh1TextEffect::None;
		float ActiveAmplitude = ShakeAmplitude;
		float ActiveSpeed = ShakeSpeed;

		for (int32 Index = 0; Index < Source.Len();)
		{
			if (Source[Index] == TEXT('<'))
			{
				const int32 ClosingIndex = Source.Find(TEXT(">"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Index);
				if (ClosingIndex != INDEX_NONE && ClosingIndex > Index)
				{
					FString TagBody = Source.Mid(Index + 1, ClosingIndex - Index - 1).TrimStartAndEnd();
					const bool bClosing = TagBody.RemoveFromStart(TEXT("/"));
					TagBody.TrimStartAndEndInline();

					if (TagBody.IsEmpty())
					{
						bRed = false;
						ActiveEffect = ECh1TextEffect::None;
						ActiveAmplitude = ShakeAmplitude;
						ActiveSpeed = ShakeSpeed;
						Index = ClosingIndex + 1;
						continue;
					}

					TArray<FString> Parts;
					TagBody.ParseIntoArray(Parts, TEXT(" "), true);
					if (Parts.Num() > 0)
					{
						const FString TagName = Parts[0];
						ECh1TextEffect ParsedEffect = ECh1TextEffect::None;
						if (TagName.Equals(TEXT("red"), ESearchCase::IgnoreCase))
						{
							bRed = !bClosing;
							Index = ClosingIndex + 1;
							continue;
						}
						if (IsEffectTag(TagName, ParsedEffect))
						{
							if (bClosing)
							{
								ActiveEffect = ECh1TextEffect::None;
								ActiveAmplitude = ShakeAmplitude;
								ActiveSpeed = ShakeSpeed;
							}
							else
							{
								ActiveEffect = ParsedEffect;
								ActiveAmplitude = FMath::Max(0.0f, GetTagFloatValue(Parts, TEXT("a"), ShakeAmplitude));
								ActiveSpeed = FMath::Max(0.1f, GetTagFloatValue(Parts, TEXT("s"), ShakeSpeed));
							}
							Index = ClosingIndex + 1;
							continue;
						}
					}
				}
			}

			FCh1AnimatedGlyph Glyph;
			Glyph.Text = Source.Mid(Index, 1);
			Glyph.Color = bRed ? EmphasisColor : DefaultColor;
			Glyph.Effect = ActiveEffect;
			Glyph.Amplitude = ActiveAmplitude;
			Glyph.Speed = ActiveSpeed;
			Result.Add(Glyph);
			++Index;
		}
		return Result;
	}

	void BuildLayout(float InAvailableWidth, TArray<FCh1LaidOutGlyph>& OutGlyphs, float& OutHeight) const
	{
		const float SafeWrapWidth = FMath::Max(80.0f, FMath::Min(WrapWidth, InAvailableWidth > 1.0f ? InAvailableWidth : WrapWidth));
		const FVector2D LineMeasure = Measure(TEXT("Ay"));
		const float LineHeight = FMath::Max(18.0f, LineMeasure.Y + 4.0f);
		float X = 0.0f;
		float Y = 0.0f;

		for (const FCh1AnimatedGlyph& Glyph : ParseMarkup())
		{
			if (Glyph.Text == TEXT("\n"))
			{
				X = 0.0f;
				Y += LineHeight;
				continue;
			}

			const FVector2D GlyphSize = Measure(Glyph.Text);
			if (X > 0.0f && X + GlyphSize.X > SafeWrapWidth && Glyph.Text != TEXT(" "))
			{
				X = 0.0f;
				Y += LineHeight;
			}

			FCh1LaidOutGlyph LaidOut;
			LaidOut.Text = Glyph.Text;
			LaidOut.Position = FVector2D(X, Y);
			LaidOut.Color = Glyph.Color;
			LaidOut.Effect = Glyph.Effect;
			LaidOut.Amplitude = Glyph.Amplitude;
			LaidOut.Speed = Glyph.Speed;
			OutGlyphs.Add(LaidOut);
			X += GlyphSize.X;
		}

		OutHeight = Y + LineHeight;
	}
};

TSharedRef<SWidget> UCh1CardTooltipWidget::RebuildWidget()
{
	TitleFont = TitleFont.HasValidFont() ? TitleFont : CheckmateFonts::Title(18);
	BodyFont = BodyFont.HasValidFont() ? BodyFont : CheckmateFonts::Body(13);

	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.035f, 0.030f, 0.026f, 0.94f))
		.Padding(FMargin(14.0f, 10.0f))
		[
			SNew(SBox)
			.WidthOverride(BodyWrapWidth + 4.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(SlateTitleText, STextBlock)
					.Text(TitleText)
					.Font(TitleFont)
					.ColorAndOpacity(TitleColor)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 7.0f, 0.0f, 8.0f)
				[
					SNew(SSeparator)
					.Thickness(1.0f)
					.ColorAndOpacity(FLinearColor(0.55f, 0.10f, 0.09f, 0.55f))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(SlateBodyText, SCh1AnimatedMarkupText)
					.Text(BodyMarkupText)
					.Font(BodyFont)
					.DefaultColor(BodyColor)
					.EmphasisColor(EmphasisColor)
					.WrapWidth(BodyWrapWidth)
					.ShakeAmplitude(ShakeAmplitude)
					.ShakeSpeed(ShakeSpeed)
				]
			]
		];
}

void UCh1CardTooltipWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (SlateTitleText)
	{
		SlateTitleText->SetText(TitleText);
		SlateTitleText->SetFont(TitleFont);
		SlateTitleText->SetColorAndOpacity(TitleColor);
	}
	if (SlateBodyText)
	{
		SlateBodyText->SetText(BodyMarkupText);
		SlateBodyText->SetStyle(BodyFont, BodyColor, EmphasisColor, BodyWrapWidth, ShakeAmplitude, ShakeSpeed);
	}
}

void UCh1CardTooltipWidget::SetCardData(const UCardData* InCardData)
{
	if (!InCardData)
	{
		TitleText = FText::GetEmpty();
		BodyMarkupText = FText::GetEmpty();
	}
	else
	{
		TitleText = !InCardData->EnglishDisplayName.IsEmpty()
			? InCardData->EnglishDisplayName
			: InCardData->DisplayLabel;
		BodyMarkupText = InCardData->SkillDescriptionMarkup;
	}

	SynchronizeProperties();
}
