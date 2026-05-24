// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Ch1Localization.h"
#include "Ch1LocSubsystem.generated.h"

/**
 * Ch1 本地化运行时子系统（v0.9）。
 *
 * 用法（C++）：
 *   UCh1LocSubsystem* Loc = GetGameInstance()->GetSubsystem<UCh1LocSubsystem>();
 *   ProgressText->SetText(FText::Format(Loc->Get("Inspection.Progress"), Args...));
 *
 * 用法（BP）：GetGameInstanceSubsystem → UCh1LocSubsystem → Get(FName) → FText
 *
 * 字典来源：BP_Chapter1GameMode 在 BeginPlay 时调 SetStrings(DA_Ch1Loc)。
 * 或直接在 Project Settings 改 DefaultStringsAssetPath（启动时自动加载）。
 */
UCLASS(BlueprintType, Config=Game, defaultconfig)
class CHECKMATE_API UCh1LocSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** 取本地化文本。默认按当前语言；未命中 key 时返回 key 名便于排错。 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Localization")
	FText Get(FName Key) const;

	UFUNCTION(BlueprintCallable, Category="Localization")
	void SetLanguage(ECh1Language NewLang);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Localization")
	ECh1Language GetLanguage() const { return CurrentLanguage; }

	UFUNCTION(BlueprintCallable, Category="Localization")
	void ToggleLanguage();

	/** GameMode 在 BeginPlay 时调，注入字典 asset。 */
	UFUNCTION(BlueprintCallable, Category="Localization")
	void SetStrings(UCh1LocStrings* InStrings);

	DECLARE_MULTICAST_DELEGATE(FOnLanguageChangedNative);
	FOnLanguageChangedNative OnLanguageChanged;

	/** Default 字典 asset 路径（启动时自动加载）。可在 DefaultGame.ini 覆盖。 */
	UPROPERTY(Config, EditAnywhere, Category="Localization")
	FSoftObjectPath DefaultStringsAssetPath;

private:
	UPROPERTY()
	ECh1Language CurrentLanguage = ECh1Language::EnUS;

	UPROPERTY()
	UCh1LocStrings* Strings = nullptr;
};
