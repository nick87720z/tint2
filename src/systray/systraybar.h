/**************************************************************************
* Copyright (C) 2009 thierry lorthiois (lorthiois@bbsoft.fr)
*
* systraybar
* systray implementation come from 'docker-1.5' by Ben Jansens,
* and from systray/xembed specification (freedesktop.org).
*
**************************************************************************/

#ifndef SYSTRAYBAR_H
#define SYSTRAYBAR_H

#include "common.h"
#include "area.h"
#include "timer.h"
#include <X11/extensions/Xdamage.h>

#define XEMBED_EMBEDDED_NOTIFY 0
// XEMBED messages

#define XEMBED_MAPPED (1 << 0)
// Flags for _XEMBED_INFO

typedef enum SystraySortMethod {
    SYSTRAY_SORT_ASCENDING,
    SYSTRAY_SORT_DESCENDING,
    SYSTRAY_SORT_LEFT2RIGHT,
    SYSTRAY_SORT_RIGHT2LEFT,
} SystraySortMethod;

typedef struct {
    Area area;  // always start with area

    GSList *list_icons;
    SystraySortMethod sort;
    int alpha, saturation, brightness;
    int icon_size, icons_per_column, icons_per_row, margin;
} Systray;

typedef struct {
    Window win;     // The actual tray icon window (created by the application)
    Window parent;  // The parent window created by tint2 to embed the icon
    int x, y;
    int width, height;
    int depth;
    gboolean reparented;
    gboolean embedded;
    int pid;        // Process PID or zero.
    int chrono;     // A number that is incremented for each new icon, used to sort them by the order in which they were created.
    char *name;     // Name of the tray icon window.
    
    // Members used for rendering
    struct timespec time_last_render;
    int num_fast_renders;
    Timer render_timer;

    // Members used for resizing
    int bad_size_counter;
    struct timespec time_last_resize;
    Timer resize_timer;

    Imlib_Image image;  // Icon contents if we are compositing the icon, otherwise null
    Damage damage;      // XDamage
} TrayWindow;

extern Window net_sel_win;  // net_sel_win != None when protocol started
extern Systray systray;
extern gboolean refresh_systray;
extern gboolean systray_enabled;
extern int systray_max_icon_size;
extern int systray_monitor;
extern gboolean systray_profile;
extern char *systray_hide_name_filter;

void default_systray(); // default global data
void cleanup_systray(); // freed memory

void init_systray();
void init_systray_panel(void *p);
// initialize protocol and panel position

void draw_systray(void *obj, cairo_t *c);
gboolean resize_systray(void *obj);
void on_change_systray(void *obj);
gboolean systray_on_monitor(int i_monitor, int num_panels);

// systray protocol
// many tray icon doesn't manage stop/restart of the systray manager
void start_net();
void stop_net();
void handle_systray_event(XClientMessageEvent *e);

gboolean add_icon(Window id);
gboolean reparent_icon(TrayWindow *traywin);
gboolean embed_icon(TrayWindow *traywin);
void remove_icon(TrayWindow *traywin, bool destroyed);

void refresh_systray_icons();
void systray_render_icon(void *t);
void systray_resize_request_event(TrayWindow *traywin, XEvent *e);
void systray_reconfigure_event(TrayWindow *traywin, XEvent *e);
void systray_property_notify(TrayWindow *traywin, XEvent *e);
void systray_destroy_event(TrayWindow *traywin);

TrayWindow *systray_find_icon(Window win);

#endif
