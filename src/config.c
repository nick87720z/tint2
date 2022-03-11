/**************************************************************************
*
* Tint2 : read/write config file
*
* Copyright (C) 2007 Pål Staurland (staura@gmail.com)
* Modified (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr) from Omega distribution
* Modified (C) 2022 Nikita Zlobin (nick87720z@gmail.com)
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

#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glib/gstdio.h>
#include <pango/pango-font.h>
#include <Imlib2.h>

#include "config.h"
#include "config-keys.h"

#ifndef TINT2CONF

#include "tint2rc.h"
#include "common.h"
#include "server.h"
#include "panel.h"
#include "task.h"
#include "taskbar.h"
#include "taskbarname.h"
#include "systraybar.h"
#include "launcher.h"
#include "clock.h"
#include "window.h"
#include "tooltip.h"
#include "timer.h"
#include "separator.h"
#include "execplugin.h"

#ifdef ENABLE_BATTERY
#include "battery.h"
#endif

#endif

#define _STR_EXTEND(s, L) \
int L; do{                \
    L = strlen(s);        \
    s = realloc(s, L+2);  \
}while(0)

// NOTE: c is 1-char string (null-terminated),
// not a char itself

// also, parts of string could be copied faster using wider data type (up to word)
#define STR_APPEND_CH(str, c)       \
do{ char *s=(str);                  \
    if (s) {                        \
        _STR_EXTEND(s, L);          \
        s[L]=(c)[0], s[L+1]=(c)[1]; \
    } else                          \
        s = strdup(c);              \
}while(0)

#define STR_PREPEND_CH(str, c) \
do{ char *s = (str);           \
    if (s) {                   \
        _STR_EXTEND(s, L);     \
        memmove (s+1, s, L+1); \
        s[0] = (c)[0];         \
    } else                     \
        s = strdup(c);         \
}while(0)

// global path
char *config_path = NULL;
char *snapshot_path = NULL;

#ifndef TINT2CONF

// --------------------------------------------------
// backward compatibility
// detect if it's an old config file (==1)
static gboolean new_config_file;

static gboolean read_bg_color_hover;
static gboolean read_border_color_hover;
static gboolean read_bg_color_press;
static gboolean read_border_color_press;
static gboolean read_panel_position;

void default_config()
{
    config_path = NULL;
    snapshot_path = NULL;
    new_config_file = FALSE;
    read_panel_position = FALSE;
}

void cleanup_config()
{
    free_and_null(config_path);
    free_and_null(snapshot_path);
}

void get_action(char *event, MouseAction *action)
{
    switch (str_index ( event,
                        (char *[]){ "close",
                                    "desktop_left",
                                    "desktop_right",
                                    "iconify",
                                    "maximize_restore",
                                    "next_task",
                                    "none",
                                    "prev_task",
                                    "shade",
                                    "toggle",
                                    "toggle_iconify"},
                        MOUSE_ACTIONS)) {
    case  0: *action = CLOSE;           break;
    case  1: *action = DESKTOP_LEFT;    break;
    case  2: *action = DESKTOP_RIGHT;   break;
    case  3: *action = ICONIFY;         break;
    case  4: *action = MAXIMIZE_RESTORE; break;
    case  5: *action = NEXT_TASK;       break;
    case  6: *action = NONE;            break;
    case  7: *action = PREV_TASK;       break;
    case  8: *action = SHADE;           break;
    case  9: *action = TOGGLE;          break;
    case 10: *action = TOGGLE_ICONIFY;  break;
    default:
        fprintf(stderr, "tint2: Error: unrecognized action '%s'. Please fix your config file.\n", event);
    }
}

int get_task_status(char *status)
{
    return  (int []){-1, TASK_ACTIVE, TASK_ICONIFIED, TASK_URGENT}
            [ str_index (status, (char *[]){"active", "iconified", "urgent"}, 3) + 1 ];
}

int config_get_monitor(char *monitor)
{
    if (strcmp(monitor, "primary") == 0) {
        for (int i = 0; i < server.num_monitors; ++i) {
            if (server.monitors[i].primary)
                return i;
        }
        return 0;
    }
    if (strcmp(monitor, "all") == 0) {
        return -1;
    }
    char *endptr;
    int ret_int = strtol(monitor, &endptr, 10);
    if (*endptr == 0)
        return ret_int - 1;
    else {
        // monitor specified by name, not by index
        int i, j;
        for (i = 0; i < server.num_monitors; ++i) {
            if (server.monitors[i].names == 0)
                // xrandr can't identify monitors
                continue;
            j = 0;
            while (server.monitors[i].names[j] != 0) {
                if (strcmp(monitor, server.monitors[i].names[j++]) == 0)
                    return i;
            }
        }
    }

    // monitor not found or xrandr can't identify monitors => all
    return -1;
}

void load_launcher_app_dir(const char *path)
{
    GList *subdirs = NULL;
    GList *files = NULL;

    GDir *d = g_dir_open(path, 0, NULL);
    if (d) {
        size_t  path_len = strlen (path),
                buf_len = 0;
        const gchar *name;
        gchar *file = NULL;
        while ((name = g_dir_read_name(d)))
        {
            size_t _buf_len = path_len + 1 + strlen (name) + 1;
            if (! file || _buf_len > buf_len) {
                file = realloc( file, (buf_len = _buf_len));
                file [buf_len-1] = '\0';
            }
            snprintf (file, buf_len - 1, "%s/%s", path, name);
            if (!g_file_test(file, G_FILE_TEST_IS_DIR) && g_str_has_suffix(file, ".desktop")) {
                files = g_list_prepend(files, file);
                file = NULL;
            } else if (g_file_test(file, G_FILE_TEST_IS_DIR)) {
                subdirs = g_list_prepend(subdirs, file);
                file = NULL;
            } else
                free( file);
        }
        if (file)
            free( file);
        g_dir_close(d);
    }

    subdirs = g_list_sort(subdirs, compare_strings);
    for (GList *l = subdirs; l; l = l->next)
    {
        gchar *dir = (gchar *)l->data;
        load_launcher_app_dir(dir);
        free( dir);
    }
    g_list_free(subdirs);

    files = g_list_sort(files, compare_strings);
    for (GList *l = files; l; l = l->next)
    {
        gchar *file = (gchar *)l->data;
        panel_config.launcher.list_apps = g_slist_append( panel_config.launcher.list_apps, file);
    }
    g_list_free(files);
}

Separator *get_or_create_last_separator()
{
    if (!panel_config.separator_list) {
        fprintf(stderr, "tint2: Warning: separator items should shart with 'separator = new'\n");
        panel_config.separator_list = g_list_append(panel_config.separator_list, create_separator());
    }
    return (Separator *)g_list_last(panel_config.separator_list)->data;
}

Execp *get_or_create_last_execp()
{
    if (!panel_config.execp_list) {
        fprintf(stderr, "tint2: Warning: execp items should start with 'execp = new'\n");
        panel_config.execp_list = g_list_append(panel_config.execp_list, create_execp());
    }
    return (Execp *)g_list_last(panel_config.execp_list)->data;
}

Button *get_or_create_last_button()
{
    if (!panel_config.button_list) {
        fprintf(stderr, "tint2: Warning: button items should start with 'button = new'\n");
        panel_config.button_list = g_list_append(panel_config.button_list, create_button());
    }
    return (Button *)g_list_last(panel_config.button_list)->data;
}

void add_entry(char *key, char *value)
{
    #define VALUES_TO_COLOR(color, first) do {                                 \
        Color *_c = &(color);                                                  \
        extract_values(value, values, 3);                                      \
        get_color(values[(first)], _c->rgb);                                   \
        _c->alpha = values[(first)+1] ? atoi(values[(first)+1]) / 100.0 : 1.0; \
    } while(0)

    #define VALUES_TO_AREA_PADDING(obj, first) do {                       \
        extract_values(value, values, 3);                                 \
        (obj).paddingx = (obj).spacing = atoi(values[(first)+0]);      \
        (obj).paddingy = values[(first)+1] ? atoi(values[(first)+1]) : 0; \
        if (values[(first)+2])                                            \
            (obj).spacing = atoi(values[(first)+2]);                     \
    } while(0)

    #define VALUES_TO_ASB(type, first) (                \
        extract_values(value, values, 3),               \
        type ## _alpha = atoi(values[(first)+0]),       \
        type ## _saturation = atoi(values[(first)+1]),  \
        type ## _brightness = atoi(values[(first)+2]) )

    #define VALUE_TO_BACKGROUND(bg) do {                  \
        int id = atoi(value);                             \
        id = (id < backgrounds->len && id >= 0) ? id : 0; \
        bg = &g_array_index(backgrounds, Background, id); \
    } while(0)

    #define VALUE_TO_GRADIENT(gradient) do {                         \
        int id = atoi(value);                                        \
        id = (id < gradients->len && id >= 0) ? id : -1;             \
        if (id >= 0)                                                 \
            gradient = &g_array_index(gradients, GradientClass, id); \
    } while(0)

    #define VALUE_DUP_IF_SET(dst) if (*value) dst = strdup(value)

    #define VALUE_TO_COMMAND(cmd) do { \
        free_and_null(cmd);            \
        VALUE_DUP_IF_SET(cmd);            \
    } while(0)

    char *values[] = {NULL, NULL, NULL, NULL};
    cfg_key_t key_i = str_index(key, cfg_keys, DICT_KEYS_NUM);

    switch (key_i) {
    /* Background and border */
    case key_scale_relative_to_dpi:
        ui_scale_dpi_ref = atof(value);
        break;
    case key_scale_relative_to_screen_height:
        ui_scale_monitor_size_ref = atof(value);
        break;
    case key_rounded: {
        // 'rounded' is the first parameter => alloc a new background
        if (backgrounds->len > 0) {
            Background *bg = &g_array_index(backgrounds, Background, backgrounds->len - 1);
            if (!read_bg_color_hover)       bg->fill_color_hover     = bg->fill_color;
            if (!read_border_color_hover)   bg->border_color_hover   = bg->border.color;
            if (!read_bg_color_press)       bg->fill_color_pressed   = bg->fill_color_hover;
            if (!read_border_color_press)   bg->border_color_pressed = bg->border_color_hover;
        }
        Background bg;
        init_background(&bg);
        bg.border.radius = atoi(value);
        g_array_append_val(backgrounds, bg);
        read_bg_color_hover     = FALSE;
        read_border_color_hover = FALSE;
        read_bg_color_press     = FALSE;
        read_border_color_press = FALSE;
        break;
    }
    case key_rounded_corners: {
        Background *bg = &g_array_index(backgrounds, Background, backgrounds->len - 1);
        bg->border.rmask = 0;
        if (extract_values(value, values, 4))
            for (int i=0; i<4 && values[i]; i++) {
                if (strcmp(values[i], "tl") == 0 || strcmp(values[i], "TL") == 0)
                    bg->border.rmask |= CORNER_TL;
                if (strcmp(values[i], "tr") == 0 || strcmp(values[i], "TR") == 0)
                    bg->border.rmask |= CORNER_TR;
                if (strcmp(values[i], "br") == 0 || strcmp(values[i], "BR") == 0)
                    bg->border.rmask |= CORNER_BR;
                if (strcmp(values[i], "bl") == 0 || strcmp(values[i], "BL") == 0)
                    bg->border.rmask |= CORNER_BL;
            }
        if (!bg->border.rmask)
            bg->border.radius = 0;
        break;
    }
    case key_border_width:
        g_array_index(backgrounds, Background, backgrounds->len - 1)
            .border.width = atoi(value);
        break;
    case key_border_sides: {
        Background *bg = &g_array_index(backgrounds, Background, backgrounds->len - 1);
        bg->border.mask = 0;
        if (strchr(value, 'l') || strchr(value, 'L'))
            bg->border.mask |= BORDER_LEFT;
        if (strchr(value, 'r') || strchr(value, 'R'))
            bg->border.mask |= BORDER_RIGHT;
        if (strchr(value, 't') || strchr(value, 'T'))
            bg->border.mask |= BORDER_TOP;
        if (strchr(value, 'b') || strchr(value, 'B'))
            bg->border.mask |= BORDER_BOTTOM;
        if (!bg->border.mask)
            bg->border.width = 0;
        break;
    }
    case key_background_color:
        VALUES_TO_COLOR (g_array_index(backgrounds, Background, backgrounds->len - 1).fill_color, 0);
        break;
    case key_border_color:
        VALUES_TO_COLOR (g_array_index(backgrounds, Background, backgrounds->len - 1).border.color, 0);
        break;
    case key_background_color_hover:
        VALUES_TO_COLOR (g_array_index(backgrounds, Background, backgrounds->len - 1).fill_color_hover, 0);
        read_bg_color_hover = 1;
        break;
    case key_border_color_hover:
        VALUES_TO_COLOR (g_array_index(backgrounds, Background, backgrounds->len - 1).border_color_hover, 0);
        read_border_color_hover = 1;
        break;
    case key_background_color_pressed:
        VALUES_TO_COLOR (g_array_index(backgrounds, Background, backgrounds->len - 1).fill_color_pressed, 0);
        read_bg_color_press = 1;
        break;
    case key_border_color_pressed:
        VALUES_TO_COLOR (g_array_index(backgrounds, Background, backgrounds->len - 1).border_color_pressed, 0);
        read_border_color_press = 1;
        break;
    case key_gradient_id:
        VALUE_TO_GRADIENT (g_array_index(backgrounds, Background, backgrounds->len - 1).gradients[MOUSE_NORMAL]);
        break;
    case key_gradient_id_hover:
    case key_hover_gradient_id:
        VALUE_TO_GRADIENT (g_array_index(backgrounds, Background, backgrounds->len - 1).gradients[MOUSE_OVER]);
        break;
    case key_gradient_id_pressed:
    case key_pressed_gradient_id:
        VALUE_TO_GRADIENT (g_array_index(backgrounds, Background, backgrounds->len - 1).gradients[MOUSE_DOWN]);
        break;
    case key_border_content_tint_weight:
        g_array_index(backgrounds, Background, backgrounds->len - 1)
            .border_content_tint_weight = MAX(0.0, MIN(1.0, atoi(value) / 100.));
        break;
    case key_background_content_tint_weight: {
        g_array_index(backgrounds, Background, backgrounds->len - 1)
            .fill_content_tint_weight = MAX(0.0, MIN(1.0, atoi(value) / 100.));
        break;
    }

    /* Gradients */
    case key_gradient: {
        // Create a new gradient
        GradientClass g;
        init_gradient(&g, gradient_type_from_string(value));
        g_array_append_val(gradients, g);
        break;
    }
    case key_start_color:
        VALUES_TO_COLOR (g_array_index(gradients, GradientClass, gradients->len - 1).start_color, 0);
        break;
    case key_end_color:
        VALUES_TO_COLOR (g_array_index(gradients, GradientClass, gradients->len - 1).end_color, 0);
        break;
    case key_color_stop: {
        GradientClass *g = &g_array_index(gradients, GradientClass, gradients->len - 1);
        ColorStop *color_stop = (ColorStop *)calloc(1, sizeof(ColorStop));
        VALUES_TO_COLOR (color_stop->color, 1);
        color_stop->offset = atof(values[0]) / 100.0;
        g->extra_color_stops = g_list_append(g->extra_color_stops, color_stop);
        break;
    }

    /* TODO: External command sinks */

    /* Panel */
    case key_panel_monitor:
        panel_config.monitor = config_get_monitor(value);
        break;
    case key_panel_shrink:
        panel_shrink = ATOB(value);
        break;
    case key_panel_size: {
        extract_values(value, values, 3);

        char *b;
        if ((b = strchr(values[0], '%'))) {
            b[0] = '\0';
            panel_config.fractional_width = TRUE;
        }
        panel_config.area.width = atoi(values[0]);
        if (panel_config.area.width == 0) {
            // full width mode
            panel_config.area.width = 100;
            panel_config.fractional_width = TRUE;
        }
        if (values[1]) {
            if ((b = strchr(values[1], '%'))) {
                b[0] = '\0';
                panel_config.fractional_height = 1;
            }
            panel_config.area.height = atoi(values[1]);
        }
        break;
    }
    case key_panel_items:
        new_config_file = TRUE;
        free_and_null(panel_items_order);
        panel_items_order = strdup(value);
        systray_enabled = FALSE;
        launcher_enabled = FALSE;
