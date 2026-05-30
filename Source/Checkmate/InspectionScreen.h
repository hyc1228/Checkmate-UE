// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InspectionScreen.generated.h"

class UCardData;
class UDollData;
class UTextBlock;
class UButton;
class UBorder;
class ADollDisplay;
class UCh1LocSubsystem;
class APostProcessVolume;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UCh1SelectedCardsBoardWidget;

/**
 * 单班检验结果。班次结束时通过 OnShiftCompleted 广播给 GameMode。
 */
USTRUCT(BlueprintType)
struct FShiftResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Shift")
	int32 TotalDolls = 0;

	UPROPERTY(BlueprintReadOnly, Category="Shift")
	int32 CorrectCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Shift")
	int32 WrongCount = 0;

	UPROPERTY(BlueprintReadOnly, Category="Shift")
	int32 TrueAcceptCount = 0;  // 正确放行（合规且放行）

	UPROPERTY(BlueprintReadOnly, Category="Shift")
	int32 TrueRejectCount = 0;  // 正确丢弃（不合规且丢弃）

	/** true = 班次成功；false = 班次失败（误判超上限）。 */
	UPROPERTY(BlueprintReadOnly, Category="Shift")
	bool bSuccess = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShiftCompleted, FShiftResult, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMisjudgmentRecorded, int32, ShiftMisjudgments, int32, ShiftWrongCount);

/**
 * Ch1 班次中的娃娃检验界面（灯盒版）。
 *
 * 工作流：
 *   1. GameMode 调 SetShiftData(K 张判据卡, 3 只娃娃)
 *   2. AddToViewport → NativeConstruct 渲染第 1 只娃娃 + K 张判据卡 label
 *   3. 玩家审视 → 按 PassButton 或 RejectButton
 *   4. UJudgmentEvaluator 算出客观裁决 → 与玩家选择对比 → toast「正确 / 误判」
 *   5. 1.0s 后推进到下一只
 *   6. 全部 3 只过完 → 广播 OnShiftCompleted(FShiftResult)
 *
 * WBP_InspectionScreen 必须提供以下命名 widget：
 *   - JudgmentCardListText (TextBlock) — 显示当班 K 张卡的字面要求
 *   - DollAttributeText    (TextBlock) — 显示当前娃娃 4 维属性
 *   - ProgressText         (TextBlock) — "1 / 3"
 *   - ToastText            (TextBlock) — "正确" / "误判：xxx"
 *   - PassButton           (Button)
 *   - RejectButton         (Button)
 */
UCLASS(BlueprintType, Blueprintable)
class CHECKMATE_API UInspectionScreen : public UUserWidget
{
	GENERATED_BODY()

public:
	/** GameMode 在 AddToViewport 前调用。 */
	UFUNCTION(BlueprintCallable, Category="Inspection")
	void SetShiftData(const TArray<UCardData*>& InJudgmentCards, const TArray<UDollData*>& InDollSequence);

	/** GameMode 在 spawn 完 3D 娃娃 actor 后调用，把 actor 引用交给本 screen。 */
	UFUNCTION(BlueprintCallable, Category="Inspection")
	void SetDollActor(ADollDisplay* InActor);

	/** ADollDisplay 在手势确认/丢弃瞬间调（早事件，立即触发评估 + 反馈）。 */
	void HandleGestureFromDoll(bool bConfirm);

	/** ADollDisplay 动画播完时调（晚事件，触发推进下一只或下班）。 */
	void OnDollAnimComplete();

	/** ADollDisplay 在玩家做出「扔」动作的瞬间调（独立于评估）—— 触发轻微震 + 风声占位。 */
	void PlayTossActionFeedback();

	/** ADollDisplay 在印章砸到娃娃的瞬间调 —— 触发强震 + 冲击。 */
	void PlayStampImpactFeedback();

	/** Twist 前的视野反转 surge：按扣边缘变实，供 GameMode / Sequencer 事件调用。 */
	UFUNCTION(BlueprintCallable, Category="Inspection|Optical Inversion")
	void TriggerOpticalInversionSurge(float DurationSec = 1.0f);

