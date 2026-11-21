import unreal
material=unreal.load_asset('/Game/HumanFishing/Ocean/Fauna/M_JellyshockFlash')
assert material
unreal.MaterialEditingLibrary.set_material_usage(material,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)
unreal.log('HF_SHOCK_MATERIAL_READY')
