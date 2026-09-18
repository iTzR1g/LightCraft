#include "launcher/downloader.h"
#include "core/http.h"
#include "core/json.h"
#include "core/fs.h"
#include "core/process.h"
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

bool Downloader::extract_natives(const std::string& jar_path, const std::string& dest_dir) {
    std::error_code ec;
    fs::create_directories(dest_dir, ec);
    core::ProcessResult res = core::run_process("unzip", {"-o", "-j", jar_path, "-d", dest_dir});
    // -o = overwrite, -j = junk paths (flat extract)
    return res.exit_code == 0;
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
        std::string manifest_url = "https://launchermeta.mojang.com/mc/game/version_manifest_v2.json";
        std::string manifest_json = core::http_get(manifest_url);
        if (manifest_json.empty()) {
            if (progress) progress(0, 0, "Network error: " + core::http_last_error());
            return false;
        }

        auto manifest = core::json_parse(manifest_json);
        if (!manifest) {
            if (progress) progress(0, 0, "Failed to parse manifest JSON");
            return false;
        }

        auto* versions = core::json_get_array(manifest.get(), "versions");
        if (!versions) {
            if (progress) progress(0, 0, "No versions array in manifest");
            return false;
        }

        bool found = false;
        int n = cJSON_GetArraySize(versions);
        for (int i = 0; i < n; i++) {
            auto* v = cJSON_GetArrayItem(versions, i);
            if (core::json_get_string(v, "id") == version_id) {
                std::string url = core::json_get_string(v, "url");
                if (progress) progress(0, 0, "Fetching version info for " + version_id + "...");
                version_json = core::http_get(url);
                if (!version_json.empty()) {
                    fs::create_directories(versions_dir);
                    core::write_file(version_file, version_json);
                    found = true;
                } else {
                    if (progress) progress(0, 0, "Failed to fetch version JSON: " + core::http_last_error());
                }
                break;
            }
        }
        if (!found && version_json.empty()) {
            if (progress) progress(0, 0, "Version " + version_id + " not found in manifest");
            return false;
        }
    }

    if (version_json.empty()) {
        if (progress) progress(0, 0, "Version data empty");
        return false;
    }

    // Build library + native tasks
    std::vector<DownloadTask> lib_tasks;
    std::vector<DownloadTask> native_tasks;
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

                // Regular artifact
                auto* artifact = core::json_get_object(dl, "artifact");
                if (artifact) {
                    std::string path = core::json_get_string(artifact, "path");
                    if (!path.empty()) {
                        std::string url = std::string(LIBRARIES_URL) + path;
                        std::string dest = (fs::path(core::libraries_dir()) / path).string();
                        std::string sha1 = core::json_get_string(artifact, "sha1");
                        lib_tasks.push_back({url, dest, sha1});
                    }
                }

                // Native classifier (natives-linux)
                auto* classifiers = core::json_get_object(dl, "classifiers");
                if (classifiers) {
                    auto* natives_linux = core::json_get_object(classifiers, "natives-linux");
                    if (natives_linux) {
                        std::string path = core::json_get_string(natives_linux, "path");
                        std::string url = core::json_get_string(natives_linux, "url");
                        if (url.empty() && !path.empty()) {
                            url = std::string(LIBRARIES_URL) + path;
                        }
                        if (!url.empty()) {
                            std::string dest;
                            if (!path.empty()) {
                                dest = (fs::path(core::libraries_dir()) / path).string();
                            } else {
                                std::string name = "native_" + std::to_string(i) + ".jar";
                                dest = (fs::path(core::libraries_dir()) / "natives" / name).string();
                            }
                            std::string sha1 = core::json_get_string(natives_linux, "sha1");
                            native_tasks.push_back({url, dest, sha1});
                        }
                    }
                }
            }
        }
    }

    // Build asset tasks
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
                if (progress) progress(0, 0, "Fetching asset index...");
                if (!download_file(index_task, nullptr)) {
                    if (progress) progress(0, 0, "Failed to download asset index: " + core::http_last_error());
                    return false;
                }

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

    int native_jar_count = (int)native_tasks.size();
    m_total = (int)lib_tasks.size() + (int)asset_tasks.size() + native_jar_count + 1;
    m_current = 0;

    // Download libraries
    if (!lib_tasks.empty()) {
        for (auto& t : lib_tasks) {
            if (progress) progress(m_current, m_total, "libs (" + std::to_string(m_current) + "/" + std::to_string(lib_tasks.size()) + ")");
            download_file(t, nullptr);
        }
    }

    // Download native jars
    std::string natives_dir = (fs::path(core::libraries_dir()) / "natives").string();
    for (auto& t : native_tasks) {
        if (progress) progress(m_current, m_total, "natives (" + std::to_string(m_current) + "/" + std::to_string(native_tasks.size()) + ")");
        if (download_file(t, nullptr)) {
            extract_natives(t.dest, natives_dir);
        }
    }

    // Download assets
    if (!asset_tasks.empty()) {
        for (auto& t : asset_tasks) {
            if (progress) progress(m_current, m_total, "assets (" + std::to_string(m_current) + "/" + std::to_string(asset_tasks.size()) + ")");
            download_file(t, nullptr);
            if (m_current % 100 == 0 && progress) {
                progress(m_current, m_total, "assets (" + std::to_string(m_current) + "/" + std::to_string(asset_tasks.size()) + ")");
            }
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
        if (progress) progress(m_current, m_total, "Downloading client jar...");
        if (!download_file(task, nullptr)) {
            if (progress) progress(m_current, m_total, "Failed: " + core::http_last_error());
            return false;
        }
    }

    if (progress) progress(m_total, m_total, "Done");
    return true;
}

}
