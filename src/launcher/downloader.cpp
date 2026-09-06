#include "launcher/downloader.h"
#include "core/http.h"
#include "core/json.h"
#include "core/fs.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace launcher {

static const char* LIBRARIES_URL = "https://libraries.minecraft.net/";

bool Downloader::ensure_dir_for(const std::string& path) {
    std::error_code ec;
    fs::create_directories(fs::path(path).parent_path(), ec);
    return true;
}

bool Downloader::verify_sha1(const std::string& path, const std::string& expected) {
    if (expected.empty()) return true;
    return core::file_exists(path);
}

void Downloader::report(ProgressCallback progress) {
    if (progress) progress(m_current, m_total, "");
}

bool Downloader::download_file(const DownloadTask& task, ProgressCallback progress) {
    if (core::file_exists(task.dest) && verify_sha1(task.dest, task.sha1)) {
        m_current++;
        report(progress);
        return true;
    }
    ensure_dir_for(task.dest);
    bool ok = core::http_download(task.url, task.dest);
    m_current++;
    report(progress);
    return ok;
}

bool Downloader::download_all(const std::string& version_id,
                              const std::string& /* game_dir */,
                              ProgressCallback progress) {
    m_current = 0;
    m_total = 0;

    std::string versions_dir = core::versions_dir().string();
    std::string version_file = (fs::path(versions_dir) / (version_id + ".json")).string();
    std::string version_json;

    if (progress) progress(0, 0, "Fetching version manifest...");

    if (core::file_exists(version_file)) {
        version_json = core::read_file(version_file);
    } else {
        std::string manifest_json = core::http_get(
            "https://launchermeta.mojang.com/mc/game/version_manifest_v2.json");
        if (manifest_json.empty()) {
            if (progress) progress(0, 0, "Failed to fetch version manifest");
            return false;
        }

        auto manifest = core::json_parse(manifest_json);
        if (!manifest) return false;

        auto* versions = core::json_get_array(manifest.get(), "versions");
        if (!versions) return false;

        int n = cJSON_GetArraySize(versions);
        for (int i = 0; i < n; i++) {
            auto* v = cJSON_GetArrayItem(versions, i);
            if (core::json_get_string(v, "id") == version_id) {
                std::string url = core::json_get_string(v, "url");
                if (progress) progress(0, 0, "Fetching version info...");
                version_json = core::http_get(url);
                if (!version_json.empty()) {
                    fs::create_directories(versions_dir);
                    core::write_file(version_file, version_json);
                }
                break;
            }
        }
    }

    if (version_json.empty()) {
        if (progress) progress(0, 0, "Version not found");
        return false;
    }

    // Count libraries
    std::vector<DownloadTask> lib_tasks;
    {
        auto root = core::json_parse(version_json);
        if (!root) return false;
        auto* libs = core::json_get_array(root.get(), "libraries");
        if (libs) {
            int n = cJSON_GetArraySize(libs);
            for (int i = 0; i < n; i++) {
                auto* lib = cJSON_GetArrayItem(libs, i);
                auto* dl = core::json_get_object(lib, "downloads");
                if (!dl) continue;
                auto* artifact = core::json_get_object(dl, "artifact");
                if (!artifact) continue;
                std::string path = core::json_get_string(artifact, "path");
                if (path.empty()) continue;
                std::string url = std::string(LIBRARIES_URL) + path;
                std::string dest = (fs::path(core::libraries_dir()) / path).string();
                std::string sha1 = core::json_get_string(artifact, "sha1");
                lib_tasks.push_back({url, dest, sha1});
            }
        }
    }

    // Count assets
    std::vector<DownloadTask> asset_tasks;
    {
        auto root = core::json_parse(version_json);
        if (!root) return false;
        auto* asset_index = core::json_get_object(root.get(), "assetIndex");
        if (asset_index) {
            std::string index_url = core::json_get_string(asset_index, "url");
            std::string id = core::json_get_string(asset_index, "id");

            if (!index_url.empty() && !id.empty()) {
                std::string index_dir = (fs::path(core::assets_dir()) / "indexes").string();
                fs::create_directories(index_dir);
                std::string index_file = (fs::path(index_dir) / (id + ".json")).string();

                DownloadTask index_task{index_url, index_file, ""};
                download_file(index_task, nullptr);

                std::string index_json = core::read_file(index_file);
                auto index_root = core::json_parse(index_json);
                if (index_root) {
                    auto* objects = core::json_get_object(index_root.get(), "objects");
                    if (objects) {
                        cJSON* child = objects->child;
                        while (child) {
                            auto* hash_item = core::json_get_object(child, "hash");
                            if (hash_item && cJSON_IsString(hash_item)) {
                                std::string hash = hash_item->valuestring;
                                std::string prefix = hash.substr(0, 2);
                                std::string url = "https://resources.download.minecraft.net/" + prefix + "/" + hash;
                                std::string dest = (fs::path(core::assets_dir()) / "objects" / prefix / hash).string();
                                asset_tasks.push_back({url, dest, hash});
                            }
                            child = child->next;
                        }
                    }
                }
            }
        }
    }

    // Count client jar
    int jar_count = 1;

    m_total = (int)lib_tasks.size() + (int)asset_tasks.size() + jar_count;
    m_current = 0;

    // Download libraries
    for (auto& t : lib_tasks) {
        if (progress) progress(m_current, m_total, t.dest);
        if (!download_file(t, nullptr)) {
            if (progress) progress(m_current, m_total, "Failed: " + t.url);
        }
    }

    // Download assets
    for (auto& t : asset_tasks) {
        if (progress) progress(m_current, m_total, t.dest);
        if (!download_file(t, nullptr)) {
            if (progress) progress(m_current, m_total, "Failed: " + t.url);
        }
    }

    // Download client jar
    {
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
        if (progress) progress(m_current, m_total, dest);
        if (!download_file(task, nullptr)) return false;
    }

    if (progress) progress(m_total, m_total, "Done");
    return true;
}

}
