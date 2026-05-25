# Rebuild DA_Ch2Level_01 to enforce explode→exit strategy.
# Layout (per spec switching-ritual + slice v0.2)：
#   - Ballet east 滑到 Pickup (7,0)
#   - Clown 跳到 Exit (3,7) 必经 destructible (2,5)/(4,5)
#   - 必须放爆炸玩偶 → 炸开 destructible → 跳过

import unreal

DA_PATH = "/Game/Ch2/Data/DA_Ch2Level_01.DA_Ch2Level_01"

# (ECh2CellType enum int values 与 .h 顺序一致)
EMPTY, WALL, DEST, PICKUP, EXIT, START, WRECK = 0, 1, 2, 3, 4, 5, 6

LAYOUT = {
    (0, 0): START,
    (7, 0): PICKUP,
    (3, 7): EXIT,
    # 阻挡 Ballet 北向：迫使玩家东滑去 Pickup
    (0, 4): WALL,
    # Exit 周围阻挡 — 只剩 (2,5) 和 (4,5) 两条 knight-landing 路径
    (1, 6): WALL,
    (5, 6): WALL,
    (2, 7): WALL,
    (4, 7): WALL,
    # 关键 destructible：必须爆炸玩偶炸开才能 knight 到 Exit
    (2, 5): DEST,
    (4, 5): DEST,
    # 装饰彩蛋：婚纱残骸（spec 锁定 接不上的铃场）
    (4, 4): WRECK,
}

da = unreal.EditorAssetLibrary.load_asset(DA_PATH)
if not da:
    unreal.log_error(f"未找到 {DA_PATH}")
else:
    cells = {}
    for (x, y), t in LAYOUT.items():
        cells[unreal.IntPoint(x, y)] = t
    da.set_editor_property("cells", cells)
    da.set_editor_property("move_budget", 15)
    da.set_editor_property("optimal_move_count", 12)
    unreal.EditorAssetLibrary.save_loaded_asset(da)
    unreal.log(f"[ok] DA_Ch2Level_01 重写完成：{len(cells)} cells / MoveBudget=15 / Optimal=12")
