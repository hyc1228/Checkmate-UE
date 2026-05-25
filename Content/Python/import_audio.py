# Auto-import 所有 .ogg in Content/Audio/Placeholder/ → SoundWave 资产。
# 重跑会强制重新导入（覆盖已有 .uasset）— 替素材时必须这样才能更新内容。
#
# 用法：Output Log Cmd → Python → 粘贴：
#   exec(open(r"D:/Unreal Projects/Checkmate/Content/Python/import_audio.py").read())

import os
import unreal

SOURCE_DIR = r"D:/Unreal Projects/Checkmate/Content/Audio/Placeholder"
DEST_PATH = "/Game/Audio/Placeholder"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
imported = []

for fname in sorted(os.listdir(SOURCE_DIR)):
    if not fname.lower().endswith((".ogg", ".wav")):
        continue
    src = os.path.join(SOURCE_DIR, fname)
    base = os.path.splitext(fname)[0]

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", src)
    task.set_editor_property("destination_path", DEST_PATH)
    task.set_editor_property("destination_name", base)
    task.set_editor_property("replace_existing", True)   # 强制覆盖
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    task.set_editor_property("automated", True)

    asset_tools.import_asset_tasks([task])
    imported.append(base)
    unreal.log(f"[ok] (re)imported {base}")

unreal.log(f"=== Done. Re-imported {len(imported)} sounds")
