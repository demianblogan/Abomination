#include "UI/MovementWindow.h"

#include "Core/Units.h"
#include "Physics/CharacterBody.h"
#include "Physics/CharacterMovement.h"
#include "UI/UIScale.h"

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <imgui.h>

namespace Abomination::UI
{
    namespace
    {
        // The window opens for the first time at the left edge, below the Performance window.
        constexpr ImVec2 InitialPosition(10.0f, 260.0f);

        // The width of the sliders in pixels at 100% scale.
        constexpr float SliderWidth = 180.0f;

        // A slider for a setting; the tooltip explains it. Returns nothing: the value is changed in place.
        void DrawSlider(const char* label, float& value, float minimum, float maximum, const char* format,
                        const char* tooltip)
        {
            ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
            ImGui::SliderFloat(label, &value, minimum, maximum, format);
            ImGui::SetItemTooltip("%s", tooltip);
        }
    }

    void DrawMovementWindow(bool* isOpen, Physics::PhysicsSettings& physicsSettings,
                            Physics::MovementSettings& movementSettings, const Physics::CharacterBody& playerBody)
    {
        ImGui::SetNextWindowPos(ScaleToUI(InitialPosition), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Movement", isOpen, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::End();
            return;
        }

        // The speedometer: the horizontal speed is what running feels like; falling and jumping are shown apart. It is
        // shown in the units of Quake too, to compare with it (320 units/s is running in Quake).
        const float horizontalSpeed = glm::length(glm::vec2(playerBody.velocity.x, playerBody.velocity.z));
        ImGui::SeparatorText("Player");
        ImGui::Text("Speed:      %5.2f m/s (%3.0f units/s)", horizontalSpeed, Core::MetersToMapUnits(horizontalSpeed));
        ImGui::Text("Vertical:   %5.2f m/s", playerBody.velocity.y);
        ImGui::Text("On ground:  %s", playerBody.isOnGround ? "yes" : "no");
        ImGui::Text("In solid:   %s", playerBody.isInSolid ? "YES (see the log)" : "no");
        ImGui::SetItemTooltip("The box started the last tick inside a brush. It is pushed out, and a warning with the\n"
                              "details is written to the log (the console, ~).");

        ImGui::SeparatorText("World");
        DrawSlider("Gravity", physicsSettings.gravity, 0.0f, 50.0f, "%.1f m/s²",
                   "Pulls everything down. Quake: 25 m/s² (800 units/s²); the real world: 9.81.");

        ImGui::SeparatorText("Walking");
        DrawSlider("Max speed", movementSettings.maxSpeed, 1.0f, 20.0f, "%.2f m/s",
                   "The fastest the player runs on their own. Default: 7 m/s, like modern shooters.\n"
                   "Quake: 10 m/s (320 units/s).");
        DrawSlider("Acceleration", movementSettings.groundAcceleration, 1.0f, 30.0f, "%.1f",
                   "How fast full speed is reached on the ground: max speed x acceleration per second. Quake: 10.");
        DrawSlider("Friction", movementSettings.friction, 0.0f, 15.0f, "%.1f",
                   "How fast the player stops without input: friction x speed per second. Quake: 4.");
        DrawSlider("Stop speed", movementSettings.stopSpeed, 0.0f, 10.0f, "%.2f m/s",
                   "Below this speed friction acts as if the player moved at it, so they stop completely.\n"
                   "Quake: 3.1 m/s (100 units/s).");
        DrawSlider("Step height", movementSettings.stepHeight, 0.0f, 1.5f, "%.2f m",
                   "The highest step walked up without jumping. Quake: 0.56 m (18 units).");

        ImGui::SeparatorText("Jumping");
        DrawSlider("Jump speed", movementSettings.jumpSpeed, 0.0f, 20.0f, "%.2f m/s",
                   "The upward speed a jump starts with. Quake: 8.4 m/s (270 units/s).");

        // The height follows from the jump speed and the gravity: v² / (2g), the height where the upward speed runs out.
        if (physicsSettings.gravity > 0.0f)
            ImGui::Text("Jump height: %.2f m",
                        movementSettings.jumpSpeed * movementSettings.jumpSpeed / (2.0f * physicsSettings.gravity));

        DrawSlider("Air acceleration", movementSettings.airAcceleration, 0.0f, 30.0f, "%.1f",
                   "How fast the player steers in the air (up to the air speed limit). Quake: 10.");
        DrawSlider("Air speed limit", movementSettings.maxAirWishSpeed, 0.0f, 10.0f, "%.2f m/s",
                   "The most speed steering adds in any one direction in the air. Quake: 0.94 m/s (30 units/s).\n"
                   "Small values keep jumps committed; turning with a strafe key still bends them (air strafing).");

        ImGui::Separator();
        if (ImGui::Button("Reset"))
        {
            physicsSettings = Physics::PhysicsSettings{};
            movementSettings = Physics::MovementSettings{};
        }
        ImGui::SetItemTooltip("Brings back the default values. The values are not saved between runs yet.");

        ImGui::End();
    }
}
