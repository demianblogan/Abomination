#include "Gameplay/Effects/Effects.h"

#include "Core/Assets/AssetLifetime.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

namespace Abomination::Gameplay
{
    namespace
    {
        // A mark lies this far in front of its wall (meters), so it is drawn over the wall and not inside it.
        constexpr float MarkLift = 0.004f;

        // A random direction within the half of all directions around axis (a unit vector), leaning towards it by
        // focus: 0 spreads over the whole half, 1 goes straight along axis.
        glm::vec3 RandomDirectionAround(Core::Random& random, const glm::vec3& axis, float focus)
        {
            // A random direction of all: three random numbers from -1 to 1, tried until they fall inside the ball of
            // radius 1 (so every direction is equally likely), then made unit length.
            glm::vec3 direction;
            do
            {
                direction = glm::vec3(random.GetFloat(-1.0f, 1.0f), random.GetFloat(-1.0f, 1.0f), random.GetFloat(-1.0f, 1.0f));
            } while (glm::dot(direction, direction) > 1.0f || glm::dot(direction, direction) < 0.0001f);
            direction = glm::normalize(direction);

            // Flipped into the half of axis, then pulled towards it.
            if (glm::dot(direction, axis) < 0.0f)
                direction = -direction;
            return glm::normalize(glm::mix(direction, axis, focus));
        }
    }

    EffectTextures LoadEffectTextures(Renderer::TextureStore& textures)
    {
        const auto load = [&textures](const char* name)
        {
            return textures.Load(std::string("Textures/Effects/") + name + ".png", Core::AssetLifetime::Global);
        };

        return EffectTextures{
            .muzzleFlash = load("MuzzleFlash"),
            .spark = load("Spark"),
            .smoke = load("Smoke"),
            .dust = load("Dust"),
            .pelletMark = load("PelletMark"),
            .blood = load("Blood"),
        };
    }

    void SpawnWallImpact(Effects& effects, const glm::vec3& point, const glm::vec3& normal)
    {
        const EffectSettings& settings = effects.settings;
        Core::Random& random = effects.random;

        for (int spark = 0; spark < settings.sparkCount; ++spark)
        {
            effects.particles.Emit(Particle{
                .position = point + normal * 0.02f,
                .velocity = RandomDirectionAround(random, normal, 0.3f) * settings.sparkSpeed * random.GetFloat(0.6f, 1.2f),
                .lifetime = settings.sparkLifetime * random.GetFloat(0.7f, 1.3f),
                .startHalfSize = settings.sparkHalfSize,
                .endHalfSize = settings.sparkHalfSize * 0.5f,
                .startColor = {1.0f, 1.0f, 1.0f, 1.0f},
                .endColor = {1.0f, 0.6f, 0.2f, 0.0f},
                .gravityScale = 1.0f,
                .drag = 1.0f,
                .texture = effects.textures.spark,
                .blend = Renderer::SpriteBlend::Additive,
            });
        }

        for (int puff = 0; puff < settings.dustCount; ++puff)
        {
            effects.particles.Emit(Particle{
                .position = point + normal * 0.05f,
                .velocity = RandomDirectionAround(random, normal, 0.6f) * settings.dustSpeed * random.GetFloat(0.5f, 1.2f),
                .lifetime = settings.dustLifetime * random.GetFloat(0.8f, 1.2f),
                .startHalfSize = settings.dustHalfSize * 0.4f,
                .endHalfSize = settings.dustHalfSize,
                .rotation = random.GetFloat(0.0f, 2.0f * std::numbers::pi_v<float>),
                .rotationSpeed = random.GetFloat(-1.0f, 1.0f),
                .startColor = {1.0f, 1.0f, 1.0f, 0.8f},
                .endColor = {1.0f, 1.0f, 1.0f, 0.0f},
                .gravityScale = 0.05f,
                .drag = 3.0f,
                .texture = effects.textures.dust,
            });
        }

        // The mark: the oldest disappears when there are too many.
        effects.decals.push_back(Decal{
            .position = point + normal * MarkLift,
            .normal = normal,
            .rotation = random.GetFloat(0.0f, 2.0f * std::numbers::pi_v<float>),
        });
        const auto maximum = static_cast<std::size_t>(std::max(settings.maximumMarkCount, 0));
        while (effects.decals.size() > maximum)
            effects.decals.erase(effects.decals.begin());
    }

