// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "CardSelectionScreen.h"

#include "AudioService.h"
#include "CardData.h"
#include "Ch1LocSubsystem.h"
#include "JudgmentCardWidget.h"

#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UCardSelectionScreen::SetShiftConfig(const TArray<UCardData*>& InPoolCards, int32 InK, float InAssemblyTimerSec)
{
	PoolCards = InPoolCards;
	K = FMath::Max(1, InK);
	AssemblyTimerSec = FMath::Max(1.0f, InAssemblyTimerSec);
	TimeRemaining = AssemblyTimerSec;
}

void UCardSelectionScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (!CardPoolContainer)
	{
		UE_LOG(LogTemp, Error, TEXT("CardSelectionScreen: missing required CardPoolContainer BindWidget"));
		return;
	}

	if (CardWidgetClass && PoolCards.Num() > 0)
	{
		// 先 spawn 所有卡
		for (UCardData* CardData : PoolCards)
		{
			if (!CardData) continue;

			UJudgmentCardWidget* NewCard = CreateWidget<UJudgmentCardWidget>(this, CardWidgetClass);
			if (NewCard)
			{
				NewCard->SetCardData(CardData);
				NewCard->SetOwningScreen(this);
				CardPoolContainer->AddChild(NewCard);
				AllCardWidgets.Add(NewCard);
			}
		}

		// 底部对齐扇形：所有卡的底边在同一水平线，只用旋转角度展开（Balatro 手牌式）。
		// z-order 中央卡最高，边缘卡递减——视觉上中心卡盖在最上面。
		const int32 N = AllCardWidgets.Num();
		if (N > 1)
		{
			const int32 CenterIdx = N / 2;
			const float EffectiveCardWidth = N <= 5 ? FMath::Max(CardWidth, 176.0f) : CardWidth;
			const float EffectiveCardHeight = N <= 5 ? FMath::Max(CardHeight, 256.0f) : CardHeight;
			const float EffectiveCardSpacing = N <= 5 ? FMath::Max(CardSpacingPx, 112.0f) : CardSpacingPx;
			const float EffectiveFanSpread = N <= 5 ? FMath::Min(FanSpreadDegrees, 28.0f) : FanSpreadDegrees;

			for (int32 i = 0; i < N; ++i)
			{
				const float t = (static_cast<float>(i) / static_cast<float>(N - 1)) - 0.5f;
				const float AngleDeg = t * EffectiveFanSpread;

				UJudgmentCardWidget* Card = AllCardWidgets[i];
				Card->SetBaseFanAngle(AngleDeg);

				if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Card->Slot))
				{
					CanvasSlot->SetAnchors(FAnchors(0.5f, 1.0f));    // panel 底部中央
					CanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f)); // Position 指向 widget 底部中央
					CanvasSlot->SetSize(FVector2D(EffectiveCardWidth, EffectiveCardHeight));

					const float Offset = (i - (N - 1) * 0.5f) * EffectiveCardSpacing;
					CanvasSlot->SetPosition(FVector2D(Offset, 0.0f));

					// 中央 z-order 最高，向两边递减
					CanvasSlot->SetZOrder(N - FMath::Abs(i - CenterIdx));
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("CardSelectionScreen: CardWidgetClass not set or PoolCards empty"));
	}

	if (BeginShiftButton)
	{
		BeginShiftButton->OnClicked.AddDynamic(this, &UCardSelectionScreen::OnBeginShiftClicked);
		BeginShiftButton->SetIsEnabled(false);
		OnBeginShiftButtonEnableChanged(false);
	}

	if (LangToggleButton)
	{
		LangToggleButton->OnClicked.AddDynamic(this, &UCardSelectionScreen::OnLangToggleClicked);
	}

	if (TimerLabel)
	{
		TimerLabel->SetText(FText::FromString(TEXT("—")));
	}

	if (UWorld* World = GetWorld())
	{
		TimeRemaining = AssemblyTimerSec;
		World->GetTimerManager().SetTimer(
			CountdownTimerHandle,
			FTimerDelegate::CreateUObject(this, &UCardSelectionScreen::TickCountdown),
			0.1f,
			true);
	}

	// 订阅语言切换 → 重 push 文本
	if (UCh1LocSubsystem* Loc = GetLoc())
	{
		LangChangedHandle = Loc->OnLanguageChanged.AddUObject(this, &UCardSelectionScreen::RefreshLocalizedTexts);
	}
	RefreshLocalizedTexts();
	RefreshConfirmEnabled();
}

