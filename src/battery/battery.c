/**************************************************************************
*
* Tint2 : Generic battery
*
* Copyright (C) 2009-2015 Sebastian Reichel <sre@ring0.de>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License version 2
* or any later version as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <pango/pangocairo.h>

#include "window.h"
#include "tooltip.h"
#include "server.h"
#include "panel.h"
#include "battery.h"
#include "timer.h"
#include "common.h"

gboolean bat1_has_font;
PangoFontDescription *bat1_font_desc;
gboolean bat2_has_font;
PangoFontDescription *bat2_font_desc;
char *bat1_format;
char *bat2_format;
struct BatteryState battery_state;
gboolean battery_enabled;
gboolean battery_tooltip_enabled;
int percentage_hide;
static Timer battery_timer;
static Timer battery_blink_timer;

#define BATTERY_BUF_SIZE 256
static char buf_bat_line1[BATTERY_BUF_SIZE];
static char buf_bat_line2[BATTERY_BUF_SIZE];

int8_t battery_low_status;
gboolean battery_low_cmd_sent;
gboolean battery_full_cmd_sent;
char *ac_connected_cmd;
char *ac_disconnected_cmd;
char *battery_low_cmd;
char *battery_full_cmd;
char *battery_lclick_command;
char *battery_mclick_command;
char *battery_rclick_command;
char *battery_uwheel_command;
char *battery_dwheel_command;
int battery_lclick_command_sink;
int battery_mclick_command_sink;
int battery_rclick_command_sink;
int battery_uwheel_command_sink;
int battery_dwheel_command_sink;
gboolean battery_found;
gboolean battery_warn;
gboolean battery_warn_red;

char *battery_sys_prefix = (char *)"";

void battery_init_fonts();
char *battery_get_tooltip(void *obj);
int battery_get_desired_size(void *obj);
void battery_dump_geometry(void *obj, int indent);

void default_battery()
{
    battery_enabled = FALSE;
    battery_tooltip_enabled = TRUE;
    battery_found = FALSE;
    percentage_hide = 101;
    battery_low_cmd_sent = FALSE;
    battery_full_cmd_sent = FALSE;
    INIT_TIMER(battery_timer);
    INIT_TIMER(battery_blink_timer);
    battery_warn = FALSE;
    battery_warn_red = FALSE;
    bat1_has_font = FALSE;
    bat1_font_desc = NULL;
    bat1_format = NULL;
    bat2_has_font = FALSE;
    bat2_font_desc = NULL;
    bat2_format = NULL;
    ac_connected_cmd = NULL;
    ac_disconnected_cmd = NULL;
    battery_low_cmd = NULL;
    battery_full_cmd = NULL;
    battery_lclick_command = NULL;
    battery_mclick_command = NULL;
    battery_rclick_command = NULL;
    battery_uwheel_command = NULL;
    battery_dwheel_command = NULL;
    battery_state.percentage = 0;
    battery_state.time.hours = 0;
    battery_state.time.minutes = 0;
    battery_state.time.seconds = 0;
    battery_state.state = BATTERY_UNKNOWN;
    BUF_0TERM (buf_bat_line1);
    BUF_0TERM (buf_bat_line2);
}

void cleanup_battery()
{
    pango_font_description_free(bat1_font_desc);
    bat1_font_desc = NULL;
    pango_font_description_free(bat2_font_desc);
    bat2_font_desc = NULL;
    free_and_null(battery_low_cmd);
    free_and_null(battery_full_cmd);
    free_and_null(bat1_format);
    free_and_null(bat2_format);
    free_and_null(battery_lclick_command);
    free_and_null(battery_mclick_command);
    free_and_null(battery_rclick_command);
    free_and_null(battery_uwheel_command);
    free_and_null(battery_dwheel_command);
    free_and_null(ac_connected_cmd);
    free_and_null(ac_disconnected_cmd);
    destroy_timer(&battery_timer);
    destroy_timer(&battery_blink_timer);
    battery_found = FALSE;

    battery_os_free();
}

char *strnappend(char *dest, const char *addendum, size_t limit)
// Appends addendum to dest, and does not allow dest to grow over limit (including NULL terminator).
// Result is undefined if either dest or addendum are not null-terminated
// WARNING: limit is used completely, it's programmer responsibility to recerve place for terminating null
{
    char *pos = memchr(dest, '\0', limit);
    return pos  ? (snprintf(pos, limit-(pos-dest), "%s", addendum), dest)
                : NULL;
}

void battery_update_text(char *dest, char *format)
{
    if (!battery_enabled || !dest || !format)
        return;
    // We want to loop over the format specifier, replacing any known symbols with our battery data.
    // First, erase anything already stored in the buffer.
    // This ensures the string will always be null-terminated.
    memset(dest, 0, BATTERY_BUF_SIZE);

    for (size_t len = strlen(format), o = 0; o < len; o++) {
        char buf[BATTERY_BUF_SIZE];
        memset(buf, 0, BATTERY_BUF_SIZE);
        char *c = &format[o];
        // Format specification:
        // %s :	State (charging, discharging, full, unknown)
        // %m :	Minutes left (estimated).
        // %h : Hours left (estimated).
        // %t :	Time left. This is equivalent to the old behaviour; i.e. "(plugged in)" or "hrs:mins" otherwise.
        // %p :	Percentage left. Includes the % sign.
        // %P :	Percentage left without the % sign.
        if (*c == '%') {
            c++;
            o++; // Skip the format control character.
            switch (*c) {
            case 's':
                // Append the appropriate status message to the string.
                strnappend(dest,
                        (battery_state.state == BATTERY_CHARGING)
                            ? "Charging"
                            : (battery_state.state == BATTERY_DISCHARGING)
                                  ? "Discharging"
                                  : (battery_state.state == BATTERY_FULL)
                                        ? "Full"
                                        : "Unknown",
                        BATTERY_BUF_SIZE-1);
                break;
            case 'm':
                snprintf(buf, strlen_const(buf), "%02d", battery_state.time.minutes);
                strnappend(dest, buf, BATTERY_BUF_SIZE-1);
                break;
            case 'h':
                snprintf(buf, strlen_const(buf), "%02d", battery_state.time.hours);
                strnappend(dest, buf, BATTERY_BUF_SIZE-1);
                break;
            case 'p':
                snprintf(buf, strlen_const(buf), "%d%%", battery_state.percentage);
                strnappend(dest, buf, BATTERY_BUF_SIZE-1);
                break;
            case 'P':
                snprintf(buf, strlen_const(buf), "%d", battery_state.percentage);
                strnappend(dest, buf, BATTERY_BUF_SIZE-1);
                break;
            case 't':
                if (battery_state.state == BATTERY_FULL) {
                    snprintf(buf, strlen_const(buf), "Full");
                    strnappend(dest, buf, BATTERY_BUF_SIZE-1);
                } else if (battery_state.time.hours > 0 && battery_state.time.minutes > 0) {
                    snprintf(buf, strlen_const(buf), "%02d:%02d", battery_state.time.hours, battery_state.time.minutes);
                    strnappend(dest, buf, BATTERY_BUF_SIZE-1);
                }
                break;

            case '%':
            case '\0':
                strnappend(dest, "%", BATTERY_BUF_SIZE-1);
                break;
            default:
                fprintf(stderr, "tint2: Battery: unrecognised format specifier '%%%c'.\n", *c);
                buf[0] = *c;
                strnappend(dest, buf, BATTERY_BUF_SIZE-1);
            }
        } else {
            buf[0] = *c;
            strnappend(dest, buf, BATTERY_BUF_SIZE-1);
        }
    }
}

void init_battery()
{
    if (!battery_enabled)
        return;

    battery_found = battery_os_init();

    if (!battery_timer.enabled_)
        change_timer(&battery_timer, true, 30000, 30000, update_battery_tick, 0);

    update_battery();
}

void reinit_battery()
{
    battery_os_free();
    battery_found = battery_os_init();
    update_battery();
}

void init_battery_panel(void *p)
{
    Panel *panel = p;
    Battery *battery = &panel->battery;

    if (!battery_enabled)
        return;

    battery_init_fonts();

    if (!battery->area.bg)
        battery->area.bg = &g_array_index(backgrounds, Background, 0);

    battery->area.parent = p;
    battery->area.panel = p;
    snprintf(battery->area.name, strlen_const(battery->area.name), "Battery");
    battery->area._draw_foreground = draw_battery;
    battery->area.size_mode = LAYOUT_FIXED;
    battery->area._resize = resize_battery;
    battery->area._get_desired_size = battery_get_desired_size;
    battery->area._is_under_mouse = full_width_area_is_under_mouse;
    battery->area.on_screen = TRUE;
    battery->area.resize_needed = TRUE;
    battery->area.has_mouse_over_effect =
        panel_config.mouse_effects && (battery_lclick_command || battery_mclick_command || battery_rclick_command ||
                                       battery_uwheel_command || battery_dwheel_command);
    battery->area.has_mouse_press_effect = battery->area.has_mouse_over_effect;
    if (battery_tooltip_enabled)
        battery->area._get_tooltip_text = battery_get_tooltip;
    area_gradients_create(&battery->area);

    if (!bat1_format && !bat2_format) {
        strdup_static(bat1_format, "%p");
        strdup_static(bat2_format, "%t");
    }
    update_battery_tick(NULL);
}

void battery_init_fonts()
{
    if (!bat1_font_desc) {
        bat1_font_desc = pango_font_description_from_string(get_default_font());
        pango_font_description_set_size(bat1_font_desc, pango_font_description_get_size(bat1_font_desc) - PANGO_SCALE);
    }
    if (!bat2_font_desc) {
        bat2_font_desc = pango_font_description_from_string(get_default_font());
        pango_font_description_set_size(bat2_font_desc, pango_font_description_get_size(bat2_font_desc) - PANGO_SCALE);
    }
}

void battery_default_font_changed()
{
    if (!battery_enabled)
        return;
    if (bat1_has_font && bat2_has_font)
        return;
    if (!bat1_has_font) {
        pango_font_description_free(bat1_font_desc);
        bat1_font_desc = NULL;
    }
    if (!bat2_has_font) {
        pango_font_description_free(bat2_font_desc);
        bat2_font_desc = NULL;
    }
    battery_init_fonts();
    for (int i = 0; i < num_panels; i++) {
        panels[i].battery.area.resize_needed = TRUE;
        schedule_redraw(&panels[i].battery.area);
    }
    schedule_panel_redraw();
}

void blink_battery(void *arg)
{
    if (!battery_enabled)
        return;
    battery_warn_red = battery_warn ? !battery_warn_red : FALSE;
    for (int i = 0; i < num_panels; i++) {
        if (panels[i].battery.area.on_screen) {
            schedule_redraw(&panels[i].battery.area);
        }
    }
}

void update_battery_tick(void *arg)
{
    if (!battery_enabled)
        return;

    gboolean old_found = battery_found;
    int old_percentage = battery_state.percentage;
    gboolean old_ac_connected = battery_state.ac_connected;
    int16_t old_hours = battery_state.time.hours;
    int8_t old_minutes = battery_state.time.minutes;
    gboolean old_warn = battery_warn;

    if (!battery_found) {
        init_battery();
        old_ac_connected = battery_state.ac_connected;
    }
    if (update_battery() != 0) {
        // Try to reconfigure on failed update
        init_battery();
    }

    if (old_ac_connected != battery_state.ac_connected) {
        if (battery_state.ac_connected)
            tint_exec_no_sn(ac_connected_cmd);
        else
            tint_exec_no_sn(ac_disconnected_cmd);
    }

    if (battery_state.percentage < battery_low_status && battery_state.state == BATTERY_DISCHARGING &&
        !battery_low_cmd_sent) {
        tint_exec_no_sn(battery_low_cmd);
        battery_low_cmd_sent = TRUE;
    }
    if (battery_state.percentage > battery_low_status && battery_state.state == BATTERY_CHARGING &&
        battery_low_cmd_sent) {
        battery_low_cmd_sent = FALSE;
    }

    if ((battery_state.percentage >= 100 || battery_state.state == BATTERY_FULL) &&
        !battery_full_cmd_sent) {
        tint_exec_no_sn(battery_full_cmd);
        battery_full_cmd_sent = TRUE;
    }
    if (battery_state.percentage < 100 && battery_state.state != BATTERY_FULL &&
        battery_full_cmd_sent) {
        battery_full_cmd_sent = FALSE;
    }

    if (!battery_blink_timer.enabled_) {
        if ((battery_state.percentage < battery_low_status &&
            battery_state.state == BATTERY_DISCHARGING) || debug_blink) {
            change_timer(&battery_blink_timer, true, 10, 1000, blink_battery, 0);
            battery_warn = TRUE;
        }
    } else {
        if (battery_state.percentage > battery_low_status ||
            battery_state.state != BATTERY_DISCHARGING) {
            stop_timer(&battery_blink_timer);
            battery_warn = FALSE;
        }
    }

    for (int i = 0; i < num_panels; i++) {
        // Show/hide if needed
        if (!battery_found) {
            hide(&panels[i].battery.area);
        } else {
            if (battery_state.percentage >= percentage_hide)
                hide(&panels[i].battery.area);
            else
                show(&panels[i].battery.area);
        }
        // Redraw if needed
        if (panels[i].battery.area.on_screen) {
            if (old_found != battery_found || old_percentage != battery_state.percentage ||
                old_hours != battery_state.time.hours || old_minutes != battery_state.time.minutes ||
                old_warn != battery_warn) {
                panels[i].battery.area.resize_needed = TRUE;
                if (!battery_warn)
                    panels[i].battery.area.bg = panel_config.battery.area.bg;
                schedule_panel_redraw();
            }
            tooltip_update_for_area (&panels[i].battery.area);
        }
    }
}

int update_battery()
{
    // Reset
    battery_state.state = BATTERY_UNKNOWN;
    battery_state.percentage = 0;
    battery_state.ac_connected = FALSE;
    battery_state_set_time(&battery_state, 0);

    int err = battery_os_update(&battery_state);

    // Clamp percentage to 100 in case battery is misreporting that its current charge is more than its max
    if (battery_state.percentage > 100) {
        battery_state.percentage = 100;
    }

    battery_update_text(buf_bat_line1, bat1_format);
    if (bat2_format != NULL) {
        battery_update_text(buf_bat_line2, bat2_format);
    }

    return err;
}

int battery_get_desired_size(void *obj)
{
    Battery *battery = obj;
    return text_area_get_desired_size(&battery->area, buf_bat_line1, buf_bat_line2, bat1_font_desc, bat2_font_desc);
}

gboolean resize_battery(void *obj)
{
    Battery *battery = obj;
    return resize_text_area(&battery->area,
                            buf_bat_line1,
                            buf_bat_line2,
                            bat1_font_desc,
                            bat2_font_desc,
                            &battery->bat1_posy,
                            &battery->bat2_posy);
}

void draw_battery(void *obj, cairo_t *c)
{
    Battery *battery = obj;
    Panel *panel = battery->area.panel;
    draw_text_area(&battery->area,
                   c,
                   buf_bat_line1,
                   buf_bat_line2,
                   bat1_font_desc,
                   bat2_font_desc,
                   battery->bat1_posy,
                   battery->bat2_posy,
                   &battery->font_color,
                   panel->scale);
    if (battery_warn && battery_warn_red) {
        cairo_set_source_rgba(c, 1, 0, 0, 1);
        cairo_set_line_width(c, 0);
        cairo_rectangle(c, 0, 0, battery->area.width, battery->area.height);
        cairo_fill(c);
    }
}

void battery_dump_geometry(void *obj, int indent)
{
    Battery *battery = obj;
    fprintf(stderr, "tint2: %*sText 1: y = %d, text = %s\n", indent, "", battery->bat1_posy, buf_bat_line1);
    fprintf(stderr, "tint2: %*sText 2: y = %d, text = %s\n", indent, "", battery->bat2_posy, buf_bat_line2);
}

char *battery_get_tooltip(void *obj)
{
    return battery_os_tooltip();
}

void battery_action(void *obj, int button, int x, int y, Time time)
{
    char *command = NULL;
    int cmd_sink = -1;
    switch (button) {
        BUTTON_CASE (1, battery_lclick_command);
        BUTTON_CASE (2, battery_mclick_command);
        BUTTON_CASE (3, battery_rclick_command);
        BUTTON_CASE (4, battery_uwheel_command);
        BUTTON_CASE (5, battery_dwheel_command);
    }
    tint_exec(command, NULL, NULL, time, obj, x, y, FALSE, TRUE);
}
