# Mac display and performance

UE 5.2's Mac fullscreen resize path uses `NSScreen.visibleFrame` for the Slate
resize notification. Native fullscreen content on a notched Retina screen can
differ from that rectangle. The previous packaged log also recorded Metal
drawable/viewport size mismatches during native fullscreen resizing.

The game-instance display subsystem observes the game NSWindow's resize, move,
fullscreen, screen and backing-scale notifications. Once the content rectangle
has settled for two samples, it reads the content view in backing pixels and
uses UE's Cocoa-to-Slate screen conversion for its origin. In native fullscreen
it updates the cached Slate geometry; in both modes it invalidates layout and
resizes the scene viewport to the real view size. The observer is removed on
shutdown. Editor windows are excluded.

Retina remains enabled. UI stays at the native viewport resolution; only the 3D
scene uses a 75% render scale. No per-widget mouse coordinate offsets are used.

The first-run Mac preset is versioned in GameUserSettings. It sets a 60 FPS cap,
VSync, medium TAA/textures/view distance and lighter shadows/postprocessing.
It does not replace resolution, window preferences, audio settings or saves.
Lumen, virtual shadows, mesh distance fields, motion blur, depth of field,
volumetric fog and SSR are disabled for this stylized game. Existing direct
lights, sky lighting and ordinary depth fog remain.

Verification results, including actual native fullscreen pointer checks and
the short before/after performance capture, are recorded in `validation.md`.
