"""Keep reimport metadata valid after archiving sources outside cooked Content."""
import unreal
from pathlib import Path
project=Path(unreal.Paths.project_dir()).resolve()
count=0
for path in unreal.EditorAssetLibrary.list_assets('/Game',True,False):
    obj=unreal.load_asset(path)
    try:data=obj.get_editor_property('asset_import_data')
    except Exception:continue
    if not data:continue
    try:filenames=data.extract_filenames()
    except Exception:continue
    changed=False
    for index,filename in enumerate(filenames):
        source=Path(filename);target=None
        if source.suffix.lower()=='.fbx':
            if '/Assets/' in filename:
                suffix=filename.split('/Assets/',1)[1]
                candidate=project/'SourceArt/OriginalImports/Assets'/suffix
                if candidate.exists():target=candidate
            elif '/HumanFishing/Ocean/' in path:
                candidates=list((project/'SourceArt/Ocean').rglob(source.name))
                if len(candidates)==1:target=candidates[0]
        elif '/HumanFishing/Ocean/Kenney/' in path and source.suffix.lower()=='.png':
            candidate=project/'SourceArt/Ocean/Kenney/Textures'/source.name
            if candidate.exists():target=candidate
        elif '/HumanFishing/Ocean/Coral/' in path and source.suffix.lower()=='.glb':
            number=source.stem.replace('coral','')
            candidate=project/'SourceArt/Ocean/MiniPoly'/('CoralSet'+number+'.glb')
            if candidate.exists():target=candidate
        if target and source.resolve()!=target.resolve():
            data.scripted_add_filename(str(target),index,'');changed=True
    if changed:
        unreal.EditorAssetLibrary.save_loaded_asset(obj,only_if_is_dirty=False);count+=1
unreal.log('HF_REIMPORT_PATHS_UPDATED '+str(count))
