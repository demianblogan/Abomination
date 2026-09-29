# Third-Party Software

Every library the project uses is listed here with its license. A library is
added to this list in the same pull request that adds it to `vcpkg.json`.
This list is the source for the libraries section of the in-game Credits
screen. Third-party **assets** are listed separately in
[ASSETS.md](ASSETS.md).

## Distributed with the game

Libraries linked into `Abomination.exe`. Their licenses must be respected in
every release: the license texts are shipped in the `Licenses/` folder of the
game package (see `CMake/Packaging.cmake`). A new library here also gets a
line there.

| Library | Version | License | Purpose                                  | Website                          |
|---------|---------|---------|------------------------------------------|----------------------------------|
| Dear ImGui | 1.92.9b | MIT  | Debug overlay (developer tools only), SDL3 and OpenGL3 backends | https://github.com/ocornut/imgui |
| EnTT    | 4.0.0   | MIT     | Entity-component-system: entities, components, views (header-only, single header in `ThirdParty/EnTT`; vcpkg has only 3.16) | https://github.com/skypjack/entt |
| GLAD    | 2.0.8   | (WTFPL OR CC0-1.0) AND Apache-2.0 | OpenGL 4.6 Core loader, generated into `ThirdParty/GLAD` | https://gen.glad.sh |
| glm     | 1.0.3   | MIT     | Math: vectors, matrices (header-only)    | https://github.com/g-truc/glm    |
| miniaudio | 0.11.25 | Unlicense OR MIT-0 | Sound: decoding OGG/WAV/MP3/FLAC, mixing in its own thread, 3D sound (`Audio`, header-only) | https://miniaud.io |
| SDL3    | 3.4.16  | Zlib    | Window, OpenGL context, events, input    | https://www.libsdl.org           |
| spdlog  | 1.17.0  | MIT     | Logging (`Core/Log`), built without fmt  | https://github.com/gabime/spdlog |
| stb_image | 2.30  | MIT or Public Domain | Decoding PNG/JPG/TGA/BMP (`Core/Image`); stb_image_write in tests only | https://github.com/nothings/stb |

## Development only

Used for building or testing; not part of the shipped game.

| Tool / library | Version | License      | Purpose                         | Website                                   |
|----------------|---------|--------------|---------------------------------|-------------------------------------------|
| GoogleTest     | 1.18.0  | BSD-3-Clause | Unit tests (`AbominationTests`) | https://github.com/google/googletest      |
| vcpkg          | —       | MIT          | C++ package manager             | https://github.com/microsoft/vcpkg        |
| CMake          | —       | BSD-3-Clause | Build system generator          | https://cmake.org                         |
| TrenchBroom    | —       | GPL-3.0      | Level editor; the game only reads the `.map` files it saves (see [LEVEL_EDITING.md](LEVEL_EDITING.md)) | https://trenchbroom.github.io |
