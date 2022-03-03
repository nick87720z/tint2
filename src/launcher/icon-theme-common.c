/**************************************************************************
* Tint2 : Icon theme handling
*
* Copyright (C) 2015       (mrovi9000@gmail.com)
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

/* http://standards.freedesktop.org/icon-theme-spec/ */

#include "icon-theme-common.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apps-common.h"
#include "common.h"
#include "cache.h"

gboolean debug_icons = FALSE;
char *icon_cache_path = NULL;

#define ICON_DIR_TYPE_SCALABLE 0
#define ICON_DIR_TYPE_FIXED 1
#define ICON_DIR_TYPE_THRESHOLD 2
typedef struct IconThemeDir {
    char *name;
    int size;
    int type;
    int max_size;
    int min_size;
    int threshold;
} IconThemeDir;

static GSList *icon_locations = NULL;
// Do not free the result.

static char *icon_extensions[] = {
    ".png", ".xpm",
    #ifdef HAVE_RSVG
    ".svg",
    #endif
    "", NULL
};

gboolean str_list_contains(const GSList *list, const char *value)
{
    // return g_slist_find_custom (list, value, g_str_equal) ? TRUE : FALSE;
    // FIXME: above variant gives different result than below

    for (const GSList *item = list; item; item = item->next)
    {
        if (g_str_equal(item->data, value))
            return TRUE;
    }
    return FALSE;
}
//~ #define str_list_contains(list, value) (g_slist_find_custom (list, value, g_str_equal) != NULL)

const GSList *get_icon_locations()
{
    if (icon_locations)
        return icon_locations;

    icon_locations = load_locations_from_env(icon_locations, "XDG_DATA_HOME", ".icons", NULL);
    icon_locations = load_locations_from_dir(icon_locations, g_get_home_dir(), ".icons", ".local/share/icons", NULL);
    icon_locations = load_locations_from_env(icon_locations, "XDG_DATA_DIRS", ".icons", ".pixmaps", NULL);

    slist_append_uniq_dup(icon_locations, "/usr/local/share/icons", g_str_equal);
    slist_append_uniq_dup(icon_locations, "/usr/local/share/pixmaps", g_str_equal);
    slist_append_uniq_dup(icon_locations, "/usr/share/icons", g_str_equal);
    slist_append_uniq_dup(icon_locations, "/usr/share/pixmaps", g_str_equal);
    slist_append_uniq_dup(icon_locations, "/opt/share/icons", g_str_equal);
    slist_append_uniq_dup(icon_locations, "/opt/share/pixmaps", g_str_equal);

    return icon_locations;
}

IconTheme *make_theme(const char *name)
{
    IconTheme *theme = calloc(1, sizeof(IconTheme));
    theme->name = strdup(name);
    theme->list_inherits = NULL;
    theme->list_directories = NULL;
    return theme;
}

enum                   {k_MaxSize, k_MinSize, k_Size, k_Threshold, k_Type, THEME_INDEX_KEYS};
char *index_opt_sv[] = {"MaxSize", "MinSize", "Size", "Threshold", "Type"};

