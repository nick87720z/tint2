#include "execplugin.h"

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
#include "tooltip.h"
#include "timer.h"
#include "common.h"

#define MAX_TOOLTIP_LEN 4096

bool debug_executors = false;

void execp_timer_callback(void *arg);
char *execp_get_tooltip(void *obj);
void execp_init_fonts();
int execp_compute_desired_size(void *obj);
void execp_dump_geometry(void *obj, int indent);

void default_execp()
{
}

Execp *create_execp()
{
    Execp *execp = (Execp *)calloc(1, sizeof(Execp) + sizeof(ExecpBackend));
    ExecpBackend *backend = execp->backend = (gpointer)(execp + 1);
    
    backend->child_pipe_stdout = -1;
    backend->child_pipe_stderr = -1;
    backend->cmd_pids = g_tree_new(cmp_ptr);
    backend->interval = 30;
    backend->icon_path = NULL;
    backend->cache_icon = TRUE;
    backend->centered = TRUE;
    backend->font_color.alpha = 0.5;
    backend->monitor = -1;
    INIT_TIMER(backend->timer);
    backend->bg = &g_array_index(backgrounds, Background, 0);
    backend->buf_stdout_capacity = 1024;
    backend->buf_stderr_capacity = 1024;
    backend->buf_stdout = calloc(backend->buf_stdout_capacity, 1);
    backend->buf_stderr = calloc(backend->buf_stderr_capacity, 1);
    backend->text = strdup("");
    return execp;
}

gpointer create_execp_frontend(gconstpointer arg, gpointer data)
{
    Execp *execp_backend = (Execp *)arg;
    ExecpBackend *backend = execp_backend->backend;
    Panel *panel = data;
    
    if (backend->monitor >= 0 &&
        panel->monitor != backend->monitor)
    {
        printf("Skipping executor '%s' with monitor %d for panel on monitor %d\n",
               backend->command,
               backend->monitor, panel->monitor);
        
        Execp *dummy = create_execp();
        dummy->frontend = (ExecpFrontend *)calloc(1, sizeof(ExecpFrontend));
        dummy->backend->instances = g_list_append(dummy->backend->instances, dummy);
        dummy->dummy = true;
        return dummy;
    }
    printf("Creating executor '%s' with monitor %d for panel on monitor %d\n",
           backend->command,
           backend->monitor, panel->monitor);

    Execp *execp_frontend = (Execp *)calloc(1, sizeof(Execp) + sizeof(ExecpFrontend));
    execp_frontend->frontend = (gpointer)(execp_frontend + 1);
    execp_frontend->backend = backend;
    backend->instances = g_list_append(backend->instances, execp_frontend);
    return execp_frontend;
}

void destroy_execp(void *obj)
{
    Execp *execp = (Execp *)obj;
    ExecpBackend *backend = execp->backend;
    
    if (execp->frontend) {
        // This is a frontend element
        backend->instances = g_list_remove_all(backend->instances, execp);
        remove_area(&execp->area);
        free_area(&execp->area);
        if (! execp->dummy) {
            goto final_free;
        }
        free_and_null(execp->frontend);
    }
    // This is a backend element
    destroy_timer(&backend->timer);
    if (backend->child) {
        kill(-backend->child, SIGHUP);
    }
    if (backend->child_pipe_stdout >= 0) {
        close(backend->child_pipe_stdout);
    }
    if (backend->child_pipe_stderr >= 0) {
        close(backend->child_pipe_stderr);
    }
    if (backend->cmd_pids) {
        g_tree_destroy(backend->cmd_pids);
    }
    pango_font_description_free(backend->font_desc);
    
    free(backend->buf_stdout);
    free(backend->buf_stderr);
    free(backend->text);
    free(backend->icon_path);
    free(backend->command);
    free(backend->tooltip);
    free(backend->lclick_command);
    free(backend->mclick_command);
    free(backend->rclick_command);
    free(backend->dwheel_command);
    free(backend->uwheel_command);
    memset (backend, 0, sizeof(*backend));

    if (backend->instances) {
        fprintf(stderr, "tint2: Error: Attempt to destroy backend while there are still frontend instances!\n");
        exit(EXIT_FAILURE);
    }
    
final_free:
    free(execp);
}

