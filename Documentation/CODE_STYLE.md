# Code Style

Rules for all C++ and GLSL code in this repository. Formatting is enforced by
[`.clang-format`](../.clang-format); everything else is enforced by review.
New rules are added to the end of the relevant section when a deviation is
found during review.

## 1. Language and tooling

- **C++23**, MSVC, warning level **`/W4`**, warnings are treated as errors.
- Source files are **UTF-8 without BOM** (`/utf-8` compiler flag).
- Code, comments, identifiers, log messages and docs are in **English**.
- Format with clang-format before committing
  (Visual Studio: *Ctrl+K, Ctrl+D*, or format on save).

## 2. Files and folders

| Item                 | Rule                                  | Example                              |
|----------------------|---------------------------------------|--------------------------------------|
| Header               | `.h`                                  | `ShaderProgram.h`                    |
| Source               | `.cpp`                                | `ShaderProgram.cpp`                  |
| Code file name       | PascalCase, named after its main type | `FlyCamera.h`                        |
| Folder (any)         | PascalCase, full words (see below)    | `SourceCode/Renderer/`, `Assets/Textures/` |
| GLSL                 | PascalCase + stage extension          | `TexturedMesh.vert`, `.frag`         |
| Asset and data file  | PascalCase                            | `Weapons.json`, `RocketLauncher.glb` |
| Tests                | `<TestedFile>Tests.cpp`               | `FlyCameraTests.cpp`                 |

- **Folders use full words, not industry abbreviations**: `SourceCode`, not
  `src`; `Documentation`, not `docs`; `Binaries`, not `bin`. Readability comes
  first. The only exceptions are names fixed by tools (`.github/`) and
  screenshot folders, which are named after the release tag
  (`Documentation/Screenshots/v0.1.0/`).
- **Code in `ThirdParty/` is not ours** and does not follow this guide: its
  file names, formatting and identifiers stay exactly as delivered, so it can
  be replaced by a newer version at any time. Only the folder names follow our
  rules (`ThirdParty/GLAD`).
- One main type per header. Small helper types that belong to it may live in
  the same file.
- Headers use `#pragma once`.
- **Headers contain only declarations; definitions go to the `.cpp`**, even
  for trivial one-line functions such as getters. Exceptions: templates and
  `constexpr` functions (the compiler must see their bodies), and `= default` /
  `= delete`, which are not bodies.

## 3. Naming

| Entity                              | Style                  | Example                            |
|-------------------------------------|------------------------|------------------------------------|
| Namespace                           | PascalCase             | `Abomination::Renderer`            |
| Class, struct, enum, alias, concept | PascalCase             | `ShaderProgram`, `EntityID`        |
| Template parameter                  | PascalCase             | `template <typename Component>`    |
| Function, method (any)              | PascalCase             | `LoadTexture()`, `Update()`        |
| Local variable, parameter           | camelCase              | `deltaTime`, `vertexCount`         |
| Private/protected data member       | `m_` + camelCase       | `m_programID`, `m_health`          |
| Public field of a plain struct      | camelCase, no prefix   | `position`, `health`               |
| Constant (incl. `constexpr`)        | PascalCase, no prefix  | `MaxPointLights`                   |
| Enum class value                    | PascalCase             | `WindowMode::Borderless`           |

- **Abbreviations are always written in capitals**, in every kind of name,
  including local variables: `GLBuffer`, `LoadJSON()`, `HUDLayer`,
  `m_entityID`, `playerHUD`, `JSONText`, `ID`, `SDLLibrary` (a local
  variable), `m_SDLLibrary`.
- **Booleans** (variables, members, parameters and constants) follow one of
  three patterns depending on what they mean:

  | Meaning                         | Pattern                                                              | Examples                                   |
  |---------------------------------|----------------------------------------------------------------------|--------------------------------------------|
  | Describes a state of something  | `is`/`are`/`was`/`were`/`have`/`had` + *noun (optional)* + **adjective** | `isGrounded`, `isDoorOpen`, `areEnemiesAlerted`, `wasLevelCompleted` |
  | An action that has to be done   | `needTo` + **verb**                                                  | `needToSaveSettings`, `NeedToTruncateFile` |
  | An action that has been done    | **noun** + **verb in the past tense**                                | `levelLoaded`, `playerJumped`, `buttonPressed` |
