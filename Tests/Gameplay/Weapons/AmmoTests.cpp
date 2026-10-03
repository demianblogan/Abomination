#include "Gameplay/Weapons/Ammo.h"

#include <gtest/gtest.h>

namespace Abomination::Gameplay
{
    TEST(Ammo, ShotTakesAmmunitionWhileThereIsEnough)
    {
        Ammo ammo;
        AddAmmo(ammo, AmmoType::Shells, 2);

        EXPECT_TRUE(TryUseAmmo(ammo, AmmoType::Shells, 1));
        EXPECT_TRUE(TryUseAmmo(ammo, AmmoType::Shells, 1));
        EXPECT_FALSE(TryUseAmmo(ammo, AmmoType::Shells, 1));
        EXPECT_EQ(GetAmmo(ammo, AmmoType::Shells), 0);
    }

    TEST(Ammo, NotEnoughTakesNothing)
    {
        Ammo ammo;
        AddAmmo(ammo, AmmoType::Cells, 3);

        EXPECT_FALSE(TryUseAmmo(ammo, AmmoType::Cells, 5));
        EXPECT_EQ(GetAmmo(ammo, AmmoType::Cells), 3);
    }

    TEST(Ammo, NeverAboveTheMaximum)
    {
        Ammo ammo;
        AddAmmo(ammo, AmmoType::Shells, 1000);
        EXPECT_EQ(GetAmmo(ammo, AmmoType::Shells), ammo.maximums[0]);

        // Other kinds are not touched.
        EXPECT_EQ(GetAmmo(ammo, AmmoType::Bullets), 0);
    }
}
