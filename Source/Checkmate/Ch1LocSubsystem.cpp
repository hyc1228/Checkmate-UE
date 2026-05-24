// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "Ch1LocSubsystem.h"

void UCh1LocSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 启动时按 config 路径自动加载字典 asset（若已配置）
	if (DefaultStringsAssetPath.IsValid())
	{
		if (UCh1LocStrings* Loaded = Cast<UCh1LocStrings>(DefaultStringsAssetPath.TryLoad()))
		{
			Strings = Loaded;
		}
	}
}

void UCh1LocSubsystem::SetStrings(UCh1LocStrings* InStrings)
{
	Strings = InStrings;
}

FText UCh1LocSubsystem::Get(FName Key) const
{
	if (Strings)
	{
		return Strings->Get(Key, CurrentLanguage);
	}
	// 未注入字典——回退到 key 文字便于排错
	return FText::FromName(Key);
}

void UCh1LocSubsystem::SetLanguage(ECh1Language NewLang)
{
	if (CurrentLanguage == NewLang) return;
	CurrentLanguage = NewLang;
	OnLanguageChanged.Broadcast();
}

void UCh1LocSubsystem::ToggleLanguage()
{
	SetLanguage(CurrentLanguage == ECh1Language::ZhCN ? ECh1Language::EnUS : ECh1Language::ZhCN);
}
