#pragma once

#include "Core/Assets/AssetCache.h"
#include "Core/Assets/AssetHandle.h"
#include "Core/Assets/AssetLifetime.h"
#include "Renderer/Assets/Mesh.h"
#include "Renderer/Assets/MeshData.h"

#include <cstddef>
#include <string>

namespace Abomination::Renderer
{
    using MeshHandle = Core::AssetHandle<Mesh>;

    // Keeps every mesh exactly once and hands out handles to them. Meshes are added from geometry built in memory and
    // named like asset paths: the parts of a level ("Maps/Test.map#Episode1/Wall_MossyBrick"), shapes of MeshPrimitives
    // ("Primitives/Cube"). Loading meshes from model files comes in 0.3.
    //
    // Get() of an invalid handle gives a fallback cube, so drawing code never has to check for nullptr: with the
    // fallback texture on it, a broken mesh reference shows up as a magenta and black cube.
    //
    // Requires a current OpenGL context. Move-only.
    class MeshStore
    {
    public:
        MeshStore();

        // Stores a mesh made of data under name for the lifetime and returns its handle. The same name again replaces the
        // mesh and keeps the handle. Empty data (no triangles) cannot be drawn: the fallback cube is stored instead, with
        // a warning.
        MeshHandle Add(const std::string& name, const MeshData& data, Core::AssetLifetime lifetime);

        // Removes every mesh of the lifetime group from video memory; their handles become invalid.
        void RemoveAll(Core::AssetLifetime lifetime);

        // The mesh of the handle. An invalid handle gives the fallback cube.
        [[nodiscard]] const Mesh& Get(MeshHandle handle) const;

        // Calls visitor(name, mesh, lifetime) for every stored mesh. For the Assets window of the debug overlay.
        template <typename Visitor>
        void VisitMeshes(Visitor&& visitor) const;

        // The name the mesh was added with, or nullptr for an invalid handle. For the entity inspector.
        [[nodiscard]] const std::string* GetName(MeshHandle handle) const;

        [[nodiscard]] std::size_t GetCount() const noexcept;

    private:
        Core::AssetCache<Mesh> m_cache;

        // Returned by Get() for invalid handles.
        Mesh m_fallbackMesh;
    };

    template <typename Visitor>
    void MeshStore::VisitMeshes(Visitor&& visitor) const
    {
        m_cache.VisitAssets(visitor);
    }
}
