#include "UI/Windows/RendererWindow.h"

#include "Renderer/RenderSettings.h"
#include "Renderer/RenderSystem.h"
#include "UI/UIScale.h"
#include "UI/Widgets.h"
#include "World/LevelMesh.h"

#include <imgui.h>

namespace Abomination::UI
{
    namespace
    {
        // The window opens for the first time at the right edge of the default window.
        constexpr ImVec2 InitialPosition(900.0f, 40.0f);
    }

    void DrawRendererWindow(bool* isOpen, Renderer::RenderSettings& settings, const Renderer::RenderStatistics& statistics,
                            const World::LevelMeshStatistics& levelStatistics, bool& isLevelReloadRequested)
    {
        ImGui::SetNextWindowPos(ScaleToUI(InitialPosition), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Renderer", isOpen))
        {
            // Two radio buttons for one setting: RadioButton(label, isActive) shows which one is chosen and returns true
            // when it is clicked.
            ImGui::TextUnformatted("Mode");
            if (ImGui::RadioButton("Solid", !settings.isWireframeEnabled))
                settings.isWireframeEnabled = false;
            ImGui::SameLine();
            if (ImGui::RadioButton("Wireframe", settings.isWireframeEnabled))
                settings.isWireframeEnabled = true;
            ImGui::Checkbox("World axes", &settings.areWorldAxesVisible);
            ImGui::SetItemTooltip("Arrows along X (red), Y (green, up) and Z (blue) from the origin of the world,\n"
                                  "1 m long, drawn over everything.");

            // How the HDR scene is fitted into what the screen shows (see Renderer::ToneMapping and Present.frag).
            ImGui::SeparatorText("Tone mapping");
            if (ImGui::RadioButton("None", settings.toneMapping == Renderer::ToneMapping::None))
                settings.toneMapping = Renderer::ToneMapping::None;
            ImGui::SetItemTooltip("Everything brighter than 1 is cut off: a flame looks like a white wall.");
            ImGui::SameLine();
            if (ImGui::RadioButton("ACES", settings.toneMapping == Renderer::ToneMapping::ACES))
                settings.toneMapping = Renderer::ToneMapping::ACES;
            ImGui::SetItemTooltip("The filmic curve: dark stays dark, bright is pressed together and never cut off.");
            DrawSlider("Exposure", settings.exposureStops, -4.0f, 4.0f, "%.1f stops",
                       "Brightness before tone mapping, like a camera: +1 doubles it, -1 halves it.");

            ImGui::SeparatorText("Last frame");
            ImGui::Text("Draw calls: %d", statistics.drawCallCount);
            ImGui::SetItemTooltip("One per drawn mesh. Many small draws cost more than a few large ones.");
            ImGui::Text("Triangles:  %d", statistics.triangleCount);

            ImGui::SeparatorText("Level");
            ImGui::Text("Brushes:   %d", levelStatistics.brushCount);
            ImGui::Text("Faces:     %d", levelStatistics.faceCount);
            ImGui::Text("Triangles: %d", levelStatistics.triangleCount);

            // Loads the map file next to the executable again. After saving the map in TrenchBroom, building the CopyAssets
            // target copies it there without closing the game (the executable itself cannot be rebuilt while it runs).
            if (ImGui::Button("Reload"))
                isLevelReloadRequested = true;
            ImGui::SetItemTooltip("Loads the map again.\n"
                                  "Build the CopyAssets target first to copy a map saved in TrenchBroom.");
        }
        ImGui::End();
    }
}
