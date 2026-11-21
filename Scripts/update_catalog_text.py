import unreal,shutil
from pathlib import Path
project=Path(unreal.Paths.project_dir()).resolve()
path='/Game/HumanFishing/UI/Backpack/DA_HumanFishingCatalog'
catalog=unreal.load_asset(path);types=catalog.get_editor_property('types');changed=False
for key,value in types.items():
    for prop in ['field_notes','capture_tips']:
        original=str(value.get_editor_property(prop))
        text=original.replace('Only bites on the $100 Bill from the Seaweed Exchange.',
                              'Accepts $100 Bill or Flash Bait from the Seaweed Exchange. Flash is the strongest bait; Dollar Bait is refused.')
        if text!=original:value.set_editor_property(prop,text);changed=True
    types[key]=value
if changed:
    backup=project/'Saved/UpdateBackups/DA_HumanFishingCatalog.uasset'
    if not backup.exists():shutil.copy2(project/'Content/HumanFishing/UI/Backpack/DA_HumanFishingCatalog.uasset',backup)
    catalog.set_editor_property('types',types);unreal.EditorAssetLibrary.save_loaded_asset(catalog)
unreal.log('HF_CATALOG_TEXT_UPDATED '+str(changed))
