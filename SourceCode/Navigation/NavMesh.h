#pragma once

#include <glm/vec3.hpp>

#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

class dtNavMesh;
class dtNavMeshQuery;

// The navigation mesh of a level: its floor cut into convex polygons, built by Recast from the triangles of the level,
// and the paths Detour finds on it (see ARCHITECTURE.md, section 13). Everything is in game meters and axes (Y up), like
// Recast itself.
namespace Abomination::Navigation
{
    // The size of who walks on the navmesh (the agent) and how fine the navmesh is. Built for the dog.
    struct NavMeshSettings
    {
        // Recast first cuts the level into cells: cellSize across, cellHeight high. Smaller cells follow the walls more
        // closely but take longer to build.
        float cellSize = 0.1f;
        float cellHeight = 0.05f;

        // The agent: how far its middle stays from walls, how much room it needs above the floor, how high a step it
        // walks up and how steep a slope (degrees; the clip ramps over stairs are 45, so a little more).
        // The defaults are the size of the dog (Gameplay::DogHalfExtents: 20.4 units to each side, 28.8 high) and the
        // step of the movement code (18 units). Recast takes the agent as a round cylinder, but the dog is a square box that
        // never turns (like every character in Quake): its corners reach sqrt(2) times as far as its sides, 0.9 m
        // instead of 0.64. With the radius of its sides, a path along a slanted face (the corner of an octagonal pillar)
        // ran so close that the corner of the box caught on it; the radius is that of the corners.
        float agentRadius = 0.9f;
        float agentHeight = 0.9f;
        float agentClimb = 0.5625f;
        float maximumSlope = 46.0f;
    };

    // The triangles the navmesh is built from: three indices into the vertices per triangle, counter-clockwise seen
    // from the side that faces out of the solid (the floor seen from above).
    struct NavMeshGeometry
    {
        std::vector<glm::vec3> vertices;
        std::vector<int> indices;
    };

    // One polygon of the navmesh, for drawing it (its corners in order).
    using NavMeshPolygon = std::vector<glm::vec3>;

    class NavMesh
    {
    public:
        // Builds the navmesh. Fails (with the reason) if Recast finds no floor to walk on or runs out of memory.
        [[nodiscard]] static std::expected<NavMesh, std::string> Build(const NavMeshGeometry& geometry,
                                                                       const NavMeshSettings& settings);

        NavMesh(NavMesh&& other) noexcept;
        NavMesh& operator=(NavMesh&& other) noexcept;
        ~NavMesh();

        // The corners of the shortest path from start to end on the floor: start first, end last (or the point of the
        // navmesh nearest to it, if end is off the navmesh but near it). Empty if either point is not near the navmesh
        // or there is no way between them. Points are looked for within a box of searchExtents around them.
        [[nodiscard]] std::vector<glm::vec3> FindPath(const glm::vec3& start, const glm::vec3& end) const;

        // Whether the agent can walk from start straight to end on the floor without leaving the navmesh (nothing in
        // the way: no wall, no hole).
        [[nodiscard]] bool IsStraightWayClear(const glm::vec3& start, const glm::vec3& end) const;

        // A random point on the floor that can be walked to from center, at most about radius away. random returns a
        // number from 0 to 1. None if center is not near the navmesh.
        [[nodiscard]] std::optional<glm::vec3> FindRandomPointAround(const glm::vec3& center, float radius,
                                                                     const std::function<float()>& random) const;

        // Every polygon of the navmesh, for the debug overlay.
        [[nodiscard]] std::vector<NavMeshPolygon> GetPolygons() const;

        // How far up and down, and to the sides, a point may be from the navmesh to be found on it (meters): a body's
        // middle is above its feet, so the height reaches further.
        static constexpr glm::vec3 SearchExtents{1.0f, 2.0f, 1.0f};

    private:
        NavMesh() = default;

        struct NavMeshDeleter
        {
            void operator()(dtNavMesh* navMesh) const noexcept;
        };
        struct QueryDeleter
        {
            void operator()(dtNavMeshQuery* query) const noexcept;
        };

        std::unique_ptr<dtNavMesh, NavMeshDeleter> m_navMesh;
        std::unique_ptr<dtNavMeshQuery, QueryDeleter> m_query;
    };
}
