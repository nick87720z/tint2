#include "button.h"

#include <string.h>
#include <stdio.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <math.h>
#include <pango/pangocairo.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>

#include "window.h"
#include "server.h"
#include "panel.h"
#include "timer.h"
#include "common.h"

char *button_get_tooltip(void *obj);
void button_init_fonts();
int button_compute_desired_size(void *obj);
void button_dump_geometry(void *obj, int indent);

void default_button()
{
}

Button *create_button()
{
    Button *button = calloc(1, sizeof(Button) + sizeof(ButtonBackend));
    ButtonBackend *backend = button->backend = (gpointer)(button + 1);
    
    backend->centered = TRUE;
    backend->font_color.alpha = 0.5;
    return button;
}

gpointer create_button_frontend(gconstpointer arg, gpointer data)
{
    Button *button_backend = (Button *)arg;

    Button *button_frontend = calloc(1, sizeof(Button) + sizeof(ButtonFrontend));
    button_frontend->frontend = (gpointer)(button_frontend + 1);
    ButtonBackend *backend = button_frontend->backend = button_backend->backend;
    backend->instances = g_list_append(backend->instances, button_frontend);
    return button_frontend;
}

void destroy_button(void *obj)
{
    Button *button = obj;
    ButtonBackend *backend = button->backend;
    ButtonFrontend *frontend = button->frontend;
    int size;
    if (frontend) {
        // This is a frontend element
        free_icon(frontend->icon);
        free_icon(frontend->icon_hover);
        free_icon(frontend->icon_pressed);
        backend->instances = g_list_remove_all(backend->instances, button);
        remove_area(&button->area);
        free_area(&button->area);
        size = sizeof(Button) + sizeof(ButtonFrontend);
    } else {
        // This is a backend element
        pango_font_description_free(backend->font_desc);
        free(backend->text);
        free(backend->icon_name);
        free(backend->tooltip);
        free(backend->lclick_command);
        free(backend->mclick_command);
        free(backend->rclick_command);
        free(backend->dwheel_command);
        free(backend->uwheel_command);
        size = sizeof(Button) + sizeof(ButtonBackend);

        if (backend->instances) {
            fprintf(stderr, "tint2: Error: Attempt to destroy backend while there are still frontend instances!\n");
            exit(EXIT_FAILURE);
        }
        button->backend = NULL;
    }
    memset(button, 0, size);
    free(button);
}

void init_button()
{
    GList *to_remove = panel_config.button_list;
    for (int k = 0; k < strlen(panel_items_order) && to_remove; k++) {
        if (panel_items_order[k] == 'P') {
            to_remove = to_remove->next;
        }
    }

    if (to_remove) {
        if (to_remove == panel_config.button_list) {
            g_list_free_full(to_remove, destroy_button);
            panel_config.button_list = NULL;
        } else {
            // Cut panel_config.button_list
            if (to_remove->prev)
                to_remove->prev->next = NULL;
            to_remove->prev = NULL;
            // Remove all elements of to_remove and to_remove itself
            g_list_free_full(to_remove, destroy_button);
        }
    }

    button_init_fonts();
    for (GList *l = panel_config.button_list; l; l = l->next) {
        Button *button = l->data;
        ButtonBackend *backend = button->backend;

        // Set missing config options
        if (!backend->bg)
            backend->bg = &g_array_index(backgrounds, Background, 0);
    }
}

