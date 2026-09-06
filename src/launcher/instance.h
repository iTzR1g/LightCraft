#pragma once
#include <string>
#include <vector>

namespace launcher {

struct Instance {
    std::string name;
    std::string mc_version;
    std::string loader;       // "vanilla", "forge", "fabric", "quilt", "neoforge"
    std::string loader_version;
    std::string java_path;
    int         ram_min = 256;   // MB - conservative for 1GB system
    int         ram_max = 512;   // MB
    std::string jvm_args;
    std::string game_args;
    bool        offline = true;
    std::string username;
    std::string uuid;

    bool save(const std::string& dir) const;
    static Instance load(const std::string& dir);
    static Instance create(const std::string& name, const std::string& mc_version);
    std::string instance_dir() const;
};

}
