import json,unreal
from pathlib import Path
bp=unreal.load_asset('/Game/HumanFishing/Player/BP_FishPlayer')
graphs=json.loads(unreal.HumanFishingEditorLibrary.describe_blueprint(bp))
(Path(unreal.Paths.project_saved_dir())/'FishMovementGraph.json').write_text(json.dumps(graphs,indent=2))
for graph in graphs:
    if any('Max Fly Speed' in n['title'] or 'MaxFlySpeed' in str(n) for n in graph['nodes']):
        unreal.log('HF_MOVEMENT_GRAPH '+json.dumps(graph))
