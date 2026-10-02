#include "Gameplay/Weapons/ViewModel.h"

#include <glm/vec4.hpp>
#include <gtest/gtest.h>

namespace Abomination::Gameplay
{
    namespace
    {
        // Where the middle of the model (its origin) ends up relative to the eyes.
        glm::vec3 GetModelPosition(const ViewModel& viewModel)
        {
            return glm::vec3(CalculateViewModelMatrix(viewModel) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        }
    }

    TEST(ViewModel, RightSideUsesOffsetAsItIs)
    {
        const ViewModel viewModel{.offset = {0.2f, -0.1f, -0.5f}, .side = ViewModelSide::Right};

        EXPECT_EQ(GetModelPosition(viewModel), glm::vec3(0.2f, -0.1f, -0.5f));
    }

    TEST(ViewModel, LeftSideMirrorsOnlyThePosition)
    {
        const ViewModel viewModel{.offset = {0.2f, -0.1f, -0.5f}, .side = ViewModelSide::Left};

        EXPECT_EQ(GetModelPosition(viewModel), glm::vec3(-0.2f, -0.1f, -0.5f));

        // The model is moved, not mirrored: its right side stays its right side.
        const glm::vec3 rightOfModel(CalculateViewModelMatrix(viewModel) * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        EXPECT_GT(rightOfModel.x, GetModelPosition(viewModel).x);
    }

    TEST(ViewModel, CenterPutsWeaponUnderTheEyes)
    {
        const ViewModel viewModel{.offset = {0.2f, -0.1f, -0.5f}, .side = ViewModelSide::Center};

        EXPECT_EQ(GetModelPosition(viewModel), glm::vec3(0.0f, -0.1f, -0.5f));
    }
}