// TODO Use UTF8 when parsing the file
IconTheme *load_theme_from_index(const char *file_name, const char *name)
{
    IconTheme *theme;
    FILE *f;
    char *line = NULL;
    size_t line_size;

    if ((f = fopen(file_name, "rt")) == NULL) {
        fprintf(stderr, "tint2: Could not open theme '%s'\n", file_name);
        return NULL;
    }

    theme = make_theme(name);
    GSList  *inh_tail = g_slist_last (theme->list_inherits),
            *dir_tail = g_slist_last (theme->list_directories);

    IconThemeDir *current_dir = NULL;
    int inside_header = 1;
    ssize_t line_len;
    while ((line_len = getline(&line, &line_size, f)) >= 0) {
        char *key, *value;

        if (line_len >= 1)
            str_strip_newline (line, line_len);

        if (line_len == 0)
            continue;

        if (inside_header) {
            if (parse_theme_line(line, &key, &value)) {
                if (strcmp(key, "Inherits") == 0) {
                    // value is like oxygen,wood,default
                    for (char *token = strtok(value, ",\n");
                         token;
                         token = strtok(NULL, ",\n"))
                    {
                        g_slist_append_tail (theme->list_inherits, inh_tail, strdup(token));
                    }
                } else if (strcmp(key, "Directories") == 0) {
                    // value is like 48x48/apps,48x48/mimetypes,32x32/apps,scalable/apps,scalable/mimetypes
                    for (char *token = strtok(value, ",\n");
                         token;
                         token = strtok(NULL, ",\n") )
                    {
                        IconThemeDir *dir = calloc (1, sizeof(IconThemeDir));
                        dir->name = strdup(token);
                        dir->max_size = dir->min_size = dir->size = -1;
                        dir->type = ICON_DIR_TYPE_THRESHOLD;
                        dir->threshold = 2;
                        g_slist_append_tail (theme->list_directories, dir_tail, dir);
                    }
                }
            }
        } else if (current_dir != NULL) {
            if (parse_theme_line(line, &key, &value))
            {
                switch ( str_index (key, index_opt_sv, THEME_INDEX_KEYS) )
                {
                    case k_Size: // Size: value is like 24
                            sscanf(value, "%d", &current_dir->size);
                            if (current_dir->max_size == -1)
                                current_dir->max_size = current_dir->size;
                            if (current_dir->min_size == -1)
                                current_dir->min_size = current_dir->size;
                            break;
                    case k_MaxSize: // MaxSize: value is like 24
                            sscanf(value, "%d", &current_dir->max_size);
                            break;
                    case k_MinSize: // MinSize: value is like 24
                            sscanf(value, "%d", &current_dir->min_size);
                            break;
                    case k_Threshold: // Threshold: value is like 2
                            sscanf(value, "%d", &current_dir->threshold);
                            break;
                    case k_Type: // Type: value is Fixed, Scalable or Threshold : default to scalable for unknown Type.
                            if (strcmp(value, "Fixed") == 0) {
                                current_dir->type = ICON_DIR_TYPE_FIXED;
                            } else if (strcmp(value, "Threshold") == 0) {
                                current_dir->type = ICON_DIR_TYPE_THRESHOLD;
                            } else {
                                current_dir->type = ICON_DIR_TYPE_SCALABLE;
                            }
                            break;
                }
            }
        }

        if (line[0] == '[' && line[line_len - 1] == ']' &&
            !str_lequal_static (line, "[Icon Theme]", line_len))
        {
            inside_header = 0;
            current_dir = NULL;
            line[line_len - 1] = '\0';
            char *dir_name = line + 1;
            for (GSList *dir_item = theme->list_directories;
                 dir_item; dir_item = dir_item->next)
            {
                IconThemeDir *dir = dir_item->data;
                if (strcmp(dir->name, dir_name) == 0) {
                    current_dir = dir;
                    break;
                }
            }
        }
    }
    fclose(f);

    free(line);
    return theme;
}

void load_theme_from_fs_dir(IconTheme *theme, const char *dir_name)
{
    gchar *file_name = g_build_filename(dir_name, "index.theme", NULL);
    if (g_file_test(file_name, G_FILE_TEST_EXISTS)) {
        g_free(file_name);
        return;
    }

    GSList *d_tail = g_slist_last (theme->list_directories);

    GDir *d = g_dir_open(dir_name, 0, NULL);
    if (d) {
        const gchar *size_name;
        while ((size_name = g_dir_read_name(d)))
        {
            gchar *full_size_name = g_build_filename(dir_name, size_name, NULL);
            if (g_file_test(file_name, G_FILE_TEST_IS_DIR)) {
                int size, size2;
                if ((sscanf(size_name, "%dx%d", &size, &size2) == 2 && size == size2) ||
                    (sscanf(size_name, "%d", &size) == 1)) {
                    GDir *dSize = g_dir_open(full_size_name, 0, NULL);
                    if (dSize) {
                        const gchar *subdir_name;
                        while ((subdir_name = g_dir_read_name(dSize)))
                        {
                            IconThemeDir *dir = calloc(1, sizeof(IconThemeDir));
                            // value is like 48x48/apps
                            gchar *value = g_build_filename(size_name, subdir_name, NULL);
                            dir->name = strdup(value);
                            g_free(value);
                            dir->max_size = dir->min_size = dir->size = size;
                            dir->type = ICON_DIR_TYPE_FIXED;
                            g_slist_append_tail (theme->list_directories, d_tail, dir);
                        }
                        g_dir_close(dSize);
                    }
                }
            }
            g_free(full_size_name);
        }
        g_dir_close(d);
    }
}

