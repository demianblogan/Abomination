#include "Renderer/Assets/GLTFLoader.h"

#include "Core/Files/FileSystem.h"
#include "Renderer/Assets/MeshTangents.h"

// cgltf is a "single-header" C library, like stb_image: its implementation is compiled only where CGLTF_IMPLEMENTATION
// is defined, in exactly one .cpp file of the program. This is that file.
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <format>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

// How a glTF file is organized, from the top down (only the parts read here):
//   node      - a place in the tree of the model: moved, turned and scaled relative to its parent; may have a mesh.
//   mesh      - one or more primitives.
//   primitive - triangles with one material: its vertex attributes (position, normal, texture coordinates) and indices.
//   accessor  - how to read one attribute from the binary data: where it starts, how many elements, of which type.
//   material  - how a surface looks; here only its base color texture is used.
//   image     - a picture (PNG, JPG) stored in the binary data of the file or in a file next to it.
//   skin      - the joints (nodes) that bend a skinned mesh, and their inverse bind matrices.
//   animation - channels, each moving one property (translation, rotation, scale) of one node through keys over time.
// cgltf parses the JSON part and links everything with pointers; the code below walks these pointers.
namespace Abomination::Renderer
{
    namespace
    {
        // cgltf reads files (the .bin and images of a .gltf) through these two functions instead of fopen(), so paths
        // with non-English characters work: cgltf gives paths as UTF-8, and our ReadBinaryFile handles them.
        cgltf_result ReadFile(const cgltf_memory_options*, const cgltf_file_options*, const char* path, cgltf_size* size,
                              void** data)
        {
            const std::expected<std::vector<std::byte>, std::string> contents =
                Core::ReadBinaryFile(std::filesystem::path(reinterpret_cast<const char8_t*>(path)));
            if (!contents.has_value())
                return cgltf_result_file_not_found;

            // cgltf frees the data with ReleaseFile below, so it is allocated with malloc, not new.
            *data = std::malloc(contents->size());
            if (*data == nullptr)
                return cgltf_result_out_of_memory;

            std::memcpy(*data, contents->data(), contents->size());
            *size = contents->size();

            return cgltf_result_success;
        }

        void ReleaseFile(const cgltf_memory_options*, const cgltf_file_options*, void* data)
        {
            std::free(data);
        }

        // Frees what cgltf_parse() allocated when it goes out of scope (see STBPixels in Image.cpp for the pattern).
        using ParsedGLTF = std::unique_ptr<cgltf_data, decltype(&cgltf_free)>;

        // Finds the attribute of a primitive, or nullptr if it has none. index tells TEXCOORD_0 from TEXCOORD_1.
        const cgltf_accessor* FindAttribute(const cgltf_primitive& primitive, cgltf_attribute_type type, int index = 0)
        {
            for (const cgltf_attribute& attribute : std::span(primitive.attributes, primitive.attributes_count))
                if (attribute.type == type && attribute.index == index)
                    return attribute.data;

            return nullptr;
        }

