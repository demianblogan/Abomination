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
| 0.3     | Boomstick              | ✅     | First hitscan weapon, damage, sound                   | View model with sway/bob/recoil, muzzle flash, particles, decals, glTF loading |
| 0.4     | It Moves               | 🔨     | First enemy: AI, navmesh, health, death, HUD          | Skeletal animation, game interface (RmlUi)               |
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

## 0.3 — Boomstick ✅

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
| 4 | `feat/view-model`               | ✅     | The shotgun in the hands: drawn in its own pass with its own field of view (never inside walls), bob while walking, sway behind the mouse; window with sliders |
| 5 | `feat/hitscan`                  | ✅     | Fire action (left mouse button), single-barrel shotgun: pellets with a random spread, time between shots, recoil of the view and the model, shot sound; debug lines of the pellets; random numbers in `Core/Math`; unlimited ammo |
| 6 | `feat/damage`                   | ✅     | `Health` component; target dummies placed in TrenchBroom (`target_dummy`); traces against entities, not only walls; damage, hit sound, a destroyed target disappears; knockback from shots; a crosshair in the middle of the screen (its look set by the weapon), hit markers for hits and a different one for kills |
| 7 | `feat/impact-effects`           | ✅     | Muzzle flash sprite; particles (sparks and dust at hits, smoke, blood on characters); decals (pellet marks on walls, a limited number); effect textures from the texture generator; the crosshair pulses with every shot; Effects window |
| 8 | `refactor/review-0.3`           | ✅     | A review of the whole project before the release: the unused TexturedMesh shader removed; the damped spring and the approach factor (`Core/Math/Spring`), the drawn transform, the sliders of the tuning windows (`UI/Widgets`) and the loading of sound variants each written once; `UpdateWeapon` and `Render` split into named steps; the overlay gets the whole `GameplayState`; Gameplay and UI split into topic folders; effects tests; `WeaponViewModel` and `DebugLineDepth` names; `MeshPass`; a flat menu bar of the debug overlay with one Weapon window (tabs) and resizable windows; a guided walkthrough of Core, Input, Platform and Renderer; release v0.3.0 |

**Done when:** the player shoots the shotgun at target dummies in the test
level, the pellets spread, the targets take damage and are destroyed, every
shot is heard and leaves a flash, particles and marks on the walls.

## 0.4 — It Moves 🔨

**Goal:** the first enemy: a melee monster that notices the player, runs to
them and strikes; it takes damage, feels pain and dies. The player has health
shown in a HUD and can die too, then the level restarts.

The enemy is a fat rotting dog: its body and texture made with Tripo, its skeleton and
animations (idle, walk, gallop, attack, pain, death) taken from a CC0 husky of Quaternius.
The navmesh comes last: until then the enemy runs straight at the player and
slides along walls with the same movement code as the player, like the
monsters of Quake; if time runs out, the navmesh moves to 0.5.

| # | Branch                          | Status | Content                                                                 |
|---|---------------------------------|--------|-------------------------------------------------------------------------|
| 1 | `feat/game-ui`                 | ✅     | The game interface on RmlUi (documents in RML/RCSS, controls, font effects such as outlines): SDL3 and OpenGL backends; fonts Oswald for the HUD and texts and Cormorant SC for titles, with every letter of the 5 languages (checked by a test) |
| 2 | `feat/hud`                      | ✅     | Health, armor (takes two thirds of a blow) and ammunition of the player (four kinds, 100 shells at the start, an empty click without them); HUD icons, vignettes and the damage arc in the style of Quake from the texture generator; sound events kept by the engine with a volume per event and per group (Effects, Voice, Music), tuned in the Audio window; the player feels damage and healing: the voice of the author (jump, hurt, death, relief), the sound of the blow, a punch of the view, a muffle of the world, a heartbeat at low health; the HUD on RmlUi with the crosshair, a red vignette, a shake and an arc towards the blow, a green vignette when healed, a pulse with the heart, a blinking empty ammunition; Player window |
| 3 | `feat/pump-action`              | ✅     | After every shot the shotgun is brought to the chest and its pump slides back and forth with a click (moved by code); the bolt, taken out of the body of the model, uncovers the window, and a smoking shell flies out, bounces off the level and stays on the floor (the 20 newest) with the sound of its fall |
| 4 | `feat/skeletal-animation`       | ✅     | Skins, joints and animation clips from glTF (with the nodes above the joints); skinning in the vertex shader; an animation player with named segments, speed and cross-fades; the first enemy model, a fat rotting dog made with Tripo and rigged with the skeleton and clips of a CC0 husky; an Animation window with the skeleton and axes |
| 5 | `feat/first-person-hands`       | ✅     | Hands holding the shotgun (WRAD ARMS, the texture slightly darkened and dirtied): posed on the shotgun in a Blender scene with IK handles (Hold, and HoldPumpBack with the left hand on the pulled pump), drawn with the weapon's matrix so they follow its recoil, turn and sway, blended between the two poses as the pump moves; the free-fly camera shows the weapon with the hands in the world |
| 6 | `feat/enemy`                    | ✅     | The dog, the first enemy: a map entity `monster_dog` placed in TrenchBroom; a character body moved by the player's movement code (sliding along walls, steps, gravity), health, hit by pellets; states idle (looking around, sniffing), patrol (wandering), alert (sight: a trace to the player within range and field of view; hearing: shots), chase (galloping straight at the player), attack (a leaping bite that hurts the player), pain; each with its clip and cross-fades; its sounds (bark, bite, landing, yelp, death); clip brushes for smooth stairs, walking along slopes, the model fitted to the ground; an Enemy window with the ranges and the lines of sight; the target dummies of 0.3 removed; Trace messages of what the dog decides (a Debug build writes from Trace on, the console hides Trace until it is checked) |
| 7 | `feat/death`                    | ✅     | The dog dies with its Death clip and its body stays: characters walk through it, shots hit it; at most N bodies (16 by default), the oldest sinks into the floor; below -40 health (a close shot, or shots at the body) it bursts into gibs that bounce and disappear, with blood and a sound (gib models and the sound approved first); the player dies: the weapon goes down, the HUD fades, the player falls on their back and the eyes close; GAME OVER (rusty iron letters) with its sound and PRESS ANY KEY TO RESTART; any key or mouse button restarts the level |
| 8 | `feat/navmesh`                  | ⏳     | Recast/Detour: a navmesh built from the brushes when a level loads, paths around walls and up stairs, the navmesh shown in the debug overlay |
| 9 | `feat/level-dressing`           | ⏳     | The nave of the flooded chapel (E1, from a concept by ChatGPT): new textures cut from the concept sheet (walls, floors, trims, a door, a window; 64×64, seamless), a plan with sizes in units, the room built in TrenchBroom with four octagonal pillars, a raised altar, a balcony and a closed arch down to the crypt |
| 10 | `refactor/review-0.4`          | ⏳     | A review of the project, a list of what changed since the last walkthrough, screenshots; release v0.4.0 |

