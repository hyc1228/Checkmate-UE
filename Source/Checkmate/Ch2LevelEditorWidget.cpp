// Copyright Yvaine / Checkmate. All Rights Reserved.

#if WITH_EDITOR

#include "Ch2LevelEditorWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "EditorAssetLibrary.h"

void UCh2LevelEditorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BrushBtn_Empty)        BrushBtn_Empty->OnClicked.AddDynamic(this, &UCh2LevelEditorWidget::OnBrushEmptyClicked);
	if (BrushBtn_Wall)         BrushBtn_Wall->OnClicked.AddDynamic(this, &UCh2LevelEditorWidget::OnBrushWallClicked);
	if (BrushBtn_Destructible) BrushBtn_Destructible->OnClicked.AddDynamic(this, &UCh2LevelEditorWidget::OnBrushDestructibleClicked);
	if (BrushBtn_Pickup)       BrushBtn_Pickup->OnClicked.AddDynamic(this, &UCh2LevelEditorWidget::OnBrushPickupClicked);
	if (BrushBtn_Exit)         BrushBtn_Exit->OnClicked.AddDynamic(this, &UCh2LevelEditorWidget::OnBrushExitClicked);
	if (BrushBtn_Start)        BrushBtn_Start->OnClicked.AddDynamic(this, &UCh2LevelEditorWidget::OnBrushStartClicked);
	if (BrushBtn_WeddingWreckage) BrushBtn_WeddingWreckage->OnClicked.AddDynamic(this, &UCh2LevelEditorWidget::OnBrushWeddingWreckageClicked);
	if (SaveButton)            SaveButton->OnClicked.AddDynamic(this, &UCh2LevelEditorWidget::OnSaveClicked);
	if (PresetBtn_PVGoldPath)  PresetBtn_PVGoldPath->OnClicked.AddDynamic(this, &UCh2LevelEditorWidget::OnPresetPVGoldPathClicked);
	if (PresetBtn_Clear)       PresetBtn_Clear->OnClicked.AddDynamic(this, &UCh2LevelEditorWidget::OnPresetClearClicked);
	if (PresetBtn_PVDefaults)  PresetBtn_PVDefaults->OnClicked.AddDynamic(this, &UCh2LevelEditorWidget::OnPresetPVDefaultsClicked);
	if (PresetBtn_Validate)    PresetBtn_Validate->OnClicked.AddDynamic(this, &UCh2LevelEditorWidget::OnPresetValidateClicked);

	SetBrush(ECh2CellType::Wall);
	RebuildGrid();
}

void UCh2LevelEditorWidget::RebuildGrid()
{
	if (!GridPanel) return;
	GridPanel->ClearChildren();
	CellButtons.Empty();

	if (!TargetAsset)
	{
		if (StatusText) StatusText->SetText(FText::FromString(TEXT("⚠ 拖一个 UCh2LevelData 到 TargetAsset")));
		return;
	}
	SpawnGridButtons();
	RefreshAllCellVisuals();
	if (StatusText) StatusText->SetText(FText::FromString(FString::Printf(
		TEXT("已加载 %s（%d×%d，cell 数 %d）"),
		*TargetAsset->GetName(), TargetAsset->GridWidth, TargetAsset->GridHeight,
		TargetAsset->Cells.Num())));
}

void UCh2CellButton::OnInternalClicked()
{
	if (Owner)
	{
		Owner->HandleCellButtonClicked(Cell);
	}
}

void UCh2LevelEditorWidget::HandleCellButtonClicked(FIntPoint Cell)
{
	OnCellClicked_Internal(Cell);
}

