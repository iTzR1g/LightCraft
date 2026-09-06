#pragma once
#include <string>
#include <vector>
#include <functional>

namespace launcher {

struct DownloadTask {
    std::string url;
    std::string dest;
    std::string sha1;
    size_t size = 0;
};

class Downloader {
public:
    using ProgressCallback = std::function<void(int current, int total, const std::string& current_file)>;

    bool download_libraries(const std::string& version_json,
                            const std::string& game_dir);
    bool download_assets(const std::string& version_json,
                         const std::string& assets_dir);
    bool download_client_jar(const std::string& version_json,
                             const std::string& versions_dir);

    bool download_all(const std::string& version_id,
                      const std::string& game_dir,
                      ProgressCallback progress = nullptr);

    int progress_current() const { return m_current; }
    int progress_total() const { return m_total; }

private:
    bool download_file(const DownloadTask& task);
    bool download_file_with_progress(const DownloadTask& task, ProgressCallback progress);
    bool ensure_dir_for(const std::string& path);
    bool verify_sha1(const std::string& path, const std::string& expected);

    int m_current = 0;
    int m_total = 0;
};

}
