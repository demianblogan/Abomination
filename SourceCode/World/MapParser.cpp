#include "World/MapParser.h"

#include "Core/Files/FileSystem.h"

#include <charconv>
#include <cstddef>
#include <format>
#include <system_error>
#include <utility>

namespace Abomination::World
{
    namespace
    {
        // --- Step 1: cutting the text into tokens ---
        //
        // A token is the smallest meaningful piece of the file: a bracket, a quoted string or a "word" (a number or a
        // texture name). The parser then only deals with tokens and never with single characters or spaces:
        //   ( 48 64 240 ) Crate [ 0 1 0 0 ]   ->   (  48  64  240  )  Crate  [  0  1  0  0  ]

        enum class TokenKind
        {
            Symbol,  // one of { } ( ) [ ]
            String,  // text in double quotes, without the quotes: property keys and values
            Word,    // anything else up to the next space: numbers and texture names
            End,     // the end of the text
        };

        struct Token
        {
            TokenKind kind = TokenKind::End;
            std::string_view text;

            // The line the token starts on, for error messages.
            int line = 1;
        };

        bool IsSymbol(char character)
        {
            return character == '{' || character == '}' || character == '(' || character == ')' || character == '[' ||
                   character == ']';
        }

        bool IsSpace(char character)
        {
            return character == ' ' || character == '\t' || character == '\r' || character == '\n';
        }

        class Tokenizer
        {
        public:
            explicit Tokenizer(std::string_view text) noexcept
                : m_text(text)
            {}

            // Reads the next token. A string_view in the token points into the original text, so nothing is copied.
            Token Next()
            {
                SkipSpacesAndComments();

                Token token;
                token.line = m_line;
                if (m_position >= m_text.size())
                    return token;

                const char character = m_text[m_position];
                if (IsSymbol(character))
                {
                    token.kind = TokenKind::Symbol;
                    token.text = m_text.substr(m_position, 1);
                    ++m_position;

                    return token;
                }

                if (character == '"')
                {
                    // Quake maps have no escape sequences in strings: everything up to the next quote is the value.
                    const std::size_t start = m_position + 1;
                    std::size_t end = start;
                    while (end < m_text.size() && m_text[end] != '"')
                    {
                        if (m_text[end] == '\n')
                            ++m_line;
                        ++end;
                    }

                    token.kind = TokenKind::String;
                    token.text = m_text.substr(start, end - start);
                    m_position = end < m_text.size() ? end + 1 : end; // skip the closing quote if there is one

                    return token;
                }

                const std::size_t start = m_position;
                while (m_position < m_text.size() && !IsSpace(m_text[m_position]) && !IsSymbol(m_text[m_position]))
                    ++m_position;

                token.kind = TokenKind::Word;
                token.text = m_text.substr(start, m_position - start);

                return token;
            }

        private:
            // Spaces and line breaks separate tokens; "//" starts a comment up to the end of the line
            // (TrenchBroom writes "// brush 0" and similar notes).
            void SkipSpacesAndComments()
            {
                while (m_position < m_text.size())
                {
                    const char character = m_text[m_position];
                    if (IsSpace(character))
                    {
                        if (character == '\n')
                            ++m_line;
                        ++m_position;
                    }
                    else if (m_text.substr(m_position, 2) == "//")
                    {
                        while (m_position < m_text.size() && m_text[m_position] != '\n')
                            ++m_position;
                    }
                    else
                    {
                        return;
                    }
                }
            }

            std::string_view m_text;
            std::size_t m_position = 0;
            int m_line = 1;
        };

        // --- Step 2: building the structures from tokens ---
        //
        // The parser follows the nesting of the file: a map is a list of entities, an entity is a list of properties and
        // brushes, a brush is a list of faces. Every function reads exactly its own part and returns an error message
        // (with the line number) as soon as the text does not look as expected.

        class Parser
        {
        public:
            explicit Parser(std::string_view text)
                : m_tokenizer(text)
            {
                Advance();
            }