#ifdef ENABLE_BATTERY
        battery_enabled = FALSE;
#endif
        clock_enabled = FALSE;
        taskbar_enabled = FALSE;
        for (int items_n = strlen(panel_items_order), j = 0; j < items_n; j++)
            switch (panel_items_order[j]) {
            case 'L':   launcher_enabled = TRUE;
                        break;
            case 'T':   taskbar_enabled = TRUE;
                        break;
            case 'B':
#ifdef ENABLE_BATTERY
                battery_enabled = TRUE;
#else
                fprintf(stderr, "tint2: tint2 has been compiled without battery support\n");
#endif
                break;
            case 'S':   // systray disabled in snapshot mode
                        if (snapshot_path == NULL)
                            systray_enabled = TRUE;
                        break;
            case 'C':   clock_enabled = TRUE;
                        break;
            }
        break;
    case key_panel_margin:
        extract_values(value, values, 3);
        panel_config.marginx = atoi(values[0]);
        if (values[1])
            panel_config.marginy = atoi(values[1]);
        break;
    case key_panel_padding:
        VALUES_TO_AREA_PADDING(panel_config.area, 0);
        break;
    case key_panel_position:
        read_panel_position = TRUE;
        extract_values(value, values, 3);
        panel_position = (
            !strcmp(values[0], "top"   ) ? TOP 
        :   !strcmp(values[0], "bottom") ? BOTTOM
        :   CENTER
        ) | (
            !values[1] ? CENTER
        :   !strcmp(values[1], "left" ) ? LEFT
        :   !strcmp(values[1], "right") ? RIGHT
        :   CENTER
        );
        panel_horizontal = !values[2] || strcmp(values[2], "vertical");
        break;
    case key_font_shadow:
        panel_config.font_shadow = ATOB(value);
        break;
    case key_panel_background_id:
        VALUE_TO_BACKGROUND (panel_config.area.bg);
        break;
    case key_wm_menu:
        wm_menu = ATOB(value);
        break;
    case key_panel_dock:
        panel_dock = ATOB(value);
        break;
    case key_panel_pivot_struts:
        panel_pivot_struts = ATOB(value);
        break;
    case key_urgent_nb_of_blink:
        max_tick_urgent = atoi(value);
        break;
    case key_panel_layer:
        panel_layer = !strcmp(value, "bottom") ? BOTTOM_LAYER
        :             !strcmp(value, "top"   ) ? TOP_LAYER
        : NORMAL_LAYER;
        break;
    case key_disable_transparency:
        server.disable_transparency = ATOB(value);
        break;
    case key_panel_window_name:
        if (value && *value) {
            free(panel_window_name);
            panel_window_name = strdup(value);
        }
        break;

    /* Battery */
    case key_battery_low_status:
