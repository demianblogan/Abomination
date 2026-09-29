#include "Renderer/Assets/GLTFLoader.h"

#include "Core/Files/FileSystem.h"

// cgltf is a "single-header" C library, like stb_image: its implementation is compiled only where CGLTF_IMPLEMENTATION
// is defined, in exactly one .cpp file of the program. This is that file.
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

#include <cstdlib>
#include <cstring>
#include <format>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

// How a glTF file is organized, from the top down (only the parts read here):
//   node      - a place in the tree of the model: moved, turned and scaled relative to its parent; may have a mesh.
//   mesh      - one or more primitives.
//   primitive - triangles with one material: its vertex attributes (position, normal, texture coordinates) and indices.
//   accessor  - how to read one attribute from the binary data: where it starts, how many elements, of which type.
//   material  - how a surface looks; here only its base color texture is used.
//   image     - a picture (PNG, JPG) stored in the binary data of the file or in a file next to it.
// cgltf parses the JSON part and links everything with pointers; the code below walks these pointers.
namespace Abomination::Renderer
{
    namespace
    {
        // cgltf reads files (the .bin and images of a .gltf) through these two functions instead of fopen(), so paths
        // with non-English characters work: cgltf gives paths as UTF-8, and our ReadBinaryFile handles them.
        cgltf_result ReadFile(const cgltf_memory_options*, const cgltf_file_options*, const char* path, cgltf_size* size,
                              void** data)
        {
            const std::expected<std::vector<std::byte>, std::string> contents =
                Core::ReadBinaryFile(std::filesystem::path(reinterpret_cast<const char8_t*>(path)));
            if (!contents.has_value())
                return cgltf_result_file_not_found;

            // cgltf frees the data with ReleaseFile below, so it is allocated with malloc, not new.
            *data = std::malloc(contents->size());
            if (*data == nullptr)
                return cgltf_result_out_of_memory;

            std::memcpy(*data, contents->data(), contents->size());
            *size = contents->size();

            return cgltf_result_success;
        }

        void ReleaseFile(const cgltf_memory_options*, const cgltf_file_options*, void* data)
        {
            std::free(data);
        }

        // Frees what cgltf_parse() allocated when it goes out of scope (see STBPixels in Image.cpp for the pattern).
        using ParsedGLTF = std::unique_ptr<cgltf_data, decltype(&cgltf_free)>;

        // Finds the attribute of a primitive, or nullptr if it has none. index tells TEXCOORD_0 from TEXCOORD_1.
        const cgltf_accessor* FindAttribute(const cgltf_primitive& primitive, cgltf_attribute_type type, int index = 0)
        {
            for (const cgltf_attribute& attribute : std::span(primitive.attributes, primitive.attributes_count))
                if (attribute.type == type && attribute.index == index)
                    return attribute.data;

            return nullptr;
        }

        // Reads the triangles of one primitive into mesh data, in the coordinates of its node.
        std::expected<MeshData, std::string> ReadPrimitive(const cgltf_primitive& primitive, bool isMirrored)
        {
            if (primitive.type != cgltf_primitive_type_triangles)
                return std::unexpected("only triangles are supported");

            const cgltf_accessor* positions = FindAttribute(primitive, cgltf_attribute_type_position);
            if (positions == nullptr)
                return std::unexpected("a primitive has no positions");

            const cgltf_accessor* normals = FindAttribute(primitive, cgltf_attribute_type_normal);
            const cgltf_accessor* texCoords = FindAttribute(primitive, cgltf_attribute_type_texcoord);

            MeshData mesh;
            mesh.vertices.resize(positions->count);
            for (cgltf_size index = 0; index < positions->count; ++index)
            {
                // cgltf_accessor_read_float converts whatever the file stores (floats, or normalized integers) to floats.
                MeshVertex& vertex = mesh.vertices[index];
                cgltf_accessor_read_float(positions, index, glm::value_ptr(vertex.position), 3);
                if (normals != nullptr)
                    cgltf_accessor_read_float(normals, index, glm::value_ptr(vertex.normal), 3);

                if (texCoords != nullptr)
                {
                    // glTF puts the texture coordinate (0, 0) at the top left corner of the image. Our images are stored
                    // with the bottom row first (see Core::Image), where v = 0 is the bottom, so v is turned around.
                    cgltf_accessor_read_float(texCoords, index, glm::value_ptr(vertex.texCoord), 2);
                    vertex.texCoord.y = 1.0f - vertex.texCoord.y;
                }
            }

            // Without indices every three vertices in a row are a triangle.
            const cgltf_size indexCount = primitive.indices != nullptr ? primitive.indices->count : positions->count;
            mesh.indices.resize(indexCount);
            for (cgltf_size index = 0; index < indexCount; ++index)
                mesh.indices[index] = primitive.indices != nullptr
                                          ? static_cast<std::uint32_t>(cgltf_accessor_read_index(primitive.indices, index))
                                          : static_cast<std::uint32_t>(index);

            // A mirroring node transform (an odd number of negative scales) turns counter-clockwise triangles clockwise,
            // and face culling would then hide their fronts. Swapping two corners of every triangle turns them back.
            if (isMirrored)
                for (std::size_t triangle = 0; triangle + 2 < mesh.indices.size(); triangle += 3)
                    std::swap(mesh.indices[triangle + 1], mesh.indices[triangle + 2]);

            return mesh;
        }

