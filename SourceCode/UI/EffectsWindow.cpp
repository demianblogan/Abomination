#include "UI/EffectsWindow.h"

#include "Gameplay/Effects.h"
#include "UI/UIScale.h"

#include <imgui.h>

namespace Abomination::UI
{
    namespace
    {
        // The window opens for the first time next to the Weapon window.
        constexpr ImVec2 InitialPosition(560.0f, 40.0f);

        // The width of the sliders in pixels at 100% scale.
        constexpr float SliderWidth = 180.0f;

        void DrawSlider(const char* label, float& value, float minimum, float maximum, const char* format)
        {
            ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
            ImGui::SliderFloat(label, &value, minimum, maximum, format);
        }

        void DrawCountSlider(const char* label, int& value, int maximum)
        {
            ImGui::SetNextItemWidth(ScaleToUI(SliderWidth));
            ImGui::SliderInt(label, &value, 0, maximum);
        }

        // The four sliders every kind of impact particle has. PushID keeps the equal labels of the kinds apart for ImGui.
        void DrawParticleSettings(const char* title, int& count, float& speed, float& lifetime, float& halfSize,
                                  float maximumSpeed)
        {
            ImGui::SeparatorText(title);
            ImGui::PushID(title);
            DrawCountSlider("Count", count, 30);
            DrawSlider("Speed", speed, 0.0f, maximumSpeed, "%.2f m/s");
            DrawSlider("Lifetime", lifetime, 0.05f, 3.0f, "%.2f s");
            DrawSlider("Half size", halfSize, 0.005f, 0.5f, "%.3f m");
            ImGui::PopID();
        }
    }

    void DrawEffectsWindow(bool* isOpen, Gameplay::Effects& effects)
    {
        ImGui::SetNextWindowPos(ScaleToUI(InitialPosition), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Effects", isOpen, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::End();
            return;
        }

        Gameplay::EffectSettings& settings = effects.settings;
        ImGui::Text("Particles: %zu / %zu", effects.particles.GetCount(), Gameplay::ParticleSystem::MaximumParticleCount);
        ImGui::Text("Marks on walls: %zu", effects.decals.size());

        DrawParticleSettings("Sparks (wall)", settings.sparkCount, settings.sparkSpeed, settings.sparkLifetime,
                             settings.sparkHalfSize, 15.0f);
        DrawParticleSettings("Dust (wall)", settings.dustCount, settings.dustSpeed, settings.dustLifetime,
                             settings.dustHalfSize, 5.0f);
        DrawParticleSettings("Blood (character)", settings.bloodCount, settings.bloodSpeed, settings.bloodLifetime,
                             settings.bloodHalfSize, 10.0f);

        ImGui::SeparatorText("Muzzle");
        DrawSlider("Flash duration", settings.flashDuration, 0.0f, 0.3f, "%.3f s");
        DrawSlider("Flash half size", settings.flashHalfSize, 0.01f, 0.4f, "%.3f m");
        ImGui::SetItemTooltip("In the space of the eyes, where the weapon in the hands is: the shotgun is 1.1 m long.");
        DrawCountSlider("Smoke count", settings.smokeCount, 20);
        DrawSlider("Smoke lifetime", settings.smokeLifetime, 0.1f, 5.0f, "%.2f s");
        DrawSlider("Smoke half size", settings.smokeHalfSize, 0.01f, 1.0f, "%.3f m");

        ImGui::SeparatorText("Marks on walls");
        DrawSlider("Mark half size", settings.markHalfSize, 0.005f, 0.2f, "%.3f m");
        DrawCountSlider("Most marks", settings.maximumMarkCount, 512);
        ImGui::SetItemTooltip("Beyond this many the oldest marks disappear.");

        if (ImGui::Button("Reset"))
            settings = Gameplay::EffectSettings{};
        ImGui::SameLine();
        if (ImGui::Button("Clear"))
            Gameplay::ClearEffects(effects);

        ImGui::End();
    }
}
