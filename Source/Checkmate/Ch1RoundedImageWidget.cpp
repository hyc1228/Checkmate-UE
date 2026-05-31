// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "Ch1RoundedImageWidget.h"

#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

class SCh1RoundedImage : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SCh1RoundedImage) {}
		SLATE_ARGUMENT(UTexture2D*, Texture)
		SLATE_ARGUMENT(FLinearColor, TintColor)
		SLATE_ARGUMENT(float, CornerRadiusPx)
		SLATE_ARGUMENT(int32, CornerSegments)
		SLATE_ARGUMENT(float, NegativeLaserStrength)
		SLATE_ARGUMENT(float, NegativeLaserDarkness)
		SLATE_ARGUMENT(FVector2D, LaserTilt)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Texture = InArgs._Texture;
		TintColor = InArgs._TintColor;
		CornerRadiusPx = InArgs._CornerRadiusPx;
		CornerSegments = InArgs._CornerSegments;
		NegativeLaserStrength = InArgs._NegativeLaserStrength;
		NegativeLaserDarkness = InArgs._NegativeLaserDarkness;
		LaserTilt = InArgs._LaserTilt;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.Tiling = ESlateBrushTileType::NoTile;
		Brush.Mirroring = ESlateBrushMirrorType::NoMirror;
		Brush.ImageSize = FVector2D(160.0f, 220.0f);
		UpdateBrushResource();
	}

	void SetTexture(UTexture2D* InTexture)
	{
		Texture = InTexture;
		UpdateBrushResource();
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	void SetTintColor(FLinearColor InTintColor)
	{
		TintColor = InTintColor;
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	void SetCornerRadius(float InCornerRadiusPx)
	{
		CornerRadiusPx = InCornerRadiusPx;
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	void SetCornerSegments(int32 InCornerSegments)
	{
		CornerSegments = InCornerSegments;
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	void SetNegativeLaserEffect(float InStrength, float InDarkness)
	{
		NegativeLaserStrength = FMath::Clamp(InStrength, 0.0f, 1.0f);
		NegativeLaserDarkness = FMath::Clamp(InDarkness, 0.0f, 1.0f);
		Invalidate(EInvalidateWidgetReason::PaintAndVolatility);
	}

	void SetLaserTilt(FVector2D InTilt)
	{
		LaserTilt = FVector2D(
			FMath::Clamp(InTilt.X, -1.0f, 1.0f),
			FMath::Clamp(InTilt.Y, -1.0f, 1.0f));
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	virtual bool ComputeVolatility() const override
	{
		return NegativeLaserStrength > 0.01f;
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
		if (LocalSize.X <= 1.0f || LocalSize.Y <= 1.0f)
		{
			return LayerId;
		}

		const FSlateBrush* BrushToRender = Texture ? &Brush : FCoreStyle::Get().GetBrush("WhiteBrush");
		const FSlateResourceHandle Handle = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*BrushToRender);
		if (!Handle.IsValid())
		{
			return LayerId;
		}

		TArray<FVector2D> Points;
		BuildRoundedRectPoints(LocalSize, Points);
		if (Points.Num() < 3)
		{
			return LayerId;
		}

		TArray<FSlateVertex> Vertices;
		TArray<SlateIndex> Indices;
		Vertices.Reserve(Points.Num() + 1);
		Indices.Reserve(Points.Num() * 3);

		// Keep the printed card art readable; the foil is a transparent film above it.
		const FLinearColor BaseTint = TintColor;
		const FColor FinalColor = BaseTint.ToFColor(true);
		const FSlateRenderTransform& RenderTransform = AllottedGeometry.GetAccumulatedRenderTransform();
		const FVector2f CenterPos(static_cast<float>(LocalSize.X * 0.5), static_cast<float>(LocalSize.Y * 0.5));
		Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
			RenderTransform,
			CenterPos,
			FVector2f(0.5f, 0.5f),
			FinalColor));

		for (const FVector2D& Point : Points)
		{
			const FVector2f LocalPos(static_cast<float>(Point.X), static_cast<float>(Point.Y));
			const FVector2f UV(
				static_cast<float>(Point.X / LocalSize.X),
				static_cast<float>(Point.Y / LocalSize.Y));
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
				RenderTransform,
				LocalPos,
				UV,
				FinalColor));
		}

		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			Indices.Add(0);
			Indices.Add(static_cast<SlateIndex>(Index + 1));
			Indices.Add(static_cast<SlateIndex>(((Index + 1) % Points.Num()) + 1));
		}

		FSlateDrawElement::MakeCustomVerts(
			OutDrawElements,
			LayerId,
			Handle,
			Vertices,
			Indices,
			nullptr,
			0,
			0,
			ShouldBeEnabled(bParentEnabled) ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect);

		int32 NextLayer = LayerId + 1;
		if (NegativeLaserStrength > 0.01f)
		{
			DrawNegativeLaserOverlay(
				AllottedGeometry,
				OutDrawElements,
				NextLayer,
				ShouldBeEnabled(bParentEnabled) ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect);
			NextLayer += 3;
		}

		return NextLayer;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return Brush.ImageSize;
	}

