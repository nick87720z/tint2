/**************************************************************************
*
* Copyright (C) 2009 Andreas.Fink (Andreas.Fink85@gmail.com)
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <cairo.h>
#include <cairo-xlib.h>

#include "server.h"
#include "tooltip.h"
#include "panel.h"
#include "timer.h"

static int x, y, width, height;
static gboolean just_shown;

// the next functions are helper functions for tooltip handling
void start_show_timer();
void start_hide_timer();
void stop_tooltip_timer();

void tooltip_init_fonts();

Tooltip g_tooltip;

void default_tooltip()
{
    // give the tooltip some reasonable default values
    memset(&g_tooltip, 0, sizeof(Tooltip));

    INIT_TIMER(g_tooltip.visibility_timer);
    INIT_TIMER(g_tooltip.update_timer);

    g_tooltip.font_color.rgb[0] = 1;
    g_tooltip.font_color.rgb[1] = 1;
    g_tooltip.font_color.rgb[2] = 1;
    g_tooltip.font_color.alpha  = 1;
    just_shown = FALSE;
}

void cleanup_tooltip()
{
    stop_tooltip_timer();
    destroy_timer(&g_tooltip.visibility_timer);
    destroy_timer(&g_tooltip.update_timer);
    tooltip_hide(NULL);
    tooltip_set_area(NULL);
    if (g_tooltip.window)
        XDestroyWindow(server.display, g_tooltip.window);
    g_tooltip.window = 0;
    pango_font_description_free(g_tooltip.font_desc);
    g_tooltip.font_desc = NULL;
}

void init_tooltip()
{
    if (!g_tooltip.bg)
        g_tooltip.bg = &g_array_index(backgrounds, Background, 0);
    tooltip_init_fonts();

    XSetWindowAttributes attr;
    attr.override_redirect = True;
    attr.event_mask = StructureNotifyMask;
    attr.colormap = server.colormap;
    attr.background_pixel = 0;
    attr.border_pixel = 0;
    unsigned long mask = CWEventMask | CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect;
    if (g_tooltip.window)
        XDestroyWindow(server.display, g_tooltip.window);
    g_tooltip.window = XCreateWindow(server.display, server.root_win,
                                     0, 0, 100, 20, 0,
                                     server.depth,
                                     InputOutput,
                                     server.visual,
                                     mask,
                                     &attr);
}

void tooltip_init_fonts()
{
    if (!g_tooltip.font_desc)
        g_tooltip.font_desc = pango_font_description_from_string(get_default_font());
}

void tooltip_default_font_changed()
{
    if (g_tooltip.has_font)
        return;
    if (!g_tooltip.has_font) {
        pango_font_description_free(g_tooltip.font_desc);
        g_tooltip.font_desc = NULL;
    }
    tooltip_init_fonts();
    tooltip_update();
}

void tooltip_trigger_show(Area *area, Panel *p, XEvent *e)
{
    // Position the tooltip in the center of the area
    x = area->posx + MIN(area->width / 3, 22) + e->xmotion.x_root - e->xmotion.x;
    y = area->posy + area->height / 2 + e->xmotion.y_root - e->xmotion.y;
    just_shown = TRUE;
    g_tooltip.panel = p;
    if (!g_tooltip.mapped)
        start_show_timer();
    else if (g_tooltip.area != area)
    {
        tooltip_set_area(area);
        tooltip_update();
        stop_tooltip_timer();
    }
}

void tooltip_show(void *arg)
{
    int mx, my;
    Window w;
    XTranslateCoordinates(server.display, server.root_win, g_tooltip.panel->main_win, x, y, &mx, &my, &w);
    Area *area = find_area_under_mouse(g_tooltip.panel, mx, my);
    if (!g_tooltip.mapped && area->_get_tooltip_text) {
        tooltip_set_area(area);
        g_tooltip.mapped = True;
        XMapWindow(server.display, g_tooltip.window);
        tooltip_update();
        XFlush(server.display);
    }
}

void tooltip_update_geometry()
{
    Panel *panel = g_tooltip.panel;
    int screen_width = server.monitors[panel->monitor].width;

    #define GET_TEXT_PIXEL_EXTENTS(s, r) {                                               \
        pango_layout_set_text(layout, (s), -1); \
        pango_layout_get_pixel_extents(layout, NULL, (r)); \
    }

    cairo_surface_t *cs = cairo_xlib_surface_create(server.display, g_tooltip.window, server.visual, width, height);
    cairo_t *c = cairo_create(cs);
    PangoContext *context = pango_cairo_create_context(c);
    pango_cairo_context_set_resolution(context, 96 * panel->scale);
    PangoLayout *layout = pango_layout_new(context);
    pango_layout_set_font_description(layout, g_tooltip.font_desc);

    PangoRectangle rect;

    GET_TEXT_PIXEL_EXTENTS("1234567890abcdef", &rect);

    int img_width, img_useful, space;
    img_useful = g_tooltip.image && (img_width = cairo_image_surface_get_width(g_tooltip.image)) > 0;
    space = left_right_bg_border_width(g_tooltip.bg) + 2 * g_tooltip.spacing * panel->scale;

    pango_layout_set_width(
        layout,
        strchr(g_tooltip.tooltip_text, '\n') ? -1 : (
            img_useful  ? space + img_width
                        : MIN( rect.width * 5, screen_width * 2 / 3)
        ) * PANGO_SCALE
    );

    pango_layout_set_wrap(layout, PANGO_WRAP_WORD);
    GET_TEXT_PIXEL_EXTENTS(g_tooltip.tooltip_text ? g_tooltip.tooltip_text : "1234567890abcdef", &rect);
    height = top_bottom_bg_border_width( g_tooltip.bg) + 2 * g_tooltip.paddingy * panel->scale + rect.height;

    if (img_useful) {
        width = space + MAX(img_width, rect.width);
        height += g_tooltip.paddingy * panel->scale + cairo_image_surface_get_height( g_tooltip.image);
    } else
        width = space + rect.width;

    if (!panel_horizontal)
        goto xlim;

    if (panel_position & BOTTOM)
        y = panel->posy - height;
    else if (panel_position & TOP)
        y = panel->posy + panel->area.height;
    else
    xlim: x = panel->posx + (panel_position & LEFT ? panel->area.width : -width);

    #undef GET_TEXT_PIXEL_EXTENTS

    g_object_unref(layout);
    g_object_unref(context);
    cairo_destroy(c);
    cairo_surface_destroy(cs);
}

void tooltip_adjust_geometry()
// adjust coordinates and size to not go offscreen
// it seems quite impossible that the height needs to be adjusted, but we do it anyway.
{
    Panel *panel = g_tooltip.panel;
    int screen_width  = server.monitors[panel->monitor].x + server.monitors[panel->monitor].width;
    int screen_height = server.monitors[panel->monitor].y + server.monitors[panel->monitor].height;
    if (x + width  <= screen_width              && 
        y + height <= screen_height             && 
        x >= server.monitors[panel->monitor].x  &&
        y >= server.monitors[panel->monitor].y) return; // no adjustment needed

    int min_x, min_y, max_width, max_height;
    if (panel_horizontal) {
        min_x = 0;
        max_width = server.monitors[panel->monitor].width;
        max_height = server.monitors[panel->monitor].height - panel->area.height;
        if (panel_position & BOTTOM)
            min_y = 0;
        else
            min_y = panel->area.height;
    } else {
        max_width = server.monitors[panel->monitor].width - panel->area.width;
        min_y = 0;
        max_height = server.monitors[panel->monitor].height;
        if (panel_position & LEFT)
            min_x = panel->area.width;
        else
            min_x = 0;
    }

    if (x + width > server.monitors[panel->monitor].x + server.monitors[panel->monitor].width)
        x = server.monitors[panel->monitor].x + server.monitors[panel->monitor].width - width;
    if (y + height > server.monitors[panel->monitor].y + server.monitors[panel->monitor].height)
        y = server.monitors[panel->monitor].y + server.monitors[panel->monitor].height - height;

    if (x < min_x)
        x = min_x;
    if (width > max_width)
        width = max_width;
    if (y < min_y)
        y = min_y;
    if (height > max_height)
        height = max_height;
}

void tooltip_update()
{
    if (!g_tooltip.tooltip_text) {
        tooltip_hide(0);
        return;
    }
    Panel *panel = g_tooltip.panel;

    tooltip_update_geometry();
    if (just_shown) {
        if (!panel_horizontal)
            y -= height / 2; // center vertically
        just_shown = FALSE;
    }
    tooltip_adjust_geometry();
    XMoveResizeWindow(server.display, g_tooltip.window, x, y, width, height);

    // Stuff for drawing the tooltip
    cairo_surface_t *cs = cairo_xlib_surface_create(server.display, g_tooltip.window, server.visual, width, height);
    cairo_t *c = cairo_create(cs);
    Color bc = g_tooltip.bg->fill_color;
    Border b = g_tooltip.bg->border;
    if (server.real_transparency) {
        clear_pixmap(g_tooltip.window, 0, 0, width, height);
        draw_rect(c, b.width, b.width, width - 2 * b.width, height - 2 * b.width, b.radius - b.width / 2.0, g_tooltip.bg->border.rmask);
        cairo_set_source_rgba(c, bc.rgb[0], bc.rgb[1], bc.rgb[2], bc.alpha);
    } else {
        cairo_rectangle(c, 0., 0, width, height);
        cairo_set_source_rgb(c, bc.rgb[0], bc.rgb[1], bc.rgb[2]);
    }
    cairo_fill_preserve (c);
    cairo_set_line_width(c, b.width);
    if (server.real_transparency)
        draw_rect(c, 0.0, 0.0, width, height, b.radius + b.width / 2.0, g_tooltip.bg->border.rmask);
    else
        cairo_rectangle(c, 0.0, 0.0, width, height);
    cairo_set_source_rgba(c, b.color.rgb[0], b.color.rgb[1], b.color.rgb[2], b.color.alpha);
    cairo_set_operator (c, CAIRO_OPERATOR_ADD);
    cairo_set_fill_rule (c, CAIRO_FILL_RULE_EVEN_ODD);
    cairo_fill (c);
    cairo_set_operator (c, CAIRO_OPERATOR_OVER);

    Color fc = g_tooltip.font_color;
    cairo_set_source_rgba(c, fc.rgb[0], fc.rgb[1], fc.rgb[2], fc.alpha);
    PangoContext *context = pango_cairo_create_context(c);
    pango_cairo_context_set_resolution(context, 96 * panel->scale);
    PangoLayout *layout = pango_layout_new(context);
    pango_layout_set_font_description(layout, g_tooltip.font_desc);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD);
    pango_layout_set_text(layout, g_tooltip.tooltip_text, -1);
    pango_layout_set_width(layout, width * PANGO_SCALE);
    pango_layout_set_height(layout, height * PANGO_SCALE);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
    // I do not know why this is the right way, but with the below cairo_move_to it seems to be centered (horiz. and
    // vert.)
    {PangoRectangle ext;
        pango_layout_get_pixel_extents(layout, &ext, NULL);
        cairo_move_to(c,
                      -ext.x / 2 + left_bg_border_width(g_tooltip.bg) + g_tooltip.spacing * panel->scale,
                      -ext.y / 2 +  top_bg_border_width(g_tooltip.bg) + g_tooltip.paddingy * panel->scale + 1);
    }
    pango_cairo_show_layout(c, layout);
    g_object_unref(layout);
    g_object_unref(context);

    if (g_tooltip.image) {
        cairo_translate(c,
                        left_bg_border_width(g_tooltip.bg)            + g_tooltip.spacing * panel->scale,
                        height - bottom_bg_border_width(g_tooltip.bg) - g_tooltip.paddingy * panel->scale 
                               - cairo_image_surface_get_height(g_tooltip.image)
        );
        cairo_set_source_surface(c, g_tooltip.image, 0, 0);
        //~ cairo_paint(c); // Leak source. Nothing to do against. DO NOT USE cairo_paint(), do not remove this warning.
        cairo_rectangle( c, 0, 0, width, height);
        cairo_fill( c);
    }

    cairo_destroy(c);
    cairo_surface_destroy(cs);
}

void tooltip_update_for_area(Area *area)
{
    if (g_tooltip.mapped && g_tooltip.area == area) {
        tooltip_set_area(area);
        tooltip_update();
    }
}

void tooltip_trigger_hide()
{
    if (g_tooltip.mapped) {
        tooltip_set_area(NULL);
        start_hide_timer();
    } else
        // tooltip not visible yet, but maybe a timer is still pending
        stop_tooltip_timer();
}

void tooltip_hide(void *arg)
{
    if (g_tooltip.mapped) {
        g_tooltip.mapped = False;
        XUnmapWindow(server.display, g_tooltip.window);
        XFlush(server.display);
    }
    g_tooltip.area = NULL;
}

void start_show_timer()
{
    change_timer(&g_tooltip.visibility_timer, true, g_tooltip.show_timeout_msec, 0, tooltip_show, 0);
}

void start_hide_timer()
{
    change_timer(&g_tooltip.visibility_timer, true, g_tooltip.hide_timeout_msec, 0, tooltip_hide, 0);
}

void stop_tooltip_timer()
{
    stop_timer(&g_tooltip.visibility_timer);
}

void tooltip_update_contents_timeout(void *arg)
{
    tooltip_set_area(g_tooltip.area);
}

void tooltip_set_area(Area *area)
{
    free_and_null( g_tooltip.tooltip_text);
    if (g_tooltip.image)
    {
        cairo_surface_destroy( g_tooltip.image);
        g_tooltip.image = NULL;
    }
    if (area)
    {
        if (area->_get_tooltip_text)
            g_tooltip.tooltip_text = area->_get_tooltip_text( area);
        if (area->_get_tooltip_image)
        {
            g_tooltip.image = area->_get_tooltip_image( area);
            if (g_tooltip.image)
                cairo_surface_reference( g_tooltip.image);
            else
                change_timer( & g_tooltip.update_timer, true, 300, 0, tooltip_update_contents_timeout, NULL);
        }
    }
    g_tooltip.area = area;
}
