# Audio Cue Table — Checkmate / Velvet Wire

> 当前已用的所有音频 cue + 还没做的项。方便后续替素材 / 配 FMOD events 时查表。
>
> **调用接口（不会再改）**：
> ```cpp
> UAudioService::PlayCueStatic(this, FName("Ch1.Stamp"));
> UAudioService::PlayCueStatic(this, FName("UI.Hover"), 0.5f);  // 第二参数音量倍率
> ```
>
> **路由**：FMOD event 优先 → 原生 SoundWave 兜底（见 `Source/Checkmate/AudioService.cpp`）
>
> **试听板**：`docs/audio-audition/index.html` 可以直接打开，或在项目根目录运行 `python3 -m http.server 8765` 后访问 `http://127.0.0.1:8765/docs/audio-audition/`。用于批量预听、上传候选音频、标记 Approved / Replace / Reject，并导出 review JSON。

---

## 状态图例

| 标记 | 含义 |
|---|---|
| ✅ | 已接入 + 占位音绑了 DA + PIE 听得到 |
| ⚠ | 已接入 + 听得到，**但占位音不符合 design 目标**，需要换更好素材 |
| 🟡 | 已接入但 DA 还没绑（跑 `bind_audio_cues.py` 应该解决） |
| 🚫 | code 里没有调用点 — 还没实现 / 还没决定怎么触发 |
| 💤 | FMOD event 未建（全部都还没建，PV 后再统一上） |

---

## 1. 当前已接入的 Cue（17 条 — code 已 hook）

| Key | 触发位置 | 占位 .ogg | 设计目标 | 占位状态 | FMOD |
|---|---|---|---|---|---|
| **Ch1.Stamp** | `DollDisplay::EnterConfirming` 印章砸下 | impactMetal_002 | 重金属盒盖"啪" + 0.2s 齿轮咬合 tail | ⚠ 缺齿轮 tail | 💤 |
| **Ch1.Toss** | `DollDisplay::EnterTossing` 娃娃被扔 | thrusterFire_001 | "咻——"+ 0.3s 落地砸开水面"咚" | ⚠ 缺落地+水面 tail | 💤 |
| **Ch1.Correct** | `InspectionScreen::StartFeedback(true)` | engineCircular_000 | 齿轮咬上、机器接收信号的短确认 | ✅ | 💤 |
| **Ch1.Wrong** | `InspectionScreen::StartFeedback(false)` | lowFrequency_explosion_000 | 低频闷响 + 哼鸣，工厂"出错了"感 | ⚠ 太爆炸感，要更"系统轻微抽搐" | 💤 |
| **Ch2.Move** | `Ch2GameMode::NotifyMoveCommitted` 棋子落子 | footstep_wood_001 | 木地板脚步 / 棋子落子；Ballet vs Clown 应不同 | ⚠ 只单一版本 | 💤 |
| **Ch2.Explode** | `Ch2GameMode::ExplodePuppet` 爆炸玩偶 BOOM | lowFrequency_explosion_001 | 闷沉爆炸 + 木裂，3D 衰减 | ✅ | 💤 |
| **Ch2.Ritual** | `NotifyPawnEnteredCell(Pickup)` + `Chapter1.RequestTwist` 翻转 | doorOpen_000 | 取扣 / 缝扣 — 金属小门 + 钟铃单击 | ⚠ 太短，spec 期待 1.4s 完整仪式 | 💤 |
| **Ch2.Victory** | `NotifyPawnEnteredCell(Exit)` | confirmation_004 | 长确认 + 钟铃 + 一丝色彩涌入 | ✅ | 💤 |
| **UI.Click** | `MainMenuWidget` / `JudgmentCardWidget` / `CardSelectionScreen::OnBeginShiftClicked` | click_001 | UI 按钮 / 卡片点击的轻确认 | ✅ | 💤 |
| **UI.Hover** | `MainMenuWidget` hover（卡 hover 已 disable 防密响） | click_005 | 极轻 tick | ✅ | 💤 |
| **Amb.Ch1** | `Chapter1GameMode::BeginPlay` | spaceEngineLow_000 | 工厂底噪 loop（传送带 + 机器嗡鸣 + 偶发金属） | ⚠ 5s 短样本，UE 自动 loop 有接缝 | 💤 |
| **Amb.Ch2** | `Ch2GameMode::BeginPlay` | spaceEngineSmall_000 | 空厂 ambient loop（远风 + 偶发滴水/共鸣） | ⚠ 5s 短样本，需 30s+ seamless | 💤 |
| **Twist.DroneAscent** | `Chapter1GameMode::RequestTwist` | lowThreeTone | CameraReverse 低频上升 / 心理压力 | ⚠ 很短，仅 PV 占位 | 💤 |
| **Twist.PearlEye** | `Chapter1GameMode::RequestTwist` | spaceTrash1 | 按扣脱落 / glitch snap | ⚠ 电子感偏强 | 💤 |
| **Twist.MechanicalEye** | `Chapter1GameMode::PlayTwistOpticalBurnout` | phaserUp4 | 机械眼启动 / bloom pulse | ⚠ 可用作第一版 | 💤 |
| **Ch1.ShiftPass** | `Chapter1GameMode::HandleShiftCompleted(success)` | twoTone1 | 班次完成铃声 | ⚠ 电子占位 | 💤 |
| **Ch1.ShiftFail** | `Chapter1GameMode::HandleShiftCompleted(fail)` | lowDown | 班次失败 / 系统下沉 | ⚠ 电子占位 | 💤 |

