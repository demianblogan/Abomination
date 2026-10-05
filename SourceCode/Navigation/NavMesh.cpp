#include "Navigation/NavMesh.h"

#include "Core/Profiling/ProfileZone.h"

#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>
#include <Recast.h>

#include <glm/common.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

namespace Abomination::Navigation
{
    namespace
    {
        // Every polygon of our navmesh can be walked on: one area and one flag.
        constexpr unsigned char WalkableArea = RC_WALKABLE_AREA;
        constexpr unsigned short WalkableFlag = 1;

        // How many polygons a path may cross, how many corners it may have, and how many polygons Detour may look at in
        // one search. A level of a few rooms has a few hundred polygons.
        constexpr int MaximumPathPolygons = 256;
        constexpr int MaximumCorners = 64;
        constexpr int MaximumSearchNodes = 2048;

        // Recast frees what it allocates with its own functions; these hold them in unique_ptr.
        struct HeightfieldDeleter { void operator()(rcHeightfield* p) const noexcept { rcFreeHeightField(p); } };
        struct CompactDeleter { void operator()(rcCompactHeightfield* p) const noexcept { rcFreeCompactHeightfield(p); } };
        struct ContoursDeleter { void operator()(rcContourSet* p) const noexcept { rcFreeContourSet(p); } };
        struct PolyMeshDeleter { void operator()(rcPolyMesh* p) const noexcept { rcFreePolyMesh(p); } };
        struct DetailDeleter { void operator()(rcPolyMeshDetail* p) const noexcept { rcFreePolyMeshDetail(p); } };

        // Detour asks for random numbers through a plain function pointer, with no room for a context: the function of
        // the search in progress is kept here for it (searches run one at a time, on the game thread).
        const std::function<float()>* currentRandom = nullptr;
        float CallCurrentRandom()
        {
            // Detour wants [0, 1): 1 is moved just below.
            return std::min((*currentRandom)(), 0.99999f);
        }

        dtQueryFilter CreateFilter()
        {
            dtQueryFilter filter;
            filter.setIncludeFlags(WalkableFlag);
            filter.setExcludeFlags(0);
            return filter;
        }
    }

    void NavMesh::NavMeshDeleter::operator()(dtNavMesh* navMesh) const noexcept
    {
        dtFreeNavMesh(navMesh);
    }

    void NavMesh::QueryDeleter::operator()(dtNavMeshQuery* query) const noexcept
    {
        dtFreeNavMeshQuery(query);
    }

    NavMesh::NavMesh(NavMesh&& other) noexcept = default;
    NavMesh& NavMesh::operator=(NavMesh&& other) noexcept = default;
    NavMesh::~NavMesh() = default;

    std::expected<NavMesh, std::string> NavMesh::Build(const NavMeshGeometry& geometry, const NavMeshSettings& settings)
    {
        if (geometry.vertices.empty() || geometry.indices.size() < 3)
            return std::unexpected("no triangles to build the navmesh from");

        // The steps of Recast, as in its own samples (Sample_SoloMesh): one tile over the whole level.
        rcContext context(false);
        rcConfig config{};
        config.cs = settings.cellSize;
        config.ch = settings.cellHeight;
        config.walkableSlopeAngle = settings.maximumSlope;
        // Heights and the radius in cells: the room above the floor rounded up (the agent must fit), the step rounded
        // down (it must not climb more than it can), the radius rounded up (it must not touch the walls).
        config.walkableHeight = static_cast<int>(std::ceil(settings.agentHeight / config.ch));
        config.walkableClimb = static_cast<int>(std::floor(settings.agentClimb / config.ch));
        config.walkableRadius = static_cast<int>(std::ceil(settings.agentRadius / config.cs));
        config.maxEdgeLen = static_cast<int>(12.0f / config.cs);
        config.maxSimplificationError = 1.3f;
        config.minRegionArea = 8 * 8;
        config.mergeRegionArea = 20 * 20;
        config.maxVertsPerPoly = DT_VERTS_PER_POLYGON;
        config.detailSampleDist = config.cs * 6.0f;
        config.detailSampleMaxError = config.ch;

        const auto* vertices = &geometry.vertices[0].x;
        const auto vertexCount = static_cast<int>(geometry.vertices.size());
        const int* triangles = geometry.indices.data();
        const auto triangleCount = static_cast<int>(geometry.indices.size() / 3);
        rcCalcBounds(vertices, vertexCount, config.bmin, config.bmax);
        rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);

        // 1. The triangles are drawn into a field of cells, column by column (a "heightfield"): every column keeps the
        //    spans of solid it crosses. Triangles flatter than the slope limit are marked walkable.
        std::unique_ptr<rcHeightfield, HeightfieldDeleter> heightfield(rcAllocHeightfield());
        if (!heightfield || !rcCreateHeightfield(&context, *heightfield, config.width, config.height, config.bmin,
                                                 config.bmax, config.cs, config.ch))
            return std::unexpected("out of memory for the heightfield");
        std::vector<unsigned char> areas(static_cast<std::size_t>(triangleCount), 0);
        rcMarkWalkableTriangles(&context, config.walkableSlopeAngle, vertices, vertexCount, triangles, triangleCount,
                                areas.data());
        if (!rcRasterizeTriangles(&context, vertices, vertexCount, triangles, areas.data(), triangleCount, *heightfield,
                                  config.walkableClimb))
            return std::unexpected("the triangles could not be rasterized");