#ifdef ENABLE_BATTERY
        battery_low_status = atoi(value);
        if (battery_low_status < 0 || battery_low_status > 100)
            battery_low_status = 0;
#endif
        break;
    case key_battery_lclick_command:
#ifdef ENABLE_BATTERY
        VALUE_DUP_IF_SET(battery_lclick_command);
#endif
        break;
    case key_battery_mclick_command:
#ifdef ENABLE_BATTERY
        VALUE_DUP_IF_SET(battery_mclick_command);
#endif
        break;
    case key_battery_rclick_command:
#ifdef ENABLE_BATTERY
        VALUE_DUP_IF_SET(battery_rclick_command);
#endif
        break;
    case key_battery_uwheel_command:
#ifdef ENABLE_BATTERY
        VALUE_DUP_IF_SET(battery_uwheel_command);
#endif
        break;
    case key_battery_dwheel_command:
#ifdef ENABLE_BATTERY
        VALUE_DUP_IF_SET(battery_dwheel_command);
#endif
        break;
    case key_battery_low_cmd:
#ifdef ENABLE_BATTERY
        VALUE_DUP_IF_SET(battery_low_cmd);

#endif
        break;
    case key_battery_full_cmd:
#ifdef ENABLE_BATTERY
        VALUE_DUP_IF_SET(battery_full_cmd);
