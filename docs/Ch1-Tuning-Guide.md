# Ch1 选卡 + 检验循环 — 代码使用 / 调参文档

> 范围：第一章「灯盒检验员」slice v0.3 — Balatro 风点选 → 检验放行/丢弃 → 班次结束统计。
> 适用对象：本仓库（UE 5.7 C++ 工程）。设计文档在 sibling repo（见 `CLAUDE.md`）。

## 0. 一图看懂数据流

```
┌─ AChapter1GameMode (C++) ────────────────────────────┐
│  BP: /Game/Ch1/Blueprints/BP_Chapter1GameMode        │
│  ──────────────────────────────────────────────────  │
│  BeginPlay 时：                                       │
│   1. CreateWidget<UCardSelectionScreen>(...)         │
│   2. screen->SetShiftConfig(ShiftPoolCards, K, sec)  │
│   3. AddToViewport                                    │
│                                                       │
│  HandleCardsAssembled (delegate)：                    │
│   1. 切到 UInspectionScreen                           │
│   2. screen->SetShiftData(selected, dolls)            │
│                                                       │
│  HandleShiftCompleted (delegate)：log 班次统计        │
└──────────────────────────────────────────────────────┘
         ↓ owns
┌─ UCardSelectionScreen ──────────────────────────────┐
│  WBP: /Game/Ch1/UI/WBP_CardSelectionScreen          │
│  ──────────────────────────────────────────────────  │
│  NativeConstruct:                                    │
│   1. for card in PoolCards:                          │
│        spawn WBP_JudgmentCard                        │
│        AddChild(CardPoolContainer)  [CanvasPanel]    │
│        compute fan position + rotation               │
│   2. start 30s countdown                             │
│                                                       │
│  HandleCardClicked(card)：toggle selection (cap K)   │
│  Confirm 按下 → OnAssemblyComplete.Broadcast(cards)  │
└──────────────────────────────────────────────────────┘
         ↓ spawns N instances
┌─ UJudgmentCardWidget ──────────────────────────────┐
│  WBP: /Game/Ch1/UI/WBP_JudgmentCard                  │
│  ──────────────────────────────────────────────────  │
│  - 拥有 UCardData 引用                                │
│  - hover：Lift + Scale + Tilt（鼠标位置驱动）         │
│  - click：toggle bIsSelected → 永久 Lift              │
│  - hover 中：z-order +1000（顶到最上层）             │
└──────────────────────────────────────────────────────┘
         ↓ reads
┌─ UCardData / UDollData (DataAssets) ────────────────┐
│  /Game/Ch1/Data/Cards/DA_Card_*.uasset (13 张)        │
│  /Game/Ch1/Data/Dolls/DA_Doll_*.uasset (3 只)         │
└──────────────────────────────────────────────────────┘
```

---

## 1. 「我想调 X 去哪里改」速查表

### A. 玩法配置 → `BP_Chapter1GameMode` 的 Class Defaults

打开 `/Game/Ch1/Blueprints/BP_Chapter1GameMode`，看右侧 Details 面板：

| 字段 | Category | 默认 | 含义 |
|---|---|---|---|
| `Shifts` | Ch1\|Shifts | 4 条 | **多班次配置**——每条是一个 FShiftConfig（见下） |
| `TransitionHoldSeconds` | Ch1\|Shifts | 2.5 | 班次间「班次 X」过场停留秒数 |
| `CardSelectionWidgetClass` | Ch1\|Classes | WBP_CardSelectionScreen | 用哪个选卡屏 |
| `InspectionWidgetClass` | Ch1\|Classes | WBP_InspectionScreen | 用哪个检验屏 |
| `ShiftTransitionWidgetClass` | Ch1\|Classes | WBP_ShiftTransition | 用哪个过场（黑屏 + 「班次 X」） |

**`Shifts` 每条（FShiftConfig）字段：**

| 字段 | 含义 |
|---|---|
| `PoolCards` | 这一班的卡池（拖 DA_Card_* 进去） |
| `DollSequence` | 这一班的娃娃池（循环抽取，不是「过完即结束」） |
| `K` | 玩家要选几张卡 |
| `CorrectGoal` | **核心节奏**——本班需正确判定多少次才下班（错的不计数继续抽） |
| `AssemblyTimerSec` | 选卡屏倒计时（默认 30） |
| `DollTimeoutSec` | 每只娃娃超时（≤0 = 无超时；Shift 3+ 推荐 6-8 秒） |

**默认 4 班升级曲线**（按 v0.4 平衡调整）：

| Shift | N (Pool) | K | CorrectGoal | Timeout | 体感 |
|---|---|---|---|---|---|
| 1 | 3 | 3 | 3 | 0 | 教学，几乎不会错 |
| 2 | 5 | 3 | 4 | 0 | 选择空间，开始撞错 |
| 3 | 7 | 4 | 5 | 8s | 时间压力，命中率下降 |
| 4 | 13 | 5 | 6 | 6s | 标准严苛，大量丢弃才能凑齐 |