void init_button_panel(void *p)
{
    Panel *panel = p;

    // Make sure this is only done once if there are multiple items
    if (panel->button_list && ((Button *)panel->button_list->data)->frontend)
        return;

    // panel->button_list is now a copy of the pointer panel_config.button_list
    // We make it a deep copy
    panel->button_list = g_list_copy_deep(panel_config.button_list, create_button_frontend, NULL);

    load_icon_themes();

    for (GList *l = panel->button_list; l; l = l->next) {
        Button *button = l->data;
        ButtonBackend *backend = button->backend;
        Area *area = &button->area;
        
        area->bg = backend->bg;
        area->paddingx = backend->paddingx;
        area->paddingy = backend->paddingy;
        area->paddingxlr = backend->paddingxlr;
        area->parent = panel;
        area->panel = panel;
        area->_dump_geometry = button_dump_geometry;
        area->_compute_desired_size = button_compute_desired_size;
        snprintf(area->name, sizeof(area->name), "Button");
        area->_draw_foreground = draw_button;
        area->size_mode = LAYOUT_FIXED;
        area->_resize = resize_button;
        area->_get_tooltip_text = button_get_tooltip;
        area->_is_under_mouse = full_width_area_is_under_mouse;
        area->has_mouse_press_effect =
            panel_config.mouse_effects &&
            (area->has_mouse_over_effect = backend->lclick_command || backend->mclick_command ||
                                           backend->rclick_command || backend->uwheel_command ||
                                           backend->dwheel_command);

        area->resize_needed = TRUE;
        area->on_screen = TRUE;
        instantiate_area_gradients(area);

        button_reload_icon(button);
    }
}

void button_init_fonts()
{
    for (GList *l = panel_config.button_list; l; l = l->next) {
        ButtonBackend *backend = ((Button *)l->data)->backend;
        if (!backend->font_desc)
            backend->font_desc = pango_font_description_from_string(get_default_font());
    }
}

void button_default_font_changed()
{
    gboolean needs_update = FALSE;
    for (GList *l = panel_config.button_list; l; l = l->next) {
        ButtonBackend *backend = ((Button *)l->data)->backend;
        if (!backend->has_font) {
            pango_font_description_free(backend->font_desc);
            backend->font_desc = NULL;
            needs_update = TRUE;
        }
    }
    if (!needs_update)
        return;

    button_init_fonts();
    for (int i = 0; i < num_panels; i++) {
        for (GList *l = panels[i].button_list; l; l = l->next) {
            Button *button = l->data;
            Area *area = &button->area;

            if (!button->backend->has_font) {
                area->resize_needed = TRUE;
                schedule_redraw(area);
            }
        }
    }
    schedule_panel_redraw();
}

void button_reload_icon(Button *button)
{
    ButtonFrontend *frontend = button->frontend;
    char *icon_name = button->backend->icon_name;
    
    free_icon(frontend->icon);
    free_icon(frontend->icon_hover);
    free_icon(frontend->icon_pressed);
    frontend->icon = NULL;
    frontend->icon_hover = NULL;
    frontend->icon_pressed = NULL;

    frontend->icon_load_size = frontend->iconw;

    if (!icon_name)
        return;

    char *new_icon_path = get_icon_path(icon_theme_wrapper, icon_name, frontend->iconw, TRUE);
    if (new_icon_path)
        frontend->icon = load_image(new_icon_path, TRUE);
    free(new_icon_path);
    // On loading error, fallback to default
    if (!frontend->icon) {
        new_icon_path = get_icon_path(icon_theme_wrapper, DEFAULT_ICON, frontend->iconw, TRUE);
        if (new_icon_path)
            frontend->icon = load_image(new_icon_path, TRUE);
        free(new_icon_path);
    }
    Imlib_Image original = frontend->icon;
    frontend->icon = scale_icon(frontend->icon, frontend->iconw);
    free_icon(original);

    if (panel_config.mouse_effects) {
        frontend->icon_hover = adjust_icon(frontend->icon,
                                           panel_config.mouse_over_alpha,
                                           panel_config.mouse_over_saturation,
                                           panel_config.mouse_over_brightness);
        frontend->icon_pressed = adjust_icon(frontend->icon,
                                             panel_config.mouse_pressed_alpha,
                                             panel_config.mouse_pressed_saturation,
                                             panel_config.mouse_pressed_brightness);
    }
    schedule_redraw(&button->area);
}

