/**************************************************************************
* Copyright (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr)
**************************************************************************/

#ifndef TASKBARNAME_H
#define TASKBARNAME_H

#include "common.h"
#include "area.h"

extern gboolean taskbarname_enabled;
extern Color taskbarname_font_color;
extern Color taskbarname_active_font_color;

void default_taskbarname();
void cleanup_taskbarname();

void init_taskbarname_panel(void *p);

void draw_taskbarname(void *obj, cairo_t *c);

gboolean resize_taskbarname(void *obj);

void taskbarname_default_font_changed();

void update_desktop_names();

#endif
