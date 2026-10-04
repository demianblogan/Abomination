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

        void Enter(DogMind& mind, DogState state)
        {
            mind.state = state;
            mind.stateTime = 0.0f;
            mind.hasBitten = false;
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
            else if (playerDistance <= settings.biteRange)
                Enter(mind, DogState::Bite);
            else if (playerDistance >= settings.leapRangeMinimum && playerDistance <= settings.leapRangeMaximum &&
                     mind.timeSinceLeap >= settings.leapCooldown && CanSeePlayer(perception, settings))
            {
                Enter(mind, DogState::Leap);
                mind.timeSinceLeap = 0.0f;
                DogDecision leap;
                leap.impulse = toPlayer * settings.leapSpeed + glm::vec3(0.0f, settings.leapUpSpeed, 0.0f);
                leap.faceDirection = toPlayer;
                leap.turnSpeed = settings.runTurnSpeed;
                leap.animation = "Gallop_Jump";
                return leap;
            }
            break;

        case DogState::Leap:
            if (mind.stateTime >= settings.leapDuration)
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
            // It turns on the spot until it faces the target, then walks straight to it.
            const glm::vec3 toTarget = Horizontal(mind.patrolTarget - perception.position);
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
            decision.faceDirection = toPlayer;
            decision.turnSpeed = settings.runTurnSpeed;
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
            if (!mind.hasBitten && mind.stateTime >= (isLeap ? settings.leapBiteTime : settings.biteTime))
            {
                mind.hasBitten = true;
                decision.bites = perception.isPlayerAlive && playerDistance <= settings.biteReach;
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
