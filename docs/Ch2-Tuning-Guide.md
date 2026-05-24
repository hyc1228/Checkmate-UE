# Ch2 棋盘逃脱 — 代码使用 / 调参文档（v0.1 原型）

> 范围：第二章 vertical slice — 俯视棋盘 + 两种走法 + 切换 ritual + 爆炸玩偶 + 出口。
> 当前阶段：Tier A（核心可玩）+ Tier B（爆炸玩偶 / 可破坏）+ Tier C 部分（PP 占位）已落地。
> 视觉资产先用 engine basic shapes 占位，等真模型来再换。

## 0. 数据流

```
DA_Ch2Level_01 (UCh2LevelData) ──── 编辑器编辑 ─── EUW_Ch2LevelEditor
       │                                              │
       │ GridWidth / GridHeight / CellSize             │ 8×8 按钮网格
       │ TMap<FIntPoint, ECh2CellType> Cells           │ Brush 工具栏
       │                                              │ Save → 写回 .uasset
       │
       │ BP_Ch2GameMode 引用
       ↓
ACh2GameMode (Ch2Test.umap 的 GameMode Override)
       │
       ├─ BuildLevel (BeginPlay)：
       │    spawn 8×8 floor + walls + pickup + exit + destructible
       │    spawn BP_Ch2Pawn 到 Start cell
       │    SetUpTopDownCamera (斜俯视 isometric)
       │
       ├─ NotifyPawnMoved(from, to, bClown)
       │    bClown == true → 在 from 留爆炸玩偶
       │    TickPuppets 推进所有玩偶 TurnsRemaining-1
       │    倒计时到 0 → ExplodePuppet 清相邻 Destructible
       │
       └─ NotifyPawnEnteredCell(to)
            ClownPickup → SetMode(Clown) + 移除拾取
            Exit       → log "Ch2 Complete"

ACh2Pawn
       │ CurrentMode = Ballet (4-dir 滑到撞墙) | Clown (knight L-jump)
       │ Tick：LMB 点 cell → DeprojectMousePosition → WorldToCell → TryMoveTo
       │ GetValidMoves() 按 mode 计算合法目标
```

---

## 1. 「我想调 X 去哪里改」速查表

### A. 关卡布局 → 用关卡编辑器 EUW

**用法：**
1. Content Browser 在 `/Game/Ch2/Editor/WBP_Ch2LevelEditor` 上**右键 → Run Editor Utility Widget**
2. 在 EUW 顶部 **TargetAsset** 属性槽（在窗口的 Details 面板，可能要先打开 Window > Editor Utility Widgets > Properties）拖一个 `UCh2LevelData`（如 `DA_Ch2Level_01`）进去
3. 工具栏点 brush 类型（Empty / Wall / Destructible / Pickup / Exit / Start）
4. 在 8×8 网格按钮上点选 → cell 按当前 brush 着色 + 文字标记
5. 改完点 **💾 Save** → 写回 DA 并保存到磁盘
6. PIE 跑 Ch2Test → 关卡按新数据 spawn

**Brush 含义：**

| Brush | 符号 | 颜色 | 含义 |
|---|---|---|---|
| Empty | · | 深灰 | 可通行（默认） |
| Wall | █ | 蓝灰 | 不可通行（永久） |
| Destructible | ▓ | 棕黄 | 不可通行，被爆炸玩偶炸开后变 Empty |
| ClownPickup | ◆ | 金 | 走上去 → 切换 Clown 模式 |
| Exit | → | 绿 | 走上去 = 胜利 |
| Start | ✦ | 青 | 玩家初始位置（每关只能有一个） |

### B. 棋盘尺寸 / Cell 大小 → 改 `UCh2LevelData` asset

| 字段 | 默认 | 说明 |
|---|---|---|
| `GridWidth` | 8 | 棋盘 X 方向 cell 数 |
| `GridHeight` | 8 | 棋盘 Y 方向 cell 数 |
| `CellSize` | 200 | 单 cell 边长（unit） |
| `Cells` | TMap | EUW 编辑后写回这里 |

> 改 GridSize 后**重开 EUW** 让 8×8 → N×M 重画。

### C. 走法参数 → `BP_Ch2Pawn` Class Defaults

```
class ACh2Pawn:
  - 没有走法参数；规则硬编码在 ComputeBalletMoves / ComputeClownMoves
    Ballet: 4 方向滑到撞墙
    Clown:  8 个 knight L-offset
  - 想加新走法：改 ComputeXxxMoves，或加新的 ECh2Mode 枚举值
```

### D. GameMode 调参 → `BP_Ch2GameMode` Class Defaults

| 字段 | 默认 | 说明 |
|---|---|---|
| `LevelData` | DA_Ch2Level_01 | 用哪份关卡 |
| `PawnClass` | BP_Ch2Pawn | 玩家 actor |
| `FloorMesh` | /Engine/BasicShapes/Plane | 地板占位 |
| `WallMesh` | /Engine/BasicShapes/Cube | 墙 / 可破坏占位 |
| `PickupMesh` | /Engine/BasicShapes/Cone | 拾取占位 |
| `ExitMesh` | /Engine/BasicShapes/Plane | 出口占位（绿） |
| `PuppetMesh` | /Engine/BasicShapes/Sphere | 爆炸玩偶占位 |
| `CameraDistance` | 2000 | 相机距棋盘中心（unit） |
| `CameraTiltDegrees` | 50 | 俯角（0=水平 / 90=正俯视） |
| `CameraYawDegrees` | 0 | 相机绕棋盘 yaw |
| `PuppetExplodeAfterTurns` | 2 | 爆炸玩偶倒计时回合数 |

