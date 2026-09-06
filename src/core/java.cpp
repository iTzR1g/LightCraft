#include "core/java.h"
#include "core/process.h"
#include <cstring>
#include <array>
#include <filesystem>

namespace fs = std::filesystem;

namespace core {

static std::vector<std::string> search_paths() {
    std::vector<std::string> paths;
    const char* home = std::getenv("HOME");
    if (home) {
        paths.push_back(std::string(home) + "/.local/bin/java");
        paths.push_back(std::string(home) + "/.sdkman/candidates/java/current/bin/java");
    }
    paths.push_back("/usr/bin/java");
    paths.push_back("/usr/local/bin/java");
    paths.push_back("/usr/lib/jvm/default/bin/java");

    const char* java_home = std::getenv("JAVA_HOME");
    if (java_home) paths.push_back(std::string(java_home) + "/bin/java");

    for (auto& dir : {"/usr/lib/jvm", "/usr/local/lib/jvm", "/opt"}) {
        std::error_code ec;
        if (!fs::is_directory(dir, ec)) continue;
        for (auto& entry : fs::directory_iterator(dir, ec)) {
            if (entry.is_directory())
                paths.push_back(entry.path().string() + "/bin/java");
        }
    }
    return paths;
}

int java_version(const std::string& java_path) {
    auto res = run_process(java_path, {"-version"});
    std::string err = res.stderr_out;
    if (err.empty()) err = res.stdout_out;

    const char* p = strstr(err.c_str(), "version \"");
    if (!p) return 0;
    p += 10;

    if (strncmp(p, "1.", 2) == 0) {
        p += 2;
        return (*p >= '0' && *p <= '9') ? (*p - '0') : 0;
    }

    int ver = 0;
    while (*p >= '0' && *p <= '9') {
        ver = ver * 10 + (*p - '0');
        p++;
    }
    return ver;
}

std::vector<JavaInstall> find_java() {
    std::vector<JavaInstall> installs;
    for (auto& path : search_paths()) {
        if (!fs::is_regular_file(path)) continue;
        JavaInstall j;
        j.path = path;
        j.version = java_version(path);
        if (j.version > 0) installs.push_back(std::move(j));
    }
    return installs;
}

std::string default_java_path() {
    auto installs = find_java();
    JavaInstall* best = nullptr;
    for (auto& j : installs) {
        if (!best || j.version > best->version) best = &j;
    }
    return best ? best->path : "java";
}

}
