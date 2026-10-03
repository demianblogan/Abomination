#include "UI/HUD.h"

#include "Core/Logging/Log.h"
#include "Core/Math/Spring.h"
#include "Gameplay/Camera/MouseLook.h"
#include "Gameplay/Characters/Armor.h"
#include "Gameplay/Characters/Health.h"
#include "Gameplay/GameplayState.h"
#include "Gameplay/Player/DamageReaction.h"
#include "Gameplay/Weapons/Ammo.h"
#include "Gameplay/Weapons/Crosshair.h"
#include "Gameplay/Weapons/Weapon.h"
#include "Renderer/Camera/CameraLens.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <memory>
#include <numbers>

namespace Abomination::UI
{
    namespace
    {
        // The window height the sizes in dp are written for (see GameUI).
        constexpr float ReferenceHeight = 1080.0f;

        // The red vignette of a blow: its strength for a blow of VignetteFullDamage or more, never weaker than
        // VignetteMinimum, and how fast it fades (strength per second). The green one of healing starts at
        // HealVignetteStrength and fades the same way.
        constexpr float VignetteFullDamage = 30.0f;
        constexpr float VignetteMinimum = 0.4f;
        constexpr float VignetteFadeRate = 1.6f;
        constexpr float HealVignetteStrength = 1.0f;
        constexpr float HealVignetteFadeRate = 2.0f;

        // The shake of the health and armor after a blow: how far (dp), how fast (back and forth per second) and how
        // long (seconds); it dies down evenly.
        constexpr float ShakeDistance = 8.0f;
        constexpr float ShakeFrequency = 18.0f;
        constexpr float ShakeDuration = 0.35f;

        // The blinking of the ammunition after an empty click: two blinks, each BlinkPeriod long, half of it nearly
        // invisible.
        constexpr float BlinkPeriod = 0.2f;
        constexpr int BlinkCount = 2;
        constexpr float BlinkOpacity = 0.15f;

        // The pulse of the health with a beat of the heart: how much bigger its icon and number get at the beat, and how fast
        // it shrinks back (pulse per second).
        constexpr float HeartPulseScale = 0.22f;
        constexpr float HeartPulseFadeRate = 6.0f;

        // The arcs of the damage direction: at this part of the window height from the middle, gone after
        // DamageArcDuration seconds (fading out all the time).
        constexpr float DamageArcRadius = 0.25f;
        constexpr float DamageArcDuration = 1.0f;

        // The angles of the four lines of a hit marker on the screen: up-right, up-left, down-left, down-right (+Y is
        // down on the screen, so up-right is -45 degrees).
        constexpr std::array<float, 4> MarkerAngles = {-45.0f, -135.0f, 135.0f, 45.0f};

        // A color as RCSS writes it: #rrggbbaa.
        std::string ToRCSSColor(const glm::vec4& color)
        {
            const auto channel = [](float value)
            {
                return static_cast<int>(std::lround(glm::clamp(value, 0.0f, 1.0f) * 255.0f));
            };
            return std::format("#{:02x}{:02x}{:02x}{:02x}", channel(color.r), channel(color.g), channel(color.b),
                               channel(color.a));
        }

        std::string ToPixels(float pixels)
        {
            return std::format("{:.2f}px", pixels);
        }

        std::string ToOpacity(float opacity)
        {
            return std::format("{:.3f}", glm::clamp(opacity, 0.0f, 1.0f));
        }

        // Sets an element visible or not; a hidden element is not drawn and does not take the mouse.
        void SetVisible(Rml::Element* element, bool isVisible)
        {
            if (element != nullptr)
                element->SetProperty("visibility", isVisible ? "visible" : "hidden");
        }
    }