IconTheme *load_theme_from_fs(const char *name, IconTheme *theme)
{
    gchar *dir_name = NULL;
    for (const GSList *location = get_icon_locations(); location; location = location->next) {
        gchar *path = (gchar *)location->data;
        dir_name = g_build_filename(path, name, NULL);
        if (g_file_test(dir_name, G_FILE_TEST_IS_DIR)) {
            if (!theme) {
                theme = make_theme(name);
            }
            load_theme_from_fs_dir(theme, dir_name);
        }
        g_free(dir_name);
        dir_name = NULL;
    }

    return theme;
}

IconTheme *load_theme(const char *name)
{
    // Look for name/index.theme in $HOME/.icons, /usr/share/icons, /usr/share/pixmaps (stop at the first found)
    // Parse index.theme -> list of IconThemeDir with attributes
    // Return IconTheme*

    if (name == NULL)
        return NULL;

    gchar *file_name = NULL;
    for (const GSList *location = get_icon_locations(); location; location = location->next)
    {
        gchar *path = (gchar *)location->data;
        file_name = g_build_filename(path, name, "index.theme", NULL);
        if (g_file_test(file_name, G_FILE_TEST_EXISTS))
            break;
        g_free (file_name);
        file_name = NULL;
    }

    IconTheme *theme;

    if (file_name) {
        theme = load_theme_from_index(file_name, name);
        g_free (file_name);
    } else
        theme = NULL;

    return load_theme_from_fs(name, theme);
}

void free_icon_theme(IconTheme *theme)
{
    if (!theme)
        return;
    free(theme->name);
    theme->name = NULL;
    free(theme->description);
    theme->description = NULL;
    for (GSList *l_inherits = theme->list_inherits; l_inherits; l_inherits = l_inherits->next) {
        free(l_inherits->data);
    }
    g_slist_free(theme->list_inherits);
    theme->list_inherits = NULL;
    for (GSList *l_dir = theme->list_directories; l_dir; l_dir = l_dir->next) {
        IconThemeDir *dir = (IconThemeDir *)l_dir->data;
        free(dir->name);
        free(l_dir->data);
    }
    g_slist_free(theme->list_directories);
    theme->list_directories = NULL;
}

void free_themes(IconThemeWrapper *wrapper)
{
    if (!wrapper)
        return;
    free(wrapper->icon_theme_name);
    for (GSList *l = wrapper->themes; l; l = l->next) {
        IconTheme *theme = (IconTheme *)l->data;
        free_icon_theme(theme);
        free(theme);
    }
    g_slist_free(wrapper->themes);
    for (GSList *l = wrapper->themes_fallback; l; l = l->next) {
        IconTheme *theme = (IconTheme *)l->data;
        free_icon_theme(theme);
        free(theme);
    }
    g_slist_free(wrapper->themes_fallback);
    g_slist_free_full(wrapper->_queued, free);
    free_cache(&wrapper->_cache);
    free(wrapper);
}

