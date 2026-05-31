import os
import unreal

SOURCE_DIR = r"E:\0000.Checkmate\Cards\Final"
TEXTURE_DEST = "/Game/Ch1/Art/Cards"
CARD_DEST = "/Game/Ch1/Data/Cards"
CARD_ROWS = [
    {"id": "PriceTag", "texture": "T_Card_PriceTag.png", "asset": "DA_Card_PriceTag"},
    {"id": "HighHeels", "texture": "T_Card_HighHeels.png", "asset": "DA_Card_HighHeels"},
    {"id": "BellGirl", "texture": "T_Card_BellGirl.png", "asset": "DA_Card_BellGirl"},
    {"id": "HeartGaze", "texture": "T_Card_HeartGaze.png", "asset": "DA_Card_HeartGaze"},
    {"id": "NoTears", "texture": "T_Card_NoTears.png", "asset": "DA_Card_NoTears"},
    {"id": "RoseGaze", "texture": "T_Card_RoseGaze.png", "asset": "DA_Card_RoseGaze"},
    {"id": "BowDiscipline", "texture": "T_Card_BowDiscipline.png", "asset": "DA_Card_BowDiscipline"},
    {"id": "SwordAndSerpent", "texture": "T_Card_SwordAndSerpent.png", "asset": "DA_Card_SwordAndSerpent"},
    {"id": "QueensGambit", "texture": "T_Card_QueensGambit.png", "asset": "DA_Card_QueensGambit"},
    {"id": "Checkmate", "texture": "T_Card_Checkmate.png", "asset": "DA_Card_Checkmate"},
    {"id": "Empty", "texture": "T_Card_Empty.png", "asset": "DA_Card_Empty"},
]

def ensure_directory(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def import_textures():
    ensure_directory(TEXTURE_DEST)
    tasks = []
    for row in CARD_ROWS:
        filename = os.path.join(SOURCE_DIR, row["texture"])
        if not os.path.exists(filename):
            unreal.log_warning(f"[Ch1Cards] Missing source texture: {filename}")
            continue

        task = unreal.AssetImportTask()
        task.set_editor_property("filename", filename)
        task.set_editor_property("destination_path", TEXTURE_DEST)
        task.set_editor_property("destination_name", os.path.splitext(row["texture"])[0])
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("automated", True)
        task.set_editor_property("save", True)
        tasks.append(task)

    if tasks:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    unreal.log(f"[Ch1Cards] Imported {len(tasks)} card textures to {TEXTURE_DEST}")


def load_texture(row):
    texture_name = os.path.splitext(row["texture"])[0]
    return unreal.EditorAssetLibrary.load_asset(f"{TEXTURE_DEST}/{texture_name}")


def make_card_asset(row):
    ensure_directory(CARD_DEST)
    asset_path = f"{CARD_DEST}/{row['asset']}"
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not asset:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.CardData)
        asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            row["asset"],
            CARD_DEST,
            unreal.CardData,
            factory,
        )
    if not asset:
        unreal.log_error(f"[Ch1Cards] Failed to create/load {asset_path}")
        return None

    asset.set_editor_property("card_id", unreal.Name(row["id"]))
    if hasattr(asset, "apply_ch1_slice_preset"):
        asset.apply_ch1_slice_preset()

    texture = load_texture(row)
    if texture:
        asset.set_editor_property("icon_texture", texture)
    else:
        unreal.log_warning(f"[Ch1Cards] Texture not found for {row['asset']}")

    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    unreal.log(f"[Ch1Cards] Configured {asset_path}")
    return asset


def configure_cards():
    card_assets = {}
    for row in CARD_ROWS:
        asset = make_card_asset(row)
        if asset:
            card_assets[row["id"]] = asset
    return card_assets


def main():
    import_textures()
    configure_cards()
    unreal.EditorAssetLibrary.save_directory(TEXTURE_DEST, only_if_is_dirty=False, recursive=True)
    unreal.EditorAssetLibrary.save_directory(CARD_DEST, only_if_is_dirty=False, recursive=True)
    unreal.log("[Ch1Cards] Done. BP_Chapter1GameMode shift pools are configured separately through UE-MCP.")


main()
