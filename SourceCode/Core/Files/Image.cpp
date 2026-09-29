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
    namespace
    {
        // Owns the pixels stb_image decoded. stb_image allocates them with malloc, so they must be freed with
        // stbi_image_free (which calls free), not with delete, which unique_ptr would call by default: the second
        // template argument is the type of the function that frees them instead (a pointer to a function taking void*).
        // std::make_unique cannot be used: it creates a new object with new, while here the memory already exists.
        using STBPixels = std::unique_ptr<stbi_uc, decltype(&stbi_image_free)>;
    }

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

        // Decodes the bytes of the file into pixels: returns width * height * 4 bytes (nullptr if the file is broken) and
        // writes the size and the number of channels in the file into the three variables above. The last argument asks
        // for 4 channels in the result, whatever the file contains: RGB images get alpha 255.
        stbi_uc* pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(fileContents->data()),
                                                static_cast<int>(fileContents->size()), &width, &height,
                                                &channelCountInFile, ImageChannelCount);

        // From here on the pixels are freed automatically on every way out of the function (nothing happens for nullptr).
        const STBPixels decodedPixels(pixels, &stbi_image_free);

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
