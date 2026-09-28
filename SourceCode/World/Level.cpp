#include "World/Level.h"

#include "Core/Logging/Log.h"
#include "Core/Scene/Name.h"
#include "Core/Scene/Transform.h"
#include "Renderer/MeshRenderer.h"
#include "World/MapCoordinates.h"

#include <charconv>
#include <optional>
#include <system_error>

namespace Abomination::World
{
    using Core::LogCategory;
    using Core::LogLevel;

    namespace
    {
        // The texture of a face is a file in Assets/Textures, named in the map by its path there without the extension:
        // "Episode1/Wall_MossyBrick" -> "Textures/Episode1/Wall_MossyBrick.png".
        std::string MakeTexturePath(const std::string& textureName)
        {
            return "Textures/" + textureName + ".png";
        }

        const MapEntity* FindEntity(const MapData& map, const std::string& className)
        {
            for (const MapEntity& entity : map.entities)
                if (const std::string* entityClass = FindProperty(entity, "classname"); entityClass != nullptr)
                    if (*entityClass == className)
                        return &entity;

            return nullptr;
        }

        PlayerStart ReadPlayerStart(const MapData& map)
        {
            PlayerStart playerStart;

            const MapEntity* entity = FindEntity(map, "info_player_start");
            if (entity == nullptr)
            {
                Core::Log::Write(LogCategory::World, LogLevel::Warning, "The map has no info_player_start");

                return playerStart;
            }

            if (const std::string* origin = FindProperty(*entity, "origin"); origin != nullptr)
            {
                // The map stores the origin of the entity; the player is placed by the center of their box, which is a
                // little higher (see PlayerStart.h). Z is up in the map, so the offset goes along map Z.
                if (const std::optional<glm::dvec3> position = ParseVectorProperty(*origin); position.has_value())
                    playerStart.boxCenter = ConvertMapPosition(*position + glm::dvec3(0.0, 0.0, PlayerBoxCenterAboveOrigin));
            }

            if (const std::string* angle = FindProperty(*entity, "angle"); angle != nullptr)
            {
                double degrees = 0.0;
                const char* end = angle->data() + angle->size();
                if (std::from_chars(angle->data(), end, degrees).ec == std::errc())
                    playerStart.yaw = ConvertMapAngleToYaw(degrees);
            }

            return playerStart;
        }
    }

    Level Level::Create(entt::registry& registry, Renderer::RenderAssets& assets, const MapData& map,
                        const std::string& mapPath)
    {
        Level level;
        level.m_playerStart = ReadPlayerStart(map);

        const MapEntity* world = FindEntity(map, "worldspawn");
        if (world == nullptr)
        {
            Core::Log::Write(LogCategory::World, LogLevel::Error, "The map {} has no worldspawn entity", mapPath);

            return level;
        }

        // Texture coordinates need the size of every texture, so the textures of the level are loaded here, into the Level
        // lifetime group like the meshes below: Unload() removes them all at once. A missing texture gets the
        // checkerboard fallback of the store (and a warning in the log), with the size of the fallback.
        const TextureSizeLookup getTextureSize = [&assets](const std::string& textureName)
        {
            const Renderer::TextureHandle handle =
                assets.textures.Load(MakeTexturePath(textureName), Core::AssetLifetime::Level);
            const Renderer::GLTexture& texture = assets.textures.Get(handle);
            return glm::ivec2(texture.GetWidth(), texture.GetHeight());
        };
        LevelMesh levelMesh = BuildLevelMesh(*world, getTextureSize);
        level.m_statistics = levelMesh.statistics;
        level.m_collisionBrushes = BuildCollisionBrushes(*world);

        // One entity per texture: every one is one draw call with its own texture. The textures were loaded above, so
        // Load() only returns their handles now.
        const Renderer::ShaderHandle shaderProgram = assets.shaders.Load("Shaders/TexturedShaded");
        for (const LevelMeshPart& part : levelMesh.parts)
        {
            const entt::entity entity = registry.create();
            registry.emplace<Core::Name>(entity, "World geometry: " + part.textureName);
            registry.emplace<Core::Transform>(entity);
            registry.emplace<Renderer::MeshRenderer>(entity, Renderer::MeshRenderer{
                .mesh = assets.meshes.Add(mapPath + "#" + part.textureName, part.data, Core::AssetLifetime::Level),
                .texture = assets.textures.Load(MakeTexturePath(part.textureName), Core::AssetLifetime::Level),
                .shaderProgram = shaderProgram,
            });
            level.m_geometryEntities.push_back(entity);
        }

        Core::Log::Write(LogCategory::World, LogLevel::Info,
                         "Level {} loaded: {} brushes, {} faces, {} triangles, {} textures, {} collision brushes", mapPath,
                         level.m_statistics.brushCount, level.m_statistics.faceCount,
                         level.m_statistics.triangleCount, levelMesh.parts.size(), level.m_collisionBrushes.size());

        return level;
    }

    void Level::Unload(entt::registry& registry, Renderer::RenderAssets& assets)
    {
        // The entities first: their components hold handles to the assets removed below. A removed handle would only
        // draw the fallback, but nothing should be left referring to a level that is gone.
        for (const entt::entity entity : m_geometryEntities)
            if (registry.valid(entity))
                registry.destroy(entity);

        assets.RemoveAll(Core::AssetLifetime::Level);
        *this = Level{};

        Core::Log::Write(LogCategory::World, LogLevel::Info, "Level unloaded");
    }

    const PlayerStart& Level::GetPlayerStart() const noexcept
    {
        return m_playerStart;
    }

    const LevelMeshStatistics& Level::GetStatistics() const noexcept
    {
        return m_statistics;
    }

    const std::vector<CollisionBrush>& Level::GetCollisionBrushes() const noexcept
    {
        return m_collisionBrushes;
    }
}
