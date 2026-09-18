#include "ui/main_window.h"
#include "core/fs.h"
#include "core/http.h"
#include "core/java.h"
#include "launcher/launch.h"
#include <FL/Fl_Text_Buffer.H>
#include <FL/fl_ask.H>
#include <thread>
#include <cstring>
#include <algorithm>

// ── Colors (Prism-inspired dark theme) ──
static constexpr Fl_Color COL_BG        = fl_rgb_color(30, 30, 30);
static constexpr Fl_Color COL_SIDEBAR   = fl_rgb_color(37, 37, 37);
static constexpr Fl_Color COL_CARD      = fl_rgb_color(45, 45, 45);
static constexpr Fl_Color COL_CARD_HVR  = fl_rgb_color(55, 55, 55);
static constexpr Fl_Color COL_CARD_SEL  = fl_rgb_color(60, 90, 140);
static constexpr Fl_Color COL_ACCENT    = fl_rgb_color(80, 160, 80);
static constexpr Fl_COLOR COL_BTN_PLAY  = fl_rgb_color(60, 150, 60);
static constexpr Fl_Color COL_TEXT      = fl_rgb_color(220, 220, 220);
static constexpr Fl_Color COL_TEXT_DIM  = fl_rgb_color(150, 150, 150);
static constexpr Fl_Color COL_INPUT_BG  = fl_rgb_color(50, 50, 50);
static constexpr Fl_Color COL_PROGRESS  = fl_rgb_color(80, 160, 80);

// ── Timer ──
static void timer_cb(void* data) {
    auto* win = static_cast<MainWindow*>(data);
    win->apply_progress();
    if (win->m_downloading) Fl::repeat_timeout(0.15, timer_cb, data);
}

// ── InstanceCard ──
InstanceCard::InstanceCard(int x, int y, int w, int h, const launcher::Instance& inst)
    : Fl_Box(x, y, w, h, ""), m_inst(inst) {
    box(FL_FLAT_BOX);
    color(COL_CARD);
    align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
    labelcolor(COL_TEXT);
    labelsize(13);
    std::string label = inst.name + "\n[" + inst.mc_version + "]";
    if (inst.loader != "vanilla") label += " (" + inst.loader + ")";
    copy_label(label.c_str());
}

void InstanceCard::draw() {
    Fl_Color bg = m_hover ? COL_CARD_HVR : (m_inst.name == parent()->user_data() ? COL_CARD_SEL : COL_CARD);
    fl_push_clip(x(), y(), w(), h());
    fl_rectf(x(), y(), w(), h(), bg);

    // Left accent bar
    fl_rectf(x(), y(), 4, h(), COL_ACCENT);

    // Text
    int tx = x() + 14, ty = y() + 10;
    fl_color(COL_TEXT);
    fl_font(FL_HELVETICA_BOLD, 14);
    fl_draw(m_inst.name.c_str(), tx, ty + 14);

    fl_color(COL_TEXT_DIM);
    fl_font(FL_HELVETICA, 11);
    std::string info = m_inst.mc_version;
    if (m_inst.loader != "vanilla") info += "  |  " + m_inst.loader;
    info += "  |  " + std::to_string(m_inst.ram_max) + " MB";
    fl_draw(info.c_str(), tx, ty + 32);

    // Play button area on right
    int bx = x() + w() - 70, by = y() + h()/2 - 16;
    fl_rectf(bx, by, 56, 32, COL_BTN_PLAY);
    fl_color(FL_WHITE);
    fl_font(FL_HELVETICA_BOLD, 12);
    fl_draw("Play", bx, by + 22, 56, FL_ALIGN_CENTER);

    fl_pop_clip();
}

int InstanceCard::handle(int event) {
    switch (event) {
    case FL_ENTER:
        m_hover = true;
        redraw();
        return 1;
    case FL_LEAVE:
        m_hover = false;
        redraw();
        return 1;
    case FL_PUSH: {
        int mx = Fl::event_x() - x();
        // Click on play button area (right 70px)
        if (mx > w() - 70) {
            do_callback();
            return 1;
        }
        // Select card
        parent()->user_data((void*)m_inst.name.c_str());
        redraw();
        return 1;
    }
    case FL_DOUBLE:
        do_callback();
        return 1;
    }
    return Fl_Box::handle(event);
}