#endif
        break;
    case key_ac_connected_cmd:
#ifdef ENABLE_BATTERY
        VALUE_DUP_IF_SET(ac_connected_cmd);
#endif
        break;
    case key_ac_disconnected_cmd:
#ifdef ENABLE_BATTERY
        VALUE_DUP_IF_SET(ac_disconnected_cmd);
#endif
        break;
    case key_bat1_font:
#ifdef ENABLE_BATTERY
        bat1_font_desc = pango_font_description_from_string(value);
        bat1_has_font = TRUE;
#endif
        break;
    case key_bat2_font:
#ifdef ENABLE_BATTERY
        bat2_font_desc = pango_font_description_from_string(value);
        bat2_has_font = TRUE;
#endif
        break;
    case key_bat1_format:
#ifdef ENABLE_BATTERY
        if (value && *value) {
            free(bat1_format);
            bat1_format = strdup(value);
        }
#endif
        break;
    case key_bat2_format:
#ifdef ENABLE_BATTERY
        if (value && *value) {
            free(bat2_format);
            bat2_format = strdup(value);
        }
#endif
        break;
    case key_battery_font_color:
#ifdef ENABLE_BATTERY
        VALUES_TO_COLOR (panel_config.battery.font_color, 0);
#endif
        break;
    case key_battery_padding:
#ifdef ENABLE_BATTERY
        VALUES_TO_AREA_PADDING(panel_config.battery.area, 0);
#endif
        break;
    case key_battery_background_id: {
#ifdef ENABLE_BATTERY
        VALUE_TO_BACKGROUND (panel_config.battery.area.bg);
#endif
        break;
    }
    case key_battery_hide:
#ifdef ENABLE_BATTERY
        percentage_hide = atoi(value);
        if (percentage_hide == 0)
            percentage_hide = 101;
#endif
        break;
    case key_battery_tooltip:
#ifdef ENABLE_BATTERY
        battery_tooltip_enabled = ATOB(value);
