#include "Gameplay/ViewModel.h"

#include "Physics/CharacterBody.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

namespace Abomination::Gameplay
{
    glm::mat4 CalculateViewModelMatrix(const ViewModel& viewModel)
    {
        glm::vec3 position = viewModel.offset;
        switch (viewModel.side)
        {
        case ViewModelSide::Right:
            break;
        case ViewModelSide::Center:
            position.x = 0.0f;
            break;
        case ViewModelSide::Left:
            position.x = -position.x;
            break;
        }

        position += CalculateViewModelMotionOffset(viewModel.motion, viewModel.motionSettings);

        return glm::translate(glm::mat4(1.0f), position);
    }

    void UpdateViewModel(const GameplayState& state, entt::registry& registry, float deltaTime)
    {
        ViewModel* viewModel = registry.try_get<ViewModel>(state.player);
        if (viewModel == nullptr)
            return;

        const Physics::CharacterBody& body = registry.get<Physics::CharacterBody>(state.player);
        const ViewModelMotionInput input{
            .horizontalSpeed = glm::length(glm::vec2(body.velocity.x, body.velocity.z)),
            .maxSpeed = state.movementSettings.maxSpeed,
            .isOnGround = body.isOnGround,
            .verticalSpeed = body.velocity.y,
            .look = registry.get<LookAngles>(state.player),
        };
        UpdateViewModelMotion(viewModel->motion, viewModel->motionSettings, input, deltaTime);
    }
}