void init_execp()
{
    GList *to_remove = panel_config.execp_list;
    for (int k = 0, len = strlen(panel_items_order); k < len && to_remove; k++) {
        if (panel_items_order[k] == 'E') {
            to_remove = to_remove->next;
        }
    }

    if (to_remove) {
        if (to_remove == panel_config.execp_list) {
            g_list_free_full(to_remove, destroy_execp);
            panel_config.execp_list = NULL;
        } else {
            // Cut panel_config.execp_list
            if (to_remove->prev)
                to_remove->prev->next = NULL;
            to_remove->prev = NULL;
            // Remove all elements of to_remove and to_remove itself
            g_list_free_full(to_remove, destroy_execp);
        }
    }

    execp_init_fonts();
    for (GList *l = panel_config.execp_list; l; l = l->next) {
        Execp *execp = l->data;

        // Set missing config options
        if (!execp->backend->bg)
            execp->backend->bg = &g_array_index(backgrounds, Background, 0);
    }
}

void init_execp_panel(void *p)
{
    Panel *panel = (Panel *)p;

    // Make sure this is only done once if there are multiple items
    if (panel->execp_list && ((Execp *)panel->execp_list->data)->frontend)
        return;

    // panel->execp_list is now a copy of the pointer panel_config.execp_list
    // We make it a deep copy
    panel->execp_list = g_list_copy_deep(panel_config.execp_list, create_execp_frontend, panel);

    for (GList *l = panel->execp_list; l; l = l->next) {
        Execp *execp = l->data;
        ExecpBackend * backend = execp->backend;
        
        execp->area.bg = backend->bg;
        execp->area.paddingx = backend->paddingx;
        execp->area.paddingy = backend->paddingy;
        execp->area.paddingxlr = backend->paddingxlr;
        execp->area.parent = panel;
        execp->area.panel = panel;
        execp->area._dump_geometry = execp_dump_geometry;
        execp->area._compute_desired_size = execp_compute_desired_size;
        snprintf(execp->area.name,
                 sizeof(execp->area.name),
                 "Execp %s",
                 backend->command ? backend->command : "null");
        execp->area._draw_foreground = draw_execp;
        execp->area.size_mode = LAYOUT_FIXED;
        execp->area._resize = resize_execp;
        execp->area._get_tooltip_text = execp_get_tooltip;
        execp->area._is_under_mouse = full_width_area_is_under_mouse;
        execp->area.has_mouse_press_effect =
            panel_config.mouse_effects &&
            (execp->area.has_mouse_over_effect = backend->lclick_command || backend->mclick_command ||
                                                 backend->rclick_command || backend->uwheel_command ||
                                                 backend->dwheel_command);

        execp->area.resize_needed = TRUE;
        execp->area.on_screen = TRUE;
        instantiate_area_gradients(&execp->area);

        change_timer(&backend->timer, true, 10, 0, execp_timer_callback, execp);

        execp_update_post_read(execp);
    }
}

void execp_init_fonts()
{
    for (GList *l = panel_config.execp_list; l; l = l->next) {
        Execp *execp = l->data;
        if (!execp->backend->font_desc)
            // TODO: uniq & shared default font description
            execp->backend->font_desc = pango_font_description_from_string(get_default_font());
    }
}

void execp_default_font_changed()
{
    gboolean needs_update = FALSE;
    for (GList *l = panel_config.execp_list; l; l = l->next) {
        Execp *execp = l->data;
        ExecpBackend * backend = execp->backend;

        if (!backend->has_font) {
            pango_font_description_free(backend->font_desc);
            backend->font_desc = NULL;
            needs_update = TRUE;
        }
    }
    if (!needs_update)
        return;

    execp_init_fonts();
    for (int i = 0; i < num_panels; i++) {
        for (GList *l = panels[i].execp_list; l; l = l->next) {
            Execp *execp = l->data;

            if (!execp->backend->has_font) {
                execp->area.resize_needed = TRUE;
                schedule_redraw(&execp->area);
            }
        }
    }
    schedule_panel_redraw();
}

void cleanup_execp()
{
    #define CLEANUP_PANEL_EXECP(p) do { \
        g_list_free_full((p).execp_list, destroy_execp); \
        (p).execp_list = NULL;                           \
    } while(0)

    // Cleanup frontends
    for (int i = 0; i < num_panels; i++)
        CLEANUP_PANEL_EXECP (panels[i]);

    // Cleanup backends
    CLEANUP_PANEL_EXECP (panel_config);

    #undef CLEANUP_PANEL_EXECP
}

