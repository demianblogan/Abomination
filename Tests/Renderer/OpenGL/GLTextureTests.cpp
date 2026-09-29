#include "Renderer/OpenGL/GLTexture.h"

#include <gtest/gtest.h>

namespace Abomination::Renderer
{
    // CalculateVideoMemorySize() needs no OpenGL context, so it can be tested without a window.
    // Every mipmap level is half the size of the previous one, down to 1x1, and takes 4 bytes per texel.

    TEST(GLTexture, VideoMemoryOfSquareTextureIncludesAllMipmapLevels)
    {
        // 64x64: 4096 + 1024 + 256 + 64 + 16 + 4 + 1 = 5461 texels x 4 bytes.
        EXPECT_EQ(GLTexture::CalculateVideoMemorySize(64, 64), 5461u * 4u);
    }

    TEST(GLTexture, VideoMemoryOfSingleTexelTexture)
    {
        EXPECT_EQ(GLTexture::CalculateVideoMemorySize(1, 1), 4u);
    }

    TEST(GLTexture, VideoMemoryOfRectangularTextureKeepsShortSideAtLeastOne)
    {
        // 8x2: 8x2 + 4x1 + 2x1 + 1x1 (the short side stops at 1) = 16 + 4 + 2 + 1 = 23 texels x 4 bytes.
        EXPECT_EQ(GLTexture::CalculateVideoMemorySize(8, 2), 23u * 4u);
    }
}
