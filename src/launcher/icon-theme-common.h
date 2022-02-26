/**************************************************************************
 * Copyright (C) 2015       (mrovi9000@gmail.com)
 *
 **************************************************************************/

#ifndef ICON_THEME_COMMON_H
#define ICON_THEME_COMMON_H

#include <glib.h>
#include "cache.h"

typedef struct IconThemeWrapper {
    char     *icon_theme_name;  // The icon theme name for which this wrapper was created
    GSList   *themes;           // List of IconThemes
    gboolean _themes_loaded;    // Themes are loaded lazily when needed.
    GSList   *themes_fallback;  // List of IconTheme*
    gboolean _fallback_loaded;  // Fallback themes are loaded lazily when needed.
    Cache _cache;

    GSList *_queued;    // List of icon theme names that have been queued for loading.
                        // Used to avoid loading the same theme twice, and to avoid cycles.
} IconThemeWrapper;

typedef struct IconTheme {
    char *name;
    char *description;
    GSList *list_inherits;    // each item is a char* (theme name)
    GSList *list_directories; // each item is an IconThemeDir*
} IconTheme;

#define parse_theme_line parse_dektop_line
// Parses a line of the form "key = value". Modifies the line.
// Returns 1 if successful, and parts are not empty.
// Key and value point to the parts.

IconThemeWrapper *load_themes(const char *icon_theme_name);
// Returns an IconThemeWrapper* containing the icon theme identified by the name icon_theme_name, all the
// inherited themes, the hicolor theme and possibly fallback themes.

void save_icon_cache(IconThemeWrapper *wrapper);

void free_themes(IconThemeWrapper *wrapper);
void free_icon_theme(IconTheme *theme);

#define DEFAULT_ICON "application-x-executable"

char *get_icon_path(IconThemeWrapper *wrapper, const char *icon_name, int size, gboolean use_fallbacks);
// Returns the full path to an icon file (or NULL) given the list of icon themes to search and the icon name
// Note: needs to be released with free().

const GSList *get_icon_locations();
// Returns a list of the directories used to store icons.
// Do not free the result, it is cached.

extern gboolean debug_icons;

extern char *icon_cache_path;

#endif
