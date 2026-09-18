#pragma once
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Select_Browser.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Progress.H>
#include "launcher/instance_manager.h"
#include "launcher/manifest.h"
#include "launcher/downloader.h"
#include <mutex>

class MainWindow : public Fl_Double_Window {
public:
    MainWindow(int w, int h, const char* title = "Lightcraft");

    // Public for timer_cb
    bool m_downloading = false;
    void apply_progress();

private:
    Fl_Select_Browser* m_instance_list;
    Fl_Button*         m_btn_new;
    Fl_Button*         m_btn_delete;
    Fl_Button*         m_btn_play;
    Fl_Button*         m_btn_settings;

    Fl_Input*     m_input_name;
    Fl_Choice*    m_choice_version;
    Fl_Choice*    m_choice_loader;
    Fl_Input*     m_input_java;
    Fl_Slider*    m_slider_ram;
    Fl_Input*     m_input_username;
    Fl_Button*    m_btn_save;
    Fl_Button*    m_btn_cancel;

    Fl_Text_Display* m_status;
    Fl_Progress*     m_progress;

    launcher::InstanceManager m_mgr;
    launcher::VersionManifest m_manifest;
    launcher::Downloader      m_downloader;
    int m_selected = -1;
    bool m_settings_open = false;

    std::mutex m_mx;
    float m_progress_val = 0;
    char m_status_buf[256] = {};

    void set_progress(float val, const std::string& text);

    static void on_new(Fl_Widget* w, void* data);
    static void on_delete(Fl_Widget* w, void* data);
    static void on_play(Fl_Widget* w, void* data);
    static void on_settings(Fl_Widget* w, void* data);
    static void on_save(Fl_Widget* w, void* data);
    static void on_cancel(Fl_Widget* w, void* data);
    static void on_list_select(Fl_Widget* w, void* data);

    void refresh_list();
    void show_settings(int idx = -1);
    void hide_settings();
    void set_status(const char* msg);
    void do_download_and_launch(int idx);
};
