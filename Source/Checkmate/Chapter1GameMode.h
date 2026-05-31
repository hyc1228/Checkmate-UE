// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Ch1ProgressPanelWidget.h"
#include "GameFramework/GameModeBase.h"
#include "InspectionScreen.h"  // for FShiftResult / FOnShiftCompleted signature
#include "Chapter1GameMode.generated.h"

class UCardSelectionScreen;
class UCardData;
class UDollData;
class UUserWidget;
class UCh1DeckReplacementWidget;
class UCh1StarterStandardWidget;
class ADollDisplay;
class UCh1LocStrings;
class APostProcessVolume;
class ULevelSequence;
class ULevelSequencePlayer;
class ALevelSequenceActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCh1PresentationCue, FName, CueId, int32, ShiftNumber, ECh1ProgressPanelMoment, Moment);

enum class ECh1DeckSlot : uint8
{
	Red,
	Black,
	Neutral
};

/**
 * 单个班次配置。在 BP_Chapter1GameMode.Shifts 数组里逐条填。
 *
 * 默认 4 班升级曲线（参考 design/gdd/judgment-card.md v1.3 §4）：
 *   Shift 1: N=3  K=3  无超时       — 教学
 *   Shift 2: N=5  K=3  无超时       — 单点微差
 *   Shift 3: N=7  K=4  每只 8 秒    — 完美伪装出现
 *   Shift 4: N=9  K=5  每只 6 秒    — 标准变化
 */
USTRUCT(BlueprintType)
struct FShiftConfig
{
	GENERATED_BODY()

	/** 这一班可选卡池（≥ K 张）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift")
	TArray<UCardData*> PoolCards;

	/** 娃娃池——按顺序循环（达 N 后回到 0）。班次以 CorrectGoal 为终止条件，而非用完池。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift")
	TArray<UDollData*> DollSequence;

	/** 玩家要选几张卡（≤ PoolCards.Num()）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift", meta=(ClampMin="0"))
	int32 K = 3;

	/** 本班需要正确判定多少次才下班（不论丢弃了多少错的）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift", meta=(ClampMin="1"))
	int32 CorrectGoal = 3;

	/** 选卡屏倒计时（秒）。到时强制开始（用未选卡顺序补齐）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift", meta=(ClampMin="1.0"))
	float AssemblyTimerSec = 30.0f;

	/** 检验屏每只娃娃超时（秒）。<= 0 表示无超时（教学班用）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift", meta=(ClampMin="0.0"))
	float DollTimeoutSec = 0.0f;

	/** 配额制度（与 CorrectGoal 二选一）：必须正确放行 N 次。0 = 不要求（仍用 CorrectGoal）。
	 *  默认 2 — 防止"全扔" exploit。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift", meta=(ClampMin="0"))
	int32 PassQuota = 2;

	/** 配额制度：必须正确丢弃 N 次。0 = 不要求。默认 1 — 防止"全放" exploit。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift", meta=(ClampMin="0"))
	int32 RejectQuota = 1;

	/** 累积误判到此数 → 班次失败（玩家被回选卡屏重新组装）。0 = 永不失败。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift", meta=(ClampMin="0"))
	int32 MaxMisjudgmentsBeforeFail = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift|Piecework")
	bool bUsePieceworkEconomy = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift|Piecework", meta=(ClampMin="0"))
	int32 DailyQuota = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift|Piecework", meta=(ClampMin="0"))
	int32 DayDollTarget = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift|Piecework", meta=(ClampMin="0"))
	int32 BaseGoodPrice = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift|Piecework", meta=(ClampMin="0"))
	int32 BaseRejectValue = 5;

	/** Optional day/shift reveal copy. Empty fields use generated fallback text. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift|Presentation")
	FText PanelEyebrow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift|Presentation")
	FText PanelTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift|Presentation")
	FText PanelBody;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift|Presentation")
	FName PanelCueId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift|Presentation")
	bool bMajorPresentationBeat = false;
};

/**
 * Ch1 GameMode：4 班循环（选卡 → 检验 → 班次过渡 → 下一班 …）。
 *
 * Editor 里 BP_Chapter1GameMode 填：
 *   - CardSelectionWidgetClass / InspectionWidgetClass / ShiftTransitionWidgetClass
 *   - Shifts 数组（推荐 4 条，按 GDD 升级曲线填）
 */
UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API AChapter1GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AChapter1GameMode();