	/** PV / Sequencer seam: temporarily lock button-edge opacity, then return to runtime formula. */
	UFUNCTION(BlueprintCallable, Category="Inspection|Optical Inversion")
	void SetOpticalInversionEdgeOverride(float EdgeOpacity, float DurationSec = 0.5f, float EdgeRadius = 0.20f);

	/** Twist EyeFall 阶段：爆白 + 短反相 + 色散。 */
	UFUNCTION(BlueprintCallable, Category="Inspection|Optical Inversion")
	void PlayTwistOpticalInversionBurnout(float DurationSec = 1.0f);

	/** MechanicalEyed 后把按扣覆盖层淡出。 */
	UFUNCTION(BlueprintCallable, Category="Inspection|Optical Inversion")
	void FadeOpticalInversionForMechanicalEye(float DurationSec = 1.5f);

	/** WBP hook for icon/stamp feedback so verdicts need less explanatory text. */
	UFUNCTION(BlueprintImplementableEvent, Category="Inspection|Feedback")
	void OnVerdictVisualFeedback(bool bCorrect, bool bPlayerChosePass, bool bGroundTruthPass);

	/** PV / console seam: move the inspection flow to a named doll from the current sequence. */
	UFUNCTION(BlueprintCallable, Category="Inspection|PV")
	bool PV_SetCurrentDollById(FName DollId);

	/** PV / console seam: move to the configured Pearl / fallback twist-trigger doll. */
	UFUNCTION(BlueprintCallable, Category="Inspection|PV")
	bool PV_SetCurrentDollToTwistTrigger(bool bPreferLast = true);

	/** 班次结束时广播。 */
	UPROPERTY(BlueprintAssignable, Category="Inspection")
	FOnShiftCompleted OnShiftCompleted;

	/** 每次误判立刻广播。Chapter1GameMode 用它统计整章死亡阈值。 */
	UPROPERTY(BlueprintAssignable, Category="Inspection")
	FOnMisjudgmentRecorded OnMisjudgmentRecorded;

