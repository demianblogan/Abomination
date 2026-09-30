#include "UI/CrosshairOverlay.h"

#include "Gameplay/Crosshair.h"
#include "Gameplay/Weapon.h"
#include "UI/UIScale.h"

#include <glm/common.hpp>
#include <imgui.h>

#include <array>
#include <numbers>

namespace Abomination::UI
{
    namespace
    {
        // The circle is drawn with this many straight segments: enough to look round at the sizes of a crosshair.
        constexpr int CircleSegmentCount = 48;

        // ImGui packs a color into 32 bits.
        ImU32 ToImGuiColor(const glm::vec4& color)
        {
            return ImGui::ColorConvertFloat4ToU32(ImVec4(color.r, color.g, color.b, color.a));
        }

        ImVec2 GetScreenCenter()
        {
            const ImVec2 size = ImGui::GetIO().DisplaySize;
            return ImVec2(size.x * 0.5f, size.y * 0.5f);
        }
    }

    void CrosshairOverlay::Draw(const Gameplay::Weapon& weapon, float verticalFOV)
    {
        UpdateMarkers(weapon, ImGui::GetIO().DeltaTime);

        // The foreground draw list of ImGui is drawn over every window: the crosshair is never hidden behind one.
        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        const Gameplay::CrosshairSettings& settings = weapon.crosshair;
        const ImVec2 center = GetScreenCenter();

        // The circle is as wide as the spread; its size does not follow the UI scale (it shows where pellets land), but
        // its line thickness does.
        const float circleRadius = Gameplay::CalculateSpreadRadiusOnScreen(weapon.settings.spreadAngle, verticalFOV,
                                                                            ImGui::GetIO().DisplaySize.y);
        drawList->AddCircle(center, circleRadius, ToImGuiColor(settings.color), CircleSegmentCount,
                            ScaleToUI(settings.circleThickness));
        if (settings.dotRadius > 0.0f)
            drawList->AddCircleFilled(center, ScaleToUI(settings.dotRadius), ToImGuiColor(settings.color));

        // A kill marker covers a hit marker of the same shot.
        if (m_secondsSinceKill < settings.killMarker.duration)
            DrawMarker(settings.killMarker, m_secondsSinceKill, circleRadius);
        else if (m_secondsSinceHit < settings.hitMarker.duration)
            DrawMarker(settings.hitMarker, m_secondsSinceHit, circleRadius);
    }

    void CrosshairOverlay::UpdateMarkers(const Gameplay::Weapon& weapon, float deltaTime)
    {
        if (!m_hasSeenWeapon)
        {
            m_seenHitCount = weapon.hitCount;
            m_seenKillCount = weapon.killCount;
            m_hasSeenWeapon = true;
        }

        m_secondsSinceHit += deltaTime;
        m_secondsSinceKill += deltaTime;

        if (weapon.hitCount != m_seenHitCount)
        {
            m_seenHitCount = weapon.hitCount;
            m_secondsSinceHit = 0.0f;
        }
        if (weapon.killCount != m_seenKillCount)
        {
            m_seenKillCount = weapon.killCount;
            m_secondsSinceKill = 0.0f;
        }
    }

    void CrosshairOverlay::DrawMarker(const Gameplay::HitMarkerSettings& settings, float secondsSinceStart,
                                      float circleRadius)
    {
        // How far through the marker it is, 0 to 1. It moves fast at first and slows down at the end (ease-out: 1 - (1 -
        // t)^2), which reads as a quick flick rather than a slide, and fades out evenly.
        const float progress = glm::clamp(secondsSinceStart / settings.duration, 0.0f, 1.0f);
        const float eased = 1.0f - (1.0f - progress) * (1.0f - progress);
        glm::vec4 color = settings.color;
        color.a *= 1.0f - progress;

        // Where the inner end of every line is, from the middle of the screen.
        const float innerDistance = circleRadius + ScaleToUI(settings.startGap + settings.travelDistance * eased);
        const float outerDistance = innerDistance + ScaleToUI(settings.lineLength);

        // The four diagonals: up-right, up-left, down-left, down-right (screen y grows downwards).
        constexpr float Diagonal = std::numbers::sqrt2_v<float> / 2.0f;
        constexpr std::array<ImVec2, 4> Directions = {
            ImVec2(Diagonal, -Diagonal), ImVec2(-Diagonal, -Diagonal), ImVec2(-Diagonal, Diagonal), ImVec2(Diagonal, Diagonal)};

        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        const ImVec2 center = GetScreenCenter();
        for (const ImVec2& direction : Directions)
        {
            const ImVec2 from(center.x + direction.x * innerDistance, center.y + direction.y * innerDistance);
            const ImVec2 to(center.x + direction.x * outerDistance, center.y + direction.y * outerDistance);
            drawList->AddLine(from, to, ToImGuiColor(color), ScaleToUI(settings.thickness));
        }
    }
}
