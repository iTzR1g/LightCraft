#pragma once
#include <string>
#include <vector>

namespace core {

struct JavaInstall {
    std::string path;
    int version = 0;
    std::string arch;
    bool is_64bit = false;
};

std::vector<JavaInstall> find_java();
std::string default_java_path();
int java_version(const std::string& java_path);

}