**The HUD of the whole game** (the parts that come later are drawn when their
feature arrives):
- bottom left: health (0-100%), above it armor (0-100%; hidden while the
  player has none);
- bottom right: the ammo of the weapon in the hands, with one of four icons:
  shells (shotgun, double-barreled shotgun), bullets (machine gun, minigun),
  rockets (rocket and grenade launcher), cells (lightning gun);
- bottom center: keys (bronze, silver, gold), if keys are added;
- top center: an active power-up and its time left, if power-ups are added.

**Done when:** a monster in the test level notices the player, runs to them
around walls and strikes; the shotgun hurts it and kills it; the HUD shows
the health of the player, who dies and restarts the level when it runs out.

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
  weapon switching (0.7); tracers from the muzzle to the hit for weapons whose
  bullets glow (machine gun, 0.7; a shotgun's pellets are not seen); a
  different mark and impact per surface (stone, wood, metal) together with the
  surface materials of the footsteps (backlog).
- **0.4** — navmesh via Recast/Detour for ground enemies; flying enemies need
  a separate approach. Frustum culling of entities (a bounding sphere tested
  against the six planes of the view) once enemies, pickups and effects bring
  many draw calls; measure first. Remove the target dummies of 0.3 when the
  first enemy arrives: `Gameplay/Characters/TargetDummy.h/.cpp`, their use in
  `WeaponSystem.cpp`, `GameplayState` and `Application`, `target_dummy` in
  `Level` and the FGD, `Models/Enemies/Dummy.glb` and its line in
  `ASSETS.md`. Move the crosshair from ImGui lines to the HUD.
- **0.5** — own lightmap compiler as part of the level compiler; sRGB textures
  and framebuffer (gamma correction) together with lighting; shader hot reload.
  Visibility of level parts (BSP leaves and PVS, or portals) in the level
  compiler, if measurements on large maps show that drawing the whole level
  as one mesh is too slow.
- **0.6** — pickups spin and bob in place, like in Quake (the `Spin`
  component made for the old demo crates is kept for them).
  Sound mixer in the Audio window of the debug overlay: a volume slider and a
  play button for every sound event, saved to a data-driven config, so the
  whole mix is balanced by ear (the bite of the dog is drowned by the blow
  and the cry of the player, the yelp of a hit dog by the shotgun).
- **0.8** — every gameplay component must be serializable; keep this in mind
  from 0.2 onwards. Options menu, *Display > FPS limit*: a list of common
  monitor refresh rates (30, 60, 75, 90, 100, 120, 144, 165, 180, 240, 280,
  360, Unlimited) plus the refresh rate of the player's monitor detected
  through SDL; a separate lower limit for menus and an unfocused window.
  The debug menu keeps its own list of values chosen for testing.
  The same settings file keeps the UI scale of the debug overlay and the
  screen mode, which are not saved between runs before that. *Display >
  Resolution* for exclusive fullscreen (the desktop resolution until then).
  *Gameplay > Weapon position*: right, center or left (the debug window has
  it since 0.3).

## Backlog

Ideas that are not assigned to a milestone yet.

- **Weapon animations of the hands** (the hands themselves come in 0.4):
  drawing, reloading and melee animations (pushing shells in one by one), made in
  Blender and played with the skeletal animation of 0.4. Until then weapons have
  no reload animation, like in Quake.
- **The weapon pulls back near walls**: only a visual touch — close to a wall
  the weapon in the hands slides a few centimeters back and down, while shots
  still come from the middle of the screen (unlike Crysis, which raised the
  weapon and stopped it from shooting). The weapon never goes into walls
  anyway (it is drawn over the world); try it once shooting exists.

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
- **Footsteps that depend on the surface** (stone, wood, …): a table from
  textures to surface materials, the material of every brush plane kept for
  collision, traces report the material they hit, the character body keeps
  the material under its feet; steps by the distance walked. Postponed in 0.3:
  in a fast shooter steps may be more annoying than useful.