// ── MainWindow ──
MainWindow::MainWindow(int w, int h, const char* title)
    : Fl_Double_Window(w, h, title)
{
    color(COL_BG);
    begin();

    int sidebar_w = 50;
    int bottom_h = 50;

    // ── Sidebar ──
    {
        Fl_Box* sb = new Fl_Box(0, 0, sidebar_w, h);
        sb->color(COL_SIDEBAR);
        sb->box(FL_FLAT_BOX);
    }

    m_btn_instances = new Fl_Button(0, 10, sidebar_w, 40, "[]");
    m_btn_instances->box(FL_FLAT_BOX);
    m_btn_instances->color(COL_SIDEBAR);
    m_btn_instances->labelcolor(COL_TEXT);
    m_btn_instances->labelsize(16);
    m_btn_instances->callback(nav_instances, this);
    m_btn_instances->tooltip("Instances");

    m_btn_create = new Fl_Button(0, 60, sidebar_w, 40, "+");
    m_btn_create->box(FL_FLAT_BOX);
    m_btn_create->color(COL_SIDEBAR);
    m_btn_create->labelcolor(COL_TEXT);
    m_btn_create->labelsize(20);
    m_btn_create->callback(nav_create, this);
    m_btn_create->tooltip("Create instance");

    m_btn_settings_btn = new Fl_Button(0, h - 50, sidebar_w, 40, "*");
    m_btn_settings_btn->box(FL_FLAT_BOX);
    m_btn_settings_btn->color(COL_SIDEBAR);
    m_btn_settings_btn->labelcolor(COL_TEXT);
    m_btn_settings_btn->labelsize(18);
    m_btn_settings_btn->callback(nav_settings, this);
    m_btn_settings_btn->tooltip("Settings");

    // ── Pages ──
    int px = sidebar_w, pw = w - sidebar_w, ph = h - bottom_h;

    // Instances page
    m_page_instances = new Fl_Group(px, 0, pw, ph);
    {
        // Header
        Fl_Box* hdr = new Fl_Box(px, 0, pw, 40, "Instances");
        hdr->box(FL_FLAT_BOX);
        hdr->color(COL_SIDEBAR);
        hdr->labelcolor(COL_TEXT);
        hdr->labelsize(16);
        hdr->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        hdr->xmargin_left(10);

        // Scrollable card list
        m_instance_scroll = new Fl_Scroll(px, 40, pw, ph - 40);
        m_instance_scroll->type(Fl_Scroll::VERTICAL);
        m_instance_scroll->box(FL_FLAT_BOX);
        m_instance_scroll->color(COL_BG);
        m_instance_scroll->end();
    }
    m_page_instances->end();

    // Create page
    m_page_create = new Fl_Group(px, 0, pw, ph);
    m_page_create->hide();
    {
        Fl_Box* hdr = new Fl_Box(px, 0, pw, 40, "Create Instance");
        hdr->box(FL_FLAT_BOX);
        hdr->color(COL_SIDEBAR);
        hdr->labelcolor(COL_TEXT);
        hdr->labelsize(16);
        hdr->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

        int cx = px + 20, cy = 60, cw = pw - 40, ch = 28, gap = 40;

        m_input_name = new Fl_Input(cx + 70, cy, cw - 70, ch, "Name:");
        m_input_name->color(COL_INPUT_BG);
        m_input_name->textcolor(COL_TEXT);
        m_input_name->labelcolor(COL_TEXT);
        m_input_name->value("My Instance");
        cy += gap;

        m_choice_version = new Fl_Choice(cx + 70, cy, cw - 70, ch, "Version:");
        m_choice_version->color(COL_INPUT_BG);
        m_choice_version->textcolor(COL_TEXT);
        m_choice_version->labelcolor(COL_TEXT);
        cy += gap;

        m_choice_loader = new Fl_Choice(cx + 70, cy, cw - 70, ch, "Loader:");
        m_choice_loader->color(COL_INPUT_BG);
        m_choice_loader->textcolor(COL_TEXT);
        m_choice_loader->labelcolor(COL_TEXT);
        m_choice_loader->add("Vanilla");
        m_choice_loader->add("Forge");
        m_choice_loader->add("Fabric");
        m_choice_loader->add("Quilt");
        m_choice_loader->add("NeoForge");
        m_choice_loader->value(0);
        cy += gap;

        m_input_java = new Fl_Input(cx + 70, cy, cw - 70, ch, "Java:");
        m_input_java->color(COL_INPUT_BG);
        m_input_java->textcolor(COL_TEXT);
        m_input_java->labelcolor(COL_TEXT);
        m_input_java->value(core::default_java_path().c_str());
        cy += gap;

        m_slider_ram_max = new Fl_Slider(cx + 70, cy, cw - 70, ch, "RAM MB:");
        m_slider_ram_max->type(FL_HORIZONTAL);
        m_slider_ram_max->color(COL_INPUT_BG);
        m_slider_ram_max->selection_color(COL_PROGRESS);
        m_slider_ram_max->minimum(256);
        m_slider_ram_max->maximum(1024);
        m_slider_ram_max->step(64);
        m_slider_ram_max->value(512);
        m_slider_ram_max->labelcolor(COL_TEXT);
        cy += gap;

        m_input_username = new Fl_Input(cx + 70, cy, cw - 70, ch, "User:");
        m_input_username->color(COL_INPUT_BG);
        m_input_username->textcolor(COL_TEXT);
        m_input_username->labelcolor(COL_TEXT);
        m_input_username->value("Player");
        cy += gap + 10;

        m_btn_create_confirm = new Fl_Button(cx, cy, 100, 36, "Create");
        m_btn_create_confirm->box(FL_FLAT_BOX);
        m_btn_create_confirm->color(COL_BTN_PLAY);
        m_btn_create_confirm->labelcolor(FL_WHITE);
        m_btn_create_confirm->callback(on_create_confirm, this);

        m_btn_create_back = new Fl_Button(cx + 110, cy, 100, 36, "Cancel");
        m_btn_create_back->box(FL_FLAT_BOX);
        m_btn_create_back->color(COL_CARD);
        m_btn_create_back->labelcolor(COL_TEXT);
        m_btn_create_back->callback(on_create_back, this);
    }
    m_page_create->end();

    // Settings page
    m_page_settings = new Fl_Group(px, 0, pw, ph);
    m_page_settings->hide();
    {
        Fl_Box* hdr = new Fl_Box(px, 0, pw, 40, "Settings");
        hdr->box(FL_FLAT_BOX);
        hdr->color(COL_SIDEBAR);
        hdr->labelcolor(COL_TEXT);
        hdr->labelsize(16);
        hdr->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

        int sx = px + 20, sy = 60, sw = pw - 40, sh = 28, sgap = 40;

        Fl_Box* lbl = new Fl_Box(sx, sy, sw, 20, "Global settings (applied to new instances)");
        lbl->labelcolor(COL_TEXT_DIM);
        lbl->labelsize(11);
        lbl->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        sy += sgap;

        m_set_java = new Fl_Input(sx + 70, sy, sw - 70, sh, "Java:");
        m_set_java->color(COL_INPUT_BG);
        m_set_java->textcolor(COL_TEXT);
        m_set_java->labelcolor(COL_TEXT);
        m_set_java->value(core::default_java_path().c_str());
        sy += sgap;

        m_set_ram_min = new Fl_Slider(sx + 70, sy, sw - 70, sh, "Min RAM:");
        m_set_ram_min->type(FL_HORIZONTAL);
        m_set_ram_min->color(COL_INPUT_BG);
        m_set_ram_min->selection_color(COL_PROGRESS);
        m_set_ram_min->minimum(128);
        m_set_ram_min->maximum(512);
        m_set_ram_min->step(64);
        m_set_ram_min->value(256);
        m_set_ram_min->labelcolor(COL_TEXT);
        sy += sgap;

        m_set_ram_max = new Fl_Slider(sx + 70, sy, sw - 70, sh, "Max RAM:");
        m_set_ram_max->type(FL_HORIZONTAL);
        m_set_ram_max->color(COL_INPUT_BG);
        m_set_ram_max->selection_color(COL_PROGRESS);
        m_set_ram_max->minimum(256);
        m_set_ram_max->maximum(1024);
        m_set_ram_max->step(64);
        m_set_ram_max->value(512);
        m_set_ram_max->labelcolor(COL_TEXT);
        sy += sgap;

        m_set_username = new Fl_Input(sx + 70, sy, sw - 70, sh, "User:");
        m_set_username->color(COL_INPUT_BG);
        m_set_username->textcolor(COL_TEXT);
        m_set_username->labelcolor(COL_TEXT);
        m_set_username->value("Player");
        sy += sgap;

        Fl_Button* save_btn = new Fl_Button(sx, sy, 100, 36, "Save");
        save_btn->box(FL_FLAT_BOX);
        save_btn->color(COL_BTN_PLAY);
        save_btn->labelcolor(FL_WHITE);
        save_btn->callback(on_save_settings, this);
    }
    m_page_settings->end();

    // ── Bottom bar ──
    {
        int by = h - bottom_h;
        Fl_Box* bb = new Fl_Box(0, by, w, bottom_h);
        bb->color(COL_SIDEBAR);
        bb->box(FL_FLAT_BOX);

        m_progress = new Fl_Progress(10, by + 6, w - 20, 12);
        m_progress->minimum(0);
        m_progress->maximum(1);
        m_progress->value(0);
        m_progress->color(COL_INPUT_BG);
        m_progress->selection_color(COL_PROGRESS);

        m_status = new Fl_Text_Display(10, by + 22, w - 20, 22);
        m_status->box(FL_NO_BOX);
        m_status->color(COL_SIDEBAR);
        m_status->textcolor(COL_TEXT_DIM);
        m_status->textsize(11);
        m_status->buffer(new Fl_Text_Buffer());
    }

    end();
    resizable(m_instance_scroll);

    m_mgr.refresh();
    rebuild_cards();
    show_page(m_page_instances);
}

