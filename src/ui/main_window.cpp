#include "ui/main_window.h"
#include "core/fs.h"
#include "core/java.h"
#include "launcher/launch.h"
#include <FL/Fl_Box.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/fl_ask.H>
#include <thread>
#include <cstdint>

MainWindow::MainWindow(int w, int h, const char* title)
    : Fl_Double_Window(w, h, title)
{
    begin();
    int bw = 80, bh = 28, pad = 6;
    int y = pad;

    // Top buttons
    m_btn_new = new Fl_Button(pad, y, bw, bh, "New");
    m_btn_new->callback(on_new, this);
    m_btn_delete = new Fl_Button(pad+bw+pad, y, bw, bh, "Delete");
    m_btn_delete->callback(on_delete, this);
    m_btn_play = new Fl_Button(w-pad-bw, y, bw, bh, "Play");
    m_btn_play->callback(on_play, this);
    m_btn_play->color(0x44aa4400);
    m_btn_play->labelcolor(FL_WHITE);
    y += bh + pad;

    // Instance list
    int list_h = h - y - bh - pad*3;
    m_instance_list = new Fl_Select_Browser(pad, y, w-pad*2, list_h);
    m_instance_list->callback(on_list_select, this);
    m_instance_list->textsize(13);
    y += list_h + pad;

    // Settings panel (overlaid, hidden initially)
    int sy = h/4;
    int sh = h/2;
    int sw = w - pad*2;
    int sx = pad;

    m_input_name = new Fl_Input(sx+80, sy+pad*2, sw-80-pad, bh, "Name:");
    m_choice_version = new Fl_Choice(sx+80, sy+pad*2+bh+pad, sw-80-pad, bh, "Version:");
    m_choice_loader = new Fl_Choice(sx+80, sy+pad*2+(bh+pad)*2, sw-80-pad, bh, "Loader:");
    m_input_java = new Fl_Input(sx+80, sy+pad*2+(bh+pad)*3, sw-80-pad, bh, "Java:");
    m_slider_ram = new Fl_Slider(sx+80, sy+pad*2+(bh+pad)*4, sw-80-pad, bh, "RAM MB:");
    m_slider_ram->type(FL_HORIZONTAL);
    m_slider_ram->minimum(128);
    m_slider_ram->maximum(768);
    m_slider_ram->step(64);
    m_input_username = new Fl_Input(sx+80, sy+pad*2+(bh+pad)*5, sw-80-pad, bh, "Username:");

    m_btn_save = new Fl_Button(sx+sw-bw*2-pad, sy+sh-bh-pad*2, bw, bh, "Save");
    m_btn_save->callback(on_save, this);
    m_btn_cancel = new Fl_Button(sx+sw-bw-pad, sy+sh-bh-pad*2, bw, bh, "Cancel");
    m_btn_cancel->callback(on_cancel, this);

    // Status bar
    m_status = new Fl_Text_Display(pad, h-bh-pad, w-pad*2, bh);
    m_status->buffer(new Fl_Text_Buffer());

    m_progress = new Fl_Progress(pad, h-bh*2-pad*2, w-pad*2, bh);
    m_progress->minimum(0);
    m_progress->maximum(1);
    m_progress->value(0);

    hide_settings();
    end();

    resizable(m_instance_list);

    // Load data
    m_mgr.refresh();
    refresh_list();
}

void MainWindow::refresh_list() {
    m_instance_list->clear();
    for (auto& inst : m_mgr.instances()) {
        std::string label = inst.name + "  [" + inst.mc_version + "]";
        if (inst.loader != "vanilla") label += " (" + inst.loader + ")";
        m_instance_list->add(label.c_str());
    }
}

void MainWindow::set_status(const char* msg) {
    m_status->buffer()->text(msg);
}

void MainWindow::show_settings(int idx) {
    m_settings_open = true;
    m_btn_new->hide();
    m_btn_delete->hide();
    m_btn_play->hide();
    m_instance_list->hide();

    m_input_name->show();
    m_choice_version->show();
    m_choice_loader->show();
    m_input_java->show();
    m_slider_ram->show();
    m_input_username->show();
    m_btn_save->show();
    m_btn_cancel->show();

    // Populate version choice
    m_choice_version->clear();
    if (!m_manifest.loaded) {
        set_status("Fetching version list...");
        Fl::flush();
        m_manifest.fetch();
    }
    auto releases = m_manifest.releases();
    for (auto& v : releases) {
        m_choice_version->add(v.id.c_str());
    }

    m_choice_loader->clear();
    m_choice_loader->add("vanilla");
    m_choice_loader->add("forge");
    m_choice_loader->add("fabric");
    m_choice_loader->add("quilt");
    m_choice_loader->add("neoforge");

    if (idx >= 0 && idx < (int)m_mgr.instances().size()) {
        auto& inst = m_mgr.instances()[idx];
        m_input_name->value(inst.name.c_str());
        m_input_java->value(inst.java_path.c_str());
        m_slider_ram->value(inst.ram_max);
        m_input_username->value(inst.username.c_str());

        // Find version in choice
        for (int i = 0; i < m_choice_version->size(); i++) {
            if (m_choice_version->text(i+1) == inst.mc_version) {
                m_choice_version->value(i);
                break;
            }
        }
        for (int i = 0; i < m_choice_loader->size(); i++) {
            if (m_choice_loader->text(i+1) == inst.loader) {
                m_choice_loader->value(i);
                break;
            }
        }
    } else {
        m_input_name->value("");
        m_input_java->value(core::default_java_path().c_str());
        m_slider_ram->value(512);
        m_input_username->value("Player");
        if (m_choice_version->size() > 0) m_choice_version->value(0);
        m_choice_loader->value(0);
    }

    m_selected = idx;
}

