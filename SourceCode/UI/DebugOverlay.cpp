#include "UI/DebugOverlay.h"

#include "Core/Time/FrameLimiter.h"
#include "Platform/Window.h"
#include "Renderer/OpenGL/OpenGLLoader.h"
#include "UI/AssetsWindow.h"
#include "UI/CollisionWindow.h"
#include "UI/MovementWindow.h"
#include "UI/PerformanceWindow.h"
#include "UI/RendererWindow.h"
#include "UI/ViewModelWindow.h"

#include <imgui.h>

#include <array>
#include <cstddef>
#include <format>
#include <string_view>
#include <utility>

namespace Abomination::UI
{
    namespace
    {
        // The files of the overlay, relative to the folders given to Create().
        constexpr std::string_view FontFileName = "Fonts/JetBrainsMonoRegular.ttf";
        constexpr std::string_view SettingsFileName = "DebugOverlay.ini";

        // Height of all overlay text in pixels. Widgets that contain text (buttons, fields, headers, menu items) grow with
        // it; sizes given in pixels (the graph width, the first size of a window) stay as they are.
        // The game interface (menus, HUD) will have its own fonts.
        constexpr float FontSize = 18.0f;

        // The limits offered in Settings > Display > FPS limit; 0 means no limit. They are chosen for testing, not for
        // players: with the simulation running at 60 ticks per second,
        //   15, 30 - a slow computer: 4 and 2 ticks in every frame;
        //   60     - exactly 1 tick in every frame;
        //   120    - an even pattern: 0, 1, 0, 1 ticks per frame;
        //   144    - an uneven pattern (0, 0, 1, 0, 1, ...), where movement stutters without interpolation;
        //   240    - common fast monitors, many frames without a tick.
        constexpr std::array FPSLimits{0, 15, 30, 60, 120, 144, 240};

        // The UI scales offered in Settings > Display > UI scale. They multiply the display scale of Windows: for a screen
        // whose Windows setting does not match how far away it is (a 4K TV at 100%, seen from the sofa).
        constexpr std::array UserUIScales{0.75f, 1.0f, 1.25f, 1.5f, 2.0f};
    }

    std::expected<DebugOverlay, std::string> DebugOverlay::Create(const Platform::Window& window,
                                                                  const std::filesystem::path& assetsDirectory,
                                                                  const std::filesystem::path& settingsDirectory)
    {
        // The order matters: the context first, then the backends that register themselves in it.
        std::expected<ImGuiLibrary, std::string> library =
            ImGuiLibrary::Initialize(assetsDirectory / FontFileName, FontSize, settingsDirectory / SettingsFileName);
        if (!library.has_value())
            return std::unexpected(library.error());

        std::expected<Platform::ImGuiPlatformBackend, std::string> platformBackend =
            Platform::ImGuiPlatformBackend::Initialize(window);
        if (!platformBackend.has_value())
            return std::unexpected(platformBackend.error());

        std::expected<Renderer::ImGuiRendererBackend, std::string> rendererBackend =
            Renderer::ImGuiRendererBackend::Initialize();
        if (!rendererBackend.has_value())
            return std::unexpected(rendererBackend.error());

        return DebugOverlay(std::move(*library), std::move(*platformBackend), std::move(*rendererBackend),
                            Renderer::GetGraphicsDeviceInfo().GPUName);
    }

    DebugOverlay::DebugOverlay(ImGuiLibrary library, Platform::ImGuiPlatformBackend platformBackend,
                               Renderer::ImGuiRendererBackend rendererBackend, std::string GPUName) noexcept
        : m_library(std::move(library))
        , m_platformBackend(std::move(platformBackend))
        , m_rendererBackend(std::move(rendererBackend))
        , m_GPUName(std::move(GPUName))
    {}