        // 2. Walkable spans next to a step the agent can climb stay walkable; ledges and spans with too little room
        //    above them do not.
        rcFilterLowHangingWalkableObstacles(&context, config.walkableClimb, *heightfield);
        rcFilterLedgeSpans(&context, config.walkableHeight, config.walkableClimb, *heightfield);
        rcFilterWalkableLowHeightSpans(&context, config.walkableHeight, *heightfield);

        // 3. Only the open space above the floor is kept, shrunk from the walls by the radius of the agent, and cut into
        //    regions (pieces of floor without holes).
        std::unique_ptr<rcCompactHeightfield, CompactDeleter> compact(rcAllocCompactHeightfield());
        if (!compact || !rcBuildCompactHeightfield(&context, config.walkableHeight, config.walkableClimb, *heightfield,
                                                   *compact))
            return std::unexpected("out of memory for the compact heightfield");
        heightfield.reset();
        if (!rcErodeWalkableArea(&context, config.walkableRadius, *compact))
            return std::unexpected("the walkable area could not be eroded");
        if (!rcBuildDistanceField(&context, *compact) ||
            !rcBuildRegions(&context, *compact, 0, config.minRegionArea, config.mergeRegionArea))
            return std::unexpected("the regions could not be built");

        // 4. The outlines of the regions are traced and simplified, then the regions are cut into convex polygons, and
        //    a finer mesh of their heights is added (the detail mesh: steps and ramps inside a polygon).
        std::unique_ptr<rcContourSet, ContoursDeleter> contours(rcAllocContourSet());
        if (!contours || !rcBuildContours(&context, *compact, config.maxSimplificationError, config.maxEdgeLen, *contours))
            return std::unexpected("the contours could not be built");
        std::unique_ptr<rcPolyMesh, PolyMeshDeleter> polyMesh(rcAllocPolyMesh());
        if (!polyMesh || !rcBuildPolyMesh(&context, *contours, config.maxVertsPerPoly, *polyMesh))
            return std::unexpected("the polygons could not be built");
        std::unique_ptr<rcPolyMeshDetail, DetailDeleter> detail(rcAllocPolyMeshDetail());
        if (!detail || !rcBuildPolyMeshDetail(&context, *polyMesh, *compact, config.detailSampleDist,
                                              config.detailSampleMaxError, *detail))
            return std::unexpected("the detail mesh could not be built");
        if (polyMesh->npolys == 0)
            return std::unexpected("no floor to walk on");

        // Every polygon is walkable floor.
        for (int index = 0; index < polyMesh->npolys; ++index)
        {
            polyMesh->flags[index] = WalkableFlag;
            polyMesh->areas[index] = WalkableArea;
        }

        // 5. Detour takes the polygons as one tile of data, which it then owns (DT_TILE_FREE_DATA).
        dtNavMeshCreateParams parameters{};
        parameters.verts = polyMesh->verts;
        parameters.vertCount = polyMesh->nverts;
        parameters.polys = polyMesh->polys;
        parameters.polyAreas = polyMesh->areas;
        parameters.polyFlags = polyMesh->flags;
        parameters.polyCount = polyMesh->npolys;
        parameters.nvp = polyMesh->nvp;
        parameters.detailMeshes = detail->meshes;
        parameters.detailVerts = detail->verts;
        parameters.detailVertsCount = detail->nverts;
        parameters.detailTris = detail->tris;
        parameters.detailTriCount = detail->ntris;
        parameters.walkableHeight = settings.agentHeight;
        parameters.walkableRadius = settings.agentRadius;
        parameters.walkableClimb = settings.agentClimb;
        std::memcpy(parameters.bmin, polyMesh->bmin, sizeof(parameters.bmin));
        std::memcpy(parameters.bmax, polyMesh->bmax, sizeof(parameters.bmax));
        parameters.cs = config.cs;
        parameters.ch = config.ch;
        parameters.buildBvTree = true;

        unsigned char* data = nullptr;
        int dataSize = 0;
        if (!dtCreateNavMeshData(&parameters, &data, &dataSize))
            return std::unexpected("Detour could not create the navmesh data");

        NavMesh navMesh;
        navMesh.m_navMesh.reset(dtAllocNavMesh());
        if (!navMesh.m_navMesh || dtStatusFailed(navMesh.m_navMesh->init(data, dataSize, DT_TILE_FREE_DATA)))
        {
            dtFree(data);
            return std::unexpected("Detour could not start the navmesh");
        }
        navMesh.m_query.reset(dtAllocNavMeshQuery());
        if (!navMesh.m_query || dtStatusFailed(navMesh.m_query->init(navMesh.m_navMesh.get(), MaximumSearchNodes)))
            return std::unexpected("Detour could not start the navmesh query");

