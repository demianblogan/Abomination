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
            ImGui::SameLine();
            ImGui::Checkbox("Show navmesh", &gameplay.isNavMeshVisible);
            ImGui::SetItemTooltip("The floor the dogs can walk on, cut into the polygons they find their way through.");
            ImGui::SetNextItemWidth(ScaleToUI(120.0f));
            ImGui::SliderInt("Bodies kept", &gameplay.maximumCorpses, 0, 64);
            ImGui::SetItemTooltip("At most this many bodies of dead monsters stay in the level;\n"
                                  "the oldest sinks into the floor.");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ScaleToUI(120.0f));
            ImGui::SliderFloat("Burst at", &gameplay.gibs.settings.burstDamage, 0.0f, 200.0f, "%.0f");
            ImGui::SetItemTooltip("A body bursts into gibs once the damage it took beyond death reaches this\n"
                                  "(40: a close shot at a wounded dog, or one shot at its body).");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(ScaleToUI(120.0f));
            ImGui::SliderInt("Gib groups kept", &gameplay.gibs.settings.maximumGroups, 0, 32);
            ImGui::SetItemTooltip("At most this many bursts of gibs lie in the level; the oldest sinks into the floor.");

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

            ImGui::SeparatorText("Voice");
            DrawSlider("Bark at least every", settings.barkIntervalMinimum, 0.1f, 10.0f, "%.1f s");
            DrawSlider("Bark at most every", settings.barkIntervalMaximum, 0.1f, 10.0f, "%.1f s",
                       "While it chases the player it barks after a random pause between these.");

            ImGui::SeparatorText("Attack");
            DrawSlider("Damage", settings.damage, 0.0f, 100.0f, "%.0f", "Health one bite takes from the player.");
            DrawSlider("Bite range", settings.biteRange, 0.3f, 4.0f, "%.2f m", "Closer than this it stops and bites.");
            DrawSlider("Bite delay", settings.biteTime, 0.0f, 1.0f, "%.2f s",
                       "How long after the bite starts the teeth close: the time the player has to step away.");
            DrawSlider("Bite reach", settings.biteReach, 0.3f, 4.0f, "%.2f m",
                       "The bite hurts if the player is still within this when the teeth close.");
            DrawSlider("Attack height", settings.attackHeightDifference, 0.2f, 4.0f, "%.2f m",
                       "How much higher or lower the player may be for a bite or a leap (a balcony is 4 m).");
            DrawSlider("Leap from", settings.leapRangeMinimum, 0.5f, 10.0f, "%.2f m");
            DrawSlider("Leap to", settings.leapRangeMaximum, 0.5f, 15.0f, "%.2f m",
                       "Between these distances it jumps at the player instead of running.");
            DrawSlider("Leap takeoff", settings.leapTakeoffTime, 0.0f, 1.0f, "%.2f s",
                       "When it pushes off after the leap starts: the moment the jump clip leaves the ground\n"
                       "(see Clip time in the Animation window).");
            DrawSlider("Leap height", settings.leapHeight, 0.05f, 2.0f, "%.2f m",
                       "How high it rises on the way; it always lands where the player was at takeoff.");
            DrawSlider("Leap max speed", settings.leapSpeedMaximum, 1.0f, 30.0f, "%.1f m/s",
                       "The fastest it flies along the ground, however far the player is.");
            DrawSlider("Leap recovery", settings.leapRecoveryTime, 0.0f, 2.0f, "%.2f s",
                       "How long it stands after landing before it runs again.");
            DrawSlider("Leap cooldown", settings.leapCooldown, 0.0f, 10.0f, "%.1f s");

            ImGui::SeparatorText("Body");
            ImGui::Checkbox("Avoid ledges", &settings.avoidsLedges);
            ImGui::SetItemTooltip("On a patrol it does not step where a corner of its box would hang over a drop.");
            ImGui::Checkbox("Fit to ground", &settings.fitsToGround);
            ImGui::SetItemTooltip("Its model is tilted and lowered to the ground under its paws (stairs, edges).");
            DrawSlider("Paw distance", settings.pawDistance, 0.05f, 1.0f, "%.2f m",
                       "How far in front of and behind its middle the ground is looked for.");
            DrawDegreeSlider("Maximum tilt", settings.maximumTilt, 0.0f, 60.0f, "%.0f deg");
            DrawSlider("Height follow", settings.heightFollowSpeed, 0.1f, 10.0f, "%.1f m/s",
                       "How fast the model glides up and down to the ground.");
            DrawDegreeSlider("Tilt follow", settings.tiltFollowSpeed, 10.0f, 720.0f, "%.0f deg/s",
                             "How fast the model tilts to the ground.");

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