---

## 2. 已借用但应独立的 cue（code 里复用了别的 key）

| 场景 | 当前用 | 应该是 | 优先级 |
|---|---|---|---|
| Ch2 关卡步数超预算 `RestartCh2Level` | 借用 `Ch1.Wrong` | **`Ch2.Fail`** 独立，更"棋盘 reset"风格 | 低 |

2026-05-30 已改：`RequestTwist` 现在调用 `Twist.DroneAscent` + `Twist.PearlEye`，burnout 时调用 `Twist.MechanicalEye`；Ch1 班次成功/失败也已分别调用 `Ch1.ShiftPass` / `Ch1.ShiftFail`。

---

## 3. 还没实现 / 还没决定的 cue（🚫）

| 提议 Key | 想用在哪 | 描述 | 优先级 |
|---|---|---|---|
| **`Music.MainMenu`** | 主菜单 BGM loop | 长 ambient 主题音乐，定调全游戏氛围 | 高（用户打开就听） |
| **`Music.Ch1Loop`** | Ch1 班次中循环 | 低频脉动节奏 + 偶发齿轮 motif | 中 |
| **`Music.Ch2Loop`** | Ch2 棋盘逃脱中循环 | 更稀疏、更冷的 piano + drone | 中 |
| **`Ch1.Card.Select`** | 玩家点选一张判据卡 | 卡片浮起 + 浮光声效；vs UI.Click 更专属 | 低 |
| **`Ch1.Card.Deselect`** | 玩家反选 | 卡片回落，短反气泡声 | 低 |
| **`Ch1.ConveyorIn`** | 娃娃从左侧传送带滑入 | 传送带"咔啦"启动 + 玩偶橡胶滑落 | 高（每只娃娃出场都该有） |
| **`Ch1.TimeoutWarn`** | Shift 3/4 娃娃倒计时 <3s | 滴答加快、紧迫的提示 | 中（已有 Kenney Digital 占位） |
| **`Ch2.Bell`** | WeddingWreckage 4-neighbor trigger | SC_Ch2.Bell: <=600ms hard-cut short bell, no reverb tail | High |
| **`Ch2.PuppetTick`** | 爆炸玩偶倒计时每回合脉冲 | 心跳 / 滴答；玩家能听出"还有 N 回合" | 中（已有 Kenney Digital 占位） |
| **`Ch2.ModeBallet`** | Ch2.Move 的 Ballet 变种 | 轻盈 + 滑步声 | 中 |
| **`Ch2.ModeClown`** | Ch2.Move 的 Clown 变种 | 笨重 + 铃铛 / 双扣摇晃声 | 中 |
| **`UI.LangToggle`** | 中英切换按钮 | 短 click，可省（用 UI.Click 也行） | 低 |
| **`Ch1.DollSlide`** | DollDisplay 玩家拖动时的低响 | 拖娃娃过桌面的橡胶摩擦 | 低 |

---

## 4. 占位音的来源（CC0）

| 来源 | License | 包名 | 用到的文件 |
|---|---|---|---|
| https://kenney.nl/assets/sci-fi-sounds | CC0 | Sci-Fi Sounds | impactMetal_002, thrusterFire_001, engineCircular_000, lowFrequency_explosion_000/001, doorOpen_000, spaceEngineLow_000, spaceEngineSmall_000 |
| https://kenney.nl/assets/interface-sounds | CC0 | Interface Sounds | confirmation_004, click_001, click_005 |
| https://kenney.nl/assets/impact-sounds | CC0 | Impact Sounds | footstep_wood_001 |
| https://kenney.nl/assets/digital-audio | CC0 | Digital Audio | spaceTrash1, phaserUp4, lowThreeTone, twoTone1, lowDown, highUp, tone1 |

占位文件改名后放在 `Content/Audio/Placeholder/*.ogg`。**改名规则**：Key 替换点为下划线（`Ch1.Stamp` → `Ch1_Stamp.ogg`）。
完整源包保留在 `Content/Audio/SourcePacks/Kenney_DigitalAudio/`，包含 `License.txt`。

## 4.1 新增 Kenney Digital Audio 候选（2026-05-30）

