#include "ui/main_window.h"
#include "core/fs.h"
#include <FL/Fl.H>
#include <cstdlib>

int main(int argc, char** argv) {
    core::ensure_dirs();

    int w = 420, h = 480;
    if (argc >= 3) {
        w = std::atoi(argv[1]);
        h = std::atoi(argv[2]);
    }

    Fl::scheme("gtk+");
    MainWindow win(w, h, "MineLaunch - Minimal Minecraft Launcher");
    win.show(argc, argv);
    return Fl::run();
}
