#include "Gameplay/Characters/CharacterCollision.h"

#include "Core/Scene/Transform.h"
#include "Gameplay/Enemies/Corpses.h"
#include "Physics/CharacterBody.h"

#include <gtest/gtest.h>

#include <vector>

namespace Abomination::Gameplay
{
    namespace
    {
        // A character: a box of 1 x 2 x 1 m around position.
        entt::entity CreateCharacter(entt::registry& registry, const glm::vec3& position)
        {
            const entt::entity entity = registry.create();
            registry.emplace<Core::Transform>(entity, Core::Transform{.position = position});
            registry.emplace<Physics::CharacterBody>(entity, Physics::CharacterBody{.halfExtents = {0.5, 1.0, 0.5}});
            return entity;
        }
    }

    TEST(CharacterCollision, BoxesOfOthersWithoutTheMoverAndBodies)
    {
        entt::registry registry;
        const entt::entity mover = CreateCharacter(registry, {0.0f, 0.0f, 0.0f});
        CreateCharacter(registry, {5.0f, 1.0f, 0.0f});
        const entt::entity body = CreateCharacter(registry, {9.0f, 0.0f, 0.0f});
        registry.emplace<Corpse>(body);
        std::vector<World::CollisionBrush> memory;

        const std::span<const World::CollisionBrush> boxes = GatherCharacterBoxes(registry, mover, memory);

        ASSERT_EQ(boxes.size(), 1u);
        EXPECT_EQ(boxes[0].planes.size(), 6u);
        EXPECT_DOUBLE_EQ(boxes[0].bounds.minimum.x, 4.5);
        EXPECT_DOUBLE_EQ(boxes[0].bounds.maximum.y, 2.0);
    }

    TEST(CharacterCollision, MemoryIsReusedAndNeverShrinks)
    {
        entt::registry registry;
        const entt::entity mover = CreateCharacter(registry, {0.0f, 0.0f, 0.0f});
        CreateCharacter(registry, {5.0f, 0.0f, 0.0f});
        const entt::entity other = CreateCharacter(registry, {-5.0f, 0.0f, 0.0f});
        std::vector<World::CollisionBrush> memory;
        static_cast<void>(GatherCharacterBoxes(registry, mover, memory));
        const World::CollisionBrush* boxesBefore = memory.data();
        const Core::Plane* planesBefore = memory[0].planes.data();

        // One character fewer: the boxes are made in the same memory, the planes where they were.
        registry.destroy(other);
        const std::span<const World::CollisionBrush> boxes = GatherCharacterBoxes(registry, mover, memory);

        EXPECT_EQ(boxes.size(), 1u);
        EXPECT_EQ(memory.size(), 2u);
        EXPECT_EQ(memory.data(), boxesBefore);
        EXPECT_EQ(memory[0].planes.data(), planesBefore);
    }
}