	UPROPERTY(EditDefaultsOnly, Category="Ch1|Classes")
	TSubclassOf<UCardSelectionScreen> CardSelectionWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Ch1|Classes")
	TSubclassOf<UInspectionScreen> InspectionWidgetClass;

	/** 班次间「Shift X」过场 widget（继承 UUserWidget，含 BindWidget: TitleText:UTextBlock）。 */
	UPROPERTY(EditDefaultsOnly, Category="Ch1|Classes")
	TSubclassOf<UUserWidget> ShiftTransitionWidgetClass;

	/** Preferred day/shift panel. If unset, a native fallback panel is used. */
	UPROPERTY(EditDefaultsOnly, Category="Ch1|Classes")
	TSubclassOf<UCh1ProgressPanelWidget> ProgressPanelWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Ch1|Classes")
	TSubclassOf<UCh1StarterStandardWidget> StarterStandardWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Ch1|Classes")
	TSubclassOf<UCh1DeckReplacementWidget> DeckReplacementWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Ch1|Presentation")
	bool bUseNativeProgressPanelFallback = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Piecework")
	bool bUsePieceworkEconomyFlow = true;

	/** Runtime-only sequence balancing: keeps enough objectively-passable dolls in each paid shift without editing DataAssets. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Piecework|Dolls")
	bool bRebalanceDollSequenceForPiecework = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Piecework|Dolls", meta=(ClampMin="0.0", ClampMax="1.0"))
	float DesiredObjectivePassDollRatio = 0.68f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Piecework|Dolls", meta=(ClampMin="0"))
	int32 MinObjectivePassDollsPerDay = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Piecework|Dolls", meta=(ClampMin="0"))
	int32 MinObjectiveRejectDollsPerDay = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Roguelike")
	bool bUseEndlessRoguelikeRun = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Roguelike")
	bool bBlockAutomaticTwistInEndlessRun = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Chapter2 Gate")
	bool bAdvanceToChapter2OnQuotaMiss = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Chapter2 Gate")
	bool bAdvanceToChapter2OnMoneyGoal = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Chapter2 Gate", meta=(ClampMin="1"))
	int32 RunMoneyToChapter2Threshold = 700;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Chapter2 Gate")
	bool bAdvanceToChapter2OnCheckmateBurst = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Chapter2 Gate", meta=(ClampMin="1"))
	int32 CheckmateBurstDayMoneyThreshold = 260;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Piecework")
	bool bRequireBridgeCardBeforeCollapse = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Roguelike", meta=(ClampMin="1"))
	int32 MaxDeckCards = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Roguelike", meta=(ClampMin="0"))
	int32 MaxRedCards = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Roguelike", meta=(ClampMin="0"))
	int32 MaxBlackCards = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Roguelike", meta=(ClampMin="0"))
	int32 ReplacementQuotaPressure = 20;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Roguelike", meta=(ClampMin="0"))
	int32 QuotaFailurePressure = 20;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Roguelike", meta=(ClampMin="0.0"))
	float MoneyQuotaPressureRate = 0.04f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Roguelike", meta=(ClampMin="1.0"))
	float EndlessQuotaGrowthRate = 1.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Roguelike", meta=(ClampMin="0"))
	int32 EndlessQuotaLinearStep = 40;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Roguelike", meta=(ClampMin="1", ClampMax="30"))
	int32 MaxEndlessDollTarget = 18;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Piecework")
	UDollData* ForcedCollapseDoll = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Piecework", meta=(ClampMin="0"))
	int32 FalsePositiveImmediateValue = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Piecework", meta=(ClampMin="0"))
	int32 FalsePositiveRecallPenalty = 65;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Piecework", meta=(ClampMin="0"))
	int32 FalseNegativeDiscardPenalty = 25;

	/** 3D 娃娃 actor class（继承 ADollDisplay；可在 BP_DollDisplay 调 mesh / 颜色等）。 */
	UPROPERTY(EditDefaultsOnly, Category="Ch1|Classes")
	TSubclassOf<ADollDisplay> DollActorClass;

	/** 3D 娃娃 spawn 位置（世界坐标）。 */
	UPROPERTY(EditDefaultsOnly, Category="Ch1|Scene")
	FTransform DollSpawnTransform = FTransform(FRotator::ZeroRotator, FVector(-120.0f, 0.0f, 70.0f));

