#include "Renderer/Assets/ShaderStore.h"

#include "Core/Logging/Log.h"

#include <optional>
#include <string_view>
#include <utility>

namespace Abomination::Renderer
{
    using Core::LogCategory;
    using Core::LogLevel;

    namespace
    {
        // The fallback program is written in the code, not in files, so it is available even when the files are not.
        // It only places the vertices like every other program (a skinned mesh bent by its joints) and paints every
        // pixel magenta. It has every uniform the render system sets, so setting them never fails. GLSL text cannot use
        // the C++ constants, so its locations are written as numbers: they must match Renderer/OpenGL/ShaderInterface.h.
        constexpr std::string_view FallbackVertexShaderSource = R"(
            #version 460 core
            layout(location = 0) in vec3 aPosition;
            layout(location = 3) in uvec4 aJoints;
            layout(location = 4) in vec4 aWeights;

            layout(location = 0) uniform mat4 uniModel;
            layout(location = 1) uniform mat4 uniView;
            layout(location = 2) uniform mat4 uniProjection;
            layout(location = 3) uniform bool uniIsSkinned;

            layout(std430, binding = 0) readonly buffer JointMatrices
            {
                mat4 uniJointMatrices[];
            };

            void main()
            {
                mat4 skin = mat4(1.0);
                if (uniIsSkinned)
                {
                    skin = aWeights.x * uniJointMatrices[aJoints.x] + aWeights.y * uniJointMatrices[aJoints.y] +
                           aWeights.z * uniJointMatrices[aJoints.z] + aWeights.w * uniJointMatrices[aJoints.w];
                }
                gl_Position = uniProjection * uniView * uniModel * skin * vec4(aPosition, 1.0);
            }
        )";

        constexpr std::string_view FallbackFragmentShaderSource = R"(
            #version 460 core
            layout(location = 0) out vec4 FragColor;

            void main()
            {
                FragColor = vec4(1.0, 0.0, 1.0, 1.0);
            }
        )";

        std::expected<GLShaderProgram, std::string> CreateFallbackProgram()
        {
            return GLShaderProgram::Create(FallbackVertexShaderSource, FallbackFragmentShaderSource, "Fallback");
        }
    }

    std::expected<ShaderStore, std::string> ShaderStore::Create(std::filesystem::path assetsDirectory)
    {
        std::expected<GLShaderProgram, std::string> fallbackProgram = CreateFallbackProgram();
        if (!fallbackProgram.has_value())
            return std::unexpected(fallbackProgram.error());

        return ShaderStore(std::move(assetsDirectory), std::move(*fallbackProgram));
    }

    ShaderStore::ShaderStore(std::filesystem::path assetsDirectory, GLShaderProgram fallbackProgram) noexcept
        : m_assetsDirectory(std::move(assetsDirectory))
        , m_fallbackProgram(std::move(fallbackProgram))
    {}

    ShaderHandle ShaderStore::Load(const std::string& name)
    {
        if (const std::optional<ShaderHandle> loadedHandle = m_cache.Find(name); loadedHandle.has_value())
            return *loadedHandle;

        // "Shaders/TexturedShaded" -> ".../Assets/Shaders/TexturedShaded.vert" and ".../Assets/Shaders/TexturedShaded.frag".
        // += appends text to the last part of a path (unlike /, which adds a new part). make_preferred() turns the forward
        // slashes of the name into the backslashes of Windows, so paths in log messages do not mix both.
        std::filesystem::path vertexShaderPath = m_assetsDirectory / name;
        vertexShaderPath.make_preferred();
        std::filesystem::path fragmentShaderPath = vertexShaderPath;
        vertexShaderPath += ".vert";
        fragmentShaderPath += ".frag";

        std::expected<GLShaderProgram, std::string> program =
            GLShaderProgram::CreateFromFiles(vertexShaderPath, fragmentShaderPath);

        if (program.has_value())
        {
            Core::Log::Write(LogCategory::Renderer, LogLevel::Debug, "Shader program loaded: {}", name);

            return m_cache.Add(name, std::move(*program));
        }

        Core::Log::Write(LogCategory::Renderer, LogLevel::Error, "Shader program {} replaced by the fallback: {}", name,
                         program.error());

        // The fallback compiled when the store was created, so it compiles now too. If it still failed, the name
        // stays unloaded: Load() then returns an invalid handle, and Get() gives m_fallbackProgram for it anyway.
        std::expected<GLShaderProgram, std::string> fallbackProgram = CreateFallbackProgram();
        if (!fallbackProgram.has_value())
            return ShaderHandle{};

        m_fallbackNames.insert(name);

        return m_cache.Add(name, std::move(*fallbackProgram));
    }

    const GLShaderProgram& ShaderStore::Get(ShaderHandle handle) const
    {
        const GLShaderProgram* program = m_cache.Get(handle);
        if (program == nullptr)
            return m_fallbackProgram;

        return *program;
    }

    std::size_t ShaderStore::GetCount() const noexcept
    {
        return m_cache.GetCount();
    }

    const std::string* ShaderStore::GetName(ShaderHandle handle) const
    {
        return m_cache.GetPath(handle);
    }
}
