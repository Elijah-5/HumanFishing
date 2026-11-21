import unreal
from pathlib import Path

blueprint=unreal.load_asset('/Game/HumanFishing/Fishing/BP_ShoreFishing')
result=unreal.HumanFishingEditorLibrary.describe_blueprint(blueprint)
Path('/private/tmp/humanfishing-fishing-graphs.json').write_text(result)
unreal.log('HF Graph inspection written to /private/tmp/humanfishing-fishing-graphs.json')