void MainWindow::set_progress(float val, const std::string& text) {
    std::lock_guard<std::mutex> lock(m_mx);
    m_progress_val = val;
    snprintf(m_status_buf, sizeof(m_status_buf), "%s", text.c_str());
}

void MainWindow::apply_progress() {
    std::lock_guard<std::mutex> lock(m_mx);
    m_progress->value(m_progress_val);
    if (m_status_buf[0]) m_status->buffer()->text(m_status_buf);
}

void MainWindow::show_page(Fl_Group* page) {
    m_page_instances->hide();
    m_page_create->hide();
    m_page_settings->hide();
    page->show();

    // Highlight active sidebar button
    m_btn_instances->color(COL_SIDEBAR);
    m_btn_create->color(COL_SIDEBAR);
    m_btn_settings_btn->color(COL_SIDEBAR);
    if (page == m_page_instances) m_btn_instances->color(COL_CARD);
    else if (page == m_page_create) m_btn_create->color(COL_CARD);
    else if (page == m_page_settings) m_btn_settings_btn->color(COL_CARD);
}

void MainWindow::nav_instances(Fl_Widget*, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    self->show_page(self->m_page_instances);
}

void MainWindow::nav_create(Fl_Widget*, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    self->populate_versions();
    self->show_page(self->m_page_create);
}

