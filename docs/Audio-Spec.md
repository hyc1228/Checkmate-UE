# Audio Spec — Checkmate / Velvet Wire

> 音频接口 = `UAudioService`（GameInstanceSubsystem，`Source/Checkmate/AudioService.*`），
> 路由：**FMOD event 优先 → 原生 SoundBase 兜底**。
> 调用点只用 `FName Key`：`UAudioService::PlayCueStatic(this, "Ch1.Stamp")`。

---

## 1. Cue Keys（C++ 已 hook 的 6+ 处）

| Key | 触发位置 | 占位用 | FMOD event path（建议） |
|---|---|---|---|
| `Ch1.Stamp`   | `DollDisplay::EnterConfirming`            | 重墨印冲击      | `event:/Ch1/Stamp` |
| `Ch1.Toss`    | `DollDisplay::EnterTossing`               | 链条 + 溜槽回声 | `event:/Ch1/Toss` |
| `Ch1.Correct` | `InspectionScreen::StartFeedback(true)`   | 短确认提示音    | `event:/Ch1/Correct` |
| `Ch1.Wrong`   | `InspectionScreen::StartFeedback(false)`  | 短错误提示音    | `event:/Ch1/Wrong` |
| `Amb.Ch1`     | `Chapter1GameMode::BeginPlay`             | 工厂环境噪      | `event:/Amb/Ch1Factory` |
| `Ch2.Move`    | `Ch2GameMode::NotifyMoveCommitted`        | 木地板脚步      | `event:/Ch2/Move` |
| `Ch2.Explode` | `Ch2GameMode::ExplodePuppet`              | 低频冲击 + 木裂 | `event:/Ch2/Explode` |
| `Ch2.Ritual`  | `Ch2GameMode::NotifyPawnEnteredCell` (Pickup) | 钟铃单击    | `event:/Ch2/Ritual` |
| `Ch2.Victory` | `Ch2GameMode::NotifyPawnEnteredCell` (Exit)   | 长确认音    | `event:/Ch2/Victory` |
| `Amb.Ch2`     | `Ch2GameMode::BeginPlay`                  | 黑白棋盘 ambient | `event:/Amb/Ch2Empty` |
| `UI.Click`    | （未 hook，留备）                          | 按钮 click      | `event:/UI/Click` |
| `UI.Hover`    | （未 hook，留备）                          | 按钮 hover 微音 | `event:/UI/Hover` |

---

## 2. 原生占位资产（已下载）

Kenney CC0 公共领域包（已放到 `Content/Audio/Placeholder/*.ogg`）：

| .ogg 文件          | 来源                              | 对应 Key       |
|---|---|---|
| `Ch1_Stamp.ogg`    | impact/impactBell_heavy_004      | Ch1.Stamp     |
| `Ch1_Toss.ogg`     | interface/drop_002               | Ch1.Toss      |
| `Ch1_Correct.ogg`  | interface/confirmation_001       | Ch1.Correct   |
| `Ch1_Wrong.ogg`    | interface/error_002              | Ch1.Wrong     |
| `Ch2_Move.ogg`     | impact/footstep_wood_001         | Ch2.Move      |
| `Ch2_Explode.ogg`  | impact/impactWood_heavy_004      | Ch2.Explode   |
| `Ch2_Ritual.ogg`   | interface/bong_001               | Ch2.Ritual    |
| `Ch2_Bell.ogg`     | temp duplicate of Ch2_Ritual     | Ch2.Bell      |
| `Ch2_Victory.ogg`  | interface/confirmation_004       | Ch2.Victory   |
| `UI_Click.ogg`     | interface/click_001              | UI.Click      |
| `UI_Hover.ogg`     | interface/click_005              | UI.Hover      |

来源：https://kenney.nl/assets/interface-sounds + https://kenney.nl/assets/impact-sounds（CC0）

2026-05-30 新增 Kenney Digital Audio（CC0）候选包：

| .ogg 文件 | 来源文件 | 建议 Key |
|---|---|---|
| `Twist_PearlEye.ogg` | digital-audio/spaceTrash1 | Twist.PearlEye |
| `Twist_MechanicalEye.ogg` | digital-audio/phaserUp4 | Twist.MechanicalEye |
| `Twist_DroneAscent.ogg` | digital-audio/lowThreeTone | Twist.DroneAscent |
| `Ch1_ShiftPass.ogg` | digital-audio/twoTone1 | Ch1.ShiftPass |
| `Ch1_ShiftFail.ogg` | digital-audio/lowDown | Ch1.ShiftFail |
| `Ch1_TimeoutWarn.ogg` | digital-audio/highUp | Ch1.TimeoutWarn |
| `Ch2_PuppetTick.ogg` | digital-audio/tone1 | Ch2.PuppetTick |

