"""Editor task: import the selected permissively licensed ocean models."""
from pathlib import Path
import json
import shutil
import unreal

project=Path(unreal.Paths.project_dir()).resolve()
sources=Path('/private/tmp/humanfishing-model-sources')
tools=unreal.AssetToolsHelpers.get_asset_tools()
root='/Game/HumanFishing/Ocean'

def task(filename,destination,name,options):
    t=unreal.AssetImportTask()
    t.filename=str(filename);t.destination_path=destination;t.destination_name=name
    t.automated=True;t.replace_existing=True;t.save=True;t.options=options
    tools.import_asset_tasks([t])
    unreal.EditorAssetLibrary.save_directory(destination,only_if_is_dirty=True,recursive=True)
    objects=[unreal.load_asset(p) for p in unreal.EditorAssetLibrary.list_assets(destination,recursive=True,include_folder=False)]
    unreal.log('HF_IMPORTED '+name+' '+str(t.imported_object_paths))
    return objects

def fbx_options(skeletal=False):
    o=unreal.FbxImportUI()
    o.set_editor_property('automated_import_should_detect_type',False)
    o.set_editor_property('import_mesh',True)
    o.set_editor_property('import_as_skeletal',skeletal)
    o.set_editor_property('mesh_type_to_import',unreal.FBXImportType.FBXIT_SKELETAL_MESH if skeletal else unreal.FBXImportType.FBXIT_STATIC_MESH)
    o.set_editor_property('import_animations',skeletal)
    o.set_editor_property('create_physics_asset',False)
    o.set_editor_property('import_materials',True)
    o.set_editor_property('import_textures',True)
    data=o.get_editor_property('skeletal_mesh_import_data' if skeletal else 'static_mesh_import_data')
    data.set_editor_property('force_front_x_axis',True)
    if not skeletal:
        data.set_editor_property('combine_meshes',True)
        data.set_editor_property('auto_generate_collision',True)
    return o

manifest={'fauna':{},'props':{},'corals':{}}
for name in ['Shark','Fish1','Fish3']:
    src=sources/'fish/Animated Fish Pack by @Quaternius/FBX'/(name+'.fbx')
    objects=task(src,root+'/Fauna','SK_'+name,fbx_options(True))
    mesh=next((o for o in objects if isinstance(o,unreal.SkeletalMesh) and o.get_name()=='SK_'+name),None)
    animations=[o for o in objects if isinstance(o,unreal.AnimSequence) and (o.get_name().startswith('SK_'+name+'_') or o.get_name()=='A_'+name+'_Swim')]
    assert mesh and animations,'Missing skeletal mesh or swimming animation for '+name
    target=root+'/Fauna/A_'+name+'_Swim'
    if animations[0].get_path_name().split('.')[0]!=target:
        assert unreal.EditorAssetLibrary.rename_asset(animations[0].get_path_name(),target)
    manifest['fauna'][name]={'mesh':mesh.get_path_name(),'animation':target}

for name in ['ship-wreck','barrel','crate','chest','rocks-a','rocks-b','rocks-c','rocks-sand-a','grass-plant']:
    objects=task(sources/'pirate/Models/FBX format'/(name+'.fbx'),root+'/Kenney','SM_'+name.replace('-','_'),fbx_options())
    mesh=next((o for o in objects if isinstance(o,unreal.StaticMesh) and o.get_name()=='SM_'+name.replace('-','_')),None)
    assert mesh,'Missing Kenney mesh '+name
    manifest['props'][name]=mesh.get_path_name()

