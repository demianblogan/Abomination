# Building

How to get the source, build the game and run the tests on Windows.

## Requirements

| Tool                 | Version        | Notes                                                        |
|----------------------|----------------|--------------------------------------------------------------|
| Windows              | 10 or 11, x64  | The only supported platform                                  |
| Visual Studio        | 2026           | Workload **Desktop development with C++** (includes CMake)   |
| Git                  | any recent     | Also used by vcpkg to fetch library recipes                  |
| vcpkg                | any recent     | Library versions are pinned by `builtin-baseline` in `vcpkg.json` |
| GPU driver           | OpenGL 4.6     | Required once rendering is added (milestone 0.1)             |

## 1. Install vcpkg (once per machine)

```bash
git clone https://github.com/microsoft/vcpkg.git C:/Development/vcpkg
C:/Development/vcpkg/bootstrap-vcpkg.bat -disableMetrics
```

Set the environment variable **`VCPKG_ROOT`** to the vcpkg folder
(*Settings → System → About → Advanced system settings → Environment
Variables*), then restart Visual Studio and terminals. `CMakePresets.json`
finds vcpkg through this variable.

## 2. Get the source

```bash
git clone https://github.com/demianblogan/Abomination.git
```

## 3. Build in Visual Studio (recommended)

1. **File → Open → Folder…** and select the repository root.
2. Visual Studio detects `CMakePresets.json` and configures the project.
   The first configuration downloads and builds all libraries through
   vcpkg — this takes a few minutes once, later runs are fast.
3. Select the configure preset **Windows (Visual Studio 2026, x64)** and the
   build preset **Debug** or **Release** in the toolbar.
4. **Build → Build All**, then **F5** runs the game.
5. **Test → Test Explorer → Run All** runs the unit tests.

## 4. Build from the command line

Use the **same CMake that Visual Studio uses** — the one bundled with it:

```
C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\
```

If no other CMake is installed, it is available as plain `cmake` in the
**Developer PowerShell for VS 2026**. A separately installed CMake comes first
on `PATH` even there — then call the bundled `cmake.exe` / `ctest.exe` by
their full path.

```bash
cmake --preset windows-msvc
cmake --build --preset debug
ctest --preset debug
```

Replace `debug` with `release` for an optimized build.

> [!WARNING]
> Do not mix different CMake versions on the same build folder (for example a
> separately installed CMake and the one inside Visual Studio). Files generated
> by a newer CMake may not be readable by an older CTest, and Test Explorer
> will then show no tests. If this happens, run
> *Project → Delete Cache and Reconfigure* in Visual Studio.

## 5. Build the game package

The package is the folder a player receives: everything needed to run the game
on another computer, without Visual Studio or the Visual C++ Redistributable.

```bash
cmake --build --preset release
cmake --install Build/windows-msvc --config Release
```

The folder `Build/Package/` then contains `Abomination.exe`, `Assets/`, the
Microsoft C++ runtime DLLs, `LICENSE.md` and `Licenses/` with the licenses of
the libraries. What goes into it is described in `CMake/Packaging.cmake`.
Delete `Build/Package/` before installing again: files removed from the
project are not removed from an existing package.

## 6. Profile the game

Development builds carry the client of the [Tracy](https://github.com/wolfpld/tracy)
profiler (CMake option `ABOMINATION_PROFILING`, on by default; the Release
workflow turns it off for the package). The game starts the client in
`main()` and listens on this computer only; data is collected only while a
profiler program is connected. How it works: ARCHITECTURE.md, section 17.

Download the Windows archive of **Tracy 0.14.1** from the
[releases](https://github.com/wolfpld/tracy/releases) — the version must be
the one of `ThirdParty/Tracy`, otherwise the programs do not connect.

**A capture in numbers** (the usual way): one command builds the game, runs
the benchmark (`Abomination.exe --benchmark`: the start map, nobody at the
controls, an invulnerable player the dogs attack, 20 s of game time),
captures it and prints what every zone costs per frame:

```bash
powershell -ExecutionPolicy Bypass -File Tools/Profiling/Capture.ps1 -Configuration Release -Label before
```

Parameters: `-Configuration Debug|Release`, `-Label` (a word for the file
names), `-TracyDirectory` (the unpacked archive, by default
`Downloads\windows-0.14.1`), `-ZoneCount`, `-NoBuild`. The capture (`.tracy`),
the tables (`.csv`) and the summary go to `Build/Profiles/`.

**Looking at frames:** run the game, start `tracy-profiler.exe`, connect to
`127.0.0.1`; *Pause* stops the live view, a click on a bar of the frame graph
opens that frame. Only one program can be connected at a time: close the
profiler window before running the script.

> [!IMPORTANT]
> Measure Release: a Debug build is many times slower in places where the
> game is not (the cost of the dogs is a Debug cost). Compare captures made
> one right after the other: a laptop that has been working for a long time
> slows down from heat, and every zone becomes slower, changed or not.
> Compare the cost of the zone that was changed, not only the frame rate,
> which varies by about 10% from run to run.

## Output

Everything is generated in `Build/<preset>/` (ignored by Git):

| Path                                   | Content                       |
|----------------------------------------|-------------------------------|
| `Build/windows-msvc/Abomination.slnx`  | Generated Visual Studio solution |
| `Build/windows-msvc/Binaries/Debug/`   | `Abomination.exe`, `AbominationTests.exe` |
| `Build/windows-msvc/Binaries/Release/` | Same, optimized               |
| `Build/Package/`                       | The game package (after `cmake --install`) |
| `Build/Profiles/`                      | Captures of `Tools/Profiling/Capture.ps1` |

The generated solution can be opened directly, but project settings must be
changed only in `CMakeLists.txt` — edits made in Visual Studio's project
properties are lost on the next CMake run.
