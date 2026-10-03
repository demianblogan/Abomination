#pragma once

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Types.h>

#include <entt/entt.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <memory>
#include <string>

namespace Rml
{
    class Context;
    class Element;
    class ElementDocument;
}

namespace Abomination::Gameplay
{
    struct GameplayState;
    struct HitMarkerSettings;
}

namespace Abomination::UI
{
    // The HUD (Assets/UI/HUD.rml): health and armor in the bottom left corner, the ammunition of the weapon in the bottom
    // right, the crosshair with its hit markers in the middle. It reacts to what happens to the player:
    //   - a blow: a red vignette (stronger for a bigger blow), a shake of the health and armor, and an arc around the
    //     crosshair pointing to where the blow came from, which keeps pointing there while the player turns;
    //   - healing: a light green vignette;
    //   - low health: the health (its icon and number) pulses with every beat of the heart;
    //   - a press of Fire without ammunition: the ammunition blinks twice.
    // Shown while the player is controlled and alive.
    //
    // The numbers reach the document through a data model of RmlUi ("hud"): the document writes {{health}}, and the HUD
    // only tells RmlUi when a number changed. Everything that moves every frame is moved and colored directly. The HUD
    // notices events by the counts the game keeps (Gameplay::DamageReaction, Weapon::hitCount, ...): it remembers the
    // counts it has seen and reacts when one grows.
    class HUD
    {
    public:
        // Creates the data model and loads the document into the context. If the document cannot be loaded, the HUD
        // stays empty (a warning is logged) and the game goes on. On the heap, because the data model keeps the addresses
        // of its values: the HUD must never move.
        [[nodiscard]] static std::unique_ptr<HUD> Load(Rml::Context& context, const std::string& documentPath);

        // Once per frame, before the context updates. viewportSize in pixels, deltaTime in seconds.
        void Update(const Gameplay::GameplayState& gameplay, const entt::registry& registry, glm::vec2 viewportSize,
                    float deltaTime);

    private:
        // An arc of the damage direction: where the blow came from (a direction in the world) and how long ago.
        struct DamageArc
        {
            Rml::Element* element = nullptr;
            glm::vec3 direction{0.0f};
            float age = 1000.0f;
        };

        // The numbers and the counts of the game this frame.
        void UpdateValues(const Gameplay::GameplayState& gameplay, const entt::registry& registry);

        // The reactions to blows, healing, heartbeats and empty clicks.
        void UpdateReactions(const Gameplay::GameplayState& gameplay, const entt::registry& registry,
                             glm::vec2 viewportSize, float deltaTime);

        // Turns every arc of the damage direction towards its blow, as seen from where the player looks now (yaw,
        // radians), at radius pixels from the middle of the screen.
        void UpdateDamageArcs(float yaw, float radius, float deltaTime);

        // Moves the parts of the crosshair: the circle of circleRadius pixels, the dot, the markers. pixelsPerDp turns
        // the sizes of the settings (dp) into pixels.
        void UpdateCrosshair(const Gameplay::GameplayState& gameplay, const entt::registry& registry, float circleRadius,
                             float pixelsPerDp, float deltaTime);

        // Places the four lines of a marker secondsSinceStart after it began, or hides them (settings nullptr).
        void UpdateMarker(const Gameplay::HitMarkerSettings* settings, float secondsSinceStart, float circleRadius,
                          float pixelsPerDp);

        Rml::ElementDocument* m_document = nullptr;
        Rml::DataModelHandle m_model;

        // The values the document shows (bound to the data model).
        int m_health = 0;
        int m_armor = 0;
        int m_ammo = 0;
        int m_lowHealth = 0;
        Rml::String m_ammoIcon;

        Rml::Element* m_damageVignette = nullptr;
        Rml::Element* m_healVignette = nullptr;
        Rml::Element* m_leftCounters = nullptr;
        Rml::Element* m_healthCounter = nullptr;
        Rml::Element* m_ammoCounter = nullptr;
        Rml::Element* m_crosshairCircle = nullptr;
        Rml::Element* m_crosshairDot = nullptr;
        std::array<Rml::Element*, 4> m_markers{};
        std::array<DamageArc, 4> m_damageArcs{};
        std::size_t m_nextDamageArc = 0;

        // The counts of the game already seen. Unseen at first, so nothing flashes when the HUD first appears.
        bool m_hasSeenCounts = false;
        int m_seenDamageCount = 0;
        int m_seenHealCount = 0;
        int m_seenHeartbeatCount = 0;
        int m_seenEmptyClickCount = 0;
        int m_seenHitCount = 0;
        int m_seenKillCount = 0;
        int m_seenShotCount = 0;

        // How strong the vignettes are now (0 to 1), how long ago the shake, the blinking and the last hit and kill
        // markers started, and how far through its beat the pulse is (1 at the beat, then down to 0).
        float m_damageVignetteStrength = 0.0f;
        float m_healVignetteStrength = 0.0f;
        float m_secondsSinceShake = 1000.0f;
        float m_secondsSinceEmptyClick = 1000.0f;
        float m_heartPulse = 0.0f;
        float m_secondsSinceHit = 1000.0f;
        float m_secondsSinceKill = 1000.0f;

        // The pulse of the circle after a shot: how much wider it is (a part of its radius) and how fast that changes.
        float m_pulse = 0.0f;
        float m_pulseVelocity = 0.0f;
    };
}
