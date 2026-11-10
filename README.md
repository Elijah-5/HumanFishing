# Human Fishing

A Blueprint project for Unreal Engine 5.2.1.

## Motion System Demo

The clip demonstrates camera-directed swimming, movement on the beach, and transitions between water and land. The fish's tail beat speeds up as it swims faster and slows down at rest and on land.

![Human Fishing motion system demo](docs/media/human-fishing-demo.gif)

## Run the project

Open `HumanFishing.uproject`. The standalone game opens the three-slot save menu. In the editor, play `L_SaveMenu` to use a save slot or play `L_ShorePrototype` directly for a temporary test session. Use WASD to move and the mouse to look. While swimming, look up or down and press W to change depth. Press Space to jump on land.

## Catching humans

Swim up to the surface and aim toward the beach. **Left-click** to cast along
the current aim, or press **E** to preview the trajectory first. Turn the camera
left or right to steer, look down to throw closer, and look up to throw farther.
When the landing ring is green, click or press **E** to cast. Press **Esc** to
cancel aiming. Test **Esc** in **Standalone Game**, since it stops Play in
Editor by default.

The fish has 100 HP. Landing on the beach drains 10 HP per second, including
while jumping. Returning to the shallows stops the damage. At zero HP, the cast
ends and the fish respawns in the deep ocean, where it recovers 20 HP per second.

Every 10–16 seconds a visitor can enter from either end of the 18×30 meter
beach, up to six at a time. Visitors walk inland, wander and pause, then walk
out. Once a dollar lands, visitors staying within 6 meters can build interest.
The gold ring shows the most interested visitor. The first visitor to fill
the ring walks to the bait, then their interest starts draining.
Interest builds at its fastest within one meter and slows gradually out to
six meters. A visitor who notices bait from farther away may observe it longer.

If a visitor loses interest or cannot reach the bait before being hooked, the
same dollar can attract another visitor. Press **E** to retrieve it while
waiting or while a visitor is approaching. At the hook prompt, **E** hooks and
**X** retrieves. Bait stays out while waiting for another visitor.

Press **E** at the hook prompt, release it, then hold **E** to reel. The blue ring
shows line durability and turns red at 25%. Releasing **E** pauses the pull while
the tourist keeps wearing down the line. The basic line lasts 12.5 seconds from
the moment you hook the tourist. Press **X** to end a cast. When you fish from
the water, the tourist stops at the dry edge of the beach. A catch fills the
first available space in the backpack and adds one to `Captures`. The tourist
returns to patrol if the backpack has no room for the catch.

When a worker braces, release **E** to avoid extra line wear during the next
tug. To catch a diver, cast from the water and reel them past the shore until
they touch the fish.

Press **Tab** to pause and open the 4×4 backpack. A beach tourist occupies one
column and two rows. Click a catch to see its name, rarity, size, and total
seaweed value. To free its cells, click **Discard...**, then **Confirm
discard**. Drag catches to rearrange them. Press **Tab** or **Esc** to close the
backpack. Hold **E** again to resume reeling.

Switch to **Field Guide** from the pause panel to read field notes and capture
tips. A successful first catch reveals its entry even after you discard it.
**Save & Return** returns to the slot menu. Each slot keeps its own backpack,
guide, HP, capture count and seaweed. Continuing starts at the last area's
safe spawn point.

Tune `DollarAppeal` and `MaxLineDurability` in `BP_ShoreFishing` Class Defaults.
Visitor profiles live in `DA_HumanFishingCatalog`. Rarity affects spawn odds,
how long a visitor stays, and the catch's seaweed value per occupied cell.
The catalog's `GetCatchSeaweedValue` function reports the total value.
Footprints below are width by height, and interest drain and line wear are
measured per second.

| Visitor | Rarity | Footprint | Seaweed value | Dollar interest | Interest drain | Line wear |
| --- | --- | --- | --- | --- | --- | --- |
| Beach tourist | White | 1×2 | 2 | 1.25× | 8 | 8 |
| Casual woman | White | 2×1 | 2 | 1.05× | 11 | 6 |
| Farmer | Green | 2×2 | 8 | 0.85× | 6 | 10 |
| Lifeguard | Green | 1×3 | 6 | 1.15× | 10 | 7 |
| Worker | Blue | 2×3 | 18 | 0.90× | 9 | 15 |
| Delivery rider | Blue | 2×2 | 12 | 1.45× | 18 | 11 |
| Businessman | Purple | 1×3 | 15 | 1.70× | 20 | 17 |
| Surf instructor | Purple | 3×1 | 15 | 0.85× | 18 | 18 |
| Diver | Gold | 2×3 | 48 | 0.65× | 25 | 22 |

