/**************************************************************************
*
* Copyright (C) 2008 Pål Staurland (staura@gmail.com)
* Modified (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr) from Omega distribution
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License version 2
* as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************************/

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <pango/pangocairo.h>

#include "common.h"
#include "server.h"
#include "config.h"
#include "window.h"
#include "task.h"
#include "panel.h"
#include "tooltip.h"

void panel_clear_background(void *obj);

MouseAction mouse_left;
MouseAction mouse_middle;
MouseAction mouse_right;
MouseAction mouse_scroll_up;
MouseAction mouse_scroll_down;
MouseAction mouse_tilt_left;
MouseAction mouse_tilt_right;

TaskbarMode taskbar_mode;
gboolean wm_menu;
gboolean panel_dock;
gboolean panel_pivot_struts;
Layer panel_layer;
PanelPosition panel_position;
gboolean panel_horizontal;
gboolean panel_redraw;
gboolean task_dragged;
char *panel_window_name = NULL;
gboolean debug_geometry;
gboolean debug_gradients;
gboolean startup_notifications;
gboolean debug_thumbnails;
gboolean debug_blink;
gboolean panel_autohide;
int panel_autohide_show_timeout;
int panel_autohide_hide_timeout;
int panel_autohide_height;
gboolean panel_shrink;
StrutPolicy panel_strut_policy;
char *panel_items_order;

int max_tick_urgent;

// panel's initial config
Panel panel_config;
// panels (one panel per monitor)
Panel *panels;
int num_panels;

GArray *backgrounds;
GArray *gradients;

double ui_scale_dpi_ref;
double ui_scale_monitor_size_ref;

Imlib_Image default_icon;
char *default_font = NULL;

void default_panel()
{
    ui_scale_dpi_ref = 0;
    ui_scale_monitor_size_ref = 0;
    panels = NULL;
    num_panels = 0;
    default_icon = NULL;
    task_dragged = FALSE;
    panel_horizontal = TRUE;
    panel_position = CENTER;
    panel_items_order = NULL;
    panel_autohide = FALSE;
    panel_autohide_show_timeout = 0;
    panel_autohide_hide_timeout = 0;
    panel_autohide_height = 5; // for vertical panels this is of course the width
    panel_shrink = FALSE;
    panel_strut_policy = STRUT_FOLLOW_SIZE;
    panel_dock = FALSE;         // default not in the dock
    panel_pivot_struts = FALSE;
    panel_layer = BOTTOM_LAYER; // default is bottom layer
    strdup_static(panel_window_name, "tint2");
    wm_menu = FALSE;
    max_tick_urgent = 14;
    mouse_left = TOGGLE_ICONIFY;
    backgrounds = g_array_new(0, 0, sizeof(Background));
    gradients = g_array_new(0, 0, sizeof(GradientClass));

    memset(&panel_config, 0, sizeof(Panel));
    snprintf(panel_config.area.name, strlen_const(panel_config.area.name), "Panel");
    panel_config.mouse_over_alpha = 100;
    panel_config.mouse_over_saturation = 0;
    panel_config.mouse_over_brightness = 10;
    panel_config.mouse_pressed_alpha = 100;
    panel_config.mouse_pressed_saturation = 0;
    panel_config.mouse_pressed_brightness = 0;
    panel_config.mouse_effects = TRUE;

    // First background is always fully transparent
    Background transparent_bg;
    init_background(&transparent_bg);
    g_array_append_val(backgrounds, transparent_bg);
    GradientClass transparent_gradient;
    init_gradient(&transparent_gradient, GRADIENT_VERTICAL);
    g_array_append_val(gradients, transparent_gradient);
}

void cleanup_panel()
{
    if (!panels)
        return;

    for (int i = 0; i < num_panels; i++) {
        Panel *p = &panels[i];

        free_area(&p->area);
        if (p->temp_pmap) {
            XFreePixmap(server.display, p->temp_pmap);
            p->temp_pmap = None;
        }
        if (p->hidden_pixmap) {
            XFreePixmap(server.display, p->hidden_pixmap);
            p->hidden_pixmap = None;
        }
        if (p->main_win) {
            XDestroyWindow(server.display, p->main_win);
            p->main_win = None;
        }
        destroy_timer(&p->autohide_timer);
        cleanup_freespace(p);
    }

    free_icon_themes();
    free_and_null( panel_items_order);
    free_and_null( panel_window_name);
    free_and_null( panels);
    free_area(&panel_config.area);

    g_array_free(backgrounds, TRUE);
    backgrounds = NULL;
    if (gradients)
    {
        for (guint i = 0; i < gradients->len; i++)
            cleanup_gradient(&g_array_index(gradients, GradientClass, i));
        g_array_free(gradients, TRUE);
    }
    gradients = NULL;
    pango_font_description_free(panel_config.g_task.font_desc);
    panel_config.g_task.font_desc = NULL;
    pango_font_description_free(panel_config.taskbarname_font_desc);
    panel_config.taskbarname_font_desc = NULL;
}

