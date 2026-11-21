"""Import generated UI art without changing the source PNGs or their alpha."""
import unreal
from pathlib import Path
project=Path(unreal.Paths.project_dir()).resolve()
destination='/Game/HumanFishing/UI/Theme'
for source in sorted((project/'SourceArt/UI').glob('T_UI_*.png')):
    task=unreal.AssetImportTask();task.filename=str(source);task.destination_path=destination;task.destination_name=source.stem
    task.automated=True;task.replace_existing=True;task.save=True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture=unreal.load_asset(destination+'/'+source.stem)
    assert isinstance(texture,unreal.Texture2D),source
    texture.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('mip_gen_settings',unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('max_texture_size',1024 if 'ShopPanel' in source.stem else 512 if 'Slot' in source.stem else 256)
    texture.set_editor_property('srgb',True)
    unreal.EditorAssetLibrary.save_loaded_asset(texture,only_if_is_dirty=False)
hud=unreal.load_asset('/Game/HumanFishing/UI/WBP_PlayerHUD')
assert unreal.HumanFishingEditorLibrary.hide_legacy_hud(hud)
unreal.EditorAssetLibrary.save_loaded_asset(hud)
unreal.log('HF_UI_ART_IMPORTED')