// Called from backend functions.
gboolean reload_icon(Execp *execp)
{
    ExecpBackend * backend = execp->backend;
    char *icon_path = backend->icon_path;

    if (backend->has_icon && icon_path) {
        if (backend->icon) {
            imlib_context_set_image(backend->icon);
            imlib_free_image();
        }
        backend->icon = load_image(icon_path, backend->cache_icon);
        if (backend->icon) {
            int w, h, ow, oh;
            imlib_context_set_image(backend->icon);
            ow = w = imlib_image_get_width();
            oh = h = imlib_image_get_height();
            if (w && h) {
                if (backend->icon_w) {
                    if (backend->icon_h) {
                        w = backend->icon_w;
                        h = backend->icon_h;
                    } else {
                        h = (int)(0.5 + h * backend->icon_w / (float)w);
                        w = backend->icon_w;
                    }
                } else {
                    if (backend->icon_h) {
                        w = (int)(0.5 + w * backend->icon_h / (float)h);
                        h = backend->icon_h;
                    }
                }
                if (w < 1)
                    w = 1;
                if (h < 1)
                    h = 1;
            }
            if (w != ow || h != oh) {
                backend->icon = imlib_create_cropped_scaled_image(0, 0, ow, oh, w, h);
                imlib_free_image();
            }
            return TRUE;
        }
    }
    return FALSE;
}

void execp_compute_icon_text_geometry(Execp *execp,
                                      int *horiz_padding,
                                      int *vert_padding,
                                      int *interior_padding,
                                      int *icon_w,
                                      int *icon_h,
                                      gboolean *text_next_line,
                                      int *txt_height,
                                      int *txt_width,
                                      int *new_size,
                                      gboolean *resized)
{
    int _icon_w, _icon_h, _vpad, _hpad, _inpad, _txt_width, _txt_height, _new_size, border_lr;
    gboolean _text_next_line, _resized;
    ExecpBackend * backend = execp->backend;
    Panel *panel = (Panel *)execp->area.panel;
    Area *area = &execp->area;

    _hpad = (panel_horizontal ? area->paddingxlr : area->paddingy) * panel->scale;
    _vpad = (panel_horizontal ? area->paddingy : area->paddingxlr) * panel->scale;
    _inpad = area->paddingx * panel->scale;

    if (backend->icon && reload_icon(execp)) {
        imlib_context_set_image(backend->icon);
        _icon_w = imlib_image_get_width();
        _icon_h = imlib_image_get_height();
    } else
        _icon_w = _icon_h = 0;

    _text_next_line = !panel_horizontal && _icon_w > area->width / 2;
    border_lr = left_right_border_width(area);

    {int w_avail, h_avail;
        if (panel_horizontal) {
            w_avail = panel->area.width;
            h_avail = area->height - 2 * _vpad - border_lr;
        } else {
            w_avail = area->width  - 2 * _hpad - border_lr;
            h_avail = panel->area.height;

            if (_icon_w && !_text_next_line)
                w_avail -= _icon_w + _inpad;
        }
        get_text_size2(backend->font_desc,
                       &_txt_height,
                       &_txt_width,
                       h_avail,
                       w_avail,
                       backend->text,
                       strlen(backend->text),
                       PANGO_WRAP_WORD_CHAR,
                       PANGO_ELLIPSIZE_NONE,
                       backend->centered ? PANGO_ALIGN_CENTER : PANGO_ALIGN_LEFT,
                       backend->has_markup,
                       panel->scale);
    }

    if (panel_horizontal) {
        _new_size = _txt_width + 2 * _hpad + border_lr;

        if (_icon_w)
            _new_size += _inpad + _icon_w;

        if ((_resized = _new_size < area->width) && area->width - _new_size < 6)
            // we try to limit the number of resizes
            _new_size = area->width;
        else
            _resized = _new_size != area->width;
    } else {
        _new_size = 2 * _vpad + top_bottom_border_width(area);

        if (!_text_next_line)
            _new_size += MAX(_txt_height, _icon_h);
        else {
            _new_size += _icon_h;

            if (strlen(backend->text))
                _new_size += _inpad + _txt_height;
        }
        _resized = _new_size != area->height;
    }
    *icon_w = _icon_w ,
    *icon_h = _icon_h ,
    *vert_padding  = _vpad ,
    *horiz_padding = _hpad ,
    *interior_padding = _inpad,
    *txt_width = _txt_width ,
    *txt_height = _txt_height ,
    *new_size = _new_size,
    *text_next_line = _text_next_line,
    *resized = _resized;
}

int execp_compute_desired_size(void *obj)
{
    Execp *execp = (Execp *)obj;
    int horiz_padding, vert_padding, interior_padding;
    int icon_w, icon_h;
    gboolean text_next_line;
    int txt_height, txt_width;
    int new_size;
    gboolean resized;
    execp_compute_icon_text_geometry(execp,
                                     &horiz_padding,
                                     &vert_padding,
                                     &interior_padding,
                                     &icon_w,
                                     &icon_h,
                                     &text_next_line,
                                     &txt_height,
                                     &txt_width,
                                     &new_size,
                                     &resized);

    return new_size;
}