	/** toast 停留时间（秒），到点后自动推进下一只。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inspection|Tuning", meta=(ClampMin="0.1"))
	float ToastHoldSeconds = 1.0f;

	/** 每只娃娃超时（秒）。<=0 表示无超时（Shift 1-2）。GameMode 按班次配置改写。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inspection|Tuning", meta=(ClampMin="0.0"))
	float DollTimeoutSec = 0.0f;

	/** 本班需要正确判定多少次才下班。GameMode 按班次配置改写。
	 *  若 PassQuota/RejectQuota 都 >0，本字段被忽略（用配额制度判断完成）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inspection|Tuning", meta=(ClampMin="1"))
	int32 CorrectGoal = 3;

	/** 必须正确放行 N 次才能下班（0 = 不要求）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inspection|Tuning", meta=(ClampMin="0"))
	int32 PassQuota = 0;

	/** 必须正确丢弃 N 次才能下班（0 = 不要求）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inspection|Tuning", meta=(ClampMin="0"))
	int32 RejectQuota = 0;

	/** 累积误判到此数 → 班次失败（玩家会被回选卡屏）。0 = 永不失败。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inspection|Tuning", meta=(ClampMin="0"))
	int32 MaxMisjudgmentsBeforeFail = 3;

	// ── 反馈 juice 参数 ────────────────────────────────────────────────────

	/** 正确闪屏颜色（绿调）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback")
	FLinearColor CorrectFlashColor = FLinearColor(0.2f, 0.9f, 0.4f, 1.0f);

	/** 错误闪屏颜色（红调）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback")
	FLinearColor WrongFlashColor = FLinearColor(0.95f, 0.25f, 0.25f, 1.0f);

	/** 闪屏峰值不透明度（0-1）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback", meta=(ClampMin="0.0", ClampMax="1.0"))
	float FlashPeakAlpha = 0.45f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback", meta=(ClampMin="0.0", ClampMax="1.0"))
	float CorrectFlashPeakAlpha = 0.36f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback", meta=(ClampMin="0.0", ClampMax="1.0"))
	float WrongFlashPeakAlpha = 0.72f;

	/** 闪屏总时长（秒）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback", meta=(ClampMin="0.05"))
	float FlashDurationSec = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback", meta=(ClampMin="0.05"))
	float CorrectFlashDurationSec = 0.38f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback", meta=(ClampMin="0.05"))
	float WrongFlashDurationSec = 0.58f;

	/** 错误震屏振幅（像素），正确震屏用 1/3。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback", meta=(ClampMin="0.0"))
	float WrongShakeAmplitude = 14.0f;

	/** 震屏时长（秒）。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback", meta=(ClampMin="0.05"))
	float ShakeDurationSec = 0.30f;

	/** Full-frame verdict pulse, layered through the Ch1 post-process volume. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback|PostProcess", meta=(ClampMin="0.05"))
	float VerdictPostProcessDurationSec = 0.55f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback|PostProcess", meta=(ClampMin="0.0", ClampMax="2.0"))
	float CorrectPostProcessPeak = 0.55f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback|PostProcess", meta=(ClampMin="0.0", ClampMax="2.0"))
	float WrongPostProcessPeak = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback|PostProcess")
	FLinearColor CorrectPostProcessTint = FLinearColor(0.08f, 1.0f, 0.22f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback|PostProcess")
	FLinearColor WrongPostProcessTint = FLinearColor(1.0f, 0.05f, 0.04f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback|Audio")
	FName CorrectVerdictCue = TEXT("Ch1.Correct");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback|Audio")
	FName WrongVerdictCue = TEXT("Ch1.Wrong");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback|Audio")
	FName PassActionCue = TEXT("Ch1.Stamp");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback|Audio")
	FName RejectActionCue = TEXT("Ch1.Toss");

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback|Audio", meta=(ClampMin="0.0", ClampMax="4.0"))
	float CorrectVerdictVolume = 0.65f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback|Audio", meta=(ClampMin="0.0", ClampMax="4.0"))
	float WrongVerdictVolume = 1.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Feedback|Audio", meta=(ClampMin="0.0", ClampMax="4.0"))
	float ButtonFallbackActionVolume = 0.85f;

	// ── Ch1 Optical Inversion Layer / PP_SubliminalEdge ─────────────────────

	/** 可选：全屏后处理材质 PP_SubliminalEdge。未设置时仍使用内置 PP fallback。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Optical Inversion")
	UMaterialInterface* OpticalInversionMaterial = nullptr;

	/** EdgeOpacity(m)=clamp(m*Step+Base, Base, Max)。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Optical Inversion", meta=(ClampMin="0.0", ClampMax="1.0"))
	float OpticalBaseOpacity = 0.05f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Optical Inversion", meta=(ClampMin="0.0", ClampMax="1.0"))
	float OpticalMisjudgmentStep = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Optical Inversion", meta=(ClampMin="0.0", ClampMax="1.0"))
	float OpticalMaxOpacity = 0.60f;

	/** Surge 时按扣覆盖向屏幕中心推进的强度。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Optical Inversion", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SurgeEdgeRadius = 0.25f;

	/** TwistBurnout 峰值短反相强度；材质内应 clamp，避免廉价长时间负片化。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Inspection|Optical Inversion", meta=(ClampMin="0.0", ClampMax="1.0"))
	float BurnoutInvertPeak = 0.40f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UTextBlock* JudgmentCardListText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UTextBlock* DollAttributeText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UTextBlock* ProgressText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="UMG Binding")
	UTextBlock* ToastText = nullptr;

	/** v0.5：UI 按钮废弃，留 BindWidgetOptional 兼容旧 WBP；3D 手势已替代。 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UMG Binding")
	UButton* PassButton = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UMG Binding")
	UButton* RejectButton = nullptr;

	/** 全屏闪屏 overlay（Border，覆盖整屏）。WBP 里命名 FlashOverlay，可选。 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UMG Binding")
	UBorder* FlashOverlay = nullptr;

	/** 倒计时显示（用于 Shift 3+ 的 DollTimeoutSec）。可选。 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UMG Binding")
	UTextBlock* DollTimerText = nullptr;

	/** 显示「还需正确 N 只」等班次目标信息。 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UMG Binding")
	UTextBlock* RemainingText = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UMG Binding")
	UButton* LangToggleButton = nullptr;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional), Category="UMG Binding")
	UTextBlock* LangToggleLabel = nullptr;

	UFUNCTION()
	void OnLangToggleClicked();

	UFUNCTION()
	void OnPassClicked();

	UFUNCTION()
	void OnRejectClicked();

private:
	UPROPERTY()
	TArray<UCardData*> JudgmentCards;

	UPROPERTY()
	TArray<UDollData*> DollSequence;

	UPROPERTY()
	ADollDisplay* DollActor = nullptr;

	int32 CurrentDollIndex = 0;
	int32 CorrectCount = 0;
	int32 WrongCount = 0;
	int32 TrueAcceptCount = 0;
	int32 TrueRejectCount = 0;
	bool bAwaitingNext = false;
	bool bShiftEnded = false;
	bool bQuotaFallbackLogged = false;
	FTimerHandle ShiftEndTimer;

	// 误判统计 + 文案漂移 state
	int32 MisjudgmentCount = 0;
	bool bHasDriftedFalsePos = false;  // 一次性：第一次 FalsePos 时 toast 漂一次
	bool bHasDriftedFalseNeg = false;  // 一次性：第一次 FalseNeg 时 toast 漂一次

	UPROPERTY()
	APostProcessVolume* MisjudgmentPPVolume = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* OpticalInversionMID = nullptr;

	float OpticalEdgeOpacityT = 0.05f;
	float OpticalEdgeRadiusT = 0.0f;
	float OpticalPulseT = 0.0f;
	float OpticalInvertT = 0.0f;
	float OpticalBurnoutT = 0.0f;
	float OpticalMechanicalFadeT = 0.0f;

	float OpticalTargetEdgeOpacityT = 0.05f;
	float OpticalTargetEdgeRadiusT = 0.0f;
	float OpticalTargetPulseT = 0.0f;
	float OpticalTargetInvertT = 0.0f;
	float OpticalTargetBurnoutT = 0.0f;
	float OpticalTargetMechanicalFadeT = 0.0f;

	float OpticalSurgeRemainingSec = 0.0f;
	float OpticalBurnoutRemainingSec = 0.0f;
	float OpticalBurnoutTotalSec = 0.0f;
	float OpticalFadeRemainingSec = 0.0f;
	float OpticalFadeTotalSec = 0.0f;

	/** 桌面 3D 纸卡 actors（diegetic 标准卡，让玩家在 3D 检验时看见自己选的判据）。 */
	UPROPERTY()
	TArray<AActor*> DeskCardActors;