void UCh2LevelEditorWidget::SpawnGridButtons()
{
	if (!TargetAsset || !GridPanel) return;

	const int32 W = TargetAsset->GridWidth;
	const int32 H = TargetAsset->GridHeight;

	for (int32 Y = 0; Y < H; ++Y)
	for (int32 X = 0; X < W; ++X)
	{
		// UMG UniformGrid Row=Y 反向（Y=0 在底，更直观）+ Column=X
		const int32 DisplayRow = (H - 1 - Y);
		const int32 DisplayCol = X;

		UCh2CellButton* Btn = NewObject<UCh2CellButton>(this);
		Btn->Cell = FIntPoint(X, Y);
		Btn->Owner = this;

		if (UUniformGridSlot* GSlot = Cast<UUniformGridSlot>(GridPanel->AddChild(Btn)))
		{
			GSlot->SetRow(DisplayRow);
			GSlot->SetColumn(DisplayCol);
		}

		UTextBlock* Label = NewObject<UTextBlock>(this);
		Btn->AddChild(Label);
		Label->SetJustification(ETextJustify::Center);

		Btn->OnClicked.AddDynamic(Btn, &UCh2CellButton::OnInternalClicked);
		CellButtons.Add(Btn->Cell, Btn);
	}
}

void UCh2LevelEditorWidget::RefreshAllCellVisuals()
{
	for (const TPair<FIntPoint, UCh2CellButton*>& P : CellButtons)
	{
		RefreshCellVisual(P.Key);
	}
}

void UCh2LevelEditorWidget::RefreshCellVisual(FIntPoint Cell)
{
	if (!TargetAsset) return;
	UCh2CellButton** BtnPtr = CellButtons.Find(Cell);
	if (!BtnPtr || !*BtnPtr) return;
	UCh2CellButton* Btn = *BtnPtr;

	const ECh2CellType Type = TargetAsset->GetCellType(Cell);
	const FLinearColor Color = ColorForType(Type);
	const FString Label = LabelForType(Type);

	FButtonStyle Style = Btn->GetStyle();
	Style.Normal.TintColor = FSlateColor(Color);
	Style.Hovered.TintColor = FSlateColor(Color * 1.2f);
	Style.Pressed.TintColor = FSlateColor(Color * 0.8f);
	Btn->SetStyle(Style);

	// 更新内嵌 TextBlock 的内容
	for (int32 i = 0; i < Btn->GetChildrenCount(); ++i)
	{
		if (UTextBlock* TB = Cast<UTextBlock>(Btn->GetChildAt(i)))
		{
			TB->SetText(FText::FromString(Label));
			TB->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
			break;
		}
	}
}

void UCh2LevelEditorWidget::OnCellClicked_Internal(FIntPoint Cell)
{
	if (!TargetAsset) return;

	if (CurrentBrush == ECh2CellType::Empty)
	{
		TargetAsset->Cells.Remove(Cell);
	}
	else
	{
		TargetAsset->Cells.Add(Cell, CurrentBrush);
	}
	TargetAsset->Modify();
	RefreshCellVisual(Cell);

	if (StatusText)
	{
		StatusText->SetText(FText::FromString(FString::Printf(
			TEXT("Cell (%d,%d) = %s （未保存）"), Cell.X, Cell.Y, *LabelForType(CurrentBrush))));
	}
}

void UCh2LevelEditorWidget::SetBrush(ECh2CellType NewBrush)
{
	CurrentBrush = NewBrush;
	if (BrushLabel)
	{
		BrushLabel->SetText(FText::FromString(FString::Printf(TEXT("当前画笔：%s"), *LabelForType(NewBrush))));
	}
}

void UCh2LevelEditorWidget::OnBrushEmptyClicked()        { SetBrush(ECh2CellType::Empty); }
void UCh2LevelEditorWidget::OnBrushWallClicked()         { SetBrush(ECh2CellType::Wall); }
void UCh2LevelEditorWidget::OnBrushDestructibleClicked() { SetBrush(ECh2CellType::Destructible); }
void UCh2LevelEditorWidget::OnBrushPickupClicked()       { SetBrush(ECh2CellType::ClownPickup); }
void UCh2LevelEditorWidget::OnBrushExitClicked()         { SetBrush(ECh2CellType::Exit); }
void UCh2LevelEditorWidget::OnBrushStartClicked()        { SetBrush(ECh2CellType::Start); }
void UCh2LevelEditorWidget::OnBrushWeddingWreckageClicked() { SetBrush(ECh2CellType::WeddingWreckage); }

