#include "launcher/downloader.h"
#include "core/http.h"
#include "core/json.h"
#include "core/fs.h"
#include "launcher/manifest.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace launcher {

static const char* LIBRARIES_URL = "https://libraries.minecraft.net/";

bool Downloader::ensure_dir_for(const std::string& path) {
    fs::create_directories(fs::path(path).parent_path());
    return true;
}

bool Downloader::verify_sha1(const std::string& path, const std::string& expected) {
    if (expected.empty()) return true;
    // Simplified: trust downloaded files for now
    // Full impl would read file and compute SHA1
    return core::file_exists(path);
}

bool Downloader::download_file(const DownloadTask& task) {
    if (core::file_exists(task.dest) && verify_sha1(task.dest, task.sha1)) {
        return true;
    }
    ensure_dir_for(task.dest);
    return core::http_download(task.url, task.dest);
}

bool Downloader::download_file_with_progress(const DownloadTask& task, ProgressCallback progress) {
    if (core::file_exists(task.dest) && verify_sha1(task.dest, task.sha1)) {
        m_current++;
        if (progress) progress(m_current, m_total, task.dest);
        return true;
    }
    ensure_dir_for(task.dest);
    bool ok = core::http_download(task.url, task.dest);
    m_current++;
    if (progress) progress(m_current, m_total, task.dest);
    return ok;
}

bool Downloader::download_client_jar(const std::string& version_json,
                                     const std::string& versions_dir) {
    auto root = core::json_parse(version_json);
    if (!root) return false;

    auto* downloads = core::json_get_object(root.get(), "downloads");
    if (!downloads) return false;
    auto* client = core::json_get_object(downloads, "client");
    if (!client) return false;

    std::string url = core::json_get_string(client, "url");
    std::string sha1 = core::json_get_string(client, "sha1");
    std::string id = core::json_get_string(root.get(), "id");

    std::string dest = (fs::path(versions_dir) / (id + ".jar")).string();
    DownloadTask task{url, dest, sha1};
    m_total = 1;
    m_current = 0;
    return download_file(task);
}

bool Downloader::download_libraries(const std::string& version_json,
                                    const std::string& lib_dir) {
    auto root = core::json_parse(version_json);
    if (!root) return false;

    auto* libs = core::json_get_array(root.get(), "libraries");
    if (!libs) return false;

    std::vector<DownloadTask> tasks;
    int n = cJSON_GetArraySize(libs);
    for (int i = 0; i < n; i++) {
        auto* lib = cJSON_GetArrayItem(libs, i);
        auto* dl = core::json_get_object(lib, "downloads");
        if (!dl) continue;
        auto* artifact = core::json_get_object(dl, "artifact");
        if (!artifact) continue;

        std::string url = core::json_get_string(artifact, "path");
        if (url.empty()) continue;
        url = std::string(LIBRARIES_URL) + url;

        std::string dest = (fs::path(lib_dir) / core::json_get_string(artifact, "path")).string();
        std::string sha1 = core::json_get_string(artifact, "sha1");

        tasks.push_back({url, dest, sha1});
    }

    m_total = tasks.size();
    m_current = 0;
    for (auto& t : tasks) {
        if (!download_file(t)) return false;
        m_current++;
    }
    return true;
}

bool Downloader::download_assets(const std::string& version_json,
                                 const std::string& assets_base) {
    auto root = core::json_parse(version_json);
    if (!root) return false;

    auto* asset_index = core::json_get_object(root.get(), "assetIndex");
    if (!asset_index) return false;

    std::string index_url = core::json_get_string(asset_index, "url");
    std::string id = core::json_get_string(asset_index, "id");

    if (index_url.empty() || id.empty()) return false;

    std::string index_dir = (fs::path(assets_base) / "indexes").string();
    fs::create_directories(index_dir);
    std::string index_file = (fs::path(index_dir) / (id + ".json")).string();

    DownloadTask index_task{index_url, index_file, ""};
    download_file(index_task);

    std::string index_json = core::read_file(index_file);
    auto index_root = core::json_parse(index_json);
    if (!index_root) return false;

    auto* objects = core::json_get_object(index_root.get(), "objects");
    if (!objects) return false;

    std::vector<DownloadTask> tasks;
    cJSON* child = objects->child;
    while (child) {
        auto* hash_item = core::json_get_object(child, "hash");
        if (hash_item && cJSON_IsString(hash_item)) {
            std::string hash = hash_item->valuestring;
            std::string prefix = hash.substr(0, 2);
            std::string url = "https://resources.download.minecraft.net/" + prefix + "/" + hash;
            std::string dest = (fs::path(assets_base) / "objects" / prefix / hash).string();
            tasks.push_back({url, dest, hash});
        }
        child = child->next;
    }

    m_total = tasks.size();
    m_current = 0;
    for (auto& t : tasks) {
        if (!download_file(t)) return false;
        m_current++;
    }
    return true;
}

bool Downloader::download_all(const std::string& version_id,
                              const std::string& game_dir,
                              ProgressCallback progress) {
    m_current = 0;
    m_total = 3;

    if (progress) progress(0, 3, "Fetching version info...");

    std::string versions_dir = core::versions_dir().string();
    std::string version_file = (fs::path(versions_dir) / (version_id + ".json")).string();
    std::string version_json;

    if (core::file_exists(version_file)) {
        version_json = core::read_file(version_file);
    } else {
        // Fetch version manifest to find this version's URL
        std::string manifest_json = core::http_get(
            "https://launchermeta.mojang.com/mc/game/version_manifest_v2.json");
        if (manifest_json.empty()) return false;

        auto manifest = core::json_parse(manifest_json);
        if (!manifest) return false;

        auto* versions = core::json_get_array(manifest.get(), "versions");
        if (!versions) return false;

        int n = cJSON_GetArraySize(versions);
        for (int i = 0; i < n; i++) {
            auto* v = cJSON_GetArrayItem(versions, i);
            if (core::json_get_string(v, "id") == version_id) {
                std::string url = core::json_get_string(v, "url");
                version_json = core::http_get(url);
                if (!version_json.empty()) {
                    fs::create_directories(versions_dir);
                    core::write_file(version_file, version_json);
                }
                break;
            }
        }
    }

    if (version_json.empty()) return false;

    if (progress) progress(1, 3, "Downloading libraries...");
    if (!download_libraries(version_json, core::libraries_dir().string())) return false;

    if (progress) progress(2, 3, "Downloading assets...");
    if (!download_assets(version_json, core::assets_dir().string())) return false;

    if (progress) progress(3, 3, "Downloading client jar...");
    if (!download_client_jar(version_json, versions_dir)) return false;

    return true;
}

}
