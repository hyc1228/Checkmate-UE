// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Ch1Localization.generated.h"

UENUM(BlueprintType)
enum class ECh1Language : uint8
{
	ZhCN UMETA(DisplayName="中文"),
	EnUS UMETA(DisplayName="English"),
};

/**
 * 单条本地化条目。加新语言：给本 struct 加 UPROPERTY FText 字段 + 在 ECh1Language 加 enum
 * + 在 Get() 加 case + 编辑器里给 DA_Ch1Loc 每条新字段填值。
 *
 * 迁移到 UE Localization Dashboard 时：本 struct 整体作废；调用方 UCh1LocSubsystem::Get(Key)
 * 改成 FText::FromStringTable(ST_Ch1, Key.ToString()) + 启动时 SetCurrentCulture。
 */
USTRUCT(BlueprintType)
struct FLocalizedEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Localization", meta=(MultiLine="true"))
	FText ZhCN;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Localization", meta=(MultiLine="true"))
	FText EnUS;

	FText Get(ECh1Language Lang) const
	{
		switch (Lang)
		{
			case ECh1Language::EnUS: return EnUS;
			case ECh1Language::ZhCN:
			default:                 return ZhCN;
		}
	}
};

/**
 * Ch1 本地化字典 DataAsset。设计 hub：/Game/Ch1/Localization/DA_Ch1Loc.uasset
 * 一处编辑双语，FName key 建议 "Module.Sub" 风格（如 "Inspection.Progress"）。
 */
UCLASS(BlueprintType)
class CHECKMATE_API UCh1LocStrings : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Localization", meta=(ForceInlineRow))
	TMap<FName, FLocalizedEntry> Strings;

	/** 未命中 key 时的回退（一般直接返回 key 文字便于调试）。 */
	FText Get(FName Key, ECh1Language Lang) const
	{
		if (const FLocalizedEntry* Entry = Strings.Find(Key))
		{
			return Entry->Get(Lang);
		}
		return FText::FromName(Key);
	}
};
