"""Rebuild DA_Ch2Level_01 from the current Ch2 PV slice layout.

Source of truth:
  F:/OneDrive - Aalto University/YvaineLocal/3.论文/design/gdd/ch2-slice-space.md

Layout v0.2.1:
  Start (5,7)
  WeddingWreckage (3,4), (4,4)
  ClownPickup (5,4)
  Destructible "jiang" gate (5,3)
  Exit (4,1)
  Other cells empty
"""

import unreal

DA_PATH = "/Game/Ch2/Data/DA_Ch2Level_01.DA_Ch2Level_01"

CELL = unreal.Ch2CellType

LAYOUT = {
    (5, 7): CELL.START,
    (3, 4): CELL.WEDDING_WRECKAGE,
    (4, 4): CELL.WEDDING_WRECKAGE,
    (5, 4): CELL.CLOWN_PICKUP,
    (5, 3): CELL.DESTRUCTIBLE,
    (4, 1): CELL.EXIT,
}

MOVE_BUDGET_DEMO = 5
OPTIMAL_MOVE_COUNT = 3


def main():
    data_asset = unreal.EditorAssetLibrary.load_asset(DA_PATH)
    if not data_asset:
        unreal.log_error(f"Missing {DA_PATH}")
        return

    cells = {
        unreal.IntPoint(x, y): cell_type
        for (x, y), cell_type in LAYOUT.items()
    }
    data_asset.set_editor_property("grid_width", 8)
    data_asset.set_editor_property("grid_height", 8)
    data_asset.set_editor_property("cells", cells)
    data_asset.set_editor_property("move_budget", MOVE_BUDGET_DEMO)
    data_asset.set_editor_property("optimal_move_count", OPTIMAL_MOVE_COUNT)
    unreal.EditorAssetLibrary.save_loaded_asset(data_asset)

    unreal.log(
        "[ok] DA_Ch2Level_01 rebuilt: "
        f"{len(cells)} cells / MoveBudget={MOVE_BUDGET_DEMO} / "
        "PV recording can use console command: PV_SetMoveBudget 99"
    )


if __name__ == "__main__":
    main()