完整源包保留在 `Content/Audio/SourcePacks/Kenney_DigitalAudio/`，包含 `License.txt`。来源：https://kenney.nl/assets/digital-audio（CC0）。

环境 ambient（Amb.Ch1 / Amb.Ch2）暂未占位 — 推荐去 freesound.org 找 1-2min 工厂底噪 + 空旷大厅底噪，命名后丢同目录。

---

## 3. 一次性把 10 个 .ogg 导成 UE SoundWave 资产

UE 编辑器里 Output Log 顶端把 **Cmd** dropdown 从 "Cmd" 切到 **Python**，粘贴：

```
py "D:/Unreal Projects/Checkmate/Content/Python/import_audio.py"
```

回车。Log 会打印 `[ok] imported Ch1_Stamp` 等 10 行。

或菜单：**Tools → Execute Python Script…** 选 `Content/Python/import_audio.py`。

导完后 `/Game/Audio/Placeholder/` 下出现 10 个 SoundWave。

---

## 4. 绑 DA_AudioCues（让 AudioService 找得到）

打开 `/Game/Audio/DA_AudioCues`，在 **NativeCues** TMap 里：

| Key (FName)   | Value (SoundWave)                                |
|---|---|
| Ch1.Stamp     | `/Game/Audio/Placeholder/Ch1_Stamp`             |
| Ch1.Toss      | `/Game/Audio/Placeholder/Ch1_Toss`              |
| Ch1.Correct   | `/Game/Audio/Placeholder/Ch1_Correct`           |
| Ch1.Wrong     | `/Game/Audio/Placeholder/Ch1_Wrong`             |
| Ch2.Move      | `/Game/Audio/Placeholder/Ch2_Move`              |
| Ch2.Explode   | `/Game/Audio/Placeholder/Ch2_Explode`           |
| Ch2.Ritual    | `/Game/Audio/Placeholder/Ch2_Ritual`            |
| Ch2.Bell      | `/Game/Audio/Placeholder/Ch2_Bell`              |
| Ch2.Victory   | `/Game/Audio/Placeholder/Ch2_Victory`           |
| UI.Click      | `/Game/Audio/Placeholder/UI_Click`              |
| UI.Hover      | `/Game/Audio/Placeholder/UI_Hover`              |
| Twist.PearlEye | `/Game/Audio/Placeholder/Twist_PearlEye`       |
| Twist.MechanicalEye | `/Game/Audio/Placeholder/Twist_MechanicalEye` |
| Twist.DroneAscent | `/Game/Audio/Placeholder/Twist_DroneAscent` |
| Ch1.ShiftPass | `/Game/Audio/Placeholder/Ch1_ShiftPass`        |
| Ch1.ShiftFail | `/Game/Audio/Placeholder/Ch1_ShiftFail`        |
| Ch1.TimeoutWarn | `/Game/Audio/Placeholder/Ch1_TimeoutWarn`    |
| Ch2.PuppetTick | `/Game/Audio/Placeholder/Ch2_PuppetTick`      |

保存。Subsystem 会在 GameInstance 启动时自动加载 `DA_AudioCues` —— 进游戏直接有声。

---

## 5. FMOD Studio 端事件清单（毕业前可慢慢做）

### 5.1 工程结构

新建 FMOD Studio 工程 `Checkmate.fspro`，建以下 Banks + Events：

```
Master Bank
  bus:/UI
  bus:/SFX
  bus:/Amb

Bank: Ch1
  event:/Ch1/Stamp        2D, oneshot, mixer route SFX
  event:/Ch1/Toss         2D, oneshot, SFX
  event:/Ch1/Correct      2D, oneshot, SFX
  event:/Ch1/Wrong        2D, oneshot, SFX
  event:/Ch1/ShiftPass    2D, oneshot, SFX
  event:/Ch1/ShiftFail    2D, oneshot, SFX
  event:/Ch1/TimeoutWarn  2D, oneshot, SFX
  event:/Amb/Ch1Factory   2D, looping, Amb（参数 Tension 0-1 可选）

Bank: Ch2
  event:/Ch2/Move         2D, oneshot, SFX（参数 Mode 0/1 = Ballet/Clown 用不同 sample）
  event:/Ch2/Explode      2D, oneshot, SFX（低频冲击）
  event:/Ch2/Ritual       2D, oneshot, SFX
  event:/Ch2/Victory      2D, oneshot, SFX
  event:/Ch2/PuppetTick   2D, oneshot, SFX
  event:/Amb/Ch2Empty     2D, looping, Amb

Bank: Twist
  event:/Twist/PearlEye       2D, oneshot, SFX
  event:/Twist/MechanicalEye  2D, oneshot, SFX
  event:/Twist/DroneAscent    2D, oneshot or loop, SFX

Bank: UI
  event:/UI/Click         2D, oneshot, UI
  event:/UI/Hover         2D, oneshot, UI
```

