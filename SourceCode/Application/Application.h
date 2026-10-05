#pragma once

#include "Application/LaunchOptions.h"
#include "Audio/AudioEngine.h"
#include "Core/Scene/Transform.h"
#include "Core/Time/FixedTimestep.h"
#include "Core/Time/FrameLimiter.h"
#include "Gameplay/GameplayState.h"
#include "Input/ActionStates.h"
#include "Input/InputBindings.h"
#include "Input/InputDevices.h"
#include "Platform/SDLLibrary.h"
#include "Platform/Window.h"
#include "Renderer/Assets/RenderAssets.h"
#include "Renderer/Camera/View.h"
#include "Renderer/Debug/DebugLineRenderer.h"
#include "Renderer/Debug/DebugLines.h"
#include "Renderer/RenderSettings.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/SkinningBuffer.h"
#include "Renderer/Sprites/SpriteBatch.h"
#include "Renderer/Sprites/SpriteRenderer.h"
#include "UI/DebugOverlay.h"
#include "UI/GameUI.h"
#include "World/CollisionDebug.h"
#include "World/Level.h"

#include <entt/entt.hpp>

#include <expected>
#include <filesystem>
#include <optional>
#include <string>

namespace Abomination::Core
{
    class FrameStatistics;
    class LogHistory;
}

namespace Abomination::World
{
    struct MapData;
}

namespace Abomination
{
    // The top-level object of the game: creates the platform objects and runs the main loop.
    // Lives in the root namespace as the only exception to the "namespace per module" rule,
    // because Abomination::Application::Application would repeat the same word twice.
    class Application
    {
    public:
        // assetsDirectory: the folder with the game files (fonts, shaders, textures), normally next to the executable.
        // logHistory: the last messages of the log (see Core::LogHistory), shown by the in-game console; it must outlive
        // the application.
        // options: what the command line asks for (the benchmark).
        [[nodiscard]] static std::expected<Application, std::string> Create(const std::filesystem::path& assetsDirectory,
                                                                            Core::LogHistory& logHistory,
                                                                            const LaunchOptions& options);

        // Runs the main loop until the window is closed. Returns the exit code of the process.
        [[nodiscard]] int Run();

    private:
        // map: the parsed start map, whose entities the constructor creates. assetsDirectory is kept for loading maps later.
        Application(Platform::SDLLibrary SDLLibrary, Platform::Window window, Audio::AudioEngine audio,
                    Renderer::RenderAssets renderAssets, const World::MapData& map, UI::DebugOverlay debugOverlay,
                    UI::GameUI gameUI, std::filesystem::path assetsDirectory, Core::LogHistory& logHistory,
                    const LaunchOptions& options);

        // The two kinds of updates of the main loop, named like in Unity:
        //   Update()      - once per frame: what must react immediately and does not depend on time
        //                   (debug overlay toggle, mouse capture, turning the camera with the mouse);
        //   FixedUpdate() - once per simulation tick, 0, 1 or several times per frame: everything that moves the world
        //                   forward in time. Always called with the same tickDuration (see Core::FixedTimestep),
        //                   so the result does not depend on the frame rate.
        void Update();
        void FixedUpdate(float tickDuration);

        // Once per frame after the ticks: what only moves the picture (the view, the weapon in the hands, the animated
        // models, the shells and gibs), so it is as smooth as the view. deltaTime: the length of the frame (seconds).
        void UpdateVisuals(float deltaTime);

        // Hot reload of shaders (Debug builds): every ShaderCheckInterval seconds, the programs whose files changed are
        // compiled again (see Renderer::ShaderStore::ReloadChangedPrograms). deltaTime: the length of the frame (seconds).
        void ReloadChangedShaders(float deltaTime);

        // Reads the start map again and replaces the loaded level with it: the old entities and Level assets are removed,
        // the new ones created. The camera stays where it is. If the map cannot be read, the old level stays.
        void ReloadLevel();

        // Draws the frame (the game, then the debug overlay on top) and shows it on the screen.
        // frameStatistics: the numbers for the overlay; deltaTime: the length of the frame (seconds), for the HUD.
        void Render(const Core::FrameStatistics& frameStatistics, float deltaTime);

        // The steps of Render():
        //   DrawEffects   - the marks on the walls and the particles, seen through the view;
        //   AddDebugLines - the lines of the debug tools of this frame (collision, characters, pellets, world axes);
        //   DrawWeaponViewModel - the weapon in the hands with the hands and its muzzle flash, over everything (in the world
        //                         when the free-fly camera looks at the player).
        void DrawEffects(const Renderer::View& view);
        void AddDebugLines(const Core::Transform& cameraTransform, float interpolationFactor);
        void DrawWeaponViewModel(float aspectRatio, const Renderer::View& view, float interpolationFactor);

