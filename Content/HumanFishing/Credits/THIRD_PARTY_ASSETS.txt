# Ocean model credits

Only the models used in the game are imported. Reimport sources live in
`SourceArt/Ocean`, outside cooked `Content`. Full downloaded packs stay outside
the repository. Existing visitor and player-fish credits remain in the README.

| Author / pack | Selected assets | License | Source |
| --- | --- | --- | --- |
| Quaternius — Animated Fish Pack | Shark, Fish1, Fish3 and their Swim animations | [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) | [Author's pack](https://quaternius.com/packs/animatedfish.html), [Shark](https://poly.pizza/m/sZR8AMLMz5) |
| Kenney — Pirate Kit 2.1 | Ship wreck, rowboat, barrel, crate, chest, three rocks, sandy rocks, grass plant, wooden structure/platform/roof | [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) | [Author's distribution](https://opengameart.org/content/pirate-kit) |
| MiniPoly — Coral Reef Set | Tube coral color variants | [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) | [Model](https://poly.pizza/m/74GL45Fvdh) |
| MiniPoly — Coral Reef Set3 | Branching coral variants | [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/) | [Model](https://poly.pizza/m/UyswwdHFiL) |
| MiniPoly — Coral Reef Set5 | Reef coral variants | [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/) | [Model](https://poly.pizza/m/AKpeJtOxNO) |
| MiniPoly — Coral Reef Set4 | Reef coral variants | [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/) | [Model](https://poly.pizza/m/1mE2eYsGFD) |
| MiniPoly — Coral Reef Set2 | Reef coral variants | [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/) | [Model](https://poly.pizza/m/HdrPoAyuCJ) |
| MiniPoly — Coral Reef Set6 | Reef coral variants | [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/) | [Model](https://poly.pizza/m/hOBkCGKjlo) |

MiniPoly's [Coral Reef Kit](https://poly.pizza/bundle/Coral-Reef-Kit-ghN8EmbYa6)
combines CC0 and attribution-licensed models; do not label the entire kit CC0.
The author palette textures are retained. Changes in this project: UE unit and
axis conversion, placement and scaling, matte material settings, and removing
emission. Swimming models use the author's supplied animation, played at a speed
appropriate to movement. No endorsement by the authors is implied.

The shark derivative adds a `Jaw` bone, a separated mouth seam, dark palate
surfaces and small low-poly eyes. The original source remains unchanged;
`Shark_Jaw.blend` and `Shark_Jaw.fbx` are the editable/exported derivative.
`Scripts/rig_shark_jaw.py` reproduces it with Blender 4.5 LTS. Its supplied swim
cycle is retained and a native animation graph layers jaw rotation on top.

Kenney Pirate Kit supplies the rocks as well as wreck and trading props, replacing
the proposed Nature Kit import so that another whole pack is unnecessary. The
gameplay floor and invisible collision boundaries remain simple primitives;
the visible underwater landmarks and reef decoration use the authored models.

The attribution notice is also staged beside the game as
`Content/HumanFishing/Credits/THIRD_PARTY_ASSETS.txt` via the packaging settings.
