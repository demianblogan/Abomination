#include "Renderer/Assets/MaterialFiles.h"

#include <gtest/gtest.h>

namespace Abomination::Renderer
{
    TEST(MaterialFiles, MapsLieNextToTheColorWithSuffixes)
    {
        const MaterialFilePaths paths = GetMaterialFilePaths("Textures/Episode1/Wall_MossyBrick.png");

        EXPECT_EQ(paths.baseColor, "Textures/Episode1/Wall_MossyBrick.png");
        EXPECT_EQ(paths.normal, "Textures/Episode1/Wall_MossyBrick_Normal.png");
        EXPECT_EQ(paths.metalRoughness, "Textures/Episode1/Wall_MossyBrick_MetalRough.png");
        EXPECT_EQ(paths.emissive, "Textures/Episode1/Wall_MossyBrick_Emissive.png");
    }

    TEST(MaterialFiles, DotInFolderIsNotTheExtension)
    {
        const MaterialFilePaths paths = GetMaterialFilePaths("Textures/Old.Set/Plaster");

        EXPECT_EQ(paths.normal, "Textures/Old.Set/Plaster_Normal");
    }
}