void MainWindow::nav_settings(Fl_Widget*, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    self->show_page(self->m_page_settings);
}

void MainWindow::rebuild_cards() {
    // Clear old cards
    m_instance_scroll->begin();
    for (auto* c : m_cards) { m_instance_scroll->remove(c); delete c; }
    m_cards.clear();

    auto& insts = m_mgr.instances();
    int cx = m_instance_scroll->x() + 4;
    int cy = m_instance_scroll->y() + 4;
    int cw = m_instance_scroll->w() - 12;

    for (auto& inst : insts) {
        auto* card = new InstanceCard(cx, cy, cw, InstanceCard::CARD_H, inst);
        card->callback(on_card_click, this);
        m_cards.push_back(card);
        cy += InstanceCard::CARD_H + 4;
    }

    // Empty state
    if (m_cards.empty()) {
        auto* empty = new Fl_Box(cx, cy + 40, cw, 30, "No instances yet.\nClick + to create one.");
        empty->labelcolor(COL_TEXT_DIM);
        empty->labelsize(13);
        m_cards.push_back(nullptr);
    }

    m_instance_scroll->end();
    m_instance_scroll->redraw();
}

void MainWindow::populate_versions() {
    m_choice_version->clear();
    if (!m_manifest.loaded) {
        m_manifest.fetch();
    }
    auto releases = m_manifest.releases();
    for (auto& v : releases) {
        m_choice_version->add(v.id.c_str());
    }
    if (m_choice_version->size() > 0) m_choice_version->value(0);
}

