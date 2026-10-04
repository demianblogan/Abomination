#pragma once

#include <string>

namespace Rml
{
    class Context;
    class Element;
    class ElementDocument;
}

namespace Abomination::Gameplay
{
    struct GameplayState;
}

namespace Abomination::UI
{
    // The death screen of the game interface (Assets/UI/DeathScreen.rml): the edges of the view darken and the eyelids
    // close; on the black GAME OVER appears (it comes out of the dark and settles), then PRESS
    // ANY KEY TO RESTART, slowly breathing (see Gameplay::PlayerDeath, which times all of it).
    class DeathScreen
    {
    public:
        // Loads the document into the context. If it cannot be loaded, the screen stays empty and a warning is logged.
        [[nodiscard]] static DeathScreen Load(Rml::Context& context, const std::string& documentPath);

        // Once per frame: shown only while the player is dead.
        void Update(const Gameplay::GameplayState& gameplay);

    private:
        Rml::ElementDocument* m_document = nullptr;
        Rml::Element* m_vignette = nullptr;
        Rml::Element* m_upperEyelid = nullptr;
        Rml::Element* m_lowerEyelid = nullptr;
        Rml::Element* m_black = nullptr;
        Rml::Element* m_message = nullptr;
        Rml::Element* m_title = nullptr;
        Rml::Element* m_titleGlow = nullptr;
        Rml::Element* m_hint = nullptr;
    };
}