            std::expected<MapData, std::string> ParseMap()
            {
                MapData map;
                while (m_current.kind != TokenKind::End)
                {
                    std::expected<MapEntity, std::string> entity = ParseEntity();
                    if (!entity.has_value())
                        return std::unexpected(entity.error());

                    map.entities.push_back(std::move(*entity));
                }

                return map;
            }

        private:
            // { "key" "value" ... { brush } ... }
            std::expected<MapEntity, std::string> ParseEntity()
            {
                if (std::expected<void, std::string> opening = ExpectSymbol("{"); !opening.has_value())
                    return std::unexpected(opening.error());

                MapEntity entity;
                while (!IsCurrentSymbol("}"))
                {
                    if (m_current.kind == TokenKind::End)
                        return std::unexpected(MakeError("the entity is not closed with '}'"));

                    if (m_current.kind == TokenKind::String)
                    {
                        // A property is two strings in a row: the key and the value.
                        std::string key(m_current.text);
                        Advance();
                        if (m_current.kind != TokenKind::String)
                            return std::unexpected(MakeError(std::format("the property \"{}\" has no value", key)));

                        entity.properties.insert_or_assign(std::move(key), std::string(m_current.text));
                        Advance();
                    }
                    else if (IsCurrentSymbol("{"))
                    {
                        std::expected<MapBrush, std::string> brush = ParseBrush();
                        if (!brush.has_value())
                            return std::unexpected(brush.error());

                        entity.brushes.push_back(std::move(*brush));
                    }
                    else
                    {
                        return std::unexpected(MakeUnexpectedTokenError("a property or a brush"));
                    }
                }
                Advance(); // the closing '}'

                return entity;
            }

            // { face face ... }
            std::expected<MapBrush, std::string> ParseBrush()
            {
                if (std::expected<void, std::string> opening = ExpectSymbol("{"); !opening.has_value())
                    return std::unexpected(opening.error());

                MapBrush brush;
                while (!IsCurrentSymbol("}"))
                {
                    if (m_current.kind == TokenKind::End)
                        return std::unexpected(MakeError("the brush is not closed with '}'"));

                    std::expected<MapFace, std::string> face = ParseFace();
                    if (!face.has_value())
                        return std::unexpected(face.error());

                    brush.faces.push_back(std::move(*face));
                }
                Advance(); // the closing '}'

                return brush;
            }

            // ( x y z ) ( x y z ) ( x y z ) texture [ ux uy uz offsetU ] [ vx vy vz offsetV ] rotation scaleU scaleV
            std::expected<MapFace, std::string> ParseFace()
            {
                MapFace face;
                for (glm::dvec3& point : face.points)
                {
                    std::expected<glm::dvec3, std::string> readPoint = ParsePoint();
                    if (!readPoint.has_value())
                        return std::unexpected(readPoint.error());

                    point = *readPoint;
                }

                if (m_current.kind != TokenKind::Word)
                    return std::unexpected(MakeUnexpectedTokenError("a texture name"));
                face.textureName = std::string(m_current.text);
                Advance();

                // The Valve 220 format writes the texture axes in square brackets. The older Quake format writes plain
                // numbers here instead; Abomination maps are always saved in the Valve format.
                if (!IsCurrentSymbol("["))
                    return std::unexpected(MakeUnexpectedTokenError("'[' (the map must be saved in the Valve 220 format)"));

                std::expected<void, std::string> axes = ParseTextureAxis(face.textureUAxis, face.textureOffsetU);
                if (axes.has_value())
                    axes = ParseTextureAxis(face.textureVAxis, face.textureOffsetV);
                if (!axes.has_value())
                    return std::unexpected(axes.error());

                for (double* value : {&face.textureRotation, &face.textureScaleU, &face.textureScaleV})
                {
                    std::expected<double, std::string> number = ParseNumber();
                    if (!number.has_value())
                        return std::unexpected(number.error());

                    *value = *number;
                }

                return face;
            }

