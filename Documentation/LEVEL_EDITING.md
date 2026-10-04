# Level Editing

Levels of Abomination are built in **[TrenchBroom](https://trenchbroom.github.io/)**,
the level editor for Quake-like games, and saved as `.map` files in
`Assets/Maps/`. The game reads these files directly.

## Setting up TrenchBroom (once per machine)

TrenchBroom supports many games; it has to be told about ours: where our
textures are and which entities (player start, later enemies, pickups, doors)
can be placed. This is described by two files kept in the repository:

| File | Content |
|------|---------|
| `Tools/TrenchBroom/Abomination/GameConfig.cfg` | Name, map format (Valve 220), where textures are |
| `Tools/TrenchBroom/Abomination/Abomination.fgd` | Entity definitions: what can be placed, size and color in the editor |

1. **Download** the latest TrenchBroom for Windows from its
   [releases](https://github.com/TrenchBroom/TrenchBroom/releases) and unpack
   it to a permanent folder (it needs no installation).
2. **Connect the game configuration.** TrenchBroom looks for custom games in
   `%APPDATA%\TrenchBroom\games`. Instead of copying our folder there, create a
   link to it, so changes in the repository are seen by TrenchBroom at once.
   In a command prompt (`cmd`) from the root of the repository:
   ```
   mkdir "%APPDATA%\TrenchBroom\games"
   mklink /J "%APPDATA%\TrenchBroom\games\Abomination" "%CD%\Tools\TrenchBroom\Abomination"
   ```
   (`mklink /J` creates a directory junction: a folder that shows the contents
   of another folder. It needs no administrator rights.)
3. **Set the game path.** Start TrenchBroom, open *Preferences → Games*,
   select *Abomination* and set *Game Path* to the root of the repository
   (the folder with `Assets/`).

## Units and axes

- **32 units = 1 meter.** The Quake player is 56 units tall (about 1.75 m);
  the grid sizes of TrenchBroom (8, 16, 32, 64) are based on this scale.
- **Z is up** in TrenchBroom. The game converts to its own axes (Y up) when it
  loads a map.

## Building the test map

`Assets/Maps/Test.map` is the first map, used to develop map loading:

1. *File → New Map*, choose *Abomination*, map format *Valve*.
2. In the material browser (the *Face* tab of the right panel) select a
   texture, for example `Wall_MossyBrick` in the *Episode1* collection: new
   brushes get the selected material. Every folder in `Assets/Textures` is a
   collection; a map stores the texture of a face by its path in
   `Assets/Textures` without the extension (`Episode1/Wall_MossyBrick`).
3. **Room.** In the 3D view, left-drag on the grid to draw a box of about
   512 × 512 units, then raise it to 256 units (hold `Alt` while dragging to
   change the height). With the box selected, use *Edit → CSG → Hollow*: the
   box becomes six brushes, walls, floor and ceiling, with the empty room
   inside.
4. **Pillar.** Inside the room draw a box of 64 × 64 units from the floor to
   the ceiling.
5. **Stairs.** In the tool options of the shape tool choose *Stairs* and draw
   a staircase of 4–6 steps against a wall.
   Then cover it with a ramp of clip: a brush with the texture
   `Common/Clip` (purple, *CLIP*) from the bottom of the first step to the
   top of the last (draw a box over the stairs, then move its top edge with
   the vertex tool). Clip is never drawn in the game; characters walk on it,
   so they go up and down the stairs smoothly instead of jumping from step to
   step, while shots and sight go through it. Clip also makes invisible
   walls.
6. **Player start.** In the entity browser (right panel) drag
   `info_player_start` onto the floor.
7. **Model.** Drag `misc_model` into the room: it shows the model of its
   `model` property (a `.glb` file in `Assets`, the shotgun by default).
8. Save as `Assets/Maps/Test.map`.

## Seeing the map in the game

The game loads `Assets/Maps/Test.map` at startup: save the map in
TrenchBroom and reload the level (a Debug build reads `Assets` of the
repository; a Release build reads the copy made next to the executable on
every build), or start the game. The player appears at `info_player_start`, looking in the
direction of its angle. A player start placed on the floor puts the player on
the floor; one placed higher lets the player fall at the start (like in
Quake). F2 switches to the free-fly camera to look at the map from anywhere.

- Faces are drawn with their textures, lighter or darker by the direction
  they face, so walls, floor and ceiling stay apart until real lighting (0.5).
  A texture that is missing shows as a magenta and black checkerboard, with a
  warning in the log.
- *Renderer* in the menu bar in the debug overlay (<kbd>F1</kbd>) switches to
  *Wireframe* to show how faces are split into triangles, and shows how many
  brushes, faces and triangles the level has.
- **Without restarting the game:** save the map, build only the
  `CopyAssets` target (`cmake --build --preset debug --target CopyAssets`,
  it works while the game runs) and press *Reload* in the Renderer window.
- **Sizes:** the player is 56 units tall (eyes at about 46). A usual crate is
  32 units (1 m) with the crate texture at scale 0.5; a 64-unit crate is a
  large container, taller than the player.
- A map that cannot be read stops the game with an error dialog giving the
  line of the problem. A brush face that does not make sense (three points on
  one line) is skipped.
- Avoid faces of different objects lying in one plane (a box standing flush
  with the top of a step shows flickering stripes there, *z-fighting*).
