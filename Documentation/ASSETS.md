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
| `Textures/Episode1/Wall_MossyBrick.png` | Alone Bull | `Tools/TextureGenerator` (64×64) | Own work | — | Old brick wall with moss, cracks and damp streaks |
| `Textures/Episode1/Floor_WetFlagstone.png` | Alone Bull | `Tools/TextureGenerator` (64×64) | Own work | — | Wet stone slabs with mud in the joints |
| `Textures/Episode1/Floor_RottenPlanks.png` | Alone Bull | `Tools/TextureGenerator` (64×64) | Own work | — | Rotten wooden planks with nails (stairs, bridges) |
| `Textures/Episode1/Crate_Rotten.png` | Alone Bull | `Tools/TextureGenerator` (64×64) | Own work | — | Dark wooden crate: frame, diagonal brace, rusty iron corners, cracks |
| `Sounds/Player/Land1.ogg` – `Land3.ogg` | Kenney | [Impact Sounds 1.0](https://kenney.nl/assets/impact-sounds) | CC0 | Renamed from `footstep_concrete_000.ogg` – `002.ogg` | Landing of the player on a stone floor |
| `Sounds/Debug/TestKnock.ogg` | Kenney | Same pack | CC0 | Renamed from `impactMetal_medium_000.ogg` | Test sound of the Audio window (3D sound in the world) |
| `Models/Weapons/Shotgun.glb` | Kain Hunter | ["Remington 870 Retro/Quake Style"](https://skfb.ly/oq7yA) on Sketchfab | CC-BY 4.0 | Yes (Blender, by Alone Bull): centered on the origin, the barrel along -Z, FBX helper nodes removed, textures resized from 1024×1024 to 256×256; renamed to `Shotgun.glb` | Shotgun (10 parts: the pump and the slide move on their own). Credit exactly: "Remington 870 Retro/Quake Style" (https://skfb.ly/oq7yA) by Kain Hunter is licensed under Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/). |
