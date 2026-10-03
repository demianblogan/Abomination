#include "Renderer/Assets/GLTFLoader.h"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <gtest/gtest.h>

#include <algorithm>
#include <expected>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

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

        // The triangle with a skin and an animation. After the bytes of TriangleBuffer (0-103): the joints of the vertices
        // as 4 unsigned bytes each (104), their weights as 4 floats each (116; the third vertex 0.25 and 0.25, to be
        // normalized), the inverse bind matrices of the skin (164: of "Tip", which moves the vertex down by 1, then of
        // "Root", the identity), the times of two keys (292: 0 and 2 s) and two rotations of "Tip" (300: none, then 90
        // degrees around Z).
        constexpr const char* SkinnedBuffer =
            "AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/AAAAAAAAAAAAAIA/"
            "AAAAAAAAAAAAAIA+AAABAAIAAAAAAAAAAQAAAAABAAAAAIA/AAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAAAAAAAAgD4AAIA+AAAAAAAAAAAAAIA/"
            "AAAAAAAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAAAAAACAPwAAAAAAAAAAAACAvwAAAAAAAIA/AACAPwAAAAAAAAAAAAAAAAAAAAAAAIA/"
            "AAAAAAAAAAAAAAAAAAAAAAAAgD8AAAAAAAAAAAAAAAAAAAAAAACAPwAAAAAAAABAAAAAAAAAAAAAAAAAAACAPwAAAAAAAAAA8wQ1P/MENT8=";

        // A skeleton of two joints, "Root" and "Tip" above it, under the node "Armature" (moved 3 m along Z). The skin
        // lists "Tip" first, so the loader must put the joints in order. "Body" is the skinned triangle (its own
        // translation must be ignored); "Scythe" is the same triangle held by "Tip", 1 m in front of it. "Wave" turns
        // "Tip".
        std::string MakeSkinnedGLTF()
        {
            return R"({
                "asset": {"version": "2.0"},
                "scene": 0,
                "scenes": [{"nodes": [0, 4]}],
                "nodes": [
                    {"name": "Armature", "translation": [0, 0, 3], "children": [1]},
                    {"name": "Root", "children": [2]},
                    {"name": "Tip", "translation": [0, 1, 0], "children": [3]},
                    {"name": "Scythe", "mesh": 0, "translation": [0, 0, 1]},
                    {"name": "Body", "mesh": 0, "skin": 0, "translation": [5, 5, 5]}
                ],
                "skins": [{"joints": [2, 1], "inverseBindMatrices": 4}],
                "meshes": [{"primitives": [{
                    "attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2, "JOINTS_0": 7, "WEIGHTS_0": 8},
                    "indices": 3
                }]}],
                "animations": [{
                    "name": "Wave",
                    "channels": [{"sampler": 0, "target": {"node": 2, "path": "rotation"}}],
                    "samplers": [{"input": 5, "output": 6, "interpolation": "LINEAR"}]
                }],
                "buffers": [{"byteLength": 332,
                             "uri": "data:application/octet-stream;base64,)" + std::string(SkinnedBuffer) + R"("}],
                "bufferViews": [
                    {"buffer": 0, "byteOffset": 0, "byteLength": 36},
                    {"buffer": 0, "byteOffset": 36, "byteLength": 36},
                    {"buffer": 0, "byteOffset": 72, "byteLength": 24},
                    {"buffer": 0, "byteOffset": 96, "byteLength": 6},
                    {"buffer": 0, "byteOffset": 104, "byteLength": 12},
                    {"buffer": 0, "byteOffset": 116, "byteLength": 48},
                    {"buffer": 0, "byteOffset": 164, "byteLength": 128},
                    {"buffer": 0, "byteOffset": 292, "byteLength": 8},
                    {"buffer": 0, "byteOffset": 300, "byteLength": 32}
                ],
                "accessors": [
                    {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3", "min": [0, 0, 0], "max": [1, 1, 0]},
                    {"bufferView": 1, "componentType": 5126, "count": 3, "type": "VEC3"},
                    {"bufferView": 2, "componentType": 5126, "count": 3, "type": "VEC2"},
                    {"bufferView": 3, "componentType": 5123, "count": 3, "type": "SCALAR"},
                    {"bufferView": 6, "componentType": 5126, "count": 2, "type": "MAT4"},
                    {"bufferView": 7, "componentType": 5126, "count": 2, "type": "SCALAR", "min": [0], "max": [2]},
                    {"bufferView": 8, "componentType": 5126, "count": 2, "type": "VEC4"},
                    {"bufferView": 4, "componentType": 5121, "count": 3, "type": "VEC4"},
                    {"bufferView": 5, "componentType": 5126, "count": 3, "type": "VEC4"}
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

    TEST_F(GLTFLoaderTest, ReadsSkeletonWithParentsFirst)
    {
        WriteFile(MakeSkinnedGLTF());

        const std::expected<ModelData, std::string> model = LoadGLTFFile(m_filePath);

        ASSERT_TRUE(model.has_value()) << model.error();
        ASSERT_TRUE(model->skeleton.has_value());
        const std::vector<SkeletonJoint>& joints = model->skeleton->joints;
        ASSERT_EQ(joints.size(), 2u);
        EXPECT_EQ(joints[0].name, "Root");
        EXPECT_FALSE(joints[0].parent.has_value());
        EXPECT_EQ(joints[1].name, "Tip");
        EXPECT_EQ(joints[1].parent, 0u);
        EXPECT_FLOAT_EQ(joints[1].translation.y, 1.0f);

        // The inverse bind matrix goes with its joint: "Tip" was first in the skin, its matrix moves down by 1.
        EXPECT_FLOAT_EQ(joints[1].inverseBindMatrix[3][1], -1.0f);
        EXPECT_FLOAT_EQ(joints[0].inverseBindMatrix[3][1], 0.0f);

        // The node above the root joint places the skeleton.
        EXPECT_FLOAT_EQ(model->skeleton->rootTransform[3][2], 3.0f);
    }

    TEST_F(GLTFLoaderTest, SkinnedVerticesReferToOrderedJoints)
    {
        WriteFile(MakeSkinnedGLTF());

        const std::expected<ModelData, std::string> model = LoadGLTFFile(m_filePath);

        ASSERT_TRUE(model.has_value()) << model.error();
        const auto body = std::ranges::find(model->parts, "Body", &ModelPartData::name);
        ASSERT_NE(body, model->parts.end());

        // The skeleton places a skinned mesh: the translation of its node is ignored.
        EXPECT_EQ(body->transform, glm::mat4(1.0f));
        EXPECT_FALSE(body->parentJoint.has_value());

        // Skin joint 0 ("Tip") is joint 1 of the skeleton, skin joint 1 ("Root") is joint 0.
        ASSERT_EQ(body->mesh.skin.size(), 3u);
        EXPECT_EQ(body->mesh.skin[0].joints.x, 1u);
        EXPECT_EQ(body->mesh.skin[1].joints.x, 0u);
        // The unused slots (weight 0) hold skin joint 0 too, which becomes joint 1 like the others.
        EXPECT_EQ(body->mesh.skin[2].joints, glm::uvec4(1u, 0u, 1u, 1u));

        // The weights 0.25 and 0.25 are made to add up to 1.
        EXPECT_FLOAT_EQ(body->mesh.skin[2].weights.x, 0.5f);
        EXPECT_FLOAT_EQ(body->mesh.skin[2].weights.y, 0.5f);
    }

    TEST_F(GLTFLoaderTest, MeshUnderJointIsHeldByIt)
    {
        WriteFile(MakeSkinnedGLTF());

        const std::expected<ModelData, std::string> model = LoadGLTFFile(m_filePath);

        ASSERT_TRUE(model.has_value()) << model.error();
        const auto scythe = std::ranges::find(model->parts, "Scythe", &ModelPartData::name);
        ASSERT_NE(scythe, model->parts.end());
        EXPECT_EQ(scythe->parentJoint, 1u);
        EXPECT_FLOAT_EQ(scythe->transform[3][2], 1.0f);
        EXPECT_TRUE(scythe->mesh.skin.empty());
    }

    TEST_F(GLTFLoaderTest, ReadsAnimationClipOfJoints)
    {
        WriteFile(MakeSkinnedGLTF());

        const std::expected<ModelData, std::string> model = LoadGLTFFile(m_filePath);

        ASSERT_TRUE(model.has_value()) << model.error();
        ASSERT_EQ(model->animations.size(), 1u);
        const AnimationClipData& clip = model->animations[0];
        EXPECT_EQ(clip.name, "Wave");
        EXPECT_FLOAT_EQ(clip.duration, 2.0f);
        ASSERT_EQ(clip.channels.size(), 1u);

        const AnimationChannelData& channel = clip.channels[0];
        EXPECT_EQ(channel.joint, 1u);
        EXPECT_EQ(channel.path, AnimationPath::Rotation);
        EXPECT_EQ(channel.interpolation, AnimationInterpolation::Linear);
        EXPECT_EQ(channel.times, (std::vector<float>{0.0f, 2.0f}));
        ASSERT_EQ(channel.values.size(), 2u);
        EXPECT_NEAR(channel.values[1].z, 0.7071068f, 1e-6f);
        EXPECT_NEAR(channel.values[1].w, 0.7071068f, 1e-6f);
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
