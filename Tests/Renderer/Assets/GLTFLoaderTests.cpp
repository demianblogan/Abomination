#include "Renderer/Assets/GLTFLoader.h"

#include <glm/vec4.hpp>
#include <gtest/gtest.h>

#include <expected>
#include <filesystem>
#include <fstream>
#include <string>

namespace Abomination::Renderer
{
    namespace
    {
        // The binary data of one triangle, written into the JSON as base64 (a "data:" URI), so the test needs no .bin
        // file: positions (0,0,0) (1,0,0) (0,1,0), normals all (0,0,1), texture coordinates (0,0) (1,0) (0,0.25), and the
        // indices 0, 1, 2 as 16-bit numbers. Byte offsets: positions 0, normals 36, texture coordinates 72, indices 96.
        constexpr const char* TriangleBuffer =
            "AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAIA/AAAAAA"
            "AAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA+AAABAAIAAAA=";

        // A glTF with one node holding the triangle. nodeTransform is inserted into the node: "translation", "scale", ...
        std::string MakeTriangleGLTF(const std::string& nodeTransform)
        {
            return R"({
                "asset": {"version": "2.0"},
                "scene": 0,
                "scenes": [{"nodes": [0]}],
                "nodes": [{"name": "Triangle", "mesh": 0)" + nodeTransform + R"(}],
                "meshes": [{"primitives": [{
                    "attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2},
                    "indices": 3
                }]}],
                "buffers": [{"byteLength": 104, "uri": "data:application/octet-stream;base64,)" + TriangleBuffer + R"("}],
                "bufferViews": [
                    {"buffer": 0, "byteOffset": 0, "byteLength": 36},
                    {"buffer": 0, "byteOffset": 36, "byteLength": 36},
                    {"buffer": 0, "byteOffset": 72, "byteLength": 24},
                    {"buffer": 0, "byteOffset": 96, "byteLength": 6}
                ],
                "accessors": [
                    {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3", "min": [0, 0, 0], "max": [1, 1, 0]},
                    {"bufferView": 1, "componentType": 5126, "count": 3, "type": "VEC3"},
                    {"bufferView": 2, "componentType": 5126, "count": 3, "type": "VEC2"},
                    {"bufferView": 3, "componentType": 5123, "count": 3, "type": "SCALAR"}
                ]
            })";
        }
    }

    // Every test gets its own file in the temporary folder; it is removed afterwards.
    class GLTFLoaderTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            const std::string testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
            m_filePath = std::filesystem::temp_directory_path() / ("AbominationGLTFTest_" + testName + ".gltf");
        }

        void TearDown() override
        {
            std::filesystem::remove(m_filePath);
        }

        void WriteFile(const std::string& contents) const
        {
            std::ofstream(m_filePath) << contents;
        }

        std::filesystem::path m_filePath;
    };

    TEST_F(GLTFLoaderTest, ReadsVerticesAndIndicesOfTriangle)
    {
        WriteFile(MakeTriangleGLTF(""));

        const std::expected<ModelData, std::string> model = LoadGLTFFile(m_filePath);

        ASSERT_TRUE(model.has_value()) << model.error();
        ASSERT_EQ(model->parts.size(), 1u);
        const ModelPartData& part = model->parts[0];
        EXPECT_EQ(part.name, "Triangle");
        EXPECT_FALSE(part.imageIndex.has_value());

        ASSERT_EQ(part.mesh.vertices.size(), 3u);
        EXPECT_EQ(part.mesh.vertices[1].position, glm::vec3(1.0f, 0.0f, 0.0f));
        EXPECT_EQ(part.mesh.vertices[2].normal, glm::vec3(0.0f, 0.0f, 1.0f));

        // v is turned around: glTF counts it from the top of the image, our textures from the bottom.
        EXPECT_FLOAT_EQ(part.mesh.vertices[1].texCoord.x, 1.0f);
        EXPECT_FLOAT_EQ(part.mesh.vertices[0].texCoord.y, 1.0f);
        EXPECT_FLOAT_EQ(part.mesh.vertices[2].texCoord.y, 0.75f);

        EXPECT_EQ(part.mesh.indices, (std::vector<std::uint32_t>{0, 1, 2}));
    }

    TEST_F(GLTFLoaderTest, NodeTransformBecomesPartTransform)
    {
        WriteFile(MakeTriangleGLTF(R"(, "translation": [0, 0, 5])"));

        const std::expected<ModelData, std::string> model = LoadGLTFFile(m_filePath);

        ASSERT_TRUE(model.has_value()) << model.error();
        const glm::vec4 movedOrigin = model->parts[0].transform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        EXPECT_EQ(movedOrigin, glm::vec4(0.0f, 0.0f, 5.0f, 1.0f));
    }

    TEST_F(GLTFLoaderTest, MirroringTransformKeepsTrianglesFacingOut)
    {
        // A negative scale mirrors the triangle, which would turn it clockwise; two corners are swapped to undo that.
        WriteFile(MakeTriangleGLTF(R"(, "scale": [-1, 1, 1])"));

        const std::expected<ModelData, std::string> model = LoadGLTFFile(m_filePath);

        ASSERT_TRUE(model.has_value()) << model.error();
        EXPECT_EQ(model->parts[0].mesh.indices, (std::vector<std::uint32_t>{0, 2, 1}));
    }

    TEST_F(GLTFLoaderTest, NotGLTFGivesError)
    {
        WriteFile("this is not a model");

        EXPECT_FALSE(LoadGLTFFile(m_filePath).has_value());
    }

    TEST_F(GLTFLoaderTest, MissingFileGivesError)
    {
        EXPECT_FALSE(LoadGLTFFile(m_filePath).has_value());
    }
}