void UCh2LevelEditorWidget::OnSaveClicked()
{
	SaveToAsset();
}

void UCh2LevelEditorWidget::SaveToAsset()
{
	if (!TargetAsset)
	{
		if (StatusText) StatusText->SetText(FText::FromString(TEXT("⚠ TargetAsset 为空，无法保存")));
		return;
	}
	TargetAsset->MarkPackageDirty();
	const FString PkgPath = TargetAsset->GetPackage()->GetName();
	const bool bOk = UEditorAssetLibrary::SaveAsset(PkgPath, /*bOnlyIfIsDirty=*/false);

	if (StatusText)
	{
		StatusText->SetText(FText::FromString(FString::Printf(
			TEXT("%s 保存 %s（%d cells）"),
			bOk ? TEXT("✓") : TEXT("✗"),
			*TargetAsset->GetName(), TargetAsset->Cells.Num())));
	}
}

void UCh2LevelEditorWidget::SetStatus(const FString& Message)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Message));
	}
}

void UCh2LevelEditorWidget::ApplyPVPreset(ECh2EditorPVPreset Preset)
{
	switch (Preset)
	{
	case ECh2EditorPVPreset::PVGoldPathRoom1:
		ApplyPVGoldPathRoom1Preset();
		break;
	case ECh2EditorPVPreset::EmptySandbox:
		ApplyEmptySandboxPreset();
		break;
	default:
		break;
	}
}

void UCh2LevelEditorWidget::ApplyStandardPVGridSettings()
{
	if (!TargetAsset) return;
	TargetAsset->GridWidth = 8;
	TargetAsset->GridHeight = 8;
	TargetAsset->CellSize = 200.0f;
}

void UCh2LevelEditorWidget::ApplyPVRecordingDefaults()
{
	if (!TargetAsset)
	{
		SetStatus(TEXT("PV defaults failed: TargetAsset is empty."));
		return;
	}

	TargetAsset->Modify();
	ApplyStandardPVGridSettings();
	TargetAsset->MoveBudget = 99;
	TargetAsset->OptimalMoveCount = 3;
	TargetAsset->MarkPackageDirty();

	RebuildGrid();
	SetStatus(TEXT("PV defaults applied: 8x8, CellSize=200, MoveBudget=99, OptimalMoveCount=3. Save when ready."));
}

void UCh2LevelEditorWidget::ApplyPVGoldPathRoom1Preset()
{
	if (!TargetAsset)
	{
		SetStatus(TEXT("PV gold path preset failed: TargetAsset is empty."));
		return;
	}

	TargetAsset->Modify();
	ApplyStandardPVGridSettings();
	TargetAsset->MoveBudget = 99;
	TargetAsset->OptimalMoveCount = 3;
	TargetAsset->Cells.Empty();

	TargetAsset->Cells.Add(FIntPoint(5, 7), ECh2CellType::Start);
	TargetAsset->Cells.Add(FIntPoint(3, 4), ECh2CellType::WeddingWreckage);
	TargetAsset->Cells.Add(FIntPoint(4, 4), ECh2CellType::WeddingWreckage);
	TargetAsset->Cells.Add(FIntPoint(5, 4), ECh2CellType::ClownPickup);
	TargetAsset->Cells.Add(FIntPoint(5, 3), ECh2CellType::Destructible);
	TargetAsset->Cells.Add(FIntPoint(4, 1), ECh2CellType::Exit);
	TargetAsset->MarkPackageDirty();

	RebuildGrid();
	SetStatus(TEXT("PV gold path ready: Start(5,7) -> Pickup(5,4) -> Clown target(3,3) -> Exit(4,1). Destructible at (5,3). Save when ready."));
}

