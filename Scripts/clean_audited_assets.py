#!/usr/bin/env python3
"""Remove only unreachable Characters packages from the saved engine audit.

Each file is copied to Saved/AssetRemovalBackups first. Git history and packaged
builds are untouched. Run audit_assets.py again and cook after any cleanup.
"""
import json,shutil
from pathlib import Path
project=Path(__file__).resolve().parents[1]
report=json.loads((project/'Saved/AssetAudit.json').read_text())
assert not report['missing'],'Resolve missing dependencies before cleanup'
removed=[]
for item in report['unused_character_assets']:
    relative=Path(item['path'])
    assert relative.parts[0]=='Characters' and '..' not in relative.parts
    source=project/'Content'/relative
    if source.exists():
        assert source.stat().st_size==item['bytes'],'Asset changed after audit: '+str(relative)
        backup=project/'Saved/AssetRemovalBackups'/relative
        backup.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,backup)
        source.unlink();removed.append(item)
# Import sources are not runtime packages and do not belong in Content.
archived=[]
for source in (project/'Content/Assets').rglob('*.fbx'):
    relative=source.relative_to(project/'Content')
    destination=project/'SourceArt/OriginalImports'/relative
    destination.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(source,destination)
    archived.append({'path':str(relative),'bytes':source.stat().st_size});source.unlink()
result={'before_bytes':report['total_bytes'],'removed':removed,'archived_sources':archived,
        'after_bytes':sum(p.stat().st_size for p in (project/'Content').rglob('*') if p.is_file()),'roots':report['roots']}
(project/'Saved/AssetCleanup.json').write_text(json.dumps(result,indent=2))
(project/'docs').mkdir(exist_ok=True)
(project/'docs/asset-removals.json').write_text(json.dumps(result,indent=2))
print('Removed',len(removed),'unreachable packages;',round(sum(x['bytes'] for x in removed)/2**20,2),'MiB')
print('Content after:',round(result['after_bytes']/2**20,2),'MiB')