private:
	UTexture2D* Texture = nullptr;
	FSlateBrush Brush;
	FLinearColor TintColor = FLinearColor::White;
	float CornerRadiusPx = 5.0f;
	int32 CornerSegments = 8;
	float NegativeLaserStrength = 0.0f;
	float NegativeLaserDarkness = 0.72f;
	FVector2D LaserTilt = FVector2D::ZeroVector;

	void UpdateBrushResource()
	{
		Brush.SetResourceObject(Texture);
		if (Texture)
		{
			Brush.ImageSize = FVector2D(Texture->GetSurfaceWidth(), Texture->GetSurfaceHeight());
		}
	}

	void AddArc(TArray<FVector2D>& OutPoints, const FVector2D& Center, float Radius, float StartDegrees, float EndDegrees) const
	{
		const int32 SegmentCount = FMath::Clamp(CornerSegments, 2, 16);
		for (int32 Segment = 0; Segment <= SegmentCount; ++Segment)
		{
			const float T = static_cast<float>(Segment) / static_cast<float>(SegmentCount);
			const float AngleRadians = FMath::DegreesToRadians(FMath::Lerp(StartDegrees, EndDegrees, T));
			OutPoints.Add(Center + FVector2D(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians)) * Radius);
		}
	}

	bool IsInsideRoundedRect(const FVector2D& Point, const FVector2D& Size) const
	{
		const float Radius = FMath::Clamp(CornerRadiusPx, 0.0f, FMath::Min(Size.X, Size.Y) * 0.5f);
		if (Radius <= 0.0f)
		{
			return Point.X >= 0.0f && Point.Y >= 0.0f && Point.X <= Size.X && Point.Y <= Size.Y;
		}

		const FVector2D InnerMin(Radius, Radius);
		const FVector2D InnerMax(Size.X - Radius, Size.Y - Radius);
		if ((Point.X >= InnerMin.X && Point.X <= InnerMax.X)
			|| (Point.Y >= InnerMin.Y && Point.Y <= InnerMax.Y))
		{
			return true;
		}

		const FVector2D CornerCenter(
			Point.X < Radius ? Radius : Size.X - Radius,
			Point.Y < Radius ? Radius : Size.Y - Radius);
		return FVector2D::Distance(Point, CornerCenter) <= Radius;
	}

	FLinearColor LaserColorAt(FVector2D UV, float TimeSeconds) const
	{
		const float TiltPush = LaserTilt.X * 0.34f - LaserTilt.Y * 0.22f;
		const float SlowSweep = FMath::Sin(TimeSeconds * 0.55f) * 0.055f;
		const float MirrorAxis = UV.X * 0.82f + UV.Y * 0.34f;
		const float MirrorCenter = 0.52f + TiltPush + SlowSweep;
		const float MirrorDistance = FMath::Abs(MirrorAxis - MirrorCenter);
		const float MirrorBand = FMath::Pow(FMath::Clamp(1.0f - MirrorDistance / 0.18f, 0.0f, 1.0f), 4.2f);

		const float RainbowPhase = UV.X * 4.2f + UV.Y * 2.7f + TiltPush * 2.4f + TimeSeconds * 0.08f;
		const float RainbowT = 0.5f + 0.5f * FMath::Sin(RainbowPhase * PI);
		const float FineGrain = FMath::Pow(0.5f + 0.5f * FMath::Sin((UV.X * 48.0f - UV.Y * 19.0f) + TimeSeconds * 1.35f), 10.0f);
		const float DirectionalScratches = FMath::Pow(0.5f + 0.5f * FMath::Sin((UV.X + UV.Y * 1.8f) * PI * 18.0f), 14.0f);
		const float FilmFalloff = FMath::SmoothStep(0.0f, 0.12f, static_cast<float>(UV.Y))
			* (1.0f - FMath::SmoothStep(0.88f, 1.0f, static_cast<float>(UV.Y)));

		const FLinearColor IceSilver(0.92f, 0.97f, 1.0f, 1.0f);
		const FLinearColor Cyan(0.20f, 0.95f, 1.0f, 1.0f);
		const FLinearColor Violet(0.72f, 0.38f, 1.0f, 1.0f);
		const FLinearColor Pink(1.0f, 0.34f, 0.62f, 1.0f);
		const FLinearColor Lime(0.66f, 1.0f, 0.58f, 1.0f);

		FLinearColor Prism = RainbowT < 0.33f
			? FMath::Lerp(Cyan, Violet, RainbowT / 0.33f)
			: (RainbowT < 0.66f
				? FMath::Lerp(Violet, Pink, (RainbowT - 0.33f) / 0.33f)
				: FMath::Lerp(Pink, Lime, (RainbowT - 0.66f) / 0.34f));

		const float PrismStrength = 0.24f + MirrorBand * 0.54f + FineGrain * 0.08f;
		FLinearColor Color = FMath::Lerp(IceSilver, Prism, FMath::Clamp(PrismStrength, 0.0f, 0.82f));
		Color.R = FMath::Min(Color.R + MirrorBand * 0.46f, 1.0f);
		Color.G = FMath::Min(Color.G + MirrorBand * 0.46f, 1.0f);
		Color.B = FMath::Min(Color.B + MirrorBand * 0.50f, 1.0f);
		Color.A = (0.035f + MirrorBand * 0.34f + FineGrain * 0.055f + DirectionalScratches * 0.045f)
			* FilmFalloff
			* NegativeLaserStrength;
		return Color;
	}

	void DrawNegativeLaserOverlay(
		const FGeometry& AllottedGeometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		ESlateDrawEffect DrawEffects) const
	{
		const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
		const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");
		const FSlateResourceHandle WhiteHandle = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*WhiteBrush);
		if (!WhiteHandle.IsValid())
		{
			return;
		}

		TArray<FVector2D> RoundedPoints;
		BuildRoundedRectPoints(LocalSize, RoundedPoints);
		if (RoundedPoints.Num() < 3)
		{
			return;
		}

		DrawRoundedSolid(
			AllottedGeometry,
			OutDrawElements,
			LayerId,
			WhiteHandle,
			FLinearColor(0.78f, 0.92f, 1.0f, 0.045f * NegativeLaserStrength),
			RoundedPoints,
			DrawEffects);

		const double CurrentTime = FSlateApplication::Get().GetCurrentTime();
		const float TimeSeconds = static_cast<float>(CurrentTime);
		const int32 Columns = 30;
		const int32 Rows = 42;
		TArray<FSlateVertex> Vertices;
		TArray<SlateIndex> Indices;
		Vertices.Reserve(Columns * Rows * 4);
		Indices.Reserve(Columns * Rows * 6);

		const FSlateRenderTransform& RenderTransform = AllottedGeometry.GetAccumulatedRenderTransform();
		for (int32 Y = 0; Y < Rows; ++Y)
		{
			for (int32 X = 0; X < Columns; ++X)
			{
				const FVector2D CellMin(
					LocalSize.X * static_cast<float>(X) / static_cast<float>(Columns),
					LocalSize.Y * static_cast<float>(Y) / static_cast<float>(Rows));
				const FVector2D CellMax(
					LocalSize.X * static_cast<float>(X + 1) / static_cast<float>(Columns),
					LocalSize.Y * static_cast<float>(Y + 1) / static_cast<float>(Rows));
				const FVector2D CellCenter = (CellMin + CellMax) * 0.5f;
				if (!IsInsideRoundedRect(CellCenter, LocalSize))
				{
					continue;
				}

				const FVector2D Points[4] = {
					CellMin,
					FVector2D(CellMax.X, CellMin.Y),
					CellMax,
					FVector2D(CellMin.X, CellMax.Y)
				};

				const SlateIndex BaseIndex = static_cast<SlateIndex>(Vertices.Num());
				for (const FVector2D& Point : Points)
				{
					const FVector2D UV(Point.X / LocalSize.X, Point.Y / LocalSize.Y);
					Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
						RenderTransform,
						FVector2f(static_cast<float>(Point.X), static_cast<float>(Point.Y)),
						FVector2f(static_cast<float>(UV.X), static_cast<float>(UV.Y)),
						LaserColorAt(UV, TimeSeconds).ToFColor(true)));
				}

				Indices.Add(BaseIndex);
				Indices.Add(BaseIndex + 1);
				Indices.Add(BaseIndex + 2);
				Indices.Add(BaseIndex);
				Indices.Add(BaseIndex + 2);
				Indices.Add(BaseIndex + 3);
			}
		}

		if (Vertices.Num() > 0)
		{
			FSlateDrawElement::MakeCustomVerts(
				OutDrawElements,
				LayerId + 1,
				WhiteHandle,
				Vertices,
				Indices,
				nullptr,
				0,
				0,
				DrawEffects);
		}

		DrawRoundedSolid(
			AllottedGeometry,
			OutDrawElements,
			LayerId + 2,
			WhiteHandle,
			FLinearColor(1.0f, 1.0f, 1.0f, 0.035f * NegativeLaserStrength),
			RoundedPoints,
			DrawEffects);
	}

	void DrawRoundedSolid(
		const FGeometry& AllottedGeometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FSlateResourceHandle& Handle,
		const FLinearColor& Color,
		const TArray<FVector2D>& Points,
		ESlateDrawEffect DrawEffects) const
	{
		TArray<FSlateVertex> Vertices;
		TArray<SlateIndex> Indices;
		Vertices.Reserve(Points.Num() + 1);
		Indices.Reserve(Points.Num() * 3);

		const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
		const FSlateRenderTransform& RenderTransform = AllottedGeometry.GetAccumulatedRenderTransform();
		const FColor FinalColor = Color.ToFColor(true);
		Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
			RenderTransform,
			FVector2f(static_cast<float>(LocalSize.X * 0.5f), static_cast<float>(LocalSize.Y * 0.5f)),
			FVector2f(0.5f, 0.5f),
			FinalColor));

		for (const FVector2D& Point : Points)
		{
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
				RenderTransform,
				FVector2f(static_cast<float>(Point.X), static_cast<float>(Point.Y)),
				FVector2f(static_cast<float>(Point.X / LocalSize.X), static_cast<float>(Point.Y / LocalSize.Y)),
				FinalColor));
		}

		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			Indices.Add(0);
			Indices.Add(static_cast<SlateIndex>(Index + 1));
			Indices.Add(static_cast<SlateIndex>(((Index + 1) % Points.Num()) + 1));
		}

		FSlateDrawElement::MakeCustomVerts(
			OutDrawElements,
			LayerId,
			Handle,
			Vertices,
			Indices,
			nullptr,
			0,
			0,
			DrawEffects);
	}

	void BuildRoundedRectPoints(const FVector2D& Size, TArray<FVector2D>& OutPoints) const
	{
		const float Radius = FMath::Clamp(CornerRadiusPx, 0.0f, FMath::Min(Size.X, Size.Y) * 0.5f);
		if (Radius <= 0.0f)
		{
			OutPoints = {
				FVector2D(0.0f, 0.0f),
				FVector2D(Size.X, 0.0f),
				FVector2D(Size.X, Size.Y),
				FVector2D(0.0f, Size.Y)
			};
			return;
		}

		AddArc(OutPoints, FVector2D(Size.X - Radius, Radius), Radius, -90.0f, 0.0f);
		AddArc(OutPoints, FVector2D(Size.X - Radius, Size.Y - Radius), Radius, 0.0f, 90.0f);
		AddArc(OutPoints, FVector2D(Radius, Size.Y - Radius), Radius, 90.0f, 180.0f);
		AddArc(OutPoints, FVector2D(Radius, Radius), Radius, 180.0f, 270.0f);
	}
};

