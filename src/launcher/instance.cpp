#include "launcher/instance.h"
#include "core/fs.h"
#include "core/json.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace launcher {

std::string Instance::instance_dir() const {
    return (core::instances_dir() / name).string();
}

bool Instance::save(const std::string& dir) const {
    cJSON* obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "name", name.c_str());
    cJSON_AddStringToObject(obj, "mc_version", mc_version.c_str());
    cJSON_AddStringToObject(obj, "loader", loader.c_str());
    cJSON_AddStringToObject(obj, "loader_version", loader_version.c_str());
    cJSON_AddStringToObject(obj, "java_path", java_path.c_str());
    cJSON_AddNumberToObject(obj, "ram_min", ram_min);
    cJSON_AddNumberToObject(obj, "ram_max", ram_max);
    cJSON_AddStringToObject(obj, "jvm_args", jvm_args.c_str());
    cJSON_AddStringToObject(obj, "game_args", game_args.c_str());
    cJSON_AddBoolToObject(obj, "offline", offline);
    cJSON_AddStringToObject(obj, "username", username.c_str());
    cJSON_AddStringToObject(obj, "uuid", uuid.c_str());

    std::string json = core::json_stringify(obj, true);
    cJSON_Delete(obj);

    std::string path = dir.empty() ? instance_dir() : dir;
    fs::create_directories(path);
    return core::write_file(fs::path(path) / "instance.json", json);
}

Instance Instance::load(const std::string& dir) {
    Instance inst;
    std::string json = core::read_file(fs::path(dir) / "instance.json");
    if (json.empty()) return inst;

    auto root = core::json_parse(json);
    if (!root) return inst;

    inst.name           = core::json_get_string(root.get(), "name");
    inst.mc_version     = core::json_get_string(root.get(), "mc_version");
    inst.loader         = core::json_get_string(root.get(), "loader", "vanilla");
    inst.loader_version = core::json_get_string(root.get(), "loader_version");
    inst.java_path      = core::json_get_string(root.get(), "java_path");
    inst.ram_min        = core::json_get_int(root.get(), "ram_min", 256);
    inst.ram_max        = core::json_get_int(root.get(), "ram_max", 512);
    inst.jvm_args       = core::json_get_string(root.get(), "jvm_args");
    inst.game_args      = core::json_get_string(root.get(), "game_args");
    inst.offline        = core::json_get_bool(root.get(), "offline", true);
    inst.username       = core::json_get_string(root.get(), "username", "Player");
    inst.uuid           = core::json_get_string(root.get(), "uuid");
    return inst;
}

Instance Instance::create(const std::string& name, const std::string& mc_version) {
    Instance inst;
    inst.name       = name;
    inst.mc_version = mc_version;
    inst.loader     = "vanilla";
    inst.java_path  = "java";
    inst.ram_min    = 256;
    inst.ram_max    = 512;
    inst.offline    = true;
    inst.username   = "Player";
    return inst;
}

}