void button_default_icon_theme_changed()
{
    for (int i = 0; i < num_panels; i++) {
        for (GList *l = panels[i].button_list; l; l = l->next) {
            button_reload_icon(l->data);
        }
    }
    schedule_panel_redraw();
}

void cleanup_button()
{
    // Cleanup frontends
    for (int i = 0; i < num_panels; i++) {
        g_list_free_full(panels[i].button_list, destroy_button);
        panels[i].button_list = NULL;
    }

    // Cleanup backends
    g_list_free_full(panel_config.button_list, destroy_button);
    panel_config.button_list = NULL;
}

int button_compute_desired_size(void *obj)
{
    Button *button = obj;
    ButtonBackend *backend = button->backend;
    Panel *panel = (Panel *)button->area.panel;
    Area *area = &button->area;
    
    int horiz_padding = (panel_horizontal ? area->paddingxlr : area->paddingy) * panel->scale;
    int vert_padding = (panel_horizontal ? area->paddingy : area->paddingxlr) * panel->scale;
    int interior_padding = area->paddingx * panel->scale;

    int icon_w, icon_h;
    if (backend->icon_name) {
        int max_icon_size = backend->max_icon_size;
        if (panel_horizontal)
            icon_h = icon_w = area->height - top_bottom_border_width(area) - 2 * vert_padding;
        else
            icon_h = icon_w = area->width - left_right_border_width(area) - 2 * horiz_padding;
        if (max_icon_size) {
            icon_w = MIN(icon_w, max_icon_size * panel->scale);
            icon_h = MIN(icon_h, max_icon_size * panel->scale);
        }
    } else {
        icon_h = icon_w = 0;
    }

    int txt_height, txt_width;
    if (backend->text) {
        if (panel_horizontal) {
            get_text_size2(backend->font_desc,
                           &txt_height,
                           &txt_width,
                           panel->area.height,
                           panel->area.width,
                           backend->text,
                           strlen(backend->text),
                           PANGO_WRAP_WORD_CHAR,
                           PANGO_ELLIPSIZE_NONE,
                           backend->centered ? PANGO_ALIGN_CENTER : PANGO_ALIGN_LEFT,
                           FALSE,
                           panel->scale);
        } else {
            int x1 = icon_w ? icon_w + interior_padding : 0;
            get_text_size2(backend->font_desc,
                           &txt_height,
                           &txt_width,
                           panel->area.height,
                           (!icon_w || x1 < 0 ? area->width : area->width - x1)
                                - 2 * horiz_padding - left_right_border_width(area),
                           backend->text,
                           strlen(backend->text),
                           PANGO_WRAP_WORD_CHAR,
                           PANGO_ELLIPSIZE_NONE,
                           backend->centered ? PANGO_ALIGN_CENTER : PANGO_ALIGN_LEFT,
                           FALSE,
                           panel->scale);
        }
    } else {
        txt_height = txt_width = 0;
    }

    if (panel_horizontal) {
        int new_size, x1, x2;
        new_size = !icon_w ? txt_width : !txt_width ? icon_w :
                    (
                        // Get bounding range, including both icon and text
                        x1 = icon_w + interior_padding,
                        x2 = x1 + txt_width,
                        MAX(x2, icon_w) - MIN(x1, 0)
                    );
        new_size += 2 * horiz_padding + left_right_border_width(area);
        return new_size;
    } else {
        int new_size;
        new_size = txt_height + 2 * vert_padding + top_bottom_border_width(area);
        new_size = MAX(new_size, icon_h + 2 * vert_padding + top_bottom_border_width(area));
        return new_size;
    }
}

