#include "Renderer/RenderSystem.h"

#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Renderer/MeshRenderer.h"
#include "Renderer/ModelRenderer.h"
#include "Renderer/OpenGL/ShaderInterface.h"

#include <glad/gl.h>

namespace Abomination::Renderer
{
    SystemShaders LoadSystemShaders(ShaderStore& shaders)
    {
        return SystemShaders{
            .wireframe = shaders.Load("Shaders/Wireframe"),
            .debugLines = shaders.Load("Shaders/DebugLines"),
        };
    }

    RenderStatistics DrawMeshes(const entt::registry& registry, const View& view, float interpolationFactor,
                                const RenderAssets& assets, const SystemShaders& systemShaders,
                                const RenderSettings& settings)
    {
        RenderStatistics statistics;

        // Depth test: for every pixel the depth buffer remembers how far the closest surface drawn there is.
        // A new pixel is drawn only if it is closer (GL_LESS, the default); otherwise it is hidden and thrown away.
        // Without it, objects drawn later would cover closer objects drawn earlier.
        glEnable(GL_DEPTH_TEST);

        // Face culling: triangles whose back side faces the camera are skipped before they reach the fragment shader.
        // The back faces of a closed object are never visible anyway, so this halves the work. OpenGL decides which side
        // is which by the order of the vertices on the screen: counter-clockwise is the front (GL_CCW, the default).
        glEnable(GL_CULL_FACE);

        // Polygon mode: how triangles are filled. GL_LINE draws only their edges (a wireframe), GL_FILL fills them.
        // It applies to both sides of triangles; back faces are still culled, so only the edges of visible faces appear.
        glPolygonMode(GL_FRONT_AND_BACK, settings.isWireframeEnabled ? GL_LINE : GL_FILL);

        // Where an entity is drawn this frame: between its last two ticks if it moves in ticks, otherwise where it is.
        const auto calculateDrawnTransform = [&](entt::entity entity, const Core::Transform& transform)
        {
            // try_get returns nullptr if the entity has no such component: only moving entities have a previous transform.
            const Core::PreviousTransform* previousTransform = registry.try_get<Core::PreviousTransform>(entity);
            return previousTransform == nullptr
                       ? transform
                       : Core::InterpolateTransform(previousTransform->value, transform, interpolationFactor);
        };

        // Draws one mesh with its texture, placed by modelMatrix.
        const auto drawMesh = [&](ShaderHandle shader, TextureHandle textureHandle, MeshHandle meshHandle,
                                  const glm::mat4& modelMatrix)
        {
            // The handles are turned into objects at the moment of use (see AssetCache::Get). In wireframe mode every mesh
            // is drawn with the wireframe shader instead of its own; the texture is still bound below, but that shader
            // does not read it.
            const GLShaderProgram& shaderProgram = assets.shaders.Get(settings.isWireframeEnabled ? systemShaders.wireframe
                                                                                                  : shader);
            const GLTexture& texture = assets.textures.Get(textureHandle);
            const Mesh& mesh = assets.meshes.Get(meshHandle);

            // Every mesh binds its program and texture again, even if the previous one used the same. That is fine for a
            // few dozen objects; sorting draws by program and texture (batching) comes when there are hundreds.
            shaderProgram.Use();
            shaderProgram.SetUniform(ModelUniform, modelMatrix);
            shaderProgram.SetUniform(ViewUniform, view.viewMatrix);
            shaderProgram.SetUniform(ProjectionUniform, view.projectionMatrix);
            texture.Bind(AlbedoTextureUnit);
            mesh.Draw();

            ++statistics.drawCallCount;
            statistics.triangleCount += static_cast<int>(mesh.GetIndexCount() / 3);
        };

        // An EnTT view (not to be confused with the camera View): all entities that have both components (const: this
        // system only reads them). each() calls the function for every such entity; because the function asks for the
        // entity as its first parameter, EnTT passes it too. Here it is needed to look for a component that is not part
        // of the EnTT view.
        const auto meshEntities = registry.view<const Core::Transform, const MeshRenderer>();
        meshEntities.each([&](entt::entity entity, const Core::Transform& transform, const MeshRenderer& meshRenderer)
        {
            const glm::mat4 modelMatrix = Core::CalculateModelMatrix(calculateDrawnTransform(entity, transform));
            drawMesh(meshRenderer.shaderProgram, meshRenderer.texture, meshRenderer.mesh, modelMatrix);
        });

        // A model is drawn part by part: each part is first placed in the model (part.transform), then the model is placed
        // in the world. Matrices apply from right to left, so the part's transform is on the right.
        const auto modelEntities = registry.view<const Core::Transform, const ModelRenderer>();
        modelEntities.each([&](entt::entity entity, const Core::Transform& transform, const ModelRenderer& modelRenderer)
        {
            const glm::mat4 entityMatrix = Core::CalculateModelMatrix(calculateDrawnTransform(entity, transform));
            for (const ModelPart& part : assets.models.Get(modelRenderer.model).parts)
                drawMesh(modelRenderer.shaderProgram, part.texture, part.mesh, entityMatrix * part.transform);
        });

        // Back to filled triangles, so whatever is drawn next (the debug overlay) is not affected.
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        return statistics;
    }
}