for i in range(1,7):
    options=unreal.GLTFImportOptions()
    options.set_editor_property('import_scale',100.)
    options.set_editor_property('generate_lightmap_u_vs',False)
    # The importer preserves each separately authored model and its palette UVs.
    objects=task(sources/('coral%d.glb'%i),root+'/Coral/Set%d'%i,'CoralSet%d'%i,options)
    assets=unreal.EditorAssetLibrary.list_assets(root+'/Coral/Set%d'%i,recursive=True,include_folder=False)
    meshes=[]
    for path in assets:
        obj=unreal.load_asset(path)
        if isinstance(obj,unreal.StaticMesh): meshes.append(path)
        if isinstance(obj,unreal.Material):
            # Use the author's color texture with a matte, readable surface.
            for prop,value in [(unreal.MaterialProperty.MP_METALLIC,0.),(unreal.MaterialProperty.MP_ROUGHNESS,.85),
                               (unreal.MaterialProperty.MP_EMISSIVE_COLOR,0.)]:
                node=unreal.MaterialEditingLibrary.create_material_expression(obj,unreal.MaterialExpressionConstant,-300,100)
                node.set_editor_property('r',value)
                unreal.MaterialEditingLibrary.connect_material_property(node,'',prop)
            unreal.MaterialEditingLibrary.recompile_material(obj)
            unreal.EditorAssetLibrary.save_loaded_asset(obj)
    assert meshes,'No coral meshes imported for set '+str(i)
    manifest['corals'][str(i)]=meshes

# Store reimport sources outside cooked Content, with each author's license.
archive=project/'SourceArt/Ocean'
archive.mkdir(parents=True,exist_ok=True)
for name in ['Shark','Fish1','Fish3']:
    destination=archive/'Quaternius';destination.mkdir(exist_ok=True)
    shutil.copy2(sources/'fish/Animated Fish Pack by @Quaternius/FBX'/(name+'.fbx'),destination/(name+'.fbx'))
shutil.copy2(sources/'fish/Animated Fish Pack by @Quaternius/License.txt',archive/'Quaternius/License.txt')
destination=archive/'Kenney';destination.mkdir(exist_ok=True)
for name in manifest['props']:
    shutil.copy2(sources/'pirate/Models/FBX format'/(name+'.fbx'),destination/(name+'.fbx'))
shutil.copy2(sources/'pirate/License.txt',destination/'License.txt')
destination=archive/'MiniPoly';destination.mkdir(exist_ok=True)
for i in range(1,7): shutil.copy2(sources/('coral%d.glb'%i),destination/('CoralSet%d.glb'%i))

unreal.EditorAssetLibrary.save_directory(root,only_if_is_dirty=True,recursive=True)
(project/'Saved/OceanAssets.json').write_text(json.dumps(manifest,indent=2))
unreal.log('HF_OCEAN_ASSETS '+json.dumps(manifest))

blueprint=unreal.load_asset('/Game/HumanFishing/Fishing/BP_ShoreFishing')
backup=project/'Saved/UpdateBackups/BP_ShoreFishing.uasset'
backup.parent.mkdir(parents=True,exist_ok=True)
if not backup.exists(): shutil.copy2(project/'Content/HumanFishing/Fishing/BP_ShoreFishing.uasset',backup)
assert unreal.HumanFishingEditorLibrary.patch_fishing_blueprint(blueprint),'Fishing graph migration failed'
assert unreal.EditorAssetLibrary.save_loaded_asset(blueprint)

unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/HumanFishing/Maps/L_DeepOcean')
items=[]
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    location=actor.get_actor_location()
    origin,extent=actor.get_actor_bounds(False)
    component=actor.get_component_by_class(unreal.StaticMeshComponent)
    mesh=component.get_editor_property('static_mesh') if component else None
    items.append({'label':actor.get_actor_label(),'class':actor.get_class().get_name(),
                  'location':[location.x,location.y,location.z],
                  'bounds_origin':[origin.x,origin.y,origin.z],'bounds_extent':[extent.x,extent.y,extent.z],
                  'mesh':mesh.get_path_name() if mesh else None})
(project/'Saved/OceanLayoutBefore.json').write_text(json.dumps(items,indent=2))
unreal.log('HF_IMPORT_AND_FISHING_MIGRATION_COMPLETE')
