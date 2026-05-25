// Copyright Yvaine / Checkmate. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "InspectionScreen.h"  // for FShiftResult / FOnShiftCompleted signature
#include "Chapter1GameMode.generated.h"

class UCardSelectionScreen;
class UCardData;
class UDollData;
class UUserWidget;
class ADollDisplay;
class UCh1LocStrings;

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shift", meta=(ClampMin="1"))
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
	int32 MaxMisjudgmentsBeforeFail = 3;
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

	/** 3D 娃娃 actor class（继承 ADollDisplay；可在 BP_DollDisplay 调 mesh / 颜色等）。 */
	UPROPERTY(EditDefaultsOnly, Category="Ch1|Classes")
	TSubclassOf<ADollDisplay> DollActorClass;

	/** 3D 娃娃 spawn 位置（世界坐标）。 */
	UPROPERTY(EditDefaultsOnly, Category="Ch1|Scene")
	FTransform DollSpawnTransform = FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, 100.0f));

	/** 本地化字典 asset（拖 DA_Ch1Loc 进来）。BeginPlay 注入到 UCh1LocSubsystem。 */
	UPROPERTY(EditDefaultsOnly, Category="Ch1|Classes")
	UCh1LocStrings* LocStringsAsset = nullptr;

	/** 班次列表。按 index 顺序依次跑。 */
	UPROPERTY(EditDefaultsOnly, Category="Ch1|Shifts")
	TArray<FShiftConfig> Shifts;

	/** 班次间过场停留时长（秒）。 */
	UPROPERTY(EditDefaultsOnly, Category="Ch1|Shifts", meta=(ClampMin="0.5"))
	float TransitionHoldSeconds = 2.5f;

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

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	UCardSelectionScreen* ActiveCardScreen = nullptr;

	UPROPERTY()
	UInspectionScreen* ActiveInspectionScreen = nullptr;

	UPROPERTY()
	UUserWidget* ActiveTransitionScreen = nullptr;

	UPROPERTY()
	ADollDisplay* ActiveDollActor = nullptr;

	int32 CurrentShiftIdx = 0;

	UFUNCTION()
	void HandleCardsAssembled(const TArray<UCardData*>& SelectedCards);

	UFUNCTION()
	void HandleShiftCompleted(FShiftResult Result);

	void BeginShift(int32 ShiftIdx);
	void ShowShiftTransition(int32 NextShiftIdx);
	void FinishCh1();
	void SetUIInputMode();

	void OpenCh2Map();
	FTimerHandle TwistHoldTimer;
};
