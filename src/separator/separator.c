// Tint2 : Separator plugin
// Author: Oskari Rauta

#include <string.h>
#include <stdio.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <pango/pangocairo.h>
#include <stdlib.h>
#include <math.h>

#include "window.h"
#include "server.h"
#include "panel.h"
#include "common.h"
#include "separator.h"

int separator_compute_desired_size(void *obj);

Separator *create_separator()
{
    Separator *separator = (Separator *)calloc(1, sizeof(Separator));
    separator->color.rgb[0] = 0.5;
    separator->color.rgb[1] = 0.5;
    separator->color.rgb[2] = 0.5;
    separator->color.alpha = 0.9;
    separator->style = SEPARATOR_DOTS;
    separator->thickness = 3;
    separator->area.paddingx = 1;
    BUF_0TERM (separator->area.name);
    return separator;
}

void destroy_separator(void *obj)
{
    Separator *separator = (Separator *)obj;
    remove_area(&separator->area);
    free_area(&separator->area);
    free_and_null(separator);
}

gpointer copy_separator(gconstpointer arg, gpointer data)
{
    Separator *copy = malloc(sizeof(Separator));
    *copy = *(Separator *)arg;
    return copy;
}

void init_separator()
{
    GList *to_remove = panel_config.separator_list;
    for_panel_items_order (&& to_remove)
        if (panel_items_order[k] == ':') {
            to_remove = to_remove->next;
        }
    if (to_remove) {
        if (to_remove == panel_config.separator_list) {
            g_list_free_full(to_remove, destroy_separator);
            panel_config.separator_list = NULL;
        } else {
            // Cut panel_config.separator_list
            if (to_remove->prev)
                to_remove->prev->next = NULL;
            to_remove->prev = NULL;
            // Remove all elements of to_remove and to_remove itself
            g_list_free_full(to_remove, destroy_separator);
        }
    }
}

void init_separator_panel(void *p)
{
    Panel *panel = (Panel *)p;

    // Make sure this is only done once if there are multiple items
    if (panel->separator_list)
        return;

    // panel->separator_list is now a copy of the pointer panel_config.separator_list
    // We make it a deep list copy
    panel->separator_list = g_list_copy_deep(panel_config.separator_list, copy_separator, NULL);

    for (GList *l = panel->separator_list; l; l = l->next) {
        Separator *separator = (Separator *)l->data;
        if (!separator->area.bg)
            separator->area.bg = &g_array_index(backgrounds, Background, 0);
        separator->area.parent = p;
        separator->area.panel = p;
        snprintf (separator->area.name, strlen_const(separator->area.name), "separator");
        separator->area.size_mode = LAYOUT_FIXED;
        separator->area.resize_needed = TRUE;
        separator->area.on_screen = TRUE;
        separator->area._resize = resize_separator;
        separator->area._compute_desired_size = separator_compute_desired_size;
        separator->area._draw_foreground = draw_separator;
        area_gradients_create(&separator->area);
    }
}

void cleanup_separator()
{
    // Cleanup frontends
    for (int i = 0; i < num_panels; i++) {
        g_list_free_full(panels[i].separator_list, destroy_separator);
        panels[i].separator_list = NULL;
    }

    // Cleanup backends
    g_list_free_full(panel_config.separator_list, destroy_separator);
    panel_config.separator_list = NULL;
}

int separator_compute_desired_size(void *obj)
{
    Separator *separator = (Separator *)obj;
    Panel *panel = (Panel*)separator->area.panel;
    if (!separator->area.on_screen)
        return 0;

    return  separator->thickness + 2 * separator->area.paddingx * panel->scale
            + (panel_horizontal ? left_right_border_width : top_bottom_border_width)(&separator->area);
}

gboolean resize_separator(void *obj)
{
    Separator *separator = (Separator *)obj;
    Panel *panel = (Panel*)separator->area.panel;
    if (!separator->area.on_screen)
        return FALSE;

    int sep_width = separator->thickness + 2 * separator->area.paddingx * panel->scale + left_right_border_width(&separator->area);
    if (panel_horizontal) {
        separator->area.width = sep_width;
        separator->length = separator->area.height - top_bottom_border_width(&separator->area);
    } else {
        separator->length = separator->area.width - left_right_border_width(&separator->area);
        separator->area.height = sep_width;
    }
    separator->length -= 2 * separator->area.paddingy * panel->scale;

    schedule_redraw(&separator->area);
    schedule_panel_redraw();
    return TRUE;
}

void draw_separator_line(void *obj, cairo_t *c);
void draw_separator_dots(void *obj, cairo_t *c);

void draw_separator(void *obj, cairo_t *c)
{
    Separator *separator = (Separator *)obj;

    switch (separator->style) {
    case SEPARATOR_LINE: draw_separator_line(separator, c);
                         return;
    case SEPARATOR_DOTS: draw_separator_dots(separator, c);
    case SEPARATOR_EMPTY: return;
    }
}

void draw_separator_line(void *obj, cairo_t *c)
{
    Separator *separator = (Separator *)obj;

    if (separator->thickness <= 0)
        return;

    cairo_set_source_rgba(c,
                          separator->color.rgb[0],
                          separator->color.rgb[1],
                          separator->color.rgb[2],
                          separator->color.alpha);
    cairo_set_line_width(c, separator->thickness);
    cairo_set_line_cap(c, CAIRO_LINE_CAP_BUTT);
    if (panel_horizontal) {
        cairo_move_to(c, separator->area.width / 2.0, separator->area.height / 2.0 - separator->length / 2.0);
        cairo_line_to(c, separator->area.width / 2.0, separator->area.height / 2.0 + separator->length / 2.0);
    } else {
        cairo_move_to(c, separator->area.width / 2.0 - separator->length / 2.0, separator->area.height / 2.0);
        cairo_line_to(c, separator->area.width / 2.0 + separator->length / 2.0, separator->area.height / 2.0);
    }
    cairo_stroke(c);
}

void draw_separator_dots(void *obj, cairo_t *c)
{
    Separator *separator = (Separator *)obj;
    if (separator->thickness <= 0)
        return;

    cairo_set_source_rgba(c,
                          separator->color.rgb[0],
                          separator->color.rgb[1],
                          separator->color.rgb[2],
                          separator->color.alpha);
    cairo_set_line_width (c, separator->thickness);
    cairo_set_line_cap(c, CAIRO_LINE_CAP_ROUND);

    int num_circles = separator->length / (1.618 * separator->thickness - 1);
    double spacing = (separator->length - num_circles * separator->thickness) / MAX(1.0, num_circles - 1.0);
    if (spacing > separator->thickness) {
        num_circles++;
        spacing = (separator->length - num_circles * separator->thickness) / MAX(1.0, num_circles - 1.0);
    }
    double offset = ((panel_horizontal ? separator->area.height : separator->area.width) - separator->length) / 2.0;
    if (num_circles == 1)
        offset += spacing / 2.0;
    
    cairo_set_dash(c, (double[]){0, separator->thickness + spacing}, 2, 0);
    if (panel_horizontal) {
        cairo_move_to(c, separator->area.width / 2.0, offset + separator->thickness / 2.0);
        offset += (num_circles - 1) * (separator->thickness + spacing);
        cairo_line_to(c, separator->area.width / 2.0, offset + separator->thickness / 2.0);
    } else {
        cairo_move_to(c, offset + separator->thickness / 2.0, separator->area.height / 2.0);
        offset += (num_circles - 1) * (separator->thickness + spacing);
        cairo_move_to(c, offset + separator->thickness / 2.0, separator->area.height / 2.0);
    }
    cairo_stroke(c);
}