    void SpawnBloodImpact(Effects& effects, const glm::vec3& point, const glm::vec3& direction)
    {
        const EffectSettings& settings = effects.settings;
        Core::Random& random = effects.random;

        // The drops fly on mostly the way the pellet went (out of the other side), some back towards the shooter.
        for (int drop = 0; drop < settings.bloodCount; ++drop)
        {
            const glm::vec3 axis = random.GetFloat(0.0f, 1.0f) < 0.7f ? direction : -direction;
            effects.particles.Emit(Particle{
                .position = point,
                .velocity = RandomDirectionAround(random, axis, 0.4f) * settings.bloodSpeed * random.GetFloat(0.5f, 1.2f),
                .lifetime = settings.bloodLifetime * random.GetFloat(0.7f, 1.3f),
                .startHalfSize = settings.bloodHalfSize,
                .endHalfSize = settings.bloodHalfSize * 0.7f,
                .rotation = random.GetFloat(0.0f, 2.0f * std::numbers::pi_v<float>),
                .startColor = {1.0f, 1.0f, 1.0f, 1.0f},
                .endColor = {1.0f, 1.0f, 1.0f, 0.0f},
                .gravityScale = 1.0f,
                .drag = 0.5f,
                .texture = effects.textures.blood,
            });
        }
    }

    void SpawnMuzzleSmoke(Effects& effects, const glm::vec3& muzzle, const glm::vec3& forward)
    {
        const EffectSettings& settings = effects.settings;
        Core::Random& random = effects.random;

        for (int puff = 0; puff < settings.smokeCount; ++puff)
        {
            effects.particles.Emit(Particle{
                .position = muzzle,
                .velocity = RandomDirectionAround(random, forward, 0.7f) * random.GetFloat(0.5f, 1.5f),
                .lifetime = settings.smokeLifetime * random.GetFloat(0.7f, 1.3f),
                .startHalfSize = settings.smokeHalfSize * 0.3f,
                .endHalfSize = settings.smokeHalfSize,
                .rotation = random.GetFloat(0.0f, 2.0f * std::numbers::pi_v<float>),
                .rotationSpeed = random.GetFloat(-0.8f, 0.8f),
                .startColor = {1.0f, 1.0f, 1.0f, 0.5f},
                .endColor = {1.0f, 1.0f, 1.0f, 0.0f},
                .gravityScale = -0.03f,
                .drag = 2.5f,
                .texture = effects.textures.smoke,
            });
        }
    }

    void AddDecalSprites(const Effects& effects, Renderer::SpriteBatch& batch)
    {
        for (const Decal& decal : effects.decals)
        {
            // Two directions lying on the wall, perpendicular to its normal and to each other, turned by the rotation of
            // the mark. Any direction not parallel to the normal gives the first one; +Y, unless the wall is a floor or
            // a ceiling, then +X.
            const bool isFloorOrCeiling = std::abs(decal.normal.y) >= 0.9f;
            const glm::vec3 helper = isFloorOrCeiling ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
            const glm::vec3 tangent = glm::normalize(glm::cross(helper, decal.normal));
            const glm::vec3 bitangent = glm::cross(decal.normal, tangent);
            const float cosine = std::cos(decal.rotation);
            const float sine = std::sin(decal.rotation);
            const float halfSize = effects.settings.markHalfSize;

            batch.AddQuad(decal.position, (tangent * cosine + bitangent * sine) * halfSize,
                          (bitangent * cosine - tangent * sine) * halfSize, glm::vec4(1.0f), effects.textures.pelletMark,
                          Renderer::SpriteBlend::Alpha);
        }
    }

    void ClearEffects(Effects& effects)
    {
        effects.particles.Clear();
        effects.decals.clear();
    }
}