- **Functions and methods start with a verb**: `CreateWindow()`,
  `ApplyDamage()`, `GetLogger()`, `ConvertToString()` — not `LoggerFor()` or
  `ToString()`. Exceptions:
  - functions that check a state and return `bool` follow the boolean
    patterns above (`IsLevelEnabled()`);
  - `FixedUpdate()` — the per-tick update of the main loop, named like in
    Unity because the term is well known in game engines.
- Getters use `Get`, setters use `Set`: member `m_width` → `GetWidth()`,
  `SetWidth(int)`; `SetVSyncEnabled(bool)`.
- Macros are avoided. The exception are the zones of the profiler
  (`Core/Profiling/ProfileZone.h`): only a macro can disappear completely from
  a build without profiling. Our macros are written in `UPPER_SNAKE_CASE` with
  parentheses, like a function call: `PROFILE_ZONE();`,
  `PROFILE_ZONE_NAMED("Swap buffers");`, `PROFILE_FRAME_MARK();`.
  `PROFILE_ZONE()` is the first line of a function body, followed by a blank
  line; a part of a function gets a block `{ PROFILE_ZONE_NAMED("…"); … }`
  with a name in plain words. Tracy's own zone macros (`ZoneScoped`) are not
  used for CPU zones: ours do nothing until the game starts the profiler.
  GPU zones are `PROFILE_GPU_ZONE("Sprites");`
  (`Renderer/OpenGL/GPUProfileZone.h`), in the drawing code only.
- No Hungarian notation (`iCount`, `pData`, `strName`).
- The root namespace is `Abomination`. Every module has a nested namespace
  matching its folder: `SourceCode/Renderer/` → `Abomination::Renderer`.
  The only exception is the class `Application` (folder `Application/`), which
  lives in the root namespace `Abomination` to avoid
  `Abomination::Application::Application`.
- Implementation details that must be visible in a header (for example,
  functions called by a template) go into a nested `Internal` namespace and
  are not called from outside the module. Details that do not need to be in a
  header live in an anonymous namespace in the `.cpp`.
- ECS components are named with a noun, without a `Component` suffix
  (`Transform`, `Spin`); systems are functions named by what they do
  (`UpdateSpinningEntities`, `DrawMeshes`). See ARCHITECTURE.md, section 10.

## 4. Formatting

Handled by clang-format. The main choices:

- 4 spaces, no tabs. Line limit **127** columns. A statement that fits into
  127 columns stays on one line; only a longer one is wrapped.
- **Allman** braces — every brace on its own line.
  Exception: the opening brace of a braced initializer list stays on the same
  line (`std::array names{`, `LogSettings{`). It starts an expression, not a
  block of code, and clang-format cannot move it.
- Contents of a namespace are **indented**.
- Pointer and reference bind to the type: `int* ptr`, `const Mesh& mesh`.
- `if`/`for`/`while` with a **single statement** have no braces; with two or
  more statements braces are required.
- A `return` that follows other statements in the same block is separated
  from them by a blank line (a comment that belongs to the `return` stays
  directly above it). A `return` that is the only statement of its block has
  no blank line.
- An **empty function body** is written as `{}` on its own line under the
  signature (or under the constructor initializer list).
- In a class declaration, the groups *copy operations*, *move operations* and
  *destructor* are separated by blank lines.

