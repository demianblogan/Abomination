#include "Application/Application.h"

#include "Core/BuildConfiguration.h"
#include "Core/Logging/Log.h"
#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Core/Time/FrameStatistics.h"
#include "Core/Time/FrameTimer.h"
#include "Gameplay/Camera/FreeFlyCameraSystem.h"
#include "Gameplay/Camera/ViewSystem.h"
#include "Gameplay/Characters/TargetDummy.h"
#include "Gameplay/Player/DamageReaction.h"
#include "Gameplay/Player/PlayerSystem.h"
#include "Gameplay/Spin.h"
#include "Gameplay/Weapons/WeaponSystem.h"
#include "Gameplay/Weapons/WeaponViewModel.h"
#include "Platform/SystemServices.h"
#include "Renderer/Camera/View.h"
#include "Renderer/OpenGL/DebugOutput.h"
#include "Renderer/OpenGL/OpenGLLoader.h"
#include "Renderer/OpenGL/RenderCommands.h"
#include "Renderer/RenderSystem.h"
#include "World/Level.h"
#include "World/MapParser.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <chrono>
#include <span>
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

    std::expected<Application, std::string> Application::Create(const std::filesystem::path& assetsDirectory,
                                                                Core::LogHistory& logHistory)
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
            .models = Renderer::ModelStore(assetsDirectory),
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

        std::expected<UI::GameUI, std::string> gameUI = UI::GameUI::Create(*window, assetsDirectory);
        if (!gameUI.has_value())
            return std::unexpected(gameUI.error());

        // Without a sound card the engine still works, silently (see Audio::AudioEngine), so it cannot stop the start.
        Audio::AudioEngine audio(assetsDirectory);

        return Application(std::move(*SDLLibrary), std::move(*window), std::move(audio), std::move(renderAssets), *map,
                           std::move(*debugOverlay), std::move(*gameUI), assetsDirectory, logHistory);
    }

    Application::Application(Platform::SDLLibrary SDLLibrary, Platform::Window window, Audio::AudioEngine audio,
                             Renderer::RenderAssets renderAssets, const World::MapData& map, UI::DebugOverlay debugOverlay,
                             UI::GameUI gameUI, std::filesystem::path assetsDirectory, Core::LogHistory& logHistory)
        : m_SDLLibrary(std::move(SDLLibrary))
        , m_window(std::move(window))
        , m_audio(std::move(audio))
        , m_renderAssets(std::move(renderAssets))
        , m_assetsDirectory(std::move(assetsDirectory))
        , m_logHistory(&logHistory)
        , m_debugOverlay(std::move(debugOverlay))
        , m_gameUI(std::move(gameUI))
    {
        m_systemShaders = Renderer::LoadSystemShaders(m_renderAssets.shaders);

        // The window passes its keyboard and mouse events to the game interface (after ImGui).
        m_window.SetRmlUiBackend(m_gameUI.GetPlatformBackend());

        // Entities are created here, not in Create(): the registry is a member, and the handles the components get from
        // m_renderAssets stay valid because they are numbers, not pointers.
        m_level = World::Level::Create(m_registry, m_renderAssets, map, StartMapPath);

        // The player appears where the map puts them. The free-fly camera waits at their eyes; F2 switches to it.
        m_gameplay = Gameplay::CreateGameplayState(m_registry, m_level.GetPlayerStart(), m_audio, m_renderAssets);
        m_gameplay.targetDummies = Gameplay::SpawnTargetDummies(m_registry, m_renderAssets, m_level.GetTargetDummyStarts());
    }

    int Application::Run()
    {
        Core::Log::Write(LogCategory::Core, LogLevel::Info, "Main loop started");

        const Core::TimePoint loopStartTime = Core::Clock::now();
        Core::FrameTimer frameTimer(loopStartTime);
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
            //    would be lost, so presses that matter (jumping) are collected every frame in Update() (see
            //    Gameplay::PlayerController::CollectFrameInput).
            const int tickCount = m_fixedTimestep.Advance(frameTimer.GetDeltaTime());
            for (int tick = 0; tick < tickCount; ++tick)
                FixedUpdate(m_fixedTimestep.GetTickDuration());

            // What only moves the picture (the weapon in the hands, the view, the particles) moves every frame, after the
            // ticks of this frame, so it is as smooth as the view.
            Gameplay::UpdateViewEffects(m_gameplay, m_registry, frameTimer.GetDeltaTime());
            Gameplay::UpdateDamageReaction(m_gameplay, m_registry, m_audio, frameTimer.GetDeltaTime());

            frameStatistics.AddFrame(frameTimer.GetDeltaTime(), tickCount);

            // 4. Drawing and showing the frame.
            Render(frameStatistics, frameTimer.GetDeltaTime());

            // 5. With an FPS limit, the frame waits here until it has lasted 1 / limit seconds. The next frame then
            //    starts right on time, and its measured delta time includes this wait.
            Platform::SleepPrecisely(m_frameLimiter.GetWaitTime(frameStartTime, Core::Clock::now()));
        }

        // How long the game really ran, for the log file: when a log ends in a bug, it shows whether it came right after the
        // start or hours later. The difference of two time points, not a sum of delta times: FrameTimer counts a long frame
        // (a breakpoint, a dragged window) as only MaxDeltaTime.
        const double sessionSeconds = std::chrono::duration<double>(Core::Clock::now() - loopStartTime).count();
        Core::Log::Write(LogCategory::Core, LogLevel::Info, "Main loop finished after {:.1f} seconds", sessionSeconds);

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

        if (m_actionStates.WasActionStarted(Input::Action::ToggleConsole))
            m_debugOverlay.ToggleConsole();

        if (m_actionStates.WasActionStarted(Input::Action::ToggleFreeFlyCamera))
            Gameplay::ToggleFreeFlyCamera(m_gameplay, m_registry);

        // The mouse is captured (hidden, locked inside the window, reporting only movement) while it turns a view: always
        // while playing, unless the debug overlay or the console is open and needs the cursor; and while LookAroundMode
        // is active (the right mouse button), like in the Unity and Unreal editors. Capturing is done here because the
        // window belongs to the application; the controllers only turn views.
        const bool isPlayerControlled = m_gameplay.controlMode == Gameplay::ControlMode::Player;
        const bool isPlaying = isPlayerControlled && !m_debugOverlay.IsVisible() && !m_debugOverlay.IsConsoleOpen();
        const bool shouldCaptureMouse = isPlaying || m_actionStates.IsActionActive(Input::Action::LookAroundMode);

        // Switching into relative mode can report one big jump of movement in that frame: it is not used for turning.
        const bool isCaptureStarting = shouldCaptureMouse && !m_isMouseCaptured;
        if (shouldCaptureMouse != m_isMouseCaptured)
        {
            m_window.SetRelativeMouseMode(shouldCaptureMouse);
            m_isMouseCaptured = shouldCaptureMouse;
        }

        // Turning follows the mouse every frame, not in ticks: it uses the mouse movement of this frame, which does not
        // depend on time. In ticks, the movement of a frame without ticks would be lost and applied twice in a frame
        // with two ticks. Only what is controlled now turns.
        const bool doesMouseTurnPlayer = m_isMouseCaptured && !isCaptureStarting;
        Gameplay::UpdatePlayerLook(m_gameplay, m_registry, m_actionStates,
                                   doesMouseTurnPlayer ? m_inputDevices.mouse.GetMovement() : glm::vec2(0.0f));
        Gameplay::UpdateFreeFlyCameraLook(m_gameplay, m_registry, m_actionStates, m_inputDevices.mouse);

        // The player shoots only while playing: a click on the debug overlay or in the console is not a shot.
        m_canPlayerShoot = isPlaying;
        Gameplay::CollectWeaponInput(m_gameplay, m_registry, m_actionStates, m_canPlayerShoot);
    }

    void Application::FixedUpdate(float tickDuration)
    {
        // First of all: remember where every interpolated entity is before this tick moves anything.
        Core::StorePreviousTransforms(m_registry);

        const std::span<const World::CollisionBrush> brushes = m_level.GetCollisionBrushes();
        Gameplay::UpdatePlayer(m_gameplay, m_registry, m_actionStates, brushes, tickDuration);
        Gameplay::UpdatePlayerSounds(m_gameplay, m_registry, m_audio);
        Gameplay::UpdateWeapon(m_gameplay, m_registry, m_actionStates, m_canPlayerShoot, brushes, m_audio, tickDuration);
        Gameplay::UpdateFreeFlyCamera(m_gameplay, m_registry, m_actionStates, brushes,
                                      m_collisionSettings.doesCameraCollide, tickDuration);
        Gameplay::UpdateTargetDummies(m_registry, m_renderAssets, brushes, m_gameplay.physicsSettings,
                                      m_gameplay.movementSettings, tickDuration);
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

        // The dummies first: their models belong to the level and are removed with it.
        Gameplay::DestroyTargetDummies(m_registry, m_gameplay.targetDummies);
        m_level.Unload(m_registry, m_renderAssets);
        Gameplay::ClearEffects(m_gameplay.effects);
        m_level = World::Level::Create(m_registry, m_renderAssets, *map, StartMapPath);
        m_gameplay.targetDummies = Gameplay::SpawnTargetDummies(m_registry, m_renderAssets, m_level.GetTargetDummyStarts());
    }

    void Application::Render(const Core::FrameStatistics& frameStatistics, float deltaTime)
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
                Gameplay::CalculateViewTransform(m_gameplay, m_registry, interpolationFactor);
            const Renderer::View view =
                Renderer::CalculateView(cameraTransform, Gameplay::GetViewLens(m_gameplay, m_registry), aspectRatio);

            // The ears are where the eyes are: 3D sounds are heard from the place the scene is seen from.
            m_audio.SetListener(cameraTransform.position, cameraTransform.rotation * Core::LocalForward);

            // In the order things cover each other: the solid world, the see-through effects in it, the debug lines, the
            // weapon in the hands, and the game interface over everything.
            m_renderStatistics = Renderer::DrawMeshes(m_registry, view, interpolationFactor, m_renderAssets,
                                                      m_systemShaders, m_renderSettings);
            DrawEffects(view);

            AddDebugLines(cameraTransform, interpolationFactor);
            const glm::vec2 viewportSize(static_cast<float>(widthInPixels), static_cast<float>(heightInPixels));

            // The line width is given at 100% display scale, like the debug overlay: on a 4K monitor at 200% it doubles.
            m_debugLineRenderer.Draw(m_debugLines, view, m_renderAssets.shaders.Get(m_systemShaders.debugLines), viewportSize,
                                     DebugLineWidth * m_window.GetDisplayScale());

            DrawWeaponViewModel(aspectRatio);

            // The game interface over everything (the HUD); it gets the mouse only while the cursor is free (the debug overlay
            // is open).
            m_gameUI.UpdateHUD(m_gameplay, m_registry, viewportSize, deltaTime);
            m_gameUI.Update(viewportSize, !m_isMouseCaptured);
            m_renderStatistics.drawCallCount +=
                m_gameUI.Render(viewportSize, m_renderAssets.shaders.Get(m_systemShaders.gameUI));
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
            .cameraCast = m_cameraCast,
            .collisionBrushCount = m_level.GetCollisionBrushes().size(),
            .logHistory = *m_logHistory,
            .audio = m_audio,
            .gameplay = m_gameplay,
        });

        m_window.SwapBuffers();
    }

    void Application::DrawEffects(const Renderer::View& view)
    {
        // The see-through things after the solid world: the marks on the walls and the particles.
        m_sprites.Clear();
        Gameplay::AddDecalSprites(m_gameplay.effects, m_sprites);
        m_gameplay.effects.particles.AddSprites(m_sprites);
        m_renderStatistics.drawCallCount += m_spriteRenderer.Draw(m_sprites, view.viewMatrix, view.projectionMatrix,
                                                                  m_renderAssets.textures,
                                                                  m_renderAssets.shaders.Get(m_systemShaders.sprites));
    }

    void Application::AddDebugLines(const Core::Transform& cameraTransform, float interpolationFactor)
    {
        m_cameraCast =
            World::UpdateCollisionDebug(m_level.GetCollisionBrushes(), m_collisionSettings, cameraTransform, m_debugLines);

        Gameplay::AddPlayerDebugBox(m_gameplay, m_registry, interpolationFactor, m_debugLines);
        if (m_collisionSettings.areColliderBoundsVisible)
            Gameplay::AddCharacterDebugBoxes(m_gameplay, m_registry, interpolationFactor, m_debugLines);
        Gameplay::AddWeaponDebugLines(m_gameplay, m_registry, m_debugLines);

        if (m_renderSettings.areWorldAxesVisible)
        {
            constexpr glm::vec3 Origin{0.0f};
            constexpr auto AlwaysVisible = Renderer::DebugLineDepth::AlwaysVisible;
            m_debugLines.AddArrow(Origin, {1.0f, 0.0f, 0.0f}, {1.0f, 0.2f, 0.2f}, AlwaysVisible);
            m_debugLines.AddArrow(Origin, {0.0f, 1.0f, 0.0f}, {0.2f, 1.0f, 0.2f}, AlwaysVisible);
            m_debugLines.AddArrow(Origin, {0.0f, 0.0f, 1.0f}, {0.3f, 0.5f, 1.0f}, AlwaysVisible);
        }
    }

    void Application::DrawWeaponViewModel(float aspectRatio)
    {
        // Only while the player is controlled: the free-fly camera has no hands.
        const auto* weaponViewModel = m_registry.try_get<Gameplay::WeaponViewModel>(m_gameplay.player);
        if (weaponViewModel == nullptr || m_gameplay.controlMode != Gameplay::ControlMode::Player)
            return;

        const Renderer::RenderStatistics statistics =
            Renderer::DrawWeaponViewModel(weaponViewModel->model, Gameplay::CalculateWeaponViewModelMatrix(*weaponViewModel),
                                          weaponViewModel->verticalFOV, aspectRatio, m_renderAssets,
                                          weaponViewModel->shaderProgram, m_systemShaders, m_renderSettings);
        m_renderStatistics.drawCallCount += statistics.drawCallCount;
        m_renderStatistics.triangleCount += statistics.triangleCount;

        if (weaponViewModel->flashTimeLeft <= 0.0f)
            return;

        // The muzzle flash, drawn with the weapon: in the space of the eyes (no view matrix) with the projection of the
        // weapon, so it sits exactly at the muzzle.
        const Gameplay::Effects& effects = m_gameplay.effects;
        m_sprites.Clear();
        m_sprites.AddBillboard(Gameplay::CalculateWeaponViewModelMuzzle(*weaponViewModel), effects.settings.flashHalfSize,
                               weaponViewModel->flashRotation, glm::vec4(1.0f), effects.textures.muzzleFlash,
                               Renderer::SpriteBlend::Additive);
        const glm::mat4 projection = Renderer::CalculateWeaponViewModelProjection(weaponViewModel->verticalFOV, aspectRatio);
        m_renderStatistics.drawCallCount +=
            m_spriteRenderer.Draw(m_sprites, glm::mat4(1.0f), projection, m_renderAssets.textures,
                                  m_renderAssets.shaders.Get(m_systemShaders.sprites));
    }
}