    void DebugOverlay::Draw(const DebugOverlayContext& context)
    {
        // 0. The scale of the interface: the display scale of Windows (2.0 on a 4K monitor at 200%) times the UI scale chosen
        //    in the menu. The style is rebuilt only when the scale changes, and before the frame starts, because ImGui picks
        //    the font for the whole frame in NewFrame().
        const float scale = context.window.GetDisplayScale() * m_userUIScale;
        if (scale != m_appliedScale)
        {
            m_library.SetScale(scale);
            m_appliedScale = scale;
        }

        // 1. Start the frame: the backends pass ImGui the window size, the time and the input of this frame.
        m_rendererBackend.StartFrame();
        m_platformBackend.StartFrame();
        ImGui::NewFrame();

        // 2. Describe the windows. Nothing is drawn yet: ImGui only records what has to be drawn.
        //    While the overlay is hidden the ImGui frame still runs, just without windows: ImGui keeps receiving the
        //    input and the time, so it is in a consistent state when the overlay is shown again. An empty frame costs
        //    practically nothing.
        if (m_isVisible)
        {
            DrawMainMenuBar(context);

            // Every window is in a file of its own and gets only what it shows from the context.
            if (m_isPerformanceWindowOpen)
                DrawPerformanceWindow(&m_isPerformanceWindowOpen, context.frameStatistics, context.fixedTimestep,
                                      context.window, context.frameLimiter, m_GPUName);

            if (m_isAssetsWindowOpen)
                DrawAssetsWindow(&m_isAssetsWindowOpen, context.renderAssets);

            if (m_isAudioWindowOpen)
                m_audioWindow.Draw(&m_isAudioWindowOpen, context.audio);

            if (m_isEntitiesWindowOpen)
                m_entitiesWindow.Draw(&m_isEntitiesWindowOpen, context.registry, context.renderAssets);

            if (m_isRendererWindowOpen)
                DrawRendererWindow(&m_isRendererWindowOpen, context.renderSettings, context.renderStatistics,
                                   context.levelStatistics, context.isLevelReloadRequested);

            if (m_isCollisionWindowOpen)
                DrawCollisionWindow(&m_isCollisionWindowOpen, context.collisionSettings, context.cameraCast,
                                    context.collisionBrushCount);

            if (m_isMovementWindowOpen)
                DrawMovementWindow(&m_isMovementWindowOpen, context.physicsSettings, context.movementSettings,
                                   context.playerBody);

            if (m_isViewModelWindowOpen)
                DrawViewModelWindow(&m_isViewModelWindowOpen, context.viewModel, context.landingDip);
        }

        // The console is drawn even while the rest of the overlay is hidden: it has its own key.
        m_consoleWindow.Draw(context.logHistory);

        // 3. ImGui turns the recorded windows into lists of triangles, and the OpenGL backend draws them.
        ImGui::Render();
        m_rendererBackend.DrawFrame();

        // 4. Remember moved or resized windows for the next run.
        m_library.SaveSettingsIfChanged();
    }

    void DebugOverlay::ToggleVisibility() noexcept
    {
        m_isVisible = !m_isVisible;
    }

    bool DebugOverlay::IsVisible() const noexcept
    {
        return m_isVisible;
    }

    void DebugOverlay::ToggleConsole() noexcept
    {
        m_consoleWindow.Toggle();
    }

    bool DebugOverlay::IsConsoleOpen() const noexcept
    {
        return m_consoleWindow.IsOpen();
    }

