#include "launcher/launch.h"
#include "core/fs.h"
#include "core/json.h"
#include "core/java.h"
#include "core/process.h"
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

namespace launcher {

static std::string build_classpath(const std::string& version_json,
                                   const std::string& client_jar) {
    auto root = core::json_parse(version_json);
    if (!root) return client_jar;

    std::string cp = client_jar;
    auto* libs = core::json_get_array(root.get(), "libraries");
    if (!libs) return cp;

    int n = cJSON_GetArraySize(libs);
    for (int i = 0; i < n; i++) {
        auto* lib = cJSON_GetArrayItem(libs, i);
        auto* dl = core::json_get_object(lib, "downloads");
        if (!dl) continue;
        auto* artifact = core::json_get_object(dl, "artifact");
        if (!artifact) continue;

        std::string path = core::json_get_string(artifact, "path");
        if (!path.empty()) {
            cp += ":" + (fs::path(core::libraries_dir()) / path).string();
        }
    }
    return cp;
}

static std::string get_main_class(const std::string& version_json) {
    auto root = core::json_parse(version_json);
    if (!root) return "net.minecraft.client.main.Main";
    return core::json_get_string(root.get(), "mainClass", "net.minecraft.client.main.Main");
}

bool launch_minecraft(const Instance& inst) {
    std::string versions_dir = core::versions_dir().string();
    std::string version_file = (fs::path(versions_dir) / (inst.mc_version + ".json")).string();
    std::string version_json = core::read_file(version_file);
    if (version_json.empty()) return false;

    std::string client_jar = (fs::path(versions_dir) / (inst.mc_version + ".jar")).string();
    std::string game_dir = inst.instance_dir() + "/.minecraft";

    fs::create_directories(game_dir);

    // Assets path
    std::string assets_index = "";
    auto root = core::json_parse(version_json);
    if (root) {
        auto* ai = core::json_get_object(root.get(), "assetIndex");
        if (ai) assets_index = core::json_get_string(ai, "id");
    }

    std::string classpath = build_classpath(version_json, client_jar);
    std::string main_class = get_main_class(version_json);

    std::string java_path = inst.java_path.empty() ? core::default_java_path() : inst.java_path;

    std::vector<std::string> args;
    args.push_back("-Xms" + std::to_string(inst.ram_min) + "M");
    args.push_back("-Xmx" + std::to_string(inst.ram_max) + "M");

    if (!inst.jvm_args.empty()) {
        std::istringstream iss(inst.jvm_args);
        std::string arg;
        while (iss >> arg) args.push_back(arg);
    }

    args.push_back("-Djava.library.path=" + core::libraries_dir().string() + "/natives");
    args.push_back("-cp");
    args.push_back(classpath);
    args.push_back(main_class);

    // Game arguments
    args.push_back("--username");
    args.push_back(inst.username.empty() ? "Player" : inst.username);
    args.push_back("--version");
    args.push_back(inst.mc_version);
    args.push_back("--gameDir");
    args.push_back(game_dir);
    args.push_back("--assetsDir");
    args.push_back(core::assets_dir().string());
    if (!assets_index.empty()) {
        args.push_back("--assetIndex");
        args.push_back(assets_index);
    }
    args.push_back("--uuid");
    args.push_back(inst.uuid.empty() ? "00000000-0000-0000-0000-000000000000" : inst.uuid);

    if (!inst.game_args.empty()) {
        std::istringstream iss(inst.game_args);
        std::string arg;
        while (iss >> arg) args.push_back(arg);
    }

    return core::launch_background(java_path, args, game_dir);
}

}