void test_launcher_read_theme_file()
{
    fprintf(stdout, YELLOW);
    IconTheme *theme = load_theme("oxygen");
    if (!theme) {
        fprintf(stderr, "tint2: Could not load theme\n");
        return;
    }
    fprintf(stderr, "tint2: Loaded theme: %s\n", theme->name);
    GSList *item = theme->list_inherits;
    while (item != NULL) {
        fprintf(stderr, "tint2: Inherits:%s\n", (char *)item->data);
        item = item->next;
    }
    item = theme->list_directories;
    while (item != NULL) {
        IconThemeDir *dir = item->data;
        fprintf(stderr, "tint2: Dir:%s Size=%d MinSize=%d MaxSize=%d Threshold=%d Type=%s\n",
               dir->name,
               dir->size,
               dir->min_size,
               dir->max_size,
               dir->threshold,
               dir->type    == ICON_DIR_TYPE_FIXED      ? "Fixed"
                : dir->type == ICON_DIR_TYPE_SCALABLE   ? "Scalable"
                : dir->type == ICON_DIR_TYPE_THRESHOLD  ? "Threshold"
                : "?????");
        item = item->next;
    }
    fprintf(stdout, RESET);
}

void load_themes_helper(const char *name, GSList **themes, GSList **queued)
{
    if (str_list_contains(*queued, name))
        return;

    GSList  *queue,
            *Q_tail, *t_tail;

    t_tail = g_slist_last (*themes);
    Q_tail = g_slist_last (*queued);
    g_slist_append_tail (*queued, Q_tail, strdup(name));
    queue = g_slist_append (NULL, strdup(name));

    // Load wrapper->themes
    while (queue) {
        char *queued_name = queue->data;
        queue = g_slist_remove(queue, queued_name);

        fprintf(stderr, "tint2:  '%s',", queued_name);
        IconTheme *theme = load_theme(queued_name);
        if (theme != NULL) {
            g_slist_append_tail (*themes, t_tail, theme);

            // Add inherited themes to queue
            GSList *inh_tail = NULL;

            for (   GSList *item = theme->list_inherits;
                    item; item = item->next )
            {
                char *parent = item->data;
                if (!str_list_contains(*queued, parent))
                {
                    g_slist_insert_after (queue, inh_tail, strdup(parent));
                    g_slist_append_tail (*queued, Q_tail, strdup(parent));
                }
            }
        }

        free (queued_name);
    }
    fprintf(stderr, "tint2: \n");

    g_slist_free_full (queue, free);
}

void load_default_theme(IconThemeWrapper *wrapper)
{
    if (wrapper->_themes_loaded)
        return;

    fprintf(stderr, GREEN "tint2: Loading icon theme %s:" RESET "\n", wrapper->icon_theme_name);

    load_themes_helper(wrapper->icon_theme_name, &wrapper->themes, &wrapper->_queued);
    load_themes_helper("hicolor",                &wrapper->themes, &wrapper->_queued);

    wrapper->_themes_loaded = TRUE;
}

void load_fallbacks(IconThemeWrapper *wrapper)
{
    if (wrapper->_fallback_loaded)
        return;

    fprintf(stderr, RED "tint2: Loading additional icon themes (this means your icon theme is incomplete)..." RESET "\n");

    // Load wrapper->themes_fallback
    const GSList *location;
    for (location = get_icon_locations(); location; location = location->next) {
        gchar *path = (gchar *)location->data;
        GDir *d = g_dir_open(path, 0, NULL);
        if (d) {
            const gchar *name;
            while ((name = g_dir_read_name(d))) {
                gchar *file_name = g_build_filename(path, name, "index.theme", NULL);
                if (g_file_test(file_name, G_FILE_TEST_EXISTS) && !g_file_test(file_name, G_FILE_TEST_IS_DIR)) {
                    load_themes_helper(name, &wrapper->themes_fallback, &wrapper->_queued);
                }
                g_free(file_name);
            }
            g_dir_close(d);
        }
    }

    wrapper->_fallback_loaded = TRUE;
}

gchar *get_icon_cache_path()
// Result is owned by function provider, don't free
// Use free_icon_cache_path() when need is expired
{
    if (! icon_cache_path)
        icon_cache_path = g_build_filename(g_get_user_cache_dir(), "tint2", "icon.cache", NULL);
    return icon_cache_path;
}

void icon_theme_common_cleanup()
{
    if (icon_locations)
        g_slist_free_full (icon_locations, free);;
    icon_locations = NULL;

    if (icon_cache_path)
        free (icon_cache_path);
    icon_cache_path = NULL;
}

