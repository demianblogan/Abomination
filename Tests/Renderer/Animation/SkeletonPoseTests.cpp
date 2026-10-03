#include "Renderer/Animation/SkeletonPose.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec4.hpp>
#include <gtest/gtest.h>

#include <numbers>
#include <vector>

namespace Abomination::Renderer
{
    namespace
    {
        // An arm of two joints: "Shoulder" at the root, 2 m above the origin of the model, and "Elbow" 1 m along its X.
        // The inverse bind matrices are those of this rest pose, as an exporter writes them.
        SkeletonData CreateArm()
        {
            SkeletonData skeleton;
            skeleton.rootTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, 0.0f));
            skeleton.joints.push_back({.name = "Shoulder"});
            skeleton.joints.push_back({.name = "Elbow", .parent = 0, .translation = {1.0f, 0.0f, 0.0f}});

            std::vector<glm::mat4> rest;
            CalculateJointMatrices(skeleton, CreateRestPose(skeleton), rest);
            for (std::size_t index = 0; index < rest.size(); ++index)
                skeleton.joints[index].inverseBindMatrix = glm::inverse(rest[index]);
            return skeleton;
        }

        glm::vec3 Transform(const glm::mat4& matrix, const glm::vec3& point)
        {
            return glm::vec3(matrix * glm::vec4(point, 1.0f));
        }
    }

    TEST(SkeletonPose, RestPoseTakesJointsFromSkeleton)
    {
        const SkeletonData skeleton = CreateArm();

        const std::vector<JointPose> pose = CreateRestPose(skeleton);

        ASSERT_EQ(pose.size(), 2u);
        EXPECT_EQ(pose[1].translation, glm::vec3(1.0f, 0.0f, 0.0f));
    }

    TEST(SkeletonPose, ChildJointHangsFromParentAndRoot)
    {
        const SkeletonData skeleton = CreateArm();
        std::vector<glm::mat4> jointMatrices;

        CalculateJointMatrices(skeleton, CreateRestPose(skeleton), jointMatrices);

        // The elbow: 1 m along X from the shoulder, which is 2 m up.
        const glm::vec3 elbow = Transform(jointMatrices[1], glm::vec3(0.0f));
        EXPECT_FLOAT_EQ(elbow.x, 1.0f);
        EXPECT_FLOAT_EQ(elbow.y, 2.0f);
    }

    TEST(SkeletonPose, TurningParentMovesChild)
    {
        // The shoulder turns 90 degrees around Z: the elbow, 1 m along its X, swings up.
        const SkeletonData skeleton = CreateArm();
        std::vector<JointPose> pose = CreateRestPose(skeleton);
        pose[0].rotation = glm::angleAxis(std::numbers::pi_v<float> / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f));
        std::vector<glm::mat4> jointMatrices;

        CalculateJointMatrices(skeleton, pose, jointMatrices);

        const glm::vec3 elbow = Transform(jointMatrices[1], glm::vec3(0.0f));
        EXPECT_NEAR(elbow.x, 0.0f, 1e-6f);
        EXPECT_NEAR(elbow.y, 3.0f, 1e-6f);
    }

    TEST(SkeletonPose, SkinningMatricesKeepVerticesAtRest)
    {
        const SkeletonData skeleton = CreateArm();
        std::vector<glm::mat4> jointMatrices;
        std::vector<glm::mat4> skinningMatrices;

        CalculateJointMatrices(skeleton, CreateRestPose(skeleton), jointMatrices);
        CalculateSkinningMatrices(skeleton, jointMatrices, skinningMatrices);

        // At rest a joint's matrix and its inverse bind matrix cancel out: the skin stays where it was modelled.
        const glm::vec3 vertex(1.5f, 2.0f, 0.0f);
        for (const glm::mat4& skinning : skinningMatrices)
        {
            const glm::vec3 moved = Transform(skinning, vertex);
            EXPECT_NEAR(moved.x, vertex.x, 1e-6f);
            EXPECT_NEAR(moved.y, vertex.y, 1e-6f);
        }
    }

    TEST(SkeletonPose, SkinnedVertexFollowsTurnedJoint)
    {
        // A vertex of the forearm, 0.5 m past the elbow; the elbow bends 90 degrees around Z, so the vertex swings up.
        const SkeletonData skeleton = CreateArm();
        std::vector<JointPose> pose = CreateRestPose(skeleton);
        pose[1].rotation = glm::angleAxis(std::numbers::pi_v<float> / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f));
        std::vector<glm::mat4> jointMatrices;
        std::vector<glm::mat4> skinningMatrices;

        CalculateJointMatrices(skeleton, pose, jointMatrices);
        CalculateSkinningMatrices(skeleton, jointMatrices, skinningMatrices);

        const glm::vec3 moved = Transform(skinningMatrices[1], glm::vec3(1.5f, 2.0f, 0.0f));
        EXPECT_NEAR(moved.x, 1.0f, 1e-6f);
        EXPECT_NEAR(moved.y, 2.5f, 1e-6f);
    }
}
