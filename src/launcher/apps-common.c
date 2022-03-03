/**************************************************************************
* Tint2 : .desktop file handling
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

/* http://standards.freedesktop.org/desktop-entry-spec/ */

#include "apps-common.h"
#include "common.h"
#include "test.h"

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>

int parse_dektop_line(char *line, char **key, char **value)
{
    if (*line == '=')
        return 0;

    char *p = strchr(line, '=');    
    if (!p)
        return 0;

    *p++ = '\0';
    if (!*p)
        return 0;

    *value = p;
    *key = line;
    return 1;
}

void expand_exec(DesktopEntry *entry, const char *path)
{
    // Expand % in exec
    // %i -> --icon Icon
    // %c -> Name
    // %k -> path
    if (entry->exec) {
        size_t buf_size = strlen(entry->exec)
                        + (entry->name ? strlen(entry->name) : 1)
                        + (entry->icon ? strlen(entry->icon) : 1) + 100;
        char *exec2 = calloc(buf_size, 1);
        char *p, *q;
        // p will never point to an escaped char
        for (p = entry->exec, q = exec2; *p; p++, q++) {
            *q = *p; // Copy
            if (*p == '\\') {
                p++, q++;
                // Copy the escaped char
                if (*p == '%') // For % we delete the backslash, i.e. write % over it
                    q--;
                *q = *p;
                if (!*p)
                    break;
                continue;
            }
            if (*p == '%') {
                p++;
                switch (*p) {
                case 'i':   if (entry->icon) {
                                snprintf(q, buf_size-1, "--icon '%s'", entry->icon);
                                char *old = q;
                                q += strlen_const("--icon ''");
                                q += strlen(entry->icon);
                                buf_size -= (size_t)(q - old);
                            }
                            break;
                case 'c':   if (entry->name) {
                                snprintf(q, buf_size-1, "'%s'", entry->name);
                                char *old = q;
                                q += strlen_const("''");
                                q += strlen(entry->name);
                                buf_size -= (size_t)(q - old);
                            } else {
                                snprintf(q, buf_size-1, "'%s'", path);
                                char *old = q;
                                q += strlen_const("''");
                                q += strlen(path);
                                buf_size -= (size_t)(q - old);
                            }
                            break;
                case 'F':
                case 'f':   snprintf(q, buf_size-1, "%c%c", '%', *p);
                            q += 2;
                            buf_size -= 2;
                            break;
                case '\0':  goto endloop;
                }
                q--; // To balance the q++ in the for
                     // Or delete the last % from q
                continue;
            }
        }
    endloop:
        *q = '\0';
        free(entry->exec);
        entry->exec = exec2;
    }
}

static char *df_opts_sv[] = {"Exec", "Icon", "NoDisplay", "Path", "StartupNotify", "Terminal"};
enum                        {i_Exec, i_Icon, i_NoDisplay, i_Path, i_StartupNotify, i_Terminal, DF_OPTIONS};

gboolean read_desktop_file_full_path(const char *path, DesktopEntry *entry)
{
    entry->name = entry->generic_name = entry->icon = entry->exec = entry->cwd = NULL;
    entry->hidden_from_menus = FALSE;
    entry->start_in_terminal = FALSE;
    entry->startup_notification = TRUE;

    FILE *fp = fopen(path, "rt");
    if (fp == NULL) {
        fprintf(stderr, "tint2: Could not open file %s\n", path);
        return FALSE;
    }

    const gchar **languages = (const gchar **)g_get_language_names();
    // lang_index_name is the index of the language for the best Name key in the language vector
    // lang_index_default is a constant that encodes the Name key without a language
    int lang_index_default = 1;
#define LANG_DBG 0
    if (LANG_DBG)
        fprintf(stderr, "tint2: Languages:");
    {   int i;
        for (i = 0; languages[i]; i++) {
            if (LANG_DBG)
                fprintf(stderr, " %s", languages[i]);
        }
        lang_index_default = i;
    }
    if (LANG_DBG)
        fputc('\n', stderr);
    // we currently do not know about any Name key at all, so use an invalid index
    int lang_index_name = lang_index_default + 1;
    int lang_index_generic_name = lang_index_default + 1;

    gboolean inside_desktop_entry = FALSE;
    char *line = NULL;
    size_t line_size;
    ssize_t len;
    while ((len = getline(&line, &line_size, fp)) >= 0) {
        if (len == 0)
            continue;
        str_strip_newline (line, len);
        if (line[0] == '[') {
            inside_desktop_entry = str_lequal_static (line, "[Desktop Entry]", len);
        }
        char *key, *value;
        if (inside_desktop_entry && parse_dektop_line(line, &key, &value))
        {
            #define _lang_to_prop(lang, prop)                                            \
            /* WARNING: only for this context, enveloping all localized keys */          \
            {                                                                            \
                if (key++[0] != '[' || value[-2] != ']')                                 \
                    continue;                                                            \
                value[-2] = '\0';                                                        \
                for (int i = 0; languages[i] && i < lang_index_name; i++)                \
                {                                                                        \
                    if (strcmp (key, languages[i]) == 0) {                               \
                        if (prop)                                                        \
                            free(prop);                                                  \
                        prop = strdup(value);                                            \
                        lang_index_name = i;                                             \
                    }                                                                    \
                }                                                                        \
            }
            /*****************************************************************/

            if (startswith_static (key, "Name")) {
                if (lang_index_name > lang_index_default && !key[strlen_const("Name")]) {
                    lang_index_name = lang_index_default;
                    entry->name = strdup(value);
                } else {
                    key += strlen_const("Name");
                    _lang_to_prop (key, entry->name);
                }
            } else if (startswith_static (key, "GenericName")) {
                if (lang_index_generic_name > lang_index_default && !key[strlen_const("GenericName")]) {
                    lang_index_generic_name = lang_index_default;
                    entry->generic_name = strdup(value);
                } else {
                    key += strlen_const("GenericName");
                    _lang_to_prop (key, entry->generic_name);
                }
            } else {
                switch (str_index (key, df_opts_sv, DF_OPTIONS))
                {
                    case i_Exec:            if (!entry->exec)
                                                entry->exec = strdup(value);
                                            break;
                    case i_Icon:            if (!entry->icon)
                                                entry->icon = strdup(value);
                                            break;
                    case i_NoDisplay:       entry->hidden_from_menus = strcasecmp(value, "true") == 0;
                                            break;
                    case i_Path:            if (!entry->cwd)
                                                entry->cwd = strdup(value);
                                            break;
                    case i_StartupNotify:   entry->startup_notification = strcasecmp(value, "true") == 0;
                                            break;
                    case i_Terminal:        entry->start_in_terminal = strcasecmp(value, "true") == 0;
                                            break;
                }
            }
            #undef _lang_to_prop
        }
    }
    fclose(fp);
    // From this point:
    // entry->name, entry->generic_name, entry->icon, entry->exec will never be empty strings (can be NULL though)

    expand_exec(entry, entry->path);

    free(line);
    return entry->exec != NULL;
}

