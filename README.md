<div align="center">

<!-- Logo goes here once it is ready:
<img src="Documentation/Images/Logo.png" alt="Abomination" width="480">
-->

# ABOMINATION

**A retro first-person shooter inspired by Quake (1996),<br>
built from scratch with modern C++ and OpenGL 4.6.**

[![C++23](https://img.shields.io/badge/C%2B%2B-23-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)](https://en.cppreference.com/w/cpp/23)
[![OpenGL 4.6](https://img.shields.io/badge/OpenGL-4.6_Core-5586A4?style=for-the-badge&logo=opengl&logoColor=white)](https://www.khronos.org/opengl/)
[![SDL3](https://img.shields.io/badge/SDL-3-1D4E89?style=for-the-badge)](https://www.libsdl.org/)
![Windows](https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white)
<br>
[![CI](https://img.shields.io/github/actions/workflow/status/demianblogan/Abomination/CI.yml?branch=main&style=flat-square&label=CI&logo=githubactions&logoColor=white)](https://github.com/demianblogan/Abomination/actions/workflows/CI.yml)
[![Release](https://img.shields.io/github/v/release/demianblogan/Abomination?style=flat-square&label=release&color=brightgreen)](https://github.com/demianblogan/Abomination/releases/latest)
[![Status](https://img.shields.io/badge/status-early_development-orange?style=flat-square)](Documentation/ROADMAP.md)
[![Milestone](https://img.shields.io/badge/milestone-0.3_Boomstick-blue?style=flat-square)](Documentation/ROADMAP.md)
[![License](https://img.shields.io/badge/license-PolyForm_Noncommercial_1.0-lightgrey?style=flat-square)](LICENSE.md)

[About](#-about) •
[Features](#-features) •
[Screenshots](#-screenshots) •
[Tech Stack](#%EF%B8%8F-tech-stack) •
[Download](#%EF%B8%8F-download) •
[Building](#-building) •
[Controls](#-controls) •
[Roadmap](#%EF%B8%8F-roadmap) •
[Documentation](#-documentation) •
[License](#-license)

</div>

---

## 🩸 About

**Abomination** is a fast-paced, old-school shooter: no reloading, no
cutscenes, no hand-holding — just you, a growing arsenal and whatever crawls,
flies and teleports out of the dark.

The project is also a deep dive into graphics programming: the engine is
written from the ground up, and every system — from the OpenGL renderer to
the enemy AI — is built to be read and learned from.

> [!NOTE]
> The game is in early development. **Version 0.2** is out: run and jump around a test level with
> Quake-style movement — follow the [roadmap](Documentation/ROADMAP.md) to see what is being built next.

## 🎯 Features

<table>
<tr>
<td width="50%" valign="top">

### 🕹️ Gameplay
- **4 episodes × 5 levels**, a unique boss at the end of each episode
- **Quake-style movement** — fast, fluid, skill-based
- **Varied arsenal** — every weapon has its own model and feel
- **Diverse enemies** — they run, crawl, jump, fly, climb walls and teleport
- Doors, buttons, pressure plates, traps and secrets

</td>
<td width="50%" valign="top">

### 🎨 Presentation
- **Retro look** — low-poly models, low-resolution pixel-crisp textures
- **Modern lighting** — baked lightmaps, dynamic lights, HDR, bloom
- Unique color palette for every episode
- Full gamepad support, including **DualSense** haptics, adaptive triggers
  and light bar
- 5 languages: English, Español, Deutsch, Русский, Українська

</td>
</tr>
</table>

## 📸 Screenshots

Every picture has the same size; click one to open it in full resolution.

### 0.2 — First Steps

<table>
<tr>
<td align="center" valign="top" width="25%"><a href="Documentation/Screenshots/v0.2.0/TestLevel.png"><img src="Documentation/Screenshots/v0.2.0/TestLevel.png" alt="The textured test level" width="176" height="99"></a><br><sub>A level built in TrenchBroom</sub></td>
<td align="center" valign="top" width="25%"><a href="Documentation/Screenshots/v0.2.0/ConsoleAndPerformance.png"><img src="Documentation/Screenshots/v0.2.0/ConsoleAndPerformance.png" alt="The in-game console and the Performance, Entities and Assets windows" width="176" height="99"></a><br><sub>Console, performance, entities and assets</sub></td>
<td align="center" valign="top" width="25%"><a href="Documentation/Screenshots/v0.2.0/CollisionAndMovement.png"><img src="Documentation/Screenshots/v0.2.0/CollisionAndMovement.png" alt="Collider bounds, the player box and a box cast with the Renderer, Collisions and Movement windows" width="176" height="99"></a><br><sub>Collider bounds, player box and a box cast</sub></td>
<td align="center" valign="top" width="25%"><a href="Documentation/Screenshots/v0.2.0/WireframeAndDisplay.png"><img src="Documentation/Screenshots/v0.2.0/WireframeAndDisplay.png" alt="Wireframe mode and the Display settings" width="176" height="99"></a><br><sub>Wireframe mode and display settings</sub></td>
</tr>
</table>

### 0.1 — Foundation

<table>
<tr>
<td align="center" valign="top" width="25%"><a href="Documentation/Screenshots/v0.1.0/RotatingCube.png"><img src="Documentation/Screenshots/v0.1.0/RotatingCube.png" alt="A rotating textured cube with the debug overlay" width="176" height="99"></a><br><sub>A textured cube and the debug overlay</sub></td>
</tr>
</table>

## 🛠️ Tech Stack

| Area            | Technology                                                                 |
|-----------------|----------------------------------------------------------------------------|
| Language        | C++23 (MSVC, `/W4 /WX`)                                                    |
| Graphics        | OpenGL 4.6 Core · Direct State Access · GLSL 4.60 · GLAD 2                 |
| Platform        | SDL3 — window, input, gamepads                                             |
| Architecture    | ECS with EnTT                                                              |
| Math            | glm                                                                        |
| Logging         | spdlog                                                                     |
| Debug tools     | Dear ImGui                                                                 |
| Levels          | TrenchBroom + own level compiler                                           |
| Build           | CMake · vcpkg · GitHub Actions                                             |
| Testing         | GoogleTest · CTest                                                         |

## ⬇️ Download

Every finished milestone is published as a ready-to-play build:
**[latest release](https://github.com/demianblogan/Abomination/releases/latest)**.
Unzip the archive anywhere and run `Abomination.exe` — nothing else to install.

Requires **Windows 10/11 (x64)** and a graphics card with **OpenGL 4.6**
support (any GPU from the last ten years with up-to-date drivers).

## 🔨 Building

Requires **Visual Studio 2026** (Desktop development with C++) and
**[vcpkg](https://github.com/microsoft/vcpkg)** with `VCPKG_ROOT` set.

```bash
git clone https://github.com/demianblogan/Abomination.git
```

Open the folder in Visual Studio — it picks up `CMakePresets.json`, vcpkg
fetches all libraries, and **F5** runs the game.
Full instructions, including the command line: **[BUILDING.md](Documentation/BUILDING.md)**.

## 🎮 Controls

The current build (version 0.2) lets you run and jump around a test level made in TrenchBroom,
with Quake-style movement.

| Input | Action |
|:-----:|--------|
| <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd> | Run forward, left, back, right (relative to the view) |
| <kbd>Space</kbd> | Jump |
| Mouse | Look around (while the debug overlay is open: hold the **right mouse button**) |
| <kbd>F2</kbd> | Switch to the free-fly camera and back (fly through walls: <kbd>W</kbd> <kbd>A</kbd> <kbd>S</kbd> <kbd>D</kbd>, <kbd>E</kbd> / <kbd>Q</kbd> up / down, <kbd>Shift</kbd> faster, right mouse button to look) |
| <kbd>F1</kbd> | Show / hide the debug overlay |
| <kbd>~</kbd> | Open / close the in-game console (the log) |
| <kbd>Alt</kbd> + <kbd>Enter</kbd> | Switch between windowed and borderless |
| <kbd>Esc</kbd> | Quit the game |

## 🗺️ Roadmap

| Version | Milestone              | Status |
|:-------:|------------------------|:------:|
| 0.1     | Foundation             | ✅     |
| 0.2     | First Steps            | ✅     |
| 0.3     | Boomstick              | 🔨     |
| 0.4     | It Moves               | ⏳     |
| 0.5     | Lights                 | ⏳     |
| 0.6     | Game Loop              | ⏳     |
| 0.7     | Arsenal & Bestiary     | ⏳     |
| 0.8     | Menus & Saves          | ⏳     |
| 0.9     | Content Complete       | ⏳     |
| **1.0** | **Release**            | ⏳     |

<sub>✅ done · 🔨 in progress · ⏳ planned — details in [ROADMAP.md](Documentation/ROADMAP.md)</sub>

## 📚 Documentation

| Document                                   | What's inside                                         |
|--------------------------------------------|-------------------------------------------------------|
| 🗺️ [Roadmap](Documentation/ROADMAP.md)               | Milestones from 0.1 to 1.0 and the current plan       |
| 🔨 [Building](Documentation/BUILDING.md)           | Requirements and build instructions                   |
| 🏛️ [Architecture](Documentation/ARCHITECTURE.md)     | Modules, dependency rules, main loop, renderer, ECS, world |
| ✍️ [Code Style](Documentation/CODE_STYLE.md)         | Naming, formatting and C++/GLSL conventions           |
| 🌿 [Git Conventions](Documentation/GIT_CONVENTIONS.md) | Branches, commits, pull requests, versions          |
| 🧱 [Level Editing](Documentation/LEVEL_EDITING.md)  | Setting up TrenchBroom and building maps              |
| 🎨 [Art Direction](Documentation/ART_DIRECTION.md)  | Episodes, palettes and rules for textures             |
| 📦 [Assets](Documentation/ASSETS.md)                 | Third-party assets and their licenses                 |
| 🧩 [Third-Party](Documentation/THIRD_PARTY.md)     | Libraries and tools with their licenses               |

## 📜 License

The source code is licensed under the
**[PolyForm Noncommercial License 1.0.0](LICENSE.md)** — you are welcome to
read, study and modify it for any noncommercial purpose.
Third-party assets keep their own licenses, see [ASSETS.md](Documentation/ASSETS.md).

---

<div align="center">

Made with 🩸 by **Alone Bull**

</div>