void UCardSelectionScreen::NativeDestruct()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(CommitAnimationTimerHandle);
	}

	if (BeginShiftButton)
	{
		BeginShiftButton->OnClicked.RemoveAll(this);
	}
	if (LangToggleButton)
	{
		LangToggleButton->OnClicked.RemoveAll(this);
	}

	if (UCh1LocSubsystem* Loc = GetLoc())
	{
		Loc->OnLanguageChanged.Remove(LangChangedHandle);
	}

	Super::NativeDestruct();
}

UCh1LocSubsystem* UCardSelectionScreen::GetLoc() const
{
	if (UWorld* W = GetWorld())
	{
		if (UGameInstance* GI = W->GetGameInstance())
		{
			return GI->GetSubsystem<UCh1LocSubsystem>();
		}
	}
	return nullptr;
}

void UCardSelectionScreen::RefreshLocalizedTexts()
{
	UCh1LocSubsystem* Loc = GetLoc();
	if (!Loc) return;

	if (HeaderText)
	{
		HeaderText->SetText(FText::Format(Loc->Get(TEXT("CardSelection.Header")), FText::AsNumber(K)));
	}
	if (CounterText)
	{
		CounterText->SetText(FText::Format(
			Loc->Get(TEXT("CardSelection.Counter")),
			FText::AsNumber(SelectedCards.Num()), FText::AsNumber(K)));
	}
	if (BeginShiftLabel)
	{
		BeginShiftLabel->SetText(Loc->Get(TEXT("CardSelection.Confirm")));
	}
	if (LangToggleLabel)
	{
		// 按钮显示切换后的语言名（便于发现"再点这个会变成 X"）
		const ECh1Language Next = (Loc->GetLanguage() == ECh1Language::ZhCN)
			? ECh1Language::EnUS : ECh1Language::ZhCN;
		LangToggleLabel->SetText(FText::FromString(Next == ECh1Language::EnUS ? TEXT("EN") : TEXT("中")));
	}
}

void UCardSelectionScreen::OnLangToggleClicked()
{
	if (UCh1LocSubsystem* Loc = GetLoc())
	{
		Loc->ToggleLanguage();
	}
}

void UCardSelectionScreen::TickCountdown()
{
	TimeRemaining -= 0.1f;
	if (TimeRemaining < 0.0f)
	{
		TimeRemaining = 0.0f;
	}

	if (TimerLabel)
	{
		const int32 Seconds = FMath::CeilToInt(TimeRemaining);
		TimerLabel->SetText(FText::AsNumber(Seconds));
	}

	if (TimeRemaining <= 0.0f)
	{
		GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);

		// 30s 到 → 用未选卡顺序补齐到 K
		for (UJudgmentCardWidget* Card : AllCardWidgets)
		{
			if (SelectedCards.Num() >= K) break;
			if (Card && !SelectedCards.Contains(Card))
			{
				SelectedCards.Add(Card);
				Card->SetSelected(true);
			}
		}

		RefreshConfirmEnabled();
		OnBeginShiftClicked();
	}
}

void UCardSelectionScreen::HandleCardClicked(UJudgmentCardWidget* ClickedCard)
{
	if (!ClickedCard || bCommitAnimationPlaying) return;

	const int32 ExistingIdx = SelectedCards.IndexOfByKey(ClickedCard);
	if (ExistingIdx != INDEX_NONE)
	{
		// 已选 → 取消
		SelectedCards.RemoveAt(ExistingIdx);
		ClickedCard->SetSelected(false);
	}
	else
	{
		// 未选 → 加入（若未到 K）
		if (SelectedCards.Num() >= K)
		{
			// 满了：忽略（也可考虑挤掉最早一张——Balatro 是直接拒绝，这里跟 Balatro）
			return;
		}
		SelectedCards.Add(ClickedCard);
		ClickedCard->SetSelected(true);
	}

	RefreshConfirmEnabled();
}