        // Reads the triangles of one primitive into mesh data, in the coordinates of its node.
        std::expected<MeshData, std::string> ReadPrimitive(const cgltf_primitive& primitive, bool isMirrored,
                                                           const std::vector<std::size_t>* jointOfSkinIndex)
        {
            if (primitive.type != cgltf_primitive_type_triangles)
                return std::unexpected("only triangles are supported");

            const cgltf_accessor* positions = FindAttribute(primitive, cgltf_attribute_type_position);
            if (positions == nullptr)
                return std::unexpected("a primitive has no positions");

            const cgltf_accessor* normals = FindAttribute(primitive, cgltf_attribute_type_normal);
            const cgltf_accessor* texCoords = FindAttribute(primitive, cgltf_attribute_type_texcoord);

            MeshData mesh;
            mesh.vertices.resize(positions->count);
            for (cgltf_size index = 0; index < positions->count; ++index)
            {
                // cgltf_accessor_read_float converts whatever the file stores (floats, or normalized integers) to floats.
                MeshVertex& vertex = mesh.vertices[index];
                cgltf_accessor_read_float(positions, index, glm::value_ptr(vertex.position), 3);
                if (normals != nullptr)
                    cgltf_accessor_read_float(normals, index, glm::value_ptr(vertex.normal), 3);

                if (texCoords != nullptr)
                {
                    // glTF puts the texture coordinate (0, 0) at the top left corner of the image. Our images are stored
                    // with the bottom row first (see Core::Image), where v = 0 is the bottom, so v is turned around.
                    cgltf_accessor_read_float(texCoords, index, glm::value_ptr(vertex.texCoord), 2);
                    vertex.texCoord.y = 1.0f - vertex.texCoord.y;
                }
            }

            // The skin of a skinned mesh: which joints pull every vertex and how much. The file numbers the joints by their
            // place in its skin; jointOfSkinIndex turns that into their place in SkeletonData::joints (see ReadSkeleton).
            if (jointOfSkinIndex != nullptr)
            {
                const cgltf_accessor* joints = FindAttribute(primitive, cgltf_attribute_type_joints);
                const cgltf_accessor* weights = FindAttribute(primitive, cgltf_attribute_type_weights);
                if (joints == nullptr || weights == nullptr || joints->count != positions->count ||
                    weights->count != positions->count)
                    return std::unexpected("a skinned primitive has no joints and weights for every vertex");

                mesh.skin.resize(positions->count);
                for (cgltf_size index = 0; index < positions->count; ++index)
                {
                    VertexSkin& skin = mesh.skin[index];
                    cgltf_uint skinJoints[4] = {};
                    cgltf_accessor_read_uint(joints, index, skinJoints, 4);
                    cgltf_accessor_read_float(weights, index, glm::value_ptr(skin.weights), 4);
                    for (int slot = 0; slot < 4; ++slot)
                    {
                        if (skinJoints[slot] >= jointOfSkinIndex->size())
                            return std::unexpected("a vertex refers to a joint the skin does not have");
                        skin.joints[slot] = static_cast<glm::uint>((*jointOfSkinIndex)[skinJoints[slot]]);
                    }

                    // Exporters round the weights; they must add up to 1, or the vertex would shrink towards the origin.
                    const float sum = skin.weights.x + skin.weights.y + skin.weights.z + skin.weights.w;
                    skin.weights = sum > 0.0f ? skin.weights / sum : glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
                }
            }

            // Without indices every three vertices in a row are a triangle.
            const cgltf_size indexCount = primitive.indices != nullptr ? primitive.indices->count : positions->count;
            mesh.indices.resize(indexCount);
            for (cgltf_size index = 0; index < indexCount; ++index)
                mesh.indices[index] = primitive.indices != nullptr
                                          ? static_cast<std::uint32_t>(cgltf_accessor_read_index(primitive.indices, index))
                                          : static_cast<std::uint32_t>(index);

            // A mirroring node transform (an odd number of negative scales) turns counter-clockwise triangles clockwise,
            // and face culling would then hide their fronts. Swapping two corners of every triangle turns them back.
            if (isMirrored)
                for (std::size_t triangle = 0; triangle + 2 < mesh.indices.size(); triangle += 3)
                    std::swap(mesh.indices[triangle + 1], mesh.indices[triangle + 2]);

            // The axes of the texture at every vertex, for the normal map. Always calculated, also when the file has them
            // (TANGENT): those are for the texture coordinates of glTF, whose V runs the other way. Without texture
            // coordinates or normals there is nothing to calculate from, and no normal map could be read either.
            if (normals != nullptr && texCoords != nullptr && !GenerateTangents(mesh))
                return std::unexpected("the tangents of a primitive could not be calculated");

            return mesh;
        }

        // The skeleton of a skin, and how to find its joints by the numbers the file uses.
        struct SkeletonReading
        {
            SkeletonData skeleton;

