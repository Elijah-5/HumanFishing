from pathlib import Path
import json,shutil
project=Path(__file__).resolve().parents[1]
destination=project/'SourceArt/UI';destination.mkdir(parents=True,exist_ok=True)
for entry in json.loads((project/'Saved/UIGeneration.json').read_text()):
    source=Path(entry['source']);assert source.is_file(),source
    shutil.copy2(source,destination/('T_UI_'+entry['asset']+'.png'))
print('Copied generated UI images into SourceArt/UI')
