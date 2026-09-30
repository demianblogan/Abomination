#include "Renderer/Assets/ModelStore.h"

#include "Core/Logging/Log.h"
#include "Renderer/Assets/GLTFLoader.h"

#include <glm/common.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <expected>
#include <limits>
#include <format>
#include <optional>
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
                textures.ExtendLifetime(part.texture, lifetime);
            }

            return *loadedHandle;
        }

        std::filesystem::path fullPath = m_assetsDirectory / path;
        fullPath.make_preferred();

        const std::expected<ModelData, std::string> data = LoadGLTFFile(fullPath);
        if (!data.has_value())
        {
            Core::Log::Write(LogCategory::Renderer, LogLevel::Warning, "Model {} replaced by the fallback: {}", path,
                             data.error());
            m_fallbackPaths.insert(path);

            return m_cache.Add(path, m_fallbackModel, lifetime);
        }

        // Only the images the parts use become textures, named after the model. A model file often holds more (normal and
        // metalness maps, which the game does not use yet), and each of them would take video memory for nothing.
        std::vector<std::optional<TextureHandle>> imageTextures(data->images.size());
        const auto getImageTexture = [&](std::size_t index)
        {
            if (!imageTextures[index].has_value())
                imageTextures[index] = textures.Add(std::format("{}#image{}", path, index), data->images[index], lifetime);

            return *imageTextures[index];
        };

        Model model;
        for (const ModelPartData& partData : data->parts)
        {
            model.parts.push_back(ModelPart{
                .name = partData.name,
                .mesh = meshes.Add(std::format("{}#{}", path, partData.name), partData.mesh, lifetime),
                .texture = partData.imageIndex.has_value() ? getImageTexture(*partData.imageIndex) : TextureHandle{},
                .transform = partData.transform,
            });
        }

        // The box around the whole model, in its own coordinates: a model exported in centimeters, or far from its origin,
        // is easy to notice in the log (it would be invisible in the game: too big, too small, or somewhere else).
        glm::vec3 minimum(std::numeric_limits<float>::max());
        glm::vec3 maximum(std::numeric_limits<float>::lowest());
        for (const ModelPartData& partData : data->parts)
        {
            for (const MeshVertex& vertex : partData.mesh.vertices)
            {
                const glm::vec3 position(partData.transform * glm::vec4(vertex.position, 1.0f));
                minimum = glm::min(minimum, position);
                maximum = glm::max(maximum, position);
            }
        }
        const glm::vec3 size = maximum - minimum;
        const glm::vec3 center = (minimum + maximum) * 0.5f;

        // Models from the internet often lie far from their origin (the shotgun was 50 m away from it), and an entity
        // places the origin of its model: such a model would stand far from the entity. So every model is moved to have
        // the center of its box at its origin; a map entity or a hand then places the middle of the model.
        const glm::mat4 centering = glm::translate(glm::mat4(1.0f), -center);
        for (ModelPart& part : model.parts)
            part.transform = centering * part.transform;

        // The front: the average of the vertices near the frontmost one (the end of a barrel), in centered coordinates.
        // The middle of the box would not do: the stock and the trigger guard pull it below the barrel.
        glm::vec3 frontSum(0.0f);
        int frontCount = 0;
        for (const ModelPartData& partData : data->parts)
        {
            for (const MeshVertex& vertex : partData.mesh.vertices)
            {
                const glm::vec3 position(partData.transform * glm::vec4(vertex.position, 1.0f));
                if (position.z <= minimum.z + ModelFrontDepth)
                {
                    frontSum += position - center;
                    ++frontCount;
                }
            }
        }
        model.front = frontCount > 0 ? frontSum / static_cast<float>(frontCount) : glm::vec3(0.0f, 0.0f, -size.z * 0.5f);
        model.size = size;

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

    std::size_t ModelStore::GetCount() const noexcept
    {
        return m_cache.GetCount();
    }
}