            // The place in skeleton.joints of the joint number N of the skin (the vertices use these numbers).
            std::vector<std::size_t> jointOfSkinIndex;

            // The place in skeleton.joints of the joint of a node (animation channels and held parts name nodes).
            std::unordered_map<const cgltf_node*, std::size_t> jointOfNode;
        };

        // The joint the node hangs from: its nearest ancestor that is a joint of the skeleton, or nullptr.
        const cgltf_node* FindParentJoint(const cgltf_node& node,
                                          const std::unordered_map<const cgltf_node*, std::size_t>& joints)
        {
            for (const cgltf_node* ancestor = node.parent; ancestor != nullptr; ancestor = ancestor->parent)
                if (joints.contains(ancestor))
                    return ancestor;
            return nullptr;
        }

        // The skeleton is the joints of the skin and every node above them. The nodes above are not bones the skin
        // follows, but they may be animated too: Blender turns the root bone of an FBX ("mixamorig:Hips") into the
        // armature node itself, and the movement of the hips is then an animation of that node. Leaving them out would
        // lose it, and everything below would stand still while it should sway, turn and step. They get the identity as
        // their inverse bind matrix: no vertex follows them directly.
        std::expected<SkeletonReading, std::string> ReadSkeleton(const cgltf_skin& skin)
        {
            const std::span skinJoints(skin.joints, skin.joints_count);

            // The nodes of the skeleton: the joints of the skin and their ancestors, each once. The numbers in the map are
            // filled below.
            std::vector<const cgltf_node*> nodes;
            std::unordered_map<const cgltf_node*, std::size_t> skeletonNodes;
            for (const cgltf_node* joint : skinJoints)
                for (const cgltf_node* node = joint; node != nullptr; node = node->parent)
                    if (skeletonNodes.emplace(node, 0).second)
                        nodes.push_back(node);

            // Where every node is in the skin (for its inverse bind matrix and for the numbers the vertices use).
            std::unordered_map<const cgltf_node*, std::size_t> skinIndexOfNode;
            for (std::size_t skinIndex = 0; skinIndex < skinJoints.size(); ++skinIndex)
                skinIndexOfNode.emplace(skinJoints[skinIndex], skinIndex);

            // The file may list the joints in any order. They are taken parents first: in every round, the nodes whose
            // parent is already taken (or which have none) are taken. A skeleton of N nodes needs at most N rounds.
            SkeletonReading reading;
            reading.jointOfSkinIndex.resize(skinJoints.size());
            std::vector<bool> isTaken(nodes.size(), false);
            while (reading.skeleton.joints.size() < nodes.size())
            {
                const std::size_t takenBefore = reading.skeleton.joints.size();
                for (std::size_t nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex)
                {
                    const cgltf_node& node = *nodes[nodeIndex];
                    if (isTaken[nodeIndex] || (node.parent != nullptr && !reading.jointOfNode.contains(node.parent)))
                        continue;

                    if (node.has_matrix)
                        return std::unexpected("a joint is placed by a matrix instead of translation, rotation and scale");

                    SkeletonJoint joint;
                    joint.name = node.name != nullptr ? node.name : std::format("joint{}", nodeIndex);
                    if (node.parent != nullptr)
                        joint.parent = reading.jointOfNode.at(node.parent);
                    if (node.has_translation)
                        joint.translation = glm::make_vec3(node.translation);

                    // glTF stores a rotation as (x, y, z, w); glm::quat is built as (w, x, y, z).
                    if (node.has_rotation)
                        joint.rotation = glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
                    if (node.has_scale)
                        joint.scale = glm::make_vec3(node.scale);

                    const std::size_t jointIndex = reading.skeleton.joints.size();
                    if (const auto skinIndex = skinIndexOfNode.find(&node); skinIndex != skinIndexOfNode.end())
                    {
                        if (skin.inverse_bind_matrices != nullptr)
                            cgltf_accessor_read_float(skin.inverse_bind_matrices, skinIndex->second,
                                                      glm::value_ptr(joint.inverseBindMatrix), 16);
                        reading.jointOfSkinIndex[skinIndex->second] = jointIndex;
                    }

                    isTaken[nodeIndex] = true;
                    reading.jointOfNode.emplace(&node, jointIndex);
                    reading.skeleton.joints.push_back(std::move(joint));
                }

                if (reading.skeleton.joints.size() == takenBefore)
                    return std::unexpected("the joints of the skin do not form a tree");
            }

            return reading;
        }