void init_panel()
{
    if (panel_config.monitor > (server.num_monitors - 1)) {
        // server.num_monitors minimum value is 1 (see get_monitors())
        fprintf(stderr, "tint2: warning : monitor not found. tint2 default to all monitors.\n");
        panel_config.monitor = 0;
    }

    fprintf(stderr, "tint2: panel items: %s\n", panel_items_order);

    icon_theme_wrapper = NULL;

    init_tooltip();
    init_systray();
    init_launcher();
    init_clock();
#ifdef ENABLE_BATTERY
    init_battery();
#endif
    init_taskbar();
    init_separator();
    init_execp();
    init_button();

    // number of panels (one monitor or 'all' monitors)
    num_panels = panel_config.monitor >= 0 ? 1 : server.num_monitors;
    panels = calloc(num_panels, sizeof(Panel));
    for (int i = 0; i < num_panels; i++)
    {
        panels[i] = panel_config;
        INIT_TIMER(panels[i].autohide_timer);
    }

    fprintf(stderr,
            "tint2: nb monitors %d, nb monitors used %d, nb desktops %d\n",
            server.num_monitors,
            num_panels,
            server.num_desktops);
    for (int i = 0; i < num_panels; i++) {
        Panel *p = &panels[i];

        if (panel_config.monitor < 0)
            p->monitor = i;
        p->scale = (ui_scale_dpi_ref > 0 && server.monitors[p->monitor].dpi > 0)
                    ? server.monitors[p->monitor].dpi / ui_scale_dpi_ref : 1;
        if (ui_scale_monitor_size_ref > 0)
            p->scale *= server.monitors[p->monitor].height / ui_scale_monitor_size_ref;
        if (p->scale > 8 || p->scale < 1./8) {
            fprintf(stderr, RED "tint2: panel %d having scale %g outside bounds, resetting to 1.0" RESET "\n", i + 1, p->scale);
            p->scale = 1;
        }
        fprintf(stderr, BLUE "tint2: panel %d uses scale %g " RESET "\n", i + 1, p->scale);
        if (!p->area.bg)
            p->area.bg = &g_array_index(backgrounds, Background, 0);
        p->area.parent = p;
        p->area.panel = p;
        snprintf(p->area.name, strlen_const(p->area.name), "Panel %d", i);
        p->area.on_screen = TRUE;
        p->area.resize_needed = TRUE;
        p->area.size_mode = LAYOUT_DYNAMIC;
        p->area._resize = resize_panel;
        p->area._clear = panel_clear_background;
        p->separator_list = NULL;
        init_panel_geometry(p);
        area_gradients_create(&p->area);
        // add children according to panel_items
        for_panel_items_order()
        {
            switch (panel_items_order[k]) {
            case 'L':   init_launcher_panel(p);
                        break;
            case 'T':   init_taskbar_panel(p);
                        break;
#ifdef ENABLE_BATTERY
            case 'B':   init_battery_panel(p);
                        break;
#endif
            case 'S':   if (systray_on_monitor(i, num_panels)) {
                            init_systray_panel(p);
                            refresh_systray = TRUE;
                        }
                        break;
            case 'C':   init_clock_panel(p);
                        break;
            case 'F':   if (!strstr(panel_items_order, "T"))
                            init_freespace_panel(p);
                        break;
            case ':':   init_separator_panel(p);
                        break;
            case 'E':   init_execp_panel(p);
                        break;
            case 'P':   init_button_panel(p);
                        break;
            }
        }
        set_panel_items_order(p);

        // catch some events
        XSetWindowAttributes att = {
            .colormap = server.colormap, .background_pixel = 0, .border_pixel = 0,
            .event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | PropertyChangeMask
        };
        if (p->mouse_effects || p->g_task.tooltip_enabled || p->clock.area._get_tooltip_text ||
            (launcher_enabled && launcher_tooltip_enabled))
        {
            att.event_mask |= PointerMotionMask | LeaveWindowMask;
        }
        if (panel_autohide)
            att.event_mask |= LeaveWindowMask | EnterWindowMask;
        p->main_win = XCreateWindow(server.display, server.root_win,
                                    p->posx, p->posy, p->area.width, p->area.height, 0,
                                    server.depth,
                                    InputOutput,
                                    server.visual,
                                    CWEventMask | CWColormap | CWBackPixel | CWBorderPixel,
                                    &att);
        if (!server.gc) {
            XGCValues gcv;
            server.gc = XCreateGC(server.display, p->main_win, 0, &gcv);
        }
        // fprintf(stderr, "tint2: panel %d : %d, %d, %d, %d\n", i, p->posx, p->posy, p->area.width, p->area.height);
        set_panel_properties(p);
        set_panel_background(p);

        if (snapshot_path)
            continue;

        // if we are not in 'snapshot' mode then map new panel
        XMapWindow(server.display, p->main_win);

        if (panel_autohide)
            autohide_trigger_hide(p, false);
    }

    taskbar_refresh_tasklist();
    reset_active_task();
    update_all_taskbars_visibility();
}

