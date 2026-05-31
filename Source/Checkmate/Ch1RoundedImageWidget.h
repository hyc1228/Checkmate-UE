// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Ch1RoundedImageWidget.generated.h"

class UTexture2D;

UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API UCh1RoundedImageWidget : public UWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Ch1|Rounded Image")
	void SetTexture(UTexture2D* InTexture);

	UFUNCTION(BlueprintCallable, Category="Ch1|Rounded Image")
	void SetTintColor(FLinearColor InTintColor);

	UFUNCTION(BlueprintCallable, Category="Ch1|Rounded Image")
	void SetCornerRadius(float InCornerRadiusPx);

	UFUNCTION(BlueprintCallable, Category="Ch1|Rounded Image|Holographic Foil")
	void SetNegativeLaserEffect(float InStrength, float InDarkness = 0.72f);

	UFUNCTION(BlueprintCallable, Category="Ch1|Rounded Image|Holographic Foil")
	void SetLaserTilt(FVector2D InTilt);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Rounded Image", meta=(ClampMin="0.0"))
	float CornerRadiusPx = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Rounded Image")
	FLinearColor TintColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Rounded Image", meta=(ClampMin="2", ClampMax="16"))
	int32 CornerSegments = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Rounded Image")
	TObjectPtr<UTexture2D> Texture = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Rounded Image|Holographic Foil", meta=(ClampMin="0.0", ClampMax="1.0"))
	float NegativeLaserStrength = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Rounded Image|Holographic Foil", meta=(ClampMin="0.0", ClampMax="1.0"))
	float NegativeLaserDarkness = 0.72f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ch1|Rounded Image|Holographic Foil")
	FVector2D LaserTilt = FVector2D::ZeroVector;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	TSharedPtr<class SCh1RoundedImage> SlateRoundedImage;
};