gboolean resize_execp(void *obj)
{
    Execp *execp = (Execp *)obj;
    ExecpBackend * backend = execp->backend;
    ExecpFrontend * frontend = execp->frontend;
    
    int horiz_padding, vert_padding, interior_padding;
    int icon_w, icon_h;
    gboolean text_next_line;
    int txt_height, txt_width;
    int new_size;
    gboolean resized;
    execp_compute_icon_text_geometry(execp,
                                     &horiz_padding,
                                     &vert_padding,
                                     &interior_padding,
                                     &icon_w,
                                     &icon_h,
                                     &text_next_line,
                                     &txt_height,
                                     &txt_width,
                                     &new_size,
                                     &resized);

    if (panel_horizontal)
        execp->area.width = new_size;
    else
        execp->area.height = new_size;

    frontend->textw = txt_width;
    frontend->texth = txt_height;
    if (backend->centered) {
        if (icon_w) {
            if (!text_next_line) {
                frontend->icony = (execp->area.height - icon_h) / 2;
                frontend->iconx = (execp->area.width - txt_width - interior_padding - icon_w) / 2;
                frontend->texty = (execp->area.height - txt_height) / 2;
                frontend->textx = frontend->iconx + icon_w + interior_padding;
            } else {
                if (strlen(backend->text)) {
                    frontend->icony = (execp->area.height - icon_h - interior_padding - txt_height) / 2;
                    frontend->iconx = (execp->area.width - icon_w) / 2;
                    frontend->texty = frontend->icony + icon_h + interior_padding;
                    frontend->textx = (execp->area.width - txt_width) / 2;
                } else {
                    frontend->icony = (execp->area.height - icon_h) / 2;
                    frontend->iconx = (execp->area.width - icon_w) / 2;
                    frontend->texty = frontend->icony + icon_h + interior_padding;
                    frontend->textx = (execp->area.width - txt_width) / 2;
                }
            }
        } else {
            frontend->texty = (execp->area.height - txt_height) / 2;
            frontend->textx = (execp->area.width - txt_width) / 2;
        }
    } else {
        if (icon_w) {
            if (!text_next_line) {
                frontend->icony = (execp->area.height - icon_h) / 2;
                frontend->iconx = left_border_width(&execp->area) + horiz_padding;
                frontend->texty = (execp->area.height - txt_height) / 2;
                frontend->textx = frontend->iconx + icon_w + interior_padding;
            } else {
                if (strlen(backend->text)) {
                    frontend->icony = (execp->area.height - icon_h - interior_padding - txt_height) / 2;
                    frontend->iconx = left_border_width(&execp->area) + horiz_padding;
                    frontend->texty = frontend->icony + icon_h + interior_padding;
                    frontend->textx = frontend->iconx;
                } else {
                    frontend->icony = (execp->area.height - icon_h) / 2;
                    frontend->iconx = left_border_width(&execp->area) + horiz_padding;
                    frontend->texty = frontend->icony + icon_h + interior_padding;
                    frontend->textx = frontend->iconx;
                }
            }
        } else {
            frontend->texty = (execp->area.height - txt_height) / 2;
            frontend->textx = left_border_width(&execp->area) + horiz_padding;
        }
    }

    schedule_redraw(&execp->area);
    return resized;
}

PangoLayout *create_execp_text_layout(Execp *execp, PangoContext *context)
{
    ExecpBackend * backend = execp->backend;
    ExecpFrontend * frontend = execp->frontend;
    
    PangoLayout *layout = pango_layout_new(context);
    pango_layout_set_font_description(layout, backend->font_desc);
    pango_layout_set_width(layout, (frontend->textw + TINT2_PANGO_SLACK) * PANGO_SCALE);
    pango_layout_set_height(layout, (frontend->texth + TINT2_PANGO_SLACK) * PANGO_SCALE);
    pango_layout_set_alignment(layout, backend->centered ? PANGO_ALIGN_CENTER : PANGO_ALIGN_LEFT);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);
    return layout;
}

