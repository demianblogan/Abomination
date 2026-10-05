#include "Core/Files/Image.h"

#include "Core/Files/FileSystem.h"

// stb_image is a "single-header" library: the header contains both the declarations and the implementation.
// The implementation is compiled only where STB_IMAGE_IMPLEMENTATION is defined, and that must be exactly one
// .cpp file in the whole program. This is that file.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// The same for stb_image_write, which writes PNG files (SaveImageFile). The tests use it too, from here.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <format>
#include <fstream>
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

        return DecodeImage(*fileContents, ToUTF8String(path));
    }

    std::expected<Image, std::string> DecodeImage(std::span<const std::byte> bytes, std::string_view name)
    {
        // Flip the rows while decoding, so the bottom row of the picture comes first (see Image).
        // The "_thread" version changes the setting only for the current thread, not for the whole program.
        stbi_set_flip_vertically_on_load_thread(1);

        int width = 0;
        int height = 0;
        int channelCountInFile = 0;

        // Decodes the bytes of the file into pixels: returns width * height * 4 bytes (nullptr if the file is broken) and
        // writes the size and the number of channels in the file into the three variables above. The last argument asks
        // for 4 channels in the result, whatever the file contains: RGB images get alpha 255.
        stbi_uc* pixels = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(bytes.data()), static_cast<int>(bytes.size()),
                                                &width, &height, &channelCountInFile, ImageChannelCount);

        // From here on the pixels are freed automatically on every way out of the function (nothing happens for nullptr).
        const STBPixels decodedPixels(pixels, &stbi_image_free);

        if (decodedPixels == nullptr)
            return std::unexpected(std::format("Failed to decode the image \"{}\": {}", name, stbi_failure_reason()));

        const std::size_t byteCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * ImageChannelCount;

        return Image{
            .width = width,
            .height = height,
            .pixels = std::vector<std::uint8_t>(decodedPixels.get(), decodedPixels.get() + byteCount),
        };
    }

    std::expected<void, std::string> SaveImageFile(const std::filesystem::path& path, const Image& image)
    {
        // Writing past the end of the pixels would read memory that is not the image.
        assert(image.width > 0 && image.height > 0);
        assert(image.pixels.size() == static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height) *
                                          ImageChannelCount);

        // The file starts with the top row, the image with the bottom one: the rows are copied in the other order. (stb has
        // a switch for this, but it is global and would flip every other write too.)
        const std::size_t rowSize = static_cast<std::size_t>(image.width) * ImageChannelCount;
        std::vector<std::uint8_t> topRowFirst(image.pixels.size());
        for (std::size_t row = 0; row < static_cast<std::size_t>(image.height); ++row)
        {
            const std::size_t sourceRow = static_cast<std::size_t>(image.height) - 1 - row;
            std::copy_n(image.pixels.begin() + static_cast<std::ptrdiff_t>(sourceRow * rowSize), rowSize,
                        topRowFirst.begin() + static_cast<std::ptrdiff_t>(row * rowSize));
        }

        // stb_image_write encodes into memory through a callback; the file is written here, so a path with any letters
        // works (stb would open it with the code page of the system).
        std::vector<char> encoded;
        const auto append = [](void* context, void* data, int size)
        {
            auto* bytes = static_cast<std::vector<char>*>(context);
            bytes->insert(bytes->end(), static_cast<char*>(data), static_cast<char*>(data) + size);
        };
        const int strideInBytes = image.width * ImageChannelCount;
        const bool isEncoded = stbi_write_png_to_func(append, &encoded, image.width, image.height, ImageChannelCount,
                                                      topRowFirst.data(), strideInBytes) != 0;
        if (!isEncoded)
            return std::unexpected(std::format("Failed to encode the image \"{}\"", ToUTF8String(path)));

        std::ofstream file(path, std::ios::binary);
        file.write(encoded.data(), static_cast<std::streamsize>(encoded.size()));
        if (!file.good())
            return std::unexpected(std::format("Failed to write the image \"{}\"", ToUTF8String(path)));

        return {};
    }
}