gboolean resize_button(void *obj)
{
    Button *button = obj;
    ButtonBackend *backend = button->backend;
    ButtonFrontend *frontend = button->frontend;
    Panel *panel = (Panel *)button->area.panel;
    Area *area = &button->area;
    
    int horiz_padding = (panel_horizontal ? area->paddingxlr : area->paddingy) * panel->scale;
    int vert_padding = (panel_horizontal ? area->paddingy : area->paddingxlr) * panel->scale;
    int interior_padding = area->paddingx * panel->scale;

    int icon_w, icon_h;
    if (backend->icon_name) {
        if (panel_horizontal)
            icon_h = icon_w = area->height - top_bottom_border_width(area) - 2 * vert_padding;
        else
            icon_h = icon_w = area->width - left_right_border_width(area) - 2 * horiz_padding;
        if (backend->max_icon_size) {
            icon_w = MIN(icon_w, backend->max_icon_size * panel->scale);
            icon_h = MIN(icon_h, backend->max_icon_size * panel->scale);
        }
    } else {
        icon_h = icon_w = 0;
    }

    frontend->iconw = icon_w;
    frontend->iconh = icon_h;
    if (frontend->icon_load_size != frontend->iconw)
        button_reload_icon(button);

    int available_w, available_h;
    if (panel_horizontal) {
        available_w = panel->area.width;
        available_h = area->height - 2 * area->paddingy - left_right_border_width(area);
    } else {
        int x1 = icon_w ? icon_w + interior_padding : 0;
        available_w = (!icon_w || x1 < 0 ? area->width : area->width - x1)
                        - 2 * horiz_padding - left_right_border_width(area);
        available_h = panel->area.height;
    }

    int txt_height, txt_width;
    if (backend->text) {
        get_text_size2(backend->font_desc,
                       &txt_height,
                       &txt_width,
                       available_h,
                       available_w,
                       backend->text,
                       strlen(backend->text),
                       PANGO_WRAP_WORD_CHAR,
                       PANGO_ELLIPSIZE_NONE,
                       backend->centered ? PANGO_ALIGN_CENTER : PANGO_ALIGN_LEFT,
                       FALSE,
                       panel->scale);
    } else {
        txt_height = txt_width = 0;
    }

    gboolean result = FALSE;
    if (panel_horizontal) {
        int new_size, x1, x2;
        new_size = !icon_w ? txt_width : !txt_width ? icon_w :
                    (
                        // Get bounding range, including both icon and text
                        x1 = icon_w + interior_padding,
                        x2 = x1 + txt_width,
                        MAX(x2, icon_w) - MIN(x1, 0)
                    );
        new_size += 2 * horiz_padding + left_right_border_width(area);
        if (new_size != area->width) {
            area->width = new_size;
            result = TRUE;
        }
    } else {
        int new_size;
        new_size = txt_height + 2 * vert_padding + top_bottom_border_width(area);
        new_size = MAX(new_size, icon_h + 2 * vert_padding + top_bottom_border_width(area));
        if (new_size != area->height) {
            area->height = new_size;
            result = TRUE;
        }
    }
    frontend->textw = txt_width;
    frontend->texth = txt_height;
    if (backend->centered) {
        if (icon_w) {
            int dx, pad, ix, tx;
            dx = icon_w + interior_padding;
            ix = dx < 0 ? -dx : 0;
            tx = ix + dx;
            pad = (area->width - MAX(ix + icon_w, tx + txt_width)) / 2;
            frontend->icony = (area->height - icon_h) / 2;
            frontend->iconx = ix + pad;
            frontend->texty = (area->height - txt_height) / 2;
            frontend->textx = tx + pad;
        } else {
            frontend->texty = (area->height - txt_height) / 2;
            frontend->textx = (area->width - txt_width) / 2;
        }
    } else {
        if (icon_w) {
            int dx = icon_w + interior_padding;
            frontend->icony = (area->height - icon_h) / 2;
            frontend->iconx = (dx < 0 ? -dx : 0) + left_border_width(area) + horiz_padding;
            frontend->texty = (area->height - txt_height) / 2;
            frontend->textx = frontend->iconx + dx;
        } else {
            frontend->texty = (area->height - txt_height) / 2;
            frontend->textx = left_border_width(area) + horiz_padding;
        }
    }

    schedule_redraw(area);

    return result;
}

