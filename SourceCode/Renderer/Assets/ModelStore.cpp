#include "Renderer/Assets/ModelStore.h"

#include "Core/Logging/Log.h"
#include "Renderer/Animation/SkeletonPose.h"
#include "Renderer/Assets/GLTFLoader.h"

#include <glm/common.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <expected>
#include <limits>
#include <format>
#include <optional>
#include <span>
#include <unordered_map>
#include <utility>

namespace Abomination::Renderer
{
    using Core::LogCategory;
    using Core::LogLevel;

    ModelStore::ModelStore(std::filesystem::path assetsDirectory)
        : m_assetsDirectory(std::move(assetsDirectory))
    {}

    ModelHandle ModelStore::Load(const std::string& path, Core::AssetLifetime lifetime, MeshStore& meshes,
                                 TextureStore& textures)
    {
        if (const std::optional<ModelHandle> loadedHandle = m_cache.Find(path); loadedHandle.has_value())
        {
            // The meshes and textures of the model get the longer lifetime too: a level model that the weapon in the hands
            // asks for with Global must not lose its parts when the level is unloaded.
            m_cache.ExtendLifetime(*loadedHandle, lifetime);
            for (const ModelPart& part : Get(*loadedHandle).parts)
            {
                meshes.ExtendLifetime(part.mesh, lifetime);

                // The maps a material does not have are invalid handles, which ExtendLifetime skips.
                const Material& material = part.material;
                for (const TextureHandle map : {material.baseColor, material.normal, material.metalRoughness, material.emissive})
                    textures.ExtendLifetime(map, lifetime);
            }

            return *loadedHandle;
        }

        std::filesystem::path fullPath = m_assetsDirectory / path;
        fullPath.make_preferred();

        std::expected<ModelData, std::string> data = LoadGLTFFile(fullPath);
        if (!data.has_value())
        {
            Core::Log::Write(LogCategory::Renderer, LogLevel::Warning, "Model {} replaced by the fallback: {}", path,
                             data.error());
            m_fallbackPaths.insert(path);

            return m_cache.Add(path, m_fallbackModel, lifetime);
        }

        // The pieces the game moves on their own become parts, and the copies left behind them get their textures. A split
        // that takes nothing (the model file was changed) is only a warning: the model is drawn as the file has it.
        std::unordered_map<std::string, TextureHandle> backingTextures;
        const auto splits = m_partSplits.find(path);
        const std::span<const ModelPartSplit> pathSplits =
            splits != m_partSplits.end() ? std::span<const ModelPartSplit>(splits->second) : std::span<const ModelPartSplit>();
        for (const ModelPartSplit& split : pathSplits)
        {
            if (!SplitModelPart(*data, split))
            {
                Core::Log::Write(LogCategory::Renderer, LogLevel::Warning, "Model {}: no piece of part {} for part {}", path,
                                 split.sourcePartName, split.partName);
                continue;
            }
            if (!split.backingPartName.empty())
                backingTextures[split.backingPartName] = textures.Load(split.backingTexturePath, lifetime);
        }

        // Only the images the materials use become textures, named after the model ("...Shotgun.glb#image0"), each once
        // however many parts use it. A color is sRGB; a normal or roughness map holds data and is read as it is (Raw). An
        // image used both ways would keep the encoding of its first use (no model of the game does that).
        std::vector<std::optional<TextureHandle>> imageTextures(data->images.size());
        const auto getImageTexture = [&](const std::optional<std::size_t>& index, TextureEncoding encoding)
        {
            if (!index.has_value())
                return TextureHandle{};
            if (!imageTextures[*index].has_value())
                imageTextures[*index] =
                    textures.Add(std::format("{}#image{}", path, *index), data->images[*index], lifetime, encoding);

            return *imageTextures[*index];
        };

        // The material of a part as the file describes it. A part without one gets the default material, with the texture
        // of a backing part if it is one (see ModelPartSplit).
        const auto createMaterial = [&](const ModelPartData& partData)
        {
            if (!partData.material.has_value())
            {
                const auto backing = backingTextures.find(partData.name);
                return Material{.baseColor = backing != backingTextures.end() ? backing->second : TextureHandle{}};
            }

            const ModelMaterialData& file = *partData.material;
            return Material{
                .baseColor = getImageTexture(file.baseColorImage, TextureEncoding::SRGB),
                .baseColorFactor = file.baseColorFactor,
                .normal = getImageTexture(file.normalImage, TextureEncoding::Raw),
                .metalRoughness = getImageTexture(file.metalRoughnessImage, TextureEncoding::Raw),
                .roughnessFactor = file.roughnessFactor,
                .metalnessFactor = file.metalnessFactor,
                .emissive = getImageTexture(file.emissiveImage, TextureEncoding::SRGB),
                .emissiveFactor = file.emissiveFactor,
            };
        };

        Model model;
        for (const ModelPartData& partData : data->parts)
        {
            model.parts.push_back(ModelPart{
                .name = partData.name,
                .mesh = meshes.Add(std::format("{}#{}", path, partData.name), partData.mesh, lifetime),
                .material = createMaterial(partData),
                .transform = partData.transform,
                .parentJoint = partData.parentJoint,
                .isSkinned = !partData.mesh.skin.empty(),
            });
        }

        // The skeleton at rest places the skinned vertices and the parts held by joints.
        std::vector<glm::mat4> restSkinningMatrices;
        if (data->skeleton.has_value())
        {
            CalculateJointMatrices(*data->skeleton, CreateRestPose(*data->skeleton), model.restJointMatrices);
            CalculateSkinningMatrices(*data->skeleton, model.restJointMatrices, restSkinningMatrices);
        }

        // Where every vertex of every part is in the model at rest: moved by the skeleton (a skinned vertex: by its joints,
        // each as much as its weight), by the joint that holds its part, or by the transform of its part.
        std::vector<std::vector<glm::vec3>> restPositions(data->parts.size());
        for (std::size_t partIndex = 0; partIndex < data->parts.size(); ++partIndex)
        {
            const ModelPartData& partData = data->parts[partIndex];
            const MeshData& mesh = partData.mesh;
            restPositions[partIndex].reserve(mesh.vertices.size());
            for (std::size_t vertexIndex = 0; vertexIndex < mesh.vertices.size(); ++vertexIndex)
            {
                const glm::vec4 position(mesh.vertices[vertexIndex].position, 1.0f);
                glm::mat4 placement = partData.transform;
                if (!mesh.skin.empty() && !restSkinningMatrices.empty())
                {
                    const VertexSkin& skin = mesh.skin[vertexIndex];
                    placement = glm::mat4(0.0f);
                    for (int slot = 0; slot < 4; ++slot)
                        placement += restSkinningMatrices[skin.joints[slot]] * skin.weights[slot];
                }
                else if (partData.parentJoint.has_value())
                {
                    placement = model.restJointMatrices[*partData.parentJoint] * partData.transform;
                }
                restPositions[partIndex].push_back(glm::vec3(placement * position));
            }
        }

        // The box around the whole model, in its own coordinates: a model exported in centimeters, or far from its origin,
        // is easy to notice in the log (it would be invisible in the game: too big, too small, or somewhere else).
        glm::vec3 minimum(std::numeric_limits<float>::max());
        glm::vec3 maximum(std::numeric_limits<float>::lowest());
        for (const std::vector<glm::vec3>& positions : restPositions)
        {
            for (const glm::vec3& position : positions)
            {
                minimum = glm::min(minimum, position);
                maximum = glm::max(maximum, position);
            }
        }
        const glm::vec3 size = maximum - minimum;
        const glm::vec3 center = (minimum + maximum) * 0.5f;

        // Models from the internet often lie far from their origin (the shotgun was 50 m away from it), and an entity
        // places the origin of its model: such a model would stand far from the entity. So every model is moved to have
        // the center of its box at its origin; a map entity or a hand then places the middle of the model. A part held
        // by a joint is moved with the skeleton (skeletonTransform), not by its own transform, which is relative to the
        // joint.
        const glm::mat4 centering = glm::translate(glm::mat4(1.0f), -center);
        for (ModelPart& part : model.parts)
            if (!part.parentJoint.has_value())
                part.transform = centering * part.transform;
        model.skeletonTransform = centering;

        // The box of every part, in the centered coordinates. The parts follow data->parts one to one.
        for (std::size_t partIndex = 0; partIndex < model.parts.size(); ++partIndex)
        {
            glm::vec3 partMinimum(std::numeric_limits<float>::max());
            glm::vec3 partMaximum(std::numeric_limits<float>::lowest());
            for (const glm::vec3& position : restPositions[partIndex])
            {
                partMinimum = glm::min(partMinimum, position - center);
                partMaximum = glm::max(partMaximum, position - center);
            }

            // A part without vertices keeps an empty box at the middle of the model.
            if (partMinimum.x <= partMaximum.x)
            {
                model.parts[partIndex].center = (partMinimum + partMaximum) * 0.5f;
                model.parts[partIndex].size = partMaximum - partMinimum;
            }
        }

        // The front: the average of the vertices near the frontmost one (the end of a barrel), in centered coordinates.
        // The middle of the box would not do: the stock and the trigger guard pull it below the barrel.
        glm::vec3 frontSum(0.0f);
        int frontCount = 0;
        for (const std::vector<glm::vec3>& positions : restPositions)
        {
            for (const glm::vec3& position : positions)
            {
                if (position.z <= minimum.z + ModelFrontDepth)
                {
                    frontSum += position - center;
                    ++frontCount;
                }
            }
        }
        model.front = frontCount > 0 ? frontSum / static_cast<float>(frontCount) : glm::vec3(0.0f, 0.0f, -size.z * 0.5f);
        model.size = size;
        model.skeleton = std::move(data->skeleton);
        model.animations = std::move(data->animations);

        const auto usedImageCount =
            std::ranges::count_if(imageTextures, [](const auto& texture) { return texture.has_value(); });
        Core::Log::Write(LogCategory::Renderer, LogLevel::Debug,
                         "Model loaded: {} ({} parts, {} of {} images used, size {:.2f} x {:.2f} x {:.2f} m, "
                         "center moved from ({:.2f}, {:.2f}, {:.2f}))",
                         path, model.parts.size(), usedImageCount, imageTextures.size(), size.x, size.y, size.z, center.x,
                         center.y, center.z);

        return m_cache.Add(path, std::move(model), lifetime);
    }

    void ModelStore::RemoveAll(Core::AssetLifetime lifetime)
    {
        for (const std::string& path : m_cache.RemoveAll(lifetime))
            m_fallbackPaths.erase(path);
    }

    const Model& ModelStore::Get(ModelHandle handle) const
    {
        const Model* model = m_cache.Get(handle);

        return model != nullptr ? *model : m_fallbackModel;
    }

    const std::string* ModelStore::GetPath(ModelHandle handle) const
    {
        return m_cache.GetPath(handle);
    }

    void ModelStore::SetPartSplits(const std::string& path, std::vector<ModelPartSplit> splits)
    {
        if (m_cache.Find(path).has_value())
            Core::Log::Write(LogCategory::Renderer, LogLevel::Warning,
                             "Model {} is already loaded: its parts are split only when it is loaded again", path);

        m_partSplits[path] = std::move(splits);
    }

    std::size_t ModelStore::GetCount() const noexcept
    {
        return m_cache.GetCount();
    }
}
