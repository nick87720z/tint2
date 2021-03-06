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

#ifndef TOOLTIP_H
#define TOOLTIP_H

#include "task.h"
#include "panel.h"
#include "timer.h"

typedef struct {
    Area *area; // don't use the area attribute if not 100% sure that this area was not freed
    char *tooltip_text;
    Panel *panel;
    Window window;
    int show_timeout_msec;
    int hide_timeout_msec;
    Bool mapped;
    int spacing;
    int paddingy;
    gboolean has_font;
    PangoFontDescription *font_desc;
    Color font_color;
    Background *bg;
    Timer visibility_timer;
    Timer update_timer;
    cairo_surface_t *image;
} Tooltip;

extern Tooltip g_tooltip;

void default_tooltip();
// default global data

void cleanup_tooltip();
// freed memory

// display update
void tooltip_update();                        // update, using set area
void tooltip_update_for_area(Area *area);     // comprehensive update function for use in widgets, FIXME
void tooltip_set_area(Area *area);            // change associated area

// TODO: undocumented
void init_tooltip();
void tooltip_trigger_show(Area *area, Panel *p, XEvent *e);
void tooltip_show(void * /*arg*/);
void tooltip_trigger_hide();
void tooltip_hide(void * /*arg*/);
void tooltip_default_font_changed();

#endif // TOOLTIP_H
