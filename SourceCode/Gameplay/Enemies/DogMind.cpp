#include "Gameplay/Enemies/DogMind.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace Abomination::Gameplay
{
    namespace
    {
        // A dog facing a target within this angle walks; farther off it first turns on the spot.
        constexpr float WalkAngle = glm::radians(12.0f);

        // The player higher than this above the feet of the dog is up on something (more than a step): the dog jumps up
        // to them, not more often than every JumpUpCooldown seconds (a jump that fell short is tried again soon).
        constexpr float ClimbHeight = 0.6f;
        constexpr float JumpUpCooldown = 1.0f;

        // The horizontal part of a direction, of length 1; zero if it has none (straight up or down, or zero).
        glm::vec3 Horizontal(glm::vec3 direction)
        {
            direction.y = 0.0f;
            const float length = glm::length(direction);
            return length > 1e-4f ? direction / length : glm::vec3(0.0f);
        }

        float HorizontalDistance(const glm::vec3& a, const glm::vec3& b)
        {
            return glm::length(glm::vec3(a.x - b.x, 0.0f, a.z - b.z));
        }

        // The push of a leap that lands where the player is (a throw), and how long it is in the air.
        //
        // Thrown up at speed v, a body rises until gravity g has taken all of v: for v / g seconds, to the height
        // v² / (2g). So rising h takes v = sqrt(2gh), and the whole flight up and down lasts t = 2v / g. To cover the
        // distance d along the ground in that time it goes d / t. For example, with g = 25 m/s² and h = 0.6 m: v =
        // sqrt(30) = 5.5 m/s up, t = 0.44 s in the air, and 3 m away 6.8 m/s forward.
        struct Leap
        {
            glm::vec3 impulse{0.0f};
            float flightTime = 0.0f;
        };

        // A leap that lands rise higher than it starts (0: on the same floor; up to jumpUpHeight: up onto the altar).
        // It rises to its apex, the higher of leapHeight and rise + jumpUpClearance, in v / g, and falls from there to
        // rise in sqrt(2 (apex - rise) / g): a fall from height d takes sqrt(2d / g).
        Leap CalculateLeap(const DogPerception& perception, const DogSettings& settings, float rise)
        {
            const float apex = std::max(settings.leapHeight, rise + settings.jumpUpClearance);
            const float upSpeed = std::sqrt(2.0f * perception.gravity * apex);
            const float flightTime =
                upSpeed / perception.gravity + std::sqrt(2.0f * (apex - rise) / perception.gravity);
            const float distance = HorizontalDistance(perception.playerPosition, perception.position);
            const float forwardSpeed = std::min(distance / flightTime, settings.leapSpeedMaximum);

            return Leap{
                .impulse = Horizontal(perception.playerPosition - perception.position) * forwardSpeed +
                           glm::vec3(0.0f, upSpeed, 0.0f),
                .flightTime = flightTime,
            };
        }

        void Enter(DogMind& mind, DogState state)
        {
            mind.state = state;
            mind.stateTime = 0.0f;
            mind.hasBitten = false;
            mind.hasLeapt = false;
        }

        void EnterIdle(DogMind& mind, const DogSettings& settings, Core::Random& random)
        {
            Enter(mind, DogState::Idle);
            mind.idleDuration = random.GetFloat(settings.idleTimeMinimum, settings.idleTimeMaximum);
            mind.isSniffing = random.GetFloat(0.0f, 1.0f) < 0.5f;
        }

        // A patrol to a random point at most patrolRadius from home (where it appeared).
        void EnterPatrol(DogMind& mind, const DogPerception& perception, const DogSettings& settings, Core::Random& random)
        {
            const float angle = random.GetFloat(0.0f, 2.0f * std::numbers::pi_v<float>);
            const float distance = random.GetFloat(0.0f, settings.patrolRadius);
            mind.patrolTarget = perception.home + glm::vec3(std::cos(angle), 0.0f, std::sin(angle)) * distance;
            mind.patrolTarget.y = perception.position.y;
            Enter(mind, DogState::Patrol);
        }
    }

    bool CanSeePlayer(const DogPerception& perception, const DogSettings& settings)
    {
        if (!perception.isPlayerAlive || !perception.hasLineOfSight)
            return false;

        const glm::vec3 toPlayer = perception.playerPosition - perception.position;
        if (glm::length(toPlayer) > settings.sightRange)
            return false;

        // Within half the field of view to either side of where it faces (the dot product of two directions of length
        // 1 is the cosine of the angle between them).
        return glm::dot(Horizontal(toPlayer), Horizontal(perception.forward)) >= std::cos(settings.fieldOfView * 0.5f);
    }

    bool NoticesPlayer(const DogPerception& perception, const DogSettings& settings)
    {
        if (!perception.isPlayerAlive)
            return false;

        const bool smellsPlayer = glm::length(perception.playerPosition - perception.position) <= settings.senseRadius;
        const bool hearsShot = perception.hasShotBeenFired &&
                               glm::length(perception.shotPosition - perception.position) <= settings.hearingRange;
        return CanSeePlayer(perception, settings) || smellsPlayer || hearsShot || perception.wasHurt;
    }

    DogDecision UpdateDogMind(DogMind& mind, const DogPerception& perception, const DogSettings& settings, float deltaTime,
                              Core::Random& random)
    {
        mind.stateTime += deltaTime;
        mind.timeSinceLeap += deltaTime;

        const bool noticesPlayer = NoticesPlayer(perception, settings);
        const glm::vec3 toPlayer = Horizontal(perception.playerPosition - perception.position);
        const float playerDistance = HorizontalDistance(perception.playerPosition, perception.position);

        // Teeth reach the player only on about the same level: a player on a balcony right above the dog is close along
        // the floor but out of reach.
        const bool isOnSameLevel =
            std::abs(perception.playerPosition.y - perception.position.y) <= settings.attackHeightDifference;

        // A hit makes it flinch, whatever it does, except in the middle of a leap or a bite.
        if (perception.wasHurt && perception.isPlayerAlive && mind.state != DogState::Leap && mind.state != DogState::Bite)
            Enter(mind, DogState::Pain);

        // First the transitions, then what it does in the state it is in.
        switch (mind.state)
        {
        case DogState::Idle:
            if (noticesPlayer)
                Enter(mind, DogState::Alert);
            else if (mind.stateTime >= mind.idleDuration)
                EnterPatrol(mind, perception, settings, random);
            break;

        case DogState::Patrol:
            if (noticesPlayer)
                Enter(mind, DogState::Alert);
            else if (perception.isBlocked || mind.stateTime >= settings.patrolTime ||
                     HorizontalDistance(mind.patrolTarget, perception.position) < 0.4f)
                EnterIdle(mind, settings, random);
            break;

        case DogState::Alert:
            if (mind.stateTime >= settings.alertTime)
                Enter(mind, DogState::Chase);
            break;

        case DogState::Chase:
            if (!perception.isPlayerAlive || playerDistance > settings.loseDistance)
                EnterIdle(mind, settings, random);
            else if (isOnSameLevel && playerDistance <= settings.biteRange)
                Enter(mind, DogState::Bite);
            // The player is up on something it cannot walk onto, but not too high (the altar): it jumps up to them.
            else if (perception.playerFeetAbove > ClimbHeight && perception.playerFeetAbove <= settings.jumpUpHeight &&
                     playerDistance <= settings.leapRangeMaximum && mind.timeSinceLeap >= JumpUpCooldown)
            {
                Enter(mind, DogState::Leap);
                mind.timeSinceLeap = 0.0f;
                mind.leapRise = perception.playerFeetAbove;
                mind.leapEndTime = settings.leapTakeoffTime + settings.leapRecoveryTime;
            }
            else if (isOnSameLevel && playerDistance >= settings.leapRangeMinimum &&
                     playerDistance <= settings.leapRangeMaximum &&
                     mind.timeSinceLeap >= settings.leapCooldown && CanSeePlayer(perception, settings))
            {
                Enter(mind, DogState::Leap);
                mind.timeSinceLeap = 0.0f;
                mind.leapRise = 0.0f;
                mind.leapEndTime = settings.leapTakeoffTime + settings.leapRecoveryTime;
            }
            break;

        case DogState::Leap:
            if (mind.stateTime >= mind.leapEndTime)
                Enter(mind, DogState::Chase);
            break;

        case DogState::Bite:
            if (mind.stateTime >= settings.biteDuration)
                Enter(mind, DogState::Chase);
            break;

        case DogState::Pain:
            if (mind.stateTime >= settings.painTime)
                Enter(mind, DogState::Chase);
            break;
        }

        DogDecision decision;
        switch (mind.state)
        {
        case DogState::Idle:
            decision.animation = mind.isSniffing ? "Idle_2_HeadLow" : "Idle";
            break;

        case DogState::Patrol:
        {
            // It turns on the spot until it faces the way, then walks: straight to the target, or to the next corner of
            // the path around what is in the way.
            const glm::vec3 toTarget = Horizontal(perception.wayPoint.value_or(mind.patrolTarget) - perception.position);
            decision.faceDirection = toTarget;
            decision.turnSpeed = settings.turnSpeed;
            const bool facesTarget = glm::dot(toTarget, Horizontal(perception.forward)) >= std::cos(WalkAngle);
            decision.speed = facesTarget ? settings.walkSpeed : 0.0f;
            decision.animation = facesTarget ? "Walk" : "Idle";
            decision.animationSpeed = facesTarget ? settings.walkAnimationSpeed : 1.0f;
            break;
        }

        case DogState::Alert:
            decision.faceDirection = toPlayer;
            decision.turnSpeed = settings.runTurnSpeed;
            decision.animation = "Idle";
            break;

        case DogState::Chase:
            decision.turnSpeed = settings.runTurnSpeed;
            if (perception.cannotGetCloser)
            {
                // As close as it can get: it stands and faces the player (barking, see Monsters.cpp), waiting for them to
                // come down.
                decision.faceDirection = toPlayer;
                decision.animation = "Idle";
                break;
            }
            // At the player, or at the next corner of the path around what is in the way.
            decision.faceDirection = Horizontal(perception.wayPoint.value_or(perception.playerPosition) - perception.position);
            decision.speed = settings.runSpeed;
            decision.animation = "Gallop";
            decision.animationSpeed = settings.runAnimationSpeed;
            break;

        case DogState::Leap:
        case DogState::Bite:
        {
            const bool isLeap = mind.state == DogState::Leap;
            decision.faceDirection = toPlayer;
            decision.turnSpeed = settings.runTurnSpeed;
            decision.animation = isLeap ? "Gallop_Jump" : "Attack";
            const bool isInReach = perception.isPlayerAlive && isOnSameLevel && playerDistance <= settings.biteReach;
            if (isLeap)
            {
                // It crouches, then pushes off at the moment its clip does, towards where the player is now.
                if (!mind.hasLeapt && mind.stateTime >= settings.leapTakeoffTime)
                {
                    mind.hasLeapt = true;
                    const Leap leap = CalculateLeap(perception, settings, mind.leapRise);
                    decision.impulse = leap.impulse;

                    // It is in the air until it lands (see CalculateLeap), then recovers.
                    mind.leapEndTime = mind.stateTime + leap.flightTime + settings.leapRecoveryTime;
                }

                // In the air it bites the moment it reaches the player: a leap that lands on them hurts at once, one
                // that falls short does not hurt at all. Not while it still crouches.
                if (mind.hasLeapt && !mind.hasBitten && isInReach)
                {
                    mind.hasBitten = true;
                    decision.bites = true;
                }
            }
            else
            {
                // Until the teeth close it keeps running in at a player who stepped back.
                if (!mind.hasBitten && playerDistance > settings.biteRange * 0.8f)
                {
                    decision.speed = settings.runSpeed;
                }
                if (!mind.hasBitten && mind.stateTime >= settings.biteTime)
                {
                    mind.hasBitten = true;
                    decision.bites = isInReach;
                }
            }
            break;
        }

        case DogState::Pain:
            decision.animation = "Idle_HitReact1";
            break;
        }

        return decision;
    }
}
