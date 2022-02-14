/**************************************************************************
*
* Tint2 : area
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

#include <assert.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrender.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pango/pangocairo.h>

#include "area.h"
#include "server.h"
#include "panel.h"
#include "common.h"

#define for_children(a,i,T)     for (T i = (a)->children; i; i = i->next)
#define for_children_rev(a,i,T) for (T i = g_list_last((a)->children); i; i = i->prev)

Area *mouse_over_area = NULL;

void init_background(Background *bg)
{
    memset(bg, 0, sizeof(Background));
    bg->border.mask = BORDER_TOP | BORDER_BOTTOM | BORDER_LEFT | BORDER_RIGHT;
}

void initialize_positions(void *obj, int offset)
{
    Area *a = (Area *)obj;
    for_children(a, l, GList *) {
        Area *child = ((Area *)l->data);
        if (panel_horizontal) {
            child->posy = offset + top_border_width(a) + a->paddingy;
            child->height = a->height - 2 * a->paddingy - top_bottom_border_width(a);
            if (child->_on_change_layout)
                child->_on_change_layout(child);
            initialize_positions(child, child->posy);
        } else {
            child->posx = offset + left_border_width(a) + a->paddingy;
            child->width = a->width - 2 * a->paddingy - left_right_border_width(a);
            if (child->_on_change_layout)
                child->_on_change_layout(child);
            initialize_positions(child, child->posx);
        }
    }
}

void relayout_fixed(Area *a)
{
    if (!a->on_screen)
        return;

    // Children are resized before the parent
    for_children(a, l, GList *)
        relayout_fixed(l->data);

    // Recalculate size
    a->_changed = UNCHANGED;
    if (a->resize_needed && a->size_mode == LAYOUT_FIXED) {
        a->resize_needed = FALSE;

        if (a->_resize && a->_resize(a)) {
            // The size has changed => resize needed for the parent
            if (a->parent)
                ((Area *)a->parent)->resize_needed = TRUE;
            a->_changed |= CHANGED_SIZE;
        }
    }
}

void relayout_dynamic(Area *a, int level)
{
    if (!a->on_screen)
        return;

    // Area is resized before its children
    if (a->resize_needed && a->size_mode == LAYOUT_DYNAMIC) {
        a->resize_needed = FALSE;

        if (a->_resize) {
            if (a->_resize(a))
                a->_changed |= CHANGED_SIZE;
            // resize children with LAYOUT_DYNAMIC
            for_children(a, l, GList *) {
                Area *child = ((Area *)l->data);
                if (child->size_mode == LAYOUT_DYNAMIC && child->children)
                    child->resize_needed = TRUE;
            }
        }
    }

    // Layout children
    if (a->children) {
        if (a->alignment == ALIGN_LEFT) {
            int pos =
                (panel_horizontal ? a->posx + left_border_width(a) : a->posy + top_border_width(a)) + a->paddingxlr;

            for_children(a, l, GList *) {
                Area *child = ((Area *)l->data);
                if (!child->on_screen)
                    continue;

                if (panel_horizontal) {
                    if (pos != child->posx) {
                        // pos changed => redraw
                        child->posx = pos;
                        child->_changed |= CHANGED_MOVE;
                    }
                } else {
                    if (pos != child->posy) {
                        // pos changed => redraw
                        child->posy = pos;
                        child->_changed |= CHANGED_MOVE;
                    }
                }

                relayout_dynamic(child, level + 1);

                pos += panel_horizontal ? child->width + a->paddingx : child->height + a->paddingx;
            }
        } else if (a->alignment == ALIGN_RIGHT) {
            int pos = (panel_horizontal ? a->posx + a->width - right_border_width(a)
                                        : a->posy + a->height - bottom_border_width(a)) -
                      a->paddingxlr;

            for_children_rev(a, l, GList *) {
                Area *child = ((Area *)l->data);
                if (!child->on_screen)
                    continue;

                pos -= panel_horizontal ? child->width : child->height;

                if (panel_horizontal) {
                    if (pos != child->posx) {
                        // pos changed => redraw
                        child->posx = pos;
                        child->_changed |= CHANGED_MOVE;
                    }
                } else {
                    if (pos != child->posy) {
                        // pos changed => redraw
                        child->posy = pos;
                        child->_changed |= CHANGED_MOVE;
                    }
                }

                relayout_dynamic(child, level + 1);

                pos -= a->paddingx;
            }
        } else if (a->alignment == ALIGN_CENTER) {

            int children_size = 0;

            for_children(a, l, GList *) {
                Area *child = ((Area *)l->data);
                if (!child->on_screen)
                    continue;

                children_size += panel_horizontal ? child->width : child->height;
                children_size += (l == a->children) ? 0 : a->paddingx;
            }

            int pos = (panel_horizontal ? a->posx + left_border_width(a) : a->posy + top_border_width(a)) + a->paddingxlr
                    + ((panel_horizontal ? a->width : a->height) - children_size) / 2;

            for_children(a, l, GList *) {
                Area *child = ((Area *)l->data);
                if (!child->on_screen)
                    continue;

                if (panel_horizontal) {
                    if (pos != child->posx) {
                        // pos changed => redraw
                        child->posx = pos;
                        child->_changed |= CHANGED_MOVE;
                    }
                } else {
                    if (pos != child->posy) {
                        // pos changed => redraw
                        child->posy = pos;
                        child->_changed |= CHANGED_MOVE;
                    }
                }

                relayout_dynamic(child, level + 1);

                pos += panel_horizontal ? child->width + a->paddingx : child->height + a->paddingx;
            }
        }
    }

    if (a->_changed) {
        // pos/size changed
        a->_redraw_needed = TRUE;
        if (a->_on_change_layout)
            a->_on_change_layout(a);
    }
}

int compute_desired_size(Area *a)
{
    if (!a->on_screen)
        return 0;
    if (a->_compute_desired_size)
        return a->_compute_desired_size(a);
    if (a->size_mode == LAYOUT_FIXED)
        fprintf(stderr, YELLOW "tint2: Area %s does not set desired size!" RESET "\n", a->name);
    return container_compute_desired_size(a);
}

int container_compute_desired_size(Area *a)
{
    if (!a->on_screen)
        return 0;
    int result = 2 * a->paddingxlr + (panel_horizontal ? left_right_border_width(a) : top_bottom_border_width(a));
    int children_count = 0;
    for_children(a, l, GList *) {
        Area *child = (Area *)l->data;
        if (child->on_screen) {
            result += compute_desired_size(child);
            children_count++;
        }
    }
    if (children_count > 0)
        result += (children_count - 1) * a->paddingx;
    return result;
}

void relayout(Area *a)
{
    relayout_fixed(a);
    relayout_dynamic(a, 1);
}

int relayout_with_constraint(Area *a, int maximum_size)
{
    int fixed_children_count = 0;
    int dynamic_children_count = 0;

    if (panel_horizontal) {
        // detect free size for LAYOUT_DYNAMIC Areas
        int size = a->width - 2 * a->paddingxlr - left_right_border_width(a);
        for_children(a, l, GList *) {
            Area *child = (Area *)l->data;
            if (child->on_screen && child->size_mode == LAYOUT_FIXED) {
                size -= child->width;
                fixed_children_count++;
            }
            if (child->on_screen && child->size_mode == LAYOUT_DYNAMIC)
                dynamic_children_count++;
        }
        if (fixed_children_count + dynamic_children_count > 0)
            size -= (fixed_children_count + dynamic_children_count - 1) * a->paddingx;

        int width = 0;
        int modulo = 0;
        if (dynamic_children_count > 0) {
            width = size / dynamic_children_count;
            modulo = size % dynamic_children_count;
            if (width > maximum_size && maximum_size > 0) {
                width = maximum_size;
                modulo = 0;
            }
        }

        // Resize LAYOUT_DYNAMIC objects
        for_children(a, l, GList *) {
            Area *child = (Area *)l->data;
            if (child->on_screen && child->size_mode == LAYOUT_DYNAMIC) {
                int old_width = child->width;
                child->width = width;
                if (modulo) {
                    child->width++;
                    modulo--;
                }
                if (child->width != old_width)
                    child->_changed |= CHANGED_SIZE;
            }
        }
    } else {
        // detect free size for LAYOUT_DYNAMIC's Area
        int size = a->height - 2 * a->paddingxlr - top_bottom_border_width(a);
        for_children(a, l, GList *) {
            Area *child = (Area *)l->data;
            if (child->on_screen && child->size_mode == LAYOUT_FIXED) {
                size -= child->height;
                fixed_children_count++;
            }
            if (child->on_screen && child->size_mode == LAYOUT_DYNAMIC)
                dynamic_children_count++;
        }
        if (fixed_children_count + dynamic_children_count > 0)
            size -= (fixed_children_count + dynamic_children_count - 1) * a->paddingx;

        int height = 0;
        int modulo = 0;
        if (dynamic_children_count) {
            height = size / dynamic_children_count;
            modulo = size % dynamic_children_count;
            if (height > maximum_size && maximum_size != 0) {
                height = maximum_size;
                modulo = 0;
            }
        }

        // Resize LAYOUT_DYNAMIC objects
        for_children(a, l, GList *) {
            Area *child = (Area *)l->data;
            if (child->on_screen && child->size_mode == LAYOUT_DYNAMIC) {
                int old_height = child->height;
                child->height = height;
                if (modulo) {
                    child->height++;
                    modulo--;
                }
                if (child->height != old_height)
                    child->_changed |= CHANGED_SIZE;
            }
        }
    }
    return 0;
}

void schedule_redraw(Area *a)
{
    a->_redraw_needed = TRUE;

    for_children(a, l, GList *)
        schedule_redraw((Area *)l->data);
    schedule_panel_redraw();
}

void draw_tree(Area *a)
{
    if (!a->on_screen)
        return;

    if (a->_redraw_needed) {
        a->_redraw_needed = FALSE;
        draw(a);
    }

    if (a->pix)
        XCopyArea(server.display,
                  a->pix, ((Panel *)a->panel)->temp_pmap, server.gc,
                  0,        0,
                  a->width, a->height,
                  a->posx,  a->posy);
    else
        fprintf(stderr, RED "tint2: %s %d: area %s has no pixmap!!!" RESET "\n", __FILE__, __LINE__, a->name);

    for_children(a, l, GList *)
        draw_tree((Area *)l->data);
}

void hide(Area *a)
{
    Area *parent = (Area *)a->parent;

    if (!a->on_screen)
        return;
    a->on_screen = FALSE;
    if (parent)
        parent->resize_needed = TRUE;
    if (panel_horizontal)
        a->width = 0;
    else
        a->height = 0;
}

void show(Area *a)
{
    Area *parent = (Area *)a->parent;

    if (a->on_screen)
        return;
    a->on_screen = TRUE;
    if (parent)
        parent->resize_needed = TRUE;
    a->resize_needed = TRUE;
    schedule_panel_redraw();
}

void gradient_pattern_destroy(GradientInstance *gi)
{
    if (gi->pattern) {
        cairo_pattern_destroy(gi->pattern);
        gi->pattern = NULL;
    }
}

void update_dependent_gradients(Area *a)
{
    if (!a->on_screen)
        return;
    if (a->_changed & CHANGED_SIZE) {
        for (GList *l = a->dependent_gradients; l; l = l->next) {
            GradientInstance *gi = (GradientInstance *)l->data;
            gradient_pattern_destroy(gi);
            update_gradient(gi);
            if (gi->area != a)
                schedule_redraw(gi->area);
        }
    }
    for_children(a, l, GList *)
        update_dependent_gradients((Area *)l->data);
}

void free_pixmaps(Area *a)
{
    a->pix = None;
    for (int i = 0; i < MOUSE_STATE_COUNT; i++) {
        if (! a->pix_by_state[i])
            continue;
        XFreePixmap(server.display, a->pix_by_state[i]);
        a->pix_by_state[i] = None;
    }
}

void draw(Area *a)
{
    if (a->_changed)
        // On resize/move, invalidate cached pixmaps
        free_pixmaps (a);

    {int pix_i = a->has_mouse_over_effect ? a->mouse_state : 0;
        if (! a->pix_by_state[pix_i]) {
            a->pix_by_state[pix_i] = a->pix = XCreatePixmap(server.display, server.root_win,
                                                            a->width, a->height, server.depth);
        } else
            a->pix = a->pix_by_state[pix_i];
    }

    if (!a->_clear) {
        // Add layer of root pixmap (or clear pixmap if real_transparency==true)
        XCopyArea(server.display,
                  ((Panel *)a->panel)->temp_pmap, a->pix, server.gc,
                  a->posx,  a->posy,
                  a->width, a->height,
                  0, 0);
    } else {
        a->_clear(a);
    }

    cairo_surface_t *cs = cairo_xlib_surface_create(server.display, a->pix, server.visual, a->width, a->height);
    cairo_t *c = cairo_create(cs);

    draw_background(a, c);

    if (a->_draw_foreground)
        a->_draw_foreground(a, c);

    cairo_destroy(c);
    cairo_surface_destroy(cs);
}

double color_percieved_brightness(double *rgb)
{
    return  rgb[0] == 0  ?
            (
                rgb[1] == 0  ?  rgb[2] * sqrt(.114) :
                rgb[2] == 0  ?  rgb[1] * sqrt(.587) :
                sqrt(   .587 * rgb[1] * rgb[1] +
                        .114 * rgb[2] * rgb[2] )
            ) :
            rgb[1] == 0  ?
            (
                rgb[2] == 0  ?  rgb[0] * sqrt(.299) :
                sqrt(   .299 * rgb[0] * rgb[0] +
                        .114 * rgb[2] * rgb[2] )
            ) :
            sqrt(   .299 * rgb[0] * rgb[0] +
                    .587 * rgb[1] * rgb[1] + (
                        rgb[2] == 0 ?
                        0 :
                        .114 * rgb[2] * rgb[2]
                    )
            );
}

double tint_color_channel(double a, double b, double tint_weight)
{
    return  (tint_weight == 0.0) ? a :
            (tint_weight == 1.0) ? b :
            sqrt(a*a * (1-tint_weight) + b*b * tint_weight);
}

void set_cairo_source_tinted(cairo_t *c, Color *color1, Color *color2, double tint_weight)
{
    double  *rgb1 = color1->rgb,
            *rgb2 = color2->rgb,
            alpha = color1->alpha,
            C, L1, L2;

    // skip for grayscale content color, invisible or black target color
    if (alpha == 0.0 || memcmp(rgb1, (double [3]){0.0, 0.0, 0.0}, sizeof(double) * 3) == 0 ||
        (rgb2[0] == rgb2[1] && rgb2[0] == rgb2[2]) )
    {
        cairo_set_source_rgba(c, rgb1[0], rgb1[1], rgb1[2], alpha);
        return;
    }

    C = (L1 = color_percieved_brightness(rgb1)) / (L2 = color_percieved_brightness(rgb2));

    // decay brightness multiplier by L1 square curve
    // (bent towards 0 for better compensation chance)
    L1 *= alpha;
    L1 *= L1;
    C = C * (1 - L1) + L1;

    // content color brightness match
    rgb2[0] *= C;
    rgb2[1] *= C;
    rgb2[2] *= C;

    // move color overflow to alpha
    double M = MAX(rgb2[0], MAX(rgb2[1], rgb2[2]));
    if (M > 1.0) {
        rgb2[0] /= M, rgb2[1] /= M, rgb2[2] /= M, alpha *= M;
    }
    // fix alpha overflow via saturation
    if (alpha > 1.0)
    {
        L2 *= C;
        double mul = (1 - L2) / (alpha - L2);
        for (int i=0; i < 2; i++)
            rgb2[i] = L2 + (rgb2[i] * alpha - L2) * mul;
        alpha = 1.0;
    }
    cairo_set_source_rgba(c,
                          tint_color_channel(rgb1[0], rgb2[0], tint_weight),
                          tint_color_channel(rgb1[1], rgb2[1], tint_weight),
                          tint_color_channel(rgb1[2], rgb2[2], tint_weight),
                          alpha);
}

void set_cairo_source_bg_color(Area *a, cairo_t *c)
{
    Color content_color, *color;

    color = (a->mouse_state == MOUSE_OVER) ? &a->bg->fill_color_hover :
            (a->mouse_state == MOUSE_DOWN) ? &a->bg->fill_color_pressed :
            &a->bg->fill_color;

    if (a->_get_content_color) {
        a->_get_content_color(a, &content_color);
        set_cairo_source_tinted(c, color, &content_color, a->bg->fill_content_tint_weight);
    } else
        cairo_set_source_rgba (c, color->rgb[0], color->rgb[1], color->rgb[2], color->alpha);
}

void set_cairo_source_border_color(Area *a, cairo_t *c)
{
    Color content_color, *color;

    color = (a->mouse_state == MOUSE_OVER) ? &a->bg->border_color_hover :
            (a->mouse_state == MOUSE_DOWN) ? &a->bg->border_color_pressed :
            &a->bg->border.color;

    if (a->_get_content_color) {
        a->_get_content_color(a, &content_color);
        set_cairo_source_tinted(c, color, &content_color, a->bg->border_content_tint_weight);
    } else
        cairo_set_source_rgba (c, color->rgb[0], color->rgb[1], color->rgb[2], color->alpha);
}

void draw_background(Area *a, cairo_t *c)
{
    cairo_push_group (c);
    cairo_set_operator(c, CAIRO_OPERATOR_ADD);
    
    if ((a->bg->fill_color.alpha > 0.0) ||
        (panel_config.mouse_effects && (a->has_mouse_over_effect || a->has_mouse_press_effect))) {

        // Not sure about this
        draw_rect(c,
                  left_border_width(a),
                  top_border_width(a),
                  a->width - left_right_border_width(a),
                  a->height - top_bottom_border_width(a),
                  a->bg->border.radius - a->bg->border.width / 2.0);
        set_cairo_source_bg_color(a, c);
        cairo_fill(c);
    }
    for (GList *l = a->gradient_instances_by_state[a->mouse_state]; l; l = l->next) {
        GradientInstance *gi = (GradientInstance *)l->data;
        if (!gi->pattern)
            update_gradient(gi);
        cairo_set_source(c, gi->pattern);
        draw_rect(c,
                  left_border_width(a),
                  top_border_width(a),
                  a->width - left_right_border_width(a),
                  a->height - top_bottom_border_width(a),
                  a->bg->border.radius - a->bg->border.width / 2.0);
        cairo_fill(c);
    }

    if (a->bg->border.width > 0) {
        cairo_set_line_width(c, a->bg->border.width);

        // draw border inside (x, y, width, height)
        set_cairo_source_border_color(a, c);
        draw_rect_on_sides(c,
                           left_border_width(a) / 2.,
                           top_border_width(a) / 2.,
                           a->width - left_right_border_width(a) / 2.,
                           a->height - top_bottom_border_width(a) / 2.,
                           a->bg->border.radius,
                           a->bg->border.mask);

        cairo_stroke(c);
    }
    cairo_pop_group_to_source (c);
    cairo_paint (c);
}

void remove_area(Area *a)
{
    Area *area = (Area *)a;
    Area *parent = (Area *)area->parent;

    area_gradients_free(a);

    if (parent) {
        parent->children = g_list_remove(parent->children, area);
        parent->resize_needed = TRUE;
        schedule_panel_redraw();
        schedule_redraw(parent);
    }

    if (mouse_over_area == a) {
        mouse_out();
    }
}

void add_area(Area *a, Area *parent)
{
    g_assert_null(a->parent);

    a->parent = parent;
    if (parent) {
        parent->children = g_list_append(parent->children, a);
        parent->resize_needed = TRUE;
        schedule_redraw(parent);
    }
}

void free_area(Area *a)
{
    if (!a)
        return;

    for_children(a, l, GList *)
        free_area(l->data);

    if (a->children) {
        g_list_free(a->children);
        a->children = NULL;
    }
    free_pixmaps (a);
    if (mouse_over_area == a) {
        mouse_over_area = NULL;
    }
    area_gradients_free(a);
}

void mouse_over(Area *area, gboolean pressed)
{
    if (mouse_over_area == area && !area)
        return;

    MouseState new_state = MOUSE_NORMAL;
    if (area) {
        if (!pressed) {
            new_state = area->has_mouse_over_effect ? MOUSE_OVER : MOUSE_NORMAL;
        } else {
            new_state =
                area->has_mouse_press_effect ? MOUSE_DOWN : area->has_mouse_over_effect ? MOUSE_OVER : MOUSE_NORMAL;
        }
    }

    if (mouse_over_area == area && mouse_over_area->mouse_state == new_state)
        return;
    mouse_out();
    if (new_state == MOUSE_NORMAL)
        return;
    mouse_over_area = area;

    mouse_over_area->mouse_state = new_state;
    mouse_over_area->pix = mouse_over_area->pix_by_state[mouse_over_area->mouse_state];
    mouse_over_area->_redraw_needed = TRUE;
    schedule_panel_redraw();
}

void mouse_out()
{
    if (!mouse_over_area)
        return;
    mouse_over_area->mouse_state = MOUSE_NORMAL;
    mouse_over_area->pix = mouse_over_area->pix_by_state[mouse_over_area->mouse_state];
    mouse_over_area->_redraw_needed = TRUE;
    schedule_panel_redraw();
    mouse_over_area = NULL;
}

gboolean area_is_first(void *obj)
{
    Area *a = obj;
    if (!a->on_screen)
        return FALSE;

    Panel *panel = a->panel;

    Area *node = &panel->area;

    while (node) {
        if (!node->on_screen || node->width == 0 || node->height == 0)
            return FALSE;
        if (node == a)
            return TRUE;

        GList *l = node->children;
        node = NULL;
        for (; l; l = l->next) {
            Area *child = l->data;
            if (!child->on_screen || child->width == 0 || child->height == 0)
                continue;
            node = child;
            break;
        }
    }

    return FALSE;
}

gboolean area_is_last(void *obj)
{
    Area *a = obj;
    if (!a->on_screen)
        return FALSE;

    Panel *panel = a->panel;

    Area *node = &panel->area;

    while (node) {
        if (!node->on_screen || node->width == 0 || node->height == 0)
            return FALSE;
        if (node == a)
            return TRUE;

        GList *l = node->children;
        node = NULL;
        for (; l; l = l->next) {
            Area *child = l->data;
            if (!child->on_screen || child->width == 0 || child->height == 0)
                continue;
            node = child;
        }
    }

    return FALSE;
}

gboolean area_is_under_mouse(void *obj, int x, int y)
{
    Area *a = obj;
    return  !a->on_screen || !a->width || !a->height
                ? FALSE
        :   a->_is_under_mouse
                ? a->_is_under_mouse(a, x, y)
        : (x >= a->posx) && (x <= a->posx + a->width) && (y >= a->posy) && (y <= a->posy + a->height);
}

gboolean full_width_area_is_under_mouse(void *obj, int x, int y)
{
    Area *a = obj;
    return  !a->on_screen
                ? FALSE
        :   a->_is_under_mouse && a->_is_under_mouse != full_width_area_is_under_mouse
                ? a->_is_under_mouse(a, x, y)
        :   panel_horizontal ? (x >= a->posx) && (x <= a->posx + a->width)
                             : (y >= a->posy) && (y <= a->posy + a->height);
}

Area *find_area_under_mouse(void *root, int x, int y)
{
    Area *result = root;
    Area *new_result = result;
    do {
        result = new_result;
        GList *it = result->children;
        while (it) {
            Area *a = (Area *)it->data;
            if (area_is_under_mouse(a, x, y)) {
                new_result = a;
                break;
            }
            it = it->next;
        }
    } while (new_result != result);
    return result;
}

int left_border_width(Area *a)
{
    return left_bg_border_width(a->bg);
}

int right_border_width(Area *a)
{
    return right_bg_border_width(a->bg);
}

int top_border_width(Area *a)
{
    return top_bg_border_width(a->bg);
}

int bottom_border_width(Area *a)
{
    return bottom_bg_border_width(a->bg);
}

int left_right_border_width(Area *a)
{
    return left_right_bg_border_width(a->bg);
}

int top_bottom_border_width(Area *a)
{
    return top_bottom_bg_border_width(a->bg);
}

int bg_border_width(Background *bg, int mask)
{
    return bg->border.mask & mask ? bg->border.width : 0;
}

int left_bg_border_width(Background *bg)
{
    return bg_border_width(bg, BORDER_LEFT);
}

int top_bg_border_width(Background *bg)
{
    return bg_border_width(bg, BORDER_TOP);
}

int right_bg_border_width(Background *bg)
{
    return bg_border_width(bg, BORDER_RIGHT);
}

int bottom_bg_border_width(Background *bg)
{
    return bg_border_width(bg, BORDER_BOTTOM);
}

int left_right_bg_border_width(Background *bg)
{
    return left_bg_border_width(bg) + right_bg_border_width(bg);
}

int top_bottom_bg_border_width(Background *bg)
{
    return top_bg_border_width(bg) + bottom_bg_border_width(bg);
}

void area_dump_geometry(Area *area, int indent)
{
    fprintf(stderr, "tint2: %*s%s:\n", indent, "", area->name);
    indent += 2;
    if (!area->on_screen) {
        fprintf(stderr, "tint2: %*shidden\n", indent, "");
        return;
    }
    fprintf(stderr,
            "tint2: %*sBox: x = %d, y = %d, w = %d, h = %d, desired size = %d\n",
            indent,
            "",
            area->posx,
            area->posy,
            area->width,
            area->height,
            compute_desired_size(area));
    fprintf(stderr,
            "tint2: %*sBorder: left = %d, right = %d, top = %d, bottom = %d\n",
            indent,
            "",
            left_border_width(area),
            right_border_width(area),
            top_border_width(area),
            bottom_border_width(area));
    fprintf(stderr,
            "tint2: %*sPadding: left = right = %d, top = bottom = %d, spacing = %d\n",
            indent,
            "",
            area->paddingxlr,
            area->paddingy,
            area->paddingx);
    if (area->_dump_geometry)
        area->_dump_geometry(area, indent);
    if (area->children) {
        fprintf(stderr, "tint2: %*sChildren:\n", indent, "");
        indent += 2;
        for_children(area, l, GList *)
            area_dump_geometry((Area *)l->data, indent);
    }
}

void area_compute_available_size(Area *area,
                             int *available_w,
                             int *available_h)
{
    Panel *panel = (Panel *)area->panel;
    if (panel_horizontal) {
        *available_w = panel->area.width;
        *available_h = area->height - 2 * area->paddingy - left_right_border_width(area);
    } else {
        *available_w = area->width - 2 * area->paddingxlr - left_right_border_width(area);
        *available_h = panel->area.height;
    }
}

void area_compute_inner_size(Area *area,
                             int *inner_w,
                             int *inner_h)
{
    if (panel_horizontal) {
        *inner_w = area->width - 2 * area->paddingxlr - left_right_border_width(area);
        *inner_h = area->height - 2 * area->paddingy - top_bottom_border_width(area);
    } else {
        *inner_w = area->width - 2 * area->paddingxlr - left_right_border_width(area);
        *inner_h = area->height - 2 * area->paddingy - top_bottom_border_width(area);
    }
}

void area_compute_text_geometry(Area *area,
                                const char *line1,
                                const char *line2,
                                PangoFontDescription *line1_font_desc,
                                PangoFontDescription *line2_font_desc,
                                int *line1_height,
                                int *line1_width,
                                int *line2_height,
                                int *line2_width)
{
    int available_w, available_h;
    area_compute_available_size(area, &available_w, &available_h);

    if (line1 && line1[0])
        get_text_size2(line1_font_desc,
                       line1_height,
                       line1_width,
                       available_h,
                       available_w,
                       line1,
                       strlen(line1),
                       PANGO_WRAP_WORD_CHAR,
                       PANGO_ELLIPSIZE_NONE,
                       PANGO_ALIGN_CENTER,
                       FALSE,
                       ((Panel*)area->panel)->scale);
    else
        *line1_width = *line1_height = 0;

    if (line2 && line2[0])
        get_text_size2(line2_font_desc,
                       line2_height,
                       line2_width,
                       available_h,
                       available_w,
                       line2,
                       strlen(line2),
                       PANGO_WRAP_WORD_CHAR,
                       PANGO_ELLIPSIZE_NONE,
                       PANGO_ALIGN_CENTER,
                       FALSE,
                       ((Panel*)area->panel)->scale);
    else
        *line2_width = *line2_height = 0;
}

int text_area_compute_desired_size(Area *area,
                                   const char *line1,
                                   const char *line2,
                                   PangoFontDescription *line1_font_desc,
                                   PangoFontDescription *line2_font_desc)
{
    int line1_height, line1_width, line2_height, line2_width;
    area_compute_text_geometry(area,
                               line1,
                               line2,
                               line1_font_desc,
                               line2_font_desc,
                               &line1_height,
                               &line1_width,
                               &line2_height,
                               &line2_width);

    return panel_horizontal ? MAX(line1_width, line2_width) + 2 * area->paddingxlr + left_right_border_width(area)
                            : line1_height + line2_height + 2 * area->paddingy + top_bottom_border_width(area);
}

gboolean resize_text_area(Area *area,
                          const char *line1,
                          const char *line2,
                          PangoFontDescription *line1_font_desc,
                          PangoFontDescription *line2_font_desc,
                          int *line1_posy,
                          int *line2_posy)
{
    gboolean result = FALSE;

    schedule_redraw(area);

    int line1_height, line1_width;
    int line2_height, line2_width;
    area_compute_text_geometry(area,
                               line1,
                               line2,
                               line1_font_desc,
                               line2_font_desc,
                               &line1_height,
                               &line1_width,
                               &line2_height,
                               &line2_width);

    int new_size = text_area_compute_desired_size(area,
                                                  line1,
                                                  line2,
                                                  line1_font_desc,
                                                  line2_font_desc);
    if (panel_horizontal) {
        if (new_size != area->width) {
            if (new_size < area->width && abs(new_size - area->width) < 6) {
                // we try to limit the number of resizes
                new_size = area->width;
            } else {
                area->width = new_size;
            }
            *line1_posy = (area->height - line1_height) / 2;
            if (line2) {
                *line1_posy -= (line2_height) / 2;
                *line2_posy = *line1_posy + line1_height;
            }
            result = TRUE;
        }
    } else {
        if (new_size != area->height) {
            area->height = new_size;
            *line1_posy = (area->height - line1_height) / 2;
            if (line2) {
                *line1_posy -= (line2_height) / 2;
                *line2_posy = *line1_posy + line1_height;
            }
            result = TRUE;
        }
    }

    return result;
}

void draw_text_area(Area *area,
                    cairo_t *c,
                    const char *line1,
                    const char *line2,
                    PangoFontDescription *line1_font_desc,
                    PangoFontDescription *line2_font_desc,
                    int line1_posy,
                    int line2_posy,
                    Color *color,
                    double scale)
{
    int inner_w, inner_h;
    area_compute_inner_size(area, &inner_w, &inner_h);

    PangoContext *context = pango_cairo_create_context(c);
    pango_cairo_context_set_resolution(context, 96 * scale);
    PangoLayout *layout = pango_layout_new(context);

    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);
    pango_layout_set_width(layout, inner_w * PANGO_SCALE);
    pango_layout_set_height(layout, inner_h * PANGO_SCALE);
    cairo_set_source_rgba(c, color->rgb[0], color->rgb[1], color->rgb[2], color->alpha);

    if (line1 && line1[0]) {
        pango_layout_set_font_description(layout, line1_font_desc);
        pango_layout_set_text(layout, line1, strlen(line1));
        pango_cairo_update_layout(c, layout);
        draw_text(layout, c, (area->width - inner_w) / 2, line1_posy, color, ((Panel *)area->panel)->font_shadow ? layout : NULL);
    }

    if (line2 && line2[0]) {
        pango_layout_set_font_description(layout, line2_font_desc);
        pango_layout_set_indent(layout, 0);
        pango_layout_set_text(layout, line2, strlen(line2));
        pango_cairo_update_layout(c, layout);
        draw_text(layout, c, (area->width - inner_w) / 2, line2_posy, color, ((Panel *)area->panel)->font_shadow ? layout : NULL);
    }

    g_object_unref(layout);
    g_object_unref(context);
}

gboolean gradient_point_area_dependent(ControlPoint *control)
{
    return (!CONST_OFFSET(control->offsets_x) ||
            !CONST_OFFSET(control->offsets_y) ||
            !CONST_OFFSET(control->offsets_r));
}

void gradient_init(Area *area, GradientClass *g, GradientInstance *gi)
{
    g_assert_nonnull(area);
    g_assert_nonnull(g);
    gi->area = area;
    gi->gradient_class = g;
    if (gradient_point_area_dependent(&g->from) ||
        gradient_point_area_dependent(&g->to))
    {
        area->dependent_gradients = g_list_append(area->dependent_gradients, gi);
    }
}

void gradient_destroy(GradientInstance *gi)
{
    gradient_pattern_destroy(gi);
    GradientClass *g = gi->gradient_class;
    if (gradient_point_area_dependent(&g->from) ||
        gradient_point_area_dependent(&g->to))
    {
        Area *area = gi->area;
        area->dependent_gradients = g_list_remove(area->dependent_gradients, gi);
    }
    gi->gradient_class = NULL;
}

void area_gradients_create(Area *area)
{
    if (debug_gradients)
        fprintf(stderr, "tint2: Initializing gradients for area %s\n", area->name);
    for (int i = 0; i < MOUSE_STATE_COUNT; i++) {
        g_assert_null(area->gradient_instances_by_state[i]);
        GradientClass *g = area->bg->gradients[i];
        if (!g)
            continue;
        GradientInstance *gi = (GradientInstance *)calloc(1, sizeof(GradientInstance));
        gradient_init(area, g, gi);
        area->gradient_instances_by_state[i] = g_list_append(area->gradient_instances_by_state[i], gi);
    }
}

void area_gradients_free(Area *area)
{
    if (debug_gradients)
        fprintf(stderr, "tint2: Freeing gradients for area %s\n", area->name);
    for (int i = 0; i < MOUSE_STATE_COUNT; i++) {
        for (GList *l = area->gradient_instances_by_state[i]; l; l = l->next) {
            gradient_destroy(l->data);
            free(l->data);
        }
        g_list_free(area->gradient_instances_by_state[i]);
        area->gradient_instances_by_state[i] = NULL;
    }
    g_assert_null(area->dependent_gradients);
}

double compute_control_point_offset(Area *area, Offset *offset)
{
    if (CONST_OFFSET(offset))
        return offset->constant_value;

    double width  = area->width,
           height = area->height;

    switch (offset->variable) {
    case SIZE_WIDTH:   return offset->multiplier * width;
    case SIZE_HEIGHT:  return offset->multiplier * height;
    case SIZE_RADIUS:  return offset->multiplier * sqrt(width * width + height * height) / 2;
    case SIZE_LEFT:    return offset->multiplier * 0;
    case SIZE_RIGHT:   return offset->multiplier * (width);
    case SIZE_TOP:     return offset->multiplier * 0;
    case SIZE_BOTTOM:  return offset->multiplier * (height);
    case SIZE_CENTERX: return offset->multiplier * (0.5 * width);
    case SIZE_CENTERY: return offset->multiplier * (0.5 * height);
    default:
        g_assert_not_reached();
        return 0;
    }
}

void compute_control_point(GradientInstance *gi, ControlPoint *control, double *x, double *y, double *r)
{
    *x = control->offsets_x ? compute_control_point_offset(gi->area, control->offsets_x) : 0;
    *y = control->offsets_y ? compute_control_point_offset(gi->area, control->offsets_y) : 0;
    *r = control->offsets_r ? compute_control_point_offset(gi->area, control->offsets_r) : 0;
}

void update_gradient(GradientInstance *gi)
{
    if (gi->pattern)
        return;

    double from_x, from_y, from_r,
           to_x,   to_y,   to_r;
    compute_control_point(gi, &gi->gradient_class->from, &from_x, &from_y, &from_r);
    compute_control_point(gi, &gi->gradient_class->to,   &to_x,   &to_y,   &to_r);
    
    switch (gi->gradient_class->type) {
    case GRADIENT_VERTICAL:
    case GRADIENT_HORIZONTAL:
        gi->pattern = cairo_pattern_create_linear(from_x, from_y, to_x, to_y);
        if (debug_gradients)
            fprintf(stderr,
                    "Creating linear gradient for area %s: %f %f, %f %f\n",
                    gi->area->name,
                    from_x, from_y,
                    to_x,   to_y);
        break;
    case GRADIENT_CENTERED:
        gi->pattern = cairo_pattern_create_radial(from_x, from_y, from_r, to_x, to_y, to_r);
        if (debug_gradients)
            fprintf(stderr,
                    "Creating radial gradient for area %s: %f %f %f, %f %f %f\n",
                    gi->area->name,
                    from_x, from_y, from_r,
                    to_x,   to_y,   to_r);
        break;
    default:
        g_assert_not_reached();
    }
    if (debug_gradients)
        fprintf(stderr,
                "Adding color stop at offset %f: %f %f %f %f\n",
                0.0,
                gi->gradient_class->start_color.rgb[0],
                gi->gradient_class->start_color.rgb[1],
                gi->gradient_class->start_color.rgb[2],
                gi->gradient_class->start_color.alpha);
    cairo_pattern_add_color_stop_rgba(gi->pattern,
                                      0,
                                      gi->gradient_class->start_color.rgb[0],
                                      gi->gradient_class->start_color.rgb[1],
                                      gi->gradient_class->start_color.rgb[2],
                                      gi->gradient_class->start_color.alpha);
    for (GList *l = gi->gradient_class->extra_color_stops; l; l = l->next) {
        ColorStop *color_stop = (ColorStop *)l->data;
        if (debug_gradients)
            fprintf(stderr,
                    "Adding color stop at offset %f: %f %f %f %f\n",
                    color_stop->offset,
                    color_stop->color.rgb[0],
                    color_stop->color.rgb[1],
                    color_stop->color.rgb[2],
                    color_stop->color.alpha);
        cairo_pattern_add_color_stop_rgba(gi->pattern,
                                          color_stop->offset,
                                          color_stop->color.rgb[0],
                                          color_stop->color.rgb[1],
                                          color_stop->color.rgb[2],
                                          color_stop->color.alpha);
    }
    if (debug_gradients)
        fprintf(stderr,
                "Adding color stop at offset %f: %f %f %f %f\n",
                1.0,
                gi->gradient_class->end_color.rgb[0],
                gi->gradient_class->end_color.rgb[1],
                gi->gradient_class->end_color.rgb[2],
                gi->gradient_class->end_color.alpha);
    cairo_pattern_add_color_stop_rgba(gi->pattern,
                                      1.0,
                                      gi->gradient_class->end_color.rgb[0],
                                      gi->gradient_class->end_color.rgb[1],
                                      gi->gradient_class->end_color.rgb[2],
                                      gi->gradient_class->end_color.alpha);
}