	/** Runtime camera polish for the graybox Ch1 test map. Keeps the doll/table visible without editing the map asset. */
	UPROPERTY(EditDefaultsOnly, Category="Ch1|Scene|Camera")
	bool bOverrideInspectionCameraTransform = true;

	/** Ch1 inspection should read like a front-facing inspection desk, not an angled board camera. */
	UPROPERTY(EditDefaultsOnly, Category="Ch1|Scene|Camera", meta=(EditCondition="bOverrideInspectionCameraTransform"))
	bool bUseFlatFrontInspectionCamera = true;

	UPROPERTY(EditDefaultsOnly, Category="Ch1|Scene|Camera", meta=(EditCondition="bOverrideInspectionCameraTransform"))
	FVector RuntimeCameraLocation = FVector(-850.0f, 0.0f, 155.0f);

	UPROPERTY(EditDefaultsOnly, Category="Ch1|Scene|Camera", meta=(EditCondition="bOverrideInspectionCameraTransform"))
	FVector RuntimeCameraLookAt = FVector(0.0f, 0.0f, 155.0f);

	UPROPERTY(EditDefaultsOnly, Category="Ch1|Scene|Camera", meta=(EditCondition="bOverrideInspectionCameraTransform", ClampMin="100.0"))
	float RuntimeCameraOrthoWidth = 620.0f;

	UPROPERTY(EditDefaultsOnly, Category="Ch1|Scene|Camera")
	bool bSpawnRuntimeInspectionBackdrop = true;

	/** 本地化字典 asset（拖 DA_Ch1Loc 进来）。BeginPlay 注入到 UCh1LocSubsystem。 */
	UPROPERTY(EditDefaultsOnly, Category="Ch1|Classes")
	UCh1LocStrings* LocStringsAsset = nullptr;

	/** 班次列表。按 index 顺序依次跑。 */
	UPROPERTY(EditDefaultsOnly, Category="Ch1|Shifts")
	TArray<FShiftConfig> Shifts;

	/** 班次间过场停留时长（秒）。 */
	UPROPERTY(EditDefaultsOnly, Category="Ch1|Shifts", meta=(ClampMin="0.5"))
	float TransitionHoldSeconds = 2.5f;

	UPROPERTY(EditDefaultsOnly, Category="Ch1|Presentation", meta=(ClampMin="0.5"))
	float InitialPanelHoldSeconds = 2.6f;

	UPROPERTY(EditDefaultsOnly, Category="Ch1|Presentation", meta=(ClampMin="0.0"))
	float ProgressPanelPostProcessDuration = 0.75f;

	UPROPERTY(EditDefaultsOnly, Category="Ch1|Presentation", meta=(ClampMin="0.0"))
	float ProgressPanelPostProcessPeak = 0.85f;

	UPROPERTY(BlueprintAssignable, Category="Ch1|Presentation")
	FOnCh1PresentationCue OnPresentationCue;

	UFUNCTION(BlueprintCallable, Category="Ch1|Presentation")
	void TriggerPresentationCue(FName CueId, int32 ShiftNumber, ECh1ProgressPanelMoment Moment, bool bMajorBeat);

	UFUNCTION(BlueprintImplementableEvent, Category="Ch1|Presentation")
	void K2_OnPresentationCue(FName CueId, int32 ShiftNumber, ECh1ProgressPanelMoment Moment, bool bMajorBeat);

	// ── Ch1 Death Branch ───────────────────────────────────────────────────

	/** 整章累计误判达到该数时触发死亡过场。0 = 关闭。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Death", meta=(ClampMin="0"))
	int32 ChapterDeathMisjudgmentThreshold = 0;

	/** 过场 Sequence map。推荐 key: Ch1.Death。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ch1|Death", meta=(ForceInlineRow))
	TMap<FName, TSoftObjectPtr<ULevelSequence>> NamedSequences;

	/** 死亡分支使用的 sequence key。未绑定时走轻量 fallback + Blueprint event。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Death")
	FName DeathSequenceKey = TEXT("Ch1.Death");

	/** fallback 中“手遮住屏幕”的黑场进入时长。正式资产请用 DeathSequenceKey 替代。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Ch1|Death", meta=(ClampMin="0.05"))
	float DeathHandCoverSeconds = 0.55f;

	/** 死亡过场是否已触发，避免误判与班次失败重复进入。 */
	UPROPERTY(BlueprintReadOnly, Category="Ch1|Death")
	bool bDeathCinematicTriggered = false;

