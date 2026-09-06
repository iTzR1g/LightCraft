#pragma once
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace core {

fs::path data_dir();
fs::path instances_dir();
fs::path assets_dir();
fs::path libraries_dir();
fs::path versions_dir();

bool ensure_dirs();
bool dir_exists(const fs::path& p);
bool file_exists(const fs::path& p);
bool remove_file(const fs::path& p);
bool remove_dir_all(const fs::path& p);

std::vector<std::string> list_dirs(const fs::path& dir);
std::string read_file(const fs::path& p);
bool write_file(const fs::path& p, const std::string& content);

}