#endif
        break;

    /* Separator */
    case key_separator:
        panel_config.separator_list = g_list_append(panel_config.separator_list, create_separator());
    case key_separator_background_id:
        VALUE_TO_BACKGROUND (get_or_create_last_separator()->area.bg);
        break;
    case key_separator_color:
        VALUES_TO_COLOR (get_or_create_last_separator()->color, 0);
        break;
    case key_separator_style: {
        Separator *separator = get_or_create_last_separator();
        separator->style = 
            g_str_equal(value, "empty") ? SEPARATOR_EMPTY
        :   g_str_equal(value, "line" ) ? SEPARATOR_LINE
        :   g_str_equal(value, "dots" ) ? SEPARATOR_DOTS
        :( fprintf(stderr, RED "tint2: Invalid separator_style value: %s" RESET "\n", value), separator->style );
        break;
    }
    case key_separator_size: {
        Separator *separator = get_or_create_last_separator();
        separator->thickness = atoi(value);
        break;
    }
    case key_separator_padding: {
        Separator *separator = get_or_create_last_separator();
        VALUES_TO_AREA_PADDING(separator->area, 0);
        break;
    }

    /* Execp */
    case key_execp:
        panel_config.execp_list = g_list_append(panel_config.execp_list, create_execp());
        break;
    case key_execp_name: {
        Execp *execp = get_or_create_last_execp();
        execp->backend->name[0] = 0;
        if (value && *value) {
            size_t len = strlen(value);
            if (len > strlen_const(execp->backend->name))
                fprintf(stderr, RED "tint2: execp_name cannot be longer than %ld bytes: '%s'" RESET "\n",
                        strlen_const(execp->backend->name), value);
            else
                memcpy (execp->backend->name, value, len+1);
        }
        break;
    }
    case key_execp_command: {
        Execp *execp = get_or_create_last_execp();
        VALUE_TO_COMMAND (execp->backend->command);
        break;
    }
    case key_execp_interval: {
        Execp *execp = get_or_create_last_execp();
        execp->backend->interval = 0;
        int v = atoi(value);
        if (v < 0) {
            fprintf(stderr, "tint2: execp_interval must be an integer >= 0\n");
        } else {
            execp->backend->interval = v;
        }
        break;
    }
    case key_execp_monitor:
        get_or_create_last_execp()->backend->monitor = config_get_monitor(value);
        break;
    case key_execp_has_icon:
        get_or_create_last_execp()->backend->has_icon = ATOB(value);
        break;
    case key_execp_continuous:
        get_or_create_last_execp()->backend->continuous = MAX(atoi(value), 0);
        break;
    case key_execp_markup:
        get_or_create_last_execp()->backend->has_markup = ATOB(value);
        break;
    case key_execp_cache_icon:
        get_or_create_last_execp()->backend->cache_icon = ATOB(value);
        break;
    case key_execp_tooltip: {
        Execp *execp = get_or_create_last_execp();
        free_and_null(execp->backend->tooltip);
        execp->backend->tooltip = strdup(value);
        execp->backend->has_user_tooltip = TRUE;
        break;
    }
    case key_execp_font: {
        Execp *execp = get_or_create_last_execp();
        pango_font_description_free(execp->backend->font_desc);
        execp->backend->font_desc = pango_font_description_from_string(value);
        execp->backend->has_font = TRUE;
        break;
    }
    case key_execp_font_color:
        VALUES_TO_COLOR (get_or_create_last_execp()->backend->font_color, 0);
        break;
    case key_execp_padding: {
        Execp *execp = get_or_create_last_execp();
        VALUES_TO_AREA_PADDING (*execp->backend, 0);
        break;
    }
    case key_execp_background_id:
        VALUE_TO_BACKGROUND (get_or_create_last_execp()->backend->bg);
        break;
    case key_execp_centered:
        get_or_create_last_execp()->backend->centered = ATOB(value);
        break;
    case key_execp_icon_w: {
        Execp *execp = get_or_create_last_execp();
        int v = atoi(value);
        if (v < 0) {
            fprintf(stderr, "tint2: execp_icon_w must be an integer >= 0\n");
        } else {
            execp->backend->icon_w = v;
        }
        break;
    }
    case key_execp_icon_h: {
        Execp *execp = get_or_create_last_execp();
        int v = atoi(value);
        if (v < 0) {
            fprintf(stderr, "tint2: execp_icon_h must be an integer >= 0\n");
        } else {
            execp->backend->icon_h = v;
        }
        break;
    }
    case key_execp_lclick_command: {
        Execp *execp = get_or_create_last_execp();
        VALUE_TO_COMMAND (execp->backend->lclick_command);
        break;
    }
    case key_execp_mclick_command: {
        Execp *execp = get_or_create_last_execp();
        VALUE_TO_COMMAND (execp->backend->mclick_command);
        break;
    }
    case key_execp_rclick_command: {
        Execp *execp = get_or_create_last_execp();
        VALUE_TO_COMMAND (execp->backend->rclick_command);
        break;
    }
    case key_execp_uwheel_command: {
        Execp *execp = get_or_create_last_execp();
        VALUE_TO_COMMAND (execp->backend->uwheel_command);
        break;
    }
    case key_execp_dwheel_command: {
        Execp *execp = get_or_create_last_execp();
        VALUE_TO_COMMAND (execp->backend->dwheel_command);
        break;
    }
    case key_execp_lclick_command_sink:
        if (value && *value)
            get_or_create_last_execp()->backend->lclick_command_sink = atoi (value);
        break;
    case key_execp_mclick_command_sink:
        if (value && *value)
            get_or_create_last_execp()->backend->mclick_command_sink = atoi (value);
        break;
    case key_execp_rclick_command_sink:
        if (value && *value)
            get_or_create_last_execp()->backend->rclick_command_sink = atoi (value);
        break;
    case key_execp_uwheel_command_sink:
        if (value && *value)
            get_or_create_last_execp()->backend->uwheel_command_sink = atoi (value);
        break;
    case key_execp_dwheel_command_sink:
        if (value && *value)
            get_or_create_last_execp()->backend->dwheel_command_sink = atoi (value);
        break;

    /* Button */
    case key_button:
        panel_config.button_list = g_list_append(panel_config.button_list, create_button());
        break;
    case key_button_icon:
        if (value && *value) {
            Button *button = get_or_create_last_button();
            button->backend->icon_name = expand_tilde(value);
        }
        break;
    case key_button_text:
        if (value && *value) {
            Button *button = get_or_create_last_button();
            free_and_null(button->backend->text);
            button->backend->text = strdup(value);
        }
        break;
    case key_button_tooltip:
        if (value && *value) {
            Button *button = get_or_create_last_button();
            free_and_null(button->backend->tooltip);
            button->backend->tooltip = strdup(value);
        }
        break;
    case key_button_font: {
        Button *button = get_or_create_last_button();
        pango_font_description_free(button->backend->font_desc);
        button->backend->font_desc = pango_font_description_from_string(value);
        button->backend->has_font = TRUE;
        break;
    }
    case key_button_font_color:
        VALUES_TO_COLOR (get_or_create_last_button()->backend->font_color, 0);
        break;
    case key_button_padding: {
        Button *button = get_or_create_last_button();
        VALUES_TO_AREA_PADDING (*button->backend, 0);
        break;
    }
    case key_button_max_icon_size:
        extract_values(value, values, 3);
        get_or_create_last_button()->backend->max_icon_size = MAX(0, atoi(value));
        break;
    case key_button_background_id:
        VALUE_TO_BACKGROUND (get_or_create_last_button()->backend->bg);
        break;
    case key_button_centered:
        get_or_create_last_button()->backend->centered = ATOB(value);
        break;
    case key_button_lclick_command: {
        Button *button = get_or_create_last_button();
        VALUE_TO_COMMAND (button->backend->lclick_command);
        break;
    }
    case key_button_mclick_command: {
        Button *button = get_or_create_last_button();
        VALUE_TO_COMMAND (button->backend->mclick_command);
        break;
    }
    case key_button_rclick_command: {
        Button *button = get_or_create_last_button();
        VALUE_TO_COMMAND (button->backend->rclick_command);
        break;
    }
    case key_button_uwheel_command: {
        Button *button = get_or_create_last_button();
        VALUE_TO_COMMAND (button->backend->uwheel_command);
        break;
    }
    case key_button_dwheel_command: {
        Button *button = get_or_create_last_button();
        VALUE_TO_COMMAND (button->backend->dwheel_command);
        break;
    }

    /* Clock */
    case key_time1_format:
        if (!new_config_file) {
            clock_enabled = TRUE;
            STR_APPEND_CH(panel_items_order, "C");
        }
        if (value && *value) {
            time1_format = strdup(value);
            clock_enabled = TRUE;
        }
        break;
    case key_time2_format:
        VALUE_DUP_IF_SET(time2_format);
        break;
    case key_time1_font:
        time1_font_desc = pango_font_description_from_string(value);
        time1_has_font = TRUE;
        break;
    case key_time1_timezone:
        VALUE_DUP_IF_SET(time1_timezone);
        break;
    case key_time2_timezone:
        VALUE_DUP_IF_SET(time2_timezone);
        break;
    case key_time2_font:
        time2_font_desc = pango_font_description_from_string(value);
        time2_has_font = TRUE;
        break;
    case key_clock_font_color:
        VALUES_TO_COLOR (panel_config.clock.font, 0);
        break;
    case key_clock_padding:
        VALUES_TO_AREA_PADDING(panel_config.clock.area, 0);
        break;
    case key_clock_background_id:
        VALUE_TO_BACKGROUND (panel_config.clock.area.bg);
        break;
    case key_clock_tooltip:
        VALUE_DUP_IF_SET(time_tooltip_format);
        break;
    case key_clock_tooltip_timezone:
        VALUE_DUP_IF_SET(time_tooltip_timezone);
        break;
    case key_clock_lclick_command:
        VALUE_DUP_IF_SET(clock_lclick_command);
        break;
    case key_clock_mclick_command:
        VALUE_DUP_IF_SET(clock_mclick_command);
        break;
    case key_clock_rclick_command:
        VALUE_DUP_IF_SET(clock_rclick_command);
        break;
    case key_clock_uwheel_command:
        VALUE_DUP_IF_SET(clock_uwheel_command);
        break;
    case key_clock_dwheel_command:
        VALUE_DUP_IF_SET(clock_dwheel_command);
        break;

    /* Taskbar */
    case key_taskbar_mode:
        taskbar_mode = !strcmp(value, "multi_desktop") ? MULTI_DESKTOP : SINGLE_DESKTOP;
        break;
    case key_taskbar_distribute_size:
        taskbar_distribute_size = ATOB(value);
        break;
    case key_taskbar_padding:
        VALUES_TO_AREA_PADDING(panel_config.g_taskbar.area, 0);
        break;
    case key_taskbar_background_id:
        VALUE_TO_BACKGROUND (panel_config.g_taskbar.background[TASKBAR_NORMAL]);
        if (panel_config.g_taskbar.background[TASKBAR_ACTIVE] == 0)
            panel_config.g_taskbar.background[TASKBAR_ACTIVE] = panel_config.g_taskbar.background[TASKBAR_NORMAL];
        break;
    case key_taskbar_active_background_id:
        VALUE_TO_BACKGROUND (panel_config.g_taskbar.background[TASKBAR_ACTIVE]);
        break;
    case key_taskbar_name:
        taskbarname_enabled = ATOB(value);
        break;
    case key_taskbar_name_padding:
        VALUES_TO_AREA_PADDING (panel_config.g_taskbar.area_name, 0);
        break;
    case key_taskbar_name_background_id:
        VALUE_TO_BACKGROUND (panel_config.g_taskbar.background_name[TASKBAR_NORMAL]);
        if (panel_config.g_taskbar.background_name[TASKBAR_ACTIVE] == 0)
            panel_config.g_taskbar.background_name[TASKBAR_ACTIVE] =
                panel_config.g_taskbar.background_name[TASKBAR_NORMAL];
        break;
    case key_taskbar_name_active_background_id:
        VALUE_TO_BACKGROUND (panel_config.g_taskbar.background_name[TASKBAR_ACTIVE]);
        break;
    case key_taskbar_name_font:
        panel_config.taskbarname_font_desc = pango_font_description_from_string(value);
        panel_config.taskbarname_has_font = TRUE;
        break;
    case key_taskbar_name_font_color:
        VALUES_TO_COLOR (taskbarname_font, 0);
        break;
    case key_taskbar_name_active_font_color:
        VALUES_TO_COLOR (taskbarname_active_font, 0);
        break;
    case key_taskbar_hide_inactive_tasks:
        hide_inactive_tasks = ATOB(value);
        break;
    case key_taskbar_hide_different_monitor:
        hide_task_diff_monitor = ATOB(value);
        break;
    case key_taskbar_hide_different_desktop:
        hide_task_diff_desktop = ATOB(value);
        break;
    case key_taskbar_hide_if_empty:
        hide_taskbar_if_empty = ATOB(value);
        break;
    case key_taskbar_always_show_all_desktop_tasks:
        always_show_all_desktop_tasks = ATOB(value);
        break;
    case key_taskbar_sort_order:
        taskbar_sort_method =
            (int []){   TASKBAR_NOSORT,
                        TASKBAR_SORT_APPLICATION,
                        TASKBAR_SORT_CENTER,
                        TASKBAR_SORT_LRU,
                        TASKBAR_SORT_MRU,
                        TASKBAR_SORT_TITLE
            }[ str_index (value, (char *[]){"application", "center", "lru", "mru", "title"}, 5) + 1 ];
        break;
    case key_task_align:
        taskbar_alignment = 
            !strcmp(value, "center") ? ALIGN_CENTER
        :   !strcmp(value, "right" ) ? ALIGN_RIGHT
        : ALIGN_LEFT;
        break;

    /* Task */
    case key_task_text:
        panel_config.g_task.has_text = ATOB(value);
        break;
    case key_task_icon:
        panel_config.g_task.has_icon = ATOB(value);
        break;
    case key_task_centered:
        panel_config.g_task.centered = ATOB(value);
        break;
    case key_task_width:
        // old parameter : just for backward compatibility
        panel_config.g_task.maximum_width = atoi(value);
        panel_config.g_task.maximum_height = 30;
        break;
    case key_task_maximum_size:
        extract_values(value, values, 3);
        panel_config.g_task.maximum_width = atoi(values[0]);
        panel_config.g_task.maximum_height = values[1] ? atoi(values[1]) : panel_config.g_task.maximum_width;
        break;
    case key_task_padding:
        VALUES_TO_AREA_PADDING(panel_config.g_task.area, 0);
        break;
    case key_task_font:
        panel_config.g_task.font_desc = pango_font_description_from_string(value);
        panel_config.g_task.has_font = TRUE;
        break;
    
    // "tooltip" is deprecated but here for backwards compatibility
    case key_task_tooltip:
    case key_tooltip:
        panel_config.g_task.tooltip_enabled = ATOB(value);
        break;
    case key_task_thumbnail:
        panel_config.g_task.thumbnail_enabled = ATOB(value);
        break;
    case key_task_thumbnail_size:
        panel_config.g_task.thumbnail_width = MAX(8, atoi(value));
        break;

    /* Systray */
    case key_systray_padding:
        if (!new_config_file && !systray_enabled) {
            systray_enabled = TRUE;
            STR_APPEND_CH(panel_items_order, "S");
        }
        VALUES_TO_AREA_PADDING(systray.area, 0);
        break;
    case key_systray_background_id:
        VALUE_TO_BACKGROUND (systray.area.bg);
        break;
    case key_systray_sort:
        systray.sort =
            !strcmp(value, "descending") ? SYSTRAY_SORT_DESCENDING
        :   !strcmp(value, "ascending" ) ? SYSTRAY_SORT_ASCENDING
        :   !strcmp(value, "left2right") ? SYSTRAY_SORT_LEFT2RIGHT
        :   !strcmp(value, "right2left") ? SYSTRAY_SORT_RIGHT2LEFT
        : systray.sort;
        break;
    case key_systray_icon_size:
        systray_max_icon_size = atoi(value);
        break;
    case key_systray_icon_asb:
        extract_values(value, values, 3);
        systray.alpha = atoi(values[0]);
        systray.saturation = atoi(values[1]);
        systray.brightness = atoi(values[2]);
        break;
    case key_systray_monitor:
        systray_monitor = MAX(0, config_get_monitor(value));
        break;
    case key_systray_name_filter:
        if (systray_hide_name_filter) {
            fprintf(stderr, "tint2: Error: duplicate option 'systray_name_filter'. Please use it only once. See "
                            "https://gitlab.com/o9000/tint2/issues/652\n");
            free(systray_hide_name_filter);
        }
        systray_hide_name_filter = strdup(value);
        break;

    /* Launcher */
    case key_launcher_padding:
        VALUES_TO_AREA_PADDING(panel_config.launcher.area, 0);
        break;
    case key_launcher_background_id:
        VALUE_TO_BACKGROUND (panel_config.launcher.area.bg);
        break;
    case key_launcher_icon_background_id:
        VALUE_TO_BACKGROUND (launcher_icon_bg);
        break;
    case key_launcher_icon_size:
        launcher_max_icon_size = atoi(value);
        break;
    case key_launcher_item_app:
        panel_config.launcher.list_apps = g_slist_append(panel_config.launcher.list_apps, expand_tilde(value));
        break;
    case key_launcher_apps_dir: {
        char *path = expand_tilde(value);
        load_launcher_app_dir(path);
        free(path);
        break;
    }
    case key_launcher_icon_theme:
        // if XSETTINGS manager running, tint2 use it.
        if (icon_theme_name_config)
            free(icon_theme_name_config);
        icon_theme_name_config = strdup(value);
        break;
    case key_launcher_icon_theme_override:
        launcher_icon_theme_override = ATOB(value);
        break;
    case key_launcher_icon_asb:
        VALUES_TO_ASB (launcher, 0);
        break;
    case key_launcher_tooltip:
        launcher_tooltip_enabled = ATOB(value);
        break;
    case key_startup_notifications:
        startup_notifications = ATOB(value);
        break;

    /* Tooltip */
    case key_tooltip_show_timeout:
        g_tooltip.show_timeout_msec = 1000 * atof(value);
        break;
    case key_tooltip_hide_timeout:
        g_tooltip.hide_timeout_msec = 1000 * atof(value);
        break;
    case key_tooltip_padding:
        extract_values(value, values, 3);
        if (values[0])
            g_tooltip.spacing = atoi(values[0]);
        if (values[1])
            g_tooltip.paddingy = atoi(values[1]);
        break;
    case key_tooltip_background_id:
        VALUE_TO_BACKGROUND (g_tooltip.bg);
        break;
    case key_tooltip_font_color:
        VALUES_TO_COLOR (g_tooltip.font_color, 0);
        break;
    case key_tooltip_font:
        g_tooltip.font_desc = pango_font_description_from_string(value);
        break;

    /* Mouse actions */
    case key_mouse_left:
        get_action(value, &mouse_left);
        break;
    case key_mouse_middle:
        get_action(value, &mouse_middle);
        break;
    case key_mouse_right:
        get_action(value, &mouse_right);
        break;
    case key_mouse_scroll_up:
        get_action(value, &mouse_scroll_up);
        break;
    case key_mouse_scroll_down:
        get_action(value, &mouse_scroll_down);
        break;
    case key_mouse_effects:
        panel_config.mouse_effects = ATOB(value);
        break;
    case key_mouse_hover_icon_asb:
        VALUES_TO_ASB (panel_config.mouse_over, 0);
        break;
    case key_mouse_pressed_icon_asb:
        VALUES_TO_ASB (panel_config.mouse_pressed, 0);
        break;

    /* autohide options */
    case key_autohide:
        panel_autohide = ATOB(value);
        break;
    case key_autohide_show_timeout:
        panel_autohide_show_timeout = 1000 * atof(value);
        break;
    case key_autohide_hide_timeout:
        panel_autohide_hide_timeout = 1000 * atof(value);
        break;
    case key_strut_policy:
        if (strcmp(value, "follow_size") == 0)
            panel_strut_policy = STRUT_FOLLOW_SIZE;
        else if (strcmp(value, "none") == 0)
            panel_strut_policy = STRUT_NONE;
        else
            panel_strut_policy = STRUT_MINIMUM;
        break;
    case key_autohide_height:
        panel_autohide_height = atoi(value);
        if (panel_autohide_height == 0) {
            // autohide need height > 0
            panel_autohide_height = 1;
        }
        break;

    // old config option
    case key_systray:
        if (!new_config_file) {
            systray_enabled = atoi(value);
            if (systray_enabled)
                STR_APPEND_CH(panel_items_order, "S");
        }
        break;
