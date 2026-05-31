import unreal

CARD_DEST = "/Game/Ch1/Data/Cards"
EXPECTED = {
    "DA_Card_PriceTag": ("PriceTag", 1),
    "DA_Card_HighHeels": ("HighHeels", 2),
    "DA_Card_BellGirl": ("BellGirl", 3),
    "DA_Card_BowDiscipline": ("BowDiscipline", 5),
    "DA_Card_HeartGaze": ("HeartGaze", 6),
    "DA_Card_RoseGaze": ("RoseGaze", 7),
    "DA_Card_NoTears": ("NoTears", 9),
    "DA_Card_SwordAndSerpent": ("SwordAndSerpent", 15),
    "DA_Card_QueensGambit": ("QueensGambit", 16),
    "DA_Card_Checkmate": ("Checkmate", 17),
    "DA_Card_Empty": ("Empty", 0),
}

failed = False
def enum_int(value):
    try:
        return int(value)
    except Exception:
        text = str(value)
        if ":" in text:
            tail = text.split(":")[-1].strip()
            digits = "".join(ch for ch in tail if ch.isdigit())
            if digits:
                return int(digits)
    return -1


for asset_name, (card_id, effect_value) in EXPECTED.items():
    asset = unreal.EditorAssetLibrary.load_asset(f"{CARD_DEST}/{asset_name}")
    if not asset:
        unreal.log_error(f"[VerifyCh1Cards] Missing {asset_name}")
        failed = True
        continue

    actual_card_id = str(asset.get_editor_property("card_id"))
    actual_effect = enum_int(asset.get_editor_property("piecework_effect"))
    icon = asset.get_editor_property("icon_texture")
    skill = str(asset.get_editor_property("skill_description_markup"))
    if actual_card_id != card_id or actual_effect != effect_value or not icon or not skill:
        unreal.log_error(
            f"[VerifyCh1Cards] Bad {asset_name}: CardId={actual_card_id}, Effect={actual_effect}, Icon={icon}, Skill={skill}"
        )
        failed = True
    else:
        unreal.log(f"[VerifyCh1Cards] OK {asset_name}: CardId={actual_card_id}, Effect={actual_effect}")

if failed:
    raise RuntimeError("Ch1 card verification failed")

unreal.log("[VerifyCh1Cards] All card assets verified")

bp_class = unreal.EditorAssetLibrary.load_blueprint_class("/Game/Ch1/Blueprints/BP_Chapter1GameMode")
if bp_class:
    cdo = unreal.get_default_object(bp_class)
    shifts = cdo.get_editor_property("shifts")
    for index, cfg in enumerate(shifts or []):
        pool = cfg.get_editor_property("pool_cards")
        names = [card.get_name() for card in pool if card]
        unreal.log(f"[VerifyCh1Cards] Shift {index + 1} K={cfg.get_editor_property('k')} Pool={names}")
else:
    unreal.log_warning("[VerifyCh1Cards] Could not load BP_Chapter1GameMode for shift-pool verification")
