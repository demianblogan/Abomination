# Roadmap

A living draft. It is reviewed after every milestone release and may change
at any time. Its main purpose is to answer two questions quickly: *where are
we now* and *what comes next*.

## Principles

- **Every milestone adds something to play and something to see.** Constant
  gameplay and visual feedback keeps development motivating.
- **Learning OpenGL comes first.** Rendering features are introduced one
  concept at a time and explained in depth.
- **Architecture grows with need.** No speculative systems (YAGNI), but
  refactoring is done as soon as a need appears.
- **Versions:** `0.1` … `0.9`, then `1.0` release — see
  [GIT_CONVENTIONS.md](GIT_CONVENTIONS.md#7-versions-and-releases).

**Status legend:** ✅ done · 🔨 in progress · ⏳ planned

## Overview

| Version | Name                   | Status | Gameplay                                              | Visual / OpenGL                                          |
|---------|------------------------|--------|-------------------------------------------------------|----------------------------------------------------------|
| 0.1     | Foundation             | ✅     | Free-fly (noclip) camera                              | Window, OpenGL 4.6 context, debug output, textured cube, ImGui overlay |
| 0.2     | First Steps            | ✅     | Quake-style movement, collision with the level        | EnTT, resource manager, TrenchBroom map loading, brush texturing |
| 0.3     | Boomstick              | 🔨     | First hitscan weapon, damage, sound                   | View model with sway/bob/recoil, muzzle flash, particles, decals, glTF loading |
| 0.4     | It Moves               | ⏳     | First enemy: AI, navmesh, health, death, HUD          | Skeletal animation, text rendering                       |
| 0.5     | Lights                 | ⏳     | Glowing projectiles, dynamic light in combat          | Lightmap baking, shadows, HDR, bloom, gamma              |
| 0.6     | Game Loop              | ⏳     | Pickups, armor, doors, buttons, plates, level exit, stats screen, level transitions — first complete level | Moving brushes, data-driven configs |
| 0.7     | Arsenal & Bestiary     | ⏳     | All weapons, projectiles, explosions, weapon switching (keys, wheel, mouse wheel), gamepad, new enemy types (incl. flying), first boss | Weapon effects, new models |
| 0.8     | Menus & Saves          | ⏳     | Autosave, manual save, quicksave, load menu           | Animated main menu, pause menu, all options, key rebinding, 5 languages, logo screen, language selection |
| 0.9     | Content Complete (Beta)| ⏳     | All 4 episodes, bosses, story texts, achievements, DualSense features | Episode palettes, polish, optimization |
| 1.0     | Release                | ⏳     | Balance, bug fixes                                    | —                                                        |

## 0.1 — Foundation ✅

**Goal:** a professional project skeleton and the first 3D image on screen
that you can fly around.

| # | Branch                          | Status | Content                                                                 |
|---|---------------------------------|--------|-------------------------------------------------------------------------|
| 1 | `docs/project-foundation`       | ✅     | Roadmap, code style, git conventions, architecture draft, assets list, PR template, clang-format, editorconfig |
| 2 | `build/cmake-vcpkg-setup`       | ✅     | CMake, presets, vcpkg manifest, folder structure, empty executable, GoogleTest, CI on GitHub Actions |
| 3 | `feat/logging`                  | ✅     | spdlog logging: levels, per-module categories, console and log file     |
| 4 | `feat/window-gl-context`        | ✅     | SDL3 window, OpenGL 4.6 Core context, GLAD 2 loader, debug output, frame timer, animated clear color |
| 5 | `feat/debug-overlay`            | ✅     | Dear ImGui overlay: version, GPU, FPS, frame time graph; keyboard state (`Input` module), F1 toggle |
| 6 | `feat/textured-cube`            | ✅     | Assets folder copied next to the executable, JetBrains Mono for the overlay; shaders, buffers, vertex array, texture with DSA and RAII wrappers; rotating textured cube with MVP matrices, depth test and face culling |
| 7 | `feat/fly-camera`               | ✅     | Mouse state, input actions and bindings, `Renderer::Camera` (yaw, pitch, perspective) with math tests, free-fly camera: WASD, Q/E, Shift, mouse look with the right button; dark gray background |

**Done when:** the game opens a window, shows a textured cube that can be
examined with a free-fly camera, the debug overlay shows FPS, CI builds and
tests every PR.

![0.1 Foundation: a rotating textured cube with the debug overlay](Screenshots/v0.1.0/RotatingCube.png)

## 0.2 — First Steps ✅

**Goal:** load a level built in TrenchBroom and run and jump around it with
Quake-style movement.

Every branch ends with something visible in the game. Systems that cannot be
seen (collision, map geometry) get debug visualizations in the overlay.

| # | Branch                          | Status | Content                                                                 |
|---|---------------------------------|--------|-------------------------------------------------------------------------|
| 1 | `feat/fixed-timestep`           | ✅     | Debug menu bar (View, Settings > Display), V-Sync toggle, FPS limit; fixed 60 Hz simulation ticks with interpolation; main loop split into `Update`, `FixedUpdate`, `Render` |
| 2 | `feat/asset-manager`            | ✅     | Typed asset handles and a generic cache, texture and shader stores with magenta fallbacks, `RenderAssets`; Assets window in the overlay, debug windows remember their positions; crate texture on the cube |
| 3 | `feat/ecs-scene`                | ✅     | EnTT 4.0.0; `Mesh` asset and mesh store; crates as entities (`Transform`, `MeshRenderer`, `Spin`), render system; interpolation as a system for every moving entity; the camera as an entity, the renderer gets a `View`; entity inspector with module colors; `DemoScene` removed |
| 4 | `feat/debug-ui-scaling`         | ✅     | Debug overlay follows the display scale of Windows (DPI) for 4K monitors, plus a manual UI scale in Settings > Display |
| 5 | `feat/map-geometry`             | ✅     | TrenchBroom game configuration and a test map; `.map` parser (Valve 220); brushes → polygons by clipping with planes; the level as one mesh and one entity, Z-up → Y-up, camera at the player start; solid shaded and wireframe render modes; Renderer window with frame and level statistics; demo crates moved into the test room |
| 6 | `feat/screen-mode`              | ✅     | Escape quits the game; key combinations in input bindings; windowed size calculated from the monitor (75% of the usable area, 16:9) instead of a fixed size; screen modes Windowed, Borderless (default) and exclusive Fullscreen in Settings > Display and with Alt+Enter |
| 7 | `feat/brush-textures`           | ✅     | Art direction (4 episodes, palettes, texture rules) and a texture generator; Episode 1 wall, floor, planks and crate textures; texture coordinates from the Valve 220 axes; the level drawn in one part per texture with texture and direction shading; crates as brushes of the test map, demo crates removed; asset lifetime groups (Global, Level), level unloading and a Reload button |
| 8 | `feat/collision`                | ✅     | `World::Level` class; collision brushes (planes, bounding boxes, bevel planes); box trace through the brushes (Quake 2 style); debug lines (wide, depth-tested or on top, boxes and arrows) and world axes; Collision window: brush bounds, a trace from the camera, a colliding free-fly camera |
| 9 | `feat/log-console`              | ✅     | Recent log messages kept in memory (a third spdlog sink); an in-game console at the bottom of the screen (the ~ key): level and module filters, colors, auto-scroll, opacity and height; the game runs without a console window |
| 10 | `feat/player-movement`         | ✅     | The player as an entity (box, look angles, camera at the eyes) and F2 to switch to the free-fly camera; a new Physics module: gravity, sliding along walls, walking with Quake acceleration and friction, steps (also when landing on stairs) with smoothed view, jumping and air control, jumps between ticks kept; mouse captured while playing; stuck-in-wall detection and push-out; Movement window with a speedometer and sliders |
| 11 | `refactor/review-0.2`          | ✅     | A review of the whole project before the release: dead code removed, outdated comments fixed; map units (`Core/Math/Units.h`), shader locations (`Renderer/OpenGL/ShaderInterface.h`), the player box and start (`World/PlayerStart.h`) each defined once; one `LookAngles` component for the player and the free-fly camera, action axes, shared directions; player components in the entity inspector; one file per debug window and View submenus; magic numbers named; the colliding free-fly camera slides along walls; physics bugs found by a random walk on the map fixed (NaN at slanted faces, pushes out of walls); assertions for preconditions; Core and Renderer split into topic folders; then a guided walkthrough of the project; release v0.2.0 |

**Done when:** a level made in TrenchBroom loads with textures, the player
walks, runs and jumps on it and collides with its walls, movement behaves the
same at any frame rate.

![0.2 First Steps: the textured test level](Screenshots/v0.2.0/TestLevel.png)

## 0.3 — Boomstick 🔨

**Goal:** a shotgun in the hands of the player: hitscan shots with a spread of
pellets, sound, recoil, damage to targets, a muzzle flash, particles and
marks on the walls.

Models and sounds come from CC0 packs (Quaternius, Kenney, CC0 sounds on
freesound) for now; own, bought or generated assets can replace them later.
Every asset is listed in `ASSETS.md` before it is committed.

| # | Branch                          | Status | Content                                                                 |
|---|---------------------------------|--------|-------------------------------------------------------------------------|
| 1 | `refactor/gameplay-systems`     | ✅     | The switching between the player and the free-fly camera and their updates move from `Application` into Gameplay systems, so the weapon has a place to plug in |
| 2 | `feat/audio`                    | ✅     | New `Audio` module on miniaudio, sound store in the asset cache, voices with a limit per sound, random pitch and variants; jump and landing sounds; Audio window (volume, playing voices, a test sound in 3D where the camera looks) |
| 3 | `feat/gltf-models`              | ✅     | glTF (`.glb`) loading: meshes, materials, textures; material store; a CC0 model standing in the test room |
| 4 | `feat/view-model`               | ⏳     | The shotgun in the hands: drawn in its own pass with its own field of view (never inside walls), bob while walking, sway behind the mouse; window with sliders |
| 5 | `feat/hitscan`                  | ⏳     | Fire action (left mouse button), single-barrel shotgun: pellets with a random spread, time between shots, recoil of the view and the model, shot sound; debug lines of the pellets; random numbers in `Core/Math`; unlimited ammo |
| 6 | `feat/damage`                   | ⏳     | `Health` component; target dummies placed in TrenchBroom (`target_dummy`); traces against entities, not only walls; damage, hit sound, a destroyed target disappears; knockback from shots |
| 7 | `feat/impact-effects`           | ⏳     | Muzzle flash sprite; particles (sparks and dust at hits, smoke); decals (pellet marks on walls, a limited number) |
| 8 | `refactor/review-0.3`           | ⏳     | A review of the project and a guided walkthrough of what changed since the last one; screenshots; release v0.3.0 |

**Done when:** the player shoots the shotgun at target dummies in the test
level, the pellets spread, the targets take damage and are destroyed, every
shot is heard and leaves a flash, particles and marks on the walls.

## Later milestones

Detailed branch plans are written when a milestone starts. Notes collected so
far:

- **0.2** — collision is our own box trace against brushes (Quake 2 style);
  a tree of brushes (BVH) only when large maps make checking every brush
  measurably slow. Jolt Physics only for queries if it becomes necessary.
  The free-fly camera becomes a debug noclip mode next to the player camera
  (branch 10). A search field above the entity list of the inspector once a
  map brings hundreds of entities.
- **0.3 → later** — ammo counter and the HUD (0.4), ammo pickups (0.6); the
  muzzle flash lights the walls (0.5); weapon animations such as drawing and
  reloading with skeletal animation (0.4); the double-barreled shotgun with
  weapon switching (0.7).
- **0.4** — navmesh via Recast/Detour for ground enemies; flying enemies need
  a separate approach. Frustum culling of entities (a bounding sphere tested
  against the six planes of the view) once enemies, pickups and effects bring
  many draw calls; measure first.
- **0.5** — own lightmap compiler as part of the level compiler; sRGB textures
  and framebuffer (gamma correction) together with lighting; shader hot reload.
  Visibility of level parts (BSP leaves and PVS, or portals) in the level
  compiler, if measurements on large maps show that drawing the whole level
  as one mesh is too slow.
- **0.6** — pickups spin and bob in place, like in Quake (the `Spin`
  component made for the old demo crates is kept for them).
- **0.8** — every gameplay component must be serializable; keep this in mind
  from 0.2 onwards. Options menu, *Display > FPS limit*: a list of common
  monitor refresh rates (30, 60, 75, 90, 100, 120, 144, 165, 180, 240, 280,
  360, Unlimited) plus the refresh rate of the player's monitor detected
  through SDL; a separate lower limit for menus and an unfocused window.
  The debug menu keeps its own list of values chosen for testing.
  The same settings file keeps the UI scale of the debug overlay and the
  screen mode, which are not saved between runs before that. *Display >
  Resolution* for exclusive fullscreen (the desktop resolution until then).

## Backlog

Ideas that are not assigned to a milestone yet.

- **Developer tools window** (ImGui): buttons and checkboxes instead of typing —
  list of levels to load, god mode, fly through walls, give weapons. Useful once
  levels and gameplay exist (around 0.6).
- **Developer commands** typed into the in-game console (branch 9), in the
  style of Half-Life: readable long command names
  (`load_level`, `toggle_god_mode`) with autocompletion while typing and
  history. Only if the tools window is not enough.
  Both would call the same **command registry**, so a button and a typed
  command run the same code.
- **Entity inspector growth**: filter the list by component ("only enemies"),
  a hierarchy once entities have parents (a weapon attached to the player),
  and a "save values to file" button once parameters live in JSON (0.6), so
  numbers tuned in the running game become the game's settings.
- **"Reset window positions"** in the View menu, for debug windows saved
  outside a smaller game window.
- **Log level chosen in the running game** (*Settings > Log level* in the
  debug overlay, or a console command): `Log::SetMinimumLevel()` instead of
  the fixed level in `Main.cpp`. Trace messages (written every frame or tick:
  physics traces, AI decisions, loaded assets) can then be switched on while
  a bug is reproduced and off again, without rebuilding. Worth doing together
  with the first real Trace messages (the AI in 0.4); today only OpenGL
  notifications are written at Trace.
- **Footsteps that depend on the surface** (stone, wood, …): a table from
  textures to surface materials, the material of every brush plane kept for
  collision, traces report the material they hit, the character body keeps
  the material under its feet; steps by the distance walked. Postponed in 0.3:
  in a fast shooter steps may be more annoying than useful.