void UCh2LevelEditorWidget::ApplyEmptySandboxPreset()
{
	if (!TargetAsset)
	{
		SetStatus(TEXT("Clear preset failed: TargetAsset is empty."));
		return;
	}

	TargetAsset->Modify();
	ApplyStandardPVGridSettings();
	TargetAsset->MoveBudget = 0;
	TargetAsset->OptimalMoveCount = 0;
	TargetAsset->Cells.Empty();
	TargetAsset->Cells.Add(FIntPoint(1, 1), ECh2CellType::Start);
	TargetAsset->Cells.Add(FIntPoint(6, 6), ECh2CellType::Exit);
	TargetAsset->MarkPackageDirty();

	RebuildGrid();
	SetStatus(TEXT("Empty sandbox ready: Start(1,1), Exit(6,6), no move budget. Save when ready."));
}

int32 UCh2LevelEditorWidget::CountCellsOfType(ECh2CellType Type) const
{
	if (!TargetAsset) return 0;
	int32 Count = 0;
	for (const TPair<FIntPoint, ECh2CellType>& P : TargetAsset->Cells)
	{
		if (P.Value == Type)
		{
			++Count;
		}
	}
	return Count;
}

FString UCh2LevelEditorWidget::ValidateTargetAsset()
{
	if (!TargetAsset)
	{
		const FString Message = TEXT("Validation failed: TargetAsset is empty.");
		SetStatus(Message);
		return Message;
	}

	TArray<FString> Lines;
	bool bHasError = false;
	bool bGoldPathReady = true;

	if (TargetAsset->GridWidth != 8 || TargetAsset->GridHeight != 8)
	{
		bHasError = true;
		bGoldPathReady = false;
		Lines.Add(FString::Printf(TEXT("ERROR: PV grid should be 8x8, current=%dx%d."), TargetAsset->GridWidth, TargetAsset->GridHeight));
	}
	if (!FMath::IsNearlyEqual(TargetAsset->CellSize, 200.0f))
	{
		bGoldPathReady = false;
		Lines.Add(FString::Printf(TEXT("WARN: PV CellSize recommends 200, current=%.1f."), TargetAsset->CellSize));
	}
	if (TargetAsset->MoveBudget != 99)
	{
		bGoldPathReady = false;
		Lines.Add(FString::Printf(TEXT("WARN: PV recording MoveBudget recommends 99, current=%d."), TargetAsset->MoveBudget));
	}

	const int32 StartCount = CountCellsOfType(ECh2CellType::Start);
	const int32 ExitCount = CountCellsOfType(ECh2CellType::Exit);
	const int32 PickupCount = CountCellsOfType(ECh2CellType::ClownPickup);
	const int32 DestructibleCount = CountCellsOfType(ECh2CellType::Destructible);
	const int32 WreckageCount = CountCellsOfType(ECh2CellType::WeddingWreckage);

	if (StartCount != 1) { bHasError = true; Lines.Add(FString::Printf(TEXT("ERROR: expected 1 Start, found %d."), StartCount)); }
	if (ExitCount != 1) { bHasError = true; Lines.Add(FString::Printf(TEXT("ERROR: expected 1 Exit, found %d."), ExitCount)); }
	if (PickupCount < 1) { bHasError = true; Lines.Add(TEXT("ERROR: expected at least 1 ClownPickup.")); }
	if (DestructibleCount < 1) { bHasError = true; Lines.Add(TEXT("ERROR: expected at least 1 Destructible.")); }
	if (WreckageCount < 2) { bGoldPathReady = false; Lines.Add(FString::Printf(TEXT("WARN: PV bell beat expects 2 WeddingWreckage cells, found %d."), WreckageCount)); }

	for (const TPair<FIntPoint, ECh2CellType>& P : TargetAsset->Cells)
	{
		const FIntPoint Cell = P.Key;
		const bool bInBounds = Cell.X >= 0 && Cell.X < TargetAsset->GridWidth
			&& Cell.Y >= 0 && Cell.Y < TargetAsset->GridHeight;
		if (!bInBounds)
		{
			bHasError = true;
			bGoldPathReady = false;
			Lines.Add(FString::Printf(TEXT("ERROR: cell (%d,%d) is out of bounds."), Cell.X, Cell.Y));
		}
	}

	const TPair<FIntPoint, ECh2CellType> GoldCells[] =
	{
		TPair<FIntPoint, ECh2CellType>(FIntPoint(5, 7), ECh2CellType::Start),
		TPair<FIntPoint, ECh2CellType>(FIntPoint(3, 4), ECh2CellType::WeddingWreckage),
		TPair<FIntPoint, ECh2CellType>(FIntPoint(4, 4), ECh2CellType::WeddingWreckage),
		TPair<FIntPoint, ECh2CellType>(FIntPoint(5, 4), ECh2CellType::ClownPickup),
		TPair<FIntPoint, ECh2CellType>(FIntPoint(5, 3), ECh2CellType::Destructible),
		TPair<FIntPoint, ECh2CellType>(FIntPoint(4, 1), ECh2CellType::Exit),
	};
	for (const TPair<FIntPoint, ECh2CellType>& Expected : GoldCells)
	{
		if (TargetAsset->GetCellType(Expected.Key) != Expected.Value)
		{
			bGoldPathReady = false;
			Lines.Add(FString::Printf(TEXT("WARN: gold path cell (%d,%d) should be %s."),
				Expected.Key.X, Expected.Key.Y, *LabelForType(Expected.Value)));
		}
	}

	if (Lines.Num() == 0)
	{
		Lines.Add(TEXT("OK: Ch2 level validates cleanly."));
	}
	Lines.Insert(FString::Printf(TEXT("PV_GOLD_PATH_READY=%s Errors=%s Cells=%d"),
		bGoldPathReady && !bHasError ? TEXT("true") : TEXT("false"),
		bHasError ? TEXT("true") : TEXT("false"),
		TargetAsset->Cells.Num()), 0);

	const FString Result = FString::Join(Lines, TEXT("\n"));
	SetStatus(Result);
	return Result;
}

