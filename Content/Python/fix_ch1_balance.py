# Ch1 平衡修：
#   1) 配额修宽松（Shift 1 教学 PassQuota=1）
#   2) 通过 duplicate DA_Doll_01 来建 5 个补充 Doll DataAssets
#
# 跑完后还需要 user 手动：
#   打开 BP_Chapter1GameMode → Shifts 数组里把新 doll 拖到 DollSequence

import unreal

# ── 1) 配额 recipe ───────────────────────────────────────────────────────────
BP_PATH = "/Game/Ch1/Blueprints/BP_Chapter1GameMode"

RECIPE = [
    {"PassQuota": 1, "RejectQuota": 1, "MaxMisjudgmentsBeforeFail": 99, "DollTimeoutSec": 0.0, "CorrectGoal": 2},
    {"PassQuota": 2, "RejectQuota": 2, "MaxMisjudgmentsBeforeFail": 3, "DollTimeoutSec": 0.0, "CorrectGoal": 4},
    {"PassQuota": 2, "RejectQuota": 2, "MaxMisjudgmentsBeforeFail": 3, "DollTimeoutSec": 8.0, "CorrectGoal": 4},
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


# ── 2) Duplicate DA_Doll_01 → 5 个补充娃娃 ──────────────────────────────────
TEMPLATE = "/Game/Ch1/Data/Dolls/DA_Doll_01_BalletPerfect"
DOLL_DIR = "/Game/Ch1/Data/Dolls"

NEW_DOLLS = [
    {"name": "DA_Doll_04_BalletVeil",        "id": "Ballet_Veil_04",
     "posture": "BalletPose", "hair": ["LongHair", "Styled"],
     "expr": ["Smile"], "acc": ["Veil"], "twist": False, "style": 1},  # Pearl
    {"name": "DA_Doll_05_BalletPerfectPearl","id": "Ballet_PerfectPearl_05",
     "posture": "BalletPose", "hair": ["LongHair", "NaturalColor", "Styled"],
     "expr": ["Smile"], "acc": ["PearlNecklace", "Veil"], "twist": True, "style": 1},
    {"name": "DA_Doll_06_BalletNoSmile",     "id": "Ballet_NoSmile_06",
     "posture": "BalletPose", "hair": ["LongHair"],
     "expr": [], "acc": ["Ribbon"], "twist": False, "style": 0},  # Standard
    {"name": "DA_Doll_07_BowingFailing",     "id": "Bowing_Fail_07",
     "posture": "Bowing", "hair": [],
     "expr": [], "acc": ["WhiteGloves"], "twist": False, "style": 0},
    {"name": "DA_Doll_08_UprightSmile",      "id": "Upright_Smile_08",
     "posture": "Upright", "hair": ["Styled"],
     "expr": ["Smile"], "acc": ["Apron"], "twist": False, "style": 2},  # Bell
]

def to_name_set(strs):
    s = set()
    for n in strs:
        s.add(unreal.Name(n))
    return s

for d in NEW_DOLLS:
    dst = f"{DOLL_DIR}/{d['name']}"
    if not unreal.EditorAssetLibrary.does_asset_exist(dst):
        if not unreal.EditorAssetLibrary.duplicate_asset(TEMPLATE, dst):
            unreal.log_error(f"[fail] 复制 {d['name']} 失败")
            continue
    asset = unreal.EditorAssetLibrary.load_asset(dst)
    if not asset:
        unreal.log_error(f"[fail] 加载新 {d['name']} 失败")
        continue
    asset.set_editor_property("doll_id", unreal.Name(d["id"]))
    asset.set_editor_property("posture", unreal.Name(d["posture"]))
    asset.set_editor_property("hair_traits", to_name_set(d["hair"]))
    asset.set_editor_property("expression_traits", to_name_set(d["expr"]))
    asset.set_editor_property("accessory_traits", to_name_set(d["acc"]))
    asset.set_editor_property("is_twist_trigger", d["twist"])
    # ButtonStyle enum 跳过（Python 不让 int → enum）；默认 Standard，需要 Pearl 的 doll
    # 在 Content Browser 里手动改 ButtonStyle 字段即可
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    unreal.log(f"[ok] 创建 {d['name']}  posture={d['posture']} pearl_twist={d['twist']}")

unreal.log("=== Done. 5 新 doll 创建完成（在 BP_Chapter1GameMode → Shifts → DollSequence 里挂上去）")