    void DebugOverlay::DrawMainMenuBar(const DebugOverlayContext& context)
    {
        // BeginMainMenuBar() creates a bar along the top edge of the screen; BeginMenu() adds a menu to it that opens
        // on click. Both return true only while they are visible/open, and only then must their End...() be called.
        if (!ImGui::BeginMainMenuBar())
            return;

        if (ImGui::BeginMenu("View"))
        {
            // MenuItem(label, shortcut, bool*) shows a check mark and flips the bool when clicked. The windows used all the
            // time are at the top; the others are grouped by the part of the engine they show, so the menu grows in width
            // (new submenus) instead of in height as tools are added.
            ImGui::MenuItem("Performance", nullptr, &m_isPerformanceWindowOpen);
            ImGui::MenuItem("Console", "`", m_consoleWindow.GetOpenFlag());
            ImGui::Separator();

            if (ImGui::BeginMenu("Engine"))
            {
                ImGui::MenuItem("Entities", nullptr, &m_isEntitiesWindowOpen);
                ImGui::MenuItem("Assets", nullptr, &m_isAssetsWindowOpen);
                ImGui::MenuItem("Audio", nullptr, &m_isAudioWindowOpen);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Rendering"))
            {
                ImGui::MenuItem("Renderer", nullptr, &m_isRendererWindowOpen);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Physics"))
            {
                ImGui::MenuItem("Collisions", nullptr, &m_isCollisionWindowOpen);
                ImGui::MenuItem("Movement", nullptr, &m_isMovementWindowOpen);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Gameplay"))
            {
                ImGui::MenuItem("View Model", nullptr, &m_isViewModelWindowOpen);
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        // Settings are grouped into submenus the same way as the options menu of the game will be (Settings > Display, ...).
        // A BeginMenu() inside an open menu becomes a submenu.
        if (ImGui::BeginMenu("Settings"))
        {
            if (ImGui::BeginMenu("Display"))
            {
                // One item per mode, the current one checked, like the FPS limits below. The tooltip is set right after
                // BeginMenu(), so it belongs to the "Screen mode" item whether the submenu is open or not.
                const bool isScreenModeMenuOpen = ImGui::BeginMenu("Screen mode");
                ImGui::SetItemTooltip("Alt+Enter switches between Windowed and Borderless.");
                if (isScreenModeMenuOpen)
                {
                    for (std::size_t index = 0; index < Platform::ScreenModeNames.size(); ++index)
                    {
                        const auto mode = static_cast<Platform::ScreenMode>(index);
                        const bool isCurrentMode = context.window.GetScreenMode() == mode;
                        // data() is safe here: the names are string literals, which end with a zero like ImGui expects.
                        if (ImGui::MenuItem(Platform::ScreenModeNames[index].data(), nullptr, isCurrentMode) && !isCurrentMode)
                            context.window.SetScreenMode(mode);
                    }

                    ImGui::EndMenu();
                }

                // Here MenuItem(label, shortcut, bool) only shows the check mark and returns true when clicked,
                // because the state belongs to the window, not to the overlay.
                const bool isVSyncEnabled = context.window.IsVSyncEnabled();
                if (ImGui::MenuItem("V-Sync", nullptr, isVSyncEnabled))
                    context.window.SetVSyncEnabled(!isVSyncEnabled);
                ImGui::SetItemTooltip("Waits for the monitor refresh: no tearing, but FPS never exceeds the refresh rate.");

                // One item per limit, the current one checked (like radio buttons).
                if (ImGui::BeginMenu("FPS limit"))
                {
                    for (const int limit : FPSLimits)
                    {
                        const bool isCurrentLimit = context.frameLimiter.GetMaxFPS() == limit;
                        if (ImGui::MenuItem(FormatFPSLimit(limit).c_str(), nullptr, isCurrentLimit))
                            context.frameLimiter.SetMaxFPS(limit);
                    }

                    ImGui::EndMenu();
                }

                // The tooltip belongs to the "UI scale" item itself, so it is set right after BeginMenu(), whether the
                // submenu is open or not.
                const bool isUIScaleMenuOpen = ImGui::BeginMenu("UI scale");
                ImGui::SetItemTooltip("Multiplies the display scale of Windows (now %.0f%%).",
                                      context.window.GetDisplayScale() * 100.0f);
                if (isUIScaleMenuOpen)
                {
                    for (const float userScale : UserUIScales)
                    {
                        const std::string label = std::format("{:.0f}%", userScale * 100.0f);
                        if (ImGui::MenuItem(label.c_str(), nullptr, m_userUIScale == userScale))
                            m_userUIScale = userScale;
                    }

                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}
