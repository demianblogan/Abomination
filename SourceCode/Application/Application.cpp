#include "Application/Application.h"

#include "Core/BuildConfiguration.h"
#include "Core/FrameStatistics.h"
#include "Core/FrameTimer.h"
#include "Core/Log.h"
#include "Core/Transform.h"
#include "Core/TransformInterpolation.h"
#include "Gameplay/Spin.h"
#include "Platform/SystemServices.h"
#include "Renderer/DebugOutput.h"
#include "Renderer/OpenGLLoader.h"
#include "Renderer/CameraLens.h"
#include "Renderer/RenderCommands.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/View.h"
#include "World/Level.h"
#include "World/MapParser.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <utility>

namespace Abomination
{
    using Core::LogCategory;
    using Core::LogLevel;

    namespace
    {
        // A neutral dark gray, so the colors of the scene are easy to judge.
        constexpr glm::vec4 BackgroundColor{0.12f, 0.12f, 0.13f, 1.0f};

        // The map loaded at start, relative to the assets folder. Its meshes are named after it in the mesh store
        // ("Maps/Test.map#Episode1/Wall_MossyBrick").
        const std::string StartMapPath = "Maps/Test.map";

        // The width of debug lines in pixels at 100% display scale.
        constexpr float DebugLineWidth = 2.5f;
    }

    std::expected<Application, std::string> Application::Create(const std::filesystem::path& assetsDirectory)
    {
        std::expected<Platform::SDLLibrary, std::string> SDLLibrary = Platform::SDLLibrary::Initialize();
        if (!SDLLibrary.has_value())
            return std::unexpected(SDLLibrary.error());

        std::expected<Platform::Window, std::string> window = Platform::Window::Create(Platform::WindowSettings{});
        if (!window.has_value())
            return std::unexpected(window.error());

        // The window has created the OpenGL context, so the OpenGL functions can be loaded now.
        std::expected<void, std::string> loadingResult = Renderer::LoadOpenGLFunctions();
        if (!loadingResult.has_value())
            return std::unexpected(loadingResult.error());

        if constexpr (Core::IsDebugBuild)
            Renderer::EnableDebugOutput();

        std::expected<Renderer::ShaderStore, std::string> shaders = Renderer::ShaderStore::Create(assetsDirectory);
        if (!shaders.has_value())
            return std::unexpected(shaders.error());

        Renderer::RenderAssets renderAssets{
            .textures = Renderer::TextureStore(assetsDirectory),
            .shaders = std::move(*shaders),
            .meshes = Renderer::MeshStore(),
        };

        // The map is only read here; its entities are created in the constructor, where the registry exists.
        std::expected<World::MapData, std::string> map = World::LoadMapFile(assetsDirectory / StartMapPath);
        if (!map.has_value())
            return std::unexpected(map.error());

        // The overlay reads its font from the assets and keeps its window settings next to the executable, like the log.
        std::expected<UI::DebugOverlay, std::string> debugOverlay =
            UI::DebugOverlay::Create(*window, assetsDirectory, Platform::GetExecutableDirectory());
        if (!debugOverlay.has_value())
            return std::unexpected(debugOverlay.error());

        return Application(std::move(*SDLLibrary), std::move(*window), std::move(renderAssets), *map,
                           std::move(*debugOverlay), assetsDirectory);
    }

    Application::Application(Platform::SDLLibrary SDLLibrary, Platform::Window window, Renderer::RenderAssets renderAssets,
                             const World::MapData& map, UI::DebugOverlay debugOverlay,
                             std::filesystem::path assetsDirectory)
        : m_SDLLibrary(std::move(SDLLibrary))
        , m_window(std::move(window))
        , m_renderAssets(std::move(renderAssets))
        , m_assetsDirectory(std::move(assetsDirectory))
        , m_debugOverlay(std::move(debugOverlay))
    {
        m_systemShaders = Renderer::LoadSystemShaders(m_renderAssets.shaders);

        // Entities are created here, not in Create(): the registry is a member, and the handles the components get from
        // m_renderAssets stay valid because they are numbers, not pointers.
        m_level = World::Level::Create(m_registry, m_renderAssets, map, StartMapPath);

        // The camera starts where the map puts the player, at the height of the player's eyes.
        const World::PlayerStart& playerStart = m_level.GetPlayerStart();
        m_camera = Gameplay::SpawnFreeFlyCamera(m_registry, playerStart.eyePosition, playerStart.yaw);
    }

