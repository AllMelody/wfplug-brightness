#ifndef WIDGETS_BRIGHTNESS_HPP
#define WIDGETS_BRIGHTNESS_HPP

#include <widget.hpp>
#include <gtkmm/button.h>
#include <gtkmm/image.h>
#include <gtkmm/menu.h>

extern "C" {
#include "plugin.h"
}

class WayfireBrightness : public PanelWidget
{
    std::unique_ptr<Gtk::Button> plugin;
    Gtk::Image *icon;
    std::unique_ptr<Gtk::Menu> menu;
    int current_pct = 50;

    void build_menu();
    void on_clicked();
    void set_brightness(int pct);
    void update_tooltip();

  public:
    void widget_init(Gtk::HBox *container) override;
    void widget_set_icon() override;
    virtual ~WayfireBrightness();
};

#endif
