#include "launcher/manifest.h"
#include "core/http.h"
#include "core/json.h"
#include "core/fs.h"
#include <algorithm>

namespace launcher {

static const char* MANIFEST_URL = "https://launchermeta.mojang.com/mc/game/version_manifest_v2.json";

bool VersionManifest::fetch() {
    std::string cached = core::read_file(core::data_dir() / "version_manifest.json");
    std::string json;

    if (!cached.empty()) {
        json = cached;
    } else {
        json = core::http_get(MANIFEST_URL);
        if (!json.empty()) {
            core::write_file(core::data_dir() / "version_manifest.json", json);
        }
    }

    if (json.empty()) return false;

    auto root = core::json_parse(json);
    if (!root) return false;

    versions.clear();
    auto* arr = core::json_get_array(root.get(), "versions");
    if (!arr) return false;

    int n = cJSON_GetArraySize(arr);
    for (int i = 0; i < n; i++) {
        auto* item = cJSON_GetArrayItem(arr, i);
        VersionEntry e;
        e.id = core::json_get_string(item, "id");
        e.type = core::json_get_string(item, "type");
        if (!e.id.empty()) versions.push_back(std::move(e));
    }
    loaded = !versions.empty();
    return loaded;
}

std::string VersionManifest::version_url(const std::string& id) const {
    for (auto& v : versions) {
        if (v.id == id) {
            std::string json = core::read_file(core::data_dir() / "version_manifest.json");
            if (json.empty()) return "";
            auto root = core::json_parse(json);
            if (!root) return "";
            auto* arr = core::json_get_array(root.get(), "versions");
            if (!arr) return "";
            int n = cJSON_GetArraySize(arr);
            for (int i = 0; i < n; i++) {
                auto* item = cJSON_GetArrayItem(arr, i);
                if (core::json_get_string(item, "id") == id) {
                    return core::json_get_string(item, "url");
                }
            }
        }
    }
    return "";
}

std::vector<VersionEntry> VersionManifest::releases() const {
    std::vector<VersionEntry> r;
    for (auto& v : versions) {
        if (v.type == "release") r.push_back(v);
    }
    return r;
}

std::vector<VersionEntry> VersionManifest::snapshots() const {
    std::vector<VersionEntry> r;
    for (auto& v : versions) {
        if (v.type == "snapshot") r.push_back(v);
    }
    return r;
}

}
