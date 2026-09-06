#include "core/fs.h"
#include <fstream>
#include <cstdlib>

namespace core {

static fs::path base_dir() {
    const char* home = std::getenv("HOME");
    if (!home) home = "/tmp";
    return fs::path(home) / ".minelaunch";
}

fs::path data_dir()       { return base_dir(); }
fs::path instances_dir()  { return base_dir() / "instances"; }
fs::path assets_dir()     { return base_dir() / "assets"; }
fs::path libraries_dir()  { return base_dir() / "libraries"; }
fs::path versions_dir()   { return base_dir() / "versions"; }

bool ensure_dirs() {
    std::error_code ec;
    fs::create_directories(instances_dir(), ec);
    fs::create_directories(assets_dir(), ec);
    fs::create_directories(libraries_dir(), ec);
    fs::create_directories(versions_dir(), ec);
    return !ec;
}

bool dir_exists(const fs::path& p)  { return fs::is_directory(p); }
bool file_exists(const fs::path& p) { return fs::exists(p); }

bool remove_file(const fs::path& p) {
    std::error_code ec;
    return fs::remove(p, ec);
}

bool remove_dir_all(const fs::path& p) {
    std::error_code ec;
    fs::remove_all(p, ec);
    return !ec;
}

std::vector<std::string> list_dirs(const fs::path& dir) {
    std::vector<std::string> result;
    std::error_code ec;
    for (auto& entry : fs::directory_iterator(dir, ec)) {
        if (entry.is_directory())
            result.push_back(entry.path().filename().string());
    }
    return result;
}

std::string read_file(const fs::path& p) {
    std::ifstream f(p);
    if (!f) return {};
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
}

bool write_file(const fs::path& p, const std::string& content) {
    std::ofstream f(p);
    if (!f) return false;
    f.write(content.data(), content.size());
    return f.good();
}

}
