#include "UI/CollisionWindow.h"

#include "UI/UIScale.h"
#include "World/CollisionDebug.h"

#include <imgui.h>

namespace Abomination::UI
{
    namespace
    {
        // The window opens for the first time below the Renderer window.
        constexpr ImVec2 InitialPosition(900.0f, 360.0f);
    }

    void DrawCollisionWindow(bool* isOpen, World::CollisionDebugSettings& settings, const World::CameraTrace& cameraTrace,
                             std::size_t collisionBrushCount)
    {
        ImGui::SetNextWindowPos(ScaleToUI(InitialPosition), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Collision", isOpen, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Collision brushes: %zu", collisionBrushCount);
            ImGui::Checkbox("Brush bounds", &settings.areBrushBoundsVisible);
            ImGui::SetItemTooltip("The bounding box of every brush (orange): the quick test before its planes.");

            ImGui::SeparatorText("Camera");
            ImGui::Checkbox("Camera collides", &settings.doesCameraCollide);
            ImGui::SetItemTooltip("The free-fly camera slides along walls instead of flying through them.");

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

            if (cameraTrace.isValid)
            {
                const World::TraceResult& result = cameraTrace.result;
                ImGui::Text("Fraction:  %.3f", result.fraction);
                ImGui::Text("Distance:  %.2f m", cameraTrace.distance);
                ImGui::Text("Normal:    (%.2f, %.2f, %.2f)", result.hitNormal.x, result.hitNormal.y, result.hitNormal.z);
                ImGui::Text("Starts in solid: %s, stuck: %s", result.startsInSolid ? "yes" : "no",
                            result.isStuck ? "yes" : "no");
            }
        }
        ImGui::End();
    }
}
