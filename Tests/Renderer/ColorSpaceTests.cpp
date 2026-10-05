#include "Renderer/ColorSpace.h"

#include <gtest/gtest.h>

namespace Abomination::Renderer
{
    TEST(ColorSpace, BlackAndWhiteStayTheSame)
    {
        EXPECT_FLOAT_EQ(ConvertSRGBToLinear(0.0f), 0.0f);
        EXPECT_FLOAT_EQ(ConvertSRGBToLinear(1.0f), 1.0f);
        EXPECT_FLOAT_EQ(ConvertLinearToSRGB(0.0f), 0.0f);
        EXPECT_NEAR(ConvertLinearToSRGB(1.0f), 1.0f, 1e-6f);
    }

    TEST(ColorSpace, HalfInSRGBIsAboutAFifthOfTheLight)
    {
        // 128 of 255 in an image file: 21.6% of the light of white, not 50%.
        EXPECT_NEAR(ConvertSRGBToLinear(128.0f / 255.0f), 0.2159f, 1e-4f);
    }

    TEST(ColorSpace, TwoHalvesOfLightAddUpInLinearValues)
    {
        // Two lamps that each make a wall look 0.5 (sRGB) make it look about 0.69 together, not 1.0.
        const float twoLamps = 2.0f * ConvertSRGBToLinear(0.5f);

        EXPECT_NEAR(ConvertLinearToSRGB(twoLamps), 0.686f, 1e-3f);
    }

    TEST(ColorSpace, ConversionsUndoEachOther)
    {
        for (float value = 0.0f; value <= 1.0f; value += 0.05f)
            EXPECT_NEAR(ConvertLinearToSRGB(ConvertSRGBToLinear(value)), value, 1e-5f);
    }

    TEST(ColorSpace, AlphaIsNotConverted)
    {
        const glm::vec4 linear = ConvertSRGBToLinear(glm::vec4(0.5f, 0.5f, 0.5f, 0.5f));

        EXPECT_NEAR(linear.r, 0.2140f, 1e-4f);
        EXPECT_FLOAT_EQ(linear.a, 0.5f);
    }
}