    int Application::Run()
    {
        Core::Log::Write(LogCategory::Core, LogLevel::Info, "Main loop started");

        Core::FrameTimer frameTimer(Core::Clock::now());
        Core::FrameStatistics frameStatistics;

        // One iteration is one frame.
        while (!m_window.IsCloseRequested())
        {
            const Core::TimePoint frameStartTime = Core::Clock::now();
            frameTimer.StartFrame(frameStartTime);

            // 1. Input: first the devices get this frame's input, then the actions are calculated from them.
            m_window.ProcessEvents(m_inputDevices);
            m_actionStates.Update(m_inputDevices, m_inputBindings);

            // 2. Everything that happens once per frame.
            Update();

            // 3. The simulation in fixed ticks: 0, 1 or several per frame, depending on how long the frame was.
            //    The ticks of this frame read the input of this frame. A frame without ticks does not lose held keys
            //    (they are still held in the next frame), but a short press that starts and stops between two ticks
            //    would be lost; it does not matter for flying, and will be handled for jumping (0.2, player movement).
            const int tickCount = m_fixedTimestep.Advance(frameTimer.GetDeltaTime());
            for (int tick = 0; tick < tickCount; ++tick)
                FixedUpdate(m_fixedTimestep.GetTickDuration());

            frameStatistics.AddFrame(frameTimer.GetDeltaTime(), tickCount);

            // 4. Drawing and showing the frame.
            Render(frameStatistics);

            // 5. With an FPS limit, the frame waits here until it has lasted 1 / limit seconds. The next frame then
            //    starts right on time, and its measured delta time includes this wait.
            Platform::SleepPrecisely(m_frameLimiter.GetWaitTime(frameStartTime, Core::Clock::now()));
        }

        Core::Log::Write(LogCategory::Core, LogLevel::Info, "Main loop finished after {:.1f} seconds",
                         frameTimer.GetTotalTime());

        return 0;
    }

    void Application::Update()
    {
        if (m_isLevelReloadRequested)
        {
            m_isLevelReloadRequested = false;
            ReloadLevel();
        }

        // Actions of the application itself (not of the game), so they are handled here. Quitting only asks the window to
        // close: the frame is finished as usual and the main loop ends before the next one.
        if (m_actionStates.WasActionStarted(Input::Action::Quit))
            m_window.RequestClose();

        // Alt+Enter, like in most Windows games: windowed <-> borderless. Exclusive fullscreen is chosen only in the menu, so
        // from it the shortcut goes to windowed.
        if (m_actionStates.WasActionStarted(Input::Action::ToggleScreenMode))
        {
            const bool isWindowed = m_window.GetScreenMode() == Platform::ScreenMode::Windowed;
            m_window.SetScreenMode(isWindowed ? Platform::ScreenMode::Borderless : Platform::ScreenMode::Windowed);
        }

        if (m_actionStates.WasActionStarted(Input::Action::ToggleDebugOverlay))
            m_debugOverlay.ToggleVisibility();

        // While LookAroundMode is active (the right mouse button by default), the mouse is captured for looking around,
        // like in the Unity and Unreal editors. The mode is switched only when the action starts or stops.
        // Capturing is done here because the window belongs to the application; the controller only turns the camera.
        if (m_actionStates.WasActionStarted(Input::Action::LookAroundMode))
            m_window.SetRelativeMouseMode(true);
        if (m_actionStates.WasActionStopped(Input::Action::LookAroundMode))
            m_window.SetRelativeMouseMode(false);

        // Turning follows the mouse every frame, not in ticks: it uses the mouse movement of this frame, which does not
        // depend on time. In ticks, the movement of a frame without ticks would be lost and applied twice in a frame
        // with two ticks.
        Core::Transform& cameraTransform = m_registry.get<Core::Transform>(m_camera);
        m_cameraController.UpdateRotation(m_registry.get<Gameplay::FreeFlyCamera>(m_camera), cameraTransform,
                                          m_actionStates, m_inputDevices.mouse);

        // The rotation from the mouse is already up to date in this frame, so it must not be interpolated between ticks:
        // drawing a rotation between the last two ticks would make the view lag behind the mouse. Setting the previous
        // rotation to the current one makes the interpolation give exactly the current rotation, while the position
        // (changed in ticks) is still interpolated.
        m_registry.get<Core::PreviousTransform>(m_camera).value.rotation = cameraTransform.rotation;
    }

    void Application::FixedUpdate(float tickDuration)
    {
        // First of all: remember where every interpolated entity is before this tick moves anything.
        Core::StorePreviousTransforms(m_registry);

        Core::Transform& cameraTransform = m_registry.get<Core::Transform>(m_camera);
        const glm::vec3 cameraPositionBefore = cameraTransform.position;
        m_cameraController.UpdateMovement(cameraTransform, m_actionStates, tickDuration);

        // A colliding camera moves only as far as its box gets. It stops at walls instead of sliding along them (sliding
        // comes with the movement of the player). A camera that starts inside a brush (the tool was switched on in a
        // wall) moves freely, so it can get out.
        if (m_collisionSettings.doesCameraCollide)
        {
            const World::TraceResult trace =
                World::TraceBox(m_level.GetCollisionBrushes(), glm::dvec3(cameraPositionBefore),
                                glm::dvec3(cameraTransform.position), World::CameraHalfExtents);
            if (!trace.startsInSolid)
                cameraTransform.position = glm::vec3(trace.endPosition);
        }

        Gameplay::UpdateSpinningEntities(m_registry, tickDuration);
    }

