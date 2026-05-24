// Copyright Yvaine / Checkmate. All Rights Reserved.

#include "InspectionScreen.h"

#include "CardData.h"
#include "DollData.h"
#include "JudgmentEvaluator.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

namespace
{
	FString DescribeCard(const UCardData* Card)
	{
		if (!Card) return TEXT("(null)");
		const FString Label = Card->DisplayLabel.IsEmpty() ? Card->CardId.ToString() : Card->DisplayLabel.ToString();

		const TCHAR* DimTag = TEXT("");
		switch (Card->Dimension)
		{
			case ECardDimension::Posture:    DimTag = TEXT("姿态"); break;
			case ECardDimension::Hair:       DimTag = TEXT("发型"); break;
			case ECardDimension::Expression: DimTag = TEXT("表情"); break;
			case ECardDimension::Accessory:  DimTag = TEXT("饰物"); break;
		}
		return FString::Printf(TEXT("[%s] %s"), DimTag, *Label);
	}

	FString JoinTraitSet(const TSet<FName>& Traits)
	{
		if (Traits.Num() == 0) return TEXT("(无)");
		TArray<FString> Parts;
		Parts.Reserve(Traits.Num());
		for (const FName& T : Traits) Parts.Add(T.ToString());
		return FString::Join(Parts, TEXT(", "));
	}
}

void UInspectionScreen::SetShiftData(const TArray<UCardData*>& InJudgmentCards, const TArray<UDollData*>& InDollSequence)
{
	JudgmentCards = InJudgmentCards;
	DollSequence = InDollSequence;
	CurrentDollIndex = 0;
	CorrectCount = 0;
	WrongCount = 0;
	bAwaitingNext = false;
}

void UInspectionScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (PassButton)
	{
		PassButton->OnClicked.AddDynamic(this, &UInspectionScreen::OnPassClicked);
	}
	if (RejectButton)
	{
		RejectButton->OnClicked.AddDynamic(this, &UInspectionScreen::OnRejectClicked);
	}

	if (ToastText)
	{
		ToastText->SetText(FText::GetEmpty());
	}

	RenderJudgmentCardsList();
	RenderCurrentDoll();
	RenderProgress();
}

void UInspectionScreen::NativeDestruct()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AdvanceTimerHandle);
	}

	if (PassButton)   PassButton->OnClicked.RemoveAll(this);
	if (RejectButton) RejectButton->OnClicked.RemoveAll(this);

	Super::NativeDestruct();
}

void UInspectionScreen::OnPassClicked()
{
	if (bAwaitingNext) return;
	HandlePlayerChoice(/*bPlayerChosePass=*/true);
}

void UInspectionScreen::OnRejectClicked()
{
	if (bAwaitingNext) return;
	HandlePlayerChoice(/*bPlayerChosePass=*/false);
}

void UInspectionScreen::HandlePlayerChoice(bool bPlayerChosePass)
{
	if (!DollSequence.IsValidIndex(CurrentDollIndex))
	{
		return;
	}

	const UDollData* Doll = DollSequence[CurrentDollIndex];

	FString FailReason;
	const EJudgmentVerdict GroundTruth = UJudgmentEvaluator::EvaluateDoll(JudgmentCards, Doll, FailReason);

	const bool bPlayerCorrect =
		(bPlayerChosePass && GroundTruth == EJudgmentVerdict::Pass) ||
		(!bPlayerChosePass && GroundTruth == EJudgmentVerdict::Reject);

	if (bPlayerCorrect) CorrectCount++;
	else                WrongCount++;

	if (ToastText)
	{
		FString Toast;
		if (bPlayerCorrect)
		{
			Toast = FString::Printf(TEXT("✓ 正确（应%s）"),
				GroundTruth == EJudgmentVerdict::Pass ? TEXT("放行") : TEXT("丢弃"));
		}
		else
		{
			Toast = FString::Printf(TEXT("✗ 误判（应%s）—— %s"),
				GroundTruth == EJudgmentVerdict::Pass ? TEXT("放行") : TEXT("丢弃"),
				FailReason.IsEmpty() ? TEXT("符合所有判据") : *FailReason);
		}
		ToastText->SetText(FText::FromString(Toast));
	}

	bAwaitingNext = true;
	SetButtonsEnabled(false);

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			AdvanceTimerHandle,
			FTimerDelegate::CreateUObject(this, &UInspectionScreen::AdvanceToNextDoll),
			FMath::Max(0.1f, ToastHoldSeconds), false
		);
	}
}

void UInspectionScreen::AdvanceToNextDoll()
{
	CurrentDollIndex++;
	bAwaitingNext = false;

	if (ToastText)
	{
		ToastText->SetText(FText::GetEmpty());
	}

	if (CurrentDollIndex >= DollSequence.Num())
	{
		FShiftResult Result;
		Result.TotalDolls = DollSequence.Num();
		Result.CorrectCount = CorrectCount;
		Result.WrongCount = WrongCount;

		SetButtonsEnabled(false);
		OnShiftCompleted.Broadcast(Result);
		return;
	}

	SetButtonsEnabled(true);
	RenderCurrentDoll();
	RenderProgress();
}

void UInspectionScreen::RenderJudgmentCardsList()
{
	if (!JudgmentCardListText) return;

	TArray<FString> Lines;
	Lines.Reserve(JudgmentCards.Num());
	for (const UCardData* Card : JudgmentCards)
	{
		Lines.Add(DescribeCard(Card));
	}

	const FString Joined = Lines.Num() > 0
		? FString::Join(Lines, TEXT("\n"))
		: TEXT("(无当班判据)");

	JudgmentCardListText->SetText(FText::FromString(Joined));
}

void UInspectionScreen::RenderCurrentDoll()
{
	if (!DollAttributeText) return;

	if (!DollSequence.IsValidIndex(CurrentDollIndex))
	{
		DollAttributeText->SetText(FText::FromString(TEXT("(无娃娃)")));
		return;
	}

	const UDollData* Doll = DollSequence[CurrentDollIndex];
	if (!Doll)
	{
		DollAttributeText->SetText(FText::FromString(TEXT("(空 doll data)")));
		return;
	}

	const FString DisplayName = Doll->DisplayName.IsEmpty() ? Doll->DollId.ToString() : Doll->DisplayName.ToString();
	const FString Body = FString::Printf(
		TEXT("【%s】\n姿态：%s\n发型：%s\n表情：%s\n饰物：%s"),
		*DisplayName,
		*Doll->Posture.ToString(),
		*JoinTraitSet(Doll->HairTraits),
		*JoinTraitSet(Doll->ExpressionTraits),
		*JoinTraitSet(Doll->AccessoryTraits)
	);

	DollAttributeText->SetText(FText::FromString(Body));
}

void UInspectionScreen::RenderProgress()
{
	if (!ProgressText) return;

	const int32 OneBased = FMath::Min(CurrentDollIndex + 1, DollSequence.Num());
	ProgressText->SetText(FText::FromString(
		FString::Printf(TEXT("%d / %d"), OneBased, DollSequence.Num())
	));
}

void UInspectionScreen::SetButtonsEnabled(bool bEnabled)
{
	if (PassButton)   PassButton->SetIsEnabled(bEnabled);
	if (RejectButton) RejectButton->SetIsEnabled(bEnabled);
}
