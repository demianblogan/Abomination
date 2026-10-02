# Assets

Every third-party asset in `Assets/` must be listed here **before** it is
committed. This list is the source for the in-game Credits screen and for
the license check before a commercial release.

## Allowed licenses

| License                         | Can be sold | Requirement                                   | Allowed |
|---------------------------------|-------------|-----------------------------------------------|---------|
| SIL Open Font License (OFL) 1.1 | ✅          | Ship the license text with the font; the font itself must not be sold separately | ✅      |
| CC0 / Public Domain             | ✅          | None                                          | ✅      |
| CC-BY 3.0 / 4.0                 | ✅          | Credit the author (Credits screen + this file)| ✅      |
| Purchased, commercial use       | ✅          | Follow the store's license terms              | ✅      |
| Pixabay Content License         | ✅          | Only as part of the game, never as a file on its own (no stock redistribution) | ✅      |
| AI-generated                    | depends     | Service terms must allow commercial use       | ⚠️ check terms |
| CC-BY-SA                        | ✅          | Modified asset must use the same license      | ⚠️ avoid |
| CC-BY-ND                        | ✅          | Asset must not be modified                    | ⚠️ only unmodified |
| CC-BY-NC, "personal use only"   | ❌          | —                                             | ❌      |
| Unknown license                 | ❌          | —                                             | ❌      |

## Rules

- File names follow the code style: PascalCase (`RocketLauncher.glb`).
- Record whether the asset was **modified** (resized, recolored, re-rigged).
- For AI-generated assets record the service and the plan/terms used.
- For purchased assets keep the receipt/license outside the repository and
  note the store here.
- Assets made by us are marked with the author `Alone Bull`.

## Asset list

