#include "Core/Files/FileWatcher.h"

#include <algorithm>
#include <optional>
#include <system_error>

namespace Abomination::Core
{
    namespace
    {
        // The write time of a file, or nothing when it cannot be read. The error_code overload does not throw: a file in
        // the middle of being saved may be missing for a moment.
        std::optional<std::filesystem::file_time_type> GetWriteTime(const std::filesystem::path& path)
        {
            std::error_code error;
            const std::filesystem::file_time_type time = std::filesystem::last_write_time(path, error);
            if (error.value() != 0)
                return std::nullopt;

            return time;
        }
    }

    void FileWatcher::Watch(const std::string& key, const std::vector<std::filesystem::path>& files)
    {
        std::erase_if(m_files, [&key](const WatchedFile& file) { return file.key == key; });

        for (const std::filesystem::path& path : files)
        {
            // A file missing now counts as written at the earliest time: when it appears, that is a change.
            const std::filesystem::file_time_type time = GetWriteTime(path).value_or(std::filesystem::file_time_type::min());
            m_files.push_back(WatchedFile{.key = key, .path = path, .reportedTime = time, .lastSeenTime = time});
        }
    }

    std::vector<std::string> FileWatcher::CollectChangedKeys()
    {
        std::vector<std::string> changedKeys;
        for (WatchedFile& file : m_files)
        {
            const std::optional<std::filesystem::file_time_type> time = GetWriteTime(file.path);
            if (!time.has_value())
                continue;

            // Still being written (or changed again): remember the time and look again at the next check.
            if (*time != file.lastSeenTime)
            {
                file.lastSeenTime = *time;
                continue;
            }

            // The same time as at the last check, but not the one reported: a finished change.
            if (*time != file.reportedTime)
            {
                file.reportedTime = *time;
                if (std::ranges::find(changedKeys, file.key) == changedKeys.end())
                    changedKeys.push_back(file.key);
            }
        }

        return changedKeys;
    }
}
