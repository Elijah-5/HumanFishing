import unreal, json
from pathlib import Path
project=Path(unreal.Paths.project_dir()).resolve()
meshes={}
for path in unreal.EditorAssetLibrary.list_assets('/Game/HumanFishing/Ocean',True,False):
    obj=unreal.load_asset(path)
    if isinstance(obj,unreal.StaticMesh):
        b=obj.get_bounds()
        meshes[path]={'origin':list(b.origin.to_tuple()),'extent':list(b.box_extent.to_tuple())}
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/HumanFishing/Maps/L_DeepOcean')
shop=[]
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label()=='SeaweedExchange':
        for c in a.get_components_by_class(unreal.SceneComponent):
            shop.append({'name':c.get_name(),'class':c.get_class().get_name(),'loc':list(c.get_world_location().to_tuple()),'scale':list(c.get_world_scale().to_tuple())})
reg=unreal.AssetRegistryHelpers.get_asset_registry()
opts=unreal.AssetRegistryDependencyOptions(include_soft_package_references=True,include_hard_package_references=True,include_searchable_names=False,include_soft_management_references=False,include_hard_management_references=False)
roots=['/Game/HumanFishing/Maps/'+n for n in ['L_SaveMenu','L_ShorePrototype','L_DeepOcean','L_ScenicCove']]
roots+=['/Game/HumanFishing/Data/DA_HumanFishingCatalog']
parents={r:None for r in roots};queue=roots[:]
while queue:
    p=queue.pop(0)
    for dependency in reg.get_dependencies(p,opts) or []:
        d=str(dependency)
        if d.startswith('/Game/') and d not in parents:
            parents[d]=p;queue.append(d)
out={'meshes':meshes,'shop':shop,'parents':parents}
(project/'Saved/OceanInspection.json').write_text(json.dumps(out,indent=2))
unreal.log('HF_INSPECTION_COMPLETE')
