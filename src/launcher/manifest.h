#pragma once
#include <string>
#include <vector>

namespace launcher {

struct VersionEntry {
    std::string id;
    std::string type; // "release", "snapshot", "old_beta", "old_alpha"
};

struct VersionManifest {
    std::vector<VersionEntry> versions;
    bool loaded = false;

    bool fetch();
    std::string version_url(const std::string& id) const;
    std::vector<VersionEntry> releases() const;
    std::vector<VersionEntry> snapshots() const;
};

}
