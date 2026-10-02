#include "Gameplay/Weapons/ViewModel.h"

#include "Physics/CharacterBody.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

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

        // The recoil turns the muzzle up around the middle of the model: a positive angle around +X (the right) turns -Z
        // (forward) towards +Y (up).
        const glm::mat4 placed = glm::translate(glm::mat4(1.0f), position);
        return glm::rotate(placed, viewModel.motion.recoilPitch, glm::vec3(1.0f, 0.0f, 0.0f));
    }

    glm::vec3 CalculateViewModelMuzzle(const ViewModel& viewModel)
    {
        return glm::vec3(CalculateViewModelMatrix(viewModel) * glm::vec4(viewModel.muzzle, 1.0f));
    }

    void UpdateViewModel(const GameplayState& state, entt::registry& registry, float deltaTime)
    {
        ViewModel* viewModel = registry.try_get<ViewModel>(state.player);
        if (viewModel == nullptr)
            return;

        viewModel->flashTimeLeft -= deltaTime;

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