void draw_button(void *obj, cairo_t *c)
{
    Button *button = obj;
    ButtonBackend *backend = button->backend;
    ButtonFrontend *frontend = button->frontend;
    Panel *panel = (Panel *)button->area.panel;
    Area *area = &button->area;

    if (frontend->icon) {
        // Render icon
        Imlib_Image image;
        if (panel_config.mouse_effects) {
            if (area->mouse_state == MOUSE_OVER)
                image = frontend->icon_hover ? frontend->icon_hover : frontend->icon;
            else if (area->mouse_state == MOUSE_DOWN)
                image = frontend->icon_pressed ? frontend->icon_pressed : frontend->icon;
            else
                image = frontend->icon;
        } else {
            image = frontend->icon;
        }

        imlib_context_set_image(image);
        render_image(area->pix, frontend->iconx, frontend->icony);
    }

    // Render text
    if (backend->text) {
        PangoContext *context = pango_cairo_create_context(c);
        pango_cairo_context_set_resolution(context, 96 * panel->scale);
        PangoLayout *layout = pango_layout_new(context);

        pango_layout_set_font_description(layout, backend->font_desc);
        pango_layout_set_width(layout, (frontend->textw + TINT2_PANGO_SLACK) * PANGO_SCALE);
        pango_layout_set_alignment(layout, backend->centered ? PANGO_ALIGN_CENTER : PANGO_ALIGN_LEFT);
        pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
        pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);
        pango_layout_set_text(layout, backend->text, strlen(backend->text));

        pango_cairo_update_layout(c, layout);
        draw_text(layout,
                  c,
                  frontend->textx,
                  frontend->texty,
                  &backend->font_color,
                  panel_config.font_shadow ? layout : NULL);

        g_object_unref(layout);
        g_object_unref(context);
    }
}

void button_dump_geometry(void *obj, int indent)
{
    Button *button = obj;
    ButtonBackend *backend = button->backend;
    ButtonFrontend *frontend = button->frontend;

    if (frontend->icon) {
        Imlib_Image tmp = imlib_context_get_image();
        imlib_context_set_image(frontend->icon);
        fprintf(stderr,
                "tint2: %*sIcon: x = %d, y = %d, w = %d, h = %d\n",
                indent,
                "",
                frontend->iconx,
                frontend->icony,
                imlib_image_get_width(),
                imlib_image_get_height());
        if (tmp)
            imlib_context_set_image(tmp);
    }
    fprintf(stderr,
            "tint2: %*sText: x = %d, y = %d, w = %d, align = %s, text = %s\n",
            indent,
            "",
            frontend->textx,
            frontend->texty,
            frontend->textw,
            backend->centered ? "center" : "left",
            backend->text);
}

void button_action(void *obj, int mouse_button, int x, int y, Time time)
{
    Button *button = (Button *)obj;
    ButtonBackend *backend = button->backend;
    char *command = NULL;
    switch (mouse_button) {
        #define BUTTON_CASE(i,c) case i: \
                                    command = backend->c; \
                                    break
        BUTTON_CASE(1, lclick_command);
        BUTTON_CASE(2, mclick_command);
        BUTTON_CASE(3, rclick_command);
        BUTTON_CASE(4, uwheel_command);
        BUTTON_CASE(5, dwheel_command);
        #undef BUTTON_CASE
    }
    tint_exec(command, NULL, NULL, time, obj, x, y, FALSE, TRUE);
}

char *button_get_tooltip(void *obj)
{
    Button *button = obj;
    ButtonBackend *backend = button->backend;

    if (backend->tooltip && strlen(backend->tooltip) > 0)
        return strdup(backend->tooltip);
    return NULL;
}
