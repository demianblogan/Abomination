#pragma once

#include "Core/AssetHandle.h"
#include "Core/AssetLifetime.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Abomination::Core
{
    // Stores assets of one type, each exactly once, and hands out AssetHandle<Asset> to them.
    // It only stores: loading from files is done by the store of each asset type (textures in Renderer, ...), which
    // first asks Find() whether the path is already loaded and only otherwise loads the file and calls Add().
    //
    // The assets live in a vector of slots. A removed asset leaves a free slot, which the next Add() reuses,
    // so the vector does not grow when a level unloads its assets and the next level loads new ones.
    //
    // Asset can be move-only (GLTexture, GLShaderProgram). A template, so the whole class is defined in this header.
    template <typename Asset>
    class AssetCache
    {
    public:
        // Returns the handle of the asset loaded from this path, or nothing if it is not in the cache.
        [[nodiscard]] std::optional<AssetHandle<Asset>> Find(std::string_view path) const;

        // Stores an asset loaded from path in the lifetime group and returns its handle. If an asset with this path is
        // already stored, it is replaced and its handle stays the same (everyone holding the handle gets the new asset);
        // it keeps the longer of its old and the new lifetime.
        AssetHandle<Asset> Add(std::string path, Asset asset, AssetLifetime lifetime = AssetLifetime::Global);

        // Moves the asset to the longer of its lifetime and the given one: a level texture that the whole game asks for
        // becomes global. A shorter lifetime never shortens it. Does nothing for an invalid handle.
        void ExtendLifetime(AssetHandle<Asset> handle, AssetLifetime lifetime);

        // Removes the asset; the handle and all its copies become invalid. Does nothing for an invalid handle.
        void Remove(AssetHandle<Asset> handle);

        // Removes every asset of the lifetime group, like Remove() for each of them, and returns their paths (the stores
        // forget what they remember about these paths).
        std::vector<std::string> RemoveAll(AssetLifetime lifetime);

        // The asset, or nullptr if the handle is invalid: default-constructed, or its asset was removed.
        // The pointer is valid until the next Add() or Remove(): the vector of slots may move its elements.
        [[nodiscard]] Asset* Get(AssetHandle<Asset> handle);
        [[nodiscard]] const Asset* Get(AssetHandle<Asset> handle) const;

        [[nodiscard]] bool IsValid(AssetHandle<Asset> handle) const;

        // The path the asset was added with, or nullptr if the handle is invalid. For tools that show which asset a
        // component refers to. Valid until the next Add() or Remove(), like Get().
        [[nodiscard]] const std::string* GetPath(AssetHandle<Asset> handle) const;

        // How many assets are stored.
        [[nodiscard]] std::size_t GetCount() const noexcept;

        // Calls visitor(path, asset, lifetime) for every stored asset, in slot order. For tools that list the assets
        // (the Assets window of the debug overlay). The visitor must not add or remove assets.
        template <typename Visitor>
        void VisitAssets(Visitor&& visitor) const;

    private:
        struct Slot
        {
            // Empty while the slot is free.
            std::optional<Asset> asset;
            std::string path;
            std::uint32_t generation = 1;
            AssetLifetime lifetime = AssetLifetime::Global;
        };

        // Lets m_handlesByPath find a std::string key by a std::string_view ("heterogeneous lookup", C++20).
        //
        // The problem: by default unordered_map<std::string, ...>::find() accepts only const std::string&.
        // Find(std::string_view) would then have to create a temporary std::string just to search, which copies
        // the characters and, for paths longer than 15 characters ("Textures/Crate.png"), allocates heap memory.
        //
        // The solution: unordered_map has a second find() that accepts any type, but the standard library enables it
        // only if both the hash and the equality function contain a type named is_transparent. The type itself is
        // never used (void by tradition); it is a marker the compiler checks for with "requires", like a flag that
        // says "this hash accepts other types than the key type too". The promise behind the marker is ours to keep:
        //   - operator() takes std::string_view, and a std::string converts to it implicitly, so one function hashes
        //     both the stored keys and the searched paths;
        //   - equal text gives an equal hash, whether it comes as std::string or std::string_view, because both are
        //     hashed as std::string_view. Otherwise find() would look into the wrong bucket.
        // The equality function is std::equal_to<> (with empty <>): the standard version that is already transparent,
        // comparing any two types with ==, and std::string == std::string_view works.
        //
        // The gain is small here (Find() runs once per loaded file, not every frame), but it costs nothing to use.
        struct PathHash
        {
            using is_transparent = void;

            std::size_t operator()(std::string_view path) const noexcept
            {
                return std::hash<std::string_view>{}(path);
            }
        };

        std::vector<Slot> m_slots;

        // Indices of free slots, reused by Add() before the vector grows.
        std::vector<std::uint32_t> m_freeSlotIndices;

        // Path -> handle, so Find() does not have to search through all slots.
        // The last two parameters are the hash and the equality function; see PathHash for why they are not the defaults.
        std::unordered_map<std::string, AssetHandle<Asset>, PathHash, std::equal_to<>> m_handlesByPath;
    };

    template <typename Asset>
    std::optional<AssetHandle<Asset>> AssetCache<Asset>::Find(std::string_view path) const
    {
        const auto iterator = m_handlesByPath.find(path);
        if (iterator == m_handlesByPath.end())
            return std::nullopt;

        return iterator->second;
    }

    template <typename Asset>
    AssetHandle<Asset> AssetCache<Asset>::Add(std::string path, Asset asset, AssetLifetime lifetime)
    {
        // The same path again: replace the asset in its slot, the handle does not change.
        if (const std::optional<AssetHandle<Asset>> existingHandle = Find(path); existingHandle.has_value())
        {
            Slot& slot = m_slots[existingHandle->index];
            slot.asset = std::move(asset);
            slot.lifetime = GetLongerLifetime(slot.lifetime, lifetime);

            return *existingHandle;
        }

        // Reuse a free slot if there is one, otherwise add a new slot at the end.
        std::uint32_t index = 0;
        if (!m_freeSlotIndices.empty())
        {
            index = m_freeSlotIndices.back();
            m_freeSlotIndices.pop_back();
        }
        else
        {
            index = static_cast<std::uint32_t>(m_slots.size());
            m_slots.emplace_back();
        }

        Slot& slot = m_slots[index];
        slot.asset = std::move(asset);
        slot.path = path;
        slot.lifetime = lifetime;

        const AssetHandle<Asset> handle{.index = index, .generation = slot.generation};
        m_handlesByPath.emplace(std::move(path), handle);

        return handle;
    }

    template <typename Asset>
    void AssetCache<Asset>::Remove(AssetHandle<Asset> handle)
    {
        if (!IsValid(handle))
            return;

        Slot& slot = m_slots[handle.index];
        m_handlesByPath.erase(slot.path);
        slot.asset.reset();
        slot.path.clear();

        // Every handle to the removed asset now has an old generation and is invalid. After 4 billion reuses of the
        // same slot the counter would wrap around to 0 and then to old values; a game never gets anywhere near that.
        ++slot.generation;
        m_freeSlotIndices.push_back(handle.index);
    }

    template <typename Asset>
    void AssetCache<Asset>::ExtendLifetime(AssetHandle<Asset> handle, AssetLifetime lifetime)
    {
        if (!IsValid(handle))
            return;

        Slot& slot = m_slots[handle.index];
        slot.lifetime = GetLongerLifetime(slot.lifetime, lifetime);
    }

    template <typename Asset>
    std::vector<std::string> AssetCache<Asset>::RemoveAll(AssetLifetime lifetime)
    {
        std::vector<std::string> removedPaths;
        for (std::uint32_t index = 0; index < m_slots.size(); ++index)
        {
            const Slot& slot = m_slots[index];
            if (!slot.asset.has_value() || slot.lifetime != lifetime)
                continue;

            // Remove() clears the path, so it is copied first. Remove() does not add or erase slots (it only frees this
            // one), so going on through the vector by index stays correct.
            removedPaths.push_back(slot.path);
            Remove(AssetHandle<Asset>{.index = index, .generation = slot.generation});
        }

        return removedPaths;
    }

    template <typename Asset>
    Asset* AssetCache<Asset>::Get(AssetHandle<Asset> handle)
    {
        if (!IsValid(handle))
            return nullptr;

        return &*m_slots[handle.index].asset;
    }

    template <typename Asset>
    const Asset* AssetCache<Asset>::Get(AssetHandle<Asset> handle) const
    {
        if (!IsValid(handle))
            return nullptr;

        return &*m_slots[handle.index].asset;
    }

    template <typename Asset>
    bool AssetCache<Asset>::IsValid(AssetHandle<Asset> handle) const
    {
        // The index must point to an existing slot, and the slot must still hold the same generation of asset.
        if (handle.index >= m_slots.size())
            return false;

        const Slot& slot = m_slots[handle.index];

        return slot.generation == handle.generation && slot.asset.has_value();
    }

    template <typename Asset>
    const std::string* AssetCache<Asset>::GetPath(AssetHandle<Asset> handle) const
    {
        if (!IsValid(handle))
            return nullptr;

        return &m_slots[handle.index].path;
    }

    template <typename Asset>
    std::size_t AssetCache<Asset>::GetCount() const noexcept
    {
        return m_handlesByPath.size();
    }

    template <typename Asset>
    template <typename Visitor>
    void AssetCache<Asset>::VisitAssets(Visitor&& visitor) const
    {
        // Free slots are skipped: they hold no asset.
        for (const Slot& slot : m_slots)
            if (slot.asset.has_value())
                visitor(slot.path, *slot.asset, slot.lifetime);
    }
}
