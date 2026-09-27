#include "UI/DebugOverlay.h"

#include "Core/AssetLifetime.h"
#include "Core/BuildConfiguration.h"
#include "Core/FixedTimestep.h"
#include "Core/FrameLimiter.h"
#include "Core/FrameStatistics.h"
#include "Core/Version.h"
#include "Platform/Window.h"
#include "Renderer/OpenGLLoader.h"
#include "Renderer/RenderAssets.h"
#include "Renderer/RenderSettings.h"
#include "Renderer/RenderSystem.h"
#include "UI/MovementWindow.h"
#include "UI/UIScale.h"
#include "World/CollisionDebug.h"
#include "World/LevelMesh.h"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <span>
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

        // Distance from the edges of the game window (below the menu bar) to the performance window, in pixels.
        constexpr float PerformanceWindowMargin = 10.0f;

        // Opacity of the performance window background: 0 is fully transparent, 1 is opaque.
        constexpr float PerformanceWindowBackgroundAlpha = 0.6f;

        // Size of the frame time graph in pixels.
        constexpr float GraphWidth = 400.0f;
        constexpr float GraphHeight = 60.0f;

        // The top of the graph is at least 1/30 s (33.3 ms): the budget of a frame at 30 FPS. A fixed minimum keeps
        // the scale stable, so the same frame time always has the same height; only longer frames stretch it.
        constexpr float MinimumGraphTopFrameTime = 1.0f / 30.0f;

        constexpr float MillisecondsPerSecond = 1000.0f;

        // The limits offered in Settings > Display > FPS limit; 0 means no limit. They are chosen for testing, not for
        // players: with the simulation running at 60 ticks per second,
        //   15, 30 - a slow computer: 4 and 2 ticks in every frame;
        //   60     - exactly 1 tick in every frame;
        //   120    - an even pattern: 0, 1, 0, 1 ticks per frame;
        //   144    - an uneven pattern (0, 0, 1, 0, 1, ...), where movement stutters without interpolation;
        //   240    - common fast monitors, many frames without a tick.
        constexpr std::array FramesPerSecondLimits{0, 15, 30, 60, 120, 144, 240};

        // The UI scales offered in Settings > Display > UI scale. They multiply the display scale of Windows: for a screen
        // whose Windows setting does not match how far away it is (a 4K TV at 100%, seen from the sofa).
        constexpr std::array UserUIScales{0.75f, 1.0f, 1.25f, 1.5f, 2.0f};

        // The window has a title bar with a close button and can be collapsed by the arrow in it, but it cannot be moved:
        //   AlwaysAutoResize   - the size always fits the contents (so it cannot be resized by hand either);
        //   NoMove             - stays pinned to the corner;
        //   NoSavedSettings    - its position and state are not written anywhere;
        //   NoFocusOnAppearing - does not take the keyboard focus from the game when it appears;
        //   NoNav              - is skipped by keyboard and gamepad navigation between ImGui windows.
        constexpr ImGuiWindowFlags PerformanceWindowFlags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                                                            ImGuiWindowFlags_NoSavedSettings |
                                                            ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

        // The Assets window opens for the first time to the right of the performance window; later ImGui restores the
        // position and size it was left at.
        constexpr ImVec2 AssetsWindowInitialPosition(450.0f, 40.0f);
        constexpr ImVec2 AssetsWindowInitialSize(620.0f, 360.0f);

        // The Renderer window opens for the first time at the right edge of the default window.
        constexpr ImVec2 RendererWindowInitialPosition(900.0f, 40.0f);

        // The Collision window opens for the first time below the Renderer window.
        constexpr ImVec2 CollisionWindowInitialPosition(900.0f, 360.0f);

        // The color of assets replaced by a fallback: the same magenta as the fallbacks themselves.
        constexpr ImVec4 FallbackTextColor(1.0f, 0.0f, 1.0f, 1.0f);

        // The Status cell of an asset: "Loaded", or a magenta "Fallback" with a tooltip that explains it.
        void DrawAssetStatus(bool isFallback)
        {
            if (!isFallback)
            {
                ImGui::TextUnformatted("Loaded");

                return;
            }

            ImGui::TextColored(FallbackTextColor, "Fallback");
            ImGui::SetItemTooltip("The file is missing or broken; the log says why.");
        }

        // "512 B", "21.3 KB" or "4.0 MB": a size in bytes for people to read.
        std::string FormatByteSize(std::size_t byteCount)
        {
            constexpr double BytesPerKilobyte = 1024.0;
            constexpr double BytesPerMegabyte = BytesPerKilobyte * 1024.0;

            const auto bytes = static_cast<double>(byteCount);
            if (bytes >= BytesPerMegabyte)
                return std::format("{:.1f} MB", bytes / BytesPerMegabyte);
            if (bytes >= BytesPerKilobyte)
                return std::format("{:.1f} KB", bytes / BytesPerKilobyte);

            return std::format("{} B", byteCount);
        }

        // "Unlimited" or "144 FPS": the text of a frame rate limit in the menu and in the performance window.
        std::string FormatFramesPerSecondLimit(int maxFramesPerSecond)
        {
            if (maxFramesPerSecond == 0)
                return "Unlimited";

            return std::format("{} FPS", maxFramesPerSecond);
        }
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

            if (m_isPerformanceWindowOpen)
                DrawPerformanceWindow(context);

            if (m_isAssetsWindowOpen)
                DrawAssetsWindow(context);

            if (m_isEntitiesWindowOpen)
                m_entitiesWindow.Draw(&m_isEntitiesWindowOpen, context.registry, context.renderAssets);

            if (m_isRendererWindowOpen)
                DrawRendererWindow(context);

            if (m_isCollisionWindowOpen)
                DrawCollisionWindow(context);

            if (m_isMovementWindowOpen)
                DrawMovementWindow(&m_isMovementWindowOpen, context.physicsSettings, context.movementSettings,
                                   context.playerBody);
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
            // MenuItem(label, shortcut, bool*) shows a check mark and flips the bool when clicked.
            ImGui::MenuItem("Performance", nullptr, &m_isPerformanceWindowOpen);
            ImGui::MenuItem("Assets", nullptr, &m_isAssetsWindowOpen);
            ImGui::MenuItem("Entities", nullptr, &m_isEntitiesWindowOpen);
            ImGui::MenuItem("Renderer", nullptr, &m_isRendererWindowOpen);
            ImGui::MenuItem("Collision", nullptr, &m_isCollisionWindowOpen);
            ImGui::MenuItem("Movement", nullptr, &m_isMovementWindowOpen);
            ImGui::MenuItem("Console", "`", m_consoleWindow.GetOpenFlag());
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
                    for (const int limit : FramesPerSecondLimits)
                    {
                        const bool isCurrentLimit = context.frameLimiter.GetMaxFramesPerSecond() == limit;
                        if (ImGui::MenuItem(FormatFramesPerSecondLimit(limit).c_str(), nullptr, isCurrentLimit))
                            context.frameLimiter.SetMaxFramesPerSecond(limit);
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

    void DebugOverlay::DrawPerformanceWindow(const DebugOverlayContext& context)
    {
        // The window is placed below the menu bar, whose height is the height of one line of ImGui widgets.
        // Both calls only affect the next Begin(). ImGuiCond_Always applies the position every frame,
        // so the window stays pinned to the corner.
        const float margin = ScaleToUI(PerformanceWindowMargin);
        const ImVec2 position(margin, ImGui::GetFrameHeight() + margin);
        ImGui::SetNextWindowPos(position, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(PerformanceWindowBackgroundAlpha);

        // The title is shown in the title bar; ImGui also identifies windows by it, so it must be unique.
        // Passing the bool adds a close button to the title bar, which sets it to false.
        // Begin() returns false when the window is collapsed or fully clipped: then its contents are skipped,
        // but End() must still be called.
        if (ImGui::Begin("Performance", &m_isPerformanceWindowOpen, PerformanceWindowFlags))
        {
            const Core::FrameStatistics& frameStatistics = context.frameStatistics;

            const std::string_view buildConfiguration = Core::IsDebugBuild ? "Debug" : "Release";
            const std::string versionText =
                std::format("Abomination {} ({})", Core::GetGameVersionString(), buildConfiguration);
            ImGui::TextUnformatted(versionText.c_str());
            ImGui::TextUnformatted(m_GPUName.c_str());

            ImGui::Separator();

            const float averageFrameTime = frameStatistics.GetAverageFrameTime();
            const float longestFrameTime = frameStatistics.GetLongestFrameTime();

            // "{:.0f}" - no digits after the point, "{:.2f}" - two digits.
            const std::string framesPerSecondText = std::format("FPS: {:.0f}", frameStatistics.GetAverageFramesPerSecond());
            const std::string frameTimeText =
                std::format("Frame time: {:.2f} ms (longest {:.2f} ms)", averageFrameTime * MillisecondsPerSecond,
                            longestFrameTime * MillisecondsPerSecond);
            const std::string frameRateSettingsText =
                std::format("V-Sync: {}, FPS limit: {}", context.window.IsVSyncEnabled() ? "on" : "off",
                            FormatFramesPerSecondLimit(context.frameLimiter.GetMaxFramesPerSecond()));
            ImGui::TextUnformatted(framesPerSecondText.c_str());
            ImGui::TextUnformatted(frameTimeText.c_str());
            ImGui::TextUnformatted(frameRateSettingsText.c_str());

            // Simulation ticks per second: actually run / target. Both are equal at any FPS; fewer actual ticks mean the
            // computer cannot simulate in real time and Core::FixedTimestep drops ticks, so the game runs slower.
            const std::string simulationText =
                std::format("Simulation: {:.0f} / {} ticks per second", frameStatistics.GetTicksPerSecond(),
                            context.fixedTimestep.GetTicksPerSecond());
            ImGui::TextUnformatted(simulationText.c_str());
            ImGui::SetItemTooltip("Actual / target. Fewer actual ticks mean the simulation cannot keep up and the game "
                                  "runs slower than real time.");

            // The graph: one point per frame, the height is the frame time. The samples are a ring buffer, so the
            // index of the oldest sample is passed as the offset: ImGui starts drawing from it and wraps around.
            // The "##" prefix hides the label: the text after it is used only as an ID.
            // The top of the graph fits the longest frame on the graph itself, which can be older than the interval
            // of the numbers above (the graph covers MaxSampleCount frames, the numbers only the last half second).
            const int oldestSampleIndex = static_cast<int>(frameStatistics.GetOldestSampleIndex());
            const std::span<const float> frameTimeSamples = frameStatistics.GetFrameTimeSamples();
            const float longestSample = frameTimeSamples.empty() ? 0.0f : std::ranges::max(frameTimeSamples);
            const float graphTop = std::max(MinimumGraphTopFrameTime, longestSample);
            const std::string graphCaption = std::format("0 - {:.0f} ms", graphTop * MillisecondsPerSecond);

            ImGui::PlotLines("##FrameTimes", frameTimeSamples.data(), static_cast<int>(frameTimeSamples.size()),
                             oldestSampleIndex, graphCaption.c_str(), 0.0f, graphTop,
                             ScaleToUI(ImVec2(GraphWidth, GraphHeight)));
        }
        ImGui::End();
    }

    void DebugOverlay::DrawAssetsWindow(const DebugOverlayContext& context)
    {
        // ImGuiCond_FirstUseEver applies the position and size only when the settings file does not know the window yet:
        // afterwards the window opens where it was left.
        ImGui::SetNextWindowPos(ScaleToUI(AssetsWindowInitialPosition), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ScaleToUI(AssetsWindowInitialSize), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Assets", &m_isAssetsWindowOpen))
        {
            const Renderer::RenderAssets& assets = context.renderAssets;

            // The header shows the totals, so they are summed before the list is drawn.
            std::size_t totalTextureMemory = 0;
            assets.textures.VisitTextures(
                [&](const std::string&, const Renderer::GLTexture& texture, bool, Core::AssetLifetime)
            {
                totalTextureMemory += texture.GetVideoMemorySize();
            });

            // A collapsing header is a clickable bar that shows or hides what follows it. The text after "###" is the ID
            // ImGui remembers the header by: the visible label changes with the numbers, the ID must stay the same.
            const std::string texturesHeader = std::format("Textures: {}, {} of video memory###Textures",
                                                           assets.textures.GetCount(), FormatByteSize(totalTextureMemory));
            if (ImGui::CollapsingHeader(texturesHeader.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                // A table: columns are set up once, then every row is filled cell by cell with TableNextColumn().
                // RowBg alternates the row background, Borders draws the lines between cells.
                if (ImGui::BeginTable("TextureTable", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
                {
                    // The path takes all the width the other columns leave; the others are as wide as their contents.
                    ImGui::TableSetupColumn("Path", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("Video memory", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("Lifetime", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableHeadersRow();

                    assets.textures.VisitTextures([](const std::string& path, const Renderer::GLTexture& texture,
                                                     bool isFallback, Core::AssetLifetime lifetime)
                    {
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(path.c_str());

                        ImGui::TableNextColumn();
                        const std::string sizeText = std::format("{}x{}", texture.GetWidth(), texture.GetHeight());
                        ImGui::TextUnformatted(sizeText.c_str());

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(FormatByteSize(texture.GetVideoMemorySize()).c_str());

                        ImGui::TableNextColumn();
                        DrawAssetStatus(isFallback);

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(Core::GetAssetLifetimeName(lifetime).data());
                    });

                    ImGui::EndTable();
                }
            }

            std::size_t totalMeshMemory = 0;
            assets.meshes.VisitMeshes([&](const std::string&, const Renderer::Mesh& mesh, Core::AssetLifetime)
            {
                totalMeshMemory += mesh.GetVideoMemorySize();
            });

            const std::string meshesHeader = std::format("Meshes: {}, {} of video memory###Meshes", assets.meshes.GetCount(),
                                                         FormatByteSize(totalMeshMemory));
            if (ImGui::CollapsingHeader(meshesHeader.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::BeginTable("MeshTable", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
                {
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Vertices", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("Triangles", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("Video memory", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableSetupColumn("Lifetime", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableHeadersRow();

                    assets.meshes.VisitMeshes([](const std::string& name, const Renderer::Mesh& mesh,
                                                 Core::AssetLifetime lifetime)
                    {
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(name.c_str());

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(std::format("{}", mesh.GetVertexCount()).c_str());

                        // Every 3 indices are one triangle.
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(std::format("{}", mesh.GetIndexCount() / 3).c_str());

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(FormatByteSize(mesh.GetVideoMemorySize()).c_str());

                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(Core::GetAssetLifetimeName(lifetime).data());
                    });

                    ImGui::EndTable();
                }
            }

            const std::string programsHeader = std::format("Shader programs: {}###ShaderPrograms", assets.shaders.GetCount());
            if (ImGui::CollapsingHeader(programsHeader.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::BeginTable("ShaderProgramTable", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
                {
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableHeadersRow();

                    assets.shaders.VisitPrograms([](const std::string& name, bool isFallback)
                    {
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(name.c_str());

                        ImGui::TableNextColumn();
                        DrawAssetStatus(isFallback);
                    });

                    ImGui::EndTable();
                }
            }
        }
        ImGui::End();
    }

    void DebugOverlay::DrawRendererWindow(const DebugOverlayContext& context)
    {
        // AlwaysAutoResize: the window is exactly as big as its contents, which do not change much.
        ImGui::SetNextWindowPos(ScaleToUI(RendererWindowInitialPosition), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Renderer", &m_isRendererWindowOpen, ImGuiWindowFlags_AlwaysAutoResize))
        {
            // Two radio buttons for one setting: RadioButton(label, isActive) shows which one is chosen and returns true
            // when it is clicked.
            ImGui::TextUnformatted("Mode");
            Renderer::RenderSettings& settings = context.renderSettings;
            if (ImGui::RadioButton("Solid", !settings.isWireframeEnabled))
                settings.isWireframeEnabled = false;
            ImGui::SameLine();
            if (ImGui::RadioButton("Wireframe", settings.isWireframeEnabled))
                settings.isWireframeEnabled = true;
            ImGui::Checkbox("World axes", &settings.areWorldAxesVisible);
            ImGui::SetItemTooltip("Arrows along X (red), Y (green, up) and Z (blue) from the origin of the world,\n"
                                  "1 m long, drawn over everything.");

            ImGui::SeparatorText("Last frame");
            ImGui::Text("Draw calls: %d", context.renderStatistics.drawCallCount);
            ImGui::SetItemTooltip("One per drawn mesh. Many small draws cost more than a few large ones.");
            ImGui::Text("Triangles:  %d", context.renderStatistics.triangleCount);

            ImGui::SeparatorText("Level");
            ImGui::Text("Brushes:   %d", context.levelStatistics.brushCount);
            ImGui::Text("Faces:     %d", context.levelStatistics.faceCount);
            ImGui::Text("Triangles: %d", context.levelStatistics.triangleCount);

            // Loads the map file next to the executable again. After saving the map in TrenchBroom, building the CopyAssets
            // target copies it there without closing the game (the executable itself cannot be rebuilt while it runs).
            if (ImGui::Button("Reload"))
                context.isLevelReloadRequested = true;
            ImGui::SetItemTooltip("Loads the map again.\n"
                                  "Build the CopyAssets target first to copy a map saved in TrenchBroom.");
        }
        ImGui::End();
    }

    void DebugOverlay::DrawCollisionWindow(const DebugOverlayContext& context)
    {
        ImGui::SetNextWindowPos(ScaleToUI(CollisionWindowInitialPosition), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Collision", &m_isCollisionWindowOpen, ImGuiWindowFlags_AlwaysAutoResize))
        {
            World::CollisionDebugSettings& settings = context.collisionSettings;

            ImGui::Text("Collision brushes: %zu", context.collisionBrushCount);
            ImGui::Checkbox("Brush bounds", &settings.areBrushBoundsVisible);
            ImGui::SetItemTooltip("The bounding box of every brush (orange): the quick test before its planes.");

            ImGui::SeparatorText("Camera");
            ImGui::Checkbox("Camera collides", &settings.doesCameraCollide);
            ImGui::SetItemTooltip("The free-fly camera stops at walls. It does not slide along them yet.");

            ImGui::SeparatorText("Trace from the camera");
            ImGui::Checkbox("Enabled", &settings.isCameraTraceEnabled);
            ImGui::SetItemTooltip("A box flies from the camera straight ahead (up to %.0f m). Yellow: where it stops;\n"
                                  "green arrow: the normal of the surface it hits.",
                                  World::CameraTraceLength);

            // One radio button per shape, on one line.
            for (std::size_t index = 0; index < World::TraceShapeNames.size(); ++index)
            {
                if (index > 0)
                    ImGui::SameLine();

                const auto shape = static_cast<World::TraceShape>(index);
                // data() is safe here: the names are string literals, which end with a zero like ImGui expects.
                if (ImGui::RadioButton(World::TraceShapeNames[index].data(), settings.cameraTraceShape == shape))
                    settings.cameraTraceShape = shape;
            }

            const World::CameraTrace& trace = context.cameraTrace;
            if (trace.isValid)
            {
                const World::TraceResult& result = trace.result;
                ImGui::Text("Fraction:  %.3f", result.fraction);
                ImGui::Text("Distance:  %.2f m", trace.distance);
                ImGui::Text("Normal:    (%.2f, %.2f, %.2f)", result.hitNormal.x, result.hitNormal.y, result.hitNormal.z);
                ImGui::Text("Starts in solid: %s, stuck: %s", result.startsInSolid ? "yes" : "no",
                            result.isStuck ? "yes" : "no");
            }
        }
        ImGui::End();
    }
}
