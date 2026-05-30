# CLAUDE.md — Checkmate UE Project

> 这份文件位于 **UE 工程仓库根**（`D:\Unreal Projects\Checkmate\`），不是设计仓库。
> 设计 + 论文 hub 在 OneDrive 下，路径见下方「Sibling repo」。

## Repository Scope

**This is the UE 5.7 game code repository for Velvet Wire / Checkmate.**

设计文档、策划案、Art Bible、论文初稿等**不在这个仓库里**——它们活在设计 hub：

- **Sibling repo (design + thesis)**:`F:\OneDrive - Aalto University\YvaineLocal\3.论文\`
  - GitHub: `https://github.com/hyc1228/CheckmateGame`（当前 design hub 已推送到此 repo）
  - 核心 source-of-truth 文档:
    - `design/策划案/双章工厂与废厂棋局.md`(v2)
    - `design/策划案/Slice-v0.1-讨论结果.md`(v0.2)
    - `design/gdd/game-concept.md`
    - `design/gdd/eye-state.md`(v1.1 已 design-review 通过)
    - `design/art/art-bible.md`(v1 锁定)
    - `design/gdd/systems-index.md`(PV-first v2)
    - `docs/workflow/UE-AI开发方案.md`(本工程的工程方案出处)

实现某个系统前**先**读对应的 GDD,不要凭印象写。

## Project — Velvet Wire / Checkmate

Solo UE5 thesis-driven indie game。两章结构:工厂检验员(Ch1)→ 棋盘格废厂逃生(Ch2)。视觉签名:按扣眼 ↔ 机械眼。论点-risk 系统:眼睛状态 / 判据卡 / 翻转演出 / 切换 ritual。

PV-first 时间线:5/10–5/23 开发(13 天)→ 5/24–5/30 PV 剪辑 → PPT 提交。

## Technology Stack

- **Engine**:Unreal Engine **5.7**(`D:\Program Files\Epic Games\UE_5.7`)
- **Language**:C++ 主,Blueprint 仅用于 UMG / AnimBP / designer-tunable
- **Build System**:Unreal Build Tool (UBT)
- **IDE**:Visual Studio 2022 Community(Rider 未装,后续可补)
- **Version Control**:Git + Git LFS,私有 GitHub repo

## Naming Conventions (UE C++ standard)

- 类前缀:`A`(Actor)、`U`(UObject)、`F`(struct)、`I`(Interface)、`E`(enum)
- 变量 / 函数:PascalCase
- 布尔:`b` 前缀(`bIsAwakened`)
- 文件名 = 类名去前缀(`ACh1Doll` → `Ch1Doll.h/cpp`)
- 模块名:PascalCase(`Checkmate`, `CheckmateEditor`)

## Workflow Constraints

- **Single dev + AI**:优先 C++(text-diff-able),Blueprint 只在必要时
- **两机协作**:Windows = 编辑器 + 编译 + PIE;Mac = 看代码 / 写笔记,不编译
- **OneDrive 隔离**:本仓库**绝不**放进 OneDrive。`Binaries/` `Intermediate/` `Saved/` `DerivedDataCache/` 全部 ignore
- **Vertical slice 思维**:每个建议过一遍「这是 7-10 分钟 PV slice 需要的吗?」过滤
- **Theory-grounded**:论点在机制里不在台词里,resist cutscene-style "解释"
- **Substrate spike 优先**:第一件实质 UE 工作是按扣眼 ↔ 机械眼材质切换的 Substrate spike(1-2 天),通过后再进其他系统
- **Exhibition build target**:优先做 Steam Deck-compatible PC build。不要优先 Web/HTML5 或手机适配；先打 Windows build,在 Steam Deck 上通过 Steam/Proton 测试。

## AI Coding Guardrails

Apply `karpathy-guidelines` for code work:

- **Think before coding**: state assumptions and surface ambiguity before implementation.
- **Simplicity first**: write the minimum code that solves the requested slice need; avoid speculative abstraction and unused configurability.
- **Surgical changes**: touch only files directly required by the task; match local style; clean up only dead code introduced by the current change.
- **Goal-driven verification**: define the success check before editing, then run available verification (`git diff --check`, build/tests when the platform is available).

## Source-of-Truth 跨仓查找

碰到不确定的设计问题:
1. 先在设计 hub 找对应 GDD(`design/gdd/<system>.md`)
2. 找不到就找策划案 v2(`design/策划案/双章工厂与废厂棋局.md`)
3. 都说不清就回问用户,不要凭印象编

## Collaboration Protocol

- 改 `Source/` 之外的目录(尤其 `Config/` `Content/`)前先问
- 大决策(命名风格切换 / 模块拆分 / 第三方依赖)用 `AskUserQuestion` 给选项
- 改完先 show draft,不要直接 commit
- **无指令不 commit**

## Build & Run (TODO — 工程建好后回填)

- [ ] 生成 VS solution:右键 `.uproject` → Generate Visual Studio project files
- [ ] 编译:VS 里 Development Editor / Win64,F5 跑起编辑器
- [ ] Live Coding:UE 编辑器右下角小按钮,Ctrl+Alt+F11 触发(改头文件结构时要关编辑器全量编译)
- [ ] 展会 build:Package Windows build → Steam Deck 添加 Non-Steam Game → Proton 测试 → Steam Input 映射右触控板 mouse / R2 left click / D-pad or stick movement
