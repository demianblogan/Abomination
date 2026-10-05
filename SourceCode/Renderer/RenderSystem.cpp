#include "Renderer/RenderSystem.h"

#include "Core/Profiling/ProfileZone.h"
#include "Core/Scene/Transform.h"
#include "Core/Scene/TransformInterpolation.h"
#include "Renderer/DrawOffset.h"
#include "Renderer/MeshRenderer.h"
#include "Renderer/ModelPose.h"
#include "Renderer/ModelRenderer.h"
#include "Renderer/OpenGL/GPUProfileZone.h"
#include "Renderer/OpenGL/RenderCommands.h"
#include "Renderer/OpenGL/ShaderInterface.h"

#include <glad/gl.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <span>
#include <vector>

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

        // What every mesh of one drawing pass shares: where the assets are, how to draw, where the scene is seen from, where
        // the work is counted and where skinned meshes put their joints. Passed to DrawMesh as one value instead of seven
        // parameters repeated for every mesh.
        struct MeshPass
        {
            const RenderAssets& assets;
            const SystemShaders& systemShaders;
            const RenderSettings& settings;
            glm::mat4 viewMatrix{1.0f};
            glm::mat4 projectionMatrix{1.0f};
            RenderStatistics& statistics;
            SkinningBuffer& skinning;
        };

        // Draws one mesh with its texture, placed by modelMatrix and seen through the matrices of the pass. A skinned mesh
        // drawn isSkinned is bent by the pose its model uploaded into the skinning buffer (drawn unbent otherwise).
        // In wireframe mode every mesh is drawn with the wireframe shader instead of its own; the texture is still
        // bound, but that shader does not read it.
        void DrawMesh(const MeshPass& pass, ShaderHandle shader, TextureHandle textureHandle, MeshHandle meshHandle,
                      const glm::mat4& modelMatrix, bool isSkinned = false)
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

            // The uniform stays set in the program until it is set again, so it is set for every mesh, skinned or not.
            shaderProgram.SetUniform(IsSkinnedUniform, isSkinned && mesh.IsSkinned());

            texture.Bind(AlbedoTextureUnit);
            mesh.Draw();

            ++pass.statistics.drawCallCount;
            pass.statistics.triangleCount += static_cast<int>(mesh.GetIndexCount() / 3);
        }
    }

    namespace
    {
        // Draws a model part by part, placed by placement (the entity in the world, or the weapon at the eyes). Each
        // part is first placed in the model, then the model is placed: matrices apply from right to left, so the part's
        // transform is on the right. A model with a skeleton is drawn in pose (the matrix of every joint, see ModelPose)
        // or, without one, at rest: its skinned parts are bent by the joints, and a part held by a joint goes where that
        // joint is. partOffsets move parts by name in the coordinates of the model (the pump of the shotgun).
        void DrawModel(const MeshPass& pass, const Model& model, ShaderHandle shader, const glm::mat4& placement,
                       const ModelPose* pose, std::span<const ModelPartOffset> partOffsets = {})
        {
            std::span<const glm::mat4> jointMatrices = model.restJointMatrices;
            if (pose != nullptr && pose->jointMatrices.size() == model.restJointMatrices.size())
                jointMatrices = pose->jointMatrices;

            // The pose goes to the video card once for the whole model, before its parts are drawn.
            const bool hasSkinnedParts = std::ranges::any_of(model.parts, &ModelPart::isSkinned);
            if (model.skeleton.has_value() && hasSkinnedParts)
                pass.skinning.UploadPose(*model.skeleton, jointMatrices);

            for (const ModelPart& part : model.parts)
            {
                glm::mat4 partMatrix = part.transform;
                if (part.parentJoint.has_value() && *part.parentJoint < jointMatrices.size())
                {
                    // The adjustment is in meters, but the joint's space may be scaled (a model in other units):
                    // it is scaled into that space first, so a centimeter stays a centimeter.
                    const glm::mat4& joint = jointMatrices[*part.parentJoint];
                    glm::mat4 adjustment(1.0f);
                    if (pose != nullptr && pose->heldPartAdjustment != glm::mat4(1.0f))
                    {
                        const float scale = glm::length(glm::vec3(joint[0]));
                        adjustment = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f / scale)) * pose->heldPartAdjustment *
                                     glm::scale(glm::mat4(1.0f), glm::vec3(scale));
                    }
                    partMatrix = model.skeletonTransform * joint * adjustment * part.transform;
                }

                for (const ModelPartOffset& partOffset : partOffsets)
                    if (partOffset.partName == part.name)
                        partMatrix = glm::translate(glm::mat4(1.0f), partOffset.offset) * partMatrix;

                DrawMesh(pass, shader, part.texture, part.mesh, placement * partMatrix,
                         part.isSkinned && model.skeleton.has_value());
            }
        }
    }

    SystemShaders LoadSystemShaders(ShaderStore& shaders)
    {
        return SystemShaders{
            .wireframe = shaders.Load("Shaders/Wireframe"),
            .debugLines = shaders.Load("Shaders/DebugLines"),
            .sprites = shaders.Load("Shaders/Sprite"),
            .gameUI = shaders.Load("Shaders/GameUI"),
            .present = shaders.Load("Shaders/Present"),
        };
    }

    glm::mat4 CalculateWeaponViewModelProjection(float verticalFOV, float aspectRatio)
    {
        return glm::perspective(verticalFOV, aspectRatio, WeaponViewModelNearPlane, WeaponViewModelFarPlane);
    }

    RenderStatistics DrawMeshes(const entt::registry& registry, const View& view, float interpolationFactor,
                                const RenderAssets& assets, const SystemShaders& systemShaders,
                                const RenderSettings& settings, SkinningBuffer& skinning)
    {
        PROFILE_ZONE();
        PROFILE_GPU_ZONE("World and models");

        RenderStatistics statistics;
        BeginMeshPass(settings);

        // Where an entity is drawn this frame: between its last two ticks if it moves in ticks, otherwise where it is.
        const auto calculateDrawnTransform = [&](entt::entity entity)
        {
            Core::Transform drawn = Core::CalculateDrawnTransform(registry, entity, interpolationFactor);

            // A character gliding up a stair is drawn below its body, a dog on a stair tilted (see DrawOffset).
            if (const DrawOffset* drawOffset = registry.try_get<DrawOffset>(entity); drawOffset != nullptr)
            {
                drawn.position += glm::mix(drawOffset->previousOffset, drawOffset->offset, interpolationFactor);

                // Multiplied on the right, the tilt turns the model around its own right axis (after its facing), not
                // around the right axis of the world.
                const float pitch = glm::mix(drawOffset->previousPitch, drawOffset->pitch, interpolationFactor);
                drawn.rotation = drawn.rotation * glm::angleAxis(pitch, Core::LocalRight);
            }

            return drawn;
        };

        const MeshPass pass{
            .assets = assets,
            .systemShaders = systemShaders,
            .settings = settings,
            .viewMatrix = view.viewMatrix,
            .projectionMatrix = view.projectionMatrix,
            .statistics = statistics,
            .skinning = skinning,
        };

        // An EnTT view (not to be confused with the camera View): all entities that have both components (const: this
        // system only reads them). each() calls the function for every such entity; because the function asks for the
        // entity as its first parameter, EnTT passes it too. Here it is needed to look for components that are not part
        // of the EnTT view (the previous transform, the draw offset).
        {
            PROFILE_ZONE_NAMED("Level and meshes");
            PROFILE_GPU_ZONE("Level and meshes");

            const auto meshEntities = registry.view<const Core::Transform, const MeshRenderer>();
            meshEntities.each([&](entt::entity entity, const Core::Transform&, const MeshRenderer& meshRenderer)
            {
                const glm::mat4 modelMatrix = Core::CalculateModelMatrix(calculateDrawnTransform(entity));
                DrawMesh(pass, meshRenderer.shaderProgram, meshRenderer.texture, meshRenderer.mesh, modelMatrix);
            });
        }

        // A model is drawn part by part (see DrawModel), in the pose its animation gives it, if it has one.
        {
            PROFILE_ZONE_NAMED("Models");
            PROFILE_GPU_ZONE("Models");

            const auto modelEntities = registry.view<const Core::Transform, const ModelRenderer>();
            modelEntities.each([&](entt::entity entity, const Core::Transform&, const ModelRenderer& modelRenderer)
            {
                const glm::mat4 entityMatrix = Core::CalculateModelMatrix(calculateDrawnTransform(entity));
                DrawModel(pass, assets.models.Get(modelRenderer.model), modelRenderer.shaderProgram, entityMatrix,
                          registry.try_get<ModelPose>(entity));
            });
        }

        EndMeshPass();

        return statistics;
    }

    RenderStatistics DrawWeaponViewModel(ModelHandle model, const glm::mat4& eyeSpaceMatrix, float verticalFOV,
                                         float aspectRatio, const RenderAssets& assets, ShaderHandle shader,
                                         const SystemShaders& systemShaders, const RenderSettings& settings,
                                         SkinningBuffer& skinning, std::span<const ModelPartOffset> partOffsets,
                                         const ModelPose* pose, bool clearsDepth, const View* worldView)
    {
        PROFILE_ZONE();
        PROFILE_GPU_ZONE("Weapon in the hands");

        RenderStatistics statistics;

        // What the world drew into the depth buffer is forgotten: the weapon is drawn over everything, so it never goes
        // into a wall however close the player stands to it. Its own depth still sorts its parts among themselves.
        if (clearsDepth)
            ClearDepth();
        BeginMeshPass(settings);

        // The weapon is placed relative to the eyes, so no view matrix is needed (the identity: the eyes are at the
        // origin, looking along -Z). Its projection has its own field of view: a wider field of view of the world does
        // not stretch the weapon. The aspect ratio is that of the window, so the weapon is not squeezed either.
        const MeshPass pass{
            .assets = assets,
            .systemShaders = systemShaders,
            .settings = settings,
            .viewMatrix = worldView != nullptr ? worldView->viewMatrix : glm::mat4(1.0f),
            .projectionMatrix = worldView != nullptr ? worldView->projectionMatrix
                                                     : CalculateWeaponViewModelProjection(verticalFOV, aspectRatio),
            .statistics = statistics,
            .skinning = skinning,
        };

        // A moved part (the pump) is shifted in the coordinates of the model, before the model is placed at the eyes.
        DrawModel(pass, assets.models.Get(model), shader, eyeSpaceMatrix, pose, partOffsets);

        EndMeshPass();

        return statistics;
    }
}