void MainWindow::hide_settings() {
    m_settings_open = false;
    m_input_name->hide();
    m_choice_version->hide();
    m_choice_loader->hide();
    m_input_java->hide();
    m_slider_ram->hide();
    m_input_username->hide();
    m_btn_save->hide();
    m_btn_cancel->hide();

    m_btn_new->show();
    m_btn_delete->show();
    m_btn_play->show();
    m_instance_list->show();
}

void MainWindow::on_new(Fl_Widget*, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    self->show_settings(-1);
}

void MainWindow::on_delete(Fl_Widget*, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    int sel = self->m_instance_list->value() - 1;
    if (sel < 0 || sel >= (int)self->m_mgr.instances().size()) return;

    auto& inst = self->m_mgr.instances()[sel];
    if (fl_choice("Delete instance '%s'?", "No", "Yes", nullptr, inst.name.c_str()) == 1) {
        self->m_mgr.delete_instance(inst.name);
        self->refresh_list();
    }
}

void MainWindow::on_play(Fl_Widget*, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    int sel = self->m_instance_list->value() - 1;
    if (sel < 0 || sel >= (int)self->m_mgr.instances().size()) return;

    self->do_download_and_launch(sel);
}

void MainWindow::on_settings(Fl_Widget*, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    int sel = self->m_instance_list->value() - 1;
    self->show_settings(sel);
}

void MainWindow::on_save(Fl_Widget*, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    std::string name = self->m_input_name->value();
    if (name.empty()) { fl_alert("Name required!"); return; }

    launcher::Instance inst;
    inst.name = name;
    inst.mc_version = self->m_choice_version->text(self->m_choice_version->value()+1);
    inst.loader = self->m_choice_loader->text(self->m_choice_loader->value()+1);
    inst.java_path = self->m_input_java->value();
    inst.ram_max = (int)self->m_slider_ram->value();
    inst.ram_min = std::min(256, inst.ram_max / 2);
    inst.username = self->m_input_username->value();
    inst.offline = true;

    if (self->m_selected >= 0) {
        self->m_mgr.save_instance(inst);
    } else {
        self->m_mgr.create_instance(name, inst.mc_version);
        self->m_mgr.save_instance(inst);
    }
    self->refresh_list();
    self->hide_settings();
}

void MainWindow::on_cancel(Fl_Widget*, void* data) {
    static_cast<MainWindow*>(data)->hide_settings();
}

void MainWindow::on_list_select(Fl_Widget*, void* /* data */) {
    // Single click selects, double click opens settings
}

void MainWindow::async_handler(void* msg_ptr) {
    auto msg = static_cast<AsyncMsg>(reinterpret_cast<intptr_t>(msg_ptr));
    // We use Fl::find() trick: the active MainWindow is the first window
    auto* win = dynamic_cast<MainWindow*>(Fl::first_window());
    if (win) win->handle_async(msg);
}

void MainWindow::handle_async(AsyncMsg msg) {
    switch (msg) {
        case MSG_DONE:
            set_status("Launching...");
            m_progress->value(1.0);
            break;
        case MSG_FAIL:
            set_status("Download failed!");
            m_progress->value(0);
            break;
        case MSG_PROGRESS:
            m_progress->value(m_progress_val);
            break;
    }
}

void MainWindow::do_download_and_launch(int idx) {
    auto& inst = m_mgr.instances()[idx];
    set_status("Downloading files...");
    m_progress->value(0);
    Fl::flush();

    std::string version = inst.mc_version;
    std::string game_dir = inst.instance_dir() + "/.minecraft";

    std::thread([this, version, game_dir, inst]() {
        bool ok = m_downloader.download_all(version, game_dir,
            [this](int cur, int total, const std::string&) {
                if (total > 0) {
                    float pct = (float)cur / (float)total;
                    m_progress_val = pct;
                    Fl::awake(async_handler, reinterpret_cast<void*>(static_cast<intptr_t>(MSG_PROGRESS)));
                }
            });

        if (ok) {
            Fl::awake(async_handler, reinterpret_cast<void*>(static_cast<intptr_t>(MSG_DONE)));
            launcher::launch_minecraft(inst);
        } else {
            Fl::awake(async_handler, reinterpret_cast<void*>(static_cast<intptr_t>(MSG_FAIL)));
        }
    }).detach();
}