#ifdef ENABLE_BATTERY
    case key_battery:
        if (!new_config_file) {
            battery_enabled = atoi(value);
            if (battery_enabled)
                STR_APPEND_CH(panel_items_order, "B");
        }
        break;
#endif
    case key_primary_monitor_first:
        fprintf(stderr,
                "tint2: deprecated config option \"%s\"\n"
                "       Please see the documentation regarding the alternatives.\n",
                key);
        break;
    default:
        /* again taskbar */
        if (g_regex_match_simple("task.*_font_color", key, 0, 0)) {
            gchar **split = g_regex_split_simple("_", key, 0, 0);
            int status = g_strv_length(split) == 3 ? TASK_NORMAL : get_task_status(split[1]);
            g_strfreev(split);
            if (status >= 0) {
                VALUES_TO_COLOR (panel_config.g_task.font[status], 0);
                panel_config.g_task.config_font_mask |= (1 << status);
            }
        } else if (g_regex_match_simple("task.*_icon_asb", key, 0, 0)) {
            gchar **split = g_regex_split_simple("_", key, 0, 0);
            int status = g_strv_length(split) == 3 ? TASK_NORMAL : get_task_status(split[1]);
            g_strfreev(split);
            if (status >= 0) {
                extract_values(value, values, 3);
                panel_config.g_task.alpha[status] = atoi(values[0]);
                panel_config.g_task.saturation[status] = atoi(values[1]);
                panel_config.g_task.brightness[status] = atoi(values[2]);
                panel_config.g_task.config_asb_mask |= (1 << status);
            }
        } else if (g_regex_match_simple("task.*_background_id", key, 0, 0)) {
            gchar **split = g_regex_split_simple("_", key, 0, 0);
            int status = g_strv_length(split) == 3 ? TASK_NORMAL : get_task_status(split[1]);
            g_strfreev(split);
            if (status >= 0) {
                VALUE_TO_BACKGROUND (panel_config.g_task.background[status]);
                panel_config.g_task.config_background_mask |= (1 << status);
                if (status == TASK_NORMAL)
                    panel_config.g_task.area.bg = panel_config.g_task.background[TASK_NORMAL];
                if (panel_config.g_task.background[status]->border_content_tint_weight > 0 ||
                    panel_config.g_task.background[status]->fill_content_tint_weight > 0)
                    panel_config.g_task.has_content_tint = TRUE;
            }
        } else
            fprintf(stderr, "tint2: invalid option \"%s\",\n  upgrade tint2 or correct your config file\n", key);
    }
    #undef VALUES_TO_COLOR
    #undef VALUES_TO_AREA_PADDING
    #undef VALUES_TO_ASB
    #undef VALUE_TO_BACKGROUND
    #undef VALUE_TO_GRADIENT
    #undef VALUE_TO_COMMAND
}

