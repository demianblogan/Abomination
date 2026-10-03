#pragma once

#include "Renderer/Assets/RenderAssets.h"
#include "Renderer/Assets/ShaderStore.h"
#include "Renderer/Camera/View.h"
#include "Renderer/RenderSettings.h"
#include "Renderer/SkinningBuffer.h"

#include <entt/entt.hpp>

namespace Abomination::Renderer
{
    // What one call of DrawMeshes sent to the GPU, for the debug overlay.
    struct RenderStatistics
    {
        // glDrawElements calls: one per drawn mesh. Every call has a cost of its own on the CPU and in the driver, so
        // thousands of small draws are slower than a few large ones, even with the same number of triangles.
        int drawCallCount = 0;
        int triangleCount = 0;
    };

    // Shaders the render system draws with on its own, whatever shader an entity has chosen.
    struct SystemShaders
    {
        // Draws every mesh in one color when the wireframe is on (see RenderSettings).
        ShaderHandle wireframe;

        // Draws DebugLines (see DebugLineRenderer).
        ShaderHandle debugLines;

        // Draws sprites: particles, the muzzle flash, marks on walls (see SpriteRenderer).
        ShaderHandle sprites;

        // Draws the game interface: the HUD, menus (see RmlUiRendererBackend).
        ShaderHandle gameUI;
    };

    // Loads the system shaders. Called once at startup, after the stores are created.
    [[nodiscard]] SystemShaders LoadSystemShaders(ShaderStore& shaders);

    // The render system: draws every entity that has a Core::Transform and a MeshRenderer or a ModelRenderer, as seen from
    // the view.
    // Entities with a Core::PreviousTransform are drawn at fraction interpolationFactor of the way from their previous
    // to their current transform (see Core/Scene/TransformInterpolation.h); the others at their current transform.
    // settings choose how to draw: filled, with the shader of every entity, or as a wireframe, with the wireframe shader
    // of systemShaders for every entity. A model with a skeleton is drawn in the pose of its Renderer::ModelPose, or at
    // rest without one; skinning holds the joint matrices of the skinned mesh being drawn. Only reads the registry:
    // drawing never changes the game.
    // Returns how much was drawn.
    RenderStatistics DrawMeshes(const entt::registry& registry, const View& view, float interpolationFactor,
                                const RenderAssets& assets, const SystemShaders& systemShaders,
                                const RenderSettings& settings, SkinningBuffer& skinning);

    // The projection the weapon in the hands is drawn with (see DrawWeaponViewModel): its own vertical field of view (radians)
    // and near and far planes close to the eyes. Sprites drawn with the weapon (its muzzle flash) use it too.
    [[nodiscard]] glm::mat4 CalculateWeaponViewModelProjection(float verticalFOV, float aspectRatio);

    // A part of a model moved from its place in the model (meters, in the coordinates of the model): the pump of a
    // shotgun pulled back. partName is the name the artist gave the part (see ModelPart::name).
    struct ModelPartOffset
    {
        std::string_view partName;
        glm::vec3 offset{0.0f};
    };

    // Draws the weapon in the hands of the player (a "view model"), after the world: over everything drawn before, with
    // its own field of view (verticalFOV, radians), so it never goes into walls and a wider field of view of the world
    // does not stretch it. eyeSpaceMatrix places the model relative to the eyes: meters, +X to the right, +Y up, -Z
    // forward (the direction the player looks). aspectRatio is that of the window (above 0). partOffsets move parts of
    // the model (the pump), the others stay where the model has them.
    RenderStatistics DrawWeaponViewModel(ModelHandle model, const glm::mat4& eyeSpaceMatrix, float verticalFOV,
                                         float aspectRatio, const RenderAssets& assets, ShaderHandle shader,
                                         const SystemShaders& systemShaders, const RenderSettings& settings,
                                         SkinningBuffer& skinning,
                                         std::span<const ModelPartOffset> partOffsets = {});
}