	/** 给 BP/Sequencer 接三段表现：手遮屏 → 扣子掉 → 传送带丢弃。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Ch1|Death")
	void K2_OnDeathCinematicRequested(int32 TotalMisjudgments);

	UFUNCTION(BlueprintCallable, Category="Ch1|Death")
	void TriggerDeathCinematic();

	UFUNCTION(BlueprintCallable, Category="Ch1|Sequences")
	bool TryPlaySequence(FName Key);

	// ── Twist (Ch1 → Ch2 翻转) ──────────────────────────────────────────────

	/** 翻转目标关卡（默认 Ch2Test）。 */
	UPROPERTY(EditDefaultsOnly, Category="Ch1|Twist")
	FName Ch2MapName = TEXT("Ch2Test");

	/** 翻转 fade-to-white 时长（秒）。lean PV 版 ≈ 1.5s。 */
	UPROPERTY(EditDefaultsOnly, Category="Ch1|Twist", meta=(ClampMin="0.5"))
	float TwistFadeSeconds = 1.5f;

	/** 翻转后停顿（秒），然后 OpenLevel(Ch2)。 */
	UPROPERTY(EditDefaultsOnly, Category="Ch1|Twist", meta=(ClampMin="0.0"))
	float TwistHoldSeconds = 2.0f;

	/** 是否处于翻转触发态（避免重复 fire）。 */
	UPROPERTY(BlueprintReadOnly, Category="Ch1|Twist")
	bool bTwistTriggered = false;

	/** 当玩家放行了带 bIsTwistTrigger=true 的 Pearl 娃娃时 fire。 */
	UFUNCTION(BlueprintCallable, Category="Ch1|Twist")
	void RequestTwist();

	UFUNCTION(BlueprintImplementableEvent, Category="Ch1|Twist")
	void K2_OnChapter2TransitionReason(FName Reason, int32 RunMoney, int32 MoneyGoal, bool bQuotaMet);

	// ── PV capture helpers (development-only console seams) ────────────────

	/** Jump the running Ch1 scene into a named PV capture preset. */
	UFUNCTION(Exec)
	void PV_Ch1Preset(FName PresetId);

	/** Force the Ch1 button-edge post-process to a fixed value for recording A8. */
	UFUNCTION(Exec)
	void PV_SetEdgeOpacity(float Opacity = 0.5f, float DurationSec = 0.5f);

	/** Make the current inspection doll look back at the camera for recording A7. */
	UFUNCTION(Exec)
	void PV_TriggerDollLookAtCamera(float HoldSeconds = 0.5f);

	/** Force the final Pearl / fallback lead-in without changing judgment rules. */
	UFUNCTION(Exec)
	void PV_TriggerFinalPearl(bool bFallback = true);

	/** Trigger the Ch1->Ch2 twist lead-in from the current recording state. */
	UFUNCTION(Exec)
	void PV_TriggerTwistLeadIn();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	UCardSelectionScreen* ActiveCardScreen = nullptr;

	UPROPERTY()
	UInspectionScreen* ActiveInspectionScreen = nullptr;

	UPROPERTY()
	UUserWidget* ActiveTransitionScreen = nullptr;

	UPROPERTY()
	UCh1StarterStandardWidget* ActiveStarterStandardScreen = nullptr;

	UPROPERTY()
	UCh1DeckReplacementWidget* ActiveDeckReplacementScreen = nullptr;

	UPROPERTY()
	ADollDisplay* ActiveDollActor = nullptr;

	UPROPERTY()
	TArray<UCardData*> LastSelectedCards;

	UPROPERTY()
	TArray<UCardData*> CurrentDeckCards;

	UPROPERTY()
	UCardData* PendingDraftCard = nullptr;

	int32 CurrentShiftIdx = 0;
	FShiftConfig ActiveRuntimeShiftConfig;
	bool bQuotaCollapseActive = false;
	bool bStarterStandardSeen = false;
	bool bInspectionLaunchInProgress = false;
	bool bForceAllowChapter2Transition = false;
	int32 RunTotalMoneyEarned = 0;
	int32 RunCardReplacementCount = 0;
	int32 RunDraftSkipCount = 0;
	int32 RunQuotaFailureCount = 0;
	FName PendingChapter2TransitionReason = NAME_None;

