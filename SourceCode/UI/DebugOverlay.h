#pragma once

#include "Platform/ImGuiPlatformBackend.h"
#include "Renderer/ImGuiRendererBackend.h"
#include "UI/ImGuiLibrary.h"
#include "UI/Windows/AudioWindow.h"
#include "UI/Windows/ConsoleWindow.h"
#include "UI/Windows/EntitiesWindow.h"

#include <entt/entt.hpp>

#include <cstddef>
#include <expected>
#include <filesystem>
#include <string>

namespace Abomination::Core
{
    class FixedTimestep;
    class FrameLimiter;
    class FrameStatistics;
    class LogHistory;
}

namespace Abomination::Platform
{
    class Window;
}

namespace Abomination::Gameplay
{
    struct GameplayState;
}

namespace Abomination::Renderer
{
    struct RenderAssets;
    struct RenderSettings;
    struct RenderStatistics;
}

namespace Abomination::World
{
    struct LevelMeshStatistics;
    struct CollisionDebugSettings;
    struct CameraCast;
}

namespace Abomination::UI
{
    // Everything the overlay shows or lets the developer change in one frame, gathered in one parameter of Draw().
    // New debug tools add their systems here (the renderer, the entities, ...) instead of adding parameters to Draw().
    // The overlay does not store these references: they are valid only during the Draw() call.
    struct DebugOverlayContext
    {
        const Core::FrameStatistics& frameStatistics;
        const Core::FixedTimestep& fixedTimestep;
        Platform::Window& window;
        Core::FrameLimiter& frameLimiter;
        const Renderer::RenderAssets& renderAssets;
        entt::registry& registry;
        Renderer::RenderSettings& renderSettings;
        const Renderer::RenderStatistics& renderStatistics;
        const World::LevelMeshStatistics& levelStatistics;
        bool& isLevelReloadRequested;
        World::CollisionDebugSettings& collisionSettings;
        const World::CameraCast& cameraCast;
        std::size_t collisionBrushCount;
        Core::LogHistory& logHistory;
        Audio::AudioEngine& audio;

        // The player and their tunable settings: the gameplay windows find the components of the player in the registry
        // (its movement, weapon, view model, recoil).
        Gameplay::GameplayState& gameplay;
    };

    // Developer overlay drawn with Dear ImGui on top of the game: a menu bar with debug windows and settings.
    // Not part of the game interface. Requires a window with loaded OpenGL functions. Move-only.
    class DebugOverlay
    {
    public:
        // The overlay gets only folders and knows the names of its own files itself (FontFileName, SettingsFileName):
        // where the game keeps its folders is decided by the application, which files the overlay needs is not its concern.
        //   assetsDirectory   - the game assets; the overlay font is read from there (the built-in font if it is missing);
        //   settingsDirectory - where the positions and sizes of the debug windows are remembered between runs.
        [[nodiscard]] static std::expected<DebugOverlay, std::string> Create(const Platform::Window& window,
                                                                             const std::filesystem::path& assetsDirectory,
                                                                             const std::filesystem::path& settingsDirectory);

        // Builds the overlay for the current frame and draws it on top of what is already in the back buffer.
        // Must be called once per frame, after the game is drawn and before the buffers are swapped.
        void Draw(const DebugOverlayContext& context);

        // Shows the overlay if it is hidden and hides it if it is shown.
        void ToggleVisibility() noexcept;

        [[nodiscard]] bool IsVisible() const noexcept;

        // Opens or closes the in-game console (see ConsoleWindow). It works while the rest of the overlay is hidden.
        void ToggleConsole() noexcept;

        [[nodiscard]] bool IsConsoleOpen() const noexcept;

    private:
        DebugOverlay(ImGuiLibrary library, Platform::ImGuiPlatformBackend platformBackend,
                     Renderer::ImGuiRendererBackend rendererBackend, std::string GPUName) noexcept;

        // The bar along the top edge of the game window: the Display menu, then one item per debug window that opens and
        // closes it (each window is in a file of its own: PerformanceWindow, AssetsWindow, ...).
        void DrawMainMenuBar(const DebugOverlayContext& context);

        // The Display menu of the menu bar: screen mode, V-Sync, FPS limit and UI scale.
        void DrawDisplayMenu(const DebugOverlayContext& context);

        // Members are destroyed in reverse order of declaration: both backends first, then the ImGui context they use.
        ImGuiLibrary m_library;
        Platform::ImGuiPlatformBackend m_platformBackend;
        Renderer::ImGuiRendererBackend m_rendererBackend;

        // Asked from the driver once: it does not change while the game runs.
        std::string m_GPUName;

        // Hidden at the start in every build, so the game starts playing at once; F1 shows it.
        bool m_isVisible = false;

        // Which debug windows are open. Changed by the View menu and by the close button of each window.
        bool m_isPerformanceWindowOpen = false;
        bool m_isAssetsWindowOpen = false;
        bool m_isEntitiesWindowOpen = false;
        bool m_isRendererWindowOpen = false;
        bool m_isCollisionWindowOpen = false;
        bool m_isPlayerWindowOpen = false;
        bool m_isMovementWindowOpen = false;
        bool m_isAudioWindowOpen = false;
        bool m_isWeaponWindowOpen = false;
        bool m_isEffectsWindowOpen = false;
        bool m_isAnimationWindowOpen = false;
        bool m_isEnemiesWindowOpen = false;

        EntitiesWindow m_entitiesWindow;
        AudioWindow m_audioWindow;
        ConsoleWindow m_consoleWindow;

        // The UI scale chosen in Display > UI scale, and the full scale (with the display scale of Windows) the
        // style was last built for; 0 until the first frame. Not saved between runs yet: settings files come with the
        // Config module (0.8).
        float m_userUIScale = 1.0f;
        float m_appliedScale = 0.0f;
    };
}
