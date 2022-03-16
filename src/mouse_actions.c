/**************************************************************************
* Copyright (C) 2017 tint2 authors
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
#include <stdlib.h>
#include <string.h>

#include "drag_and_drop.h"
#include "panel.h"
#include "server.h"
#include "task.h"
#include "window.h"

gboolean tint2_handles_click(Panel *panel, XButtonEvent *e)
{
    if (click_task(panel, e->x, e->y))
        switch (e->button) {
        case 1: return !!mouse_left;
        case 2: return !!mouse_middle;
        case 3: return !!mouse_right;
        case 4: return !!mouse_scroll_up;
        case 5: return !!mouse_scroll_down;
        default: return FALSE;
        }
    if (click_launcher_icon(panel, e->x, e->y))
        return (e->button == 1);
    // no launcher/task clicked --> check if taskbar clicked
    if (click_taskbar(panel, e->x, e->y) && e->button == 1 && taskbar_mode == MULTI_DESKTOP)
        return TRUE;
    if (click_clock(panel, e->x, e->y))
        switch (e->button) {
        case 1: return !!clock_lclick_command;
        case 2: return !!clock_mclick_command;
        case 3: return !!clock_rclick_command;
        case 4: return !!clock_uwheel_command;
        case 5: return !!clock_dwheel_command;
        default: return FALSE;
        }
#ifdef ENABLE_BATTERY
    if (click_battery(panel, e->x, e->y))
        switch (e->button) {
        case 1: return !!battery_lclick_command;
        case 2: return !!battery_mclick_command;
        case 3: return !!battery_rclick_command;
        case 4: return !!battery_uwheel_command;
        case 5: return !!battery_dwheel_command;
        default: return FALSE;
        }
#endif
    return
        !!click_execp(panel, e->x, e->y) ||
        !!click_button(panel, e->x, e->y);
}

void handle_mouse_press_event(XEvent *e)
{
    Panel *panel = get_panel(e->xany.window);
    if (!panel)
        return;

    if (wm_menu && !tint2_handles_click(panel, &e->xbutton)) {
        forward_click(e);
        return;
    }
    task_drag = click_task(panel, e->xbutton.x, e->xbutton.y);

    lower_if_bottom (panel);
}

void handle_mouse_move_event(XEvent *e)
{
    Panel *panel;
    if (!task_drag || !(panel = get_panel(e->xany.window)))
        return;

    // Find the taskbar on the event's location
    Taskbar *event_taskbar = click_taskbar(panel, e->xbutton.x, e->xbutton.y);
    if (event_taskbar == NULL)
        return;

    // Find the task on the event's location
    Task *event_task = click_task(panel, e->xbutton.x, e->xbutton.y);

    // If the event takes place on the same taskbar as the task being dragged
    if (&event_taskbar->area == task_drag->area.parent) {
        if (taskbar_sort_method == TASKBAR_NOSORT) {
            // Swap the task_drag with the task on the event's location (if they differ)
            GList *drag_iter, *task_iter;
            if ((event_task && event_task != task_drag) &&
                (drag_iter = g_list_find (event_taskbar->area.children, task_drag)) &&
                (task_iter = g_list_find (event_taskbar->area.children, event_task)) )
            {
                gpointer temp = task_iter->data;
                task_iter->data = drag_iter->data;
                drag_iter->data = temp;
                event_taskbar->area.resize_needed = TRUE;
                schedule_panel_redraw();
                task_dragged = TRUE;
            }
        }
    } else { // The event is on another taskbar than the task being dragged
        if (task_drag->desktop == ALL_DESKTOPS || taskbar_mode != MULTI_DESKTOP)
            return;

        Taskbar *drag_taskbar = (Taskbar *)task_drag->area.parent;
        remove_area((Area *)task_drag);

        event_taskbar->area.children =  (event_taskbar->area.posx > drag_taskbar->area.posx ||
                                         event_taskbar->area.posy > drag_taskbar->area.posy) ?
                                        g_list_insert ( event_taskbar->area.children, task_drag,
                                                        taskbarname_enabled ? 1 : 0 ) :
                                        g_list_append ( event_taskbar->area.children, task_drag );

        // Move task to other desktop (but avoid the 'Window desktop changed' code in 'event_property_notify')
        task_drag->area.parent = &event_taskbar->area;
        task_drag->desktop = event_taskbar->desktop;

        change_window_desktop(task_drag->win, event_taskbar->desktop);
        if (hide_task_diff_desktop)
            change_desktop(event_taskbar->desktop);

        if (taskbar_sort_method != TASKBAR_NOSORT)
            sort_tasks(event_taskbar);

        event_taskbar->area.resize_needed = TRUE;
        drag_taskbar->area.resize_needed = TRUE;
        task_dragged = TRUE;
        schedule_panel_redraw();
        panel->area.resize_needed = TRUE;
    }
}

void handle_mouse_release_event(XEvent *e)
{
    Panel *panel = get_panel(e->xany.window);
    if (!panel)
        return;

    if (wm_menu && !tint2_handles_click(panel, &e->xbutton)) {
        forward_click(e);
        lower_if_bottom (panel);
        task_drag = NULL;
        return;
    }

    MouseAction action;
    switch (e->xbutton.button) {
    case 1: action = mouse_left;        break;
    case 2: action = mouse_middle;      break;
    case 3: action = mouse_right;       break;
    case 4: action = mouse_scroll_up;   break;
    case 5: action = mouse_scroll_down; break;
    case 6: action = mouse_tilt_left;   break;
    case 7: action = mouse_tilt_right;  break;
    default: action = TOGGLE_ICONIFY;
    }

    Clock *clock = click_clock(panel, e->xbutton.x, e->xbutton.y);
    if (clock) {
        clock_action(clock, e->xbutton.button,
                            e->xbutton.x - clock->area.posx,
                            e->xbutton.y - clock->area.posy,
                            e->xbutton.time);
        lower_if_bottom (panel);
        task_drag = NULL;
        return;
    }

#ifdef ENABLE_BATTERY
    Battery *battery = click_battery(panel, e->xbutton.x, e->xbutton.y);
    if (battery) {
        battery_action(battery, e->xbutton.button,
                                e->xbutton.x - battery->area.posx,
                                e->xbutton.y - battery->area.posy,
                                e->xbutton.time);
        lower_if_bottom (panel);
        task_drag = NULL;
        return;
    }
#endif

    Execp *execp = click_execp(panel, e->xbutton.x, e->xbutton.y);
    if (execp) {
        execp_action(execp, e->xbutton.button,
                            e->xbutton.x - execp->area.posx,
                            e->xbutton.y - execp->area.posy,
                            e->xbutton.time);
        lower_if_bottom (panel);
        task_drag = NULL;
        return;
    }

    Button *button = click_button(panel, e->xbutton.x, e->xbutton.y);
    if (button) {
        button_action(button,   e->xbutton.button,
                                e->xbutton.x - button->area.posx,
                                e->xbutton.y - button->area.posy,
                                e->xbutton.time);
        lower_if_bottom (panel);
        task_drag = NULL;
        return;
    }

    if (e->xbutton.button == 1 && click_launcher(panel, e->xbutton.x, e->xbutton.y)) {
        LauncherIcon *icon = click_launcher_icon(panel, e->xbutton.x, e->xbutton.y);
        if (icon)
            launcher_action(icon,e, e->xbutton.x - icon->area.posx,
                                    e->xbutton.y - icon->area.posy);
        task_drag = NULL;
        return;
    }

    Taskbar *taskbar;
    if (!(taskbar = click_taskbar(panel, e->xbutton.x, e->xbutton.y))) {
        // TODO: check better solution to keep window below
        lower_if_bottom (panel);
        task_drag = NULL;
        return;
    }

    // drag and drop task
    if (task_dragged) {
        task_drag = NULL;
        task_dragged = FALSE;
        return;
    }

    // switch desktop
    if (taskbar_mode == MULTI_DESKTOP) {
        gboolean diff_desktop = FALSE;
        if (taskbar->desktop != server.desktop &&
            action != CLOSE && action != DESKTOP_LEFT && action != DESKTOP_RIGHT)
        {
            diff_desktop = TRUE;
            change_desktop(taskbar->desktop);
        }
        Task *task = click_task(panel, e->xbutton.x, e->xbutton.y);
        if (task) {
            if (diff_desktop && action == TOGGLE_ICONIFY) {
                if (!window_is_active(task->win))
                    activate_window(task->win);
            } else
                task_handle_mouse_event(task, action);
        }
    } else
        task_handle_mouse_event(click_task(panel, e->xbutton.x, e->xbutton.y), action);

    // to keep window below
    lower_if_bottom (panel);
}