| Key | Placeholder file | Source file | 用途 |
|---|---|---|---|
| `Twist.PearlEye` | `Twist_PearlEye.ogg` | `spaceTrash1.ogg` | 按扣脱落 / glitch snap 候选 |
| `Twist.MechanicalEye` | `Twist_MechanicalEye.ogg` | `phaserUp4.ogg` | 机械眼启动候选 |
| `Twist.DroneAscent` | `Twist_DroneAscent.ogg` | `lowThreeTone.ogg` | 翻转低频上升占位 |
| `Ch1.ShiftPass` | `Ch1_ShiftPass.ogg` | `twoTone1.ogg` | 班次完成提示 |
| `Ch1.ShiftFail` | `Ch1_ShiftFail.ogg` | `lowDown.ogg` | 班次失败 / 系统下沉 |
| `Ch1.TimeoutWarn` | `Ch1_TimeoutWarn.ogg` | `highUp.ogg` | 倒计时 warning |
| `Ch2.PuppetTick` | `Ch2_PuppetTick.ogg` | `tone1.ogg` | 爆炸玩偶回合 tick |

---

## 5. FMOD 集成进度

**全部 💤 未建**。FMOD plugin 已装（`Plugins/FMODStudio/`），C++ 路由已就位（`#if WITH_FMOD_CHECKMATE`），但 FMOD Studio 工程还没建任何 event。

PV 提交后建议建 banks：
```
Master/UI       UI.Click / UI.Hover / Music.MainMenu
Master/Ch1      Stamp / Toss / Correct / Wrong / Card.* / Shift.* / Conveyor
Master/Ch2      Move (param: Mode 0/1) / Explode / Ritual / Victory / PuppetTick
Master/Twist    PearlEye / MechanicalEye / 主拍点
Master/Amb      Ch1Factory (param: Tension) / Ch2Empty
Master/Music    MainMenu / Ch1Loop / Ch2Loop
```

## PV Slice Note

`Ch2.Bell` is now a live UE key for the WeddingWreckage 4-neighbor trigger. Current native placeholder is `/Game/Audio/Placeholder/Ch2_Bell`, duplicated from `Ch2_Ritual`; replace it with a <=600ms hard-cut short bell for final `SC_Ch2.Bell`.

具体建 event 的 step 见 `docs/Audio-Spec.md §5`。

---

## 6. 替换素材 / 加新 cue 的工作流

### 6.1 替单条原生占位

1. 把新 .ogg 放到 `Content/Audio/Placeholder/`，**用同一文件名**覆盖（如 `Ch1_Stamp.ogg`）
2. Output Log Cmd → Python 模式 → 粘贴：
   ```python
   exec(open(r"D:/Unreal Projects/Checkmate/Content/Python/import_audio.py").read())
   ```
3. UE 自动 reimport，AudioService 用新声音，无需改代码

### 6.2 加新 cue key（如 `Twist.PearlEye`）

1. 在调用点加 `UAudioService::PlayCueStatic(this, FName("Twist.PearlEye"))`
2. 放 .ogg 到 `Content/Audio/Placeholder/Twist_PearlEye.ogg`
3. 改 `Content/Python/bind_audio_cues.py` 的 `KEY_TO_ASSET` map 加一行
4. 跑两个脚本（import_audio.py + bind_audio_cues.py）
5. **把新 cue 加到本文档第 1 节表里**（记账）

### 6.3 上 FMOD 替占位

1. FMOD Studio 建对应 event（命名见第 5 节）
2. Build banks → 输出到 `Plugins/FMODStudio/Content/Banks/`
3. UE 自动 reimport banks
4. 打开 `/Game/Audio/DA_AudioCues`，**FmodEventPaths** map 里填同名 key + event 路径
5. 重启 PIE — AudioService 优先走 FMOD，没填的 key 继续走原生兜底

---

## 7. 调参（DA_AudioCues 上）

- `NativeCues` : `TMap<FName, USoundBase*>` —— 原生占位（12 条已绑）
- `FmodEventPaths` : `TMap<FName, FString>` —— FMOD event 路径（空）
- `VolumeOverrides` : `TMap<FName, float>` —— per-key 音量倍率（空，要静音某条加 0.0）

主音量在 `UAudioService::MasterVolume`（默认 1.0）。

---

## 8. 文件速查

```
Source/Checkmate/AudioService.h           接口 + 路由
Source/Checkmate/AudioCueTable.h          DataAsset schema
Content/Audio/Placeholder/*.ogg           占位
Content/Audio/DA_AudioCues.uasset         12 条 cue 映射
Content/Python/import_audio.py            一键 import
Content/Python/bind_audio_cues.py         一键 bind map
docs/Audio-Spec.md                        FMOD setup 详解
docs/Audio-Cue-Table.md                   本文档（需求 + 状态）
```
