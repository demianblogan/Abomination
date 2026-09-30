#pragma once

#include "Core/Math/Random.h"
#include "Gameplay/Particles.h"
#include "Renderer/Assets/TextureStore.h"
#include "Renderer/Sprites/SpriteBatch.h"

#include <glm/vec3.hpp>

#include <cstddef>
#include <vector>

// The visual effects of shots: the flash and smoke at the muzzle, sparks and dust where a pellet hits a wall, blood where
// it hits a character, and the marks pellets leave on walls. Only for the eyes: nothing in the game depends on them.
namespace Abomination::Gameplay
{
    // The textures of the effects (made by Tools/TextureGenerator), loaded once for the whole game.
    struct EffectTextures
    {
        Renderer::TextureHandle muzzleFlash;
        Renderer::TextureHandle spark;
        Renderer::TextureHandle smoke;
        Renderer::TextureHandle dust;
        Renderer::TextureHandle pelletMark;
        Renderer::TextureHandle blood;
    };

    [[nodiscard]] EffectTextures LoadEffectTextures(Renderer::TextureStore& textures);

    // Values a designer tunes (in the Effects window of the debug overlay). Sizes are half sizes in meters, speeds in
    // meters per second, times in seconds.
    struct EffectSettings
    {
        // Sparks at a wall: bright, fast, short, falling.
        int sparkCount = 3;
        float sparkSpeed = 4.0f;
        float sparkLifetime = 0.25f;
        float sparkHalfSize = 0.025f;

        // Dust at a wall: slow puffs that grow and fade.
        int dustCount = 2;
        float dustSpeed = 0.7f;
        float dustLifetime = 0.7f;
        float dustHalfSize = 0.12f;

        // Blood at a character: drops that fly off and fall.
        int bloodCount = 6;
        float bloodSpeed = 2.5f;
        float bloodLifetime = 0.5f;
        float bloodHalfSize = 0.035f;

        // The flash at the muzzle, drawn with the weapon in the hands (half size in the space of the eyes), and the smoke
        // that drifts from the muzzle into the world.
        float flashDuration = 0.05f;
        float flashHalfSize = 0.09f;
        int smokeCount = 3;
        float smokeLifetime = 0.9f;
        float smokeHalfSize = 0.12f;

        // The marks of pellets on walls: their half size, and how many stay before the oldest disappear.
        float markHalfSize = 0.035f;
        int maximumMarkCount = 64;
    };

    // A mark on a wall (a decal): a small flat quad lying on the surface where a pellet hit.
    struct Decal
    {
        glm::vec3 position{0.0f};
        glm::vec3 normal{0.0f, 1.0f, 0.0f};
        float rotation = 0.0f;
    };

    // Everything the effects need between frames.
    struct Effects
    {
        EffectTextures textures;
        EffectSettings settings;
        ParticleSystem particles;

        // The marks, oldest first; beyond maximumMarkCount the oldest one is removed.
        std::vector<Decal> decals;

        Core::Random random;
    };

    // A pellet hit a wall at point, whose surface faces normal (a unit vector): sparks, dust and a mark.
    void SpawnWallImpact(Effects& effects, const glm::vec3& point, const glm::vec3& normal);

    // A pellet flying along direction (a unit vector) hit a character at point: drops of blood.
    void SpawnBloodImpact(Effects& effects, const glm::vec3& point, const glm::vec3& direction);

    // A shot: smoke drifting from the muzzle (in the world) forward along forward.
    void SpawnMuzzleSmoke(Effects& effects, const glm::vec3& muzzle, const glm::vec3& forward);

    // Adds the marks on walls as flat quads.
    void AddDecalSprites(const Effects& effects, Renderer::SpriteBatch& batch);

    // Removes every particle and mark (when the level is replaced).
    void ClearEffects(Effects& effects);
}