> 「`CorrectGoal` 而非过完池」是 v0.4 关键改动：后期 K 高 → 卡组要求严苛 → 大部分娃娃需丢弃 → 玩家被迫做大量「丢弃」动作，符合「越后期玩家丢的越多」的预期手感。

### B. 扇形布局 → `WBP_CardSelectionScreen` 的 Class Defaults

打开 `/Game/Ch1/UI/WBP_CardSelectionScreen`，左上角 `Class Defaults`，Details 面板：

| 字段 | Category | 默认 | 调整方向 |
|---|---|---|---|
| `FanSpreadDegrees` | CardSelection\|Fan | 36 | 扇更宽 → 提高（如 60）；更窄/平 → 降（如 15） |
| `CardSpacingPx` | CardSelection\|Fan | 65 | 想更挤重叠 → 降（如 40）；完全不重叠 → ≥ CardWidth |
| `CardWidth` | CardSelection\|Fan | 120 | 单卡渲染宽度 |
| `CardHeight` | CardSelection\|Fan | 180 | 单卡渲染高度 |
| `CardWidgetClass` | CardSelection\|Classes | WBP_JudgmentCard | 一般不动 |

> 改 `CardWidth/Height` 后**也要**进 `WBP_JudgmentCard` 把根 `SizeBox`（叫 `CardSize`）的 Width/Height Override 改成一致 —— 否则 C++ 给的 slot 尺寸跟 widget 自报的 desired size 不一致，hit-test 区域会偏。

### B-2. 检验屏反馈 juice → `WBP_InspectionScreen` 的 Class Defaults

打开 `/Game/Ch1/UI/WBP_InspectionScreen`，`Class Defaults`：

| 字段 | Category | 默认 | 用途 |
|---|---|---|---|
| `CorrectFlashColor` | Inspection\|Feedback | 绿 (0.2, 0.9, 0.4) | 判对时全屏闪屏颜色 |
| `WrongFlashColor` | Inspection\|Feedback | 红 (0.95, 0.25, 0.25) | 判错时全屏闪屏颜色 |
| `FlashPeakAlpha` | Inspection\|Feedback | 0.45 | 闪屏峰值不透明度（0-1） |
| `FlashDurationSec` | Inspection\|Feedback | 0.35 | 闪屏总时长 |
| `WrongShakeAmplitude` | Inspection\|Feedback | 14 | 错误震屏振幅（像素）；正确震 1/3 |
| `ShakeDurationSec` | Inspection\|Feedback | 0.30 | 震屏时长 |
| `ToastHoldSeconds` | Inspection\|Tuning | 1.0 | toast 文字停留 |

### C. 单卡动效 → `WBP_JudgmentCard` 的 Class Defaults

打开 `/Game/Ch1/UI/WBP_JudgmentCard`，`Class Defaults` 面板：

| 字段 | Category | 默认 | 用途 |
|---|---|---|---|
| `MaxTiltAngleDegrees` | Card\|Tilt | 15 | 鼠标在卡角落时倾斜幅度（Balatro 招牌的伪 3D 晃） |
| `MaxShearAmount` | Card\|Tilt | 0.10 | 透视错觉强度（剪切量） |
| `HoverLiftPixels` | Card\|Hover | 12 | 悬停时向上浮 |
| `HoverScaleMultiplier` | Card\|Hover | 1.05 | 悬停时放大倍数 |
| `SelectedLiftPixels` | Card\|Selection | 32 | 选中后永久浮起 |
| `SmoothingSpeed` | Card\|Smoothing | 14 | 动画插值速度（越大越跟手） |

### D. 卡面贴图（13 张 PNG）→ 每张 `DA_Card_*`

流程：

1. **导入 PNG**：把图片拖到 `/Game/Ch1/Art/Cards/`（自动生成 `T_Card_BalletPose` 这种 Texture2D，命名随你）
2. **绑定到 DataAsset**：双击 `DA_Card_BalletPose`，在 `IconTexture` 字段下拉框选刚才导入的 Texture
3. **PIE 启动** —— `UJudgmentCardWidget::SetCardData` 会在运行时检测：
   - `IconTexture` 有值 → 用图（color tint 自动重置为白）
   - `IconTexture` 为空 → 回退**维度色块**占位（暗红=姿态 / 深棕=发型 / 暗金=表情 / 冷青=饰物，Pearl-compat 卡再亮 25%）

> 不需要改任何 WBP，不需要写蓝图 —— 卡面是「数据驱动」的。

### E. 卡的判定规则 / 元数据 → 每张 `DA_Card_*`

| 字段 | 含义 |
|---|---|
| `CardId` | 唯一 FName，如 "BalletPose"。**必须跟 `UDollData` 里对应 trait FName 一致**才会判定为命中 |
| `Dimension` | Posture / Hair / Expression / Accessory 之一 |
| `DisplayLabel` | 卡面字面要求（中文 FText） |
| `bIsPearlCompatible` | 是否为「构成玩家自己画像」的卡（视觉略提亮 + 后续 Substrate 翻转用得上） |
| `PostureValue` | **仅 Dimension=Posture 用**：FName，跟 `UDollData.Posture` 精确等值匹配 |
| `IconTexture` | 见 D |