        // Saves the frame drawn so far (before the buffers are swapped) as BenchmarkScreenshotFileName next to the executable.
        void SaveBenchmarkScreenshot();

        // Members are destroyed in reverse order of declaration: the overlay and the assets first (they use OpenGL),
        // then the window, then SDL, which the window needs.
        Platform::SDLLibrary m_SDLLibrary;
        Platform::Window m_window;

        // The sound card, the loaded sounds and the voices that play them.
        Audio::AudioEngine m_audio;

        // Every graphics asset of the game, loaded once.
        Renderer::RenderAssets m_renderAssets;

        // Shaders the renderer uses on its own (the wireframe, the debug lines), loaded once in the constructor.
        Renderer::SystemShaders m_systemShaders;

        // How the scene is drawn (changed in the Renderer window of the overlay) and what the last frame cost (shown there).
        Renderer::RenderSettings m_renderSettings;
        Renderer::RenderStatistics m_renderStatistics;

        // Lines any code can add during a frame for debugging; drawn over the scene and cleared at the end of Render().
        Renderer::DebugLines m_debugLines;
        Renderer::DebugLineRenderer m_debugLineRenderer;

        // The sprites of a frame (particles, marks, the muzzle flash) and what draws them.
        Renderer::SpriteBatch m_sprites;
        Renderer::SpriteRenderer m_spriteRenderer;

        // The joint matrices of the skinned mesh being drawn (characters bent by their skeletons).
        Renderer::SkinningBuffer m_skinningBuffer;

        // The folder with the game files, for loading maps after startup.
        std::filesystem::path m_assetsDirectory;

        // Not owned: main() owns the history and keeps it alive longer than the application. A pointer, not a reference,
        // so Application stays movable.
        Core::LogHistory* m_logHistory = nullptr;

        // The loaded level (its statistics are shown in the Renderer window).
        World::Level m_level;

        // Set by the Reload button of the Renderer window; the level is reloaded at the start of the next frame, not in the
        // middle of drawing the overlay.
        bool m_isLevelReloadRequested = false;

        // The collision tools of the debug overlay (Collisions in the menu bar) and the camera cast of the last frame.
        World::CollisionDebugSettings m_collisionSettings;
        World::CameraCast m_cameraCast;

        // All entities of the game and their components. Components hold only handles to assets, never pointers, so they
        // stay valid when Application (and with it m_renderAssets) is moved out of Create().
        entt::registry m_registry;

        // The player, the free-fly camera, who of them is controlled and how they move (see Gameplay::GameplayState).
        Gameplay::GameplayState m_gameplay;

        // Time since the last look for changed shader files (see ReloadChangedShaders).
        float m_secondsSinceShaderCheck = 0.0f;

        // The mouse is in relative mode (see Update).
        bool m_isMouseCaptured = false;

        // The player is controlled and nothing needs the cursor: Fire shoots (see Update).
        bool m_canPlayerShoot = false;

        UI::DebugOverlay m_debugOverlay;

        // The game interface (the HUD, later the menus), drawn over the game and under the debug overlay. Declared after the
        // window, so it is destroyed first, while the OpenGL context still exists.
        UI::GameUI m_gameUI;

        // State of the keyboard and the mouse for the current frame: the window fills it, the game reads it.
        Input::InputDevices m_inputDevices;

        // Which keys and buttons trigger which actions, and the state of every action for the current frame.
        Input::InputBindings m_inputBindings = Input::InputBindings::CreateDefault();
        Input::ActionStates m_actionStates;

        // Keeps the frame rate at or below the limit chosen in the debug overlay (no limit by default).
        Core::FrameLimiter m_frameLimiter;

        // How many times per second the simulation runs. 60 is enough for a single-player game; it is one constant,
        // so it can be raised later (120) if movement or physics ever needs finer steps.
        static constexpr int SimulationTicksPerSecond = 60;

        // Splits the time of every frame into simulation ticks of 1 / SimulationTicksPerSecond seconds.
        Core::FixedTimestep m_fixedTimestep{SimulationTicksPerSecond};

        // In a benchmark (--benchmark), the ticks left until the game closes itself; empty in a normal game.
        std::optional<int> m_benchmarkTicksLeft;

        // Set halfway through the benchmark: the next frame is saved (see SaveBenchmarkScreenshot).
        bool m_isBenchmarkScreenshotDue = false;
    };
}