The `HumanFishingGameplay` project plugin provides the backpack and health UI.
On a fresh checkout, build the plugin with the UE 5.2 C++ toolchain for your
platform when Unreal prompts to rebuild the project modules. Catch sizes and
portraits are configured in `DA_HumanFishingCatalog`.

## Beach developer mode

Start Play in `L_ShorePrototype` in the editor or a Development build and
open the UE console with the backtick key (below Esc on a US keyboard).

| Command | Effect |
| --- | --- |
| `hf.dev on` | Start a temporary test, reveal the full Field Guide and pause random arrivals. |
| `hf.dev list` | List the visitor IDs in the current catalog. |
| `hf.dev spawn Diver` | Spawn a specific visitor on the beach near the fish. Replace `Diver` with any listed ID. |
| `hf.dev off` | Restore progress and the area from before the test. |

Manual spawns bypass rarity, unlocks, and the crowd cap. They use the normal
visitor AI and can be caught. Testing can continue across both areas. Returning
to the menu or quitting discards the temporary test progress.

## Deep ocean travel

From the shoreline, turn away from the beach and continue swimming offshore. Crossing
the offshore boundary loads `L_DeepOcean` automatically. In the ocean, turn
back toward the edge you arrived from and swim to that boundary to return to shore.
There is no interaction button. Only actors of the fish player Blueprint activate travel.

The deep-ocean blockout has a larger swim area, an 18-meter-deep seabed, rocks, water
surface, and blue haze. Movement uses the existing fish Blueprint and the same controls.
The spawn points sit outside the travel zones to prevent immediate back-and-forth loads.

## Player counters and HUD

The upper-left HUD reads the inventory subsystem through `GetCaptureCount` and
`GetSeaweedBalance`. To change the balance, call `SetSeaweed` on the
`HumanFishingInventory` game-instance subsystem so the active slot autosaves.

| Asset | Purpose |
| --- | --- |
| `Content/HumanFishing/Maps/L_ShorePrototype` | Test map with a beach, a bounded swim area, and a graybox base |
| `Content/HumanFishing/Maps/L_DeepOcean` | Offshore swimming blockout with a return route |
| `Content/HumanFishing/Travel/BP_LevelTravelZone` | Fish-triggered level travel with an editable DestinationMap |
| `Content/HumanFishing/Maps/L_SaveMenu` | Start or continue one of three save slots |
| `Content/HumanFishing/Core/BP_HumanFishingGameInstance` | Game-instance Blueprint used by the project |
| `Content/HumanFishing/UI/WBP_PlayerHUD` | Bound capture/currency display |
| `Content/HumanFishing/Core/BP_HumanFishingGameMode` | Default game mode and pawn selection |
| `Content/HumanFishing/Player/BP_FishPlayer` | Player pawn with swimming, land movement, camera following, and swim animation speed control |
| `Content/Assets/Fish/A_FishSwim` | Looping fish tail animation |
| `Content/ThirdPerson/Input` | Enhanced Input assets used by the player |

The first gameplay milestone is moving the fish through the water area and delivering a placeholder catch to the base. Add fishing, NPC, and UI assets under `Content/HumanFishing` as those systems are implemented. Keep working copies of experimental maps separate from the shared prototype map.

Use regular Git for the project assets. Keep new assets under 20 MiB each and aim for a `Content` folder under 150 MiB for the first playable version. Keep builds and source art archives outside the repository. Check the distribution terms before adding third-party assets to this public repository.

Fish model: [Low poly fish](https://www.fab.com/listings/e6d7b903-fdea-4ce6-9d60-6ec09baaf78d) by Kultasusi.

Shore tourist: [Beach Character](https://poly.pizza/m/DojKLcO34E) by Quaternius, [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/).

Bait: [Dollar](https://poly.pizza/m/YmaCrICQe2) by J-Toastie, [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/). The project imports the original mesh and embedded texture, uses a two-sided material, and scales the bait in the level.