        // The animation clips of the file, with only the channels that move joints of the skeleton (a clip may also move
        // other nodes, like a camera, which the game does not animate).
        std::vector<AnimationClipData> ReadAnimations(const cgltf_data& data,
                                                      const std::unordered_map<const cgltf_node*, std::size_t>& jointOfNode)
        {
            std::vector<AnimationClipData> clips;
            for (const cgltf_animation& animation : std::span(data.animations, data.animations_count))
            {
                AnimationClipData clip;
                clip.name = animation.name != nullptr ? animation.name : std::format("clip{}", clips.size());
                for (const cgltf_animation_channel& channel : std::span(animation.channels, animation.channels_count))
                {
                    const auto joint = jointOfNode.find(channel.target_node);
                    if (joint == jointOfNode.end())
                        continue;

                    AnimationChannelData result{.joint = joint->second};
                    int componentCount = 3;
                    switch (channel.target_path)
                    {
                    case cgltf_animation_path_type_translation:
                        result.path = AnimationPath::Translation;
                        break;
                    case cgltf_animation_path_type_rotation:
                        result.path = AnimationPath::Rotation;
                        componentCount = 4;
                        break;
                    case cgltf_animation_path_type_scale:
                        result.path = AnimationPath::Scale;
                        break;
                    default:
                        continue;
                    }

                    // A cubic spline stores three values per key (the tangent in, the value, the tangent out); only the
                    // value is kept, and the channel changes linearly between keys: close enough for exported clips,
                    // which have a key on every frame.
                    const cgltf_animation_sampler& sampler = *channel.sampler;
                    const bool isCubic = sampler.interpolation == cgltf_interpolation_type_cubic_spline;
                    result.interpolation = sampler.interpolation == cgltf_interpolation_type_step
                                               ? AnimationInterpolation::Step
                                               : AnimationInterpolation::Linear;

                    result.times.resize(sampler.input->count);
                    result.values.resize(sampler.input->count);
                    for (cgltf_size key = 0; key < sampler.input->count; ++key)
                    {
                        cgltf_accessor_read_float(sampler.input, key, &result.times[key], 1);
                        const cgltf_size valueIndex = isCubic ? key * 3 + 1 : key;
                        cgltf_accessor_read_float(sampler.output, valueIndex, glm::value_ptr(result.values[key]),
                                                  componentCount);
                    }

                    if (!result.times.empty())
                        clip.duration = std::max(clip.duration, result.times.back());
                    clip.channels.push_back(std::move(result));
                }

                if (!clip.channels.empty())
                    clips.push_back(std::move(clip));
            }
            return clips;
        }

        // Decodes one image of the file: from the binary data of a .glb, or from a file next to a .gltf.
        std::expected<Core::Image, std::string> ReadImage(const cgltf_image& image, const std::filesystem::path& modelPath,
                                                          std::string_view name)
        {
            if (image.buffer_view != nullptr)
            {
                const auto* bytes = reinterpret_cast<const std::byte*>(cgltf_buffer_view_data(image.buffer_view));
                return Core::DecodeImage(std::span(bytes, image.buffer_view->size), name);
            }

            if (image.uri == nullptr || std::string_view(image.uri).starts_with("data:"))
                return std::unexpected(std::format("the image {} is not stored in a supported way", name));

            // A relative path, with special characters written as %xx ("My%20Texture.png"); cgltf_decode_uri turns them
            // back in place, so a copy is decoded.
            std::string uri = image.uri;
            uri.resize(cgltf_decode_uri(uri.data()));

            const std::filesystem::path relativePath(reinterpret_cast<const char8_t*>(uri.c_str()));
            return Core::LoadImageFile(modelPath.parent_path() / relativePath);
        }

