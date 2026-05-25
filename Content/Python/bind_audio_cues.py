# Bind 10 NativeCues into /Game/Audio/DA_AudioCues.
# Run after import_audio.py.
#   Tools → Run Python Script → 选这个文件

import unreal

DA_PATH = "/Game/Audio/DA_AudioCues.DA_AudioCues"
PLACEHOLDER = "/Game/Audio/Placeholder/"

KEY_TO_ASSET = {
    "Ch1.Stamp":   "Ch1_Stamp",
    "Ch1.Toss":    "Ch1_Toss",
    "Ch1.Correct": "Ch1_Correct",
    "Ch1.Wrong":   "Ch1_Wrong",
    "Ch2.Move":    "Ch2_Move",
    "Ch2.Explode": "Ch2_Explode",
    "Ch2.Ritual":  "Ch2_Ritual",
    "Ch2.Victory": "Ch2_Victory",
    "UI.Click":    "UI_Click",
    "UI.Hover":    "UI_Hover",
    "Amb.Ch1":     "Amb_Ch1",
    "Amb.Ch2":     "Amb_Ch2",
}

da = unreal.EditorAssetLibrary.load_asset(DA_PATH)
if not da:
    unreal.log_error(f"无法加载 {DA_PATH}")
else:
    cues = {}
    for key, name in KEY_TO_ASSET.items():
        path = f"{PLACEHOLDER}{name}.{name}"
        sw = unreal.EditorAssetLibrary.load_asset(path)
        if sw is None:
            unreal.log_warning(f"[miss] {path}")
            continue
        cues[unreal.Name(key)] = sw

    da.set_editor_property("native_cues", cues)
    unreal.EditorAssetLibrary.save_loaded_asset(da)
    unreal.log(f"[ok] DA_AudioCues 绑了 {len(cues)} 条 NativeCues")
