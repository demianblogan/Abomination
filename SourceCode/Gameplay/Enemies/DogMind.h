#pragma once

#include "Core/Math/Random.h"

#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <optional>
#include <string_view>

// What the dog decides, tick by tick, from what it perceives: a small state machine, like the monsters of Quake. It
// knows nothing of the level or the registry (see Monsters.cpp, which perceives for it and carries out its decisions),
// so it can be tested on its own.
namespace Abomination::Gameplay
{
    enum class DogState
    {
        Idle,     // stands, looks around and sniffs the ground
        Patrol,   // turns towards a point near where it appeared, then walks there with its head down
        Alert,    // has just noticed the player: turns to them for a moment
        Chase,    // runs at the player, wherever they go, until they are far enough
        Leap,     // jumps at the player from a few meters and bites in the air
        Bite,     // bites the player standing next to it
        Pain,     // flinches after being hit
    };

    // Names for the debug overlay, in the order of the enum values.
    inline constexpr std::array<std::string_view, 7> DogStateNames = {"Idle", "Patrol", "Alert", "Chase",
                                                                      "Leap", "Bite",   "Pain"};

    // Values a designer tunes (in the Enemies window, Dog tab). Distances in meters, speeds in meters per second, times in
    // seconds, angles in radians.
    struct DogSettings
    {
        // Sight: how far it sees, and how wide (the whole angle of its field of view; a wall hides the player).
        float sightRange = 18.0f;
        float fieldOfView = glm::radians(140.0f);

        // Its nose and ears: the player closer than this is noticed whatever way it faces, even behind a wall.
        float senseRadius = 4.0f;

        // How far a shot is heard.
        float hearingRange = 25.0f;

        // Once it chases the player, it follows them everywhere until they are farther than this.
        float loseDistance = 30.0f;

        // Walking (patrol) and running (chase), and how fast their clips play (1: as made).
        float walkSpeed = 1.3f;
        float runSpeed = 7.0f;
        float walkAnimationSpeed = 1.0f;
        float runAnimationSpeed = 1.4f;

        // How fast it turns: on the spot before a walk, and while running.
        float turnSpeed = glm::radians(700.0f);
        float runTurnSpeed = glm::radians(720.0f);

        // Idle for a random time between these, then it walks to a point at most patrolRadius from where it appeared,
        // giving up after patrolTime if it cannot get there.
        float idleTimeMinimum = 2.0f;
        float idleTimeMaximum = 5.0f;
        float patrolRadius = 6.0f;
        float patrolTime = 8.0f;

        float alertTime = 0.5f;

        // While it chases the player it barks again and again, each time after a random pause between these.
        float barkIntervalMinimum = 0.8f;
        float barkIntervalMaximum = 2.0f;

        // The bite: closer than biteRange it bites, biteTime after the start (still running in while the player is
        // farther than most of biteRange), hurting the player if they are within biteReach then; the bite is over after
        // biteDuration. A short biteTime: a long one let the player step out of every bite.
        float biteRange = 1.3f;
        float biteReach = 1.7f;
        float biteTime = 0.1f;
        float biteDuration = 1.0f;

        // How much higher or lower the middle of the player may be than the middle of the dog for a bite or a leap
        // (meters). On one floor it is half a meter (the player is taller), a player on the back of the dog is 1.3 higher
        // (before they slide off); a balcony is 4 m.
        float attackHeightDifference = 1.5f;

        // The leap: from leapRangeMinimum to leapRangeMaximum away it crouches, and leapTakeoffTime after the start
        // (the moment its clip pushes off) jumps to where the player is then, rising leapHeight on the way, no faster
        // than leapSpeedMaximum along the ground. In the air it bites as soon as the player is within biteReach. After
        // landing it stands for leapRecoveryTime; it does not leap again for leapCooldown.
        float leapRangeMinimum = 2.2f;
        float leapRangeMaximum = 4.0f;
        float leapTakeoffTime = 0.25f;
        float leapHeight = 0.6f;
        float leapSpeedMaximum = 12.0f;
        float leapRecoveryTime = 0.25f;
        float leapCooldown = 2.5f;

