# Auto-import all .ogg in Content/Audio/Placeholder/ → SoundWave assets.
# Run in UE editor: 在 Output Log 顶端 "Cmd:" 切到 Python 模式输入：
#   py "D:/Unreal Projects/Checkmate/Content/Python/import_audio.py"
# 或菜单 Tools > Execute Python Script... 选这个文件。

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

    # 已经导过就跳过
    existing = f"{DEST_PATH}/{base}.{base}"
    if unreal.EditorAssetLibrary.does_asset_exist(existing):
        unreal.log(f"[skip] {base} 已存在")
        continue

    task = unreal.AssetImportTask()
    task.set_editor_property("filename", src)
    task.set_editor_property("destination_path", DEST_PATH)
    task.set_editor_property("destination_name", base)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    task.set_editor_property("automated", True)

    asset_tools.import_asset_tasks([task])
    imported.append(base)
    unreal.log(f"[ok] imported {base}")

unreal.log(f"=== Done. Imported {len(imported)} sounds: {imported}")