```cpp
// ShaderProgram.h
namespace Abomination::Renderer
{
    class ShaderProgram
    {
    public:
        explicit ShaderProgram(std::uint32_t programID) noexcept;

        ShaderProgram(const ShaderProgram&) = delete;
        ShaderProgram& operator=(const ShaderProgram&) = delete;

        ShaderProgram(ShaderProgram&& other) noexcept;
        ShaderProgram& operator=(ShaderProgram&& other) noexcept;

        ~ShaderProgram();

        [[nodiscard]] std::uint32_t GetProgramID() const noexcept;

        void Bind() const;

    private:
        std::uint32_t m_programID = 0;
    };
}

// ShaderProgram.cpp
namespace Abomination::Renderer
{
    ShaderProgram::ShaderProgram(std::uint32_t programID) noexcept
        : m_programID(programID)
    {}

    std::uint32_t ShaderProgram::GetProgramID() const noexcept
    {
        return m_programID;
    }

    void ShaderProgram::Bind() const
    {
        if (m_programID == 0)
            return;

        glUseProgram(m_programID);
    }
}
```

## 5. Includes

Order, separated by a blank line (clang-format sorts inside groups):

1. The matching header (in a `.cpp`).
2. Project headers — quotes, path from `SourceCode/`: `#include "Renderer/Assets/Mesh.h"`.
3. Third-party headers — angle brackets: `#include <glm/glm.hpp>`.
4. Standard library headers: `#include <vector>`.

- Include what you use; do not rely on transitive includes.
- Prefer forward declarations in headers when a full type is not needed.

## 6. Modern C++ usage

**Ownership and resources**

- No raw `new`/`delete`. Use values, `std::unique_ptr`, rarely
  `std::shared_ptr` (only for truly shared ownership).
- Raw pointers and references are **non-owning** only.
- Every resource (OpenGL objects, SDL handles, files) is wrapped in an **RAII**
  type. OpenGL wrappers are **move-only**: copy is deleted, move transfers the
  ID and zeroes the source.

**Functions and classes**

- `const` everything that can be `const`.
- `[[nodiscard]]` on functions whose result must not be ignored.
- `noexcept` on move operations, destructors and trivial getters.
- `override` on overridden virtual functions; `final` when inheritance is not
  intended. Constructors with one argument are `explicit`.
- Default member initializers instead of initializing in constructors when
  possible: `float m_speed = 5.0f;`.
- `auto` when the type is obvious from the right side or is noise
  (iterators, lambdas); spell the type when it carries meaning.
- `enum class` only, never plain `enum`.
- `switch`: no empty `default: break;` — values that are not listed are simply
  skipped anyway. A `default` is written only when it does something (returns
  a fallback value, logs, …). A `switch` over our own `enum class` lists every
  value and has **no** `default`, so the compiler warns when a new value is
  added to the enum and a `switch` misses it.
- Fixed-width integers (`std::uint32_t`, `std::int16_t`) for data with a
  defined size (file formats, GPU data); `int`/`std::size_t` otherwise.
- `std::string_view` / `std::span` for non-owning parameters.
- **Explicit checks, no implicit conversion to `bool`**: pointers (raw and
  smart) are compared with `nullptr`, `std::optional` is checked with
  `has_value()`.
  `if (logger != nullptr)`, `if (texture.has_value())` — not `if (logger)`,
  `if (texture)`.

**Errors**

