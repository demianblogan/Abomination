#include "Gameplay/Effects/Fires.h"

#include "Core/Profiling/ProfileZone.h"
#include "Core/Scene/Transform.h"
#include "Renderer/Fire.h"
#include "Renderer/Light.h"

#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>

#include <cmath>

namespace Abomination::Gameplay
{
    namespace
    {
        // The sheets of Tools/TextureGenerator (RunFire): 16 frames, 4 to a row.
        constexpr int FlipbookFrameCount = 16;
        constexpr int FlipbookColumns = 4;

        // Frames per second: the torn flame of wood is quick, the drop of a candle slow.
        constexpr float WildFlameFrameRate = 12.0f;
        constexpr float CalmFlameFrameRate = 8.0f;

        // The bottom of the flame in its frame is a little above the bottom of the frame (the round bottom of the
        // flame): the sprite is lowered by this part of its height, so the flame sits on its wick or its coals.
        constexpr float FlameBottomInFrame = 0.12f;

        void EmitSpark(Effects& effects, const glm::vec3& bottom, const Renderer::Flame& flame)
        {
            Core::Random& random = effects.random;
            const float spread = flame.height * 0.2f;
            effects.particles.Emit(Particle{
                .position = bottom + glm::vec3(random.GetFloat(-spread, spread), flame.height * 0.4f,
                                               random.GetFloat(-spread, spread)),
                .velocity = {random.GetFloat(-0.3f, 0.3f), random.GetFloat(0.8f, 1.6f), random.GetFloat(-0.3f, 0.3f)},
                .lifetime = random.GetFloat(0.7f, 1.3f),
                .startHalfSize = 0.012f,
                .endHalfSize = 0.004f,
                .startColor = {1.0f, 0.75f, 0.35f, 1.0f},
                .endColor = {1.0f, 0.3f, 0.1f, 0.0f},
                .gravityScale = -0.05f,
                .drag = 0.8f,
                .texture = effects.textures.spark,
                .blend = Renderer::SpriteBlend::Additive,
            });
        }

        void EmitSmoke(Effects& effects, const glm::vec3& bottom, const Renderer::Flame& flame)
        {
            Core::Random& random = effects.random;
            effects.particles.Emit(Particle{
                .position = bottom + glm::vec3(0.0f, flame.height * 0.9f, 0.0f),
                .velocity = {random.GetFloat(-0.05f, 0.05f), random.GetFloat(0.35f, 0.55f), random.GetFloat(-0.05f, 0.05f)},
                .lifetime = random.GetFloat(2.0f, 3.0f),
                .startHalfSize = flame.height * 0.25f,
                .endHalfSize = flame.height * 1.0f,
                .rotation = random.GetFloat(0.0f, glm::two_pi<float>()),
                .rotationSpeed = random.GetFloat(-0.5f, 0.5f),
                .startColor = {0.25f, 0.24f, 0.22f, 0.3f},
                .endColor = {0.3f, 0.3f, 0.3f, 0.0f},
                .drag = 0.3f,
                .texture = effects.textures.smoke,
                .blend = Renderer::SpriteBlend::Alpha,
            });
        }
    }

    FlipbookFrame CalculateFlipbookFrame(int frame)
    {
        const int column = frame % FlipbookColumns;
        const int rowFromTop = frame / FlipbookColumns;
        constexpr float Step = 1.0f / FlipbookColumns;

        // The first row of the image is the top of the texture: the highest texture coordinates.
        return FlipbookFrame{
            .minimum = {column * Step, 1.0f - (rowFromTop + 1) * Step},
            .maximum = {(column + 1) * Step, 1.0f - rowFromTop * Step},
        };
    }

    float CalculateFlicker(float time, float speed, float phase)
    {
        // Weights add up to 1, so the sum stays within -1..1. The speeds are not multiples of each other.
        const float angle = time * speed + phase;
        return 0.5f * std::sin(angle) + 0.3f * std::sin(angle * 2.3f + phase) + 0.2f * std::sin(angle * 5.1f + 2.0f * phase);
    }

    void UpdateFires(entt::registry& registry, Effects& effects, float deltaTime)
    {
        PROFILE_ZONE();

        effects.time += deltaTime;
        const float time = effects.time;

        // The light goes brighter and dimmer and wanders a few centimeters around its place, each axis at its own pace.
        for (const auto [entity, flicker, light, transform] :
             registry.view<const Renderer::Flicker, Renderer::Light, Core::Transform>().each())
        {
            light.intensity = flicker.baseIntensity * (1.0f + flicker.strength * CalculateFlicker(time, flicker.speed,
                                                                                                   flicker.phase));
            const glm::vec3 wander(CalculateFlicker(time, flicker.speed * 0.7f, flicker.phase + 1.0f),
                                   CalculateFlicker(time, flicker.speed * 0.5f, flicker.phase + 2.0f),
                                   CalculateFlicker(time, flicker.speed * 0.6f, flicker.phase + 3.0f));
            transform.position = flicker.basePosition + wander * flicker.wander;
        }

        // Sparks and smoke at their rates; what is less than one particle this frame waits for the next ones.
        for (const auto [entity, flame, transform] : registry.view<Renderer::Flame, const Core::Transform>().each())
        {
            flame.sparksDue += flame.sparksPerSecond * deltaTime;
            for (; flame.sparksDue >= 1.0f; flame.sparksDue -= 1.0f)
                EmitSpark(effects, transform.position, flame);

            flame.smokeDue += flame.smokePerSecond * deltaTime;
            for (; flame.smokeDue >= 1.0f; flame.smokeDue -= 1.0f)
                EmitSmoke(effects, transform.position, flame);
        }
    }

    void AddFlameSprites(const entt::registry& registry, const Effects& effects, const glm::vec3& cameraPosition,
                         Renderer::SpriteBatch& batch)
    {
        for (const auto [entity, flame, transform] : registry.view<const Renderer::Flame, const Core::Transform>().each())
        {
            const bool isCalm = flame.kind == Renderer::FlameKind::Calm;
            const float frameRate = isCalm ? CalmFlameFrameRate : WildFlameFrameRate;

            // The phase also shifts the animation, so flames side by side show different frames.
            const float frames = effects.time * frameRate + flame.phase / glm::two_pi<float>() * FlipbookFrameCount;
            const int frame = static_cast<int>(std::floor(frames)) % FlipbookFrameCount;
            const FlipbookFrame part = CalculateFlipbookFrame(frame);

            // Turned to the camera only around the vertical: right is level and across the view.
            glm::vec3 toCamera = cameraPosition - transform.position;
            toCamera.y = 0.0f;
            const float distance = glm::length(toCamera);
            const glm::vec3 across =
                distance > 1e-4f ? glm::vec3(toCamera.z, 0.0f, -toCamera.x) / distance : glm::vec3(1.0f, 0.0f, 0.0f);

            const float halfSize = flame.height * 0.5f;
            const glm::vec3 center = transform.position + glm::vec3(0.0f, halfSize - flame.height * FlameBottomInFrame, 0.0f);
            batch.AddQuad(center, across * halfSize, glm::vec3(0.0f, halfSize, 0.0f), glm::vec4(1.0f),
                          isCalm ? effects.textures.candleFire : effects.textures.fire, Renderer::SpriteBlend::Additive,
                          part.minimum, part.maximum);
        }
    }
}