	TArray<FVector> DeskCardStartLocations;
	TArray<FVector> DeskCardTargetLocations;
	TArray<FRotator> DeskCardStartRotations;
	TArray<FRotator> DeskCardTargetRotations;
	TArray<float> DeskCardDropElapsed;
	TArray<float> DeskCardDropDelay;

	/** 桌面纸卡 spawn 起点（doll 前方下方）。Y 方向是相机左右展开方向。 */
	UPROPERTY(EditDefaultsOnly, Category="Inspection|DeskCards")
	FVector DeskCardOrigin = FVector(-30.0f, -240.0f, 40.0f);

	UPROPERTY(EditDefaultsOnly, Category="Inspection|DeskCards")
	float DeskCardSpacing = 90.0f;

	/** 卡的"立起来"旋转（默认 Pitch=-90 让 plane normal 朝相机 -X 方向）。 */
	UPROPERTY(EditDefaultsOnly, Category="Inspection|DeskCards")
	FRotator DeskCardRotation = FRotator(-90.0f, 0.0f, 0.0f);

	/** 桌面纸卡 emissive 材质（绑 M_Highlight 让卡发光显眼）。 */
	UPROPERTY(EditDefaultsOnly, Category="Inspection|DeskCards")
	UMaterialInterface* DeskCardMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Inspection|DeskCards", meta=(ClampMin="0.0"))
	float DeskCardDropHeight = 140.0f;

