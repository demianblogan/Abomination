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

        // The uniforms of a material and of the light (Lit.frag) are declared and used too: a uniform a program does not
        // use is removed by the compiler, and setting it is then an OpenGL error. They are used in a way the compiler
        // cannot remove but that changes nothing: only an absurd sum (above 1e30) would add anything to the magenta.
        constexpr std::string_view FallbackFragmentShaderSource = R"(
            #version 460 core
            layout(location = 4) uniform vec4 uniBaseColorFactor;
            layout(location = 5) uniform float uniRoughnessFactor;
            layout(location = 6) uniform float uniMetalnessFactor;
            layout(location = 7) uniform vec3 uniEmissiveFactor;
            layout(location = 8) uniform vec3 uniSunDirection;
            layout(location = 9) uniform vec3 uniSunColor;
            layout(location = 10) uniform vec3 uniAmbientColor;
            layout(location = 11) uniform int uniShadingView;
            layout(location = 12) uniform float uniParallaxDepth;
            layout(location = 13) uniform bool uniIsSpecularAntiAliasingEnabled;
            layout(location = 14) uniform int uniParallaxStepCount;

            layout(location = 0) out vec4 FragColor;

            void main()
            {
                float sum = uniBaseColorFactor.x + uniRoughnessFactor + uniMetalnessFactor + uniEmissiveFactor.x +
                            uniSunDirection.x + uniSunColor.x + uniAmbientColor.x + float(uniShadingView) +
                            uniParallaxDepth + float(uniIsSpecularAntiAliasingEnabled) + float(uniParallaxStepCount);
                FragColor = vec4(1.0, 0.0, 1.0, 1.0) + vec4(step(1e30, abs(sum)));
            }
        )";

        std::expected<GLShaderProgram, std::string> CreateFallbackProgram()
        {
            return GLShaderProgram::Create(FallbackVertexShaderSource, FallbackFragmentShaderSource, "Fallback");
        }

        // The two files of a program.
        struct ShaderFilePaths
        {
            std::filesystem::path vertexShader;
            std::filesystem::path fragmentShader;
        };

        // "Shaders/Lit" -> ".../Assets/Shaders/Lit.vert" and ".../Assets/Shaders/Lit.frag".
        // += appends text to the last part of a path (unlike /, which adds a new part). make_preferred() turns the forward
        // slashes of the name into the backslashes of Windows, so paths in log messages do not mix both.
        ShaderFilePaths GetShaderFilePaths(const std::filesystem::path& assetsDirectory, const std::string& name)
        {
            std::filesystem::path vertexShaderPath = assetsDirectory / name;
            vertexShaderPath.make_preferred();
            std::filesystem::path fragmentShaderPath = vertexShaderPath;
            vertexShaderPath += ".vert";
            fragmentShaderPath += ".frag";

            return ShaderFilePaths{.vertexShader = vertexShaderPath, .fragmentShader = fragmentShaderPath};
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

        // The files are watched from now on, also when they are broken: fixing them replaces the fallback (see
        // ReloadChangedPrograms).
        const ShaderFilePaths paths = GetShaderFilePaths(m_assetsDirectory, name);
        m_fileWatcher.Watch(name, {paths.vertexShader, paths.fragmentShader});

        std::expected<GLShaderProgram, std::string> program =
            GLShaderProgram::CreateFromFiles(paths.vertexShader, paths.fragmentShader);

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

    int ShaderStore::ReloadChangedPrograms()
    {
        int reloadedCount = 0;
        for (const std::string& name : m_fileWatcher.CollectChangedKeys())
        {
            const ShaderFilePaths paths = GetShaderFilePaths(m_assetsDirectory, name);
            std::expected<GLShaderProgram, std::string> program =
                GLShaderProgram::CreateFromFiles(paths.vertexShader, paths.fragmentShader);
            if (!program.has_value())
            {
                // A typo in the middle of editing must not turn the world magenta: the old program keeps drawing.
                Core::Log::Write(LogCategory::Renderer, LogLevel::Error,
                                 "Shader program {} not reloaded, the old one stays: {}", name, program.error());
                continue;
            }

            // Adding a loaded name replaces its program and keeps its handle (see Core::AssetCache::Add).
            m_cache.Add(name, std::move(*program));
            m_fallbackNames.erase(name);
            Core::Log::Write(LogCategory::Renderer, LogLevel::Info, "Shader program reloaded: {}", name);
            ++reloadedCount;
        }

        return reloadedCount;
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