    std::unique_ptr<HUD> HUD::Load(Rml::Context& context, const std::string& documentPath)
    {
        auto hudPointer = std::make_unique<HUD>();
        HUD& hud = *hudPointer;

        // The data model must exist before the document that uses it is loaded. Bind gives RmlUi the address of every
        // value: the HUD lives on the heap, so the addresses stay valid.
        Rml::DataModelConstructor constructor = context.CreateDataModel("hud");
        if (constructor)
        {
            constructor.Bind("health", &hud.m_health);
            constructor.Bind("armor", &hud.m_armor);
            constructor.Bind("ammo", &hud.m_ammo);
            constructor.Bind("lowHealth", &hud.m_lowHealth);
            constructor.Bind("ammoIcon", &hud.m_ammoIcon);
            hud.m_model = constructor.GetModelHandle();
        }

        hud.m_document = context.LoadDocument(documentPath);
        if (hud.m_document == nullptr)
        {
            Core::Log::Write(Core::LogCategory::UI, Core::LogLevel::Warning, "HUD not loaded: {}", documentPath);
            return hudPointer;
        }

        Rml::ElementDocument& document = *hud.m_document;
        hud.m_damageVignette = document.GetElementById("vignette-damage");
        hud.m_healVignette = document.GetElementById("vignette-heal");
        hud.m_leftCounters = document.GetElementById("counters-left");
        hud.m_healthCounter = document.GetElementById("health-counter");
        hud.m_ammoCounter = document.GetElementById("ammo-counter");
        hud.m_crosshairCircle = document.GetElementById("crosshair-circle");
        hud.m_crosshairDot = document.GetElementById("crosshair-dot");
        for (std::size_t index = 0; index < hud.m_markers.size(); ++index)
            hud.m_markers[index] = document.GetElementById(std::format("marker-{}", index));
        for (std::size_t index = 0; index < hud.m_damageArcs.size(); ++index)
            hud.m_damageArcs[index].element = document.GetElementById(std::format("damage-arc-{}", index));

        return hudPointer;
    }

    void HUD::Update(const Gameplay::GameplayState& gameplay, const entt::registry& registry, glm::vec2 viewportSize,
                     float deltaTime)
    {
        if (m_document == nullptr)
            return;

        // Only the living player has a HUD: the free-fly camera shows the world without it, and death hides it at once.
        const bool isVisible = gameplay.controlMode == Gameplay::ControlMode::Player &&
                               registry.get<Gameplay::Health>(gameplay.player).current > 0.0f;
        if (isVisible != m_document->IsVisible())
        {
            if (isVisible)
                m_document->Show();
            else
                m_document->Hide();
        }

        // The counts are followed even while the HUD is hidden, so a blow taken in the free-fly camera does not flash
        // when the HUD comes back.
        UpdateValues(gameplay, registry);
        if (!isVisible)
        {
            m_hasSeenCounts = false;
            return;
        }

        UpdateReactions(gameplay, registry, viewportSize, deltaTime);

        // The circle is exactly as wide as the spread on the screen (it shows where the pellets land), widened by the
        // pulse of a shot; the other sizes are in dp and follow the height of the window.
        const auto& weapon = registry.get<Gameplay::Weapon>(gameplay.player);
        const float verticalFOV = registry.get<Renderer::CameraLens>(gameplay.player).verticalFOV;
        const float spreadRadius =
            Gameplay::CalculateSpreadRadiusOnScreen(weapon.settings.spreadAngle, verticalFOV, viewportSize.y);
        UpdateCrosshair(gameplay, registry, spreadRadius * (1.0f + m_pulse), viewportSize.y / ReferenceHeight, deltaTime);
    }

