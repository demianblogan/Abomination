#pragma once

#include "Renderer/RenderAssets.h"
#include "World/LevelMesh.h"
#include "World/MapData.h"

#include <entt/entt.hpp>
#include <glm/vec3.hpp>

#include <string>
#include <vector>

namespace Abomination::World
{
    // Where the player appears and where they look, in game coordinates (from the info_player_start entity).
    struct PlayerStart
    {
        // The position of the player's eyes.
        glm::vec3 eyePosition{0.0f};

        // Radians, like Gameplay::FreeFlyCamera::yaw.
        float yaw = 0.0f;
    };

    // A loaded level: the entities that draw its static geometry, where the player starts and numbers about it; its
    // collision data comes next. Created from a parsed map by Create(), removed by Unload().
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
        //   - the player start (info_player_start): not an entity, its position and angle are kept.
        // Problems (no world, no player start) are logged; the level is then partly or fully empty.
        [[nodiscard]] static Level Create(entt::registry& registry, Renderer::RenderAssets& assets, const MapData& map,
                                          const std::string& mapPath);

        // Destroys the entities of the level and removes every asset of the Level lifetime group (its textures and
        // meshes) from video memory. Global assets (shaders) stay. The level becomes empty.
        void Unload(entt::registry& registry, Renderer::RenderAssets& assets);

        [[nodiscard]] const PlayerStart& GetPlayerStart() const noexcept;
        [[nodiscard]] const LevelMeshStatistics& GetStatistics() const noexcept;

    private:
        // The entities that draw the static geometry, one per texture; empty if the map has no world.
        std::vector<entt::entity> m_geometryEntities;

        PlayerStart m_playerStart;
        LevelMeshStatistics m_statistics;
    };
}