        float damage = 10.0f;
        float painTime = 0.4f;

        // Its body on stairs and ledges (see Monsters.cpp and GroundFit.h). On a patrol it does not step where a corner
        // of its box would have no ground under it within two steps (a chasing or leaping dog jumps down after the player).
        bool avoidsLedges = true;

        // Its model tilted and lowered to the ground under its paws, pawDistance in front of and behind its middle; tilted
        // at most maximumTilt; gliding to a new fit at heightFollowSpeed and tiltFollowSpeed.
        bool fitsToGround = true;
        float pawDistance = 0.48f;
        float maximumTilt = glm::radians(30.0f);
        float heightFollowSpeed = 2.5f;
        float tiltFollowSpeed = glm::radians(180.0f);
    };

    // What the dog perceives this tick (gathered by Monsters.cpp).
    struct DogPerception
    {
        glm::vec3 position{0.0f};

        // Where it faces (horizontal, length 1).
        glm::vec3 forward{0.0f, 0.0f, -1.0f};

        // Where it appeared: it patrols around it.
        glm::vec3 home{0.0f};

        glm::vec3 playerPosition{0.0f};
        bool isPlayerAlive = true;

        // Nothing stands between its eyes and the player's (a trace through the level).
        bool hasLineOfSight = false;

        // A shot was fired this tick, and where.
        bool hasShotBeenFired = false;
        glm::vec3 shotPosition{0.0f};

        // It lost health this tick.
        bool wasHurt = false;

        // It tried to walk or run in the last tick but hardly moved: something is in its way.
        bool isBlocked = false;

        // How fast things fall (m/s², Physics::PhysicsSettings::gravity): a leap is aimed with it.
        float gravity = 25.0f;

        // Where to run to on the way to the player (chase) or to the patrol target (patrol): the next corner of the
        // path around walls (found on the navmesh by Monsters.cpp). None: straight at the player or the target.
        std::optional<glm::vec3> wayPoint;
    };

    // The state of the mind between ticks.
    struct DogMind
    {
        DogState state = DogState::Idle;
        float stateTime = 0.0f;

        // How long this idle lasts, looking around or sniffing; where the patrol goes.
        float idleDuration = 3.0f;
        bool isSniffing = false;
        glm::vec3 patrolTarget{0.0f};

        // The bite of this attack has been done (it bites once per attack); how long ago it last leapt.
        bool hasBitten = false;
        float timeSinceLeap = 1000.0f;

        // The leap has pushed off, and when it ends (its time in the state: takeoff, flight, recovery).
        bool hasLeapt = false;
        float leapEndTime = 0.0f;
    };

    // What the dog does this tick, decided by UpdateDogMind.
    struct DogDecision
    {
        // Where to face (horizontal; zero keeps the facing) and how fast to turn there.
        glm::vec3 faceDirection{0.0f};
        float turnSpeed = 0.0f;

        // How fast to move: always forward, where it faces, so it never walks sideways. A push to give at once (the
        // leap).
        float speed = 0.0f;
        glm::vec3 impulse{0.0f};

        // The animation segment to play ("Idle", "Walk", ...) and how fast.
        std::string_view animation = "Idle";
        float animationSpeed = 1.0f;

        // The bite happens now: the player is hurt by settings.damage.
        bool bites = false;
    };

    // Whether the player can be seen: within the range, within the field of view, and with a line of sight.
    [[nodiscard]] bool CanSeePlayer(const DogPerception& perception, const DogSettings& settings);

    // Whether the dog notices the player now: it sees them, smells them close by, or hears a shot.
    [[nodiscard]] bool NoticesPlayer(const DogPerception& perception, const DogSettings& settings);

    // Moves the mind on by deltaTime and decides what the dog does now.
    [[nodiscard]] DogDecision UpdateDogMind(DogMind& mind, const DogPerception& perception, const DogSettings& settings,
                                            float deltaTime, Core::Random& random);
}
