#include "Renderer/Assets/MeshStore.h"

#include "Core/Logging/Log.h"
#include "Renderer/Assets/MeshPrimitives.h"

namespace Abomination::Renderer
{
    using Core::LogCategory;
    using Core::LogLevel;

    MeshStore::MeshStore()
        : m_fallbackMesh(Mesh::Create(CreateCubeMeshData()))
    {}

    MeshHandle MeshStore::Add(const std::string& name, const MeshData& data, Core::AssetLifetime lifetime)
    {
        // An empty buffer cannot be created in OpenGL (glNamedBufferStorage needs a size above 0), and there would be
        // nothing to draw anyway.
        if (data.vertices.empty() || data.indices.size() < 3)
        {
            Core::Log::Write(LogCategory::Renderer, LogLevel::Warning, "Mesh {} has no triangles, replaced by the fallback",
                             name);

            return m_cache.Add(name, Mesh::Create(CreateCubeMeshData()), lifetime);
        }

        Core::Log::Write(LogCategory::Renderer, LogLevel::Debug, "Mesh added: {} ({} vertices, {} indices)", name,
                         data.vertices.size(), data.indices.size());

        return m_cache.Add(name, Mesh::Create(data), lifetime);
    }

    void MeshStore::ExtendLifetime(MeshHandle handle, Core::AssetLifetime lifetime)
    {
        m_cache.ExtendLifetime(handle, lifetime);
    }

    void MeshStore::RemoveAll(Core::AssetLifetime lifetime)
    {
        // The returned names are not needed: unlike the texture store, this one remembers nothing else about its meshes.
        m_cache.RemoveAll(lifetime);
    }

    const Mesh& MeshStore::Get(MeshHandle handle) const
    {
        const Mesh* mesh = m_cache.Get(handle);
        if (mesh == nullptr)
            return m_fallbackMesh;

        return *mesh;
    }

    std::size_t MeshStore::GetCount() const noexcept
    {
        return m_cache.GetCount();
    }

    const std::string* MeshStore::GetName(MeshHandle handle) const
    {
        return m_cache.GetPath(handle);
    }
}