### F. 娃娃属性 → 每张 `DA_Doll_*`

| 字段 | 含义 |
|---|---|
| `DollId` | 唯一 FName |
| `DisplayName` | 灰盒显示名 |
| `Posture` | FName，与 `UCardData.PostureValue` 比对 |
| `HairTraits / ExpressionTraits / AccessoryTraits` | `TSet<FName>`，每个 trait 与某张 Card 的 `CardId` 对应 |

判定逻辑（`UJudgmentEvaluator::EvaluateDoll`，`Source/Checkmate/JudgmentEvaluator.cpp`）：
- 玩家所选每张卡逐一对娃娃检查
- **任意一张不命中** → Reject
- **全部命中** → Pass
- 命中规则：
  - Posture 卡：`Doll.Posture == Card.PostureValue`（FName 等值）
  - 其他维度：`Doll.<X>Traits.Contains(Card.CardId)`

---

## 2. C++ 源码索引（出 bug 时直接跳）

| 关心的东西 | 文件 | 关键函数 / 行 |
|---|---|---|
| 班次入口 / 串接 | `Source/Checkmate/Chapter1GameMode.cpp` | `BeginPlay`, `HandleCardsAssembled`, `HandleShiftCompleted` |
| 选卡屏构造（spawn 卡 + 扇形布局） | `Source/Checkmate/CardSelectionScreen.cpp` | `NativeConstruct` |
| 倒计时 / 强制开始 | 同上 | `TickCountdown` |
| 点选切换 / Confirm | 同上 | `HandleCardClicked`, `OnBeginShiftClicked` |
| 卡 hover/tilt 数学 | `Source/Checkmate/JudgmentCardWidget.cpp` | `NativeOnMouseMove`, `NativeTick`, `ApplyTiltToTransform` |
| 卡 click toggle | 同上 | `NativeOnMouseButtonDown/Up`, `SetSelected` |
| 卡贴图 / 占位色 | 同上 | `SetCardData` |
| 检验 + toast | `Source/Checkmate/InspectionScreen.cpp` | `HandlePlayerChoice`, `AdvanceToNextDoll` |
| 判定规则 | `Source/Checkmate/JudgmentEvaluator.cpp` | `EvaluateDoll` |

---

## 3. 重编 vs Live Coding

| 改动类型 | 用什么 | 备注 |
|---|---|---|
| 只改函数体（cpp） | UE 编辑器右下角 Live Coding 按钮 / `Ctrl+Alt+F11` | 几秒钟，不用关编辑器 |
| 改 `.h`（加字段 / 加 UFUNCTION / 加 UPROPERTY） | 关编辑器 → VS F5 重编 → 重启 | UHT 要重新生成反射，Live Coding 必失败 |
| 改 UPROPERTY 默认值（C++ 改了默认） | 关编辑器重编 | WBP/BP CDO 不会自动 reset，要手动 `Reset to Default` 或 BP 改 |

UBT 命令行（自动化）：
```
"D:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" CheckmateEditor Win64 Development -Project="<full path>\Checkmate.uproject" -waitmutex
```

---

## 4. Asset 路径速查

```
/Game/Ch1/
├── Blueprints/
│   └── BP_Chapter1GameMode        # 整循环入口；World Settings → GameMode Override 设这个
├── Data/
│   ├── Cards/                     # 13 张 DA_Card_*
│   └── Dolls/                     # 3 只 DA_Doll_*（v0.4：池，循环抽取）
├── UI/
│   ├── WBP_CardSelectionScreen    # 选卡屏（含 13 卡扇形布局）
│   ├── WBP_JudgmentCard           # 单卡 widget（hover/tilt/select）
│   ├── WBP_InspectionScreen       # 检验屏（Pass/Reject + 反馈闪屏/震屏）
│   ├── WBP_ShiftTransition        # 班次间过场（黑屏 + 「班次 X」字幕）
│   └── WBP_CardSlot               # ★ 已弃用（v0.3 起改点选模式，此 WBP 不再被引用）
└── Maps/
    └── Ch1Test                    # 测试关；World Settings 指 BP_Chapter1GameMode
```

---

## 5. 历史 / 已知遗留

- **`UCardSlotWidget` / `UCardSelectionDragOp` / `WBP_CardSlot`** —— 来自 v0.1 的 drag-drop 卡选方案，v0.3 改点选后整套作废。`CardSlotWidget.cpp::NativeOnDrop` 已 neuter 成 no-op。完全清理时连同三个文件 + asset 一起删，并 regenerate VS project files。
- **`UE_MCP_Bridge` 插件** —— 修了一处 unity-build 函数重定义（`PIEInputReplayer.cpp` 里 `ISOTimestampNow` 重命名为 `ISOTimestampNowReplayer`，避免与同名 `PIEInputRecorder.cpp` 在 unity TU 内冲突）。插件升级时可能要重打。
- **未做**：Pearl-compat 卡的 holographic foil 视效 —— C++ 已经留了 `CardMID` + `MaterialParam_HoloIntensity` 接口，等 `M_JudgmentCard_2D` 材质做出来直接接。
