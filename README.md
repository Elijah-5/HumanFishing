# Human Fishing

A Blueprint project for Unreal Engine 5.2.1.

## Motion System Demo

The clip demonstrates camera-directed swimming, movement on the beach, and transitions between water and land. The fish's tail beat speeds up as it swims faster and slows down at rest and on land.

![Human Fishing motion system demo](docs/media/human-fishing-demo.gif)

## Run the project

Open `HumanFishing.uproject` and click **Play** in `L_ShorePrototype`. Use WASD to move and the mouse to look. While swimming, look up or down and press W to change depth. Press Space to jump on land.

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

After a dollar lands on reachable sand, the tourist builds interest while within
6 meters of it. The gold ring fills faster at close range. When it fills, the
tourist walks to the bait. The ring drains once they arrive. If it empties,
retrieve the bait with **X** and cast again to get their attention.

Press **E** at the hook prompt, release it, then hold **E** to reel. The blue ring
shows line durability and turns red at 25%. Releasing **E** pauses the pull while
the tourist keeps wearing down the line. The basic line lasts 12.5 seconds from
the moment you hook the tourist. Press **X** to end a cast. When you fish from
the water, the tourist stops at the dry edge of the beach. A catch fills the
first available space in the backpack and adds one to `Captures`. The tourist
returns to patrol if the backpack has no room for the catch.

Press **Tab** to pause and open the 4×4 backpack. A beach tourist occupies one
column and two rows. Drag catches to rearrange them. Press **Tab** or **Esc** to
close the backpack. Hold **E** again to resume reeling. The backpack and HP
carry between shore and deep ocean during the session.

Tune `DollarAppeal` and `MaxLineDurability` in `BP_ShoreFishing` Class Defaults.
`BP_BeachHuman` defines `MoneyPreference`, `InterestDecayPerSecond`, and
`LineWearPerSecond` for the tourist.

The `HumanFishingGameplay` project plugin provides the backpack and health UI.
On a fresh checkout, build the plugin with the UE 5.2 C++ toolchain for your
platform when Unreal prompts to rebuild the project modules. Catch sizes and
portraits are configured in `DA_HumanFishingCatalog`.

## Deep ocean travel

From the shoreline, turn away from the beach and continue swimming offshore. Crossing
the offshore boundary loads `L_DeepOcean` automatically. In the ocean, turn
back toward the edge you arrived from and swim to that boundary to return to shore.
There is no interaction button. Only actors of the fish player Blueprint activate travel.

The deep-ocean blockout has a larger swim area, an 18-meter-deep seabed, rocks, water
surface, and blue haze. Movement uses the existing fish Blueprint and the same controls.
The spawn points sit outside the travel zones to prevent immediate back-and-forth loads.

## Player counters and HUD

The upper-left HUD shows Captures and Seaweed from `BP_HumanFishingGameInstance`.

To access them in a Blueprint: Get Game Instance -> Cast to BP_HumanFishingGameInstance
-> Get/Set Captures or Seaweed.

| Asset | Purpose |
| --- | --- |
| `Content/HumanFishing/Maps/L_ShorePrototype` | Test map with a beach, a bounded swim area, and a graybox base |
| `Content/HumanFishing/Maps/L_DeepOcean` | Offshore swimming blockout with a return route |
| `Content/HumanFishing/Travel/BP_LevelTravelZone` | Fish-triggered level travel with an editable DestinationMap |
| `Content/HumanFishing/Core/BP_HumanFishingGameInstance` | Session counters for Captures and Seaweed |
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
