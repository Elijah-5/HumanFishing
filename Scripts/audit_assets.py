"""Read-only Asset Registry closure, including native runtime load paths."""
import unreal,json,re
from pathlib import Path
project=Path(unreal.Paths.project_dir()).resolve()
registry=unreal.AssetRegistryHelpers.get_asset_registry();registry.search_all_assets(True)
options=unreal.AssetRegistryDependencyOptions(include_soft_package_references=True,include_hard_package_references=True,include_searchable_names=False,include_soft_management_references=True,include_hard_management_references=True)
roots={'/Game/HumanFishing/Maps/'+n for n in ['L_SaveMenu','L_ShorePrototype','L_DeepOcean','L_ScenicCove']}
roots.add('/Game/HumanFishing/UI/Backpack/DA_HumanFishingCatalog')
for base in [project/'Plugins/HumanFishingGameplay/Source',project/'Config']:
    for p in base.rglob('*'):
        if p.suffix.lower() in ['.cpp','.h','.ini']:
            roots.update(re.findall(r'/Game/[A-Za-z0-9_/.]+',p.read_text(errors='replace')))
roots={r.split('.')[0].rstrip('/') for r in roots}
roots={r for r in roots if registry.get_asset_by_object_path(r+'.'+r.rsplit('/',1)[-1]).is_valid()}
parents={p:None for p in roots};queue=list(roots);missing=set()
while queue:
    current=queue.pop(0)
    for ref in registry.get_dependencies(current,options) or []:
        ref=str(ref)
        if ref.startswith('/Game/') and ref not in parents:
            parents[ref]=current;queue.append(ref)
            path=project/'Content'/(ref[6:]+'.uasset')
            if not path.exists() and not path.with_suffix('.umap').exists():missing.add(ref)
all_files={str(p.relative_to(project/'Content')):p.stat().st_size for p in (project/'Content').rglob('*') if p.is_file()}
unused=[]
for rel,size in all_files.items():
    path=Path(rel)
    if rel.startswith('Characters/') and path.suffix in ['.uasset','.umap']:
        package='/Game/'+str(path.with_suffix(''))
        if package not in parents:unused.append({'path':rel,'bytes':size,'reason':'No hard, soft, management, native runtime, catalog or map dependency after replacing the legacy visitor template mesh.'})
result={'roots':sorted(roots),'parents':parents,'missing':sorted(missing),'files':all_files,'total_bytes':sum(all_files.values()),'unused_character_assets':unused}
(project/'Saved/AssetAudit.json').write_text(json.dumps(result,indent=2))
unreal.log('HF_ASSET_AUDIT '+json.dumps({'roots':len(roots),'reachable':len(parents),'missing':sorted(missing),'unused_character_assets':len(unused),'unused_MiB':round(sum(x['bytes'] for x in unused)/2**20,2)}))
