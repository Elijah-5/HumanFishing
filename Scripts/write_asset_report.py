#!/usr/bin/env python3
"""Summarize the recorded dependency audit and current working-tree sizes."""
import json,subprocess
from pathlib import Path
project=Path(__file__).resolve().parents[1]
cleanup=json.loads((project/'Saved/AssetCleanup.json').read_text())
audit=json.loads((project/'Saved/AssetAudit.json').read_text())
tree=subprocess.check_output(['git','ls-tree','-r','-l','HEAD','Content'],cwd=project,text=True)
baseline=sum(int(line.split('\t')[0].split()[-1]) for line in tree.splitlines())
size=lambda directory:sum(p.stat().st_size for p in directory.rglob('*') if p.is_file())
mib=lambda n:f'{n/2**20:.2f} MiB'
current=size(project/'Content');added=size(project/'Content/HumanFishing/Ocean')
removed=sum(x['bytes'] for x in cleanup['removed']);archived=sum(x['bytes'] for x in cleanup['archived_sources'])
text=f'''# Asset audit — 20 November 2026

This audit covers UE 5.2.1 on `main`. Sizes are logical file sizes, excluding
`Saved`, caches, builds and Git history.

| Measurement | Size |
| --- | ---: |
| Original Content at HEAD | {mib(baseline)} |
| After imports, before cleanup | {mib(cleanup['before_bytes'])} |
| Removed unreachable template packages | {mib(removed)} |
| FBX sources moved out of Content | {mib(archived)} |
| Current Content | {mib(current)} |
| New ocean runtime assets | {mib(added)} |
| Generated UI runtime assets | {mib(size(project/'Content/HumanFishing/UI/Theme'))} |
| Jellyshock runtime meshes and materials | {mib(size(project/'Content/HumanFishing/Equipment/Jellyshock'))} |
| Generated UI source art, outside Content | {mib(size(project/'SourceArt/UI'))} |
| Ocean reimport sources, outside Content | {mib(size(project/'SourceArt/Ocean'))} |
| Net reduction of Content | {mib(baseline-current)} |

## Dependency evidence

The Asset Registry traversal includes hard, soft and management package references.
Roots are the four maps, the catalog, project configuration and native `/Game/`
load paths extracted from plugin source. The recorded closure has
{len(audit['roots'])} roots and {len(audit['parents'])} reachable project packages;
missing project dependencies: **{len(audit['missing'])}**. Editor preview references
are retained by this traversal and were inspected before deleting anything.

The legacy `Characters/Mannequin_UE4/Meshes/Human1` Blueprint remains referenced by
the player. Its unused template skeletal component pointed at `SKM_Quinn`, which
pulled in the Manny/Quinn skeleton, post-process rigs, pose assets and large
textures. That default component now uses the existing `SK_BeachHuman` mesh.
The legacy actor class and its behavior are retained. A fresh traversal then
proved that **{len(cleanup['removed'])}** template packages were unreachable.

The [complete removal manifest](asset-removals.json) lists every package, its
size, and the reason for deletion. Removed packages were backed up under
`Saved/AssetRemovalBackups` first. Required visitor/player meshes, animations,
skeletons, materials and Enhanced Input assets remain.

Original FBX files from `Content/Assets` were moved to
`SourceArt/OriginalImports/Assets`; they were archived, not discarded. Import
metadata was repointed to the archived sources. Selected ocean source FBX/GLB
files and palette textures are in `SourceArt/Ocean`, with licenses. The cook
includes `/Game/HumanFishing/Ocean/Fauna` explicitly because shark meshes are
loaded by native runtime path. Credits are staged as NonUFS files.
The generated UI library is kept under `UI/Theme`, with source PNGs and exact
prompts outside Content. Its native runtime paths and explicit cook directory
are included in the dependency audit. Unused alternate action icons remain
available as part of the requested art set but are not displayed in the shop.

## Retained data

The user's macOS configuration and Retina resource files are retained. At the
user's later cleanup request, the old prototype project, UE 5.8 copy, old packaged
builds, plugin binary backups, removed-asset backups and duplicate staging output
were deleted. The current app is `Build/Mac-Walkthrough/HumanFishing.app`.
Original player saves were backed up under `Saved/WalkthroughBackups` before
increasing the two packaged-game slots to at least 1,000 seaweed for the demo.
Git history was not
rewritten, so existing historical Git packs do not shrink. No whole downloaded
model pack was added. Generated validation output remains under ignored `Saved`.

## Reproduce

```sh
python3 Scripts/run_editor_python.py Scripts/audit_assets.py
python3 Scripts/validate_project.py blueprints
python3 Scripts/validate_project.py cook
```

The final validation results and remaining manual checks are recorded in
[validation.md](validation.md). Asset provenance is in
[THIRD_PARTY_ASSETS.md](THIRD_PARTY_ASSETS.md).
'''
(project/'docs/asset-audit.md').write_text(text)
print('Content:',mib(current),'Net reduction:',mib(baseline-current))
