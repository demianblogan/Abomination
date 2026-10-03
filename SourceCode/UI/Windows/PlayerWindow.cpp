#include "UI/Windows/PlayerWindow.h"

#include "Core/Scene/Transform.h"
#include "Gameplay/Characters/Armor.h"
#include "Gameplay/Characters/Health.h"
#include "Gameplay/GameplayState.h"
#include "Gameplay/Player/DamageReaction.h"
#include "Gameplay/Weapons/Ammo.h"
#include "UI/UIScale.h"
#include "UI/Widgets.h"

#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>

#include <cmath>
#include <cstddef>
#include <numbers>

namespace Abomination::UI
{
    namespace
    {
        // The window opens for the first time at the left edge, below the Movement window.
        constexpr ImVec2 InitialPosition(10.0f, 640.0f);

        // What the test buttons do. The blow comes from a point this far from the player, on a random side.
        constexpr float TestDamage = 15.0f;
        constexpr float TestArmor = 50.0f;
        constexpr float TestBlowDistance = 2.0f;
    }

    void DrawPlayerWindow(bool* isOpen, Gameplay::GameplayState& gameplay, entt::registry& registry,
                          Audio::AudioEngine& audio)
    {
        ImGui::SetNextWindowPos(ScaleToUI(InitialPosition), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Player", isOpen))
        {
            ImGui::End();
            return;
        }

        auto& health = registry.get<Gameplay::Health>(gameplay.player);
        auto& armor = registry.get<Gameplay::Armor>(gameplay.player);
        auto& ammo = registry.get<Gameplay::Ammo>(gameplay.player);
        auto& reaction = registry.get<Gameplay::DamageReaction>(gameplay.player);

        ImGui::SeparatorText("Health and armor");
        DrawSlider("Health", health.current, 0.0f, health.maximum, "%.0f");
        DrawSlider("Armor", armor.current, 0.0f, armor.maximum, "%.0f",
                   "Takes two thirds of every hit while it lasts. The HUD hides it at 0.");

        // The same blow as an enemy will strike: through the armor, from a random side, with every reaction.
        if (ImGui::Button("Hurt"))
        {
            const float angle = reaction.random.GetFloat(0.0f, 2.0f * std::numbers::pi_v<float>);
            const glm::vec3 position = registry.get<Core::Transform>(gameplay.player).position;
            const glm::vec3 source = position + glm::vec3(std::cos(angle), 0.0f, std::sin(angle)) * TestBlowDistance;
            static_cast<void>(Gameplay::DamagePlayer(gameplay, registry, audio,
                                                     {.amount = TestDamage, .sourcePosition = source}));
        }
        ImGui::SetItemTooltip("A blow of 15 damage from a random side, through the armor.");
        ImGui::SameLine();
        if (ImGui::Button("Give armor"))
            armor.current = armor.current + TestArmor < armor.maximum ? armor.current + TestArmor : armor.maximum;
        ImGui::SameLine();
        if (ImGui::Button("Heal"))
            Gameplay::HealPlayer(gameplay, registry, audio, health.maximum);
        ImGui::SetItemTooltip("Back to full health, with a sigh of relief.");
        ImGui::SameLine();
        if (ImGui::Button("Revive"))
            health.current = health.maximum;
        ImGui::SetItemTooltip("Full health without the sigh: also brings a dead player back.");

        ImGui::SeparatorText("Ammunition");
        for (std::size_t index = 0; index < ammo.counts.size(); ++index)
            DrawIntSlider(Gameplay::AmmoTypeNames[index].data(), ammo.counts[index], 0, ammo.maximums[index]);

        if (ImGui::Button("Fill up"))
            ammo.counts = ammo.maximums;

        ImGui::SeparatorText("Reaction to damage");
        DrawSlider("Punch", reaction.punchKick, 0.0f, 4.0f, "%.2f",
                   "How hard a blow of 20 damage jerks the view in a random direction. 0 turns it off.");
        DrawSlider("Punch stiffness", reaction.punchStiffness, 20.0f, 500.0f, "%.0f",
                   "How fast the view comes back. Bigger: quicker and smaller jerks.");
        DrawSlider("Muffle at damage", reaction.muffleFullDamage, 5.0f, 100.0f, "%.0f",
                   "A blow this strong muffles the world fully; weaker ones less.");
        DrawSlider("Muffle duration", reaction.muffleDuration, 0.0f, 2.0f, "%.2f s",
                   "How long the world takes to sound clear again. 0 turns the muffle off.");
        DrawSlider("Low health", reaction.lowHealth, 0.0f, 100.0f, "%.0f",
                   "At this health or less the heart beats and the HUD pulses with it.");

        ImGui::End();
    }
}
