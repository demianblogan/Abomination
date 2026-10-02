#include "Gameplay/Weapons/WeaponViewModel.h"

#include <glm/vec4.hpp>
#include <gtest/gtest.h>

namespace Abomination::Gameplay
{
    namespace
    {
        // Where the middle of the model (its origin) ends up relative to the eyes.
        glm::vec3 GetModelPosition(const WeaponViewModel& weaponViewModel)
        {
            return glm::vec3(CalculateWeaponViewModelMatrix(weaponViewModel) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        }
    }

    TEST(WeaponViewModel, RightSideUsesOffsetAsItIs)
    {
        const WeaponViewModel weaponViewModel{.offset = {0.2f, -0.1f, -0.5f}, .side = WeaponViewModelSide::Right};

        EXPECT_EQ(GetModelPosition(weaponViewModel), glm::vec3(0.2f, -0.1f, -0.5f));
    }

    TEST(WeaponViewModel, LeftSideMirrorsOnlyThePosition)
    {
        const WeaponViewModel weaponViewModel{.offset = {0.2f, -0.1f, -0.5f}, .side = WeaponViewModelSide::Left};

        EXPECT_EQ(GetModelPosition(weaponViewModel), glm::vec3(-0.2f, -0.1f, -0.5f));

        // The model is moved, not mirrored: its right side stays its right side.
        const glm::vec3 rightOfModel(CalculateWeaponViewModelMatrix(weaponViewModel) * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        EXPECT_GT(rightOfModel.x, GetModelPosition(weaponViewModel).x);
    }

    TEST(WeaponViewModel, CenterPutsWeaponUnderTheEyes)
    {
        const WeaponViewModel weaponViewModel{.offset = {0.2f, -0.1f, -0.5f}, .side = WeaponViewModelSide::Center};

        EXPECT_EQ(GetModelPosition(weaponViewModel), glm::vec3(0.0f, -0.1f, -0.5f));
    }
}