    void HUD::UpdateValues(const Gameplay::GameplayState& gameplay, const entt::registry& registry)
    {
        // The numbers, rounded up so that 0.5 health left still shows 1 (0 means dead).
        const auto setValue = [this](int& value, int newValue, const char* name)
        {
            if (value == newValue)
                return;
            value = newValue;
            m_model.DirtyVariable(name);
        };

        const auto& health = registry.get<Gameplay::Health>(gameplay.player);
        const auto* armor = registry.try_get<Gameplay::Armor>(gameplay.player);
        const auto* reaction = registry.try_get<Gameplay::DamageReaction>(gameplay.player);
        setValue(m_health, static_cast<int>(std::ceil(health.current)), "health");
        setValue(m_armor, armor != nullptr ? static_cast<int>(std::ceil(armor->current)) : 0, "armor");
        setValue(m_lowHealth, reaction != nullptr ? static_cast<int>(reaction->lowHealth) : 0, "lowHealth");

        if (const auto* weapon = registry.try_get<Gameplay::Weapon>(gameplay.player); weapon != nullptr)
        {
            const auto* ammo = registry.try_get<Gameplay::Ammo>(gameplay.player);
            const Gameplay::AmmoType type = weapon->settings.ammoType;
            setValue(m_ammo, ammo != nullptr ? Gameplay::GetAmmo(*ammo, type) : 0, "ammo");

            const Rml::String icon = std::format("Icons/{}.png", Gameplay::AmmoTypeNames[static_cast<std::size_t>(type)]);
            if (icon != m_ammoIcon)
            {
                m_ammoIcon = icon;
                m_model.DirtyVariable("ammoIcon");
            }
        }
    }

    void HUD::UpdateReactions(const Gameplay::GameplayState& gameplay, const entt::registry& registry,
                              glm::vec2 viewportSize, float deltaTime)
    {
        const auto& reaction = registry.get<Gameplay::DamageReaction>(gameplay.player);
        const auto& weapon = registry.get<Gameplay::Weapon>(gameplay.player);
        if (!m_hasSeenCounts)
        {
            m_seenDamageCount = reaction.damageCount;
            m_seenHealCount = reaction.healCount;
            m_seenHeartbeatCount = reaction.heartbeatCount;
            m_seenEmptyClickCount = weapon.emptyClickCount;
            m_seenHitCount = weapon.hitCount;
            m_seenKillCount = weapon.killCount;
            m_seenShotCount = weapon.shotCount;
            m_hasSeenCounts = true;
        }

        // A blow: the vignette, the shake and an arc towards where it came from.
        if (reaction.damageCount != m_seenDamageCount)
        {
            m_seenDamageCount = reaction.damageCount;
            const float strength = std::max(VignetteMinimum, std::min(1.0f, reaction.lastDamage / VignetteFullDamage));
            m_damageVignetteStrength = std::max(m_damageVignetteStrength, strength);
            m_secondsSinceShake = 0.0f;

            if (reaction.lastDamageDirection.has_value())
            {
                DamageArc& arc = m_damageArcs[m_nextDamageArc];
                arc.direction = *reaction.lastDamageDirection;
                arc.age = 0.0f;
                m_nextDamageArc = (m_nextDamageArc + 1) % m_damageArcs.size();
            }
        }

        if (reaction.healCount != m_seenHealCount)
        {
            m_seenHealCount = reaction.healCount;
            m_healVignetteStrength = HealVignetteStrength;
        }

        if (reaction.heartbeatCount != m_seenHeartbeatCount)
        {
            m_seenHeartbeatCount = reaction.heartbeatCount;
            m_heartPulse = 1.0f;
        }

        if (weapon.emptyClickCount != m_seenEmptyClickCount)
        {
            m_seenEmptyClickCount = weapon.emptyClickCount;
            m_secondsSinceEmptyClick = 0.0f;
        }

        // The vignettes fade out evenly.
        m_damageVignetteStrength = std::max(0.0f, m_damageVignetteStrength - VignetteFadeRate * deltaTime);
        m_healVignetteStrength = std::max(0.0f, m_healVignetteStrength - HealVignetteFadeRate * deltaTime);
        if (m_damageVignette != nullptr)
            m_damageVignette->SetProperty("opacity", ToOpacity(m_damageVignetteStrength));
        if (m_healVignette != nullptr)
            m_healVignette->SetProperty("opacity", ToOpacity(m_healVignetteStrength));

        // The shake: back and forth sideways, dying down evenly.
        const float pixelsPerDp = viewportSize.y / ReferenceHeight;
        m_secondsSinceShake += deltaTime;
        if (m_leftCounters != nullptr)
        {
            float shake = 0.0f;
            if (m_secondsSinceShake < ShakeDuration)
            {
                const float fade = 1.0f - m_secondsSinceShake / ShakeDuration;
                shake = std::sin(m_secondsSinceShake * ShakeFrequency * 2.0f * std::numbers::pi_v<float>) * ShakeDistance *
                        pixelsPerDp * fade;
            }
            m_leftCounters->SetProperty("transform", std::format("translateX({:.2f}px)", shake));
        }

        // The pulse of the health (its icon and number) with the heart: big at the beat, shrinking back.
        m_heartPulse = std::max(0.0f, m_heartPulse - HeartPulseFadeRate * deltaTime);
        if (m_healthCounter != nullptr)
            m_healthCounter->SetProperty("transform", std::format("scale({:.3f})", 1.0f + HeartPulseScale * m_heartPulse));

        // The blinking of the ammunition: nearly invisible in the first half of every blink period.
        m_secondsSinceEmptyClick += deltaTime;
        if (m_ammoCounter != nullptr)
        {
            const bool isBlinking = m_secondsSinceEmptyClick < BlinkPeriod * BlinkCount;
            const bool isDim = isBlinking && std::fmod(m_secondsSinceEmptyClick, BlinkPeriod) < BlinkPeriod * 0.5f;
            m_ammoCounter->SetProperty("opacity", ToOpacity(isDim ? BlinkOpacity : 1.0f));
        }

        UpdateDamageArcs(registry.get<Gameplay::LookAngles>(gameplay.player).yaw, viewportSize.y * DamageArcRadius,
                         deltaTime);
    }