	UFUNCTION()
	void HandleCardsAssembled(const TArray<UCardData*>& SelectedCards);

	UFUNCTION()
	void HandleDeckReplacementResolved(UCardData* NewCard, UCardData* ReplacedCard);

	UFUNCTION()
	void HandleStarterStandardConfirmed();

	UFUNCTION()
	void HandleShiftCompleted(FShiftResult Result);

	UFUNCTION()
	void HandleMisjudgmentRecorded(int32 ShiftMisjudgments, int32 ShiftWrongCount);

	void BeginShift(int32 ShiftIdx);
	void ShowStarterStandardPanel();
	void LaunchInspectionForCurrentShift();
	bool ApplyDraftSelectionOrRequestReplacement(const TArray<UCardData*>& SelectedCards);
	bool CanAddCardToDeck(UCardData* Card) const;
	void AddCardToDeck(UCardData* Card);
	TArray<UCardData*> GetReplacementCandidates(UCardData* NewCard) const;
	ECh1DeckSlot GetDeckSlot(const UCardData* Card) const;
	int32 CountDeckSlot(ECh1DeckSlot Slot) const;
	int32 GetUnlockedSlotCap(ECh1DeckSlot Slot, int32 ShiftIdx) const;
	FShiftConfig BuildRuntimeShiftConfig(int32 ShiftIdx) const;
	TArray<UCardData*> BuildRoguelikeCardPool(int32 ShiftIdx) const;
	TArray<UDollData*> BuildBalancedInspectionDollSequence(
		const TArray<UDollData*>& SourceSequence,
		const TArray<UCardData*>& ActiveCards,
		int32 TargetCount) const;
	bool DoesDollPassActiveCards(const UDollData* Doll, const TArray<UCardData*>& ActiveCards) const;
	int32 ComputeDailyQuota(const FShiftConfig& BaseCfg, int32 ShiftIdx) const;
	int32 ComputeDayDollTarget(const FShiftConfig& BaseCfg, int32 ShiftIdx) const;
	void ClearDeckReplacementScreen();
	TArray<UCardData*> BuildDraftPoolForShift(const FShiftConfig& Cfg) const;
	void ShowShiftIntro(int32 ShiftIdx);
	void ShowShiftTransition(int32 NextShiftIdx);
	void ShowRetryTransition();
	void ShowProgressPanel(
		int32 ShiftIdx,
		ECh1ProgressPanelMoment Moment,
		float HoldSeconds,
		FTimerDelegate CompletionDelegate);
	void FinishCh1();
	void TriggerChapter2Transition(FName Reason, bool bQuotaMet);
	void StartQuotaCollapseInspection();
	UDollData* FindFallbackCollapseDoll() const;
	bool HasSelectedBridgeCard() const;
	bool HasSelectedCheckmateCard() const;
	void PlayTwistOpticalBurnout();
	void SetUIInputMode();
	void ApplyRuntimeCameraFraming(AActor* CameraActor) const;
	void EnsureRuntimeInspectionBackdrop();
	FCh1ProgressPanelPayload BuildProgressPanelPayload(int32 ShiftIdx, ECh1ProgressPanelMoment Moment) const;
	void StartProgressPanelPostProcessCue(bool bMajorBeat);
	void UpdateProgressPanelPostProcessCue(float DeltaSeconds);

	void OpenCh2Map();
	void PlayDeathButtonDropFallback();
	FTimerHandle TwistHoldTimer;
	FTimerHandle TwistOpticalBurnoutTimer;
	FTimerHandle ProgressPanelTimer;
	FTimerHandle DeathFallbackTimer;

	UPROPERTY()
	ULevelSequencePlayer* ActiveSequencePlayer = nullptr;

	UPROPERTY()
	ALevelSequenceActor* ActiveSequenceActor = nullptr;

	UPROPERTY()
	APostProcessVolume* ProgressPanelPostProcessVolume = nullptr;

	UPROPERTY()
	TArray<AActor*> RuntimeInspectionBackdropActors;

	float ProgressPanelPostProcessElapsed = 0.0f;
	bool bProgressPanelPostProcessActive = false;
	bool bProgressPanelPostProcessMajor = false;
	int32 TotalMisjudgmentsThisChapter = 0;
};