- **Programmer errors** (broken invariants) → assertions (active in Debug).
  A precondition written in a comment ("ticksPerSecond must be greater than
  0") gets an `assert` at the start of the function when it is cheap to check
  and breaking it would cause NaN, an OpenGL error or memory access out of
  bounds: `assert(ticksPerSecond > 0);`. Checks at compile time use
  `static_assert`.
- **Recoverable errors** (missing file, bad JSON, shader compile error) →
  return `std::expected<T, Error>`.
- **Fatal startup errors** (no OpenGL 4.6) → log and exit with a message box.
- Exceptions are not thrown by our code. Exceptions from third-party libraries
  (e.g. nlohmann-json) are caught at the boundary where the library is used.

**OpenGL**

- OpenGL types and calls (`GLuint`, `gl*`) appear only inside the `Renderer`
  module. The rest of the code talks to the renderer's high-level API
  (see `ARCHITECTURE.md`). There is no graphics-API abstraction layer
  (RHI): OpenGL is the only backend.
- Use **Direct State Access** (OpenGL 4.5+): `glCreateBuffers`,
  `glNamedBufferStorage`, `glTextureStorage2D`, … — not bind-to-edit.

## 7. Comments

- Comments explain **why**, not what. Good names explain what.
- Comments use `//` only (no `///`, no `/* */` for documentation).
- Public API in headers gets a short `//` comment when its behaviour is not
  obvious from the name.
- `// TODO(Module): ...` for known unfinished work; do not leave commented-out
  code.

## 8. Units and coordinate system

- Distances in **meters**, time in **seconds**, angles in **radians**
  (convert degrees only at the boundary: UI, config files).
- World space is **right-handed, Y-up**, −Z is forward — the OpenGL/glm
  convention. Data from other conventions (TrenchBroom is Z-up) is converted
  once, at load time.
- Direction constants say which space they are in: `World…` is an axis of the
  world (`WorldUp`), `Local…` a direction in an object's own coordinates
  (`LocalForward` = (0, 0, −1)), which is turned by the object's rotation to get
  its direction in the world.

## 9. GLSL

- `#version 460 core` at the top of every shader.
- Functions PascalCase, locals and parameters camelCase — same as C++.
- Shader-global variables are PascalCase; a prefix shows where they come
  from:

| Kind                                                      | Style              | Example                                 |
|-----------------------------------------------------------|--------------------|-----------------------------------------|
| Vertex attribute — `in` of the vertex shader (from a VBO) | `a` + PascalCase   | `aPosition`, `aTexCoord`                |
| Uniform (including samplers)                              | `uni` + PascalCase | `uniViewProjection`, `uniAlbedoTexture` |
| `in`/`out` between stages — same name on both sides       | PascalCase         | `TexCoord`, `WorldNormal`               |
| Output of the fragment shader (to the framebuffer)        | PascalCase         | `FragColor`                             |

- Explicit `layout(location = N)` / `layout(binding = N)` on all inputs,
  outputs, samplers and blocks.

```glsl
// TexturedMesh.vert
#version 460 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

layout(location = 0) uniform mat4 uniViewProjection;

layout(location = 0) out vec2 TexCoord;

void main()
{
    TexCoord = aTexCoord;
    gl_Position = uniViewProjection * vec4(aPosition, 1.0);
}
```

```glsl
// TexturedMesh.frag
#version 460 core

layout(location = 0) in vec2 TexCoord;

layout(binding = 0) uniform sampler2D uniAlbedoTexture;

layout(location = 0) out vec4 FragColor;

void main()
{
    FragColor = texture(uniAlbedoTexture, TexCoord);
}
```

## 10. Data files (JSON)

- Keys are **snake_case**: `"move_speed": 7.5`.
- Units follow section 8 unless the key says otherwise (`"fov_degrees": 90`).

## 11. Tests

- GoogleTest. One test file per tested unit: `FlyCameraTests.cpp`.
- `TEST(Suite, Behavior)`: suite is the tested type, behavior describes the
  expected result in PascalCase: `TEST(FlyCamera, MovesForwardAlongViewDirection)`.
- When tests share setup code, a fixture class named `<TestedType>Test` is used
  with `TEST_F`: `class LogTest : public ::testing::Test`. The `Test` suffix
  avoids clashes with a namespace or class of the same name (`Log`).
- Arrange / Act / Assert, separated by blank lines.
- Tests do not need an OpenGL context. Code that needs one is kept thin so
  the logic around it can be tested separately.

## 12. CMake

- Commands, functions and variables of our own follow CMake's convention:
  lowercase snake_case with the `abomination_` prefix for functions
  (`abomination_set_compiler_options`), UPPER_SNAKE for variables
  (`GENERATED_DIR`).
- Target names are PascalCase: `AbominationCore`, `AbominationTests`.
- Source files are listed explicitly — no `file(GLOB ...)`.
- Compiler options and dependencies are set per target
  (`target_compile_options`, `target_link_libraries`), never globally.
