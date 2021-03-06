/**************************************************************************
*
* Tint2 : taskbar
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <Imlib2.h>

#include "task.h"
#include "taskbar.h"
#include "server.h"
#include "window.h"
#include "panel.h"
#include "strnatcmp.h"
#include "tooltip.h"

GHashTable *win_to_task;

Task *active_task;
Task *task_drag;
gboolean taskbar_enabled;
gboolean taskbar_distribute_size;
gboolean hide_task_diff_desktop;
gboolean hide_inactive_tasks;
gboolean hide_task_diff_monitor;
gboolean hide_taskbar_if_empty;
gboolean always_show_all_desktop_tasks;
TaskbarSortMethod taskbar_sort_method;
Alignment taskbar_alignment;
static Timer thumbnail_update_timer_all;
static Timer thumbnail_update_timer_active;
static Timer thumbnail_update_timer_tooltip;

static GList *taskbar_task_orderings = NULL;
static GList *taskbar_thumbnail_jobs_done = NULL;

void taskbar_init_fonts();
int taskbar_get_desired_size(void *obj);

// Removes the task with &win = key. The other args are ignored.
void taskbar_remove_task(Window *win);

void taskbar_update_thumbnails(void *arg);

guint win_hash(gconstpointer key)
{
    return *((const Window *)key);
}

gboolean win_compare(gconstpointer a, gconstpointer b)
{
    return (*((const Window *)a) == *((const Window *)b));
}

void free_ptr_array(gpointer data)
{
    g_ptr_array_free(data, 1);
}

void default_taskbar()
{
    win_to_task = NULL;
    urgent_list = NULL;
    taskbar_enabled = FALSE;
    taskbar_distribute_size = FALSE;
    hide_task_diff_desktop = FALSE;
    hide_inactive_tasks = FALSE;
    hide_task_diff_monitor = FALSE;
    hide_taskbar_if_empty = FALSE;
    always_show_all_desktop_tasks = FALSE;
    taskbar_thumbnail_jobs_done = NULL;
    taskbar_sort_method = TASKBAR_NOSORT;
    taskbar_alignment = ALIGN_LEFT;
    default_taskbarname();
}

void taskbar_clear_orderings()
{
    if (!taskbar_task_orderings)
        return;
    for (GList *order = taskbar_task_orderings, *p;
         order;
         order = (p = order)->next, g_list_free_1( p))
    {
        if (sizeof(Window) <= sizeof(gpointer))
            g_list_free( order->data );
        else
            g_list_free_full( order->data, free);
    }
    taskbar_task_orderings = NULL;
}

void taskbar_save_orderings()
{
    taskbar_clear_orderings();
    GList   *tbto_tail = NULL;
    for (int i = 0; i < num_panels; i++)
    {
        Panel *panel = &panels[i];
        for (int j = 0; j < panel->num_desktops; j++)
        {
            Taskbar *taskbar = &panel->taskbar[j];
            GList   *task_order = NULL,
                    *to_tail = NULL;
            for_taskbar_tasks( taskbar, c)
            {
                if (sizeof(Window) > sizeof(gpointer)) {
                    Window *window = calloc( 1, sizeof(Window));
                    *window = ((Task *)c->data)->win;
                    g_list_append_tail( task_order, to_tail, window );
                } else
                    g_list_append_tail( task_order, to_tail, (gpointer)((Task *)c->data)->win );
            }
            g_list_append_tail( taskbar_task_orderings, tbto_tail, task_order);
        }
    }
}

void cleanup_taskbar()
{
    destroy_timer(&thumbnail_update_timer_all);
    destroy_timer(&thumbnail_update_timer_active);
    destroy_timer(&thumbnail_update_timer_tooltip);
    g_list_free(taskbar_thumbnail_jobs_done);
    taskbar_save_orderings();
    if (win_to_task) {
        while (g_hash_table_size(win_to_task)) {
            GHashTableIter iter;
            gpointer key, value;

            g_hash_table_iter_init(&iter, win_to_task);
            if (g_hash_table_iter_next(&iter, &key, &value))
                taskbar_remove_task(key);
        }
        g_hash_table_destroy(win_to_task);
        win_to_task = NULL;
    }
    cleanup_taskbarname();
    for (int i = 0; i < num_panels; i++)
    {
        Panel *panel = &panels[i];
        for (int j = 0; j < panel->num_desktops; j++)
        {
            Taskbar *taskbar = &panel->taskbar[j];
            free_area(&taskbar->area);
            // remove taskbar from the panel
            remove_area((Area *)taskbar);
        }
        if (panel->taskbar)
            free_and_null( panel->taskbar);
    }

    g_slist_free(urgent_list);
    urgent_list = NULL;

    destroy_timer(&urgent_timer);

    for (int state = 0; state < TASK_STATE_COUNT; state++) {
        g_list_free(panel_config.g_task.gradient[state]);
    }

    for (int state = 0; state < TASKBAR_STATE_COUNT; state++) {
        g_list_free(panel_config.g_taskbar.gradient[state]);
        g_list_free(panel_config.g_taskbar.gradient_name[state]);
    }
}

void init_taskbar()
{
    INIT_TIMER(urgent_timer);
    INIT_TIMER(thumbnail_update_timer_all);
    INIT_TIMER(thumbnail_update_timer_active);
    INIT_TIMER(thumbnail_update_timer_tooltip);

    if (!panel_config.g_task.has_text && !panel_config.g_task.has_icon)
        panel_config.g_task.has_text = panel_config.g_task.has_icon = TRUE;

    if (panel_config.g_task.thumbnail_width < 8)
        panel_config.g_task.thumbnail_width = 210;

    if (!win_to_task)
        win_to_task = g_hash_table_new_full(win_hash, win_compare, free, free_ptr_array);

    active_task = NULL;
    task_drag = NULL;
}

#define TaskConfig_AsbMask_Copy_if_unset(d, s)                                           \
if ((panel->g_task.config_asb_mask & ( 1 << (d) )) == 0) do{                             \
    panel->g_task.alpha     [(d)] = panel->g_task.alpha     [(s)];                       \
    panel->g_task.saturation[(d)] = panel->g_task.saturation[(s)];                       \
    panel->g_task.brightness[(d)] = panel->g_task.brightness[(s)];                       \
}while(0)

#define TaskConfig_FontMask_Copy_if_unset(d, s)                                          \
if ((panel->g_task.config_font_mask & ( 1 << (d) )) == 0)                                \
    panel->g_task.font [(d)] = panel->g_task.font [(s)];

#define TaskConfig_BgMask_Copy_if_unset(d, s)                                            \
if ((panel->g_task.config_background_mask & ( 1 << (d) )) == 0)                          \
    panel->g_task.background[(d)] = panel->g_task.background[(s)];

void init_taskbar_panel(void *p)
{
    Panel *panel = p;

    if (!panel->g_taskbar.background[TASKBAR_NORMAL])
    {
        panel->g_taskbar.background[TASKBAR_NORMAL] = 
        panel->g_taskbar.background[TASKBAR_ACTIVE] = &g_array_index(backgrounds, Background, 0);
    }
    if (!panel->g_taskbar.background_name[TASKBAR_NORMAL])
    {
        panel->g_taskbar.background_name[TASKBAR_NORMAL] = 
        panel->g_taskbar.background_name[TASKBAR_ACTIVE] = &g_array_index(backgrounds, Background, 0);
    }
    if (!panel->g_task.area.bg)
        panel->g_task.area.bg = &g_array_index(backgrounds, Background, 0);
    taskbar_init_fonts();

    // taskbar name
    panel->g_taskbar.area_name.panel = panel;
    snprintf (panel->g_taskbar.area_name.name, strlen_const(panel->g_taskbar.area_name.name), "Taskbarname");
    panel->g_taskbar.area_name.size_mode            = LAYOUT_FIXED;
    panel->g_taskbar.area_name._resize              = resize_taskbarname;
    panel->g_taskbar.area_name._is_under_mouse      = full_width_area_is_under_mouse;
    panel->g_taskbar.area_name._draw_foreground     = draw_taskbarname;
    panel->g_taskbar.area_name._on_change_layout    = 0;
    panel->g_taskbar.area_name.resize_needed        = TRUE;
    panel->g_taskbar.area_name.on_screen            = TRUE;

    // taskbar
    panel->g_taskbar.area.parent = panel;
    panel->g_taskbar.area.panel = panel;
    snprintf (panel->g_taskbar.area.name, strlen_const(panel->g_taskbar.area.name), "Taskbar");
    panel->g_taskbar.area.size_mode             = LAYOUT_DYNAMIC;
    panel->g_taskbar.area.alignment             = taskbar_alignment;
    panel->g_taskbar.area._resize               = resize_taskbar;
    panel->g_taskbar.area._get_desired_size     = taskbar_get_desired_size;
    panel->g_taskbar.area._is_under_mouse       = full_width_area_is_under_mouse;
    panel->g_taskbar.area.resize_needed         = TRUE;
    panel->g_taskbar.area.on_screen             = TRUE;
    if (panel_horizontal)
    {
        panel->g_taskbar.area.posy   = top_border_width( & panel->area) + panel->area.paddingy * panel->scale;
        panel->g_taskbar.area.height = panel->area.height
                                     - top_bottom_border_width( & panel->area)
                                     - 2 * panel->area.paddingy * panel->scale;

        panel->g_taskbar.area_name.posy   = panel->g_taskbar.area.posy;
        panel->g_taskbar.area_name.height = panel->g_taskbar.area.height;
    }
    else
    {
        panel->g_taskbar.area.posx  = left_border_width( & panel->area) + panel->area.paddingy * panel->scale;
        panel->g_taskbar.area.width = panel->area.width
                                    - left_right_border_width( & panel->area)
                                    - 2 * panel->area.paddingy * panel->scale;

        panel->g_taskbar.area_name.posx  = panel->g_taskbar.area.posx;
        panel->g_taskbar.area_name.width = panel->g_taskbar.area.width;
    }

    // task
    panel->g_task.area.panel = panel;
    snprintf (panel->g_task.area.name, strlen_const(panel->g_task.area.name), "Task");
    panel->g_task.area.size_mode            = LAYOUT_DYNAMIC;
    panel->g_task.area._draw_foreground     = draw_task;
    panel->g_task.area._on_change_layout    = on_change_task;
    panel->g_task.area.resize_needed        = TRUE;
    panel->g_task.area.on_screen            = TRUE;
    if ((panel->g_task.config_asb_mask & (1 << TASK_NORMAL)) == 0)
    {
        panel->g_task.alpha[TASK_NORMAL] = 100;
        panel->g_task.saturation[TASK_NORMAL] = 0;
        panel->g_task.brightness[TASK_NORMAL] = 0;
    }
    TaskConfig_AsbMask_Copy_if_unset (TASK_ACTIVE,    TASK_NORMAL);
    TaskConfig_AsbMask_Copy_if_unset (TASK_ICONIFIED, TASK_NORMAL);
    TaskConfig_AsbMask_Copy_if_unset (TASK_URGENT,    TASK_NORMAL);

    if ((panel->g_task.config_font_mask & (1 << TASK_NORMAL)) == 0)
        panel->g_task.font[TASK_NORMAL] = (Color){{1, 1, 1}, 1};
    TaskConfig_FontMask_Copy_if_unset (TASK_ACTIVE,     TASK_NORMAL);
    TaskConfig_FontMask_Copy_if_unset (TASK_ICONIFIED,  TASK_NORMAL);
    TaskConfig_FontMask_Copy_if_unset (TASK_URGENT,     TASK_ACTIVE);

    if ((panel->g_task.config_background_mask & (1 << TASK_NORMAL)) == 0)
        panel->g_task.background[TASK_NORMAL] = &g_array_index(backgrounds, Background, 0);
    TaskConfig_BgMask_Copy_if_unset (TASK_ACTIVE,     TASK_NORMAL);
    TaskConfig_BgMask_Copy_if_unset (TASK_ICONIFIED,  TASK_NORMAL);
    TaskConfig_BgMask_Copy_if_unset (TASK_URGENT,     TASK_ACTIVE);

    if (!panel->g_task.maximum_width || !panel_horizontal)
        panel->g_task.maximum_width = server.monitors[panel->monitor].width;
    if (!panel->g_task.maximum_height || panel_horizontal)
        panel->g_task.maximum_height = server.monitors[panel->monitor].height;

    if (panel_horizontal) {
        panel->g_task.area.posy = panel->g_taskbar.area.posy
                                + top_bg_border_width( panel->g_taskbar.background[TASKBAR_NORMAL])
                                + panel->g_taskbar.area.paddingy * panel->scale;

        panel->g_task.area.width  = panel->g_task.maximum_width;
        panel->g_task.area.height = panel->g_taskbar.area.height
                                    - top_bottom_bg_border_width( panel->g_taskbar.background[TASKBAR_NORMAL])
                                    - 2 * panel->g_taskbar.area.paddingy * panel->scale;
    } else {
        panel->g_task.area.posx = panel->g_taskbar.area.posx
                                + left_bg_border_width( panel->g_taskbar.background[TASKBAR_NORMAL])
                                + panel->g_taskbar.area.paddingy * panel->scale;

        panel->g_task.area.width =  panel->g_taskbar.area.width
                                    - left_right_bg_border_width( panel->g_taskbar.background[TASKBAR_NORMAL])
                                    - 2 * panel->g_taskbar.area.paddingy * panel->scale;

        panel->g_task.area.height = panel->g_task.maximum_height * panel->scale;
    }

    for (int j = 0; j < TASK_STATE_COUNT; ++j)
    {
        if (!panel->g_task.background[j])
            panel->g_task.background[j] = &g_array_index(backgrounds, Background, 0);

        if (panel->g_task.background[j]->border.radius > panel->g_task.area.height / 2)
        {
            fprintf(stderr,
                    "tint2: task%sbackground_id has a too large rounded value. Please fix your tint2rc\n",
                    j == 0 ? "_" : j == 1 ? "_active_" : j == 2 ? "_iconified_" : "_urgent_");
            g_array_append_val(backgrounds, *panel->g_task.background[j]);
            panel->g_task.background[j] = &g_array_index(backgrounds, Background, backgrounds->len - 1);
            panel->g_task.background[j]->border.radius = panel->g_task.area.height / 2;
        }
    }

    panel->g_task.text_posx = panel->g_task.area.paddingx * panel->scale
                            + left_bg_border_width(panel->g_task.background[0]);

    panel->g_task.text_height = panel->g_task.area.height
                                - 2 * panel->g_task.area.paddingy * panel->scale
                                - top_bottom_border_width (&panel->g_task.area);
    if (panel->g_task.has_icon)
    {
        panel->g_task.icon_size1 = MIN( MIN(panel->g_task.maximum_width,
                                            panel->g_task.maximum_height) * panel->scale,
                                        MIN(panel->g_task.area.width, panel->g_task.area.height) )
                                   - MAX( left_right_border_width (&panel->g_task.area),
                                          top_bottom_border_width (&panel->g_task.area) )
                                   - 2 * panel->g_task.area.paddingy * panel->scale;

        panel->g_task.text_posx += panel->g_task.icon_size1 + panel->g_task.area.spacing * panel->scale;
        panel->g_task.icon_posy = (panel->g_task.area.height - panel->g_task.icon_size1) / 2;
    }

    Taskbar *taskbar;
    panel->num_desktops = server.num_desktops;
    panel->taskbar = calloc(server.num_desktops, sizeof(Taskbar));
    for (int j = 0; j < panel->num_desktops; j++)
    {
        taskbar = &panel->taskbar[j];
        taskbar->area = panel->g_taskbar.area;
        taskbar->desktop = j;
        taskbar->area.bg = panel->g_taskbar.background[j == server.desktop ? TASKBAR_ACTIVE : TASKBAR_NORMAL];
        area_gradients_reset( & taskbar->area);
    }
    init_taskbarname_panel(panel);
    taskbar_start_thumbnail_timer(THUMB_MODE_ALL);
}
#undef TaskConfig_AsbMask_Copy_if_unset
#undef TaskConfig_FontMask_Copy_if_unset
#undef TaskConfig_BgMask_Copy_if_unset

void taskbar_start_thumbnail_timer(ThumbnailUpdateMode mode)
{
    if (!panel_config.g_task.thumbnail_enabled)
        return;
    if (debug_thumbnails)
        fprintf(stderr, BLUE "tint2: taskbar_start_thumbnail_timer %s" RESET "\n", mode == THUMB_MODE_ACTIVE_WINDOW ? "active" : mode == THUMB_MODE_TOOLTIP_WINDOW ? "tooltip" : "all");
    change_timer (  mode == THUMB_MODE_ALL           ? &thumbnail_update_timer_all    :
                    mode == THUMB_MODE_ACTIVE_WINDOW ? &thumbnail_update_timer_active :
                    &thumbnail_update_timer_tooltip,

                    true,
                    mode == THUMB_MODE_TOOLTIP_WINDOW ? 1000     : 500,
                    mode == THUMB_MODE_ALL            ? 1000 * 10 : 0,
                    taskbar_update_thumbnails,
                    (void *)(long)mode );
}

void taskbar_init_fonts()
{
    for (int i = 0; i < num_panels; i++)
        if (!panels[i].g_task.font_desc)
        {
            panels[i].g_task.font_desc = pango_font_description_from_string(get_default_font());
            pango_font_description_set_size(panels[i].g_task.font_desc,
                                            pango_font_description_get_size(panels[i].g_task.font_desc) - PANGO_SCALE);
        }
}

void taskbar_default_font_changed()
{
    if (!taskbar_enabled)
        return;

    gboolean needs_update = FALSE;
    for (int i = 0; i < num_panels; i++)
        if (!panels[i].g_task.has_font)
        {
            pango_font_description_free(panels[i].g_task.font_desc);
            panels[i].g_task.font_desc = NULL;
            needs_update = TRUE;
        }
    if (!needs_update)
        return;
    taskbar_init_fonts();
    for (int i = 0; i < num_panels; i++)
        for (int j = 0; j < panels[i].num_desktops; j++)
        {
            Taskbar *taskbar = &panels[i].taskbar[j];
            for (GList *c = taskbar->area.children; c; c = c->next) {
                Task *t = c->data;
                t->area.resize_needed = TRUE;
                schedule_redraw(&t->area);
            }
        }
    schedule_panel_redraw();
}

void taskbar_remove_task(Window *win)
{
    remove_task(get_task(*win));
}

Task *get_task(Window win)
{
    GPtrArray *task_buttons = get_task_buttons(win);
    return  task_buttons ?
            g_ptr_array_index(task_buttons, 0) : NULL;
}

GPtrArray *get_task_buttons(Window win)
{
    return  (win_to_task && taskbar_enabled) ?
            g_hash_table_lookup(win_to_task, &win) : NULL;
}

int compare_windows(const void *a, const void *b)
{
    Window wina = *(Window *)a;
    Window winb = *(Window *)b;

    for (GList *order = taskbar_task_orderings; order; order = order->next) {
        int posa = -1;
        int posb = -1;
        int pos = 0;
        for (GList *item = order->data; item; item = item->next, pos++)
        {
            Window win = sizeof(Window) <= sizeof(gpointer) ? (Window)(item->data) : *(Window *)item->data;
            if (win == wina)
                posa = pos;
            if (win == winb)
                posb = pos;
        }
        if (posa >= 0 && posb >= 0)
            return posa - posb;
    }

    return (char *)a - (char *)b;
}

void sort_win_list(Window *windows, int count)
{
    Window *result = malloc( count * sizeof(Window));
    memcpy( result, windows, count * sizeof(Window));
    qsort( windows, count, sizeof(Window), compare_windows);

    memcpy(windows, result, count * sizeof(Window));
    free(result);
}

void taskbar_refresh_tasklist()
{
    if (!taskbar_enabled)
        return;

    int num_results;
    Window *win = get_property(server.root_win, server.atom [_NET_CLIENT_LIST], XA_WINDOW, &num_results);
    if (!win)
        return;

    Window *sorted = calloc(num_results, sizeof(Window));
    memcpy(sorted, win, num_results * sizeof(Window));
    if (taskbar_task_orderings)
    {
        sort_win_list(sorted, num_results);
        taskbar_clear_orderings();
    }

    GList *win_list = g_hash_table_get_keys(win_to_task);
    for (GList *it = win_list, *p;
         it;
         it = (p = it)->next, g_list_free_1( p))
    {
        int i;
        for (i = 0; i < num_results; i++)
            if (*(Window *)it->data == sorted[i])
                break;
        if (i == num_results)
            taskbar_remove_task(it->data);
    }

    // Add any new
    for (int i = 0; i < num_results; i++)
        if (!get_task(sorted[i]))
            add_task(sorted[i]);

    XFree(win);
    free(sorted);
}

int taskbar_get_desired_size(void *obj)
{
    Taskbar *taskbar = obj;
    Panel *panel = taskbar->area.panel;

    if (taskbar_mode != MULTI_DESKTOP || taskbar_distribute_size)
        return container_get_desired_size( & taskbar->area);

    int result = 0;
    for (int i = 0; i < panel->num_desktops; i++)
    {
        Taskbar *t = &panel->taskbar[i];
        int size = container_get_desired_size( & t->area);
        if (size > result)
            result = size;
    }
    return result;
}

gboolean resize_taskbar(void *obj)
{
    Taskbar *taskbar = obj;
    Panel *panel = taskbar->area.panel;

    if (panel_horizontal) {
        relayout_with_constraint(&taskbar->area, panel->g_task.maximum_width * panel->scale);

        int text_width = panel->g_task.maximum_width * panel->scale;
        for_taskbar_tasks( taskbar, l)
            if (((Task *)l->data)->area.on_screen)
            {
                text_width = ((Task *)l->data)->area.width;
                break;
            }
        taskbar->text_width = text_width - panel->g_task.text_posx - right_border_width(&panel->g_task.area)
                            - panel->g_task.area.paddingx * panel->scale;
    } else {
        relayout_with_constraint(&taskbar->area, panel->g_task.maximum_height * panel->scale);

        taskbar->text_width = taskbar->area.width
                            - 2 * panel->g_taskbar.area.paddingy * panel->scale
                            - panel->g_task.text_posx
                            - right_border_width (&panel->g_task.area)
                            - panel->g_task.area.paddingx * panel->scale;
    }
    return FALSE;
}

gboolean taskbar_is_empty(Taskbar *taskbar)
{
    for_taskbar_tasks( taskbar, l)
        if (((Task *)l->data)->area.on_screen)
            return FALSE;
    return TRUE;
}

void update_taskbar_visibility(Taskbar *taskbar)
{
    (   taskbar->desktop == server.desktop || (
            taskbar_mode == MULTI_DESKTOP && !(hide_taskbar_if_empty && taskbar_is_empty(taskbar))
        ) ?
        show : hide
    )(&taskbar->area);
}

void update_all_taskbars_visibility()
{
    for (int i = 0; i < num_panels; i++) {
        Panel *panel = &panels[i];

        for (int j = 0; j < panel->num_desktops; j++)
            update_taskbar_visibility(&panel->taskbar[j]);
    }
}

void set_taskbar_state(Taskbar *taskbar, TaskbarState state)
{
    update_taskbar_visibility(taskbar);
    if (taskbarname_enabled)
    {
        taskbar->bar_name.area.bg = panels[0].g_taskbar.background_name[state];
        area_gradients_reset( & taskbar->bar_name.area);

        if (taskbar->area.on_screen)
            schedule_redraw( & taskbar->bar_name.area);
    }
    taskbar->area.bg = panels[0].g_taskbar.background[state];
    area_gradients_reset( & taskbar->area);
    if (taskbar->area.on_screen)
    {
        schedule_redraw( & taskbar->area);
        if (taskbar_mode == MULTI_DESKTOP) {
            GList *lfirst = taskbarname_enabled ? taskbar->area.children->next : taskbar->area.children;

            Background **bg = panels[0].g_taskbar.background;
            if (bg[TASKBAR_NORMAL] != bg[TASKBAR_ACTIVE])
            {
                for (GList *l = lfirst; l; l = l->next)
                    schedule_redraw(l->data);
            }
            if (hide_task_diff_desktop)
            {
                for (GList *l = lfirst; l; l = l->next) {
                    Task *task = l->data;
                    set_task_state(task, task->current_state);
                }
            }
        }
    }
    schedule_panel_redraw();
}

#define NONTRIVIAL 2
gint compare_tasks_trivial(Task *a, Task *b, Taskbar *taskbar)
{
    return  a == b ? 0
        :   !taskbarname_enabled ? NONTRIVIAL
        :   a == taskbar->area.children->data ? -1
        :   b == taskbar->area.children->data ? 1
        :   NONTRIVIAL;
}

gboolean contained_within(Task *a, Task *b)
{
    return ((a->win_x <= b->win_x) &&
            (a->win_y <= b->win_y) &&
            (a->win_x + a->win_w >= b->win_x + b->win_w) &&
            (a->win_y + a->win_h >= b->win_y + b->win_h));
}

gint compare_task_centers(Task *a, Task *b, Taskbar *taskbar)
{
    int a_horiz_c, a_vert_c, b_horiz_c, b_vert_c,
        trivial = compare_tasks_trivial(a, b, taskbar);

    return  trivial != NONTRIVIAL ? trivial
        // If a window has the same coordinates and size as the other,
        // they are considered to be equal in the comparison.
        :   ((a->win_x == b->win_x) && (a->win_y == b->win_y) &&
             (a->win_w == b->win_w) && (a->win_h == b->win_h)) ? 0
        // If a window is completely contained in another,
        // then it is considered to come after (to the right/bottom) of the other.
        :   contained_within(a, b) ? -1
        :   contained_within(b, a) ? 1
        :(  a_horiz_c = a->win_x + a->win_w / 2,
            a_vert_c  = a->win_y + a->win_h / 2,
            b_horiz_c = b->win_x + b->win_w / 2,
            b_vert_c  = b->win_y + b->win_h / 2,
            (panel_horizontal ? a_horiz_c != b_horiz_c : a_vert_c == b_vert_c)
            ?   a_horiz_c - b_horiz_c
            :   a_vert_c - b_vert_c );
}

gint compare_task_titles(Task *a, Task *b, Taskbar *taskbar)
{
    int trivial = compare_tasks_trivial(a, b, taskbar);
    return  trivial != NONTRIVIAL ? trivial
        :   strnatcasecmp(  a->title ? a->title : "",
                            b->title ? b->title : "");
}

gint compare_task_applications(Task *a, Task *b, Taskbar *taskbar)
{
    int trivial = compare_tasks_trivial(a, b, taskbar);
    return  trivial != NONTRIVIAL ? trivial 
        :   strnatcasecmp(  a->application ? a->application : "",
                            b->application ? b->application : "");
}

gint compare_tasks(Task *a, Task *b, Taskbar *taskbar)
{
    int trivial = compare_tasks_trivial(a, b, taskbar);
    if (trivial != NONTRIVIAL)
        return trivial;
        
    switch (taskbar_sort_method) {
    case TASKBAR_NOSORT:            return 0;
    case TASKBAR_SORT_CENTER:       return compare_task_centers(a, b, taskbar);
    case TASKBAR_SORT_TITLE:        return compare_task_titles(a, b, taskbar);
    case TASKBAR_SORT_APPLICATION:  return compare_task_applications(a, b, taskbar);
    case TASKBAR_SORT_LRU:          return compare_timespecs(&a->last_activation_time, &b->last_activation_time);
    case TASKBAR_SORT_MRU:          return -compare_timespecs(&a->last_activation_time, &b->last_activation_time);
    }
    return 0;
}

gboolean taskbar_needs_sort(Taskbar *taskbar)
{
    if (taskbar_sort_method != TASKBAR_NOSORT)
        for (GList *i = taskbar->area.children, *j = i ? i->next : NULL;
            i && j;
            i = i->next, j = j->next)
        {
            if (compare_tasks(i->data, j->data, taskbar) > 0)
                return TRUE;
        }
    return FALSE;
}

void sort_tasks(Taskbar *taskbar)
{
    if (!taskbar || !taskbar_needs_sort(taskbar))
        return;

    taskbar->area.children = g_list_sort_with_data(taskbar->area.children, (GCompareDataFunc)compare_tasks, taskbar);
    schedule_panel_redraw();
}

void sort_taskbar_for_win(Window win)
{
    if (taskbar_sort_method == TASKBAR_NOSORT)
        return;

    GPtrArray *task_buttons;
    Task      *task0;
    if ((task_buttons = get_task_buttons( win)) &&
        (task0 = g_ptr_array_index( task_buttons, 0)))
    {
        get_window_coordinates(win, &task0->win_x, &task0->win_y, &task0->win_w, &task0->win_h);
        sort_tasks( task0->area.parent);
        for (int i = 1; i < task_buttons->len; ++i)
        {
            Task *task = g_ptr_array_index(task_buttons, i);
            task->win_x = task0->win_x;
            task->win_y = task0->win_y;
            task->win_w = task0->win_w;
            task->win_h = task0->win_h;
            sort_tasks(task->area.parent);
        }
    }
}

void update_minimized_icon_positions(void *p)
{
    Panel *panel = p;
    for (int i = 0; i < panel->num_desktops; i++) {
        Taskbar *taskbar = &panel->taskbar[i];

        if (!taskbar->area.on_screen)
            continue;
        for (GList *c = taskbar->area.children; c; c = c->next) {
            Area *area = c->data;

            if (area->_on_change_layout)
                area->_on_change_layout(area);
        }
    }
}

void taskbar_update_thumbnails(void *arg)
{
    if (!panel_config.g_task.thumbnail_enabled)
        return;
    ThumbnailUpdateMode mode = (ThumbnailUpdateMode)(long)arg;
    if (debug_thumbnails)
        fprintf(stderr, BLUE "tint2: taskbar_update_thumbnails %s" RESET "\n", mode == THUMB_MODE_ACTIVE_WINDOW ? "active" :
                                                                               mode == THUMB_MODE_TOOLTIP_WINDOW ? "tooltip" : "all");
    GList * jdone_tail = g_list_last (taskbar_thumbnail_jobs_done);

    double start_time = get_time();
    for (int i = 0; i < num_panels; i++) {
        Panel *panel = &panels[i];

        for (int j = 0; j < panel->num_desktops; j++) {
            Taskbar *taskbar = &panel->taskbar[j];
            for_taskbar_tasks( taskbar, c)
            {
                Task *t = c->data;
                if ((mode == THUMB_MODE_ALL && t->current_state == TASK_ACTIVE && !g_list_find(taskbar_thumbnail_jobs_done, t)) ||
                    (mode == THUMB_MODE_ACTIVE_WINDOW && t->current_state == TASK_ACTIVE) ||
                    (mode == THUMB_MODE_TOOLTIP_WINDOW && g_tooltip.mapped && g_tooltip.area == &t->area))
                {
                    task_refresh_thumbnail(t);
                    if (mode == THUMB_MODE_ALL)
                        g_list_append_tail (taskbar_thumbnail_jobs_done, jdone_tail, t);
                    if (t->thumbnail && mode == THUMB_MODE_TOOLTIP_WINDOW)
                        taskbar_start_thumbnail_timer(THUMB_MODE_TOOLTIP_WINDOW);
                }
                if (mode == THUMB_MODE_ALL &&
                    get_time() - start_time > 0.030)
                {
                    change_timer(&thumbnail_update_timer_all, true, 50, 10 * 1000, taskbar_update_thumbnails, arg);
                    return;
                }
            }
        }
    }
    if (mode == THUMB_MODE_ALL &&
        taskbar_thumbnail_jobs_done)
    {
        g_list_free(taskbar_thumbnail_jobs_done);
        taskbar_thumbnail_jobs_done = NULL;
        change_timer(&thumbnail_update_timer_all, true, 10 * 1000, 10 * 1000, taskbar_update_thumbnails, arg);
    }
}
