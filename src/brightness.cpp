#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <glibmm.h>
#include <gtkmm/checkmenuitem.h>
#include "gtk-utils.hpp"
#include "brightness.hpp"

#define PLUGIN_TITLE N_("Brightness")
#define PWM_CHIP   "/sys/class/pwm/pwmchip0"
#define PWM_CHAN   PWM_CHIP "/pwm0"
#define PWM_PERIOD 1000000
#define DEFAULT_PCT 50

namespace fs = std::filesystem;

extern "C" {
    conf_table_t conf_table[] = {
        { CONF_TYPE_NONE, NULL, NULL, NULL },
    };

    WayfireWidget *create() { return new WayfireBrightness; }
    void destroy(WayfireWidget *w) { delete w; }

    const conf_table_t *config_params(void) { return conf_table; }
    const char *display_name(void) { return PLUGIN_TITLE; }
    const char *package_name(void) { return GETTEXT_PACKAGE; }
}

static fs::path state_path()
{
    return fs::path(g_get_user_config_dir()) / "wfplug-brightness" / "level";
}

static int load_state()
{
    std::ifstream f(state_path());
    if (!f) return DEFAULT_PCT;
    int pct = DEFAULT_PCT;
    f >> pct;
    if (pct < 10) pct = 10;
    if (pct > 100) pct = 100;
    return pct;
}

static void save_state(int pct)
{
    fs::path p = state_path();
    fs::create_directories(p.parent_path());
    std::ofstream f(p);
    f << pct << '\n';
}

static bool write_sysfs(const char *path, const std::string& value)
{
    int fd = open(path, O_WRONLY);
    if (fd < 0)
    {
        g_warning("brightness: open %s: %s", path, strerror(errno));
        return false;
    }
    ssize_t n = write(fd, value.data(), value.size());
    int saved = errno;
    close(fd);
    if (n != (ssize_t) value.size())
    {
        g_warning("brightness: write %s: %s", path, strerror(saved));
        return false;
    }
    return true;
}

static int read_int(const char *path)
{
    std::ifstream f(path);
    if (!f) return -1;
    int v = -1;
    f >> v;
    return v;
}

static bool try_apply_pwm(int pct)
{
    int duty = PWM_PERIOD * pct / 100;
    if (!write_sysfs(PWM_CHAN "/period",     std::to_string(PWM_PERIOD)) ||
        !write_sysfs(PWM_CHAN "/duty_cycle", std::to_string(duty)) ||
        !write_sysfs(PWM_CHAN "/enable",     "1"))
        return false;
    return read_int(PWM_CHAN "/duty_cycle") == duty &&
           read_int(PWM_CHAN "/enable") == 1;
}

static void apply_pwm(int pct)
{
    if (!fs::exists(PWM_CHAN))
    {
        write_sysfs(PWM_CHIP "/export", "0");
        for (int i = 0; i < 100 && !fs::exists(PWM_CHAN "/period"); i++)
            g_usleep(20000);
    }
    for (int attempt = 0; attempt < 5; attempt++)
    {
        if (try_apply_pwm(pct)) return;
        g_usleep(100000);
    }
    g_warning("brightness: failed to apply %d%% after retries", pct);
}

void WayfireBrightness::set_brightness(int pct)
{
    current_pct = pct;
    apply_pwm(pct);
    save_state(pct);
    update_tooltip();
}

void WayfireBrightness::update_tooltip()
{
    gchar *s = g_strdup_printf(_("Brightness: %d%%"), current_pct);
    plugin->set_tooltip_text(s);
    g_free(s);
}

void WayfireBrightness::build_menu()
{
    menu = std::make_unique<Gtk::Menu>();
    for (int pct = 100; pct >= 10; pct -= 10)
    {
        gchar *label = g_strdup_printf("%d%%", pct);
        auto item = Gtk::manage(new Gtk::CheckMenuItem(label));
        g_free(label);
        item->set_active(pct == current_pct);
        item->signal_activate().connect([this, pct] { set_brightness(pct); });
        menu->append(*item);
    }
    menu->show_all();
}

void WayfireBrightness::on_clicked()
{
    build_menu();
    menu->popup_at_widget(plugin.get(),
        Gdk::GRAVITY_SOUTH_WEST, Gdk::GRAVITY_NORTH_WEST, nullptr);
}

void WayfireBrightness::init(Gtk::HBox *container)
{
    plugin = std::make_unique<Gtk::Button>();
    plugin->set_name(PLUGIN_NAME);
    container->pack_start(*plugin, false, false);

    icon = Gtk::manage(new Gtk::Image());
    set_image_icon(*icon, "brightness-panel", 22);
    plugin->set_image(*icon);
    plugin->set_always_show_image(true);

    plugin->signal_clicked().connect(
        sigc::mem_fun(*this, &WayfireBrightness::on_clicked));

    current_pct = load_state();
    update_tooltip();

    plugin->show_all();
}

WayfireBrightness::~WayfireBrightness() {}
