#pragma once

#include "Core/Assets/AssetCache.h"
#include "Core/Assets/AssetHandle.h"
#include "Core/Assets/AssetLifetime.h"
#include "Renderer/Assets/MeshStore.h"
#include "Renderer/Assets/ModelPartSplit.h"
#include "Renderer/Assets/TextureStore.h"

#include <glm/mat4x4.hpp>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
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

        // The box around the part in the model (meters, in the coordinates of the centered model, see Model::size): its
        // middle and its size. To find a place on the model by a part, like the window of a shotgun that shells fly out of.
        glm::vec3 center{0.0f};
        glm::vec3 size{0.0f};

        // A part held by a joint (see ModelPartData::parentJoint): transform then places it relative to that joint, and
        // the part is drawn at Model::skeletonTransform times the joint's matrix times transform. A skinned part has a
        // skin in its mesh, and transform places the skinned vertices in the model.
        std::optional<std::size_t> parentJoint;
        bool isSkinned = false;
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

        // The skeleton and its clips, for a model that has them (see ModelData). skeletonTransform moves the joints'
        // matrices into the centered model (the centering above); restJointMatrices is the skeleton at rest, drawn when
        // nothing animates the model (see Renderer::ModelPose).
        std::optional<SkeletonData> skeleton;
        std::vector<AnimationClipData> animations;
        glm::mat4 skeletonTransform{1.0f};
        std::vector<glm::mat4> restJointMatrices;
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
        // "Models/Weapons/Shotgun.glb". The splits set for the path (see SetPartSplits) are made before the meshes are.
        [[nodiscard]] ModelHandle Load(const std::string& path, Core::AssetLifetime lifetime, MeshStore& meshes,
                                       TextureStore& textures);

        // Sets the pieces to take out of parts of the model at path into parts of their own (see ModelPartSplit), whoever
        // loads it. Must be called before the model is loaded: a loaded model keeps its parts.
        void SetPartSplits(const std::string& path, std::vector<ModelPartSplit> splits);

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

        // The pieces to take out of the parts of a model when it is loaded, by path (see SetPartSplits).
        std::unordered_map<std::string, std::vector<ModelPartSplit>> m_partSplits;

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
