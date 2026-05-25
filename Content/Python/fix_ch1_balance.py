# Ch1 平衡修：
#   1) 配额修宽松（Shift 1 教学 PassQuota=1）
#   2) 新建 5 个补充 Doll DataAssets（增加合规变体 + 明显瑕疵）
#
# 跑完后还需要 user 手动：
#   打开 BP_Chapter1GameMode → Shifts 数组里把新 doll 拖到 DollSequence
#
# 用法：Output Log Cmd → Python → 粘贴：
#   exec(open(r"D:/Unreal Projects/Checkmate/Content/Python/fix_ch1_balance.py").read())

import unreal

# ── 1) 配额 recipe ───────────────────────────────────────────────────────────
BP_PATH = "/Game/Ch1/BP_Chapter1GameMode"

RECIPE = [
    # Shift 1 教学：1 放行 + 1 丢弃，不会失败
    {"PassQuota": 1, "RejectQuota": 1, "MaxMisjudgmentsBeforeFail": 99, "DollTimeoutSec": 0.0, "CorrectGoal": 2},
    # Shift 2：稍紧
    {"PassQuota": 2, "RejectQuota": 2, "MaxMisjudgmentsBeforeFail": 3, "DollTimeoutSec": 0.0, "CorrectGoal": 4},
    # Shift 3：时间压力
    {"PassQuota": 2, "RejectQuota": 2, "MaxMisjudgmentsBeforeFail": 3, "DollTimeoutSec": 8.0, "CorrectGoal": 4},
    # Shift 4：Pearl 路径浮现
    {"PassQuota": 3, "RejectQuota": 2, "MaxMisjudgmentsBeforeFail": 3, "DollTimeoutSec": 6.0, "CorrectGoal": 5},
]

bp_class = unreal.EditorAssetLibrary.load_blueprint_class(BP_PATH)
if not bp_class:
    unreal.log_error(f"无法加载 {BP_PATH}")
else:
    cdo = unreal.get_default_object(bp_class)
    shifts = cdo.get_editor_property("shifts")
    if shifts:
        for i, cfg in enumerate(shifts):
            recipe = RECIPE[min(i, len(RECIPE) - 1)]
            for k, v in recipe.items():
                snake = "".join(["_" + c.lower() if c.isupper() else c for c in k]).lstrip("_")
                cfg.set_editor_property(snake, v)
            unreal.log(f"  [Shift {i+1}] PassQ={recipe['PassQuota']} RejQ={recipe['RejectQuota']} "
                       f"Max={recipe['MaxMisjudgmentsBeforeFail']} TO={recipe['DollTimeoutSec']}s")
        cdo.set_editor_property("shifts", shifts)
        bp_pkg = unreal.EditorAssetLibrary.load_asset(BP_PATH)
        if bp_pkg:
            unreal.EditorAssetLibrary.save_loaded_asset(bp_pkg)
        unreal.log("[ok] Shifts recipe 更新完成")
    else:
        unreal.log_warning("Shifts 数组为空 — 请先在 BP 里手填")


# ── 2) 新建 5 个补充娃娃 ─────────────────────────────────────────────────────
# 命名 + 属性 — 增加合规变体（让 Shift 1-2 教学时容易碰到对的）+ 明显瑕疵
NEW_DOLLS = [
    # 合规：典型芭蕾完美变体
    {"name": "DA_Doll_04_BalletVeil",     "id": "Ballet_Veil_04",
     "posture": "BalletPose", "hair": {"LongHair", "Styled"},
     "expr": {"Smile"}, "acc": {"Veil"}, "twist": False, "style": "Pearl"},
    # 合规：另一种 Pearl 主路径触发候选
    {"name": "DA_Doll_05_BalletPerfectPearl", "id": "Ballet_PerfectPearl_05",
     "posture": "BalletPose", "hair": {"LongHair", "NaturalColor", "Styled"},
     "expr": {"Smile"}, "acc": {"PearlNecklace", "Veil"}, "twist": True, "style": "Pearl"},
    # 半合规：合规姿态但少了表情（让 Shift 3+ 玩家深思）
    {"name": "DA_Doll_06_BalletNoSmile",  "id": "Ballet_NoSmile_06",
     "posture": "BalletPose", "hair": {"LongHair"},
     "expr": set(), "acc": {"Ribbon"}, "twist": False, "style": "Standard"},
    # 明显瑕疵：姿态错（Bowing 而非 BalletPose）
    {"name": "DA_Doll_07_BowingFailing",  "id": "Bowing_Fail_07",
     "posture": "Bowing", "hair": set(),
     "expr": set(), "acc": {"WhiteGloves"}, "twist": False, "style": "Standard"},
    # 边缘：Upright 姿态 + 仅 Smile，看似 OK 但姿态可能不符
    {"name": "DA_Doll_08_UprightSmile",   "id": "Upright_Smile_08",
     "posture": "Upright", "hair": {"Styled"},
     "expr": {"Smile"}, "acc": {"Apron"}, "twist": False, "style": "Bell"},
]

DOLL_DIR = "/Game/Ch1/Data/Dolls"
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

def name_set(strs):
    return unreal.Set(unreal.Name)([unreal.Name(s) for s in strs])

style_enum = unreal.load_object(None, "/Script/Checkmate.EButtonEyeStyle")

for d in NEW_DOLLS:
    full_path = f"{DOLL_DIR}/{d['name']}"
    if unreal.EditorAssetLibrary.does_asset_exist(full_path):
        unreal.log(f"[skip] {d['name']} 已存在")
        continue
    factory = unreal.DataAssetFactory()
    cls = unreal.load_class(None, "/Script/Checkmate.DollData")
    factory.data_asset_class = cls
    asset = asset_tools.create_asset(d["name"], DOLL_DIR, cls, factory)
    if not asset:
        unreal.log_error(f"[fail] 创建 {d['name']} 失败")
        continue
    asset.set_editor_property("doll_id", unreal.Name(d["id"]))
    asset.set_editor_property("posture", unreal.Name(d["posture"]))
    asset.set_editor_property("hair_traits", name_set(d["hair"]))
    asset.set_editor_property("expression_traits", name_set(d["expr"]))
    asset.set_editor_property("accessory_traits", name_set(d["acc"]))
    asset.set_editor_property("b_is_twist_trigger", d["twist"])
    # ButtonStyle enum
    style_map = {"Standard": 0, "Pearl": 1, "Bell": 2, "VeilPin": 3}
    asset.set_editor_property("button_style", style_map.get(d["style"], 0))
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    unreal.log(f"[ok] 创建 {d['name']}  posture={d['posture']} pearl={d['twist']}")

unreal.log("=== Done. 5 新 doll 创建完成（在 BP_Chapter1GameMode → Shifts → DollSequence 里挂上去）")
