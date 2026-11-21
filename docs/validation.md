# Validation — 20 November 2026

Environment: Unreal Engine 5.2.1, Apple Silicon macOS, installed Xcode 26.6,
with the scoped 15.4 build-version shim described in the README.

| Check | Result |
| --- | --- |
| Final runtime and editor plugin compilation | Passed |
| Equipment focused checks | 6 / 6 passed: purchases, sprint, v2 migration, temporary shark stun, aim cone/prediction, violet stock and v3 migration |
| Final focused `HumanFishing.Shop.BackpackUpgrade` check | Passed |
| Packaged equipment smoke, 1280×720 | Passed: shop, expansion, swimming/jaw animation, normal stun/recovery, blocked launch without spending, point-blank aiming and violet defeat with separate stock |
| Packaged Retina-sized layout, 3456×2168 | Passed: same short equipment flow; shop portraits, currency and both jellyfish icons inspected for uniform proportions |
| Walkthrough Development app build and cook | Passed: 0 cook errors, 0 warnings; archived Mac app |
| Packaged app startup | Passed: isolated profile, NullRHI startup and clean exit; Retina capability enabled |
| Earlier `HumanFishing.*` automation | 10 / 10 passed before the final backpack/UI refinements |
| Earlier compile of all Blueprints | 0 errors, 0 warnings |
| Earlier Mac cook of all four maps | 0 errors, 0 warnings |
| Asset Registry after template removal | 0 missing project dependencies |
| Ocean scene captures | Inspected; corrected prop rotation and overexposure |
| Isolated Standalone flow | Passed: 19 stages, including purchase, refund, consumption, key, Cove round trip and continue |
| Native Retina pointer check | Passed with the user's physical mouse: green-button native fullscreen, quantity controls, purchases and details; 3456×2168 backing view with origin 0,66 |

At the user's request, final verification was limited to plugin compilation,
focused equipment checks and one short equipment smoke run. A later request for a
walkthrough build reran the necessary cook/package and one simple packaged startup;
the full travel flow was not repeated. The expansion test covers
purchase cost, repeat-purchase rejection, the fifth column, preserving existing
positions, load fraction, and in-memory save serialization/validation.

Automation covers stock, affordability, quantity and overflow bounds, zero-stock
cast rejection, reserve/retrieve/commit/failure and repeated settlement, unlimited
Dollar bait, merchant acceptance of Flash, Flash bonuses, key repeat purchase,
v1 migration and one-time compensation, existing shop redemption/upgrades, and
shark state/load/theft rules.

`validate_project.py standalone` uses a disposable profile under `/private/tmp`.
It tests capture registration and redemption, purchases, the real fishing
inventory interfaces, key gating, map travel, Arrival spawn selection, saving and
continuing. It captures the native game UI at 1280×720 and 1920×1080. The fixture
does not replace manual assessment of visitor aiming/reeling feel.

On this Xcode/macOS combination, UE's startup math self-check logs
`VectorRound32-FloatMax`. This precedes the project suite; all ten earlier named project
tests and the final focused expansion test pass. The earlier Blueprint/cook commandlets finished successfully. Logs retain
this engine diagnostic instead of suppressing it.

Generated reports, logs and images: `Saved/Validation` (ignored by Git). No player
profile, old packaged build or Git history was modified by validation.

Final quick checks can be reproduced with:

```sh
python3 Scripts/build_macos.py
python3 Scripts/validate_project.py tests --filter HumanFishing.Shop.BackpackUpgrade
python3 Scripts/validate_project.py tests --filter HumanFishing.Equipment
python3 Scripts/validate_project.py smoke
```

## Short Mac performance capture

Two packaged-game captures used the same initial Deep Ocean view, a 1280×720
window, no audio and separate disposable profiles on the M1 Max. Each recorded
600 frames; the first 120 were discarded. These are short scene samples, not
a guarantee for every map or fullscreen resolution.

| Timing, milliseconds | Previous mean / p95 | Updated mean / p95 |
| --- | ---: | ---: |
| GPU | 12.99 / 13.34 | 4.78 / 4.94 |
| Render thread | 2.78 / 2.99 | 1.46 / 1.56 |
| RHI thread | 4.75 / 5.06 | 3.23 / 3.46 |
| Game thread | 1.00 / 1.07 | 1.01 / 1.11 |
| Frame interval | 13.67 / 14.11 | 16.67 / 16.79 |

The updated frame interval reflects the intentional 60 FPS cap. GPU work fell
about 63% with the lighter effects and 75% scene render scale; UI remains native
resolution. Raw summaries are `Saved/Validation/performance-before.json` and
`performance-after.json`. Reproduce a current sample with
`python3 Scripts/benchmark_macos.py --label after` while other game/editor windows
are closed.

The smoke run uses the same disposable profile and stops after its short shop/equipment check;
it does not replay the 19-stage end-to-end fixture. Manual follow-up is left to
the user: fishing/reeling feel, travel-zone placement, scene brightness, shop
quantity controls and selling, and other resolutions/displays. The native
fullscreen purchase/quantity/details check was completed on this Mac. Automated
coordinate clicks retained an old system-cursor position in UE; a temporary input
probe identified that automation limitation, and the user confirmed physical
mouse clicks worked. The probe was removed after verification.

The Jellyshock presentation update uses the cooked app for visual checks so
editor-only shader compilation does not delay inspection. Captures include
`JellyshockModel1280`, `JellyshockBurst1280`, `VioletJellyshockModel1280`,
`VioletJellyshockBurst1280`, and corresponding `3456` captures under
`Saved/Validation/Images`. Normal impacts have two soft-edged pressure rings,
eight arcs and eighteen bubbles; violet hits remove the first shark and use
three rings, sixteen arcs and twenty-eight bubbles. Four simultaneous bursts
are allowed. The brief opaque flash is hidden before it can become a dark ball.

Current reproduction commands:

```sh
python3 Scripts/validate_project.py tests --filter HumanFishing.Equipment
python3 Scripts/package_macos.py
python3 Scripts/validate_project.py smoke --packaged
python3 Scripts/validate_project.py smoke --packaged --width 3456 --height 2168
```

README demos use `smoke --packaged --readme-capture` to record actual 1280×720
gameplay frames. The short fixture removes the map's other sharks before spawning
its target so a passing patrol cannot intercept the test shot. The optional capture
mode writes PNG sequences under `Saved/Validation/ReadmeFrames`; the macOS ImageIO
encoder preserves frame proportions and capture timing. See the README for commands.
