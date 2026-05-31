// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "Ch1StarterStandardWidget.generated.h"

class SWidget;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStarterStandardConfirmed);

UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API UCh1StarterStandardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCh1StarterStandardWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable, Category="Ch1|Starter Standard")
	FOnStarterStandardConfirmed OnConfirmed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Starter Standard")
	TSoftObjectPtr<UTexture2D> PlaceholderImage;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	FSlateBrush PlaceholderBrush;

	FReply HandleClicked();
};