### 5.2 导入到 UE

1. FMOD Studio → File → **Build...** → 输出到 `Plugins/FMODStudio/Content/Banks/`
   （或 Edit → Preferences → Build → 指向项目 Banks 目录）
2. UE 编辑器 → Window → FMOD → **FMOD Audio Settings** → Studio Project Path 指向 `.fspro`
3. UE 自动 reimport banks → 在 Content Browser 看到 FMODEvent 资产

### 5.3 绑到 DA_AudioCues

打开 `DA_AudioCues`，**FmodEventPaths** TMap 填：

| Key         | Value                  |
|---|---|
| Ch1.Stamp   | `event:/Ch1/Stamp`     |
| Ch1.Toss    | `event:/Ch1/Toss`      |
| Ch1.Correct | `event:/Ch1/Correct`   |
| Ch1.Wrong   | `event:/Ch1/Wrong`     |
| Amb.Ch1     | `event:/Amb/Ch1Factory` |
| Ch2.Move    | `event:/Ch2/Move`      |
| Ch2.Explode | `event:/Ch2/Explode`   |
| Ch2.Ritual  | `event:/Ch2/Ritual`    |
| Ch2.Victory | `event:/Ch2/Victory`   |
| Amb.Ch2     | `event:/Amb/Ch2Empty`  |
| UI.Click    | `event:/UI/Click`      |
| UI.Hover    | `event:/UI/Hover`      |
| Twist.PearlEye | `event:/Twist/PearlEye` |
| Twist.MechanicalEye | `event:/Twist/MechanicalEye` |
| Twist.DroneAscent | `event:/Twist/DroneAscent` |
| Ch1.ShiftPass | `event:/Ch1/ShiftPass` |
| Ch1.ShiftFail | `event:/Ch1/ShiftFail` |
| Ch1.TimeoutWarn | `event:/Ch1/TimeoutWarn` |
| Ch2.Bell | `event:/Ch2/Bell` |
| Ch2.PuppetTick | `event:/Ch2/PuppetTick` |

`Build.cs` 已经 `PublicDefinitions.Add("WITH_FMOD_CHECKMATE=1")`，AudioService 会优先走 FMOD path；任何 key 在 `FmodEventPaths` 找不到就退回 NativeCues。

---

## 6. 新机器 setup（重要：FMOD plugin 不在 git）

`.gitignore` 屏蔽了 `Plugins/FMODStudio/`（376 MB，超 LFS 配额）。新机器 clone 后：

1. 去 https://www.fmod.com/download 下载 **FMOD for Unreal**（要 2.03.x for UE 5.7）
2. 解压拖到 `D:/Unreal Projects/Checkmate/Plugins/` —— 注意要让 `FMODStudio/`、`FMODStudioNiagara/`、`FMODStudioOpenXR/` 三个文件夹**直接放在 Plugins/ 下**，不要带版本号外壳
3. 删 `Binaries/` `Intermediate/`，重建 VS 工程文件 + 全量编译

---

## 7. 接口形状（不会再改）

```cpp
// 单次播 cue
UAudioService::PlayCueStatic(this, FName("Ch1.Stamp"));
UAudioService::PlayCueStatic(this, FName("Ch1.Wrong"), 0.7f);  // 音量倍率

// 在 game mode 里 ambient
if (auto* GI = GetGameInstance())
    if (auto* Svc = GI->GetSubsystem<UAudioService>())
        Svc->PlayAmbient(FName("Amb.Ch1"), 2.0f);  // 2s fade-in

// 在世界位置
Svc->PlayCueAtLocation(FName("Ch2.Explode"), ExplodeLoc);
```

## PV Slice Note

`Ch2.Bell` is bound to `/Game/Audio/Placeholder/Ch2_Bell` for the WeddingWreckage 4-neighbor trigger. This is a temporary duplicate of `Ch2_Ritual`; final asset should be `SC_Ch2.Bell`, <=600ms, hard-cut with no reverb tail.

所有调用点都不需要知道是 FMOD 还是原生，Subsystem 内部分流。
