#pragma once

#include <entt/entt.hpp>
#include <glm/trigonometric.hpp>

namespace Abomination::Audio
{
    class AudioEngine;
}

namespace Abomination::World
{
    struct PlayerStart;
}

// The death of the player. The weapon goes down out of sight and the HUD fades out; the head drops a little, then
// jerks back and the player falls on their back, looking up; the edges of the view darken as the eyes grow heavy, then
// the eyes close. On the black, GAME OVER appears with its sound, and a moment later PRESS ANY KEY TO RESTART; a key
// restarts the level. Dead, the player neither moves, looks nor shoots.
namespace Abomination::Gameplay
{
    struct GameplayState;

    // Values a designer tunes. Times in seconds since the death, angles in radians, distances in meters.
    struct PlayerDeathSettings
    {
        // The weapon in the hands goes down out of the view in weaponLowerTime, weaponLowerDistance down; the HUD
        // fades out in hudFadeTime.
        float weaponLowerTime = 0.4f;
        float weaponLowerDistance = 0.5f;
        float hudFadeTime = 0.5f;

        // The head drops by nodPitch in nodTime; then until fallEndTime it jerks back to endPitch (looking up) while
        // the body falls on its back: the eyes go down to eyeHeight above the floor, fallBack backwards, and the view
        // tilts by roll.
        float nodTime = 0.35f;
        float nodPitch = glm::radians(15.0f);
        float fallEndTime = 1.2f;
        float endPitch = glm::radians(80.0f);
        float eyeHeight = 0.25f;
        float fallBack = 0.3f;
        float roll = glm::radians(8.0f);

        // The edges of the view darken (a black vignette, at most vignetteOpacity) from vignetteStart to fallEndTime;
        // the eyes close from eyesCloseStart in eyesCloseTime. Then GAME OVER, and hintDelay later the hint and the
        // restart.
        float vignetteStart = 0.3f;
        float vignetteOpacity = 0.7f;
        float eyesCloseStart = 1.4f;
        float eyesCloseTime = 0.5f;
        float hintDelay = 0.5f;
    };

    // The player is dead, and for how long (seconds).
    struct PlayerDeath
    {
        PlayerDeathSettings settings;
        bool isDead = false;
        float time = 0.0f;

        // GAME OVER has been shown (its sound is played once, when it appears).
        bool hasShownGameOver = false;
    };

    // How the view of the dead player moves away from where it would be: the eyes lower and further back, the pitch
    // instead of the one of the look, and a tilt to the side.
    struct DeathView
    {
        float drop = 0.0f;
        float back = 0.0f;
        float pitch = 0.0f;
        float roll = 0.0f;
    };

    [[nodiscard]] bool IsPlayerDead(const GameplayState& state) noexcept;

    // Once per frame: notices the death (no health left), counts the time since it, and plays the sound of GAME OVER
    // when it appears.
    void UpdatePlayerDeath(GameplayState& state, const entt::registry& registry, Audio::AudioEngine& audio, float deltaTime);

    // The view at the moment of death. eyesAboveFloor is how high the eyes of the living player are above the floor,
    // lookPitch where they looked when they died.
    [[nodiscard]] DeathView CalculateDeathView(const PlayerDeath& death, float eyesAboveFloor, float lookPitch);

    // How far the weapon has gone down (0 not at all, 1 out of sight), and how visible the HUD still is (1 fully).
    [[nodiscard]] float CalculateWeaponLowering(const PlayerDeath& death);
    [[nodiscard]] float CalculateHUDOpacity(const PlayerDeath& death);

    // How dark the edges of the view are (0 to vignetteOpacity), and how closed the eyes are (0 open, 1 closed).
    [[nodiscard]] float CalculateDeathVignette(const PlayerDeath& death);
    [[nodiscard]] float CalculateEyesClosed(const PlayerDeath& death);

    // GAME OVER is on the screen (the eyes are closed); the hint is shown too, and a key restarts the level.
    [[nodiscard]] bool IsGameOverShown(const PlayerDeath& death);
    [[nodiscard]] bool CanRestartAfterDeath(const PlayerDeath& death);

    // The player starts again where the map puts them, as at the start of the game: full health, no armor, the shells of
    // the start, looking along the start, alive.
    void RespawnPlayer(GameplayState& state, entt::registry& registry, const World::PlayerStart& playerStart);
}
