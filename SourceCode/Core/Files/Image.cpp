#include "Core/Files/Image.h"

#include "Core/Files/FileSystem.h"

// stb_image is a "single-header" library: the header contains both the declarations and the implementation.
// The implementation is compiled only where STB_IMAGE_IMPLEMENTATION is defined, and that must be exactly one
// .cpp file in the whole program. This is that file.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstddef>
#include <format>
#include <memory>

namespace Abomination::Core
{
    std::expected<Image, std::string> LoadImageFile(const std::filesystem::path& path)
    {
        // The file is read by our own function (it handles any path, including non-English characters),
        // and stb_image only decodes the bytes in memory.
        const std::expected<std::vector<std::byte>, std::string> fileContents = ReadBinaryFile(path);
        if (!fileContents.has_value())
            return std::unexpected(fileContents.error());

        // Flip the rows while decoding, so the bottom row of the picture comes first (see Image).
        // The "_thread" version changes the setting only for the current thread, not for the whole program.
        stbi_set_flip_vertically_on_load_thread(1);

        int width = 0;
        int height = 0;
        int channelCountInFile = 0;

        // The last argument asks for 4 channels in the result, whatever the file contains: RGB images get alpha 255.
        // stb_image allocates the pixels with malloc; unique_ptr with stbi_image_free releases them automatically.
        std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> decodedPixels(
            stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(fileContents->data()),
                                  static_cast<int>(fileContents->size()), &width, &height, &channelCountInFile,
                                  ImageChannelCount),
            &stbi_image_free);

        if (decodedPixels == nullptr)
            return std::unexpected(
                std::format("Failed to decode the image \"{}\": {}", ToUTF8String(path), stbi_failure_reason()));

        const std::size_t byteCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * ImageChannelCount;

        return Image{
            .width = width,
            .height = height,
            .pixels = std::vector<std::uint8_t>(decodedPixels.get(), decodedPixels.get() + byteCount),
        };
    }
}
