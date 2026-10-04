#pragma once

#include "Renderer/Assets/RenderAssets.h"
#include "World/CollisionBrush.h"
#include "World/LevelMesh.h"
#include "World/MapData.h"
#include "World/MonsterStart.h"
#include "World/PlayerStart.h"

#include <entt/entt.hpp>

#include <string>
#include <vector>

namespace Abomination::World
{
    // A loaded level: the entities that draw its static geometry, its collision brushes, where the player starts and
    // numbers about it. Created from a parsed map by Create(), removed by Unload().
    //
    // The level puts its entities into a registry and its assets into stores that the application owns, not the level.
    // So it cannot clean up in its destructor (the registry may already be gone by then): Unload() is called explicitly
    // before the level is replaced. A default-constructed level is empty.
    class Level
    {
    public:
        // Creates the level of a parsed map:
        //   - the world (worldspawn): its brushes become one mesh per texture, stored in the mesh store under
        //     "mapPath#textureName", and one entity draws each of them (together they play the part of entity 0, the
        //     world, in Quake). Textures and meshes go to the Level lifetime group;
        //   - the models (misc_model): one entity each, with the model file of the "model" property at the origin of the
        //     map entity, turned by its angle; the model goes to the Level lifetime group;
        //   - the player start (info_player_start): not an entity, its position and angle are kept;
        //   - the monsters (monster_dog, ...): not entities either, only their places are kept for the Gameplay module.
        // Problems (no world, no player start) are logged; the level is then partly or fully empty.
        [[nodiscard]] static Level Create(entt::registry& registry, Renderer::RenderAssets& assets, const MapData& map,
                                          const std::string& mapPath);

        // Destroys the entities of the level and removes every asset of the Level lifetime group (its textures and
        // meshes) from video memory. Global assets (shaders) stay. The level becomes empty.
        void Unload(entt::registry& registry, Renderer::RenderAssets& assets);

        [[nodiscard]] const PlayerStart& GetPlayerStart() const noexcept;
        [[nodiscard]] const LevelMeshStatistics& GetStatistics() const noexcept;

        // Where the monsters of the map stand (see MonsterStart).
        [[nodiscard]] const std::vector<MonsterStart>& GetMonsterStarts() const noexcept;

        // The solid brushes of the world that characters collide with, clip included (see CollisionBrush, IsClipBrush).
        [[nodiscard]] const std::vector<CollisionBrush>& GetCollisionBrushes() const noexcept;

        // The brushes that stop shots, shells and sight: all but clip.
        [[nodiscard]] const std::vector<CollisionBrush>& GetShotBrushes() const noexcept;

    private:
        // The entities of the level: those that draw the static geometry (one per texture) and the models standing in it.
        std::vector<entt::entity> m_entities;

        std::vector<CollisionBrush> m_collisionBrushes;
        std::vector<CollisionBrush> m_shotBrushes;

        PlayerStart m_playerStart;
        std::vector<MonsterStart> m_monsterStarts;
        LevelMeshStatistics m_statistics;
    };
}