    void Application::ReloadLevel()
    {
        // The new map is read before anything is removed: a map with an error (for example saved in the middle of editing)
        // leaves the current level as it is.
        std::expected<World::MapData, std::string> map = World::LoadMapFile(m_assetsDirectory / StartMapPath);
        if (!map.has_value())
        {
            Core::Log::Write(LogCategory::World, LogLevel::Error, "Level not reloaded: {}", map.error());

            return;
        }

        m_level.Unload(m_registry, m_renderAssets);
        m_level = World::Level::Create(m_registry, m_renderAssets, *map, StartMapPath);
    }

    void Application::Render(const Core::FrameStatistics& frameStatistics)
    {
        const int widthInPixels = m_window.GetWidthInPixels();
        const int heightInPixels = m_window.GetHeightInPixels();

        Renderer::SetViewport(widthInPixels, heightInPixels);
        Renderer::ClearFrame(BackgroundColor);

        // Nothing drawn, nothing counted: a minimized window shows zeros in the Renderer window.
        m_renderStatistics = {};

        // A minimized window has a height of 0: there is nothing to draw, and the aspect ratio would divide by zero.
        if (widthInPixels > 0 && heightInPixels > 0)
        {
            const float interpolationFactor = m_fixedTimestep.GetInterpolationFactor();
            const float aspectRatio = static_cast<float>(widthInPixels) / static_cast<float>(heightInPixels);

            // The camera is drawn from where it is between the last two ticks, like every other interpolated entity.
            const Core::Transform cameraTransform =
                Core::InterpolateTransform(m_registry.get<Core::PreviousTransform>(m_camera).value,
                                           m_registry.get<Core::Transform>(m_camera), interpolationFactor);
            const Renderer::View view =
                Renderer::CalculateView(cameraTransform, m_registry.get<Renderer::CameraLens>(m_camera), aspectRatio);

            m_renderStatistics = Renderer::DrawMeshes(m_registry, view, interpolationFactor, m_renderAssets,
                                                      m_systemShaders, m_renderSettings);

            m_cameraTrace = World::UpdateCollisionDebug(m_level.GetCollisionBrushes(), m_collisionSettings,
                                                        cameraTransform, m_debugLines);

            if (m_renderSettings.areWorldAxesVisible)
            {
                constexpr glm::vec3 Origin{0.0f};
                constexpr auto OnTop = Renderer::DebugLineDepth::OnTop;
                m_debugLines.AddArrow(Origin, {1.0f, 0.0f, 0.0f}, {1.0f, 0.2f, 0.2f}, OnTop);
                m_debugLines.AddArrow(Origin, {0.0f, 1.0f, 0.0f}, {0.2f, 1.0f, 0.2f}, OnTop);
                m_debugLines.AddArrow(Origin, {0.0f, 0.0f, 1.0f}, {0.3f, 0.5f, 1.0f}, OnTop);
            }

            // The line width is given at 100% display scale, like the debug overlay: on a 4K monitor at 200% it doubles.
            const glm::vec2 viewportSize(static_cast<float>(widthInPixels), static_cast<float>(heightInPixels));
            m_debugLineRenderer.Draw(m_debugLines, view, m_renderAssets.shaders.Get(m_systemShaders.debugLines), viewportSize,
                                     DebugLineWidth * m_window.GetDisplayScale());
        }

        // The lines of this frame are drawn (or, in a minimized window, dropped); the next frame adds its own.
        m_debugLines.Clear();

        // The overlay is drawn last, on top of the game.
        m_debugOverlay.Draw({
            .frameStatistics = frameStatistics,
            .fixedTimestep = m_fixedTimestep,
            .window = m_window,
            .frameLimiter = m_frameLimiter,
            .renderAssets = m_renderAssets,
            .registry = m_registry,
            .renderSettings = m_renderSettings,
            .renderStatistics = m_renderStatistics,
            .levelStatistics = m_level.GetStatistics(),
            .isLevelReloadRequested = m_isLevelReloadRequested,
            .collisionSettings = m_collisionSettings,
            .cameraTrace = m_cameraTrace,
            .collisionBrushCount = m_level.GetCollisionBrushes().size(),
        });

        m_window.SwapBuffers();
    }
}
