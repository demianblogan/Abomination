#pragma once

#include "Core/Files/Image.h"
#include "Renderer/Assets/MeshData.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace Abomination::Renderer
{
    // The material of a part as the model file describes it (glTF metallic-roughness): the images of its maps, as indices
    // into ModelData::images, and the numbers they are multiplied by. The defaults are those of glTF for a material
    // that leaves them out. ModelStore turns it into a Renderer::Material.
    struct ModelMaterialData
    {
        std::optional<std::size_t> baseColorImage;
        std::optional<std::size_t> normalImage;
        std::optional<std::size_t> metalRoughnessImage;
        std::optional<std::size_t> emissiveImage;

        glm::vec4 baseColorFactor{1.0f};
        float roughnessFactor = 1.0f;
        float metalnessFactor = 1.0f;
        glm::vec3 emissiveFactor{0.0f};
    };

    // One part of a model: a piece with one material that moves as a whole. The shotgun has ten: the body, the pump,
    // the barrel, the stock, the loading gate and others. Keeping the parts apart (not merged into one mesh) lets the game move
    // one of them later, for example slide the pump back after a shot.
    struct ModelPartData
    {
        // The name the artist gave the part ("pump_shotgun_0"), to find it by.
        std::string name;

        // The geometry, in the coordinates of the part itself.
        MeshData mesh;

        // Where the part is in the model: moves the coordinates of the part into those of the whole model (meters, +Y up).
        // In a model file parts sit in a tree of nodes, each moved, turned and scaled relative to its parent; this is the
        // whole chain from the root to the part multiplied into one matrix.
        glm::mat4 transform{1.0f};

        // What the part is made of; none for a part without a material in the file (it gets the default material).
        std::optional<ModelMaterialData> material;

        // A part held by a joint of the skeleton (the scythe in a hand): the index of the joint in SkeletonData::joints,
        // and then transform places the part relative to that joint, so it moves with it. A skinned part (its mesh has a
        // skin) has neither: the skeleton places every vertex, and transform is the identity.
        std::optional<std::size_t> parentJoint;
    };

    // One joint (a "bone") of a skeleton: a node of the model file that skinned vertices follow (see VertexSkin).
    struct SkeletonJoint
    {
        std::string name;

        // The joint it hangs from (an index into SkeletonData::joints, always smaller than this joint's own), or none for a
        // root joint, which hangs from SkeletonData::rootTransform.
        std::optional<std::size_t> parent;

        // Where it is relative to its parent when no animation moves it (the rest pose): moved, turned and scaled. An
        // animation replaces these three, each on its own (see AnimationChannelData).
        glm::vec3 translation{0.0f};
        glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 scale{1.0f};

        // Takes a vertex from the model into the space of this joint as it was when the skin was bound to the
        // skeleton (the inverse of the joint's transform in the model then). The joint's transform now times this matrix
        // moves the vertex as far as the joint has moved since then.
        glm::mat4 inverseBindMatrix{1.0f};
    };

    // The skeleton of a model: its joints, parents before their children, so one pass from the first to the last can
    // combine every joint with its already combined parent.
    struct SkeletonData
    {
        std::vector<SkeletonJoint> joints;

        // Where the root joints hang in the model. The glTF loader takes the nodes above the joints of the skin into the
        // skeleton (they may be animated), so for a loaded model this is the identity.
        glm::mat4 rootTransform{1.0f};
    };

    // What an animation channel moves: the place of a joint, its rotation, or its scale.
    enum class AnimationPath
    {
        Translation,
        Rotation,
        Scale,
    };

    // How a channel goes from one key to the next: jumps (Step) or changes evenly (Linear; a rotation along the shortest
    // arc, see Renderer::SampleAnimationChannel).
    enum class AnimationInterpolation
    {
        Step,
        Linear,
    };

    // One property of one joint over time: keys at times (seconds, rising), with a value each. A translation or a scale
    // uses x, y and z of the value; a rotation is a quaternion stored as (x, y, z, w), as in glTF.
    struct AnimationChannelData
    {
        std::size_t joint = 0;
        AnimationPath path = AnimationPath::Translation;
        AnimationInterpolation interpolation = AnimationInterpolation::Linear;
        std::vector<float> times;
        std::vector<glm::vec4> values;
    };

    // An animation clip ("Walk", "Attack"): channels that move joints of the skeleton over duration seconds.
    struct AnimationClipData
    {
        std::string name;
        float duration = 0.0f;
        std::vector<AnimationChannelData> channels;
    };

    // A model read from a file, in ordinary memory, before it is uploaded to the GPU. Needs no OpenGL, so it can be
    // tested.
    struct ModelData
    {
        std::vector<ModelPartData> parts;

        // The base color textures of the parts, decoded. An image that could not be decoded is left empty (width 0).
        std::vector<Core::Image> images;

        // The skeleton that bends the skinned parts and holds the parts with a parentJoint, and its animation clips. A
        // rigid model (the shotgun) has neither.
        std::optional<SkeletonData> skeleton;
        std::vector<AnimationClipData> animations;
    };
}