void UCh1RoundedImageWidget::SetTexture(UTexture2D* InTexture)
{
	Texture = InTexture;
	if (SlateRoundedImage)
	{
		SlateRoundedImage->SetTexture(Texture);
	}
}

void UCh1RoundedImageWidget::SetTintColor(FLinearColor InTintColor)
{
	TintColor = InTintColor;
	if (SlateRoundedImage)
	{
		SlateRoundedImage->SetTintColor(TintColor);
	}
}

void UCh1RoundedImageWidget::SetCornerRadius(float InCornerRadiusPx)
{
	CornerRadiusPx = FMath::Max(0.0f, InCornerRadiusPx);
	if (SlateRoundedImage)
	{
		SlateRoundedImage->SetCornerRadius(CornerRadiusPx);
	}
}

void UCh1RoundedImageWidget::SetNegativeLaserEffect(float InStrength, float InDarkness)
{
	NegativeLaserStrength = FMath::Clamp(InStrength, 0.0f, 1.0f);
	NegativeLaserDarkness = FMath::Clamp(InDarkness, 0.0f, 1.0f);
	if (SlateRoundedImage)
	{
		SlateRoundedImage->SetNegativeLaserEffect(NegativeLaserStrength, NegativeLaserDarkness);
	}
}

void UCh1RoundedImageWidget::SetLaserTilt(FVector2D InTilt)
{
	LaserTilt = FVector2D(
		FMath::Clamp(InTilt.X, -1.0f, 1.0f),
		FMath::Clamp(InTilt.Y, -1.0f, 1.0f));
	if (SlateRoundedImage)
	{
		SlateRoundedImage->SetLaserTilt(LaserTilt);
	}
}

TSharedRef<SWidget> UCh1RoundedImageWidget::RebuildWidget()
{
	return SAssignNew(SlateRoundedImage, SCh1RoundedImage)
		.Texture(Texture)
		.TintColor(TintColor)
		.CornerRadiusPx(CornerRadiusPx)
		.CornerSegments(CornerSegments)
		.NegativeLaserStrength(NegativeLaserStrength)
		.NegativeLaserDarkness(NegativeLaserDarkness)
		.LaserTilt(LaserTilt);
}

void UCh1RoundedImageWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (SlateRoundedImage)
	{
		SlateRoundedImage->SetTexture(Texture);
		SlateRoundedImage->SetTintColor(TintColor);
		SlateRoundedImage->SetCornerRadius(CornerRadiusPx);
		SlateRoundedImage->SetCornerSegments(CornerSegments);
		SlateRoundedImage->SetNegativeLaserEffect(NegativeLaserStrength, NegativeLaserDarkness);
		SlateRoundedImage->SetLaserTilt(LaserTilt);
	}
}

void UCh1RoundedImageWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	SlateRoundedImage.Reset();
}
