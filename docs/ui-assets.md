# Generated ocean UI art

Generated with the built-in image generation tool for this project. The source
PNGs retain their original alpha and are stored in `SourceArt/UI`. The complete
prompt set is recorded in [prompts.json](../SourceArt/UI/prompts.json).

The set uses navy, seafoam, sand and brass colors, with broad polygonal facets
and clear silhouettes. Text is drawn by the game, not baked into the artwork.

| Source PNG | Use |
| --- | --- |
| `T_UI_Seaweed.png` | Currency in the HUD, shop, backpack and locked-entry cost |
| `T_UI_CoveKey.png` | Scenic Cove key product |
| `T_UI_Purchase.png` | Available source art; omitted from price buttons to avoid clutter |
| `T_UI_SellOne.png` | Available source art; the shop uses a clear Sell selected label |
| `T_UI_SellAll.png` | Available source art; the shop uses a prominent Sell all label |
| `T_UI_Upgrade.png` | Available source art; owned products use a plain Owned label |
| `T_UI_Locked.png` | Locked entrance requirement |
| `T_UI_Backpack.png` | Backpack Expansion product |
| `T_UI_Slot.png` | Framed shop product icons; backpack cells retain their plain style |
| `T_UI_ShopPanel.png` | Quiet ocean-themed shop backdrop |
| `T_UI_Jellyshock.png` | Jellyshock Capsule shop product and underwater stock counter |
| `T_UI_TidalFins.png` | Tidal Fins permanent sprint upgrade |
| `T_UI_VioletJellyshock.png` | Premium violet jellyfish, shop product and G stock counter |

Imported textures live under `Content/HumanFishing/UI/Theme`. Icons are limited
to 256 pixels at runtime, the slot to 512 and the panel to 1024. UI textures use
the UI group, preserve alpha, and do not generate mipmaps. The slot's transparent
padding is excluded through brush UVs; source PNGs are unchanged. Runtime brushes
keep strong GC references and are included explicitly in the cook settings.
All standalone icons use ScaleToFit with the source texture's aspect ratio;
they are never stretched to fill square or rectangular slots. The lock appears
only at a locked sea-area entrance. Bait has a unit price and quantity-based total;
permanent products show a single purchase price.

## Aspect-ratio review

All active icon paths were reviewed with the Jellyshock presentation update.
Shop portraits, generated item art and the square product frame share the
`FitImage`/`Icon` ScaleToFit wrapper. Detail arrows also have a square fit box.
HUD bait portraits, Field Guide cards/details and fishing readouts use ScaleToFit;
the backpack's custom painter calculates one uniform scale from the source
dimensions and centers the result. Currency and locked-entry icons use the same
shared wrapper. Flat color bars and panel backgrounds are layout surfaces, not
item icons. The hidden legacy HUD is not displayed by the game.

## Jellyshock world model

The bottle illustration represents the purchased container. Its released
jellyfish is an authored 1,066-triangle mesh set with translucent, unlit teal
materials and a warm lightning core. Editable source and mesh budgets live in
`SourceArt/Equipment/Jellyshock`; `Scripts/create_jellyshock_model.py` reproduces
the source in Blender 4.5 and `Scripts/import_jellyshock.py` imports the UE assets.
The runtime meshes and six materials live in
`Content/HumanFishing/Equipment/Jellyshock` and are explicitly cooked.
Tentacles sway independently, the bell contracts, and the short electric wake
and impact burst use instanced meshes. Impact visuals have a four-burst cap and
expire automatically; they do not add dynamic lights or change the Mac preset.
The violet variant reuses the geometry with separate purple material instances,
a larger bell and a three-ring nova. Its generated icon has the same bottle
proportions and alpha as the original. The built-in generation prompt is also
recorded in `SourceArt/UI/violet-jellyshock-prompt.json`.

To reimport after changing the source artwork:

```sh
python3 Scripts/run_editor_python.py Scripts/import_ui_assets.py
```

The built-in generator's original files remain in the Codex generated-image
directory. Project references use only the workspace copies.
