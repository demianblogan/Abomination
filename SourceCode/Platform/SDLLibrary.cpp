#include "Platform/SDLLibrary.h"

#include "Core/Logging/Log.h"

#include <SDL3/SDL.h>

#include <format>
#include <utility>

namespace Abomination::Platform
{
    using Core::LogCategory;
    using Core::LogLevel;

    std::expected<SDLLibrary, std::string> SDLLibrary::Initialize()
    {
        // The video subsystem is enough for now: it creates windows and OpenGL contexts and also starts the events subsystem.
        if (!SDL_Init(SDL_INIT_VIDEO))
            return std::unexpected(std::format("Failed to initialize SDL: {}", SDL_GetError()));

        const int version = SDL_GetVersion();
        Core::Log::Write(LogCategory::Platform, LogLevel::Info, "SDL {}.{}.{} initialized", SDL_VERSIONNUM_MAJOR(version),
                         SDL_VERSIONNUM_MINOR(version), SDL_VERSIONNUM_MICRO(version));

        SDLLibrary library;
        library.m_isActive = true;

        return library;
    }

    SDLLibrary::SDLLibrary(SDLLibrary&& other) noexcept
        : m_isActive(std::exchange(other.m_isActive, false))
    {}

    SDLLibrary& SDLLibrary::operator=(SDLLibrary&& other) noexcept
    {
        if (this != &other)
        {
            if (m_isActive)
                SDL_Quit();

            m_isActive = std::exchange(other.m_isActive, false);
        }

        return *this;
    }

    SDLLibrary::~SDLLibrary()
    {
        if (m_isActive)
        {
            SDL_Quit();
            Core::Log::Write(LogCategory::Platform, LogLevel::Info, "SDL shut down");
        }
    }
}
