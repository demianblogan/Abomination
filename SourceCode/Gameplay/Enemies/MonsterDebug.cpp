#include "Gameplay/Enemies/MonsterDebug.h"

#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Gameplay/Enemies/Monsters.h"
#include "Gameplay/GameplayState.h"
#include "Navigation/NavMesh.h"
#include "Renderer/Debug/DebugLines.h"

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cmath>
#include <cstddef>
#include <numbers>

namespace Abomination::Gameplay
{
    void AddMonsterDebugLines(const GameplayState& state, const entt::registry& registry, float interpolationFactor,
                              Renderer::DebugLines& lines)
    {
        if (!state.areDogSensesVisible)
            return;

        const DogSettings& settings = state.dogSettings;
        constexpr int Segments = 48;
        constexpr float FullTurn = 2.0f * std::numbers::pi_v<float>;

        // A circle lying flat around center.
        const auto addCircle = [&](const glm::vec3& center, float radius, const glm::vec3& color)
        {
            for (int segment = 0; segment < Segments; ++segment)
            {
                const float a = FullTurn * static_cast<float>(segment) / Segments;
                const float b = FullTurn * static_cast<float>(segment + 1) / Segments;
                lines.AddLine(center + glm::vec3(std::cos(a), 0.0f, std::sin(a)) * radius,
                              center + glm::vec3(std::cos(b), 0.0f, std::sin(b)) * radius, color);
            }
        };

        for (const auto [entity, dog, transform] : registry.view<const Dog, const Core::Transform>().each())
        {
            const Core::Transform drawn = Core::CalculateDrawnTransform(registry, entity, interpolationFactor);

            // On the floor under it, a little above so the lines are not hidden in it.
            const float floorOffset = 0.02f - static_cast<float>(DogHalfExtents.y);
            const glm::vec3 floor = drawn.position + glm::vec3(0.0f, floorOffset, 0.0f);

            // The field of view: two edges from the dog to the range of its sight, and the arc between them.
            glm::vec3 forward = drawn.rotation * Core::LocalForward;
            forward.y = 0.0f;
            forward = glm::normalize(forward);
            const float facing = std::atan2(forward.z, forward.x);
            const float half = settings.fieldOfView * 0.5f;
            const glm::vec3 sightColor(1.0f, 0.9f, 0.2f);
            glm::vec3 previous{};
            for (int segment = 0; segment <= Segments; ++segment)
            {
                const float angle = facing - half + settings.fieldOfView * static_cast<float>(segment) / Segments;
                const glm::vec3 point = floor + glm::vec3(std::cos(angle), 0.0f, std::sin(angle)) * settings.sightRange;
                if (segment == 0 || segment == Segments)
                    lines.AddLine(floor, point, sightColor);
                if (segment > 0)
                    lines.AddLine(previous, point, sightColor);
                previous = point;
            }

            // Its way around walls: from where it is through the corners of its path (white).
            if (dog.path.size() >= 2)
            {
                const glm::vec3 lift(0.0f, 0.06f, 0.0f);
                const glm::vec3 pathColor(1.0f, 1.0f, 1.0f);
                lines.AddLine(floor + lift, dog.path[1] + lift, pathColor);
                for (std::size_t corner = 2; corner < dog.path.size(); ++corner)
                    lines.AddLine(dog.path[corner - 1] + lift, dog.path[corner] + lift, pathColor);
            }

            addCircle(floor, settings.senseRadius, glm::vec3(1.0f, 0.5f, 0.1f));
            addCircle(floor, settings.hearingRange, glm::vec3(0.3f, 0.5f, 1.0f));

            // The patrol area stays where the dog appeared.
            addCircle(dog.home + glm::vec3(0.0f, floorOffset, 0.0f), settings.patrolRadius, glm::vec3(0.3f, 1.0f, 0.3f));
        }
    }

    void AddNavMeshDebugLines(const GameplayState& state, const Navigation::NavMesh* navMesh, Renderer::DebugLines& lines)
    {
        if (!state.isNavMeshVisible || navMesh == nullptr)
            return;

        // A little above the floor, so the lines are not hidden in it.
        constexpr glm::vec3 Lift{0.0f, 0.04f, 0.0f};
        constexpr glm::vec3 Color{0.3f, 0.95f, 0.55f};
        for (const Navigation::NavMeshPolygon& polygon : navMesh->GetPolygons())
            for (std::size_t corner = 0; corner < polygon.size(); ++corner)
                lines.AddLine(polygon[corner] + Lift, polygon[(corner + 1) % polygon.size()] + Lift, Color);
    }
}