    void HUD::UpdateDamageArcs(float yaw, float radius, float deltaTime)
    {
        // Where the player looks and their right, along the ground (yaw 0 looks along -Z, a positive yaw turns left; see
        // LookAngles).
        const glm::vec3 forward(-std::sin(yaw), 0.0f, -std::cos(yaw));
        const glm::vec3 right(std::cos(yaw), 0.0f, -std::sin(yaw));

        for (DamageArc& arc : m_damageArcs)
        {
            arc.age += deltaTime;
            if (arc.element == nullptr)
                continue;

            if (arc.age >= DamageArcDuration)
            {
                arc.element->SetProperty("opacity", "0");
                continue;
            }

            // The angle of the blow on the screen, clockwise from straight up: up is in front of the player, right is to
            // their right, down is behind them. The arc is pushed up from the middle by the radius, then turned by that
            // angle around the middle (a CSS transform list applies from right to left).
            const float angle = glm::degrees(std::atan2(glm::dot(arc.direction, right), glm::dot(arc.direction, forward)));
            arc.element->SetProperty("transform", std::format("rotate({:.1f}deg) translateY({:.2f}px)", angle, -radius));
            arc.element->SetProperty("opacity", ToOpacity(1.0f - arc.age / DamageArcDuration));
        }
    }

