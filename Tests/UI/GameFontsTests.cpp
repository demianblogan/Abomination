#include "Core/Files/FileSystem.h"

// stb_truetype reads the character map of a font file. The game itself draws text with RmlUi (FreeType); the test only
// needs to know which characters a font has. Its implementation is compiled here, the only place that uses it.
#define STB_TRUETYPE_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable : 4244)
#include <stb_truetype.h>
#pragma warning(pop)

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace Abomination::UI
{
    namespace
    {
        // Every letter and sign the texts of the game may use in its five languages (English, Spanish, German, Russian,
        // Ukrainian), in UTF-8. A font of the game must have all of them, or a word would show the "missing" box.
        constexpr std::string_view GameRequiredCharacters =
            // English, digits and common punctuation.
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.,:;!?'\"()[]-+/%*=&#@_<>"
            // Spanish.
            "ÁÉÍÓÚÜÑáéíóúüñ¿¡«»"
            // German.
            "ÄÖÜäöüß„“‚‘"
            // Russian.
            "АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюя"
            // Ukrainian letters that Russian does not have, and its apostrophe.
            "ІіЇїЄєҐґ’"
            // Typography shared by all of them.
            "–—…”№€";

        // The code points of UTF-8 text (valid text only: the string above).
        std::vector<char32_t> DecodeUTF8(std::string_view text)
        {
            std::vector<char32_t> characters;
            for (std::size_t index = 0; index < text.size();)
            {
                const auto first = static_cast<std::uint8_t>(text[index]);
                const int length = first < 0x80 ? 1 : (first & 0xE0) == 0xC0 ? 2 : (first & 0xF0) == 0xE0 ? 3 : 4;
                char32_t codepoint = length == 1 ? first : first & (0xFF >> (length + 1));
                for (int offset = 1; offset < length; ++offset)
                    codepoint = (codepoint << 6) | (static_cast<std::uint8_t>(text[index + static_cast<std::size_t>(offset)]) & 0x3F);
                characters.push_back(codepoint);
                index += static_cast<std::size_t>(length);
            }
            return characters;
        }

        // Checks every required character against the character map of a font of the game (Assets/Fonts in the source).
        void ExpectEveryGameCharacter(const std::string& fileName)
        {
            const auto path = std::filesystem::path(ABOMINATION_TEST_ASSETS_DIRECTORY) / "Fonts" / fileName;
            const std::expected<std::vector<std::byte>, std::string> bytes = Core::ReadBinaryFile(path);
            ASSERT_TRUE(bytes.has_value()) << bytes.error();

            const auto* data = reinterpret_cast<const unsigned char*>(bytes->data());
            stbtt_fontinfo font;
            ASSERT_NE(stbtt_InitFont(&font, data, stbtt_GetFontOffsetForIndex(data, 0)), 0) << fileName;

            // Glyph 0 is the "missing character" box every font has.
            for (const char32_t character : DecodeUTF8(GameRequiredCharacters))
            {
                EXPECT_NE(stbtt_FindGlyphIndex(&font, static_cast<int>(character)), 0)
                    << std::format("{} has no U+{:04X}", fileName, static_cast<unsigned int>(character));
            }
        }
    }

    TEST(GameFonts, TextFontHasEveryLetterOfTheGame)
    {
        ExpectEveryGameCharacter("OswaldBold.ttf");
    }

    TEST(GameFonts, TitleFontHasEveryLetterOfTheGame)
    {
        ExpectEveryGameCharacter("CormorantSCBold.ttf");
    }
}
