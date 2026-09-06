#include "core/process.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>

namespace core {

ProcessResult run_process(const std::string& exe,
                          const std::vector<std::string>& args,
                          const std::string& work_dir) {
    int stdout_pipe[2];
    int stderr_pipe[2];
    pipe(stdout_pipe);
    pipe(stderr_pipe);

    pid_t pid = fork();
    if (pid == 0) {
        if (!work_dir.empty()) chdir(work_dir.c_str());

        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        std::vector<const char*> argv;
        argv.push_back(exe.c_str());
        for (auto& a : args) argv.push_back(a.c_str());
        argv.push_back(nullptr);

        execvp(exe.c_str(), const_cast<char* const*>(argv.data()));
        _exit(127);
    }

    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    std::string out, err;
    char buf[4096];
    ssize_t n;
    while ((n = read(stdout_pipe[0], buf, sizeof(buf))) > 0) out.append(buf, n);
    while ((n = read(stderr_pipe[0], buf, sizeof(buf))) > 0) err.append(buf, n);
    close(stdout_pipe[0]);
    close(stderr_pipe[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    return {WIFEXITED(status) ? WEXITSTATUS(status) : -1, std::move(out), std::move(err)};
}

bool launch_background(const std::string& exe,
                       const std::vector<std::string>& args,
                       const std::string& work_dir) {
    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        if (!work_dir.empty()) chdir(work_dir.c_str());

        std::vector<const char*> argv;
        argv.push_back(exe.c_str());
        for (auto& a : args) argv.push_back(a.c_str());
        argv.push_back(nullptr);

        execvp(exe.c_str(), const_cast<char* const*>(argv.data()));
        _exit(127);
    }
    return pid > 0;
}

}
