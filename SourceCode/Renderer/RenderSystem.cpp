#include "Renderer/RenderSystem.h"

#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Renderer/MeshRenderer.h"
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

        // An EnTT view (not to be confused with the camera View): all entities that have both components (const: this
        // system only reads them). each() calls the function for every such entity; because the function asks for the
        // entity as its first parameter, EnTT passes it too. Here it is needed to look for a component that is not part
        // of the EnTT view.
        const auto meshEntities = registry.view<const Core::Transform, const MeshRenderer>();
        meshEntities.each([&](entt::entity entity, const Core::Transform& transform, const MeshRenderer& meshRenderer)
        {
            // try_get returns nullptr if the entity has no such component: only moving entities have a previous transform.
            const Core::PreviousTransform* previousTransform = registry.try_get<Core::PreviousTransform>(entity);
            const Core::Transform drawnTransform = previousTransform == nullptr
                                                       ? transform
                                                       : Core::InterpolateTransform(previousTransform->value, transform,
                                                                                    interpolationFactor);

            // The handles are turned into objects at the moment of use (see AssetCache::Get). In wireframe mode every mesh
            // is drawn with the wireframe shader instead of its own; the texture is still bound below, but that shader
            // does not read it.
            const ShaderHandle shaderHandle =
                settings.isWireframeEnabled ? systemShaders.wireframe : meshRenderer.shaderProgram;
            const GLShaderProgram& shaderProgram = assets.shaders.Get(shaderHandle);
            const GLTexture& texture = assets.textures.Get(meshRenderer.texture);
            const Mesh& mesh = assets.meshes.Get(meshRenderer.mesh);

            // Every entity binds its program and texture again, even if the previous one used the same. That is fine
            // for a few dozen objects; sorting draws by program and texture (batching) comes when there are hundreds.
            shaderProgram.Use();
            shaderProgram.SetUniform(ModelUniform, Core::CalculateModelMatrix(drawnTransform));
            shaderProgram.SetUniform(ViewUniform, view.viewMatrix);
            shaderProgram.SetUniform(ProjectionUniform, view.projectionMatrix);
            texture.Bind(AlbedoTextureUnit);
            mesh.Draw();

            ++statistics.drawCallCount;
            statistics.triangleCount += static_cast<int>(mesh.GetIndexCount() / 3);
        });

        // Back to filled triangles, so whatever is drawn next (the debug overlay) is not affected.
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        return statistics;
    }
}
