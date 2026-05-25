# 修 BP_Chapter1GameMode 的 Shifts 配额，让"全扔" exploit 失效。
# Per spec judgment-card.md §4 PV 调参 + 用户实测反馈。
#
# Run: Tools → Run Python Script → 选这个文件

import unreal

BP_PATH = "/Game/Ch1/BP_Chapter1GameMode.BP_Chapter1GameMode"

# 推荐配额（per spec PV 调参）
RECIPE = [
    # ShiftIdx 0 (Shift 1 教学班)
    {"PassQuota": 2, "RejectQuota": 1, "MaxMisjudgmentsBeforeFail": 99, "DollTimeoutSec": 0.0, "CorrectGoal": 3},
    # ShiftIdx 1 (Shift 2)
    {"PassQuota": 3, "RejectQuota": 2, "MaxMisjudgmentsBeforeFail": 3, "DollTimeoutSec": 0.0, "CorrectGoal": 5},
    # ShiftIdx 2 (Shift 3)
    {"PassQuota": 3, "RejectQuota": 2, "MaxMisjudgmentsBeforeFail": 3, "DollTimeoutSec": 8.0, "CorrectGoal": 5},
    # ShiftIdx 3 (Shift 4)
    {"PassQuota": 3, "RejectQuota": 2, "MaxMisjudgmentsBeforeFail": 3, "DollTimeoutSec": 6.0, "CorrectGoal": 5},
]

bp_class = unreal.EditorAssetLibrary.load_blueprint_class(BP_PATH.split(".")[0])
if not bp_class:
    unreal.log_error(f"无法加载 {BP_PATH}")
else:
    cdo = unreal.get_default_object(bp_class)
    shifts = cdo.get_editor_property("shifts")
    # shifts 是 Array of FShiftConfig (StructBase)
    n = len(shifts) if shifts else 0
    unreal.log(f"BP_Chapter1GameMode 现有 {n} 个 shifts")

    if shifts:
        for i, cfg in enumerate(shifts):
            recipe = RECIPE[min(i, len(RECIPE) - 1)]
            for k, v in recipe.items():
                snake = "".join(["_" + c.lower() if c.isupper() else c for c in k]).lstrip("_")
                cfg.set_editor_property(snake, v)
            unreal.log(f"  [Shift {i+1}] PassQ={recipe['PassQuota']} RejectQ={recipe['RejectQuota']} "
                       f"Max={recipe['MaxMisjudgmentsBeforeFail']} Timeout={recipe['DollTimeoutSec']}s")
        # 触发 BP 重 compile + 保存
        cdo.set_editor_property("shifts", shifts)
        # 保存 BP package
        bp_pkg = unreal.EditorAssetLibrary.load_asset(BP_PATH)
        if bp_pkg:
            unreal.EditorAssetLibrary.save_loaded_asset(bp_pkg)
        unreal.log("[ok] BP_Chapter1GameMode 全部 Shifts 配额已更新")
    else:
        unreal.log_warning("Shifts 数组为空 — 请先在 BP 里手填 4 条 Shift")
