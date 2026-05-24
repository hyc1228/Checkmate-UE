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
	if (SaveButton)            SaveButton->OnClicked.AddDynamic(this, &UCh2LevelEditorWidget::OnSaveClicked);

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

FLinearColor UCh2LevelEditorWidget::ColorForType(ECh2CellType Type)
{
	switch (Type)
	{
	case ECh2CellType::Empty:        return FLinearColor(0.20f, 0.20f, 0.20f);
	case ECh2CellType::Wall:         return FLinearColor(0.40f, 0.40f, 0.45f);
	case ECh2CellType::Destructible: return FLinearColor(0.70f, 0.50f, 0.30f);
	case ECh2CellType::ClownPickup:  return FLinearColor(0.95f, 0.75f, 0.20f);
	case ECh2CellType::Exit:         return FLinearColor(0.20f, 0.90f, 0.50f);
	case ECh2CellType::Start:        return FLinearColor(0.50f, 0.85f, 0.95f);
	}
	return FLinearColor::Black;
}

FString UCh2LevelEditorWidget::LabelForType(ECh2CellType Type)
{
	switch (Type)
	{
	case ECh2CellType::Empty:        return TEXT("·");
	case ECh2CellType::Wall:         return TEXT("█");
	case ECh2CellType::Destructible: return TEXT("▓");
	case ECh2CellType::ClownPickup:  return TEXT("◆");
	case ECh2CellType::Exit:         return TEXT("→");
	case ECh2CellType::Start:        return TEXT("✦");
	}
	return TEXT("?");
}

#endif  // WITH_EDITOR
