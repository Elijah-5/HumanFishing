from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()/'Scripts'
for name in ['import_ui_assets.py','rebalance_ocean.py']:
    exec(compile((root/name).read_text(),str(root/name),'exec'))