void panel_get_size(Panel *panel)
// FIXME: width and height terms are somehow messed in this code
{
    Monitor *mon = server.monitors + panel->monitor;
    if (panel_horizontal)
    {
        if (panel->area.width == 0) {
            panel->fractional_width = TRUE;
            panel->area.width = 100;
        }
        if (panel->area.height == 0) {
            panel->fractional_height = FALSE;
            panel->area.height = 32;
        }
        if (panel->fractional_width)
            panel->area.width = (mon->width - panel->marginx) * panel->area.width / 100;
        if (panel->fractional_height)
            panel->area.height = (mon->height - panel->marginy) * panel->area.height / 100;
        if (panel->area.bg->border.radius > panel->area.height / 2) {
            fprintf(stderr, "tint2: panel_background_id rounded is too big... please fix your tint2rc\n");
            g_array_append_val(backgrounds, *panel->area.bg);
            panel->area.bg = &g_array_index(backgrounds, Background, backgrounds->len - 1);
            panel->area.bg->border.radius = panel->area.height / 2;
        }
        ////////////////////////////////
        if (!panel->fractional_width)
            panel->area.width *= panel->scale;
        if (!panel->fractional_height)
            panel->area.height *= panel->scale;
    }
    else
    {
        if (panel->area.height == 0) {
            panel->fractional_height = TRUE;
            panel->area.height = 100;
        }
        if (panel->area.width == 0) {
            panel->fractional_width = FALSE;
            panel->area.width = 140;
        }
        int old_panel_height = panel->area.height;

        panel->area.height = panel->fractional_width ?
            (mon->height - panel->marginy) * panel->area.width / 100 :
            panel->area.width;

        panel->area.width = panel->fractional_height ?
            (mon->width - panel->marginx) * old_panel_height / 100 :
            old_panel_height;

        if (panel->area.bg->border.radius > panel->area.width / 2) {
            fprintf(stderr, "tint2: panel_background_id rounded is too big... please fix your tint2rc\n");
            g_array_append_val(backgrounds, *panel->area.bg);
            panel->area.bg = &g_array_index(backgrounds, Background, backgrounds->len - 1);
            panel->area.bg->border.radius = panel->area.width / 2;
        }
        ////////////////////////////////
        if (!panel->fractional_width)
            panel->area.height *= panel->scale;
        if (!panel->fractional_height)
            panel->area.width *= panel->scale;
    }

    if (panel->area.width + panel->marginx > mon->width)
        panel->area.width = mon->width - panel->marginx;
    if (panel->area.height + panel->marginy > mon->height)
        panel->area.height = mon->height - panel->marginy;

    panel->max_size = panel_horizontal ? panel->area.width : panel->area.height;
}

void panel_get_position(Panel *panel)
{
    Monitor *mon = server.monitors + panel->monitor;
    // panel position determined here
    panel->posx = mon->x + (
        panel_position & LEFT  ? panel->marginx
    :   panel_position & RIGHT ?  mon->width - panel->area.width - panel->marginx
    :   panel_horizontal       ? (mon->width - panel->area.width) / 2
    :                          panel->marginx);

    panel->posy = mon->y + (
        panel_position & TOP    ? panel->marginy
    :   panel_position & BOTTOM ? mon->height - panel->area.height - panel->marginy
    :                           (mon->height - panel->area.height) / 2);

    // autohide or strut_policy=minimum
    if (panel_horizontal) {
        panel->hidden_width = panel->area.width;
        panel->hidden_height = panel_autohide_height;
    } else {
        panel->hidden_width  = panel_autohide_height;
        panel->hidden_height = panel->area.height;
    }
    // fprintf(stderr, "tint2: panel : posx %d, posy %d, width %d, height %d\n", panel->posx, panel->posy, panel->area.width,
    // panel->area.height);
}

void init_panel_geometry(Panel *panel)
{
    panel_get_size(panel);
    panel_get_position(panel);
}

