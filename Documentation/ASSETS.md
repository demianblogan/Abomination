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
| `UI/HUD.rml`, `UI/HUD.rcss` | Alone Bull | — | Own work | — | The HUD: health, armor, ammunition, crosshair, vignettes, arcs of the damage direction |
| `UI/Icons/Health.png`, `Armor.png`, `Shells.png`, `Bullets.png`, `Rockets.png`, `Cells.png`, `KeyBronze.png`, `KeySilver.png`, `KeyGold.png` | Alone Bull | `Tools/TextureGenerator` (32×32, saved 4× larger with every pixel a 4×4 block) | Own work | — | Icons of the HUD in the style of the world: health, armor, the four kinds of ammunition, the three keys |
| `UI/Images/VignetteDamage.png`, `VignetteHeal.png`, `DamageArc.png` | Alone Bull | `Tools/TextureGenerator` (vignettes 128×72, arc 48×16, saved 4× larger with every pixel a 4×4 block) | Own work | — | Effects of the HUD: the red vignette of a blow, the light green glow of healing, the arc that shows where a blow came from |
| `UI/Images/VignetteDeath.png`, `Eyelid.png`, `TitleGlow.png` | Alone Bull | `Tools/TextureGenerator` (vignette 256×144, eyelid 256×128, glow 256×96, smooth) | Own work | — | The death of the player: the soft black edges of heavy eyes, an eyelid with a soft curved edge (the lower one is the same picture turned), the red glow behind GAME OVER |
| `UI/Images/GameOver.png` | Alone Bull | `Tools/Blender/MakeGameOverTitle.py` (Oswald Bold letters as rusty iron with dried blood, lit from below; 1600×400, transparent) | Own work | — | The GAME OVER title of the death screen |
| `Textures/Weapons/ShotgunShell.png` | Alone Bull | `Tools/TextureGenerator` (16×16) | Own work | — | The ejected shotgun shell: a tarnished brass base, a worn red tube, a dark crimp |
| `Textures/Weapons/ShotgunOpening.png` | Alone Bull | `Tools/TextureGenerator` (16×16) | Own work | — | The dark opening the bolt of the shotgun uncovers when it slides back with the pump |
| `Textures/Episode1/Wall_MossyBrick.png` | Alone Bull | `Tools/TextureGenerator` (64×64) | Own work | — | Old brick wall with moss, cracks and damp streaks |
| `Textures/Episode1/Floor_WetFlagstone.png` | Alone Bull | `Tools/TextureGenerator` (64×64) | Own work | — | Wet stone slabs with mud in the joints |
| `Textures/Episode1/Floor_RottenPlanks.png` | Alone Bull | `Tools/TextureGenerator` (64×64) | Own work | — | Rotten wooden planks with nails (stairs, bridges) |
| `Textures/Episode1/Crate_Rotten.png` | Alone Bull | `Tools/TextureGenerator` (64×64) | Own work | — | Dark wooden crate: frame, diagonal brace, rusty iron corners, cracks |
| `Textures/Episode1/Wall_MossyBlocks.png`, `Wall_CrackedPlaster.png`, `Wall_RottenPlanks.png`, `Floor_Mud.png`, `Floor_MossyFlagstone.png` | Alone Bull (generated with ChatGPT) | Seamless textures generated with ChatGPT one by one (1254×1254) | Own work | Shrunk to 128×128 (area average) | The flooded chapel: mossy stone blocks, plaster cracked over brick, rotten planks, mossy flagstones, mud with stones |
| `Textures/Episode1/Trim_Arches.png`, `Trim_IronBand.png` | Alone Bull (generated with ChatGPT) | Seamless trims generated with ChatGPT (1254×1254) | Own work | The band in the middle cut out (rows 294–921) and shrunk to 128×64 | The flooded chapel: a frieze of gothic arches, a rusty iron band with rivets |
| `Textures/Episode1/Wall_GothicNiches.png` | Alone Bull (generated with ChatGPT) | A concept texture sheet of episode 1 generated with ChatGPT | Own work | Cut out of the sheet by `Tools/Blender/MakeChapelTextures.py` (made seamless at the period of its pattern), shrunk to 64×64 | The flooded chapel: a wall with gothic niches |
| `Textures/Episode1/Door_Chapel.png`, `Window_Chapel.png` | Alone Bull (generated with ChatGPT) | Drawn by ChatGPT on flat magenta (1024×2048, 887×1774) | Own work | The magenta made transparent and shrunk to 64×128 by `Tools/Blender/MakeCutoutTexture.py` (cutouts: the wall shows around the arch) | The flooded chapel: an arched door with iron bands, a gothic window with a rusty grille |
| `Textures/Episode1/Pillar_MossyBlocks.png`, `Pillar_WetFlagstone.png` | Alone Bull | Copies of `Wall_MossyBlocks.png` and `Floor_WetFlagstone.png` | Own work | — | The pillars of the chapel and their bases: the same pictures, but without a height map, so without parallax (it shows a box behind glass on every narrow face of an octagon). Material files (0.6) will replace the copies with variants of one material |
| `Textures/Episode1/*_Normal.png`, `*_MetalRough.png` (of every texture above), `*_Height.png` (of the masonry: `Wall_MossyBrick`, `Wall_MossyBlocks`, `Wall_GothicNiches`, `Trim_Arches`, `Floor_MossyFlagstone`, `Floor_WetFlagstone`) | Alone Bull | `Tools/MaterialMaps` (made from the color texture of the same name, at its size) | Own work | — | The maps of the materials of episode 1: relief (normals and height from the brightness), roughness and metalness (settings per texture: wet flagstones smooth, plaster rough, the iron band metal) |
| `Textures/Common/Clip.png` | Alone Bull | PowerShell (System.Drawing, 64×64) | Own work | — | Clip brushes (invisible ramps and walls only characters collide with): seen only in TrenchBroom, never in the game |
| `Sounds/Player/Land1.ogg` – `Land3.ogg` | Kenney | [Impact Sounds 1.0](https://kenney.nl/assets/impact-sounds) | CC0 | Renamed from `footstep_concrete_000.ogg` – `002.ogg` | Landing of the player on a stone floor |
| `Sounds/Debug/TestKnock.ogg` | Kenney | Same pack | CC0 | Renamed from `impactMetal_medium_000.ogg` | Test sound of the Audio window (3D sound in the world) |
| `Models/Weapons/Shotgun.glb` | Kain Hunter | ["Remington 870 Retro/Quake Style"](https://skfb.ly/oq7yA) on Sketchfab | CC-BY 4.0 | Yes (Blender, by Alone Bull): centered on the origin, the barrel along -Z, FBX helper nodes removed, textures resized from 1024×1024 to 256×256; renamed to `Shotgun.glb` | Shotgun (10 parts: the pump and the slide move on their own). Credit exactly: "Remington 870 Retro/Quake Style" (https://skfb.ly/oq7yA) by Kain Hunter is licensed under Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/). |
| `Sounds/Weapons/Shotgun/Fire1.ogg` | Pixabay contributor (not recorded) | [Pixabay sound effects, search "shotgun"](https://pixabay.com/sound-effects/search/shotgun/) | Pixabay Content License | Renamed to `Fire1.ogg` | Shotgun shot. Downloaded from the search results, so the exact page was not recorded; replace or identify it before a commercial release |
| `Sounds/Weapons/Shotgun/DryFire1.ogg` | Alone Bull | — | Own work | Converted from MP3 to OGG mono, peak at -1 dB | The click of the trigger when the shells run out |
| `Sounds/Weapons/Shotgun/Pump1.ogg` | Alone Bull | — | Own work | Converted from MP3 to OGG mono, otherwise unchanged | The pump worked after a shot: a clack at the back (0.265 s), a clack at the front |
| `Sounds/Weapons/Shotgun/ShellDrop1.ogg` – `ShellDrop3.ogg` | Alone Bull | — | Own work | The first 3 of 15 falls cut out of one recording, 10 ms fade in and 30 ms fade out, converted from MP3 to OGG mono | A spent shell hitting the floor |
| `Sounds/Player/HitMelee1.ogg` | Alone Bull | — | Own work | Converted from MP3 to OGG mono, silence at the end cut, peak at -1 dB | A melee blow to the player |
| `Sounds/Player/Heartbeat1.ogg` | Alone Bull | — | Own work | The first two beats of a longer recording (exactly one second), cut in the quiet between beats with 10 ms fades over the quiet there so it starts and ends without a click; otherwise unchanged; OGG mono | The heart at low health (lub-dub), played once a second |
| `Sounds/Enemies/Dog/Bark1.ogg` | Pixabay (found by Alone Bull) | [Pixabay sound effects](https://pixabay.com/sound-effects/) | Pixabay Content License | Silence trimmed, pitched down by 20 % (lower and slower, more menacing), compressed (louder), 60 ms fade out, peak -1 dB; OGG mono | The dog barks when it notices the player |
| `Sounds/Enemies/Dog/Bite1.ogg` | Pixabay (found by Alone Bull) | Same | Pixabay Content License | Cut from 2.1 s to the bite (0.63 s), compressed (louder), 150 ms fade out, peak -1 dB; OGG mono | The bite of the dog that hurts the player |
| `Sounds/Enemies/Dog/Land1.ogg` | Pixabay (found by Alone Bull) | Same | Pixabay Content License | Silence trimmed (0.45 s), peak -1 dB; OGG mono | The dog lands after a leap |
| `Sounds/Enemies/Dog/Hurt1.ogg` | Pixabay (found by Alone Bull) | Same | Pixabay Content License | Background noise removed (FFT denoise and a gate), compressed (louder), whole length kept, peak -1 dB; OGG mono | The yelp of a hit dog |
| `Sounds/Enemies/Dog/Death1.ogg` | Pixabay (found by Alone Bull) | Same | Pixabay Content License | Background noise removed (FFT denoise and a gate), compressed (louder), trimmed to the two yelps (1.1 s), 200 ms fade out, peak -1 dB; OGG mono | The death of the dog |
| `Sounds/Enemies/Gib1.ogg` | Pixabay (found by Alone Bull) | [Pixabay sound effects](https://pixabay.com/sound-effects/) | Pixabay Content License | Cut to the splash (0.93 s), compressed (louder), 250 ms fade out, peak -1 dB; OGG mono | A body bursts into gibs |
| `Sounds/UI/GameOver1.ogg` | Pixabay (found by Alone Bull) | [Pixabay sound effects](https://pixabay.com/sound-effects/) | Pixabay Content License | Cut to 4.8 s (the silence at the end removed), 0.6 s fade out; OGG stereo | The chord of GAME OVER after the death of the player |
| `Sounds/Player/Voice/Jump1-3.ogg`, `Hurt1-3.ogg`, `Death1-3.ogg`, `Relief1-3.ogg` | Alone Bull | — | Own work (the voice of the author) | Takes cut from four recordings at the pauses, silence trimmed, peak at -1 dB, OGG mono; the best three of each kept (Jump3 cut short before a second sound) | The voice of the player: jumping, hurt, dying, healed |
| `Models/Enemies/Dog.glb` | Alone Bull (model and texture made with Tripo); skeleton and animations: Quaternius | Body: own work; animations: the Husky of [Ultimate Animated Animals](https://quaternius.com/packs/ultimateanimatedanimals.html) | Own work; animations CC0 | Rigged by `Tools/Blender/RigDog.py`: stood up, the Husky's skeleton fitted to the body, the legs moved into the dog's and their clips retargeted at 60% of the swing, the skin bound to the nearest bones, the texture scaled from 2048 to 256 | The first enemy (0.4): a fat rotting dog |
| `Models/Enemies/Gibs/Gib1.glb` – `Gib3.glb` | Alone Bull | `Tools/Blender/MakeGibs.py` | Own work | — | The gibs of the dog: a lump of its body, a piece of a leg with the bone, a small scrap (20–100 triangles); a 64×64 texture made from the skin of the dog (fur patches, raw meat, bone) |
| `Models/Weapons/Hands.glb` | wriks; posed by Alone Bull | [WRAD ARMS](https://wriks.itch.io/wrad-arms) on itch.io | CC0 | Prepared by `Tools/Blender/PrepareHands.py` (scaled to meters, the IK handles left out of the skin, the pale texture slightly darkened, less pink, with patches of grime, scaled from 512 to 256), posed on the shotgun in `Tools/Blender/HandsPose.blend` (poses Hold and HoldPumpBack), exported by `Tools/Blender/ExportHandsPoses.py` | The hands of the player holding the weapon |
| `Sounds/Weapons/Hit1.ogg` – `Hit3.ogg` | Kenney | [Impact Sounds 1.0](https://kenney.nl/assets/impact-sounds) | CC0 | Renamed from `impactPunch_medium_000.ogg` – `002.ogg` | A shot hurt something (temporary) |
| `Sounds/Weapons/Kill1.ogg` – `Kill3.ogg` | Kenney | Same pack | CC0 | Renamed from `impactPlate_heavy_000.ogg` – `002.ogg` | A shot killed something (temporary) |
| `Textures/Effects/MuzzleFlash.png` | Alone Bull | `Tools/TextureGenerator` (32×32) | Own work | — | Flash at the muzzle of a weapon |
| `Textures/Effects/Spark.png` | Alone Bull | `Tools/TextureGenerator` (16×16) | Own work | — | Spark of a pellet hitting a wall |
| `Textures/Effects/Smoke.png` | Alone Bull | `Tools/TextureGenerator` (32×32) | Own work | — | Smoke from the muzzle |
| `Textures/Effects/Dust.png` | Alone Bull | `Tools/TextureGenerator` (32×32) | Own work | — | Dust of a pellet hitting a wall |
| `Textures/Effects/PelletMark.png` | Alone Bull | `Tools/TextureGenerator` (16×16) | Own work | — | Mark a pellet leaves on a wall (decal) |
| `Textures/Effects/Blood.png` | Alone Bull | `Tools/TextureGenerator` (16×16) | Own work | — | Drop of blood of a pellet hitting a character |
