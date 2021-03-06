/**************************************************************************
 * Copyright (C) 2010       (mrovi@interfete-web-club.com)
 *
 *
 **************************************************************************/

#ifndef LAUNCHER_H
#define LAUNCHER_H

#include "common.h"
#include "area.h"
#include "xsettings-client.h"
#include "icon-theme-common.h"

extern IconThemeWrapper *icon_theme_wrapper;
void load_icon_themes();
void free_icon_themes();

typedef struct Launcher {
    Area area;          // always start with area
    GSList *list_apps;  // List of char*, each is a path to a app.desktop file
    GSList *list_icons; // List of LauncherIcon*
    int icon_size;
} Launcher;

typedef struct LauncherIcon {
    Area area;          // always start with area
    char *config_path;
    Imlib_Image image;
    Imlib_Image image_hover;
    Imlib_Image image_pressed;
    char *cmd;
    char *cwd;
    gboolean start_in_terminal;
    gboolean startup_notification;
    char *icon_name;
    char *icon_path;
    char *icon_tooltip;
    int icon_size;
    int x, y;
} LauncherIcon;

extern gboolean launcher_enabled;
extern int launcher_max_icon_size;
extern gboolean launcher_tooltip_enabled;
extern int launcher_alpha;
extern int launcher_saturation;
extern int launcher_brightness;
extern char *icon_theme_name_xsettings; // theme name
extern char *icon_theme_name_config;
extern gboolean launcher_icon_theme_override;
extern Background *launcher_icon_bg;
extern GList *launcher_icon_gradients;

void default_launcher();
// default global data

void init_launcher();
// initialize launcher : y position, precision, ...
// NOTE: dumb (not implemented)

void init_launcher_panel(void *panel);
void cleanup_launcher();
void cleanup_launcher_theme(Launcher *launcher);

gboolean resize_launcher(void *obj);
void draw_launcher(void *obj, cairo_t *c);
void launcher_default_icon_theme_changed();

void launcher_load_icons(Launcher *launcher);
// Populates the list_icons list

void launcher_action(LauncherIcon *icon, XEvent *e, int x, int y);

void test_launcher_read_desktop_file();
void test_launcher_read_theme_file();

#endif
