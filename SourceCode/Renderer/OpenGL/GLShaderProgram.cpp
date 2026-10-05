#include "Renderer/OpenGL/GLShaderProgram.h"

#include "Core/Files/FileSystem.h"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include <format>
#include <utility>

namespace Abomination::Renderer
{
    // The ID is stored as std::uint32_t, so the header does not need GLAD. Both types must be the same size.
    static_assert(sizeof(GLuint) == sizeof(std::uint32_t));

    namespace
    {
        // The compiler and linker write their messages (errors, warnings) into a log stored with the object.
        std::string GetShaderInfoLog(GLuint shaderID)
        {
            GLint logLength = 0;
            glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLength);

            // The length counts the zero at the end of the text; the length written does not, so the string is cut to it.
            // Otherwise the zero would end up in the log file.
            std::string log(static_cast<std::size_t>(logLength), '\0');
            GLsizei writtenLength = 0;
            glGetShaderInfoLog(shaderID, logLength, &writtenLength, log.data());
            log.resize(static_cast<std::size_t>(writtenLength));

            return log;
        }

        std::string GetProgramInfoLog(GLuint programID)
        {
            GLint logLength = 0;
            glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &logLength);

            // Cut to the length written, without the zero at the end (see GetShaderInfoLog).
            std::string log(static_cast<std::size_t>(logLength), '\0');
            GLsizei writtenLength = 0;
            glGetProgramInfoLog(programID, logLength, &writtenLength, log.data());
            log.resize(static_cast<std::size_t>(writtenLength));

            return log;
        }

        // Compiles one shader stage (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER). The driver compiles GLSL itself,
        // while the game is running; errors are known only now, not when the game is built.
        std::expected<GLuint, std::string> CompileShader(GLenum stage, std::string_view source, std::string_view debugName)
        {
            const GLuint shaderID = glCreateShader(stage);

            // glShaderSource accepts an array of strings; we pass one string with an explicit length,
            // so it does not have to end with '\0'.
            const GLchar* sourceText = source.data();
            const GLint sourceLength = static_cast<GLint>(source.size());
            glShaderSource(shaderID, 1, &sourceText, &sourceLength);
            glCompileShader(shaderID);

            GLint compileStatus = GL_FALSE;
            glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compileStatus);
            if (compileStatus == GL_FALSE)
            {
                const std::string log = GetShaderInfoLog(shaderID);
                glDeleteShader(shaderID);

                const std::string_view stageName = stage == GL_VERTEX_SHADER ? "vertex" : "fragment";

                return std::unexpected(
                    std::format("Failed to compile the {} shader of \"{}\":\n{}", stageName, debugName, log));
            }

            return shaderID;
        }
    }

    std::expected<GLShaderProgram, std::string> GLShaderProgram::Create(std::string_view vertexShaderSource,
                                                                        std::string_view fragmentShaderSource,
                                                                        std::string_view debugName)
    {
        const std::expected<GLuint, std::string> vertexShaderID =
            CompileShader(GL_VERTEX_SHADER, vertexShaderSource, debugName);
        if (!vertexShaderID.has_value())
            return std::unexpected(vertexShaderID.error());

        const std::expected<GLuint, std::string> fragmentShaderID =
            CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource, debugName);
        if (!fragmentShaderID.has_value())
        {
            glDeleteShader(*vertexShaderID);

            return std::unexpected(fragmentShaderID.error());
        }

        // Linking connects the stages: the outputs of the vertex shader to the inputs of the fragment shader.
        const GLuint programID = glCreateProgram();
        glAttachShader(programID, *vertexShaderID);
        glAttachShader(programID, *fragmentShaderID);
        glLinkProgram(programID);

        // After linking the program contains the compiled code of both stages, so the shader objects are not needed:
        // they are detached and deleted right away.
        glDetachShader(programID, *vertexShaderID);
        glDetachShader(programID, *fragmentShaderID);
        glDeleteShader(*vertexShaderID);
        glDeleteShader(*fragmentShaderID);

        GLint linkStatus = GL_FALSE;
        glGetProgramiv(programID, GL_LINK_STATUS, &linkStatus);
        if (linkStatus == GL_FALSE)
        {
            const std::string log = GetProgramInfoLog(programID);
            glDeleteProgram(programID);

            return std::unexpected(std::format("Failed to link the shader program \"{}\":\n{}", debugName, log));
        }

        // A human-readable name for the object: it is shown in debug output messages and in graphics debuggers.
        glObjectLabel(GL_PROGRAM, programID, static_cast<GLsizei>(debugName.size()), debugName.data());

        return GLShaderProgram(programID);
    }

    std::expected<GLShaderProgram, std::string> GLShaderProgram::CreateFromFiles(
        const std::filesystem::path& vertexShaderPath, const std::filesystem::path& fragmentShaderPath)
    {
        const std::expected<std::string, std::string> vertexShaderSource = Core::ReadTextFile(vertexShaderPath);
        if (!vertexShaderSource.has_value())
            return std::unexpected(vertexShaderSource.error());

        const std::expected<std::string, std::string> fragmentShaderSource = Core::ReadTextFile(fragmentShaderPath);
        if (!fragmentShaderSource.has_value())
            return std::unexpected(fragmentShaderSource.error());

        const std::string debugName = std::format("{} + {}", Core::ToUTF8String(vertexShaderPath.filename()),
                                                  Core::ToUTF8String(fragmentShaderPath.filename()));

        return Create(*vertexShaderSource, *fragmentShaderSource, debugName);
    }

    GLShaderProgram::GLShaderProgram(std::uint32_t programID) noexcept
        : m_programID(programID)
    {}

    GLShaderProgram::GLShaderProgram(GLShaderProgram&& other) noexcept
        : m_programID(std::exchange(other.m_programID, 0))
    {}

    GLShaderProgram& GLShaderProgram::operator=(GLShaderProgram&& other) noexcept
    {
        if (this != &other)
        {
            // glDelete* functions ignore the ID 0, so a moved-from object needs no special check.
            glDeleteProgram(m_programID);
            m_programID = std::exchange(other.m_programID, 0);
        }

        return *this;
    }

    GLShaderProgram::~GLShaderProgram()
    {
        glDeleteProgram(m_programID);
    }

    void GLShaderProgram::Use() const
    {
        glUseProgram(m_programID);
    }

    void GLShaderProgram::SetUniform(std::uint32_t location, const glm::mat4& value) const
    {
        // Direct State Access: sets the uniform of this program by its ID, the program does not have to be in use.
        // (The old way: glUseProgram(id) first, then glUniformMatrix4fv.)
        // 1 - one matrix; GL_FALSE - do not transpose: glm already stores matrices column by column, as OpenGL expects;
        // glm::value_ptr - a pointer to the 16 floats of the matrix.
        glProgramUniformMatrix4fv(m_programID, static_cast<GLint>(location), 1, GL_FALSE, glm::value_ptr(value));
    }

    void GLShaderProgram::SetUniform(std::uint32_t location, const glm::vec2& value) const
    {
        glProgramUniform2f(m_programID, static_cast<GLint>(location), value.x, value.y);
    }

    void GLShaderProgram::SetUniform(std::uint32_t location, float value) const
    {
        glProgramUniform1f(m_programID, static_cast<GLint>(location), value);
    }

    void GLShaderProgram::SetUniform(std::uint32_t location, bool value) const
    {
        // GLSL has no function for bools: a bool uniform is set as an integer, 0 or 1.
        glProgramUniform1i(m_programID, static_cast<GLint>(location), value ? 1 : 0);
    }
}
