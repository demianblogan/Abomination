#pragma once

#include <cstddef>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace Abomination::Core
{
    // Reads a whole text file (a shader, a JSON configuration) into a string.
    // Returns an error message if the file does not exist or cannot be read.
    [[nodiscard]] std::expected<std::string, std::string> ReadTextFile(const std::filesystem::path& path);

    // Reads a whole binary file (an image, a font, a sound) into an array of bytes.
    // Returns an error message if the file does not exist or cannot be read.
    [[nodiscard]] std::expected<std::vector<std::byte>, std::string> ReadBinaryFile(const std::filesystem::path& path);

    // The path as UTF-8 text: for messages (the log file and the console are UTF-8) and for libraries that take UTF-8
    // paths (Dear ImGui). path.string() must not be used for them: on Windows it converts to the code page of the system
    // (Windows-1251 for Russian), so "C:\Users\Дмитрий" turns into other bytes, and a character missing from that code
    // page even throws an exception.
    [[nodiscard]] std::string ToUTF8String(const std::filesystem::path& path);
}
