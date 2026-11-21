# Asset audit — 20 November 2026

This audit covers UE 5.2.1 on `main`. Sizes are logical file sizes, excluding
`Saved`, caches, builds and Git history.

| Measurement | Size |
| --- | ---: |
| Original Content at HEAD | 539.93 MiB |
| After imports, before cleanup | 544.08 MiB |
| Removed unreachable template packages | 380.86 MiB |
| FBX sources moved out of Content | 38.73 MiB |
| Current Content | 136.12 MiB |
| New ocean runtime assets | 4.23 MiB |
| Generated UI runtime assets | 11.39 MiB |
| Jellyshock runtime meshes and materials | 0.13 MiB |
| Generated UI source art, outside Content | 11.62 MiB |
| Ocean reimport sources, outside Content | 3.24 MiB |
| Net reduction of Content | 403.81 MiB |

## Dependency evidence

The Asset Registry traversal includes hard, soft and management package references.
Roots are the four maps, the catalog, project configuration and native `/Game/`
load paths extracted from plugin source. The recorded closure has
34 roots and 414 reachable project packages;
missing project dependencies: **0**. Editor preview references
are retained by this traversal and were inspected before deleting anything.

The legacy `Characters/Mannequin_UE4/Meshes/Human1` Blueprint remains referenced by
the player. Its unused template skeletal component pointed at `SKM_Quinn`, which
pulled in the Manny/Quinn skeleton, post-process rigs, pose assets and large
textures. That default component now uses the existing `SK_BeachHuman` mesh.
The legacy actor class and its behavior are retained. A fresh traversal then
proved that **105** template packages were unreachable.

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