void load_icon_cache(IconThemeWrapper *wrapper)
{
    if (wrapper->_cache.loaded)
        return;

    fprintf(stderr, GREEN "tint2: Loading icon theme cache..." RESET "\n");
    load_cache(&wrapper->_cache, get_icon_cache_path());
}

void save_icon_cache(IconThemeWrapper *wrapper)
{
    if (!wrapper || !wrapper->_cache.dirty)
        return;

    fprintf(stderr, GREEN "tint2: Saving icon theme cache..." RESET "\n");
    save_cache(&wrapper->_cache, get_icon_cache_path());
}

IconThemeWrapper *load_themes(const char *icon_theme_name)
{
    IconThemeWrapper *wrapper = calloc(1, sizeof(IconThemeWrapper));

    if (!icon_theme_name) {
        fprintf(stderr, "tint2: Missing icon_theme_name theme, default to 'hicolor'.\n");
        icon_theme_name = "hicolor";
    }

    wrapper->icon_theme_name = strdup(icon_theme_name);

    return wrapper;
}

int directory_matches_size(IconThemeDir *dir, int size)
{
    switch (dir->type) {
    case ICON_DIR_TYPE_FIXED:       return  dir->size == size;
    case ICON_DIR_TYPE_SCALABLE:    return  dir->min_size <= size &&
                                            dir->max_size >= size;
    default:                        return  dir->size - dir->threshold <= size &&
                                            dir->size + dir->threshold >= size;
    }
}

int directory_size_distance(IconThemeDir *dir, int size)
{
    switch (dir->type) {
    case ICON_DIR_TYPE_FIXED:       return  abs (dir->size - size);
    case ICON_DIR_TYPE_SCALABLE:    return  size < dir->min_size ? dir->min_size - size
                                        :   size > dir->max_size ? size - dir->max_size
                                        :   0;
    default:                        return  (size < dir->size - dir->threshold) ? dir->min_size - size
                                        :   (size > dir->size + dir->threshold) ? size - dir->max_size
                                        :   0;
    }
}

gint compare_theme_directories(gconstpointer a, gconstpointer b, gpointer size_query)
// Compares size_query distance to theme's distances
{
    int size = GPOINTER_TO_INT(size_query);
    const IconThemeDir *da = (const IconThemeDir *)a;
    const IconThemeDir *db = (const IconThemeDir *)b;
    return abs(da->size - size) - abs(db->size - size);
}

#define is_full_path(s)   ((Bool)(s[0] == '/'))
#define file_exists(path) ((Bool)g_file_test(path, G_FILE_TEST_EXISTS))

char *icon_path_from_full_path(const char *s)
{
    if (is_full_path(s) && file_exists(s))
        return strdup(s);
    char *expanded = expand_tilde(s);
    if (is_full_path(expanded) && file_exists(expanded))
        return expanded;
    free(expanded);
    return NULL;
}

