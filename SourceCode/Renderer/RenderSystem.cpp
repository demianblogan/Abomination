#include "Renderer/RenderSystem.h"

#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Renderer/DrawOffset.h"
#include "Renderer/MeshRenderer.h"
#include "Renderer/ModelRenderer.h"
#include "Renderer/OpenGL/RenderCommands.h"
#include "Renderer/OpenGL/ShaderInterface.h"

#include <glad/gl.h>
#include <glm/ext/matrix_clip_space.hpp>

namespace Abomination::Renderer
{
    namespace
    {
        // The weapon in the hands is never farther than a meter from the eyes, so its near and far planes are much
        // closer than those of the world: the barrel may come within centimeters of the eyes without being cut off.
        constexpr float ViewModelNearPlane = 0.01f;
        constexpr float ViewModelFarPlane = 10.0f;

        // Sets the states every drawing pass starts with.
        void BeginMeshPass(const RenderSettings& settings)
        {
            // Depth test: for every pixel the depth buffer remembers how far the closest surface drawn there is.
            // A new pixel is drawn only if it is closer (GL_LESS, the default); otherwise it is hidden and thrown away.
            // Without it, objects drawn later would cover closer objects drawn earlier.
            glEnable(GL_DEPTH_TEST);

            // Face culling: triangles whose back side faces the camera are skipped before they reach the fragment
            // shader. The back faces of a closed object are never visible anyway, so this halves the work. OpenGL decides
            // which side is which by the order of the vertices on the screen: counter-clockwise is the front (GL_CCW).
            glEnable(GL_CULL_FACE);

            // Polygon mode: how triangles are filled. GL_LINE draws only their edges (a wireframe), GL_FILL fills them.
            // It applies to both sides of triangles; back faces are still culled, so only the edges of visible faces
            // appear.
            glPolygonMode(GL_FRONT_AND_BACK, settings.isWireframeEnabled ? GL_LINE : GL_FILL);
        }

        // Back to filled triangles, so whatever is drawn next (the debug overlay) is not affected.
        void EndMeshPass()
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        // Draws one mesh with its texture, placed by modelMatrix and seen through viewMatrix and projectionMatrix.
        // In wireframe mode every mesh is drawn with the wireframe shader instead of its own; the texture is still
        // bound, but that shader does not read it.
        void DrawMesh(const RenderAssets& assets, const SystemShaders& systemShaders, const RenderSettings& settings,
                      ShaderHandle shader, TextureHandle textureHandle, MeshHandle meshHandle, const glm::mat4& modelMatrix,
                      const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, RenderStatistics& statistics)
        {
            // The handles are turned into objects at the moment of use (see AssetCache::Get).
            const GLShaderProgram& shaderProgram =
                assets.shaders.Get(settings.isWireframeEnabled ? systemShaders.wireframe : shader);
            const GLTexture& texture = assets.textures.Get(textureHandle);
            const Mesh& mesh = assets.meshes.Get(meshHandle);

            // Every mesh binds its program and texture again, even if the previous one used the same. That is fine for a
            // few dozen objects; sorting draws by program and texture (batching) comes when there are hundreds.
            shaderProgram.Use();
            shaderProgram.SetUniform(ModelUniform, modelMatrix);
            shaderProgram.SetUniform(ViewUniform, viewMatrix);
            shaderProgram.SetUniform(ProjectionUniform, projectionMatrix);
            texture.Bind(AlbedoTextureUnit);
            mesh.Draw();

            ++statistics.drawCallCount;
            statistics.triangleCount += static_cast<int>(mesh.GetIndexCount() / 3);
        }
    }

    SystemShaders LoadSystemShaders(ShaderStore& shaders)
    {
        return SystemShaders{
            .wireframe = shaders.Load("Shaders/Wireframe"),
            .debugLines = shaders.Load("Shaders/DebugLines"),
            .sprites = shaders.Load("Shaders/Sprite"),
        };
    }

    glm::mat4 CalculateViewModelProjection(float verticalFOV, float aspectRatio)
    {
        return glm::perspective(verticalFOV, aspectRatio, ViewModelNearPlane, ViewModelFarPlane);
    }