void UCardSelectionScreen::RefreshConfirmEnabled()
{
	const bool bShouldEnable = (SelectedCards.Num() >= K) && !bCommitAnimationPlaying;
	if (BeginShiftButton)
	{
		BeginShiftButton->SetIsEnabled(bShouldEnable);
		OnBeginShiftButtonEnableChanged(bShouldEnable);
	}
	// 计数变了也更新 CounterText
	if (UCh1LocSubsystem* Loc = GetLoc())
	{
		if (CounterText)
		{
			CounterText->SetText(FText::Format(
				Loc->Get(TEXT("CardSelection.Counter")),
				FText::AsNumber(SelectedCards.Num()), FText::AsNumber(K)));
		}
	}

	// Pearl 纯净度：选中卡里 Pearl-compatible 的张数（spec：Pearl 主路径暗示）
	int32 VisualPearlCount = 0;
	const TArray<UCardData*> VisualPickedCards = GetAssembledCardData();
	for (const UCardData* Card : VisualPickedCards)
	{
		if (Card && Card->bIsPearlCompatible)
		{
			++VisualPearlCount;
		}
	}
	const float SelectionRatio = K > 0 ? static_cast<float>(SelectedCards.Num()) / static_cast<float>(K) : 0.0f;
	const float PurityRatio = K > 0 ? static_cast<float>(VisualPearlCount) / static_cast<float>(K) : 0.0f;
	if (SelectionProgressBar)
	{
		SelectionProgressBar->SetPercent(FMath::Clamp(SelectionRatio, 0.0f, 1.0f));
	}
	if (PurityProgressBar)
	{
		PurityProgressBar->SetPercent(FMath::Clamp(PurityRatio, 0.0f, 1.0f));
	}
	OnSelectionVisualsChanged(SelectedCards.Num(), K, PurityRatio);

	if (PurityText)
	{
		int32 PearlCount = 0;
		for (const UJudgmentCardWidget* W : SelectedCards)
		{
			// 从 widget 取关联 CardData。HandleCardClicked 里 widget 已绑 CardData。
			// 这里通过 widget 的 GetCardData() 获取（widget 在 CardWidgetClass 里暴露）。
		}
		// 简单方案：从 PoolCards 中按 widget 找回 — 我们存了 SelectedCards 为 widget 而非 data。
		// 用现有 GetAssembledCardData() 取选中的 CardData 列表
		const TArray<UCardData*> Picked = GetAssembledCardData();
		for (const UCardData* C : Picked)
		{
			if (C && C->bIsPearlCompatible) ++PearlCount;
		}
		const float Ratio = K > 0 ? static_cast<float>(PearlCount) / K : 0.0f;
		FString Tail = TEXT("");
		if (Ratio >= 0.66f && SelectedCards.Num() == K) Tail = TEXT("（这套标准会通向某个完美的人）");
		else if (Ratio <= 0.33f && SelectedCards.Num() == K) Tail = TEXT("（这套标准很普通）");
		const FString Msg = FString::Printf(TEXT("标准纯净度 %d / %d %s"), PearlCount, K, *Tail);
		PurityText->SetText(FText::FromString(Msg));
	}
}

void UCardSelectionScreen::OnBeginShiftClicked()
{
	if (bCommitAnimationPlaying || SelectedCards.Num() < K)
	{
		return;
	}

	UAudioService::PlayCueStatic(this, FName("UI.Click"));
	UAudioService::PlayCueStatic(this, FName("Ch1.CardPlace"), 0.65f);

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);
	}

	StartAssemblyCommitAnimation();
}

void UCardSelectionScreen::StartAssemblyCommitAnimation()
{
	bCommitAnimationPlaying = true;
	PendingSelectedData = GetAssembledCardData();

	if (BeginShiftButton)
	{
		BeginShiftButton->SetIsEnabled(false);
		OnBeginShiftButtonEnableChanged(false);
	}

	OnAssemblyCommitStarted();

	int32 DiscardIdx = 0;
	for (int32 i = 0; i < AllCardWidgets.Num(); ++i)
	{
		UJudgmentCardWidget* Card = AllCardWidgets[i];
		if (!Card) continue;

		const int32 SelectedIdx = SelectedCards.IndexOfByKey(Card);
		if (SelectedIdx != INDEX_NONE)
		{
			Card->PlaySelectionCommitDrop(SelectedIdx, SelectedCards.Num(), SelectedIdx * CardOutroStaggerSec);
		}
		else
		{
			const float Direction = (i < AllCardWidgets.Num() * 0.5f) ? -1.0f : 1.0f;
			Card->PlaySelectionDiscardFly(DiscardIdx, AllCardWidgets.Num(), Direction, DiscardIdx * CardOutroStaggerSec);
			++DiscardIdx;
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			CommitAnimationTimerHandle,
			FTimerDelegate::CreateUObject(this, &UCardSelectionScreen::FinishAssemblyCommit),
			CommitAnimationDurationSec,
			false);
	}
	else
	{
		FinishAssemblyCommit();
	}
}

void UCardSelectionScreen::FinishAssemblyCommit()
{
	if (!bCommitAnimationPlaying)
	{
		return;
	}

	bCommitAnimationPlaying = false;
	OnAssemblyComplete.Broadcast(PendingSelectedData);
	PendingSelectedData.Reset();
}

TArray<UCardData*> UCardSelectionScreen::GetAssembledCardData() const
{
	TArray<UCardData*> Out;
	Out.Reserve(SelectedCards.Num());
	for (UJudgmentCardWidget* Card : SelectedCards)
	{
		if (Card)
		{
			if (UCardData* Data = Card->GetCardData())
			{
				Out.Add(Data);
			}
		}
	}
	return Out;
}
