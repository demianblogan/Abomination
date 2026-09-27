#include "UI/ImGuiLibrary.h"

#include "Core/FileSystem.h"
#include "Core/Log.h"

#include <imgui.h>

#include <filesystem>
#include <utility>

namespace Abomination::UI
{
    using Core::LogCategory;
    using Core::LogLevel;

    namespace
    {
        // Adds the font ImGui draws all its text with. The first font added becomes the default one, so this must be
        // done before the first frame. A missing or broken font file is not fatal for a debug tool: the built-in
        // vector font is used instead, and a warning goes to the log.
        void AddDebugUIFont(const std::filesystem::path& fontPath, float fontSize)
        {
            ImFontAtlas* fonts = ImGui::GetIO().Fonts;

            // AddFontFromFileTTF stops the program with an assertion if the file does not exist, so check first.
            if (std::filesystem::exists(fontPath))
            {
                if (fonts->AddFontFromFileTTF(Core::ToUTF8String(fontPath).c_str(), fontSize) != nullptr)
                    return;
            }

            Core::Log::Write(LogCategory::UI, LogLevel::Warning, "Failed to load the font \"{}\", using the built-in font",
                             Core::ToUTF8String(fontPath));

            // The built-in vector font stays sharp at any size, unlike the built-in pixel font (sharp only at 13 px).
            fonts->AddFontDefaultVector();
        }

        // Sets the whole ImGui style for a scale: the dark colors, the font size and all paddings, spacings and borders.
        // ScaleAllSizes() multiplies the current sizes, so calling it twice would scale twice: the style is reset to the
        // defaults first, and every change of scale starts from the same base.
        // FontScaleDpi scales all text; ImGui 1.92 draws the font anew at the needed size, so large text stays sharp.
        void ApplyStyle(float fontSize, float scale)
        {
            ImGuiStyle& style = ImGui::GetStyle();
            style = ImGuiStyle();
            ImGui::StyleColorsDark();
            style.FontSizeBase = fontSize;
            style.ScaleAllSizes(scale);
            style.FontScaleDpi = scale;
        }
    }

    std::expected<ImGuiLibrary, std::string> ImGuiLibrary::Initialize(const std::filesystem::path& fontPath, float fontSize,
                                                                     std::filesystem::path settingsPath)
    {
        // Checks that the ImGui headers we compile with match the compiled ImGui library.
        IMGUI_CHECKVERSION();

        if (ImGui::CreateContext() == nullptr)
            return std::unexpected("Failed to create the ImGui context");

        // Window settings. By default ImGui reads and writes imgui.ini by itself, through IniFilename: a plain
        // const char* it keeps and uses at any time. Our ImGuiLibrary is moved (into DebugOverlay, then into
        // Application), and a pointer into a moved string could be left pointing at freed memory. So ImGui's own file
        // handling is turned off (nullptr), and the file is loaded here and saved by SaveSettingsIfChanged() instead.
        ImGui::GetIO().IniFilename = nullptr;
        if (std::filesystem::exists(settingsPath))
            ImGui::LoadIniSettingsFromDisk(Core::ToUTF8String(settingsPath).c_str());

        AddDebugUIFont(fontPath, fontSize);
        ApplyStyle(fontSize, 1.0f);

        Core::Log::Write(LogCategory::UI, LogLevel::Info, "Dear ImGui {} initialized", IMGUI_VERSION);

        ImGuiLibrary library(std::move(settingsPath), fontSize);
        library.m_isActive = true;

        return library;
    }

    ImGuiLibrary::ImGuiLibrary(std::filesystem::path settingsPath, float fontSize) noexcept
        : m_settingsPath(std::move(settingsPath))
        , m_fontSize(fontSize)
    {}

    ImGuiLibrary::ImGuiLibrary(ImGuiLibrary&& other) noexcept
        : m_settingsPath(std::move(other.m_settingsPath))
        , m_fontSize(other.m_fontSize)
        , m_isActive(std::exchange(other.m_isActive, false))
    {}

    ImGuiLibrary& ImGuiLibrary::operator=(ImGuiLibrary&& other) noexcept
    {
        if (this != &other)
        {
            if (m_isActive)
            {
                SaveSettings();
                ImGui::DestroyContext();
            }

            m_settingsPath = std::move(other.m_settingsPath);
            m_fontSize = other.m_fontSize;
            m_isActive = std::exchange(other.m_isActive, false);
        }

        return *this;
    }

    ImGuiLibrary::~ImGuiLibrary()
    {
        // Changes made during the last few seconds are not saved yet (ImGui waits before reporting them), so the
        // settings are written once more before the context disappears.
        if (m_isActive)
        {
            SaveSettings();
            ImGui::DestroyContext();
        }
    }

    void ImGuiLibrary::SetScale(float scale)
    {
        ApplyStyle(m_fontSize, scale);
    }

    void ImGuiLibrary::SaveSettingsIfChanged()
    {
        // ImGui sets WantSaveIniSettings (only while IniFilename is nullptr) some seconds after a window was moved,
        // resized, opened or collapsed. The flag must be cleared by whoever saves.
        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantSaveIniSettings)
            return;

        SaveSettings();
        io.WantSaveIniSettings = false;
    }

    void ImGuiLibrary::SaveSettings() const
    {
        ImGui::SaveIniSettingsToDisk(Core::ToUTF8String(m_settingsPath).c_str());
    }
}