char *get_icon_path_helper(GSList *themes, const char *icon_name, int size)
{
    if (!icon_name)
        return NULL;

    char *result = icon_path_from_full_path (icon_name);
    if (result)
        return result;

    int icon_name_len = strlen (icon_name);
    const GSList *basenames = get_icon_locations();

    // Best size match
    // Contrary to the freedesktop spec, we are not choosing the closest icon in size, but the next larger icon
    // otherwise the quality is worse when scaling up (for size 22, if you can choose 16 or 32, you're better with 32)
    // We do fallback to the closest size if we cannot find a larger or equal icon

    // These 3 variables are used for keeping the closest size match
    int     min_size    = INT_MAX;
    char    *best_name  = NULL;
    GSList  *best_theme = NULL;

    // These 3 variables are used for keeping the next larger match
    int     next_size   = -1;
    char    *next_name  = NULL;
    GSList  *next_theme = NULL;

    size_t file_name_size = 4096;
    char *file_name = calloc (file_name_size, 1);

    for (GSList *t_iter = themes; t_iter; t_iter = t_iter->next)
    {
        IconTheme *theme = t_iter->data;
        char *theme_name = theme->name;
        size_t theme_name_len = strlen (theme_name);

        if (debug_icons)
            fprintf (stderr, "tint2: Searching theme: %s\n", theme->name);

        // Sort by distances to given size
        theme->list_directories = g_slist_sort_with_data ( theme->list_directories,
                                                           compare_theme_directories,
                                                           GINT_TO_POINTER(size) );

        for (GSList *d_iter = theme->list_directories; d_iter; d_iter = d_iter->next)
        {
            IconThemeDir *dir = d_iter->data;
            char *dir_name = dir->name;
            int dir_name_len = strlen (dir_name);
            int dir_size_dist = directory_size_distance (dir, size);

            if (    // Closest match
                !   (dir_size_dist < min_size &&
                     (!best_theme || t_iter == best_theme))
                ||
                    // Next larger match
                !   (dir->size >= size &&
                     (next_size == -1 || dir->size < next_size) &&
                     (!next_theme || t_iter == next_theme))
            ) continue;

            if (debug_icons)
                fprintf (stderr, "tint2: Searching directory: %s\n", dir->name);

            const GSList *base;
            for (base = basenames; base; base = base->next)
            {
                char *base_name = (char *)base->data;
                size_t base_name_len = strlen (base_name);
                for (char **ext = icon_extensions; *ext; ext++)
                {
                    char *extension = *ext;

                    size_t fname_size_new = base_name_len + theme_name_len + dir_name_len +
                                            icon_name_len + strlen (extension)  + 100;
                    if (fname_size_new > file_name_size)
                        file_name = realloc (file_name, (file_name_size = fname_size_new));

                    // filename = directory/$(themename)/subdirectory/iconname.extension
                    snprintf (  file_name, (size_t)file_name_size - 1, "%s/%s/%s/%s%s",
                                base_name, theme_name, dir_name, icon_name, extension);
                    if (debug_icons)
                        fprintf (stderr, "tint2: Checking %s\n", file_name);

                    if (g_file_test (file_name, G_FILE_TEST_EXISTS))
                    {
                        if (debug_icons)
                            fprintf (stderr, "tint2: Found potential match: %s\n", file_name);

                        // Closest match
                        if ((!best_theme || t_iter == best_theme) &&
                            dir_size_dist < min_size )
                        {
                            if (best_name) {
                                free (best_name);
                                best_name = NULL;
                            }
                            best_name = strdup (file_name);
                            min_size = dir_size_dist;
                            best_theme = t_iter;

                            if (debug_icons)
                                fprintf (stderr, "tint2: best_name = %s; min_size = %d\n", best_name, min_size);
                        }
                        // Next larger match
                        if (dir->size >= size &&
                            (next_size == -1 || dir->size < next_size) &&
                            (!next_theme || t_iter == next_theme))
                        {
                            if (next_name) {
                                free (next_name);
                                next_name = NULL;
                            }
                            next_name = strdup (file_name);
                            next_size = dir->size;
                            next_theme = t_iter;

                            if (debug_icons)
                                fprintf (stderr, "tint2: next_name = %s; next_size = %d\n", next_name, next_size);
                        }
                    }
                }
            }
        }
    }
    free (file_name);
    file_name = NULL;
    if (next_name)
    {
        free (best_name);
        return next_name;
    }
    if (best_name)
        return best_name;

    // Look in unthemed icons
    {
        if (debug_icons)
            fprintf (stderr, "tint2: Searching unthemed icons\n");

        for (const GSList *base = basenames; base; base = base->next) {
            for (char **ext = icon_extensions; *ext; ext++)
            {
                char *base_name = (char *)base->data;
                char *extension = *ext;
                size_t file_name_size2 = strlen (base_name) + strlen (icon_name) + strlen (extension) + 100;
                file_name = calloc (file_name_size2, 1);

                // filename = directory/iconname.extension
                snprintf (file_name, file_name_size2 - 1, "%s/%s%s", base_name, icon_name, extension);
                if (debug_icons)
                    fprintf (stderr, "tint2: Checking %s\n", file_name);

                if (g_file_test (file_name, G_FILE_TEST_EXISTS))
                {
                    if (debug_icons)
                        fprintf (stderr, "tint2: Found %s\n", file_name);
                    return file_name;
                }
                else
                {
                    free (file_name);
                    file_name = NULL;
                }
            }
        }
    }

    return NULL;
}