void draw_execp(void *obj, cairo_t *c)
{
    Execp *execp = (Execp *)obj;
    Panel *panel = (Panel *)execp->area.panel;
    ExecpBackend * backend = execp->backend;
    ExecpFrontend * frontend = execp->frontend;

    PangoContext *context = pango_cairo_create_context(c);
    pango_cairo_context_set_resolution(context, 96 * panel->scale);
    PangoLayout *layout = create_execp_text_layout(execp, context);
    PangoLayout *shadow_layout = NULL;

    if (backend->has_icon && backend->icon) {
        imlib_context_set_image(backend->icon);
        // Render icon
        render_image(execp->area.pix, frontend->iconx, frontend->icony);
    }

    // draw layout
    if (!backend->has_markup) {
        pango_layout_set_text(layout, backend->text, strlen(backend->text));
    } else {
        pango_layout_set_markup(layout, backend->text, strlen(backend->text));
        if (panel_config.font_shadow) {
            shadow_layout = create_execp_text_layout(execp, context);
            if (!layout_set_markup_strip_colors(shadow_layout, backend->text)) {
                g_object_unref(shadow_layout);
                shadow_layout = NULL;
            }
        }
    }

    pango_cairo_update_layout(c, layout);
    draw_text(layout,
              c,
              frontend->textx,
              frontend->texty,
              &backend->font_color,
              shadow_layout);

    g_object_unref(layout);
    g_object_unref(context);
}

void execp_dump_geometry(void *obj, int indent)
{
    Execp *execp = (Execp *)obj;
    ExecpBackend * backend = execp->backend;
    ExecpFrontend * frontend = execp->frontend;

    if (backend->has_icon && backend->icon) {
        Imlib_Image tmp = imlib_context_get_image();
        imlib_context_set_image(backend->icon);
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
            "tint2: %*sText: x = %d, y = %d, w = %d, h = %d, align = %s, text = %s\n",
            indent,
            "",
            frontend->textx,
            frontend->texty,
            frontend->textw,
            frontend->texth,
            backend->centered ? "center" : "left",
            backend->text);
}

void execp_force_update(Execp *execp)
{
    ExecpBackend * backend = execp->backend;
    if (backend->child_pipe_stdout > 0) {
        // Command currently running, nothing to do
    } else {
        // Run command right away
        change_timer(&backend->timer, true, 10, 0, execp_timer_callback, execp);
    }
}

