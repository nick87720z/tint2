/**************************************************************************
*
* Tint2 : clock
*
* Copyright (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr) from Omega distribution
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

#include <string.h>
#include <stdio.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <pango/pangocairo.h>
#include <stdlib.h>

#include "window.h"
#include "tooltip.h"
#include "server.h"
#include "panel.h"
#include "clock.h"
#include "timer.h"
#include "common.h"

char *time1_format;
char *time1_timezone;
char *time2_format;
char *time2_timezone;
char *time_tooltip_format;
char *time_tooltip_timezone;
char *clock_lclick_command;
char *clock_mclick_command;
char *clock_rclick_command;
char *clock_uwheel_command;
char *clock_dwheel_command;
int clock_lclick_command_sink;
int clock_mclick_command_sink;
int clock_rclick_command_sink;
int clock_uwheel_command_sink;
int clock_dwheel_command_sink;
struct timeval time_clock;
gboolean time1_has_font;
PangoFontDescription *time1_font_desc;
gboolean time2_has_font;
PangoFontDescription *time2_font_desc;
static char buf_time[256];
static char buf_date[256];
static char buf_tooltip[512];
gboolean clock_enabled;
static Timer clock_timer;

void clock_init_fonts();
char *clock_get_tooltip(void *obj);
int clock_get_desired_size(void *obj);
void clock_dump_geometry(void *obj, int indent);

void default_clock()
{
    clock_enabled = TRUE;
    time1_format = NULL;
    time1_timezone = NULL;
    time2_format = NULL;
    time2_timezone = NULL;
    INIT_TIMER(clock_timer);
    time_tooltip_format = NULL;
    time_tooltip_timezone = NULL;
    clock_lclick_command = NULL;
    clock_mclick_command = NULL;
    clock_rclick_command = NULL;
    clock_uwheel_command = NULL;
    clock_dwheel_command = NULL;
    time1_has_font = FALSE;
    time1_font_desc = NULL;
    time2_has_font = FALSE;
    time2_font_desc = NULL;
    buf_time[0]    = '\0', BUF_0TERM (buf_time);
    buf_date[0]    = '\0', BUF_0TERM (buf_date);
    buf_tooltip[0] = '\0', BUF_0TERM (buf_tooltip);
}

void cleanup_clock()
{
    pango_font_description_free(time1_font_desc);
    time1_font_desc = NULL;
    pango_font_description_free(time2_font_desc);
    time2_font_desc = NULL;
    free_and_null( time1_format);
    free_and_null( time2_format);
    free_and_null( time_tooltip_format);
    free_and_null( time1_timezone);
    free_and_null( time2_timezone);
    free_and_null( time_tooltip_timezone);
    free_and_null( clock_lclick_command);
    free_and_null( clock_mclick_command);
    free_and_null( clock_rclick_command);
    free_and_null( clock_uwheel_command);
    free_and_null( clock_dwheel_command);
    destroy_timer(&clock_timer);
}

struct tm *clock_gettime_for_tz(const char *timezone)
{
    if (timezone) {
        const char *old_tz = getenv("TZ");
        setenv("TZ", timezone, 1);
        struct tm *result = localtime(&time_clock.tv_sec);
        if (old_tz)
            setenv("TZ", old_tz, 1);
        else
            unsetenv("TZ");
        return result;
    } else
        return localtime(&time_clock.tv_sec);
}

void update_clock_text(char *dst, size_t size, const char *format,
                       const char *timezone, bool *changed)
{
    if (!dst || !format)
        return;

    char tmp[256] = "";
    strncpy(tmp, dst, strlen_const(tmp));
    strftime(dst, size, format, clock_gettime_for_tz(timezone));
    *changed = *changed || strcmp(dst, tmp) != 0;
}

void update_clocks()
{
    bool changed = false;
    update_clock_text(buf_time, sizeof(buf_time), time1_format, time1_timezone, &changed);
    update_clock_text(buf_date, sizeof(buf_date), time2_format, time2_timezone, &changed);
    if (changed) {
        for (int i = 0; i < num_panels; i++)
        {
            panels[i].clock.area.resize_needed = TRUE;
            tooltip_update_for_area (&panels[i].clock.area);
        }
        schedule_panel_redraw();
    }
}

int ms_until_second_change(struct timeval* tm)
{
    long us_until_change = 1000000 - tm->tv_usec;
    // compute ms, rounding up so we don't ask to wait too short
    int ms = (us_until_change+999)/1000;
    return ms;
}

void update_clocks_sec(void *arg)
{
    gettimeofday(&time_clock, 0);
    update_clocks();
    change_timer(&clock_timer, true, ms_until_second_change(&time_clock), 0, update_clocks_sec, 0);
}

void init_clock()
{
}

void init_clock_panel(void *p)
{
    Panel *panel = p;
    Clock *clock = &panel->clock;

    if (!clock->area.bg)
        clock->area.bg = &g_array_index(backgrounds, Background, 0);
    clock_init_fonts();
    clock->area.parent = p;
    clock->area.panel = p;
    snprintf(clock->area.name, strlen_const(clock->area.name), "Clock");
    clock->area._is_under_mouse = full_width_area_is_under_mouse;
    clock->area.has_mouse_press_effect = clock->area.has_mouse_over_effect =
        panel_config.mouse_effects && (clock_lclick_command || clock_mclick_command || clock_rclick_command ||
                                       clock_uwheel_command || clock_dwheel_command);
    clock->area._draw_foreground = draw_clock;
    clock->area.size_mode = LAYOUT_FIXED;
    clock->area._resize = resize_clock;
    clock->area._get_desired_size = clock_get_desired_size;
    clock->area._dump_geometry = clock_dump_geometry;
    // check consistency
    if (!time1_format)
        return;

    clock->area.resize_needed = TRUE;
    clock->area.on_screen = TRUE;
    area_gradients_create(&clock->area);

    if (time_tooltip_format) {
        clock->area._get_tooltip_text = clock_get_tooltip;
        strftime(buf_tooltip, sizeof(buf_tooltip), time_tooltip_format, clock_gettime_for_tz(time_tooltip_timezone));
    }

    if (!clock_timer.enabled_)
        update_clocks_sec(NULL);
}

void clock_init_fonts()
{
    if (!time1_font_desc)
    {
        time1_font_desc = pango_font_description_from_string(get_default_font());
        pango_font_description_set_weight (time1_font_desc, PANGO_WEIGHT_BOLD);
        pango_font_description_set_size   (time1_font_desc,
                                           pango_font_description_get_size (time1_font_desc));
    }
    if (!time2_font_desc)
    {
        time2_font_desc = pango_font_description_from_string(get_default_font());
        pango_font_description_set_size(time2_font_desc,
                                        pango_font_description_get_size (time2_font_desc) - PANGO_SCALE);
    }
}

void clock_default_font_changed()
{
    if (!clock_enabled)
        return;
    if (time1_has_font && time2_has_font)
        return;
    if (!time1_has_font) {
        pango_font_description_free(time1_font_desc);
        time1_font_desc = NULL;
    }
    if (!time2_has_font) {
        pango_font_description_free(time2_font_desc);
        time2_font_desc = NULL;
    }
    clock_init_fonts();
    for (int i = 0; i < num_panels; i++) {
        panels[i].clock.area.resize_needed = TRUE;
        schedule_redraw(&panels[i].clock.area);
    }
    schedule_panel_redraw();
}

void clock_get_text_geometry(Clock *clock,
                                 int *time_height,
                                 int *time_width,
                                 int *date_height,
                                 int *date_width)
{
    area_get_text_geometry(&clock->area,
                               buf_time,
                               time2_format ? buf_date : NULL,
                               time1_font_desc,
                               time2_font_desc,
                               time_height,
                               time_width,
                               date_height,
                               date_width);
}

int clock_get_desired_size(void *obj)
{
    Clock *clock = obj;
    return text_area_get_desired_size(&clock->area,
                                          buf_time,
                                          time2_format ? buf_date : NULL,
                                          time1_font_desc,
                                          time2_font_desc);
}

gboolean resize_clock(void *obj)
{
    Clock *clock = obj;
    return resize_text_area(&clock->area,
                            buf_time,
                            time2_format ? buf_date : NULL,
                            time1_font_desc,
                            time2_font_desc,
                            &clock->time1_posy,
                            &clock->time2_posy);
}

void draw_clock(void *obj, cairo_t *c)
{
    Clock *clock = obj;
    Panel *panel = clock->area.panel;
    draw_text_area(&clock->area,
                   c,
                   buf_time,
                   time2_format ? buf_date : NULL,
                   time1_font_desc,
                   time2_font_desc,
                   clock->time1_posy,
                   clock->time2_posy,
                   &clock->font,
                   panel->scale);
}

void clock_dump_geometry(void *obj, int indent)
{
    Clock *clock = obj;
    fprintf(stderr, "tint2: %*sText 1: y = %d, text = %s\n", indent, "", clock->time1_posy, buf_time);
    if (time2_format)
        fprintf(stderr, "tint2: %*sText 2: y = %d, text = %s\n", indent, "", clock->time2_posy, buf_date);
}

char *clock_get_tooltip(void *obj)
{
    strftime(buf_tooltip, sizeof(buf_tooltip), time_tooltip_format, clock_gettime_for_tz(time_tooltip_timezone));
    return strdup(buf_tooltip);
}

void clock_action(void *obj, int button, int x, int y, Time time)
{
    char *command = NULL;
    int cmd_sink = -1;
    switch (button) {
        BUTTON_CASE (1, clock_lclick_command);
        BUTTON_CASE (2, clock_mclick_command);
        BUTTON_CASE (3, clock_rclick_command);
        BUTTON_CASE (4, clock_uwheel_command);
        BUTTON_CASE (5, clock_dwheel_command);
    }
    tint_exec(command, NULL, NULL, time, obj, x, y, FALSE, TRUE);
}