gboolean config_read_file(const char *path)
{
    fprintf(stderr, "tint2: Loading config file: %s\n", path);

    FILE *fp = fopen(path, "r");
    if (!fp)
        return FALSE;

    char *line = NULL;
    size_t line_size = 0;
    while (getline(&line, &line_size, fp) >= 0) {
        char *key, *value;
        if (parse_line(line, &key, &value) & PARSED_KEY)
            add_entry(key, value);
    }
    free(line);
    fclose(fp);

    if (!read_panel_position) {
        panel_horizontal = TRUE;
        panel_position = BOTTOM;
    }

    // append Taskbar item
    if (!new_config_file) {
        taskbar_enabled = TRUE;
        STR_PREPEND_CH(panel_items_order, "T");
    }

    if (backgrounds->len > 0) {
        Background *bg = &g_array_index(backgrounds, Background, backgrounds->len - 1);
        if (!read_bg_color_hover)       bg->fill_color_hover     = bg->fill_color;
        if (!read_border_color_hover)   bg->border_color_hover   = bg->border.color;
        if (!read_bg_color_press)       bg->fill_color_pressed   = bg->fill_color_hover;
        if (!read_border_color_press)   bg->border_color_pressed = bg->border_color_hover;
    }

    return TRUE;
}

gboolean config_read_default_path()
{
#define PATH_TEMPLATE   ("%s" G_DIR_SEPARATOR_S "tint2" G_DIR_SEPARATOR_S "tint2rc")
#define _path_size(dir)       (snprintf( NULL, 0, PATH_TEMPLATE, (dir) ) + 1)
#define _path_build(buf, dir) (sprintf( (buf), PATH_TEMPLATE, (dir) ))
#define _mkdir_if_missing(path) do{ if (!g_file_test((path), G_FILE_TEST_IS_DIR)) \
                                        g_mkdir_with_parents((path), 0700); } while(0)
    const gchar *const *sysdir;
    gchar *userpath, *syspath;
    gchar *userdir = (gchar *)(fetch_user_config_dir(), user_config_dir);

    // Check tint2rc in user directory as mandated by XDG specification.
    // Use shared path for both user config file and its dir
    gchar *dir_end;
    {
        int size = _path_size(userdir);
        _path_build((userpath = malloc(size)), userdir);
        if (g_file_test(userpath, G_FILE_TEST_EXISTS))
            goto done;
        dir_end = userpath + size - strlen_const("tint2rc");
    }
    // copy tint2rc from system directory to user directory

    fprintf(stderr, "tint2: could not find a config file! Creating a default one.\n");
    // According to the XDG Base Directory Specification
    // (https://specifications.freedesktop.org/basedir-spec/basedir-spec-0.6.html)
    // if the user's config directory does not exist, we should create it with permissions set to 0700.
    _mkdir_if_missing (userdir);
    *dir_end = '\0';
    _mkdir_if_missing (userpath);
    *dir_end = G_DIR_SEPARATOR_S[0];

    syspath = NULL;
    sysdir = g_get_system_config_dirs();
    for (int size, oldsize = 0; *sysdir; (syspath[0] = '\0'), sysdir++)
    {
        size = _path_size(*sysdir);
        if (size > oldsize)
            syspath = realloc (syspath, (oldsize = size));
        _path_build(syspath, *sysdir);
        if (g_file_test(syspath, G_FILE_TEST_EXISTS))
            break;
    }
    if (syspath) {
        if (syspath[0])
            // copy system config to user directory
            copy_file(syspath, userpath);
        else {
            // or generate default one
            FILE *f = fopen(userpath, "w");
            if (f) {
                fwrite(themes_tint2rc, 1, themes_tint2rc_len, f);
                fclose(f);
            }
        }
        free( syspath);
    }
done:;
    gboolean result = config_read_file(userpath);
    config_path = userpath;
    return result;
#undef _mkdir_if_missing
#undef _path_build
#undef _path_size
#undef PATH_TEMPLATE
}

gboolean config_read()
{
    return config_path  ? config_read_file (config_path)
                        : config_read_default_path ();
}

#endif