        // Decodes one image of the file: from the binary data of a .glb, or from a file next to a .gltf.
        std::expected<Core::Image, std::string> ReadImage(const cgltf_image& image, const std::filesystem::path& modelPath,
                                                          std::string_view name)
        {
            if (image.buffer_view != nullptr)
            {
                const auto* bytes = reinterpret_cast<const std::byte*>(cgltf_buffer_view_data(image.buffer_view));
                return Core::DecodeImage(std::span(bytes, image.buffer_view->size), name);
            }

            if (image.uri == nullptr || std::string_view(image.uri).starts_with("data:"))
                return std::unexpected(std::format("the image {} is not stored in a supported way", name));

            // A relative path, with special characters written as %xx ("My%20Texture.png"); cgltf_decode_uri turns them
            // back in place, so a copy is decoded.
            std::string uri = image.uri;
            uri.resize(cgltf_decode_uri(uri.data()));

            const std::filesystem::path relativePath(reinterpret_cast<const char8_t*>(uri.c_str()));
            return Core::LoadImageFile(modelPath.parent_path() / relativePath);
        }
    }

    std::expected<ModelData, std::string> LoadGLTFFile(const std::filesystem::path& path)
    {
        const std::string pathText = Core::ToUTF8String(path);

        const std::expected<std::vector<std::byte>, std::string> fileContents = Core::ReadBinaryFile(path);
        if (!fileContents.has_value())
            return std::unexpected(fileContents.error());

        cgltf_options options{};
        options.file.read = &ReadFile;
        options.file.release = &ReleaseFile;

        // 1. The JSON part: the structure of the model. The file type (.glb or .gltf) is recognized by the contents.
        cgltf_data* rawData = nullptr;
        if (cgltf_parse(&options, fileContents->data(), fileContents->size(), &rawData) != cgltf_result_success)
            return std::unexpected(std::format("not a glTF 2.0 file: {}", pathText));
        const ParsedGLTF data(rawData, &cgltf_free);

        // 2. The binary data the accessors point into: the second part of a .glb, or the .bin file of a .gltf.
        if (cgltf_load_buffers(&options, data.get(), pathText.c_str()) != cgltf_result_success)
            return std::unexpected(std::format("the binary data of the model could not be read: {}", pathText));

        // 3. Checks that every accessor stays inside its buffer, so reading them below cannot go out of bounds.
        if (cgltf_validate(data.get()) != cgltf_result_success)
            return std::unexpected(std::format("the model is broken: {}", pathText));

        ModelData model;

        // The images first, so the parts can refer to them by index. A broken image is left empty; the part then gets
        // the fallback texture.
        for (const cgltf_image& image : std::span(data->images, data->images_count))
        {
            const std::string name = std::format("{}#image{}", pathText, model.images.size());
            std::expected<Core::Image, std::string> decoded = ReadImage(image, path, name);
            model.images.push_back(decoded.has_value() ? std::move(*decoded) : Core::Image{});
        }

        // Every node with a mesh gives one part per primitive.
        for (const cgltf_node& node : std::span(data->nodes, data->nodes_count))
        {
            if (node.mesh == nullptr)
                continue;

            // The whole chain of transforms from the root of the tree to this node, as one matrix (column-major, like
            // glm). A negative determinant means the transform mirrors the part (see ReadPrimitive).
            glm::mat4 transform(1.0f);
            cgltf_node_transform_world(&node, glm::value_ptr(transform));
            const bool isMirrored = glm::determinant(glm::mat3(transform)) < 0.0f;

            const std::span primitives(node.mesh->primitives, node.mesh->primitives_count);
            for (std::size_t primitiveIndex = 0; primitiveIndex < primitives.size(); ++primitiveIndex)
            {
                const cgltf_primitive& primitive = primitives[primitiveIndex];
                std::expected<MeshData, std::string> mesh = ReadPrimitive(primitive, isMirrored);
                if (!mesh.has_value())
                    return std::unexpected(std::format("{}: {}", pathText, mesh.error()));

                // A mesh of one primitive is named like the node; more primitives get a number each.
                std::string name = node.name != nullptr ? node.name : std::format("part{}", model.parts.size());
                if (primitives.size() > 1)
                    name += std::format("#{}", primitiveIndex);

                ModelPartData part{.name = std::move(name), .mesh = std::move(*mesh), .transform = transform};

                // The base color texture of the material, if it has one, as an index into model.images.
                const cgltf_material* material = primitive.material;
                if (material != nullptr && material->has_pbr_metallic_roughness)
                {
                    const cgltf_texture* texture = material->pbr_metallic_roughness.base_color_texture.texture;
                    if (texture != nullptr && texture->image != nullptr)
                        part.imageIndex = static_cast<std::size_t>(texture->image - data->images);
                }

                model.parts.push_back(std::move(part));
            }
        }

        if (model.parts.empty())
            return std::unexpected(std::format("the model has no meshes: {}", pathText));

        return model;
    }
}
