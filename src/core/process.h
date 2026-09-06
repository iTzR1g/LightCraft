#pragma once
#include <string>
#include <vector>

namespace core {

struct ProcessResult {
    int exit_code = -1;
    std::string stdout_out;
    std::string stderr_out;
};

ProcessResult run_process(const std::string& exe,
                          const std::vector<std::string>& args,
                          const std::string& work_dir = "");

bool launch_background(const std::string& exe,
                       const std::vector<std::string>& args,
                       const std::string& work_dir = "");

}
