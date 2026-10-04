#include "UI/Windows/EnemiesWindow.h"

#include "Gameplay/Characters/Health.h"
#include "Gameplay/Enemies/Monsters.h"
#include "Gameplay/GameplayState.h"
#include "UI/UIScale.h"
#include "UI/Widgets.h"

#include <imgui.h>

#include <cstddef>

namespace Abomination::UI
{
    namespace
    {
        // The window opens for the first time next to the Player window.
        constexpr ImVec2 InitialPosition(560.0f, 120.0f);

        void DrawDogTab(Gameplay::GameplayState& gameplay, entt::registry& registry)
        {
            Gameplay::DogSettings& settings = gameplay.dogSettings;

            ImGui::Checkbox("Show senses", &gameplay.areDogSensesVisible);
            ImGui::SetItemTooltip("Lines on the floor around every dog: its sight (yellow), the radius it smells the player\n"
                                  "in (orange), the range it hears shots in (blue), the area it patrols (green).");
            ImGui::SameLine();
            ImGui::Checkbox("Freeze", &gameplay.areMonstersFrozen);
            ImGui::SetItemTooltip("Every monster stands where it is and decides nothing.");

            // Every dog with what it does now.
            ImGui::SeparatorText("Dogs");
            int number = 0;
            for (const auto [entity, dog, health] : registry.view<const Gameplay::Dog, const Gameplay::Health>().each())
            {
                ImGui::Text("Dog %d: %s, health %.0f / %.0f", ++number,
                            Gameplay::DogStateNames[static_cast<std::size_t>(dog.mind.state)].data(), health.current,
                            health.maximum);
            }
            if (number == 0)
                ImGui::TextUnformatted("No dogs in the level.");

            ImGui::SeparatorText("Senses");
            DrawSlider("Sight range", settings.sightRange, 1.0f, 60.0f, "%.1f m", "How far it sees (a wall hides the player).");
            DrawDegreeSlider("Field of view", settings.fieldOfView, 10.0f, 360.0f, "%.0f deg",
                             "How wide it sees: the whole angle, centered on where it faces.");
            DrawSlider("Sense radius", settings.senseRadius, 0.0f, 15.0f, "%.1f m",
                       "The player closer than this is noticed whatever way the dog faces, even behind a wall.");
            DrawSlider("Hearing range", settings.hearingRange, 0.0f, 80.0f, "%.1f m", "How far a shot is heard.");
            DrawSlider("Lose distance", settings.loseDistance, 2.0f, 100.0f, "%.1f m",
                       "Once it chases the player, it follows them everywhere until they are farther than this.");

            ImGui::SeparatorText("Movement");
            DrawSlider("Walk speed", settings.walkSpeed, 0.1f, 5.0f, "%.2f m/s");
            DrawSlider("Walk animation", settings.walkAnimationSpeed, 0.1f, 3.0f, "%.2f x", "How fast the walk clip plays.");
            DrawSlider("Run speed", settings.runSpeed, 0.5f, 15.0f, "%.2f m/s");
            DrawSlider("Run animation", settings.runAnimationSpeed, 0.1f, 3.0f, "%.2f x", "How fast the run clip plays.");
            DrawDegreeSlider("Turn speed", settings.turnSpeed, 30.0f, 1440.0f, "%.0f deg/s",
                             "How fast it turns on the spot before a walk.");
            DrawDegreeSlider("Run turn speed", settings.runTurnSpeed, 30.0f, 1440.0f, "%.0f deg/s",
                             "How fast it turns while it runs at the player.");

            ImGui::SeparatorText("Patrol");
            DrawSlider("Patrol radius", settings.patrolRadius, 0.0f, 30.0f, "%.1f m",
                       "It never wanders farther than this from where it appeared.");
            DrawSlider("Idle at least", settings.idleTimeMinimum, 0.0f, 20.0f, "%.1f s");
            DrawSlider("Idle at most", settings.idleTimeMaximum, 0.0f, 20.0f, "%.1f s");
            DrawSlider("Give up walk after", settings.patrolTime, 1.0f, 30.0f, "%.1f s");

            ImGui::SeparatorText("Attack");
            DrawSlider("Damage", settings.damage, 0.0f, 100.0f, "%.0f", "Health one bite takes from the player.");
            DrawSlider("Bite range", settings.biteRange, 0.3f, 4.0f, "%.2f m", "Closer than this it stops and bites.");
            DrawSlider("Bite reach", settings.biteReach, 0.3f, 4.0f, "%.2f m",
                       "The bite hurts if the player is still within this when the teeth close.");
            DrawSlider("Leap from", settings.leapRangeMinimum, 0.5f, 10.0f, "%.2f m");
            DrawSlider("Leap to", settings.leapRangeMaximum, 0.5f, 15.0f, "%.2f m",
                       "Between these distances it jumps at the player instead of running.");
            DrawSlider("Leap speed", settings.leapSpeed, 0.0f, 15.0f, "%.2f m/s");
            DrawSlider("Leap height", settings.leapUpSpeed, 0.0f, 10.0f, "%.2f m/s",
                       "How fast it goes up at the start of a leap.");
            DrawSlider("Leap cooldown", settings.leapCooldown, 0.0f, 10.0f, "%.1f s");

            ImGui::Separator();
            if (ImGui::Button("Reset"))
                settings = Gameplay::DogSettings{};
        }
    }

    void DrawEnemiesWindow(bool* isOpen, Gameplay::GameplayState& gameplay, entt::registry& registry)
    {
        ImGui::SetNextWindowPos(ScaleToUI(InitialPosition), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Enemies", isOpen))
        {
            ImGui::End();
            return;
        }

        if (ImGui::BeginTabBar("EnemyTabs"))
        {
            if (ImGui::BeginTabItem("Dog"))
            {
                DrawDogTab(gameplay, registry);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::End();
    }
}