            // ( x y z )
            std::expected<glm::dvec3, std::string> ParsePoint()
            {
                if (std::expected<void, std::string> opening = ExpectSymbol("("); !opening.has_value())
                    return std::unexpected(opening.error());

                glm::dvec3 point(0.0);
                if (std::expected<void, std::string> coordinates = ParseVector(point); !coordinates.has_value())
                    return std::unexpected(coordinates.error());

                if (std::expected<void, std::string> closing = ExpectSymbol(")"); !closing.has_value())
                    return std::unexpected(closing.error());

                return point;
            }

            // [ x y z offset ]
            std::expected<void, std::string> ParseTextureAxis(glm::dvec3& axis, double& offset)
            {
                if (std::expected<void, std::string> opening = ExpectSymbol("["); !opening.has_value())
                    return opening;

                if (std::expected<void, std::string> direction = ParseVector(axis); !direction.has_value())
                    return direction;

                std::expected<double, std::string> readOffset = ParseNumber();
                if (!readOffset.has_value())
                    return std::unexpected(readOffset.error());

                offset = *readOffset;

                return ExpectSymbol("]");
            }

            // Three numbers in a row: the x, y and z of a point or a direction.
            std::expected<void, std::string> ParseVector(glm::dvec3& vector)
            {
                // glm::dvec3::length() is the number of components of the type (3), not the length of a vector.
                for (glm::length_t component = 0; component < glm::dvec3::length(); ++component)
                {
                    std::expected<double, std::string> number = ParseNumber();
                    if (!number.has_value())
                        return std::unexpected(number.error());

                    vector[component] = *number;
                }

                return {};
            }

            std::expected<double, std::string> ParseNumber()
            {
                if (m_current.kind != TokenKind::Word)
                    return std::unexpected(MakeUnexpectedTokenError("a number"));

                // std::from_chars converts text to a number without allocating memory and without depending on the
                // language settings of the system (which could expect "0,5" instead of "0.5").
                double value = 0.0;
                const char* begin = m_current.text.data();
                const char* end = begin + m_current.text.size();
                const std::from_chars_result result = std::from_chars(begin, end, value);
                if (result.ec != std::errc() || result.ptr != end)
                    return std::unexpected(MakeUnexpectedTokenError("a number"));

                Advance();

                return value;
            }

            std::expected<void, std::string> ExpectSymbol(std::string_view symbol)
            {
                if (!IsCurrentSymbol(symbol))
                    return std::unexpected(MakeUnexpectedTokenError(std::format("'{}'", symbol)));

                Advance();

                return {};
            }

            bool IsCurrentSymbol(std::string_view symbol) const
            {
                return m_current.kind == TokenKind::Symbol && m_current.text == symbol;
            }

            void Advance()
            {
                m_current = m_tokenizer.Next();
            }

            std::string MakeError(std::string_view message) const
            {
                return std::format("Line {}: {}", m_current.line, message);
            }

            std::string MakeUnexpectedTokenError(std::string_view expected) const
            {
                if (m_current.kind == TokenKind::End)
                    return MakeError(std::format("expected {} but the file ended", expected));

                return MakeError(std::format("expected {} but found '{}'", expected, m_current.text));
            }

            Tokenizer m_tokenizer;

            // The token the parser is looking at; Advance() moves to the next one.
            Token m_current;
        };
    }

    std::expected<MapData, std::string> ParseMap(std::string_view text)
    {
        Parser parser(text);

        return parser.ParseMap();
    }

    std::expected<MapData, std::string> LoadMapFile(const std::filesystem::path& path)
    {
        const std::expected<std::string, std::string> text = Core::ReadTextFile(path);
        if (!text.has_value())
            return std::unexpected(text.error());

        std::expected<MapData, std::string> map = ParseMap(*text);
        if (!map.has_value())
            return std::unexpected(std::format("{}: {}", Core::ToUTF8String(path.filename()), map.error()));

        return map;
    }
}
