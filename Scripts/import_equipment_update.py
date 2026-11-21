"""Import the jaw-enabled shark, generated item art, and lightweight ocean settings."""
import unreal,runpy,shutil
from pathlib import Path
project=Path(unreal.Paths.project_dir()).resolve()
backup=project/'Saved/EquipmentAssetBackups';backup.mkdir(parents=True,exist_ok=True)
for name in ['SK_Shark','SK_Shark_Skeleton','A_Shark_Swim']:
    source=project/'Content/HumanFishing/Ocean/Fauna'/(name+'.uasset')
    if source.exists() and not (backup/source.name).exists():shutil.copy2(source,backup/source.name)
source=project/'SourceArt/Ocean/Quaternius/Shark_Jaw.fbx'
destination='/Game/HumanFishing/Ocean/Fauna'
tools=unreal.AssetToolsHelpers.get_asset_tools()
def perform(name,options):
    task=unreal.AssetImportTask();task.filename=str(source);task.destination_path=destination;task.destination_name=name
    task.options=options;task.automated=True;task.replace_existing=True;task.save=True
    tools.import_asset_tasks([task]);unreal.log('HF_SHARK_IMPORTED '+str(task.imported_object_paths))
options=unreal.FbxImportUI();options.automated_import_should_detect_type=False
options.import_mesh=True;options.import_as_skeletal=True;options.import_animations=False
options.mesh_type_to_import=unreal.FBXImportType.FBXIT_SKELETAL_MESH
options.skeleton=None
options.create_physics_asset=False;options.import_materials=True;options.import_textures=False
options.skeletal_mesh_import_data.force_front_x_axis=True
perform('SK_Shark_Jaw',options)
options=unreal.FbxImportUI();options.automated_import_should_detect_type=False
options.import_mesh=False;options.import_animations=True;options.import_as_skeletal=True
options.mesh_type_to_import=unreal.FBXImportType.FBXIT_ANIMATION
options.skeleton=unreal.load_asset(destination+'/SK_Shark_Jaw_Skeleton')
options.anim_sequence_import_data.force_front_x_axis=True
options.override_animation_name='A_Shark_Jaw_Swim'
perform('A_Shark_Jaw_Swim',options)
unreal.EditorAssetLibrary.save_directory(destination,only_if_is_dirty=True,recursive=True)
flash=unreal.load_asset(destination+'/M_JellyshockFlash')
if not flash:
    flash=tools.create_asset('M_JellyshockFlash',destination,unreal.Material,unreal.MaterialFactoryNew())
    flash.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
    flash.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
    library=unreal.MaterialEditingLibrary
    color=library.create_material_expression(flash,unreal.MaterialExpressionConstant3Vector,-400,0)
    color.set_editor_property('constant',unreal.LinearColor(.08,1.0,1.5))
    library.connect_material_property(color,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    rim=library.create_material_expression(flash,unreal.MaterialExpressionFresnel,-400,160)
    strength=library.create_material_expression(flash,unreal.MaterialExpressionScalarParameter,-400,300)
    strength.set_editor_property('parameter_name','Strength');strength.set_editor_property('default_value',.5)
    multiply=library.create_material_expression(flash,unreal.MaterialExpressionMultiply,-100,160)
    library.connect_material_expressions(rim,'',multiply,'A');library.connect_material_expressions(strength,'',multiply,'B')
    library.connect_material_property(multiply,'',unreal.MaterialProperty.MP_OPACITY)
    library.recompile_material(flash);unreal.EditorAssetLibrary.save_loaded_asset(flash)
unreal.MaterialEditingLibrary.set_material_usage(flash,unreal.MaterialUsage.MATUSAGE_SKELETAL_MESH)
unreal.MaterialEditingLibrary.recompile_material(flash)
unreal.EditorAssetLibrary.save_loaded_asset(flash)
runpy.run_path(str(project/'Scripts/import_ui_assets.py'),run_name='__main__')
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level('/Game/HumanFishing/Maps/L_DeepOcean')
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    pass # Runtime mesh update policy and volumetric-fog settings are owned by native/config code.
levels.save_current_level()
unreal.log('HF_EQUIPMENT_ASSETS_READY')