        // The index in the file of the image a texture shows, if it shows one.
        std::optional<std::size_t> GetImageIndex(const cgltf_data& data, const cgltf_texture* texture)
        {
            if (texture == nullptr || texture->image == nullptr)
                return std::nullopt;

            return static_cast<std::size_t>(texture->image - data.images);
        }

        // A material of the file: its maps and numbers, metallic-roughness (the material model of glTF 2.0). The numbers a
        // file leaves out keep the defaults of glTF, which cgltf has filled in already.
        ModelMaterialData ReadMaterial(const cgltf_data& data, const cgltf_material& material)
        {
            ModelMaterialData result;
            if (material.has_pbr_metallic_roughness)
            {
                const cgltf_pbr_metallic_roughness& surface = material.pbr_metallic_roughness;
                result.baseColorImage = GetImageIndex(data, surface.base_color_texture.texture);
                result.metalRoughnessImage = GetImageIndex(data, surface.metallic_roughness_texture.texture);
                result.baseColorFactor = glm::make_vec4(surface.base_color_factor);
                result.roughnessFactor = surface.roughness_factor;
                result.metalnessFactor = surface.metallic_factor;
            }
            result.normalImage = GetImageIndex(data, material.normal_texture.texture);
            result.emissiveImage = GetImageIndex(data, material.emissive_texture.texture);
            result.emissiveFactor = glm::make_vec3(material.emissive_factor);

            // KHR_materials_emissive_strength: the emission may be brighter than 1, which the factor alone cannot say.
            if (material.has_emissive_strength)
                result.emissiveFactor *= material.emissive_strength.emissive_strength;

            return result;
        }
    }

