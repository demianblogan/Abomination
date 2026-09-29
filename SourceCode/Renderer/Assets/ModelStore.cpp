#include "Renderer/Assets/ModelStore.h"

#include "Core/Logging/Log.h"
#include "Renderer/Assets/GLTFLoader.h"

#include <algorithm>
#include <expected>
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
            m_cache.ExtendLifetime(*loadedHandle, lifetime);

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

        const auto usedImageCount = std::ranges::count_if(imageTextures, [](const auto& texture) { return texture.has_value(); });
        Core::Log::Write(LogCategory::Renderer, LogLevel::Debug, "Model loaded: {} ({} parts, {} of {} images used)", path,
                         model.parts.size(), usedImageCount, imageTextures.size());

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
