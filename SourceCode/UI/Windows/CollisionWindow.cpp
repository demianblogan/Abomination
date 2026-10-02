#include "UI/Windows/CollisionWindow.h"

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

    void DrawCollisionWindow(bool* isOpen, World::CollisionDebugSettings& settings, const World::CameraCast& cameraCast,
                             std::size_t collisionBrushCount)
    {
        ImGui::SetNextWindowPos(ScaleToUI(InitialPosition), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Collisions", isOpen, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Colliders: %zu", collisionBrushCount);
            ImGui::Checkbox("Draw collider bounds", &settings.areColliderBoundsVisible);
            ImGui::SetItemTooltip("The bounding box of every collider (orange): the quick test before its planes.");

            ImGui::SeparatorText("Camera");
            ImGui::Checkbox("Enable collisions", &settings.doesCameraCollide);
            ImGui::SetItemTooltip("The free-fly camera slides along walls instead of flying through them.");

            ImGui::SeparatorText("Cast from the camera");
            ImGui::Checkbox("Enabled", &settings.isCameraCastEnabled);
            ImGui::SetItemTooltip("A ray or a box flies from the camera straight ahead (up to %.0f m).\n"
                                  "Yellow: where it stops; green arrow: the normal of the surface it hits.",
                                  World::CameraCastLength);

            // One radio button per shape, on one line.
            for (std::size_t index = 0; index < World::CastShapeNames.size(); ++index)
            {
                if (index > 0)
                    ImGui::SameLine();

                const auto shape = static_cast<World::CastShape>(index);
                // data() is safe here: the names are string literals, which end with a zero like ImGui expects.
                if (ImGui::RadioButton(World::CastShapeNames[index].data(), settings.cameraCastShape == shape))
                    settings.cameraCastShape = shape;
            }

            if (cameraCast.isValid)
            {
                const World::TraceResult& result = cameraCast.result;
                ImGui::Text("Fraction:  %.3f", result.fraction);
                ImGui::Text("Distance:  %.2f m", cameraCast.distance);
                ImGui::Text("Normal:    (%.2f, %.2f, %.2f)", result.hitNormal.x, result.hitNormal.y, result.hitNormal.z);
                ImGui::Text("Starts in solid: %s, stuck: %s", result.startsInSolid ? "yes" : "no",
                            result.isStuck ? "yes" : "no");
            }
        }
        ImGui::End();
    }
}
