#include "Core/FileSystem.h"

#include <format>
#include <fstream>
#include <ios>

namespace Abomination::Core
{
    namespace
    {
        // Reads the whole file into any contiguous container of one-byte elements (std::string, std::vector<std::byte>).
        template <typename Container>
        std::expected<Container, std::string> ReadWholeFile(const std::filesystem::path& path)
        {
            // std::ios::binary: the bytes are read exactly as they are. In text mode Windows would turn "\r\n" into "\n",
            // and the size measured below would no longer match the number of characters read.
            // std::ios::ate ("at end"): the file opens with the read position at its end, so tellg() returns its size.
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file.is_open())
                return std::unexpected(std::format("Failed to open the file \"{}\"", ToUTF8String(path)));

            const std::streamsize size = file.tellg();
            if (size < 0)
                return std::unexpected(std::format("Failed to get the size of the file \"{}\"", ToUTF8String(path)));

            Container contents(static_cast<std::size_t>(size), typename Container::value_type{});

            // Go back to the beginning and read everything in one call.
            file.seekg(0, std::ios::beg);
            if (!file.read(reinterpret_cast<char*>(contents.data()), size))
                return std::unexpected(std::format("Failed to read the file \"{}\"", ToUTF8String(path)));

            return contents;
        }
    }

    std::expected<std::string, std::string> ReadTextFile(const std::filesystem::path& path)
    {
        return ReadWholeFile<std::string>(path);
    }

    std::expected<std::vector<std::byte>, std::string> ReadBinaryFile(const std::filesystem::path& path)
    {
        return ReadWholeFile<std::vector<std::byte>>(path);
    }

    std::string ToUTF8String(const std::filesystem::path& path)
    {
        // u8string() gives the path in UTF-8 as std::u8string, a string of char8_t. The bytes are exactly the ones needed;
        // only the character type differs, so they are copied into an ordinary std::string.
        const std::u8string text = path.u8string();

        return std::string(text.begin(), text.end());
    }
}
