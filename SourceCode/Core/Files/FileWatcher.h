#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Abomination::Core
{
    // Notices files changed on disk while the game runs, for hot reload (the shaders of the Renderer). Files are watched in
    // groups under a key ("Shaders/TexturedShaded" for its .vert and .frag), and a changed group is reported by its key.
    //
    // A change is reported only when the file has stopped changing: its new write time must be the same at two checks in
    // a row. An editor may write a file in several steps, and a file read between them is cut off; waiting one check
    // reads it whole. Checks are made by the caller (every half a second for shaders), so a change is seen after one to
    // two checks.
    //
    // A file that cannot be read (deleted, renamed by the editor while saving) is skipped until it is back.
    class FileWatcher
    {
    public:
        // Starts watching files under key, from their write times now. Watching the same key again replaces its files.
        void Watch(const std::string& key, const std::vector<std::filesystem::path>& files);

        // The keys of the groups with a file changed since the last report and not changing anymore (see above). Each change
        // is reported once.
        [[nodiscard]] std::vector<std::string> CollectChangedKeys();

    private:
        struct WatchedFile
        {
            std::string key;
            std::filesystem::path path;

            // The write time of the last reported version, and the one seen at the last check.
            std::filesystem::file_time_type reportedTime;
            std::filesystem::file_time_type lastSeenTime;
        };

        std::vector<WatchedFile> m_files;
    };
}