void execp_action(void *obj, int button, int x, int y, Time time)
{
    Execp *execp = (Execp *)obj;
    ExecpBackend * backend = execp->backend;
    
    char *command = NULL;
    switch (button) {
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
    if (command) {
        setenvd("EXECP_X", x);
        setenvd("EXECP_Y", y);
        setenvd("EXECP_W", execp->area.width);
        setenvd("EXECP_H", execp->area.height);
        pid_t pid = tint_exec(command, NULL, NULL, time, obj, x, y, FALSE, TRUE);
        unsetenv("EXECP_X");
        unsetenv("EXECP_Y");
        unsetenv("EXECP_W");
        unsetenv("EXECP_H");
        if (pid > 0)
            g_tree_insert(backend->cmd_pids, GINT_TO_POINTER(pid), GINT_TO_POINTER(1));
    } else {
        execp_force_update(execp);
    }
}

void execp_cmd_completed(Execp *execp, pid_t pid)
{
    g_tree_remove(execp->backend->cmd_pids, GINT_TO_POINTER(pid));
    execp_force_update(execp);
}

void execp_timer_callback(void *arg)
{
    Execp *execp = (Execp *)arg;
    ExecpBackend * backend = execp->backend;

    if (!backend->command)
        return;

    // Still running!
    if (backend->child_pipe_stdout > 0)
        return;

    int pipe_fd_stdout[2];
    if (pipe(pipe_fd_stdout)) {
        // TODO maybe write this in tooltip, but if this happens we're screwed anyways
        fprintf(stderr, "tint2: Execp: Creating pipe failed!\n");
        return;
    }

    fcntl(pipe_fd_stdout[0], F_SETFL, O_NONBLOCK | fcntl(pipe_fd_stdout[0], F_GETFL));

    int pipe_fd_stderr[2];
    if (pipe(pipe_fd_stderr)) {
        close(pipe_fd_stdout[1]);
        close(pipe_fd_stdout[0]);
        // TODO maybe write this in tooltip, but if this happens we're screwed anyways
        fprintf(stderr, "tint2: Execp: Creating pipe failed!\n");
        return;
    }

    fcntl(pipe_fd_stderr[0], F_SETFL, O_NONBLOCK | fcntl(pipe_fd_stderr[0], F_GETFL));

    // Fork and run command, capturing stdout in pipe
    pid_t child = fork();
    if (child == -1) {
        // TODO maybe write this in tooltip, but if this happens we're screwed anyways
        fprintf(stderr, "tint2: Fork failed.\n");
        close(pipe_fd_stdout[1]);
        close(pipe_fd_stdout[0]);
        close(pipe_fd_stderr[1]);
        close(pipe_fd_stderr[0]);
        return;
    } else if (child == 0) {
        if (debug_executors)
            fprintf(stderr, "tint2: Executing: %s\n", backend->command);
        // We are in the child
        close(pipe_fd_stdout[0]);
        dup2(pipe_fd_stdout[1], 1); // 1 is stdout
        close(pipe_fd_stdout[1]);
        close(pipe_fd_stderr[0]);
        dup2(pipe_fd_stderr[1], 2); // 2 is stderr
        close(pipe_fd_stderr[1]);
        close_all_fds();
        setpgid(0, 0);
        execl("/bin/sh", "/bin/sh", "-c", backend->command, NULL);
        // This should never happen!
        fprintf(stderr, "execl() failed\nexecl() failed\n");
        exit(0);
    }
    close(pipe_fd_stdout[1]);
    close(pipe_fd_stderr[1]);
    backend->child = child;
    backend->child_pipe_stdout = pipe_fd_stdout[0];
    backend->child_pipe_stderr = pipe_fd_stderr[0];
    backend->buf_stdout[backend->buf_stdout_length = 0] = '\0';
    backend->buf_stderr[backend->buf_stderr_length = 0] = '\0';
    backend->last_update_start_time = time(NULL);
}

int read_from_pipe(int fd, char **buffer, ssize_t *buffer_length, ssize_t *buffer_capacity, gboolean *eof)
{
    *eof = FALSE;
    ssize_t total = 0;
    while (1) {
        // Make sure there is free space in the buffer
        ssize_t req_cap = *buffer_length + 1024;
        ssize_t count;
        if (*buffer_capacity < req_cap) {
            do    *buffer_capacity *= 2;
            while (*buffer_capacity < req_cap);
            *buffer = (char *)realloc(*buffer, *buffer_capacity);
        }
        count = read(fd, *buffer + *buffer_length,
                         *buffer_capacity - *buffer_length - 1);
        if (count > 0) {
            // Successful read
            total += count;
            *buffer_length += count;
            (*buffer)[*buffer_length] = '\0';
            continue;
        } else if (count == 0) {
            // End of file
            *eof = TRUE;
            break;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No more data available at the moment
            break;
        } else if (errno == EINTR) {
            // Harmless interruption by signal
            continue;
        } else {
            // Error
            *eof = TRUE;
            break;
        }
        break;
    }
    return total;
}

#if 1
char *last_substring(char *s, char *sub)
// All in one function variant; separated one is in #else block.
// Switch to that variant when separated 'starts_with' is really necessary
// (perhaps, then it would go to common place)
{
    char *result = NULL,
         *p=s, *q;

    while (1) {
        q=sub;

        // search for first char
        for(;; p++) {
            if (!*p)
                return result;
            if (*p == *q)
                break;
        }
        s=p;
        
        // when found, check for full substring
        while (p++, q++, 1) {
            if (!*q) {
                result = s;
                break;
            }
            if (!*p)
                return result;

            if (*p != *q) {
                p = s += 1;
                break;
            }
        }
    }
}

#else
int starts_with(char *s, char *pref)
{
    for (char *p = s;; p++, pref++) {
        if (!*pref)
            return p - s;
        if (!*p || *p != *pref)
            return 0;
    }
}

char *last_substring(char *s, char *sub)
{
    char *result = NULL;
    int l;
    while (1)
    {
        for (;; s++) {
            if (!*s)
                return result;
            if (*s == *sub)
                break;
        }
        if ( (l = starts_with(s, sub)) ) {
            result = s;
            s += l;
        } else
            s++;
    }
    return result;
}
#endif

void rstrip(char *s)
{
    char *p = strchr(s, '\0') - 1;
    while ( p != s - 1 &&
            (*p == ' ' || *p == '\n')
    ) p--;
    p[1] = 0;
}

gboolean read_execp(void *obj)
{
    Execp *execp = (Execp *)obj;
    ExecpBackend * backend = execp->backend;

    if (backend->child_pipe_stdout < 0)
        return FALSE;

    gboolean stdout_eof, stderr_eof;
    int count;
    read_from_pipe(backend->child_pipe_stdout,
                   &backend->buf_stdout,
                   &backend->buf_stdout_length,
                   &backend->buf_stdout_capacity,
                   &stdout_eof);
    count =
    read_from_pipe(backend->child_pipe_stderr,
                   &backend->buf_stderr,
                   &backend->buf_stderr_length,
                   &backend->buf_stderr_capacity,
                   &stderr_eof);

    gboolean command_finished = stdout_eof && stderr_eof;
    gboolean result = FALSE;

    if (command_finished) {
        close(backend->child_pipe_stdout);
        close(backend->child_pipe_stderr);
        backend->child = 0;
        backend->child_pipe_stdout =
        backend->child_pipe_stderr = -1;
        if (backend->interval)
            change_timer(&backend->timer, true, backend->interval * 1000, 0, execp_timer_callback, execp);
    }

    char ansi_clear_screen[] = "\x1b[2J";
    if (backend->continuous) {
        // Handle stderr
        if (!backend->has_user_tooltip) {
            free_and_null(backend->tooltip);
            char *start = last_substring(backend->buf_stderr, ansi_clear_screen);
            if (start) {
                start += sizeof(ansi_clear_screen) - 1;
                memmove(backend->buf_stderr, start, strlen(start) + 1);
                backend->buf_stderr_length = (ssize_t)strlen(backend->buf_stderr);
            }
            if (backend->buf_stderr_length > MAX_TOOLTIP_LEN) {
                backend->buf_stderr_length = MAX_TOOLTIP_LEN;
                backend->buf_stderr[backend->buf_stderr_length] = '\0';
            }
            backend->tooltip = strdup(backend->buf_stderr);
            rstrip(backend->tooltip);
            if (count > 0)
                result = TRUE;
        } else {
            backend->buf_stderr_length = 0;
            backend->buf_stderr[backend->buf_stderr_length] = '\0';
        }
        
        // Handle stdout
        // Count lines in buffer
        int num_lines = 0;
        char *end = NULL;
        for (char *c = backend->buf_stdout; *c; c++) {
            if (*c == '\n') {
                num_lines++;
                if (num_lines == backend->continuous)
                    end = c;
            }
        }
        if (num_lines >= backend->continuous) {
            if (end)
                *end = '\0';
            free_and_null(backend->text);
            free_and_null(backend->icon_path);
            if (!backend->has_icon) {
                backend->text = strdup(backend->buf_stdout);
            } else {
                char *text = strchr(backend->buf_stdout, '\n');
                if (text) {
                    *text = '\0';
                    text++;
                    backend->text = strdup(text);
                } else {
                    backend->text = strdup("");
                }
                backend->icon_path = expand_tilde(backend->buf_stdout);
            }
            size_t len = strlen(backend->text);
            if (len > 0 && backend->text[len - 1] == '\n')
                backend->text[len - 1] = '\0';

            if (end) {
                char *next = end + 1;
                ssize_t copied = next - backend->buf_stdout;
                ssize_t remaining = backend->buf_stdout_length - copied;
                if (remaining > 0) {
                    memmove(backend->buf_stdout, next, (size_t)remaining);
                    backend->buf_stdout_length = remaining;
                    backend->buf_stdout[backend->buf_stdout_length] = '\0';
                } else {
                    backend->buf_stdout_length = 0;
                    backend->buf_stdout[backend->buf_stdout_length] = '\0';
                }
            }

            backend->last_update_finish_time = time(NULL);
            backend->last_update_duration =
                backend->last_update_finish_time - backend->last_update_start_time;
            result = TRUE;
        }
    } else if (command_finished) {
        // Handle stdout
        free_and_null(backend->text);
        free_and_null(backend->icon_path);
        if (!backend->has_icon) {
            backend->text = strdup(backend->buf_stdout);
        } else {
            char *text = strchr(backend->buf_stdout, '\n');
            if (text) {
                *text = '\0';
                text++;
                backend->text = strdup(text);
            } else {
                backend->text = strdup("");
            }
            backend->icon_path = strdup(backend->buf_stdout);
        }
        int len = strlen(backend->text);
        if (len > 0 && backend->text[len - 1] == '\n')
            backend->text[len - 1] = '\0';
        backend->buf_stdout_length = 0;
        backend->buf_stdout[backend->buf_stdout_length] = '\0';
        // Handle stderr
        if (!backend->has_user_tooltip) {
            free_and_null(backend->tooltip);
            char *start = last_substring(backend->buf_stderr, ansi_clear_screen);
            if (start)
                start += sizeof(ansi_clear_screen) - 1;
            else
                start = backend->buf_stderr;
            if (*start) {
                backend->tooltip = strdup(start);
                rstrip(backend->tooltip);
                if (strlen(backend->tooltip) > MAX_TOOLTIP_LEN)
                    backend->tooltip[MAX_TOOLTIP_LEN] = '\0';
            }
        }
        backend->buf_stderr_length = 0;
        backend->buf_stderr[backend->buf_stderr_length] = '\0';
        //
        backend->last_update_finish_time = time(NULL);
        backend->last_update_duration =
            backend->last_update_finish_time - backend->last_update_start_time;
        result = TRUE;
    }
    return result;
}

const char *time_to_string(int s, char *buffer, size_t buffer_size)
{
    if (s < 60) {
        snprintf(buffer, buffer_size, "%ds", s);
    } else if (s < 3600) {
        int m = s / 60;
        snprintf(buffer, buffer_size, "%d:%02ds", m, s % 60);
    } else {
        int h = s / 3600;
        int m = (s %= 3600) / 60;
        snprintf(buffer, buffer_size, "%d:%02d:%02ds", h, m, s % 60);
    }

    return buffer;
}

char *execp_get_tooltip(void *obj)
// FIXME: strdup each time? Needs refactoring.
{
    Execp *execp = (Execp *)obj;
    ExecpBackend * backend = execp->backend;

    if (backend->tooltip)
        return strlen(backend->tooltip) > 0 ? strdup(backend->tooltip) : NULL;

    time_t now = time(NULL);

    char tmp_buf1[256];
    char tmp_buf2[256];
    char tmp_buf3[256];
    if (backend->child_pipe_stdout < 0) {
        // Not executing command
        if (backend->last_update_finish_time) {
            // We updated at least once
            if (backend->interval > 0) {
                snprintf(backend->tooltip_text,
                         sizeof(backend->tooltip_text),
                         "Last update finished %s ago (took %s). Next update starting in %s.",
                         time_to_string((int)(now - backend->last_update_finish_time), tmp_buf1, sizeof(tmp_buf1)),
                         time_to_string((int)backend->last_update_duration, tmp_buf2, sizeof(tmp_buf2)),
                         time_to_string((int)(backend->interval - (now - backend->last_update_finish_time)),
                                        tmp_buf3, sizeof(tmp_buf3)));
            } else {
                snprintf(backend->tooltip_text,
                         sizeof(backend->tooltip_text),
                         "Last update finished %s ago (took %s).",
                         time_to_string((int)(now - backend->last_update_finish_time), tmp_buf1, sizeof(tmp_buf1)),
                         time_to_string((int)backend->last_update_duration, tmp_buf2, sizeof(tmp_buf2)));
            }
        } else {
            // we never requested an update
            snprintf(backend->tooltip_text, sizeof(backend->tooltip_text), "Never updated. No update scheduled.");
        }
    } else {
        // Currently executing command
        if (backend->last_update_finish_time) {
            // we finished updating at least once
            snprintf(backend->tooltip_text,
                     sizeof(backend->tooltip_text),
                     "Last update finished %s ago. Update in progress (started %s ago).",
                     time_to_string((int)(now - backend->last_update_finish_time), tmp_buf1, sizeof(tmp_buf1)),
                     time_to_string((int)(now - backend->last_update_start_time), tmp_buf3, sizeof(tmp_buf3)));
        } else {
            // we never finished an update
            snprintf(backend->tooltip_text,
                     sizeof(backend->tooltip_text),
                     "First update in progress (started %s seconds ago).",
                     time_to_string((int)(now - backend->last_update_start_time), tmp_buf1, sizeof(tmp_buf1)));
        }
    }
    return strdup(backend->tooltip_text);
}

void execp_update_post_read(Execp *execp)
{
    ExecpBackend * backend = execp->backend;
    
    int icon_h, icon_w;
    
    if (reload_icon(execp) && backend->icon) {
        imlib_context_set_image(backend->icon);
        icon_w = imlib_image_get_width();
        icon_h = imlib_image_get_height();
    } else
        icon_w = icon_h = 0;

    if (( !icon_h || !icon_w ) && !backend->text[0]) {
        // Easy to test with bash -c 'R=$(( RANDOM % 2 )); [ $R -eq 0 ] && echo HELLO $R'
        hide(&execp->area);
    } else {
        show(&execp->area);
        execp->area.resize_needed = TRUE;
        schedule_panel_redraw();
    }
    tooltip_update_for_area(&execp->area);
}

void handle_execp_events()
{
    for (GList *l = panel_config.execp_list; l; l = l->next) {
        Execp *execp = (Execp *)l->data;
        if (read_execp(execp)) {
            GList *l_instance;
            for (l_instance = execp->backend->instances; l_instance; l_instance = l_instance->next) {
                Execp *instance = (Execp *)l_instance->data;
                execp_update_post_read(instance);
            }
        }
    }
}
