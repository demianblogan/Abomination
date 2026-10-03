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
#include <glm/ext/matrix_transform.hpp>

namespace Abomination::Renderer
{
    namespace
    {
        // The weapon in the hands is never farther than a meter from the eyes, so its near and far planes are much
        // closer than those of the world: the barrel may come within centimeters of the eyes without being cut off.
        constexpr float WeaponViewModelNearPlane = 0.01f;
        constexpr float WeaponViewModelFarPlane = 10.0f;

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

        // What every mesh of one drawing pass shares: where the assets are, how to draw, where the scene is seen from and
        // where the work is counted. Passed to DrawMesh as one value instead of six parameters repeated for every mesh.
        struct MeshPass
        {
            const RenderAssets& assets;
            const SystemShaders& systemShaders;
            const RenderSettings& settings;
            glm::mat4 viewMatrix{1.0f};
            glm::mat4 projectionMatrix{1.0f};
            RenderStatistics& statistics;
        };

        // Draws one mesh with its texture, placed by modelMatrix and seen through the matrices of the pass.
        // In wireframe mode every mesh is drawn with the wireframe shader instead of its own; the texture is still
        // bound, but that shader does not read it.
        void DrawMesh(const MeshPass& pass, ShaderHandle shader, TextureHandle textureHandle, MeshHandle meshHandle,
                      const glm::mat4& modelMatrix)
        {
            // The handles are turned into objects at the moment of use (see AssetCache::Get).
            const GLShaderProgram& shaderProgram =
                pass.assets.shaders.Get(pass.settings.isWireframeEnabled ? pass.systemShaders.wireframe : shader);
            const GLTexture& texture = pass.assets.textures.Get(textureHandle);
            const Mesh& mesh = pass.assets.meshes.Get(meshHandle);

            // Every mesh binds its program and texture again, even if the previous one used the same. That is fine for a
            // few dozen objects; sorting draws by program and texture (batching) comes when there are hundreds.
            shaderProgram.Use();
            shaderProgram.SetUniform(ModelUniform, modelMatrix);
            shaderProgram.SetUniform(ViewUniform, pass.viewMatrix);
            shaderProgram.SetUniform(ProjectionUniform, pass.projectionMatrix);
            texture.Bind(AlbedoTextureUnit);
            mesh.Draw();

            ++pass.statistics.drawCallCount;
            pass.statistics.triangleCount += static_cast<int>(mesh.GetIndexCount() / 3);
        }
    }

    SystemShaders LoadSystemShaders(ShaderStore& shaders)
    {
        return SystemShaders{
            .wireframe = shaders.Load("Shaders/Wireframe"),
            .debugLines = shaders.Load("Shaders/DebugLines"),
            .sprites = shaders.Load("Shaders/Sprite"),
            .gameUI = shaders.Load("Shaders/GameUI"),
        };
    }

    glm::mat4 CalculateWeaponViewModelProjection(float verticalFOV, float aspectRatio)
    {
        return glm::perspective(verticalFOV, aspectRatio, WeaponViewModelNearPlane, WeaponViewModelFarPlane);
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

        const MeshPass pass{
            .assets = assets,
            .systemShaders = systemShaders,
            .settings = settings,
            .viewMatrix = view.viewMatrix,
            .projectionMatrix = view.projectionMatrix,
            .statistics = statistics,
        };

        // An EnTT view (not to be confused with the camera View): all entities that have both components (const: this
        // system only reads them). each() calls the function for every such entity; because the function asks for the
        // entity as its first parameter, EnTT passes it too. Here it is needed to look for components that are not part
        // of the EnTT view (the previous transform, the draw offset).
        const auto meshEntities = registry.view<const Core::Transform, const MeshRenderer>();
        meshEntities.each([&](entt::entity entity, const Core::Transform&, const MeshRenderer& meshRenderer)
        {
            const glm::mat4 modelMatrix = Core::CalculateModelMatrix(calculateDrawnTransform(entity));
            DrawMesh(pass, meshRenderer.shaderProgram, meshRenderer.texture, meshRenderer.mesh, modelMatrix);
        });

        // A model is drawn part by part: each part is first placed in the model (part.transform), then the model is placed
        // in the world. Matrices apply from right to left, so the part's transform is on the right.
        const auto modelEntities = registry.view<const Core::Transform, const ModelRenderer>();
        modelEntities.each([&](entt::entity entity, const Core::Transform&, const ModelRenderer& modelRenderer)
        {
            const glm::mat4 entityMatrix = Core::CalculateModelMatrix(calculateDrawnTransform(entity));
            for (const ModelPart& part : assets.models.Get(modelRenderer.model).parts)
                DrawMesh(pass, modelRenderer.shaderProgram, part.texture, part.mesh, entityMatrix * part.transform);
        });

        EndMeshPass();

        return statistics;
    }

    RenderStatistics DrawWeaponViewModel(ModelHandle model, const glm::mat4& eyeSpaceMatrix, float verticalFOV,
                                         float aspectRatio, const RenderAssets& assets, ShaderHandle shader,
                                         const SystemShaders& systemShaders, const RenderSettings& settings,
                                         std::span<const ModelPartOffset> partOffsets)
    {
        RenderStatistics statistics;

        // What the world drew into the depth buffer is forgotten: the weapon is drawn over everything, so it never goes
        // into a wall however close the player stands to it. Its own depth still sorts its parts among themselves.
        ClearDepth();
        BeginMeshPass(settings);

        // The weapon is placed relative to the eyes, so no view matrix is needed (the identity: the eyes are at the
        // origin, looking along -Z). Its projection has its own field of view: a wider field of view of the world does
        // not stretch the weapon. The aspect ratio is that of the window, so the weapon is not squeezed either.
        const MeshPass pass{
            .assets = assets,
            .systemShaders = systemShaders,
            .settings = settings,
            .viewMatrix = glm::mat4(1.0f),
            .projectionMatrix = CalculateWeaponViewModelProjection(verticalFOV, aspectRatio),
            .statistics = statistics,
        };
        for (const ModelPart& part : assets.models.Get(model).parts)
        {
            // A moved part is shifted in the coordinates of the model, before the model is placed at the eyes.
            glm::mat4 partMatrix = part.transform;
            for (const ModelPartOffset& partOffset : partOffsets)
                if (partOffset.partName == part.name)
                    partMatrix = glm::translate(glm::mat4(1.0f), partOffset.offset) * partMatrix;

            DrawMesh(pass, shader, part.texture, part.mesh, eyeSpaceMatrix * partMatrix);
        }

        EndMeshPass();

        return statistics;
    }
}