void UCh2LevelEditorWidget::OnPresetPVGoldPathClicked()
{
	ApplyPVGoldPathRoom1Preset();
}

void UCh2LevelEditorWidget::OnPresetClearClicked()
{
	ApplyEmptySandboxPreset();
}

void UCh2LevelEditorWidget::OnPresetPVDefaultsClicked()
{
	ApplyPVRecordingDefaults();
}

void UCh2LevelEditorWidget::OnPresetValidateClicked()
{
	ValidateTargetAsset();
}

FLinearColor UCh2LevelEditorWidget::ColorForType(ECh2CellType Type)
{
	switch (Type)
	{
	case ECh2CellType::Empty:           return FLinearColor(0.20f, 0.20f, 0.20f);
	case ECh2CellType::Wall:            return FLinearColor(0.40f, 0.40f, 0.45f);
	case ECh2CellType::Destructible:    return FLinearColor(0.70f, 0.50f, 0.30f);
	case ECh2CellType::ClownPickup:     return FLinearColor(0.95f, 0.75f, 0.20f);
	case ECh2CellType::Exit:            return FLinearColor(0.20f, 0.90f, 0.50f);
	case ECh2CellType::Start:           return FLinearColor(0.50f, 0.85f, 0.95f);
	case ECh2CellType::WeddingWreckage: return FLinearColor(0.85f, 0.85f, 0.90f);
	}
	return FLinearColor::Black;
}

FString UCh2LevelEditorWidget::LabelForType(ECh2CellType Type)
{
	switch (Type)
	{
	case ECh2CellType::Empty:           return TEXT("·");
	case ECh2CellType::Wall:            return TEXT("█");
	case ECh2CellType::Destructible:    return TEXT("▓");
	case ECh2CellType::ClownPickup:     return TEXT("◆");
	case ECh2CellType::Exit:            return TEXT("→");
	case ECh2CellType::Start:           return TEXT("✦");
	case ECh2CellType::WeddingWreckage: return TEXT("♥");
	}
	return TEXT("?");
}

#endif  // WITH_EDITOR
