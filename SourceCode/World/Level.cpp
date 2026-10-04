#include "World/Level.h"

#include "Core/Logging/Log.h"
#include "Core/Scene/Name.h"
#include "Core/Scene/Transform.h"
#include "Renderer/MeshRenderer.h"
#include "Renderer/ModelRenderer.h"
#include "World/MapCoordinates.h"

#include <glm/gtc/quaternion.hpp>

#include <charconv>
#include <optional>

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

        // The yaw of the "angle" property of a point entity (0 if it has none or it is not a number).
        float ReadEntityYaw(const MapEntity& entity)
        {
            double degrees = 0.0;
            if (const std::string* angle = FindProperty(entity, "angle"); angle != nullptr)
                std::from_chars(angle->data(), angle->data() + angle->size(), degrees);

            return ConvertMapAngleToYaw(degrees);
        }

        // Where a character with the box of the player appears: the center of its box and its yaw, from the origin and
        // the angle of its map entity.
        PlayerStart ReadCharacterStart(const MapEntity& entity)
        {
            PlayerStart start;
            if (const std::string* origin = FindProperty(entity, "origin"); origin != nullptr)
            {
                // The map stores the origin of the entity; the character is placed by the center of its box, which is a
                // little higher (see PlayerStart.h). Z is up in the map, so the offset goes along map Z.
                if (const std::optional<glm::dvec3> position = ParseVectorProperty(*origin); position.has_value())
                    start.boxCenter = ConvertMapPosition(*position + glm::dvec3(0.0, 0.0, PlayerBoxCenterAboveOrigin));
            }

            start.yaw = ReadEntityYaw(entity);

            return start;
        }

        PlayerStart ReadPlayerStart(const MapData& map)
        {
            const MapEntity* entity = FindEntity(map, "info_player_start");
            if (entity == nullptr)
            {
                Core::Log::Write(LogCategory::World, LogLevel::Warning, "The map has no info_player_start");

                return PlayerStart{};
            }

            return ReadCharacterStart(*entity);
        }

        // A model standing in the level (misc_model): its file ("model"), origin and angle.
        entt::entity CreateModelEntity(entt::registry& registry, Renderer::RenderAssets& assets, const MapEntity& mapEntity,
                                       Renderer::ShaderHandle shaderProgram)
        {
            const std::string* modelPath = FindProperty(mapEntity, "model");
            if (modelPath == nullptr)
            {
                Core::Log::Write(LogCategory::World, LogLevel::Warning, "A misc_model has no model property, skipped");

                return entt::null;
            }

            Core::Transform transform;
            if (const std::string* origin = FindProperty(mapEntity, "origin"); origin != nullptr)
                if (const std::optional<glm::dvec3> position = ParseVectorProperty(*origin); position.has_value())
                    transform.position = ConvertMapPosition(*position);

            // The angle turns the model around the vertical axis (+Y in the game).
            transform.rotation = glm::angleAxis(ReadEntityYaw(mapEntity), glm::vec3(0.0f, 1.0f, 0.0f));

            const entt::entity entity = registry.create();
            registry.emplace<Core::Name>(entity, "Model: " + *modelPath);
            registry.emplace<Core::Transform>(entity, transform);
            registry.emplace<Renderer::ModelRenderer>(entity, Renderer::ModelRenderer{
                .model = assets.LoadModel(*modelPath, Core::AssetLifetime::Level),
                .shaderProgram = shaderProgram,
            });

            return entity;
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
            level.m_entities.push_back(entity);
        }

        // Models standing in the level. Like the textures and meshes, they belong to the Level lifetime group.
        for (const MapEntity& mapEntity : map.entities)
        {
            const std::string* className = FindProperty(mapEntity, "classname");
            if (className == nullptr)
                continue;

            if (*className == "misc_model")
            {
                const entt::entity entity = CreateModelEntity(registry, assets, mapEntity, shaderProgram);
                if (entity != entt::null)
                    level.m_entities.push_back(entity);
            }
            else if (className->starts_with("monster_"))
            {
                // Only the place is kept: monsters are gameplay entities, created by the Gameplay module.
                MonsterStart start{.className = *className, .yaw = ReadEntityYaw(mapEntity)};
                if (const std::string* origin = FindProperty(mapEntity, "origin"); origin != nullptr)
                    if (const std::optional<glm::dvec3> position = ParseVectorProperty(*origin); position.has_value())
                        start.origin = ConvertMapPosition(*position);
                level.m_monsterStarts.push_back(std::move(start));
            }
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
        for (const entt::entity entity : m_entities)
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

    const std::vector<MonsterStart>& Level::GetMonsterStarts() const noexcept
    {
        return m_monsterStarts;
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
