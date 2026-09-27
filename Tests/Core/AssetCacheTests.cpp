#include "Core/AssetCache.h"

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Abomination::Core
{
    // Strings stand in for real assets: the cache does not care what it stores.
    class AssetCacheTest : public ::testing::Test
    {
    protected:
        AssetCache<std::string> m_cache;
    };

    TEST_F(AssetCacheTest, AddedAssetCanBeReadByHandle)
    {
        const AssetHandle<std::string> handle = m_cache.Add("Textures/Crate.png", "crate");

        ASSERT_NE(m_cache.Get(handle), nullptr);
        EXPECT_EQ(*m_cache.Get(handle), "crate");
        EXPECT_EQ(m_cache.GetCount(), 1u);
    }

    TEST_F(AssetCacheTest, FindReturnsHandleOfLoadedPath)
    {
        const AssetHandle<std::string> handle = m_cache.Add("Textures/Crate.png", "crate");

        const std::optional<AssetHandle<std::string>> foundHandle = m_cache.Find("Textures/Crate.png");

        ASSERT_TRUE(foundHandle.has_value());
        EXPECT_EQ(*foundHandle, handle);
        EXPECT_FALSE(m_cache.Find("Textures/Missing.png").has_value());
    }

    TEST_F(AssetCacheTest, DefaultHandleIsInvalid)
    {
        m_cache.Add("Textures/Crate.png", "crate");

        const AssetHandle<std::string> handle;

        EXPECT_FALSE(m_cache.IsValid(handle));
        EXPECT_EQ(m_cache.Get(handle), nullptr);
    }

    TEST_F(AssetCacheTest, RemovedAssetMakesHandleInvalid)
    {
        const AssetHandle<std::string> handle = m_cache.Add("Textures/Crate.png", "crate");

        m_cache.Remove(handle);

        EXPECT_FALSE(m_cache.IsValid(handle));
        EXPECT_EQ(m_cache.Get(handle), nullptr);
        EXPECT_FALSE(m_cache.Find("Textures/Crate.png").has_value());
        EXPECT_EQ(m_cache.GetCount(), 0u);
    }

    TEST_F(AssetCacheTest, ReusedSlotDoesNotRespondToOldHandle)
    {
        const AssetHandle<std::string> oldHandle = m_cache.Add("Textures/Crate.png", "crate");
        m_cache.Remove(oldHandle);

        const AssetHandle<std::string> newHandle = m_cache.Add("Textures/Wall.png", "wall");

        // The free slot is reused, but with a new generation: the old handle does not give the new asset.
        EXPECT_EQ(newHandle.index, oldHandle.index);
        EXPECT_NE(newHandle.generation, oldHandle.generation);
        EXPECT_EQ(m_cache.Get(oldHandle), nullptr);
        EXPECT_EQ(*m_cache.Get(newHandle), "wall");
    }

    TEST_F(AssetCacheTest, AddingSamePathReplacesAssetAndKeepsHandle)
    {
        const AssetHandle<std::string> handle = m_cache.Add("Textures/Crate.png", "old crate");

        const AssetHandle<std::string> sameHandle = m_cache.Add("Textures/Crate.png", "new crate");

        EXPECT_EQ(sameHandle, handle);
        EXPECT_EQ(*m_cache.Get(handle), "new crate");
        EXPECT_EQ(m_cache.GetCount(), 1u);
    }

    TEST_F(AssetCacheTest, RemovingTwiceDoesNothing)
    {
        const AssetHandle<std::string> handle = m_cache.Add("Textures/Crate.png", "crate");
        const AssetHandle<std::string> otherHandle = m_cache.Add("Textures/Wall.png", "wall");

        m_cache.Remove(handle);
        m_cache.Remove(handle);

        EXPECT_EQ(m_cache.GetCount(), 1u);
        EXPECT_EQ(*m_cache.Get(otherHandle), "wall");
    }

    TEST_F(AssetCacheTest, VisitAssetsSkipsRemovedAssets)
    {
        m_cache.Add("Textures/Crate.png", "crate");
        const AssetHandle<std::string> wallHandle = m_cache.Add("Textures/Wall.png", "wall");
        m_cache.Add("Textures/Door.png", "door", AssetLifetime::Level);
        m_cache.Remove(wallHandle);

        std::string visited;
        m_cache.VisitAssets([&](const std::string& path, const std::string& asset, AssetLifetime lifetime)
        {
            visited += path + "=" + asset + (lifetime == AssetLifetime::Level ? " (level)" : "") + ";";
        });

        EXPECT_EQ(visited, "Textures/Crate.png=crate;Textures/Door.png=door (level);");
    }

    TEST_F(AssetCacheTest, GetPathGivesPathOfValidHandleOnly)
    {
        const AssetHandle<std::string> handle = m_cache.Add("Textures/Crate.png", "crate");

        ASSERT_NE(m_cache.GetPath(handle), nullptr);
        EXPECT_EQ(*m_cache.GetPath(handle), "Textures/Crate.png");

        m_cache.Remove(handle);
        EXPECT_EQ(m_cache.GetPath(handle), nullptr);
    }

    TEST(AssetCache, StoresMoveOnlyAssets)
    {
        // GPU resources (GLTexture, GLShaderProgram) can be moved but not copied; the cache must accept them.
        AssetCache<std::unique_ptr<int>> cache;

        const AssetHandle<std::unique_ptr<int>> handle = cache.Add("Numbers/FortyTwo", std::make_unique<int>(42));

        ASSERT_NE(cache.Get(handle), nullptr);
        EXPECT_EQ(**cache.Get(handle), 42);
    }

    TEST_F(AssetCacheTest, AssetsAreGlobalByDefault)
    {
        // Removing the Level group does not touch an asset added without a lifetime.
        const AssetHandle<std::string> handle = m_cache.Add("Shaders/Wireframe", "wireframe");

        m_cache.RemoveAll(AssetLifetime::Level);

        EXPECT_TRUE(m_cache.IsValid(handle));
    }

    TEST_F(AssetCacheTest, RemoveAllRemovesOnlyThatLifetime)
    {
        const AssetHandle<std::string> global = m_cache.Add("Shaders/Wireframe", "wireframe");
        const AssetHandle<std::string> wall = m_cache.Add("Textures/Wall.png", "wall", AssetLifetime::Level);
        const AssetHandle<std::string> floor = m_cache.Add("Textures/Floor.png", "floor", AssetLifetime::Level);

        const std::vector<std::string> removedPaths = m_cache.RemoveAll(AssetLifetime::Level);

        EXPECT_EQ(removedPaths, (std::vector<std::string>{"Textures/Wall.png", "Textures/Floor.png"}));
        EXPECT_FALSE(m_cache.IsValid(wall));
        EXPECT_FALSE(m_cache.IsValid(floor));
        EXPECT_FALSE(m_cache.Find("Textures/Wall.png").has_value());
        EXPECT_TRUE(m_cache.IsValid(global));
        EXPECT_EQ(m_cache.GetCount(), 1u);
    }

    TEST_F(AssetCacheTest, SlotsOfRemovedLevelAreReusedWithNewGeneration)
    {
        // The next level loads its assets into the slots the previous one left; handles of the old level stay invalid.
        const AssetHandle<std::string> oldWall = m_cache.Add("Textures/Wall.png", "wall", AssetLifetime::Level);
        m_cache.RemoveAll(AssetLifetime::Level);

        const AssetHandle<std::string> newWall = m_cache.Add("Textures/Wall.png", "new wall", AssetLifetime::Level);

        EXPECT_EQ(newWall.index, oldWall.index);
        EXPECT_NE(newWall.generation, oldWall.generation);
        EXPECT_FALSE(m_cache.IsValid(oldWall));
        ASSERT_NE(m_cache.Get(newWall), nullptr);
        EXPECT_EQ(*m_cache.Get(newWall), "new wall");
    }

    TEST_F(AssetCacheTest, LongerLifetimeWins)
    {
        // A level texture the whole game asks for too must survive the level.
        const AssetHandle<std::string> handle = m_cache.Add("Textures/Wall.png", "wall", AssetLifetime::Level);
        m_cache.ExtendLifetime(handle, AssetLifetime::Global);

        // A shorter lifetime never shortens it: neither when extending nor when the asset is added again. So removing
        // the Level group leaves it.
        m_cache.ExtendLifetime(handle, AssetLifetime::Level);
        m_cache.Add("Textures/Wall.png", "reloaded wall", AssetLifetime::Level);

        m_cache.RemoveAll(AssetLifetime::Level);
        EXPECT_TRUE(m_cache.IsValid(handle));
    }
}
