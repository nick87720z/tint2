/**************************************************************************
*
* Tint2 : freespace
*
* Copyright (C) 2011 Mishael A Sibiryakov (death@junki.org)
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
#include "server.h"
#include "panel.h"
#include "freespace.h"
#include "common.h"

int freespace_get_desired_size(void *obj);

void init_freespace_panel(void *p)
{
    Panel *panel = p;

    // Make sure this is only done once if there are multiple items
    if (panel->freespace_list)
        return;

    GList *fs_tail = NULL;
    for_panel_items_order ()
    {
        if (panel_items_order[k] == 'F') {
            FreeSpace *freespace = calloc(1, sizeof(FreeSpace));
            g_list_append_tail (panel->freespace_list, fs_tail, freespace);
            if (!freespace->area.bg)
                freespace->area.bg = &g_array_index(backgrounds, Background, 0);
            freespace->area.parent = p;
            freespace->area.panel = p;
            snprintf(freespace->area.name, strlen_const(freespace->area.name), "Freespace");
            freespace->area.size_mode = LAYOUT_FIXED;
            freespace->area.resize_needed = TRUE;
            freespace->area.on_screen = TRUE;
            freespace->area._resize = resize_freespace;
            freespace->area._get_desired_size = freespace_get_desired_size;
        }
    }
}

void cleanup_freespace(Panel *panel)
{
    if (panel->freespace_list) {
        g_list_free_full(panel->freespace_list, free);
        panel->freespace_list = NULL;
    }
}

int freespace_get_max_size(Panel *panel)
{
    if (panel_shrink)
        return 0;
    // Get space used by every element except the freespace
    int size = 0;
    int spacers = 0;
    for (GList *walk = panel->area.children; walk; walk = walk->next) {
        Area *a = walk->data;
        if (a->on_screen)
        {
            if (a->_resize == resize_freespace)
                spacers++;
            else
                size += panel_horizontal ? a->width  + panel->area.spacing  * panel->scale
                                         : a->height + panel->area.paddingy * panel->scale;
        }
    }
    size = (panel_horizontal ? panel->area.width  - left_right_border_width(&panel->area)
                             : panel->area.height - top_bottom_border_width(&panel->area)) - size - panel->area.paddingx * panel->scale;

    return size / spacers;
}

int freespace_get_desired_size(void *obj)
{
    FreeSpace *freespace = obj;
    return freespace_get_max_size(freespace->area.panel);
}

gboolean resize_freespace(void *obj)
{
    FreeSpace *freespace = obj;
    Panel *panel = freespace->area.panel;
    if (!freespace->area.on_screen)
        return FALSE;

    int old_size = panel_horizontal ? freespace->area.width : freespace->area.height;
    int size = freespace_get_max_size(panel);
    if (old_size == size)
        return FALSE;

    if (panel_horizontal)
        freespace->area.width = size;
    else
        freespace->area.height = size;

    schedule_redraw(&freespace->area);
    schedule_panel_redraw();
    return TRUE;
}
