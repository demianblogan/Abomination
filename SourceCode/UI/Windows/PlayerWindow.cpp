#include "UI/Windows/PlayerWindow.h"

#include "Gameplay/Characters/Armor.h"
#include "Gameplay/Characters/Health.h"
#include "Gameplay/Weapons/Ammo.h"
#include "UI/UIScale.h"
#include "UI/Widgets.h"

#include <imgui.h>

#include <cstddef>

namespace Abomination::UI
{
    namespace
    {
        // The window opens for the first time at the left edge, below the Movement window.
        constexpr ImVec2 InitialPosition(10.0f, 640.0f);

        // What the test buttons do.
        constexpr float TestDamage = 15.0f;
        constexpr float TestArmor = 50.0f;
    }

    void DrawPlayerWindow(bool* isOpen, Gameplay::Health& health, Gameplay::Armor& armor, Gameplay::Ammo& ammo)
    {
        ImGui::SetNextWindowPos(ScaleToUI(InitialPosition), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Player", isOpen))
        {
            ImGui::End();
            return;
        }

        ImGui::SeparatorText("Health and armor");
        DrawSlider("Health", health.current, 0.0f, health.maximum, "%.0f");
        DrawSlider("Armor", armor.current, 0.0f, armor.maximum, "%.0f",
                   "Takes two thirds of every hit while it lasts. The HUD hides it at 0.");

        // The same damage as an enemy will deal: through the armor, with the red flash of the HUD.
        if (ImGui::Button("Hurt"))
            static_cast<void>(Gameplay::ApplyDamage(health, armor, TestDamage));
        ImGui::SetItemTooltip("15 damage, through the armor.");
        ImGui::SameLine();
        if (ImGui::Button("Give armor"))
            armor.current = armor.current + TestArmor < armor.maximum ? armor.current + TestArmor : armor.maximum;
        ImGui::SameLine();
        if (ImGui::Button("Heal"))
            health.current = health.maximum;

        ImGui::SeparatorText("Ammunition");
        for (std::size_t index = 0; index < ammo.counts.size(); ++index)
            DrawIntSlider(Gameplay::AmmoTypeNames[index].data(), ammo.counts[index], 0, ammo.maximums[index]);

        if (ImGui::Button("Fill up"))
            ammo.counts = ammo.maximums;

        ImGui::End();
    }
}