| File | Author | Source | License | Modified | Notes |
|------|--------|--------|---------|----------|-------|
| `Fonts/JetBrainsMonoRegular.ttf` | The JetBrains Mono Project Authors | [JetBrains Mono 2.304](https://github.com/JetBrains/JetBrainsMono/releases/tag/v2.304) | SIL Open Font License 1.1 | Renamed from `JetBrainsMono-Regular.ttf` | Debug overlay font. The license must ship with the font |
| `Fonts/JetBrainsMonoLicense.txt` | The JetBrains Mono Project Authors | Same archive, `OFL.txt` | — | Renamed from `OFL.txt` | License text of the font |
| `Fonts/OswaldBold.ttf` | The Oswald Project Authors | [OswaldFont](https://github.com/googlefonts/OswaldFont), `fonts/ttf/Oswald-Bold.ttf` | SIL Open Font License 1.1 | Renamed from `Oswald-Bold.ttf` | Text font of the game: the HUD and every ordinary text (Latin and Cyrillic). The license must ship with the font |
| `Fonts/OswaldLicense.txt` | The Oswald Project Authors | Same repository, `OFL.txt` | — | Renamed from `OFL.txt` | License text of the font |
| `Fonts/CormorantSCBold.ttf` | The Cormorant Project Authors | [Google Fonts](https://github.com/google/fonts/tree/main/ofl/cormorantsc), `ofl/cormorantsc/CormorantSC-Bold.ttf` | SIL Open Font License 1.1 | Renamed from `CormorantSC-Bold.ttf` | Title font of the game: big headings such as "Game Over" (Latin and Cyrillic). The license must ship with the font |
| `Fonts/CormorantSCLicense.txt` | The Cormorant Project Authors | Same folder, `OFL.txt` | — | Renamed from `OFL.txt` | License text of the font |
| `UI/Base.rcss` | The RmlUi authors, Alone Bull | Adapted from the base style sheet of the [RmlUi samples](https://github.com/mikke89/RmlUi) | MIT (the license of RmlUi, shipped in `Licenses/RmlUi.txt`) | Reduced to the rules the game uses | Display types of the elements every document starts from |
| `UI/Icons/Health.png`, `Armor.png`, `Shells.png`, `Bullets.png`, `Rockets.png`, `Cells.png`, `KeyBronze.png`, `KeySilver.png`, `KeyGold.png` | Alone Bull | `Tools/TextureGenerator` (32×32, saved 4× larger with every pixel a 4×4 block) | Own work | — | Icons of the HUD in the style of the world: health, armor, the four kinds of ammunition, the three keys |
| `Textures/Episode1/Wall_MossyBrick.png` | Alone Bull | `Tools/TextureGenerator` (64×64) | Own work | — | Old brick wall with moss, cracks and damp streaks |
| `Textures/Episode1/Floor_WetFlagstone.png` | Alone Bull | `Tools/TextureGenerator` (64×64) | Own work | — | Wet stone slabs with mud in the joints |
| `Textures/Episode1/Floor_RottenPlanks.png` | Alone Bull | `Tools/TextureGenerator` (64×64) | Own work | — | Rotten wooden planks with nails (stairs, bridges) |
| `Textures/Episode1/Crate_Rotten.png` | Alone Bull | `Tools/TextureGenerator` (64×64) | Own work | — | Dark wooden crate: frame, diagonal brace, rusty iron corners, cracks |
| `Sounds/Player/Land1.ogg` – `Land3.ogg` | Kenney | [Impact Sounds 1.0](https://kenney.nl/assets/impact-sounds) | CC0 | Renamed from `footstep_concrete_000.ogg` – `002.ogg` | Landing of the player on a stone floor |
| `Sounds/Debug/TestKnock.ogg` | Kenney | Same pack | CC0 | Renamed from `impactMetal_medium_000.ogg` | Test sound of the Audio window (3D sound in the world) |
| `Models/Weapons/Shotgun.glb` | Kain Hunter | ["Remington 870 Retro/Quake Style"](https://skfb.ly/oq7yA) on Sketchfab | CC-BY 4.0 | Yes (Blender, by Alone Bull): centered on the origin, the barrel along -Z, FBX helper nodes removed, textures resized from 1024×1024 to 256×256; renamed to `Shotgun.glb` | Shotgun (10 parts: the pump and the slide move on their own). Credit exactly: "Remington 870 Retro/Quake Style" (https://skfb.ly/oq7yA) by Kain Hunter is licensed under Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/). |
| `Sounds/Weapons/Shotgun/Fire1.ogg` | Pixabay contributor (not recorded) | [Pixabay sound effects, search "shotgun"](https://pixabay.com/sound-effects/search/shotgun/) | Pixabay Content License | Renamed to `Fire1.ogg` | Shotgun shot. Downloaded from the search results, so the exact page was not recorded; replace or identify it before a commercial release |
| `Models/Enemies/Dummy.glb` | iJUNE | ["[ FREE ] Dummy Model"](https://sketchfab.com/3d-models/free-dummy-model-8f3cb85451214dd6ad38209e46f47182) on Sketchfab | CC-BY 4.0 | — | Temporary target dummy of 0.3, removed with the first enemy (0.4) |
| `Sounds/Weapons/Hit1.ogg` – `Hit3.ogg` | Kenney | [Impact Sounds 1.0](https://kenney.nl/assets/impact-sounds) | CC0 | Renamed from `impactPunch_medium_000.ogg` – `002.ogg` | A shot hurt something (temporary) |
| `Sounds/Weapons/Kill1.ogg` – `Kill3.ogg` | Kenney | Same pack | CC0 | Renamed from `impactPlate_heavy_000.ogg` – `002.ogg` | A shot killed something (temporary) |
| `Textures/Effects/MuzzleFlash.png` | Alone Bull | `Tools/TextureGenerator` (32×32) | Own work | — | Flash at the muzzle of a weapon |
| `Textures/Effects/Spark.png` | Alone Bull | `Tools/TextureGenerator` (16×16) | Own work | — | Spark of a pellet hitting a wall |
| `Textures/Effects/Smoke.png` | Alone Bull | `Tools/TextureGenerator` (32×32) | Own work | — | Smoke from the muzzle |
| `Textures/Effects/Dust.png` | Alone Bull | `Tools/TextureGenerator` (32×32) | Own work | — | Dust of a pellet hitting a wall |
| `Textures/Effects/PelletMark.png` | Alone Bull | `Tools/TextureGenerator` (16×16) | Own work | — | Mark a pellet leaves on a wall (decal) |
| `Textures/Effects/Blood.png` | Alone Bull | `Tools/TextureGenerator` (16×16) | Own work | — | Drop of blood of a pellet hitting a character |