void MainWindow::on_card_click(Fl_Widget* w, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    auto* card = dynamic_cast<InstanceCard*>(w);
    if (!card) return;
    launcher::Instance inst = card->instance();
    self->do_download_and_launch(
        self->m_mgr.find(inst.name));
}

void MainWindow::on_create_confirm(Fl_Widget*, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    std::string name = self->m_input_name->value();
    if (name.empty()) { fl_alert("Name required!"); return; }

    std::string ver = self->m_choice_version->text(self->m_choice_version->value() + 1);
    std::string loader = self->m_choice_loader->text(self->m_choice_loader->value() + 1);

    // Lowercase loader name
    std::transform(loader.begin(), loader.end(), loader.begin(), ::tolower);

    launcher::Instance inst;
    inst.name = name;
    inst.mc_version = ver;
    inst.loader = loader;
    inst.java_path = self->m_input_java->value();
    inst.ram_max = (int)self->m_slider_ram_max->value();
    inst.ram_min = std::min(256, inst.ram_max / 2);
    inst.username = self->m_input_username->value();
    inst.offline = true;

    self->m_mgr.create_instance(name, ver);
    self->m_mgr.save_instance(inst);
    self->rebuild_cards();
    self->show_page(self->m_page_instances);
}

void MainWindow::on_create_back(Fl_Widget*, void* data) {
    static_cast<MainWindow*>(data)->show_page(
        static_cast<MainWindow*>(data)->m_page_instances);
}

void MainWindow::on_save_settings(Fl_Widget*, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    fl_message("Settings saved (for new instances)");
}

// ── Download & Launch ──
void MainWindow::do_download_and_launch(int idx) {
    if (idx < 0 || idx >= (int)m_mgr.instances().size()) return;
    auto& inst = m_mgr.instances()[idx];
    set_progress(0, "Downloading " + inst.mc_version + "...");
    m_downloading = true;
    Fl::flush();

    std::string version = inst.mc_version;
    std::string game_dir = inst.instance_dir() + "/.minecraft";
    MainWindow* self = this;

    Fl::add_timeout(0.15, timer_cb, this);

    std::thread([self, version, game_dir, inst]() {
        bool ok = self->m_downloader.download_all(version, game_dir,
            [self](int cur, int total, const std::string& file) {
                float pct = (total > 0) ? (float)cur / (float)total : 0;
                std::string label;
                if (!file.empty()) {
                    auto pos = file.find_last_of('/');
                    label = (pos != std::string::npos) ? file.substr(pos + 1) : file;
                } else {
                    label = std::to_string(cur) + " / " + std::to_string(total);
                }
                self->set_progress(pct, label);
            });

        self->m_downloading = false;

        if (ok) {
            self->set_progress(1.0f, "Launching " + inst.mc_version + "...");
            launcher::launch_minecraft(inst);
        } else {
            std::string err = "Download failed";
            std::string last = core::http_last_error();
            if (!last.empty()) err += ": " + last;
            self->set_progress(0, err);
        }
    }).detach();
}
