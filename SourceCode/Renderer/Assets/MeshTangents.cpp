#include "Renderer/Assets/MeshTangents.h"

#include <mikktspace.h>

#include <cassert>
#include <cstddef>

namespace Abomination::Renderer
{
    namespace
    {
        // MikkTSpace is a C library: it asks for the mesh through functions it is given (an "interface" of function
        // pointers) and hands every result to another one. The mesh itself travels through the user data of its context.
        MeshData& GetMesh(const SMikkTSpaceContext* context)
        {
            return *static_cast<MeshData*>(context->m_pUserData);
        }

        // The vertex of a corner of a triangle (face) of the mesh.
        MeshVertex& GetVertex(const SMikkTSpaceContext* context, int face, int corner)
        {
            MeshData& mesh = GetMesh(context);
            const std::size_t index = static_cast<std::size_t>(face) * 3 + static_cast<std::size_t>(corner);

            return mesh.vertices[mesh.indices[index]];
        }

        int GetFaceCount(const SMikkTSpaceContext* context)
        {
            return static_cast<int>(GetMesh(context).indices.size() / 3);
        }

        int GetCornerCount(const SMikkTSpaceContext*, int)
        {
            return 3;
        }

        void GetPosition(const SMikkTSpaceContext* context, float position[], int face, int corner)
        {
            const glm::vec3& value = GetVertex(context, face, corner).position;
            position[0] = value.x;
            position[1] = value.y;
            position[2] = value.z;
        }

        void GetNormal(const SMikkTSpaceContext* context, float normal[], int face, int corner)
        {
            const glm::vec3& value = GetVertex(context, face, corner).normal;
            normal[0] = value.x;
            normal[1] = value.y;
            normal[2] = value.z;
        }

        void GetTexCoord(const SMikkTSpaceContext* context, float texCoord[], int face, int corner)
        {
            const glm::vec2& value = GetVertex(context, face, corner).texCoord;
            texCoord[0] = value.x;
            texCoord[1] = value.y;
        }

        // The result for one corner: the tangent and the sign of the bitangent, the convention of MeshVertex::tangent.
        void SetTangent(const SMikkTSpaceContext* context, const float tangent[], float sign, int face, int corner)
        {
            GetVertex(context, face, corner).tangent = glm::vec4(tangent[0], tangent[1], tangent[2], sign);
        }
    }

    bool GenerateTangents(MeshData& mesh)
    {
        assert(mesh.indices.size() % 3 == 0);

        SMikkTSpaceInterface callbacks{};
        callbacks.m_getNumFaces = GetFaceCount;
        callbacks.m_getNumVerticesOfFace = GetCornerCount;
        callbacks.m_getPosition = GetPosition;
        callbacks.m_getNormal = GetNormal;
        callbacks.m_getTexCoord = GetTexCoord;
        callbacks.m_setTSpaceBasic = SetTangent;

        SMikkTSpaceContext context{};
        context.m_pInterface = &callbacks;
        context.m_pUserData = &mesh;

        // genTangSpaceDefault returns a "tbool" (an int): 0 when it failed.
        return genTangSpaceDefault(&context) != 0;
    }
}
