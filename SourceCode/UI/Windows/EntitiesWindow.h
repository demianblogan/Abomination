#pragma once

#include <entt/entt.hpp>

namespace Abomination::Renderer
{
    struct RenderAssets;
}

namespace Abomination::UI
{
    // The Entities window of the debug overlay (Entities in the menu bar), an entity inspector: the list of all entities
    // on the left, the components of the selected one on the right. Values can be changed in place (move the player, widen
    // the camera's field of view), so the effect is visible at once.
    //
    // Every component type the inspector knows has its own small drawing function; components it does not know yet are
    // not shown. A new component type gets a drawing function here when it is added to the game.
    class EntitiesWindow
    {
    public:
        // Draws the window while *isOpen is true; its close button sets *isOpen to false.
        // The assets turn the handles of components into readable names ("Textures/Episode1/Crate_Rotten.png").
        void Draw(bool* isOpen, entt::registry& registry, const Renderer::RenderAssets& assets);

    private:
        void DrawEntityList(const entt::registry& registry);

        // The entity chosen in the list; entt::null while nothing is chosen. The selection is kept between frames,
        // and an entity destroyed meanwhile is recognized by registry.valid() (its version no longer matches).
        entt::entity m_selectedEntity = entt::null;
    };
}
