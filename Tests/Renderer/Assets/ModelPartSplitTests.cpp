#include "Renderer/Assets/ModelPartSplit.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Abomination::Renderer
{
    namespace
    {
        // A part of two triangles side by side along X: the first from x = 0 to 1, the second from x = 2 to 3. All
        // vertices face +Z and have texture coordinates (x / 4, 0.5).
        ModelData CreateTwoTriangleModel()
        {
            MeshData mesh;
            for (const float x : {0.0f, 1.0f, 0.0f, 2.0f, 3.0f, 2.0f})
            {
                const float y = mesh.vertices.size() % 3 == 2 ? 1.0f : 0.0f;
                mesh.vertices.push_back({.position = {x, y, 0.0f}, .texCoord = {x / 4.0f, 0.5f}, .normal = {0.0f, 0.0f, 1.0f}});
            }
            mesh.indices = {0, 1, 2, 3, 4, 5};

            ModelData model;
            model.parts.push_back({.name = "Body", .mesh = mesh, .imageIndex = 0});
            return model;
        }

        ModelPartSplit CreateSplitOfSecondTriangle()
        {
            return ModelPartSplit{
                .sourcePartName = "Body",
                .partName = "Bolt",
                .boxMinimum = {1.5f, -1.0f, -1.0f},
                .boxMaximum = {3.5f, 2.0f, 1.0f},
                .texCoordMinimum = {0.0f, 0.0f},
                .texCoordMaximum = {1.0f, 1.0f},
            };
        }
    }

    TEST(ModelPartSplit, MovesTrianglesInsideBoxToNewPart)
    {
        ModelData model = CreateTwoTriangleModel();

        ASSERT_TRUE(SplitModelPart(model, CreateSplitOfSecondTriangle()));

        ASSERT_EQ(model.parts.size(), 2u);
        EXPECT_EQ(model.parts[0].mesh.indices, (std::vector<std::uint32_t>{0, 1, 2}));

        const ModelPartData& bolt = model.parts[1];
        EXPECT_EQ(bolt.name, "Bolt");
        EXPECT_EQ(bolt.imageIndex, model.parts[0].imageIndex);
        ASSERT_EQ(bolt.mesh.vertices.size(), 3u);
        EXPECT_EQ(bolt.mesh.indices, (std::vector<std::uint32_t>{0, 1, 2}));
        EXPECT_FLOAT_EQ(bolt.mesh.vertices[0].position.x, 2.0f);
    }

    TEST(ModelPartSplit, TextureRectangleLeavesTrianglesOutsideIt)
    {
        // The second triangle is inside the box, but its texture coordinates (0.5 to 0.75) are outside the rectangle.
        ModelData model = CreateTwoTriangleModel();
        ModelPartSplit split = CreateSplitOfSecondTriangle();
        split.texCoordMaximum = {0.4f, 1.0f};

        EXPECT_FALSE(SplitModelPart(model, split));
        EXPECT_EQ(model.parts.size(), 1u);
        EXPECT_EQ(model.parts[0].mesh.indices.size(), 6u);
    }

    TEST(ModelPartSplit, MissingSourcePartChangesNothing)
    {
        ModelData model = CreateTwoTriangleModel();
        ModelPartSplit split = CreateSplitOfSecondTriangle();
        split.sourcePartName = "Stock";

        EXPECT_FALSE(SplitModelPart(model, split));
        EXPECT_EQ(model.parts.size(), 1u);
    }

    TEST(ModelPartSplit, BackingIsInsetCopyWithoutModelTexture)
    {
        ModelData model = CreateTwoTriangleModel();
        ModelPartSplit split = CreateSplitOfSecondTriangle();
        split.backingPartName = "Opening";
        split.backingInset = 0.01f;

        ASSERT_TRUE(SplitModelPart(model, split));

        ASSERT_EQ(model.parts.size(), 3u);
        const ModelPartData& backing = model.parts[1];
        const ModelPartData& bolt = model.parts[2];
        EXPECT_EQ(backing.name, "Opening");
        EXPECT_FALSE(backing.imageIndex.has_value());
        ASSERT_EQ(backing.mesh.vertices.size(), bolt.mesh.vertices.size());

        // Moved against the normal (+Z): 0.01 m behind the bolt.
        for (std::size_t index = 0; index < bolt.mesh.vertices.size(); ++index)
            EXPECT_FLOAT_EQ(backing.mesh.vertices[index].position.z, bolt.mesh.vertices[index].position.z - 0.01f);
    }
}