	UPROPERTY(EditDefaultsOnly, Category="Inspection|DeskCards", meta=(ClampMin="0.05"))
	float DeskCardDropDurationSec = 0.45f;

	UPROPERTY(EditDefaultsOnly, Category="Inspection|DeskCards", meta=(ClampMin="0.0"))
	float DeskCardDropStaggerSec = 0.08f;

	UPROPERTY(EditDefaultsOnly, Category="Inspection|DeskCards")
	bool bHideJudgmentTextWhenDeskCardsSpawn = true;

	UPROPERTY(EditDefaultsOnly, Category="Inspection|DeskCards")
	bool bHideDollAttributeTextWhenDollActorSpawn = true;

	/** 左上角悬挂判据板：显示玩家已选卡，hover 时显示详细解释。 */
	UPROPERTY(EditDefaultsOnly, Category="Inspection|SelectedCardsBoard")
	bool bShowSelectedCardsBoard = true;

	UPROPERTY(EditDefaultsOnly, Category="Inspection|SelectedCardsBoard")
	TSubclassOf<UCh1SelectedCardsBoardWidget> SelectedCardsBoardWidgetClass;

	UPROPERTY()
	UCh1SelectedCardsBoardWidget* SelectedCardsBoardWidget = nullptr;

	FTimerHandle AdvanceTimerHandle;
	FTimerHandle DollTimeoutHandle;
	FTimerHandle OpticalOverrideTimerHandle;
	float CurrentDollTimeRemaining = 0.0f;

	// 反馈状态（NativeTick 驱动衰减）
	float FlashElapsed = 0.0f;
	float FlashTotal = 0.0f;
	FLinearColor FlashTargetColor = FLinearColor::White;
	float ActiveFlashPeakAlpha = 0.45f;
	float ShakeElapsed = 0.0f;
	float ShakeTotal = 0.0f;
	float ShakeAmplitude = 0.0f;
	float VerdictPostProcessElapsed = 0.0f;
	float VerdictPostProcessTotal = 0.0f;
	float VerdictPostProcessPeak = 0.0f;
	bool bVerdictPostProcessCorrect = true;

	void RenderJudgmentCardsList();
	void RenderCurrentDoll();
	void RenderProgress();
	void HandlePlayerChoice(bool bPlayerChosePass);
	void AdvanceToNextDoll();
	void SetButtonsEnabled(bool bEnabled);

	void StartFeedback(bool bCorrect, bool bPlayerChosePass, bool bGroundTruthPass);
	void PlayVerdictAudio(bool bCorrect, bool bPlayerChosePass);
	void StartVerdictPostProcessFeedback(bool bCorrect);
	void UpdateVerdictPostProcessFeedback(float DeltaSeconds);
	void RestorePostProcessAfterVerdictFeedback();
	void HandleDollTimeout();
	void PushCurrentDollToActor();

	/** 误判累积导致镜头 vignette↑/saturation↓（论点：判错让世界变冷）。 */
	void ApplyMisjudgmentPressure();

	/** 创建/刷新全局 Ch1 optical inversion PP volume。 */
	void EnsureOpticalInversionPostProcess();
	void UpdateOpticalInversion(float DeltaSeconds);
	void PushOpticalInversionParameters();
	float ComputeOpticalEdgeOpacityFromMisjudgments() const;
	void RestoreOpticalInversionRuntimeTargets();

	/** Spawn/refresh 桌面 K 张 3D 纸卡（diegetic 标准）。 */
	void SpawnDeskCards();
	void ClearDeskCards();
	void UpdateDeskCardDrops(float DeltaSeconds);
	void SpawnSelectedCardsBoard();
	void ClearSelectedCardsBoard();

	/** 立即检查本班是否该终止（成功/失败）。返回 true 表示已 schedule 终止 broadcast。 */
	bool CheckShiftTermination();
	bool IsQuotaPossibleWithCurrentCriteria() const;

	void RefreshLocalizedTexts();
	UCh1LocSubsystem* GetLoc() const;

	FDelegateHandle LangChangedHandle;
};