    RenderStatistics DrawMeshes(const entt::registry& registry, const View& view, float interpolationFactor,
                                const RenderAssets& assets, const SystemShaders& systemShaders,
                                const RenderSettings& settings)
    {
        RenderStatistics statistics;
        BeginMeshPass(settings);

        // Where an entity is drawn this frame: between its last two ticks if it moves in ticks, otherwise where it is.
        const auto calculateDrawnTransform = [&](entt::entity entity)
        {
            Core::Transform drawn = Core::CalculateDrawnTransform(registry, entity, interpolationFactor);

            // A character gliding up a stair is drawn below its body (see DrawOffset).
            if (const DrawOffset* drawOffset = registry.try_get<DrawOffset>(entity); drawOffset != nullptr)
                drawn.position += glm::mix(drawOffset->previousOffset, drawOffset->offset, interpolationFactor);

            return drawn;
        };

        const auto drawMesh = [&](ShaderHandle shader, TextureHandle texture, MeshHandle mesh, const glm::mat4& modelMatrix)
        {
            DrawMesh(assets, systemShaders, settings, shader, texture, mesh, modelMatrix, view.viewMatrix,
                     view.projectionMatrix, statistics);
        };

        // An EnTT view (not to be confused with the camera View): all entities that have both components (const: this
        // system only reads them). each() calls the function for every such entity; because the function asks for the
        // entity as its first parameter, EnTT passes it too. Here it is needed to look for components that are not part
        // of the EnTT view (the previous transform, the draw offset).
        const auto meshEntities = registry.view<const Core::Transform, const MeshRenderer>();
        meshEntities.each([&](entt::entity entity, const Core::Transform&, const MeshRenderer& meshRenderer)
        {
            const glm::mat4 modelMatrix = Core::CalculateModelMatrix(calculateDrawnTransform(entity));
            drawMesh(meshRenderer.shaderProgram, meshRenderer.texture, meshRenderer.mesh, modelMatrix);
        });

        // A model is drawn part by part: each part is first placed in the model (part.transform), then the model is placed
        // in the world. Matrices apply from right to left, so the part's transform is on the right.
        const auto modelEntities = registry.view<const Core::Transform, const ModelRenderer>();
        modelEntities.each([&](entt::entity entity, const Core::Transform&, const ModelRenderer& modelRenderer)
        {
            const glm::mat4 entityMatrix = Core::CalculateModelMatrix(calculateDrawnTransform(entity));
            for (const ModelPart& part : assets.models.Get(modelRenderer.model).parts)
                drawMesh(modelRenderer.shaderProgram, part.texture, part.mesh, entityMatrix * part.transform);
        });

        EndMeshPass();

        return statistics;
    }

    RenderStatistics DrawViewModel(ModelHandle model, const glm::mat4& eyeSpaceMatrix, float verticalFOV, float aspectRatio,
                                   const RenderAssets& assets, ShaderHandle shader, const SystemShaders& systemShaders,
                                   const RenderSettings& settings)
    {
        RenderStatistics statistics;

        // What the world drew into the depth buffer is forgotten: the weapon is drawn over everything, so it never goes
        // into a wall however close the player stands to it. Its own depth still sorts its parts among themselves.
        ClearDepth();
        BeginMeshPass(settings);

        // The weapon is placed relative to the eyes, so no view matrix is needed (the identity: the eyes are at the
        // origin, looking along -Z). Its projection has its own field of view: a wider field of view of the world does
        // not stretch the weapon. The aspect ratio is that of the window, so the weapon is not squeezed either.
        const glm::mat4 projection = CalculateViewModelProjection(verticalFOV, aspectRatio);
        for (const ModelPart& part : assets.models.Get(model).parts)
            DrawMesh(assets, systemShaders, settings, shader, part.texture, part.mesh, eyeSpaceMatrix * part.transform,
                     glm::mat4(1.0f), projection, statistics);

        EndMeshPass();

        return statistics;
    }
}