gboolean resize_panel(void *obj)
{
    Panel *panel = obj;
    relayout_with_constraint(&panel->area, 0);

    // fprintf(stderr, "tint2: resize_panel\n");
    if (taskbar_enabled) {
        if (taskbar_mode != MULTI_DESKTOP)
        {
            // propagate width/height on hidden taskbar
            int width  = panel->taskbar[ server.desktop].area.width;
            int height = panel->taskbar[ server.desktop].area.height;
            for (int i = 0; i < panel->num_desktops; i++)
            {
                if (i == server.desktop)
                    continue;
                if (panel->taskbar[i].area.width  != width || panel->taskbar[i].area.height != height)
                {
                    panel->taskbar[i].area.resize_needed = TRUE;
                    panel->taskbar[i].area.width  = width;
                    panel->taskbar[i].area.height = height;
                }
            }
        }
        else if (taskbar_distribute_size)
        {
            for (int i = 0; i < panel->num_desktops; i++) {
                Taskbar *taskbar = &panel->taskbar[i];

                taskbar->area.old_width  = taskbar->area.width;
                taskbar->area.old_height = taskbar->area.height;
            }

            int total_size = 0;
            int num_tasks = 0;

            for (int i = 0; i < panel->num_desktops; i++)
            {
                Taskbar *taskbar = &panel->taskbar[i];
                if (! taskbar->area.on_screen)
                    continue;

                // The total available size, with excluded borders, padding, spacings
                // and taskbarname

                if (panel_horizontal) {
                    total_size += taskbar->area.width;
                    taskbar->area.width  = left_right_border_width((Area *)taskbar)
                                            + 2 * taskbar->area.paddingx * panel->scale;
                } else {
                    total_size += taskbar->area.height;
                    taskbar->area.height = top_bottom_border_width((Area *)taskbar)
                                            + 2 * taskbar->area.paddingx * panel->scale;
                }

                if (taskbarname_enabled && taskbar->area.children) {
                    Area *name = (Area *)&taskbar->bar_name;
                    if (name->on_screen) {
                        if (panel_horizontal)
                            taskbar->area.width += name->width;
                        else
                            taskbar->area.height += name->height;
                    }
                }
                int gaps_count = -1;
                for (GList *l = taskbar->area.children; l; l = l->next)
                {
                    Area *child = l->data;
                    if (!child->on_screen)
                        continue;

                    gaps_count++;

                    // By the way: Compute the total number of tasks
                    if (!taskbarname_enabled || l != taskbar->area.children)
                        num_tasks++;
                }
                if (gaps_count > 0) {
                    if (panel_horizontal)
                        taskbar->area.width  += gaps_count * taskbar->area.spacing * panel->scale;
                    else
                        taskbar->area.height += gaps_count * taskbar->area.spacing * panel->scale;
                }
                total_size -= panel_horizontal ? taskbar->area.width : taskbar->area.height;
            }

            // Distribute the remaining size between taskbars

            if (num_tasks > 0)
            {
                int task_size = total_size / num_tasks;
                if (taskbar_alignment != ALIGN_LEFT)
                    task_size = MIN(task_size, panel_horizontal ? panel_config.g_task.maximum_width
                                                                : panel_config.g_task.maximum_height);
                for (int i = 0; i < panel->num_desktops; i++)
                {
                    Taskbar *taskbar = &panel->taskbar[i];
                    if (!taskbar->area.on_screen)
                        continue;

                    for (GList *l = taskbar->area.children; l; l = l->next)
                    {
                        if (taskbarname_enabled && l == taskbar->area.children)
                            continue;
                        Area *child = l->data;
                        if (child->on_screen)
                        {
                            if (panel_horizontal)
                                taskbar->area.width += task_size;
                            else
                                taskbar->area.height += task_size;
                        }
                    }
                }
                int slack = total_size - num_tasks * task_size;
                switch (taskbar_alignment)
                {
                    case ALIGN_RIGHT:
                        for (int i = 0; i < panel->num_desktops; i++)
                        {
                            Taskbar *taskbar = &panel->taskbar[i];
                            if (!taskbar->area.on_screen)
                                continue;

                            if (panel_horizontal)
                                taskbar->area.width += slack;
                            else
                                taskbar->area.height += slack;

                            break;
                        }
                        break;
                    case ALIGN_CENTER: {
                        Taskbar *left_taskbar = NULL;
                        Taskbar *right_taskbar = NULL;
                        for (int i = 0; i < panel->num_desktops; i++)
                        {
                            Taskbar *taskbar = &panel->taskbar[i];
                            if (!taskbar->area.on_screen)
                                continue;
                            left_taskbar = taskbar;
                            break;
                        }
                        if (! left_taskbar)
                            break;
                        for (int i = panel->num_desktops - 1; ; i--)
                        {
                            Taskbar *taskbar = &panel->taskbar[i];
                            if (!taskbar->area.on_screen)
                                continue;
                            right_taskbar = taskbar;
                            break;
                        }
                        if (panel_horizontal) {
                            if (left_taskbar != right_taskbar) {
                                slack /= 2;
                                right_taskbar->area.width += slack;
                                right_taskbar->area.alignment = ALIGN_LEFT;
                                left_taskbar->area.alignment = ALIGN_RIGHT;
                            }
                            left_taskbar->area.width += slack;
                        } else {
                            if (left_taskbar != right_taskbar) {
                                slack /= 2;
                                right_taskbar->area.height += slack;
                                right_taskbar->area.alignment = ALIGN_LEFT;
                                left_taskbar->area.alignment = ALIGN_RIGHT;
                            }
                            left_taskbar->area.height += slack;
                        }
                        break;
                    }
                }
            } else
                // No tasks => expand the first visible taskbar
                for (int i = 0; i < panel->num_desktops; i++)
                {
                    Taskbar *taskbar = &panel->taskbar[i];
                    if (!taskbar->area.on_screen)
                        continue;

                    if (panel_horizontal)
                        taskbar->area.width += total_size;
                    else
                        taskbar->area.height += total_size;

                    break;
                }
            for (int i = 0; i < panel->num_desktops; i++) {
                Taskbar *taskbar = &panel->taskbar[i];
                taskbar->area.resize_needed = taskbar->area.old_width  != taskbar->area.width ||
                                              taskbar->area.old_height != taskbar->area.height;
            }
        }
    }
    for (GList *l = panel->freespace_list; l; l = l->next)
        resize_freespace(l->data);
    return FALSE;
}

