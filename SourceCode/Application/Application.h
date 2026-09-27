#pragma once

#include "Core/FixedTimestep.h"
#include "Core/FrameLimiter.h"
#include "Core/Transform.h"
#include "Gameplay/FreeFlyCameraController.h"
#include "Gameplay/PlayerController.h"
#include "Physics/CharacterMovement.h"
#include "Input/ActionStates.h"
#include "Input/InputBindings.h"
#include "Input/InputDevices.h"
#include "Platform/SDLLibrary.h"
#include "Platform/Window.h"
#include "Renderer/CameraLens.h"
#include "Renderer/RenderAssets.h"
#include "Renderer/DebugLineRenderer.h"
#include "Renderer/DebugLines.h"
#include "Renderer/RenderSettings.h"
#include "Renderer/RenderSystem.h"
#include "UI/DebugOverlay.h"
#include "World/CollisionDebug.h"
#include "World/Level.h"

#include <entt/entt.hpp>

#include <expected>
#include <filesystem>
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
        [[nodiscard]] static std::expected<Application, std::string> Create(const std::filesystem::path& assetsDirectory,
                                                                            Core::LogHistory& logHistory);

        // Runs the main loop until the window is closed. Returns the exit code of the process.
        [[nodiscard]] int Run();

    private:
        // map: the parsed start map, whose entities the constructor creates. assetsDirectory is kept for loading maps later.
        Application(Platform::SDLLibrary SDLLibrary, Platform::Window window, Renderer::RenderAssets renderAssets,
                    const World::MapData& map, UI::DebugOverlay debugOverlay, std::filesystem::path assetsDirectory,
                    Core::LogHistory& logHistory);

        // The two kinds of updates of the main loop, named like in Unity:
        //   Update()      - once per frame: what must react immediately and does not depend on time
        //                   (debug overlay toggle, mouse capture, turning the camera with the mouse);
        //   FixedUpdate() - once per simulation tick, 0, 1 or several times per frame: everything that moves the world
        //                   forward in time. Always called with the same tickDuration (see Core::FixedTimestep),
        //                   so the result does not depend on the frame rate.
        void Update();
        void FixedUpdate(float tickDuration);

        // Reads the start map again and replaces the loaded level with it: the old entities and Level assets are removed,
        // the new ones created. The camera stays where it is. If the map cannot be read, the old level stays.
        void ReloadLevel();

        // Switches between the player and the free-fly camera. The free-fly camera starts at the eyes of the player, looking
        // the same way, so the view does not jump.
        void ToggleFreeFlyCamera();

        // Where the scene is drawn from this frame: the eyes of the player or the free-fly camera, between their last two
        // ticks (see Core::InterpolateTransform).
        [[nodiscard]] Core::Transform CalculateViewTransform(float interpolationFactor) const;

        // The camera lens of the entity the scene is drawn through.
        [[nodiscard]] const Renderer::CameraLens& GetViewLens() const;

        // Draws the frame (the game, then the debug overlay on top) and shows it on the screen.
        // frameStatistics: the numbers for the overlay.
        void Render(const Core::FrameStatistics& frameStatistics);

        // Members are destroyed in reverse order of declaration: the overlay and the assets first (they use OpenGL),
        // then the window, then SDL, which the window needs.
        Platform::SDLLibrary m_SDLLibrary;
        Platform::Window m_window;

        // Every graphics asset of the game, loaded once.
        Renderer::RenderAssets m_renderAssets;

        // Shaders the render system uses on its own (the wireframe), loaded once in the constructor.
        Renderer::SystemShaders m_systemShaders;

        // How the scene is drawn (changed in the Renderer window of the overlay) and what the last frame cost (shown there).
        Renderer::RenderSettings m_renderSettings;
        Renderer::RenderStatistics m_renderStatistics;

        // Lines any code can add during a frame for debugging; drawn over the scene and cleared at the end of Render().
        Renderer::DebugLines m_debugLines;
        Renderer::DebugLineRenderer m_debugLineRenderer;

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

        // The collision tools of the debug overlay (View > Collision) and the camera trace of the last frame.
        World::CollisionDebugSettings m_collisionSettings;
        World::CameraTrace m_cameraTrace;

        // All entities of the game and their components. Components hold only handles to assets, never pointers, so they
        // stay valid when Application (and with it m_renderAssets) is moved out of Create().
        entt::registry m_registry;

        // Who the input controls and whose eyes the scene is drawn through: the player, or the free-fly camera (F2).
        enum class ControlMode
        {
            Player,
            FreeFlyCamera,
        };
        ControlMode m_controlMode = ControlMode::Player;

        // Gravity and other settings of the physical world.
        Physics::PhysicsSettings m_physicsSettings;

        // How the player walks (speed, acceleration, friction, steps).
        Physics::MovementSettings m_movementSettings;

        // The mouse is in relative mode (see Update).
        bool m_isMouseCaptured = false;

        // The player entity (an entity number: its components live in m_registry) and the controller that turns input into
        // what the player does.
        entt::entity m_player = entt::null;
        Gameplay::PlayerController m_playerController;

        // The free-fly camera entity, a debug "noclip" view that flies through walls, and the controller that flies it.
        entt::entity m_freeFlyCamera = entt::null;
        Gameplay::FreeFlyCameraController m_freeFlyCameraController;
        UI::DebugOverlay m_debugOverlay;

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
    };
}