**替换美术：** 把上面 mesh 字段拖你的真模型 asset 进去即可，其余逻辑不动。

### E. 视觉滤镜（黑白基调）→ `PP_BlackWhite` PostProcessVolume

关卡里有个 `PP_BlackWhite`（无界后处理）。在它的 Details 面板：
- `Settings → Color Grading → Global → Saturation` 改 `(0, 0, 0, 1)` = 全屏黑白
- `Settings → Color Grading → Global → Contrast` 微调 `(1.1, 1.1, 1.1, 1)` 提对比

slice spec 还有「出口处一抹彩色 + 控制响应轻微软化」—— 后续可在 Exit cell 附近加一个 unbound priority=1 的 PP 覆盖局部彩色。

---

## 2. C++ 源码索引

| 关心的 | 文件 | 关键 |
|---|---|---|
| 关卡数据 schema | `Source/Checkmate/Ch2LevelData.h` | `ECh2CellType` / `UCh2LevelData` |
| GameMode 整循环 | `Source/Checkmate/Ch2GameMode.h/cpp` | `BeginPlay` / `BuildLevel` / `SetUpTopDownCamera` / `NotifyPawnMoved` |
| 玩家走法 | `Source/Checkmate/Ch2Pawn.h/cpp` | `ComputeBalletMoves` / `ComputeClownMoves` / `Tick` 鼠标 click |
| 爆炸玩偶 | 同 GameMode | `SpawnPuppetAt` / `TickPuppets` / `ExplodePuppet` |
| 关卡编辑器 EUW | `Source/Checkmate/Ch2LevelEditorWidget.h/cpp` | `RebuildGrid` / `OnCellClicked_Internal` / `SaveToAsset` |

---

## 3. Asset 路径

```
/Game/Ch2/
├── Blueprints/
│   ├── BP_Ch2GameMode      # GameMode override（Ch2Test 用它）
│   └── BP_Ch2Pawn          # 玩家 actor（BP 继承 ACh2Pawn）
├── Data/
│   └── DA_Ch2Level_01      # 关卡数据 v0.1（EUW 编辑这个）
├── Editor/
│   └── WBP_Ch2LevelEditor  # EUW 关卡编辑器（右键 Run Editor Utility Widget）
└── Maps/
    └── Ch2Test             # 测试关；World Settings 指 BP_Ch2GameMode
```

---

## 4. 玩法测试流程

1. 打开 `Ch2Test.umap`
2. PIE → 玩家小立柱（白）出现在 Start cell (0,0)
3. **鼠标点高亮区域**（蓝色 hint 暂未做，你点合法 cell 它就过去）
   - Ballet 模式：4 方向滑到撞墙的尽头
   - 撞了一阵 → 走到 ClownPickup (默认 (2,7)) → 切换到小丑模式（金色）
4. **小丑 knight L-跳** → 跳完上一个 cell 留个橘色玩偶
5. 玩偶 2 回合后爆炸 → 邻 4 cell 的 Destructible 消失
6. 走到 Exit (默认 (7,7)) → log "Ch2 Complete"

> 当前还没做：合法 cell hover 高亮、切换 ritual fade、出口色彩 pulse。

---

## 5. 已知 TODO / 后续

- **合法目标高亮** —— 当前玩家不知道哪些 cell 能点；C++ 加 Tick 检测 hover cell + 加个 UMG hint
- **切换 ritual 演出** —— ClownPickup 当前是瞬间换 mode + tint。slice spec 要「取扣/缝扣」动画
- **出口色彩脉冲** —— Exit cell 颜色 pulse 暗示「这里有色彩」（呼应 sensorium 扩张）
- **婚纱残骸** —— 第三只娃娃，slice 仅作场景对象（暂未做）
- **3D 棋盘渲染问题** —— 当前 PIE 截图疑似看不到 grid，需要在编辑器内实际 PIE 验证。Floor 用 Plane 在斜俯视下可能太薄，可换成 Z=0.05 的 cube。

---

## 6. 加新关卡

1. Content Browser 右键 → Create Data Asset → `Ch2LevelData` → 命名（如 `DA_Ch2Level_02`）
2. 用 EUW Load 这个 asset → 编辑 → Save
3. 复制 `Ch2Test.umap` 重命名 → World Settings 把 GameMode Override 的 `LevelData` UPROPERTY override 改成新 asset
4. PIE 跑新关

---

## 7. 加新走法（如要做第三只娃娃 / 婚纱）

1. `Ch2Pawn.h` 的 `ECh2Mode` 加新枚举（如 `Bride`）
2. 加 `ComputeBrideMoves` 私有函数
3. `GetValidMoves()` switch 加分支
4. `SetMode` 加颜色映射
5. 触发：加新 `ECh2CellType` 拾取（如 `BridePickup`），或者复用 ClownPickup + 多阶段切换状态机