    void HUD::UpdateCrosshair(const Gameplay::GameplayState& gameplay, const entt::registry& registry, float circleRadius,
                              float pixelsPerDp, float deltaTime)
    {
        const auto& weapon = registry.get<Gameplay::Weapon>(gameplay.player);
        const Gameplay::CrosshairSettings& settings = weapon.crosshair;

        // The markers and the pulse react to the counts of the weapon (see Weapon::hitCount).
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

        // A new shot kicks the pulse; the spring brings the circle back to the spread.
        if (weapon.shotCount != m_seenShotCount)
        {
            m_seenShotCount = weapon.shotCount;
            m_pulseVelocity += settings.pulseKick;
        }
        Core::UpdateDampedSpring(m_pulse, m_pulseVelocity, settings.pulseStiffness, deltaTime);

        const std::string color = ToRCSSColor(settings.color);

        // The circle: a square box with a border, rounded into a ring. The middle of its border lies on the radius, so
        // the box is 2 * radius - thickness wide inside the border and starts half a border outside the radius.
        if (m_crosshairCircle != nullptr)
        {
            const float thickness = settings.circleThickness * pixelsPerDp;
            const float outerRadius = circleRadius + thickness * 0.5f;
            m_crosshairCircle->SetProperty("width", ToPixels(2.0f * circleRadius - thickness));
            m_crosshairCircle->SetProperty("height", ToPixels(2.0f * circleRadius - thickness));
            m_crosshairCircle->SetProperty("left", ToPixels(-outerRadius));
            m_crosshairCircle->SetProperty("top", ToPixels(-outerRadius));
            m_crosshairCircle->SetProperty("border-width", ToPixels(thickness));
            m_crosshairCircle->SetProperty("border-color", color);
            m_crosshairCircle->SetProperty("border-radius", ToPixels(outerRadius));
        }

        if (m_crosshairDot != nullptr)
        {
            const float dotRadius = settings.dotRadius * pixelsPerDp;
            m_crosshairDot->SetProperty("width", ToPixels(2.0f * dotRadius));
            m_crosshairDot->SetProperty("height", ToPixels(2.0f * dotRadius));
            m_crosshairDot->SetProperty("left", ToPixels(-dotRadius));
            m_crosshairDot->SetProperty("top", ToPixels(-dotRadius));
            m_crosshairDot->SetProperty("border-radius", ToPixels(dotRadius));
            m_crosshairDot->SetProperty("background-color", color);
        }

        // A kill marker covers a hit marker of the same shot.
        if (m_secondsSinceKill < settings.killMarker.duration)
            UpdateMarker(&settings.killMarker, m_secondsSinceKill, circleRadius, pixelsPerDp);
        else if (m_secondsSinceHit < settings.hitMarker.duration)
            UpdateMarker(&settings.hitMarker, m_secondsSinceHit, circleRadius, pixelsPerDp);
        else
            UpdateMarker(nullptr, 0.0f, circleRadius, pixelsPerDp);
    }

    void HUD::UpdateMarker(const Gameplay::HitMarkerSettings* settings, float secondsSinceStart, float circleRadius,
                           float pixelsPerDp)
    {
        if (settings == nullptr)
        {
            for (Rml::Element* marker : m_markers)
                SetVisible(marker, false);
            return;
        }

        // How far through the marker it is, 0 to 1. It moves fast at first and slows down at the end (ease-out: 1 - (1 -
        // t)^2), which reads as a quick flick rather than a slide, and fades out evenly.
        const float progress = glm::clamp(secondsSinceStart / settings->duration, 0.0f, 1.0f);
        const float eased = 1.0f - (1.0f - progress) * (1.0f - progress);
        glm::vec4 color = settings->color;
        color.a *= 1.0f - progress;

        // Every line starts at the middle of the screen; its transform turns it to its diagonal and pushes it out to
        // where its inner end is.
        const float innerDistance = circleRadius + (settings->startGap + settings->travelDistance * eased) * pixelsPerDp;
        const float thickness = settings->thickness * pixelsPerDp;
        for (std::size_t index = 0; index < m_markers.size(); ++index)
        {
            Rml::Element* marker = m_markers[index];
            if (marker == nullptr)
                continue;

            SetVisible(marker, true);
            marker->SetProperty("width", ToPixels(settings->lineLength * pixelsPerDp));
            marker->SetProperty("height", ToPixels(thickness));
            marker->SetProperty("margin-top", ToPixels(-thickness * 0.5f));
            marker->SetProperty("background-color", ToRCSSColor(color));
            marker->SetProperty("transform", std::format("rotate({:.1f}deg) translateX({:.2f}px)", MarkerAngles[index],
                                                         innerDistance));
        }
    }
}
