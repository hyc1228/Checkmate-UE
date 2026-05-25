# Setup MI_Eye_Pearl / MI_Eye_Mechanical vector parameters.
# Already created via MCP; this fills Color since the MCP vector format is finicky.

import unreal

CONFIGS = [
    ("/Game/Materials/MI_Eye_Pearl.MI_Eye_Pearl",
     unreal.LinearColor(0.96, 0.92, 0.82, 1.0), 0.8),
    ("/Game/Materials/MI_Eye_Mechanical.MI_Eye_Mechanical",
     unreal.LinearColor(0.33, 0.40, 0.64, 1.0), 4.5),
]

for path, color, intensity in CONFIGS:
    mi = unreal.EditorAssetLibrary.load_asset(path)
    if mi is None:
        unreal.log_error(f"未找到 {path}")
        continue
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(mi, "Color", color)
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mi, "Intensity", intensity)
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    unreal.log(f"[ok] {path} Color={color} Intensity={intensity}")
