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
            ImGui::Checkbox("Lights", &settings.areLightsVisible);
            ImGui::SetItemTooltip("Every light in its color: a star where it is (over everything) and where its light\n"
                                  "ends: three circles of its range, or the cone of a spot light.");

            // What the Lit shader shows and the made-up light it lights with until the level has lights (see
            // Renderer::ShadingView and Renderer::SceneLighting).
            ImGui::SeparatorText("Shading");
            constexpr const char* ShadingViewNames[] = {"Final", "Base color", "Normals", "Roughness", "Metalness", "Height"};
            int shadingView = static_cast<int>(settings.shadingView);
            if (ImGui::Combo("View", &shadingView, ShadingViewNames, IM_ARRAYSIZE(ShadingViewNames)))
                settings.shadingView = static_cast<Renderer::ShadingView>(shadingView);
            ImGui::SetItemTooltip("Final: the lit picture. The others show one property of the surfaces, without light:\n"
                                  "normals as colors (red right, green up, blue towards the camera), roughness and\n"
                                  "metalness as gray (black 0, white 1), the height of parallax as gray (white high).");
            DrawSlider("Sun", settings.sunIntensity, 0.0f, 10.0f, "%.2f",
                       "The made-up sun from above. About 3 (pi) makes a white surface facing it white.");
            DrawSlider("Ambient", settings.ambientIntensity, 0.0f, 2.0f, "%.2f",
                       "Light from everywhere, so the side away from the sun is not black.");
            ImGui::Checkbox("Parallax", &settings.isParallaxEnabled);
            ImGui::SetItemTooltip("Parallax occlusion mapping of the materials with a height map (masonry):\n"
                                  "stones hide the joints behind them when seen from the side.");
            DrawSlider("Parallax depth", settings.parallaxDepthScale, 0.0f, 3.0f, "x %.2f",
                       "Times the depth of every material with a height map.");
            DrawIntSlider("Parallax steps", settings.parallaxStepCount, 4, 32,
                          "Steps into the relief: more is smoother at a grazing angle and costs more.");
            ImGui::Checkbox("Specular anti-aliasing", &settings.isSpecularAntiAliasingEnabled);
            ImGui::SetItemTooltip("Widens highlights where the surface turns fast from pixel to pixel,\n"
                                  "so they do not flicker when the camera moves.");

            // How the HDR scene is fitted into what the screen shows (see Renderer::ToneMapping and Present.frag).
            ImGui::SeparatorText("Tone mapping");
            if (ImGui::RadioButton("None", settings.toneMapping == Renderer::ToneMapping::None))
                settings.toneMapping = Renderer::ToneMapping::None;
            ImGui::SetItemTooltip("Everything brighter than 1 is cut off: a flame looks like a white wall.");
            ImGui::SameLine();
            if (ImGui::RadioButton("ACES", settings.toneMapping == Renderer::ToneMapping::ACES))
                settings.toneMapping = Renderer::ToneMapping::ACES;
            ImGui::SetItemTooltip("The filmic S-curve (Narkowicz): more contrast; bright values are pressed together,\n"
                                  "never cut off; colors more saturated.");
            ImGui::SameLine();
            if (ImGui::RadioButton("ACES (Hill)", settings.toneMapping == Renderer::ToneMapping::HillACES))
                settings.toneMapping = Renderer::ToneMapping::HillACES;
            ImGui::SetItemTooltip("ACES fitted closer to the real one: bright colors keep their hue, a little darker.");
            ImGui::SameLine();
            if (ImGui::RadioButton("AgX", settings.toneMapping == Renderer::ToneMapping::AgX))
                settings.toneMapping = Renderer::ToneMapping::AgX;
            ImGui::SetItemTooltip("Blender 4: very bright colors go to white, like a photograph of a flame;\n"
                                  "softer contrast.");
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