void update_strut(Panel *p)
{
    if (panel_strut_policy == STRUT_NONE) {
        XDeleteProperty(server.display, p->main_win, server.atom [_NET_WM_STRUT]);
        XDeleteProperty(server.display, p->main_win, server.atom [_NET_WM_STRUT_PARTIAL]);
        return;
    }

    // Reserved space
    unsigned int d1, screen_width, screen_height;
    Window d2;
    int d3;
    XGetGeometry(server.display, server.root_win, &d2, &d3, &d3, &screen_width, &screen_height, &d1, &d1);
    Monitor monitor = server.monitors[p->monitor];
    StrutType struts[STRUT_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    if (panel_horizontal ^ panel_pivot_struts) {
        int height = p->area.height + p->marginy;
        if (panel_strut_policy == STRUT_MINIMUM
            || (panel_strut_policy == STRUT_FOLLOW_SIZE && panel_autohide && p->is_hidden))

            height = p->hidden_height;
        if (panel_position & TOP) {
            struts[STRUT_TOP] = height + monitor.y;
            struts[STRUT_TOP_X1] = p->posx;
            // p->area.width - 1 allowed full screen on monitor 2
            struts[STRUT_TOP_X2] = p->posx + p->area.width - 1;
        } else {
            struts[STRUT_BOTTOM] = height + screen_height - monitor.y - monitor.height;
            struts[STRUT_BOTTOM_X1] = p->posx;
            // p->area.width - 1 allowed full screen on monitor 2
            struts[STRUT_BOTTOM_X2] = p->posx + p->area.width - 1;
        }
    } else {
        int width = p->area.width + p->marginx;
        if (panel_strut_policy == STRUT_MINIMUM || (panel_strut_policy == STRUT_FOLLOW_SIZE && panel_autohide && p->is_hidden))
            width = p->hidden_width;
        if (panel_position & LEFT) {
            struts[STRUT_LEFT] = width + monitor.x;
            struts[STRUT_LEFT_Y1] = p->posy;
            // p->area.width - 1 allowed full screen on monitor 2
            struts[STRUT_LEFT_Y2] = p->posy + p->area.height - 1;
        } else {
            struts[STRUT_RIGHT] = width + screen_width - monitor.x - monitor.width;
            struts[STRUT_RIGHT_Y1] = p->posy;
            // p->area.width - 1 allowed full screen on monitor 2
            struts[STRUT_RIGHT_Y2] = p->posy + p->area.height - 1;
        }
    }
    // Old specification : fluxbox need _NET_WM_STRUT.
    XChangeProperty(server.display, p->main_win,
                    server.atom [_NET_WM_STRUT],
                    XA_CARDINAL,
                    32,
                    PropModeReplace,
                    (unsigned char *)&struts,
                    STRUT_COUNT_OLD);
    XChangeProperty(server.display, p->main_win,
                    server.atom [_NET_WM_STRUT_PARTIAL],
                    XA_CARDINAL,
                    32,
                    PropModeReplace,
                    (unsigned char *)&struts,
                    STRUT_COUNT);
}

void set_panel_items_order(Panel *p)
{
    if (p->area.children) {
        g_list_free(p->area.children);
        p->area.children = 0;
    }
    GList *ch_tail = NULL;

    #define ADD_CHILD(c) g_list_append_tail (p->area.children, ch_tail, (c))

    int i_execp = 0;
    int i_separator = 0;
    int i_freespace = 0;
    int i_button = 0;
    for_panel_items_order()
    {
        int i = p - panels;
        switch (panel_items_order[k]) {
        case 'L':   ADD_CHILD (&p->launcher);
                    p->launcher.area.resize_needed = TRUE;
                    break;
        case 'T':   for (int j = 0; j < p->num_desktops; j++)
                        ADD_CHILD (&p->taskbar[j]);
                    break;
#ifdef ENABLE_BATTERY
        case 'B':   ADD_CHILD (&p->battery);
                    break;
#endif
        case 'S':   if (systray_on_monitor(i, num_panels))
                        ADD_CHILD (&systray);
                    break;
        case 'C':   ADD_CHILD (&p->clock);
                    break;
        case 'F': { GList *item = g_list_nth(p->freespace_list, i_freespace);
                    i_freespace++;
                    if (item)
                        ADD_CHILD (item->data);
                    break; }
        case ':': { GList *item = g_list_nth(p->separator_list, i_separator);
                    i_separator++;
                    if (item)
                        ADD_CHILD (item->data);
                    break; }
        case 'E': { GList *item = g_list_nth(p->execp_list, i_execp);
                    i_execp++;
                    if (item)
                        ADD_CHILD (item->data);
                    break; }
        case 'P': { GList *item = g_list_nth(p->button_list, i_button);
                    i_button++;
                    if (item)
                        ADD_CHILD (item->data);
                    break; }
        }
    }
    initialize_positions (&p->area);

    #undef ADD_CHILD
}

void place_panel_all_desktops(Panel *p)
{
    long val = ALL_DESKTOPS;
    XChangeProperty(server.display, p->main_win,
                    server.atom [_NET_WM_DESKTOP],
                    XA_CARDINAL,
                    32,
                    PropModeReplace,
                    (unsigned char *)&val,
                    1);
}

void set_panel_layer(Panel *p, Layer layer)
{
    int num_atoms;
    Atom state[4] = {
        [0] = server.atom [_NET_WM_STATE_SKIP_PAGER],
        [1] = server.atom [_NET_WM_STATE_SKIP_TASKBAR],
        [2] = server.atom [_NET_WM_STATE_STICKY],
    };
    if (layer == NORMAL_LAYER)
        num_atoms = 3;
    else
        num_atoms = 4,
        state[3] = server.atom[ layer == BOTTOM_LAYER ? _NET_WM_STATE_BELOW : _NET_WM_STATE_ABOVE ];
    XChangeProperty(server.display, p->main_win,
                    server.atom [_NET_WM_STATE],
                    XA_ATOM,
                    32,
                    PropModeReplace,
                    (unsigned char *)state,
                    num_atoms);
}

void replace_panel_all_desktops(Panel *p)
{
    XClientMessageEvent m;
    memset(&m, 0, sizeof(m));
    m.type = ClientMessage;
    m.send_event = True;
    m.display = server.display;
    m.window = p->main_win;
    m.message_type = server.atom [_NET_WM_DESKTOP];
    m.format = 32;
    m.data.l[0] = ALL_DESKTOPS;
    XSendEvent(server.display, server.root_win, False, SubstructureRedirectMask | SubstructureNotifyMask, (XEvent *)&m);
    XSync(server.display, False);
}

void set_panel_window_geometry(Panel *panel)
{
    update_strut(panel);

    // Fixed position and non-resizable window
    // Allow panel move and resize when tint2 reload config file
    XSizeHints size_hints = {
        .flags = PPosition | PMinSize | PMaxSize,
        .min_width = panel_autohide ? panel->hidden_width : panel->area.width,
        .max_width = panel->area.width,
        .min_height = panel_autohide ? panel->hidden_height : panel->area.height,
        .max_height = panel->area.height,
    };
    XSetWMNormalHints(server.display, panel->main_win, &size_hints);

    if (!panel->is_hidden)
        XMoveResizeWindow(server.display, panel->main_win,
                          panel->posx, panel->posy, panel->area.width, panel->area.height);
    else if (panel_horizontal)
        XMoveResizeWindow(server.display, panel->main_win,
                          panel->posx,
                          panel_position & TOP ? panel->posy : panel->posy + panel->area.height - panel_autohide_height,
                          panel->hidden_width, panel->hidden_height);
    else
        XMoveResizeWindow(server.display, panel->main_win,
                          panel_position & LEFT ? panel->posx : panel->posx + panel->area.width - panel_autohide_height,
                          panel->posy,
                          panel->hidden_width, panel->hidden_height);
}

void set_panel_properties(Panel *p)
{
    XStoreName(server.display, p->main_win, panel_window_name);
    XSetIconName(server.display, p->main_win, panel_window_name);

    gsize len;
    gchar *name = g_locale_to_utf8(panel_window_name, -1, NULL, &len, NULL);
    if (name != NULL) {
        XChangeProperty(server.display, p->main_win,
                        server.atom [_NET_WM_NAME],
                        server.atom [UTF8_STRING],
                        8,
                        PropModeReplace,
                        (unsigned char *)name,
                        (int)len);
        XChangeProperty(server.display, p->main_win,
                        server.atom [_NET_WM_ICON_NAME],
                        server.atom [UTF8_STRING],
                        8,
                        PropModeReplace,
                        (unsigned char *)name,
                        (int)len);
        g_free(name);
    }

    long pid = getpid();
    XChangeProperty(server.display, p->main_win,
                    server.atom [_NET_WM_PID],
                    XA_CARDINAL,
                    32,
                    PropModeReplace,
                    (unsigned char *)&pid,
                    1);

    // Dock
    XChangeProperty(server.display, p->main_win,
                    server.atom [_NET_WM_WINDOW_TYPE],
                    XA_ATOM,
                    32,
                    PropModeReplace,
                    (unsigned char *)&server.atom [_NET_WM_WINDOW_TYPE_DOCK],
                    1);

    place_panel_all_desktops(p);
    set_panel_layer(p, panel_layer);

    XWMHints wmhints;
    memset(&wmhints, 0, sizeof(wmhints));
    if (panel_dock) {
        // Necessary for placing the panel into the dock on Openbox and Fluxbox.
        // See https://gitlab.com/o9000/tint2/issues/465
        wmhints.flags = IconWindowHint | WindowGroupHint | StateHint;
        wmhints.icon_window = wmhints.window_group = p->main_win;
        wmhints.initial_state = WithdrawnState;
    }
    // We do not need keyboard input focus.
    wmhints.flags |= InputHint;
    wmhints.input = False;
    XSetWMHints(server.display, p->main_win, &wmhints);

    // Undecorated
    long prop[5] = {2, 0, 0, 0, 0};
    XChangeProperty(server.display, p->main_win,
                    server.atom [_MOTIF_WM_HINTS],
                    server.atom [_MOTIF_WM_HINTS],
                    32,
                    PropModeReplace,
                    (unsigned char *)prop,
                    5);

    // XdndAware - Register for Xdnd events
    Atom version = 4;
    XChangeProperty(server.display, p->main_win,
                    server.atom [XdndAware],
                    XA_ATOM,
                    32,
                    PropModeReplace,
                    (unsigned char *)&version,
                    1);

    // Set WM_CLASS
    XClassHint *classhint = XAllocClassHint();
    classhint->res_name = (char *)"tint2";
    classhint->res_class = (char *)"Tint2";
    XSetClassHint(server.display, p->main_win, classhint);
    XFree(classhint);

    set_panel_window_geometry(p);
}

void panel_clear_background(void *obj)
{
    Panel *p = obj;
    if (! p->area.pix)
        return;

    if (server.real_transparency)
        clear_pixmap(p->area.pix, 0, 0, p->area.width, p->area.height);
    else {
        get_root_pixmap();
        // copy background (server.root_pmap) in panel.area.pix
        Window dummy;
        int x, y;
        XTranslateCoordinates(server.display, p->main_win, server.root_win, 0, 0, &x, &y, &dummy);
        if (panel_autohide && p->is_hidden)
        {
            if (panel_horizontal) {
                if (panel_position & BOTTOM)
                    y -= p->area.height - p->hidden_height;
            } else
                if (panel_position & RIGHT)
                    x -= p->area.width - p->hidden_width;
        }
        XSetTSOrigin(server.display, server.gc, -x, -y);
        XFillRectangle(server.display, p->area.pix, server.gc, 0, 0, p->area.width, p->area.height);
    }
}

void set_panel_background(Panel *p)
{
    panel_clear_background(p);
    schedule_redraw(&p->area);

    if (p->hidden_pixmap) {
        XFreePixmap(server.display, p->hidden_pixmap);
        p->hidden_pixmap = None;
    }
}

Panel *get_panel(Window win)
{
    for (int i = 0; i < num_panels; i++)
        if (panels[i].main_win == win)
            return &panels[i];
    return NULL;
}

Taskbar *click_taskbar(Panel *panel, int x, int y)
{
    for (int i = 0; i < panel->num_desktops; i++) {
        Taskbar *taskbar = &panel->taskbar[i];
        if (area_is_under_mouse(taskbar, x, y))
            return taskbar;
    }
    return NULL;
}

Task *click_task(Panel *panel, int x, int y)
{
    Taskbar *taskbar = click_taskbar(panel, x, y);
    if (taskbar)
        for_taskbar_tasks( taskbar, l)
        {
            Task *task = l->data;
            if (area_is_under_mouse(task, x, y))
                return task;
        }
    return NULL;
}

Launcher *click_launcher(Panel *panel, int x, int y)
{
    Launcher *launcher = &panel->launcher;
    return area_is_under_mouse(launcher, x, y) ? launcher : NULL;
}

LauncherIcon *click_launcher_icon(Panel *panel, int x, int y)
{
    Launcher *launcher = click_launcher(panel, x, y);
    if (launcher)
        for (GSList *l = launcher->list_icons; l; l = l->next) {
            LauncherIcon *icon = l->data;
            if (area_is_under_mouse(icon, x, y))
                return icon;
        }
    return NULL;
}

Clock *click_clock(Panel *panel, int x, int y)
{
    Clock *clock = &panel->clock;
    return area_is_under_mouse(clock, x, y) ? clock : NULL;
}

#ifdef ENABLE_BATTERY
Battery *click_battery(Panel *panel, int x, int y)
{
    Battery *bat = &panel->battery;
    return area_is_under_mouse(bat, x, y) ? bat : NULL;
}
#endif

Execp *click_execp(Panel *panel, int x, int y)
{
    for (GList *l = panel->execp_list; l; l = l->next) {
        Execp *execp = l->data;
        if (area_is_under_mouse(execp, x, y))
            return execp;
    }
    return NULL;
}

Button *click_button(Panel *panel, int x, int y)
{
    for (GList *l = panel->button_list; l; l = l->next) {
        Button *button = l->data;
        if (area_is_under_mouse(button, x, y))
            return button;
    }
    return NULL;
}

void stop_autohide_timer(Panel *p)
{
    stop_timer(&p->autohide_timer);
}

void autohide_show(void *p)
{
    Panel *panel = p;
    stop_autohide_timer(panel);
    panel->is_hidden = FALSE;
    XMapSubwindows(server.display, panel->main_win); // systray windows
    set_panel_window_geometry(panel);
    set_panel_layer(panel, TOP_LAYER);
    refresh_systray = TRUE; // ugly hack, because we actually only need to call XSetBackgroundPixmap
    schedule_panel_redraw();
}

void autohide_hide(void *p)
{
    Panel *panel = p;
    stop_autohide_timer(panel);
    set_panel_layer(panel, panel_layer);
    panel->is_hidden = TRUE;
    XUnmapSubwindows(server.display, panel->main_win); // systray windows
    set_panel_window_geometry(panel);
    schedule_panel_redraw();
}

void autohide_trigger_show(Panel *p, bool forced)
{
    if (!p)
        return;
    change_timer(&p->autohide_timer, true, forced ? 0 : panel_autohide_show_timeout, 0, autohide_show, p);
}

void autohide_trigger_hide(Panel *p, bool forced)
{
    if (!p)
        return;

    if (!forced) // check, is mouse over system tray icon
    {
        Window root, child;
        int xr, yr, xw, yw;
        unsigned int mask;
        if (XQueryPointer(server.display, p->main_win, &root, &child, &xr, &yr, &xw, &yw, &mask))
        {
            // WARNING: first_hide flag is important to make it work from init_panel(), when XQueryPointer
            // returns true, but None child, though mouse coordinates are valid
            static bool first_hide = true;
            if (child)
                return;
            if (first_hide)
            {
                first_hide = false;
                if (area_is_under_mouse (&p->area, xw, yw))
                    return;
            }
        }
    }
    change_timer(&p->autohide_timer, true, forced ? 0 : panel_autohide_hide_timeout, 0, autohide_hide, p);
}

void shrink_panel(Panel *panel)
{
    int size = MIN(get_desired_size(&panel->area), panel->max_size);
    gboolean update = FALSE;
    if (panel_horizontal) {
        if (panel->area.width != size) {
            panel->area.width = size;
            update = TRUE;
        }
    } else {
        if (panel->area.height != size) {
            panel->area.height = size;
            update = TRUE;
        }
    }
    if (update) {
        panel_get_position(panel);
        set_panel_window_geometry(panel);
        set_panel_background(panel);
        panel->area.resize_needed = TRUE;
        systray.area.resize_needed = TRUE;
        schedule_redraw(&systray.area);
        refresh_systray = TRUE;
        update_minimized_icon_positions(panel);
    }
}

void render_panel(Panel *panel)
{
    relayout(&panel->area);
    if (debug_geometry)
        area_dump_geometry(&panel->area, 0);
    update_dependent_gradients(&panel->area);
    draw_tree(&panel->area);
}

const char *get_default_font()
{
    return default_font ? default_font : DEFAULT_FONT;
}

void default_icon_theme_changed()
{
    if (!launcher_enabled && !panel_config.button_list)
        return;
    if (launcher_icon_theme_override && icon_theme_name_config)
        return;

    free_icon_themes();
    load_icon_themes();

    launcher_default_icon_theme_changed();
    button_default_icon_theme_changed();
}

void default_font_changed()
{
#ifdef ENABLE_BATTERY
    battery_default_font_changed();
#endif
    clock_default_font_changed();
    execp_default_font_changed();
    button_default_font_changed();
    taskbar_default_font_changed();
    taskbarname_default_font_changed();
    tooltip_default_font_changed();
}

void _schedule_panel_redraw(const char *file, const char *function, const int line)
{
    panel_redraw = TRUE;
    if (debug_fps) {
        fprintf(stderr, YELLOW "tint2: %s %s %d: triggering panel redraw" RESET "\n", file, function, line);
    }
}

void save_panel_screenshot(const Panel *panel, const char *path)
{
    imlib_context_set_drawable(panel->temp_pmap);
    Imlib_Image img = imlib_create_image_from_drawable(0, 0, 0, panel->area.width, panel->area.height, 1);

    if (!img) {
        XImage *ximg = XGetImage(   server.display, panel->temp_pmap,
                                    0, 0, panel->area.width, panel->area.height,
                                    AllPlanes, ZPixmap);
        if (ximg) {
            DATA32 *pixels = calloc(panel->area.width * panel->area.height, sizeof(DATA32));
            for (int x = 0; x < panel->area.width; x++)
                for (int y = 0; y < panel->area.height; y++)
                {
                    DATA32 xpixel = XGetPixel(ximg, x, y);

                    DATA32 r = (xpixel >> 16) & 0xff;
                    DATA32 g = (xpixel >> 8) & 0xff;
                    DATA32 b = (xpixel >> 0) & 0xff;
                    DATA32 a = 0x0;

                    DATA32 argb = (a << 24) | (r << 16) | (g << 8) | b;
                    pixels[y * panel->area.width + x] = argb;
                }
            XDestroyImage(ximg);
            img = imlib_create_image_using_data(panel->area.width, panel->area.height, pixels);
        }
    }

    if (img) {
        imlib_context_set_image(img);
        if (!panel_horizontal) {
            // rotate 90° vertical panel
            imlib_image_flip_horizontal();
            imlib_image_flip_diagonal();
        }
        imlib_save_image(path);
        imlib_free_image();
    }
}

void save_screenshot(const char *path)
{
    Panel *panel = &panels[0];

    if (panel->area.width > server.monitors[0].width)
        panel->area.width = server.monitors[0].width;

    panel->temp_pmap = XCreatePixmap(   server.display, server.root_win,
                                        panel->area.width, panel->area.height, server.depth);
    render_panel(panel);

    XSync(server.display, False);

    save_panel_screenshot(panel, path);
}