char *get_icon_path_from_cache(IconThemeWrapper *wrapper, const char *icon_name, int size)
{
    if (!wrapper || !icon_name || strlen(icon_name) == 0)
        return NULL;

    load_icon_cache(wrapper);

    gchar *key = g_strdup_printf("%s\t%s\t%d", wrapper->icon_theme_name, icon_name, size);
    const gchar *value = get_from_cache(&wrapper->_cache, key);
    g_free(key);

    if (!value) {
        fprintf(stderr,
                YELLOW "Icon path not found in cache: theme = %s, icon = %s, size = %d" RESET "\n",
                wrapper->icon_theme_name,
                icon_name,
                size);
        return NULL;
    }

    if (!g_file_test(value, G_FILE_TEST_EXISTS))
        return NULL;

    // fprintf(stderr, "tint2: Icon path found in cache: theme = %s, icon = %s, size = %d, path = %s\n",
    // wrapper->icon_theme_name, icon_name, size, value);

    return strdup(value);
}

void add_icon_path_to_cache(IconThemeWrapper *wrapper, const char *icon_name, int size, const char *path)
{
    if (!wrapper || !icon_name || strlen(icon_name) == 0 || !path || strlen(path) == 0)
        return;

    fprintf(stderr,
            "Adding icon path to cache: theme = %s, icon = %s, size = %d, path = %s\n",
            wrapper->icon_theme_name,
            icon_name,
            size,
            path);

    load_icon_cache(wrapper);

    gchar *key = g_strdup_printf("%s\t%s\t%d", wrapper->icon_theme_name, icon_name, size);
    add_to_cache(&wrapper->_cache, key, path);
    g_free(key);
}

char *get_icon_path(IconThemeWrapper *wrapper, const char *icon_name, int size, gboolean use_fallbacks)
{
    if (debug_icons)
        fprintf(stderr,
                "Searching for icon %s with size %d, fallbacks %sallowed\n",
                icon_name,
                size,
                use_fallbacks ? "" : "not ");
    if (!wrapper) {
        if (debug_icons)
            fprintf(stderr,
                    "Icon search aborted, themes not loaded\n");
        return NULL;
    }

    if (!icon_name || strlen(icon_name) == 0)
        goto notfound;

    char *path = get_icon_path_from_cache(wrapper, icon_name, size);
    if (path) {
        if (debug_icons)
            fprintf(stderr,
                    "Icon found in cache: %s\n", path);
        return path;
    }

    load_default_theme(wrapper);

    icon_name = icon_name ? icon_name : DEFAULT_ICON;
    path = get_icon_path_helper(wrapper->themes, icon_name, size);
    if (path) {
        if (debug_icons)
            fprintf(stderr, "tint2: Icon found: %s\n", path);
        add_icon_path_to_cache(wrapper, icon_name, size, path);
        return path;
    }

    if (!use_fallbacks)
        goto notfound;
    fprintf(stderr, YELLOW "tint2: Icon not found in default theme: %s" RESET "\n", icon_name);
    load_fallbacks(wrapper);

    path = get_icon_path_helper(wrapper->themes_fallback, icon_name, size);
    if (path) {
        add_icon_path_to_cache(wrapper, icon_name, size, path);
        return path;
    }

notfound:
    fprintf(stderr, RED "tint2: Could not find icon '%s', using default." RESET "\n", icon_name);
    path = get_icon_path_helper(wrapper->themes, DEFAULT_ICON, size);
    if (path)
        return path;
    path = get_icon_path_helper(wrapper->themes_fallback, DEFAULT_ICON, size);
    return path;
}