    std::expected<ModelData, std::string> LoadGLTFFile(const std::filesystem::path& path)
    {
        const std::string pathText = Core::ToUTF8String(path);

        const std::expected<std::vector<std::byte>, std::string> fileContents = Core::ReadBinaryFile(path);
        if (!fileContents.has_value())
            return std::unexpected(fileContents.error());

        cgltf_options options{};
        options.file.read = &ReadFile;
        options.file.release = &ReleaseFile;

        // 1. The JSON part: the structure of the model. The file type (.glb or .gltf) is recognized by the contents.
        cgltf_data* rawData = nullptr;
        if (cgltf_parse(&options, fileContents->data(), fileContents->size(), &rawData) != cgltf_result_success)
            return std::unexpected(std::format("not a glTF 2.0 file: {}", pathText));
        const ParsedGLTF data(rawData, &cgltf_free);

        // 2. The binary data the accessors point into: the second part of a .glb, or the .bin file of a .gltf.
        if (cgltf_load_buffers(&options, data.get(), pathText.c_str()) != cgltf_result_success)
            return std::unexpected(std::format("the binary data of the model could not be read: {}", pathText));

        // 3. Checks that every accessor stays inside its buffer, so reading them below cannot go out of bounds.
        if (cgltf_validate(data.get()) != cgltf_result_success)
            return std::unexpected(std::format("the model is broken: {}", pathText));

        ModelData model;

        // The images first, so the parts can refer to them by index. Only the images a material uses as one of its maps are
        // decoded (a file may hold more: thumbnails, maps of extensions the game does not read). Every other image is left
        // empty in its place (the indices stay those of the file); so is a broken one, whose part then gets the fallback
        // texture.
        std::vector<bool> isUsed(data->images_count, false);
        for (const cgltf_material& material : std::span(data->materials, data->materials_count))
        {
            const ModelMaterialData maps = ReadMaterial(*data, material);
            for (const std::optional<std::size_t> image :
                 {maps.baseColorImage, maps.normalImage, maps.metalRoughnessImage, maps.emissiveImage})
                if (image.has_value())
                    isUsed[*image] = true;
        }
        for (std::size_t index = 0; index < data->images_count; ++index)
        {
            if (!isUsed[index])
            {
                model.images.emplace_back();
                continue;
            }
            const std::string name = std::format("{}#image{}", pathText, index);
            std::expected<Core::Image, std::string> decoded = ReadImage(data->images[index], path, name);
            model.images.push_back(decoded.has_value() ? std::move(*decoded) : Core::Image{});
        }

        // The skeleton, before the parts that refer to its joints. One skin per model: a character is one skinned body.
        if (data->skins_count > 1)
            return std::unexpected(std::format("only one skin per model is supported: {}", pathText));

        std::optional<SkeletonReading> skeleton;
        if (data->skins_count == 1)
        {
            std::expected<SkeletonReading, std::string> reading = ReadSkeleton(data->skins[0]);
            if (!reading.has_value())
                return std::unexpected(std::format("{}: {}", pathText, reading.error()));
            skeleton = std::move(*reading);
        }

        // Every node with a mesh gives one part per primitive.
        for (const cgltf_node& node : std::span(data->nodes, data->nodes_count))
        {
            if (node.mesh == nullptr)
                continue;

            // Where the part is: a skinned mesh is placed by its skeleton (glTF ignores the transform of its node); a mesh
            // under a joint is placed relative to that joint (the chain of transforms from the joint down to the node);
            // any other mesh by the whole chain of transforms from the root of the tree to its node. All as matrices,
            // column-major like glm. A negative determinant means the transform mirrors the part (see ReadPrimitive).
            glm::mat4 transform(1.0f);
            std::optional<std::size_t> parentJoint;
            const bool isSkinned = node.skin != nullptr && skeleton.has_value();
            const cgltf_node* holdingJoint = skeleton.has_value() ? FindParentJoint(node, skeleton->jointOfNode) : nullptr;
            if (!isSkinned && holdingJoint != nullptr)
            {
                for (const cgltf_node* link = &node; link != holdingJoint; link = link->parent)
                {
                    glm::mat4 local(1.0f);
                    cgltf_node_transform_local(link, glm::value_ptr(local));
                    transform = local * transform;
                }
                parentJoint = skeleton->jointOfNode.at(holdingJoint);
            }
            else if (!isSkinned)
            {
                cgltf_node_transform_world(&node, glm::value_ptr(transform));
            }
            const bool isMirrored = glm::determinant(glm::mat3(transform)) < 0.0f;

            const std::span primitives(node.mesh->primitives, node.mesh->primitives_count);
            for (std::size_t primitiveIndex = 0; primitiveIndex < primitives.size(); ++primitiveIndex)
            {
                const cgltf_primitive& primitive = primitives[primitiveIndex];
                std::expected<MeshData, std::string> mesh =
                    ReadPrimitive(primitive, isMirrored, isSkinned ? &skeleton->jointOfSkinIndex : nullptr);
                if (!mesh.has_value())
                    return std::unexpected(std::format("{}: {}", pathText, mesh.error()));

                // A mesh of one primitive is named like the node; more primitives get a number each.
                std::string name = node.name != nullptr ? node.name : std::format("part{}", model.parts.size());
                if (primitives.size() > 1)
                    name += std::format("#{}", primitiveIndex);

                ModelPartData part{
                    .name = std::move(name), .mesh = std::move(*mesh), .transform = transform, .parentJoint = parentJoint};

                // The material of the primitive, with its maps as indices into model.images.
                if (primitive.material != nullptr)
                    part.material = ReadMaterial(*data, *primitive.material);

                model.parts.push_back(std::move(part));
            }
        }

        if (model.parts.empty())
            return std::unexpected(std::format("the model has no meshes: {}", pathText));

        if (skeleton.has_value())
        {
            model.animations = ReadAnimations(*data, skeleton->jointOfNode);
            model.skeleton = std::move(skeleton->skeleton);
        }

        return model;
    }
}