        return navMesh;
    }

    std::vector<glm::vec3> NavMesh::FindPath(const glm::vec3& start, const glm::vec3& end) const
    {
        PROFILE_ZONE();

        const dtQueryFilter filter = CreateFilter();
        dtPolyRef startRef = 0;
        dtPolyRef endRef = 0;
        glm::vec3 startOnMesh(0.0f);
        glm::vec3 endOnMesh(0.0f);
        m_query->findNearestPoly(&start.x, &SearchExtents.x, &filter, &startRef, &startOnMesh.x);
        m_query->findNearestPoly(&end.x, &SearchExtents.x, &filter, &endRef, &endOnMesh.x);
        if (startRef == 0 || endRef == 0)
            return {};

        // A*: the corridor of polygons from the start to the end (see the lecture, part 5)...
        std::array<dtPolyRef, MaximumPathPolygons> corridor{};
        int corridorSize = 0;
        if (dtStatusFailed(m_query->findPath(startRef, endRef, &startOnMesh.x, &endOnMesh.x, &filter, corridor.data(),
                                             &corridorSize, MaximumPathPolygons)) ||
            corridorSize == 0)
            return {};

        // ...and the string pulled through it: the corners (part 6). If the end cannot be reached, the corridor ends at
        // the polygon nearest to it, so the path goes as close as it can.
        glm::vec3 pathEnd = endOnMesh;
        if (corridor[static_cast<std::size_t>(corridorSize - 1)] != endRef)
            m_query->closestPointOnPoly(corridor[static_cast<std::size_t>(corridorSize - 1)], &end.x, &pathEnd.x, nullptr);
        std::array<glm::vec3, MaximumCorners> corners{};
        int cornerCount = 0;
        m_query->findStraightPath(&startOnMesh.x, &pathEnd.x, corridor.data(), corridorSize, &corners[0].x, nullptr,
                                  nullptr, &cornerCount, MaximumCorners);
        return std::vector<glm::vec3>(corners.begin(), corners.begin() + cornerCount);
    }

    bool NavMesh::IsStraightWayClear(const glm::vec3& start, const glm::vec3& end) const
    {
        PROFILE_ZONE();

        const dtQueryFilter filter = CreateFilter();
        dtPolyRef startRef = 0;
        glm::vec3 startOnMesh(0.0f);
        m_query->findNearestPoly(&start.x, &SearchExtents.x, &filter, &startRef, &startOnMesh.x);
        if (startRef == 0)
            return false;

        // A ray along the floor: t is the part of the way it got before a wall of the navmesh (FLT_MAX if none).
        float t = 0.0f;
        glm::vec3 hitNormal(0.0f);
        std::array<dtPolyRef, MaximumPathPolygons> visited{};
        int visitedCount = 0;
        if (dtStatusFailed(m_query->raycast(startRef, &startOnMesh.x, &end.x, &filter, &t, &hitNormal.x, visited.data(),
                                            &visitedCount, MaximumPathPolygons)))
            return false;
        return t >= 1.0f;
    }

    std::optional<glm::vec3> NavMesh::FindRandomPointAround(const glm::vec3& center, float radius,
                                                            const std::function<float()>& random) const
    {
        PROFILE_ZONE();

        const dtQueryFilter filter = CreateFilter();
        dtPolyRef centerRef = 0;
        glm::vec3 centerOnMesh(0.0f);
        m_query->findNearestPoly(&center.x, &SearchExtents.x, &filter, &centerRef, &centerOnMesh.x);
        if (centerRef == 0)
            return std::nullopt;

        currentRandom = &random;
        dtPolyRef pointRef = 0;
        glm::vec3 point(0.0f);
        const dtStatus status = m_query->findRandomPointAroundCircle(centerRef, &centerOnMesh.x, radius, &filter,
                                                                     CallCurrentRandom, &pointRef, &point.x);
        currentRandom = nullptr;
        if (dtStatusFailed(status) || pointRef == 0)
            return std::nullopt;
        return point;
    }

    std::vector<NavMeshPolygon> NavMesh::GetPolygons() const
    {
        std::vector<NavMeshPolygon> polygons;
        const dtNavMesh& navMesh = *m_navMesh;
        for (int tileIndex = 0; tileIndex < navMesh.getMaxTiles(); ++tileIndex)
        {
            const dtMeshTile* tile = navMesh.getTile(tileIndex);
            if (tile == nullptr || tile->header == nullptr)
                continue;

            for (int polyIndex = 0; polyIndex < tile->header->polyCount; ++polyIndex)
            {
                const dtPoly& poly = tile->polys[polyIndex];
                NavMeshPolygon& polygon = polygons.emplace_back();
                for (unsigned int corner = 0; corner < poly.vertCount; ++corner)
                {
                    const float* vertex = &tile->verts[poly.verts[corner] * 3];
                    polygon.emplace_back(vertex[0], vertex[1], vertex[2]);
                }
            }
        }
        return polygons;
    }
}