gboolean read_desktop_file(const char *path, DesktopEntry *entry)
{
    gboolean success = FALSE;

    gchar *full_path = NULL;
    int full_path_avail = 0;

    entry->path = strdup(path);
    entry->name = entry->generic_name = entry->icon = entry->exec = entry->cwd = NULL;

    if (path[0] == '/')
        return read_desktop_file_full_path(path, entry);

    // Expect valid ID - according to freedesktop spec, it's unique inside location.
    // dashes in filename - undefined case

    size_t id_len = strlen (path);
    int dashes_n = 0;
    uint8_t dash_pos[64];

    for (char *p = strchr (path, '-'); p; p = strchr (p+1, '-'))
        dash_pos [dashes_n++] = p - path;

    // Dash to slash replacement variants are iterated with bitfield.
    // Although that's not likely - need avoid negative range to not break iteration
    if (dashes_n > sizeof(long) * 8 - 1)
        dashes_n = sizeof(long) * 8 - 1;

    for (const GSList *location = get_apps_locations(); location; location = location->next)
    {
        char *location_path = location->data;
        size_t dir_len = strlen (location_path);
        size_t full_path_len = dir_len + id_len + 2;

        if (full_path_len > full_path_avail)
            full_path = realloc (full_path, (full_path_avail = full_path_len));

        sprintf (full_path, "%s/%s", location_path, path);
        gchar *name_p = full_path + strlen (location_path) + 1;

        // Check different possible path variants for same ID
        // subdirs variant is preferred
        for (long dash_switch = (1 << dashes_n) - 1; dash_switch >= 0; dash_switch--)
        {
            for (int i = 0; i < dashes_n; i++)
                name_p [dash_pos [i]] = (1 << i) & dash_switch ? '/' : '-';

            //~ fprintf (stderr, "DEBUG: %s: ID to path variant: '%s'\n", __FUNCTION__, full_path);

            if ((g_file_test (full_path, G_FILE_TEST_IS_REGULAR)) &&
                (success = read_desktop_file_full_path (full_path, entry))
            ) goto end0;
        }
    }
end0:
    if (full_path)
        free (full_path);
    return success;
}

void free_desktop_entry(DesktopEntry *entry)
{
    free_and_null(entry->name);
    free_and_null(entry->generic_name);
    free_and_null(entry->icon);
    free_and_null(entry->exec);
    free_and_null(entry->path);
    free_and_null(entry->cwd);
}

void test_read_desktop_file()
{
    fprintf(stderr, YELLOW);
    DesktopEntry entry;
    read_desktop_file("/usr/share/applications/firefox.desktop", &entry);
    fprintf(stderr, "tint2: Name:%s GenericName:%s Icon:%s Exec:%s\n", entry.name, entry.generic_name, entry.icon, entry.exec);
    fprintf(stderr, RESET);
}

GSList *apps_locations = NULL;
// Do not free the result.
const GSList *get_apps_locations()
{
    if (apps_locations)
        return apps_locations;

    apps_locations = load_locations_from_env(apps_locations, "XDG_DATA_HOME", "applications", NULL);
    apps_locations = load_locations_from_dir(apps_locations, g_get_home_dir(), ".local/share/applications", NULL);
    apps_locations = load_locations_from_env(apps_locations, "XDG_DATA_DIRS", "applications", NULL);

    slist_append_uniq_dup(apps_locations, "/usr/local/share/applications", g_str_equal);
    slist_append_uniq_dup(apps_locations, "/usr/share/applications", g_str_equal);
    slist_append_uniq_dup(apps_locations, "/opt/share/applications", g_str_equal);

    return apps_locations;
}

// TESTS

STR_ARRAY_TEST_SORTED (df_opts_sv, ARRAY_SIZE(df_opts_sv));
