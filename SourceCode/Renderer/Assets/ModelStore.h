#pragma once

#include "Core/Assets/AssetCache.h"
#include "Core/Assets/AssetHandle.h"
#include "Core/Assets/AssetLifetime.h"
#include "Renderer/Assets/MeshStore.h"
#include "Renderer/Assets/TextureStore.h"

#include <glm/mat4x4.hpp>

#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

namespace Abomination::Renderer
{
    // One part of a loaded model (see ModelPartData): its mesh and texture in the stores, and where it is in the model.
    struct ModelPart
    {
        std::string name;
        MeshHandle mesh;
        TextureHandle texture;
        glm::mat4 transform{1.0f};
    };

    // A model ready to be drawn: its parts. The meshes and textures themselves live in the mesh and texture stores, named
    // after the model file ("Models/Weapons/Shotgun.glb#Pump_low_Shotgun_0", "...#image0"), so they are listed in the
    // Assets window and removed with the lifetime group like every other mesh and texture.
    struct Model
    {
        std::vector<ModelPart> parts;

        // The size of the box around the whole model (meters). The model is centered on its origin (see ModelStore), so
        // the box goes from -size / 2 to size / 2. Used to scale a model to the size it must have in the game.
        glm::vec3 size{1.0f};

        // The middle of the front of the model (its -Z end): the average of the vertices within FrontDepth of its
        // frontmost point. For a weapon pointing forward it is the muzzle, where the flash appears.
        glm::vec3 front{0.0f};
    };

    // How deep the front of a model is taken (meters, see Model::front).
    inline constexpr float ModelFrontDepth = 0.01f;

    using ModelHandle = Core::AssetHandle<Model>;

    // Loads models from glTF files (see LoadGLTFFile) and keeps every model exactly once.
    //
    // A missing or broken file does not stop the game: the model becomes one part with invalid handles, which the mesh
    // and texture stores turn into their fallbacks, a magenta and black cube. A warning is logged once per file.
    //
    // Requires a current OpenGL context (the meshes and textures go to video memory). Move-only.
    class ModelStore
    {
    public:
        // assetsDirectory: the folder all model paths are relative to.
        explicit ModelStore(std::filesystem::path assetsDirectory);

        // Returns the model loaded from path, loading it on the first call: its meshes go to meshes and its textures to
        // textures, with the same lifetime. path is relative to the assets directory and uses forward slashes:
        // "Models/Weapons/Shotgun.glb".
        [[nodiscard]] ModelHandle Load(const std::string& path, Core::AssetLifetime lifetime, MeshStore& meshes,
                                       TextureStore& textures);

        // Removes every model of the lifetime group; their handles become invalid. Their meshes and textures are removed
        // by their own stores (see RenderAssets::RemoveAll).
        void RemoveAll(Core::AssetLifetime lifetime);

        // The model of the handle. An invalid handle gives the fallback model (one magenta and black cube).
        [[nodiscard]] const Model& Get(ModelHandle handle) const;

        // Calls visitor(path, model, isFallback, lifetime) for every loaded model. For the Assets window.
        template <typename Visitor>
        void VisitModels(Visitor&& visitor) const;

        // The path the model was loaded from, or nullptr for an invalid handle. For the entity inspector.
        [[nodiscard]] const std::string* GetPath(ModelHandle handle) const;

        [[nodiscard]] std::size_t GetCount() const noexcept;

    private:
        std::filesystem::path m_assetsDirectory;
        Core::AssetCache<Model> m_cache;

        // Paths whose file could not be loaded and which hold the fallback model instead.
        std::unordered_set<std::string> m_fallbackPaths;

        // Returned by Get() for invalid handles: one part with invalid handles.
        Model m_fallbackModel{.parts = {ModelPart{.name = "Fallback"}}};
    };

    template <typename Visitor>
    void ModelStore::VisitModels(Visitor&& visitor) const
    {
        m_cache.VisitAssets([&](const std::string& path, const Model& model, Core::AssetLifetime lifetime)
        {
            visitor(path, model, m_fallbackPaths.contains(path), lifetime);
        });
    }
}
