# Manual Config QA Checklist

- Ball speed extremes: create/load configs with speed 1 and 10; launch and observe noticeably slow vs fast ball.
- Seeds: use -1 (time-based) and a fixed seed (e.g., 1234); confirm fixed seed gives repeatable runs after restart, while -1 varies.
- Starting level: set a level >1 and verify the game starts there; set an out-of-range level and confirm it falls back to level 1.
- Save/load round-trip: save a config, quit the app, relaunch, load it, and check speed/seed/level values and HUD config name.