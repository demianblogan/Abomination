#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace Abomination::Core
{
    // An image decoded from a file (PNG, JPG, TGA, BMP), ready to be uploaded into a texture.
    // Every pixel takes 4 bytes: red, green, blue, alpha (0-255 each), even if the file had no alpha.
    // The rows are stored from the BOTTOM row of the picture to the TOP row: OpenGL expects the first row of a
    // texture to be the bottom one, while image files store the top row first, so the rows are flipped on load.
    struct Image
    {
        int width = 0;
        int height = 0;
        std::vector<std::uint8_t> pixels;
    };

    // Bytes per pixel in Image::pixels.
    inline constexpr int ImageChannelCount = 4;

    // Reads and decodes an image file. Returns an error message if the file cannot be read or is not an image.
    // (Not called LoadImage: windows.h defines a macro with that name.)
    [[nodiscard]] std::expected<Image, std::string> LoadImageFile(const std::filesystem::path& path);
}
