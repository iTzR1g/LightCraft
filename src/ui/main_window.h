#pragma once
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include "launcher/instance_manager.h"
#include "launcher/manifest.h"
#include "launcher/downloader.h"
#include <mutex>
#include <string>
#include <vector>

class InstanceCard : public Fl_Box {
public:
    InstanceCard(int x, int y, int w, int h, const launcher::Instance& inst);
    void draw() override;
    int handle(int event) override;
    launcher::Instance instance() const { return m_inst; }

    static constexpr int CARD_H = 80;

private:
    launcher::Instance m_inst;
    bool m_hover = false;
};

class MainWindow : public Fl_Double_Window {
public:
    MainWindow(int w, int h, const char* title = "Lightcraft");

    bool m_downloading = false;
    void apply_progress();

private:
    // Sidebar
    Fl_Button* m_btn_instances;
    Fl_Button* m_btn_create;
    Fl_Button* m_btn_settings_btn;

    // Pages
    Fl_Group* m_page_instances;
    Fl_Group* m_page_create;
    Fl_Group* m_page_settings;

    // Instance page
    Fl_Scroll* m_instance_scroll;
    std::vector<InstanceCard*> m_cards;

    // Create page
    Fl_Input*    m_input_name;
    Fl_Choice*   m_choice_version;
    Fl_Choice*   m_choice_loader;
    Fl_Input*    m_input_java;
    Fl_Slider*   m_slider_ram_max;
    Fl_Input*    m_input_username;
    Fl_Button*   m_btn_create_confirm;
    Fl_Button*   m_btn_create_back;

    // Settings page
    Fl_Input*  m_set_java;
    Fl_Slider* m_set_ram_min;
    Fl_Slider* m_set_ram_max;
    Fl_Input*  m_set_username;

    // Bottom bar
    Fl_Progress*     m_progress;
    Fl_Text_Display* m_status;

    // Data
    launcher::InstanceManager m_mgr;
    launcher::VersionManifest m_manifest;
    launcher::Downloader      m_downloader;

    std::mutex m_mx;
    float m_progress_val = 0;
    char m_status_buf[256] = {};

    void set_progress(float val, const std::string& text);

    // Navigation
    void show_page(Fl_Group* page);
    static void nav_instances(Fl_Widget*, void*);
    static void nav_create(Fl_Widget*, void*);
    static void nav_settings(Fl_Widget*, void*);

    // Instance actions
    static void on_card_click(Fl_Widget*, void*);

    // Create actions
    static void on_create_confirm(Fl_Widget*, void*);
    static void on_create_back(Fl_Widget*, void*);

    // Settings actions
    static void on_save_settings(Fl_Widget*, void*);

    void rebuild_cards();
    void populate_versions();
};
