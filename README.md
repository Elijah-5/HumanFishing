# Human Fishing

A Blueprint project for Unreal Engine 5.2.1.

## Motion System Demo

The clip demonstrates camera-directed swimming, movement on the beach, and transitions between water and land. The fish's tail beat speeds up as it swims faster and slows down at rest and on land.

![Human Fishing motion system demo](docs/media/human-fishing-demo.gif)

## Run the project

Open `HumanFishing.uproject` and click **Play** in `L_ShorePrototype`. Use WASD to move and the mouse to look. While swimming, look up or down and press W to change depth. Press Space to jump on land.

| Asset | Purpose |
| --- | --- |
| `Content/HumanFishing/Maps/L_ShorePrototype` | Test map with a beach, a bounded swim area, and a graybox base |
| `Content/HumanFishing/Core/BP_HumanFishingGameMode` | Default game mode and pawn selection |
| `Content/HumanFishing/Player/BP_FishPlayer` | Player pawn with swimming, land movement, camera following, and swim animation speed control |
| `Content/Assets/Fish/A_FishSwim` | Looping fish tail animation |
| `Content/ThirdPerson/Input` | Enhanced Input assets used by the player |

The first gameplay milestone is moving the fish through the water area and delivering a placeholder catch to the base. Add fishing, NPC, and UI assets under `Content/HumanFishing` as those systems are implemented. Keep working copies of experimental maps separate from the shared prototype map.

Use regular Git for the project assets. Keep new assets under 20 MiB each and aim for a `Content` folder under 150 MiB for the first playable version. Keep builds and source art archives outside the repository. Check the distribution terms before adding third-party assets to this public repository.

Fish model: [Low poly fish](https://www.fab.com/listings/e6d7b903-fdea-4ce6-9d60-6ec09baaf78d) by Kultasusi.
