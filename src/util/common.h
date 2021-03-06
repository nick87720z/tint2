/**************************************************************************
* Common declarations
*
**************************************************************************/

#ifndef COMMON_H
#define COMMON_H

#define WM_CLASS_TINT "panel"
#define TINT2_PANGO_SLACK 0

#include <glib.h>
#include <Imlib2.h>
#include <pango/pangocairo.h>
#include "area.h"
#include "colors.h"

#define MAX3(a, b, c) MAX(MAX(a, b), c)
#define MIN3(a, b, c) MIN(MIN(a, b), c)
#define ATOB(x)       (atoi((x)) > 0)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define strlen_const(s) (ARRAY_SIZE(s) - 1)

#define str_has_const_suffix( str, suf, tmpint)                                          \
/* args: string, const substring, temporary integer */                                   \
(                                                                                        \
    tmpint = strlen( str),                                                               \
    tmpint >= strlen_const( suf)                                                         \
    && memcmp( str + tmpint - strlen_const( suf), suf, strlen_const( suf)) == 0          \
)

#define BUF_0TERM(s)                                                                     \
/** Write null byte to the end of static buffer */                                       \
( s[ strlen_const (s) ]='\0' )

#define STRBUF_AUTO(n,s)                                                                 \
/** Create static (auto) string buffer n with size                                       \
    Setting last byte to null */                                                         \
char n[s]={ [(s)-1]='\0' }

#define STRBUF_AUTO_PRINTF(n, ...)                                                       \
/** Print text to static string buffer, ensuring terminating null */                     \
do{ BUF_0TERM (n);                                                                       \
    snprintf(n, strlen_const(n), ##__VA_ARGS__);                                         \
} while (0)

#define g_slist_append_tail(list, tail, data)                                            \
/* Enhanced list grow, keeping track of tail */                                          \
{                                                                                        \
    void *_d_ = (data);                                                                  \
    tail = g_slist_append (tail, _d_);                                                   \
    if (list)   tail = tail->next;                                                       \
    else        list = tail;                                                             \
}

#define g_list_append_tail(list, tail, data)                                             \
/* Same for doubly-linked lists */                                                       \
{                                                                                        \
    void *d = (data);                                                                    \
    tail = g_list_append (tail, d);                                                      \
    if (list)   tail = tail->next;                                                       \
    else        list = tail;                                                             \
}

#define g_slist_insert_after(list, tail, d)                                              \
/* Grow list from start, keeping track of position */                                    \
{                                                                                        \
    void *_d_ = (d);                                                                     \
    GSList *new_node;                                                                    \
    if (! tail)                                                                          \
        tail = list = g_slist_prepend (list, _d_);                                       \
    else {                                                                               \
        new_node = g_slist_alloc ();                                                     \
        new_node->next = tail->next;                                                     \
        tail->next = new_node;                                                           \
        new_node->data = _d_;                                                            \
    }                                                                                    \
}

#define DEBUG_PRINT_LIST_CONTENT(l, pre, ...) {                                          \
    if (!(l))                                                                            \
        fprintf (stderr, "DEBUG: %s: " #l " empty\n", __FUNCTION__);                     \
    else {                                                                               \
        fprintf (stderr, "DEBUG: %s: " #pre " in " #l ":", __FUNCTION__);                \
        for (GSList *i = (l); i; i = i->next)                                            \
            fprintf (stderr, __VA_ARGS__);                                               \
        fprintf (stderr, "\n");                                                          \
    }                                                                                    \
}
#define DEBUG_PRINT_LIST_STRINGS(l) DEBUG_PRINT_LIST_CONTENT(l, strings, " '%s'", (char *)i->data)
#define DEBUG_PRINT_LIST_THEMES(l)  DEBUG_PRINT_LIST_CONTENT(l, themes,  " '%s'", ((IconTheme *)i->data)->name)

typedef enum MouseAction
// mouse actions
{
    NONE,
    CLOSE,
    TOGGLE,
    ICONIFY,
    SHADE,
    TOGGLE_ICONIFY,
    MAXIMIZE_RESTORE,
    MAXIMIZE,
    RESTORE,
    DESKTOP_LEFT,
    DESKTOP_RIGHT,
    NEXT_TASK,
    PREV_TASK,
    MOUSE_ACTIONS
} MouseAction;

#define ALL_DESKTOPS 0xFFFFFFFF

extern const char *home_dir;
extern const char *user_config_dir;
extern size_t home_dir_len;
extern size_t user_config_dir_len;

char *strdup_printf (size_t *len, const char *fmt,...);

void fetch_home_dir (void);
void fetch_user_config_dir (void);
void write_string(int fd, const char *s);
void log_string(int fd, const char *s);

void dump_backtrace(int log_fd);

void safe_sleep(int seconds);
// sleep() returns early when signals arrive. This function does not.

const char *signal_name(int sig);

const char *get_home_dir();

void copy_file(const char *path_src, const char *path_dest);
// Copies a file to another path

int str_index(const char *s, char *array[], int size);
// Finds string 's' in sorted strings array.
// Array must be sorted with strcmp-compatible comparison method.
// Returns string index if found, -1 otherwise

int str_array_sort_errors (char **a, int len);
// Checks strings sorted array for sort errors.
// Prints errors to stderr.
// Returns errors number (0 if no errors).

#define STR_ARRAY_TEST_SORTED(a, len)                                                    \
TEST( a ## _is_sorted )                                                                  \
{                                                                                        \
    int result = str_array_sort_errors (a, len);                                         \
    ASSERT_EQUAL(result, 0);                                                             \
}

int compare_strings(const void *a, const void *b);

int parse_line(const char *line, char **key, char **value);
/// Parses lines with the format 'key = value' into key and value.
/// Strips surrounding space from key and value.
/// Values may contain any graphical characters with spaces in the middle.
/// Returns 1 if both key and value could be read, zero otherwise
/// (single '=' per line produces empty strings for key and valye)
/// !!! returned strings are part of line and should not be used with free or realloc.

#define PARSED_OK    0x1
#define PARSED_KEY   0x2
#define PARSED_VALUE 0x4

int extract_values(
/// Delimits string to value tokens by replacing terminating delimiters with null byte.
/// Array must have space for at least 'tnum' pointers. If less tokens found, remaining
/// pointers are not modified.
/// Returns number of detected tokens.
    char *str,
    /// Null-terminated string to be delimited
    char **tvec,
    /// Array to write token pointers into.
    /// WARN: Pointers refer inside 'str', don't try to free or realloc them.
    unsigned tnum
    /// Maximum (positive) number of tokens to be returned.
);

pid_t tint_exec(
// Executes a command in a shell.
    const char *command,
    const char *dir,
    const char *tooltip,
    Time time,
    Area *area,
    int x,
    int y,
    gboolean terminal,
    gboolean startup_notification);

void tint_exec_no_sn(const char *command);
int setenvd(const char *name, const int value);

#define str_strip_newline(s, len) do {                                                   \
/* Strip newline character. Decrements len if newline was detected. */                   \
    if (s[--len] == '\n')                                                                \
        s[len] = '\0';                                                                   \
    else                                                                                 \
        len++;                                                                           \
} while(0)

char *expand_tilde(const char *s);
// Returns a copy of s in which "~" is expanded to the path to the user's home directory.
// The caller takes ownership of the string.

char *contract_tilde(const char *s);
// The opposite of expand_tilde: replaces the path to the user's home directory with "~".
// The caller takes ownership of the string.

// Color utils
int hex_char_to_int(char c);
int hex_to_rgb(char *hex, int *rgb);
void get_color(char *hex, double *rgb);

Imlib_Image load_image(const char *path, int cached);

void adjust_asb (   DATA32 *data, int w, int h,
// Adjusts the alpha/saturation/brightness on an ARGB image.
                    float alpha_adjust,     // multiplicative:
                                            //  0 = full transparency
                                            //  1 = no adjustment
                                            //  2 = twice the current opacity
                    float satur_adjust,     // additive:
                                            // -1 = full grayscale
                                            //  0 = no adjustment
                                            //  1 = full color
                    float bright_adjust);   // additive:
                                            // -1 = black
                                            //  0 = no adjustment
                                            //  1 = white
Imlib_Image adjust_img   (Imlib_Image original, int alpha, int saturation, int brightness);
void        adjust_color (Color      *color,    int alpha, int saturation, int brightness);
// Ditto

void create_heuristic_mask(DATA32 *data, int w, int h);

void render_image( Imlib_Image image, Drawable d, int x, int y);
// Renders the current Imlib image to a drawable. Wrapper around imlib_render_image_on_drawable.

void get_text_size2(const PangoFontDescription *font,
                    int *height,
                    int *width,
                    int available_height,
                    int available_width,
                    const char *text,
                    int text_len,
                    PangoWrapMode wrap,
                    PangoEllipsizeMode ellipsis,
                    PangoAlignment alignment,
                    gboolean markup,
                    double scale);

gboolean layout_set_markup_strip_colors(PangoLayout *layout, const char *markup);
void draw_text(PangoLayout *layout, cairo_t *c, int posx, int posy, Color *color, PangoLayout *shadow_layout);

void draw_rect(cairo_t *c, double x, double y, double w, double h, double r, int corner_mask);
// Draws a rounded rectangle

void clear_pixmap(Pixmap p, int x, int y, int w, int h);
// Clears the pixmap (with transparent color)

void close_all_fds();

GSList *load_locations_from_dir(GSList *locations, const char *dir, ...);
GSList *load_locations_from_env(GSList *locations, const char *var, ...);

GSList *slist_append_uniq(
// Search ref in list. If not found - append to list using assign function.
    GSList          *list,          // Destination list
    gconstpointer   ref,            // Data to be appended
    GCompareFunc    eq,             // Comparison callback for uniqueness check (e.g. strcmp)
    void* (*assign)(const void *)   // Callback to process data before append (e.g. strdup)
                                    // Set to NULL to make ref used directly
);

#define slist_append_uniq_direct(list, ref, eq)                                          \
/* Convenience wrapper for slist_append_uniq_func(), appending ref value directly */     \
(                                                                                        \
    slist_append_uniq(list, ref, eq, NULL)                                               \
)

#define slist_append_uniq_dup(list, ref, eq)                                             \
/* Convenience wrapper for slist_append_uniq_func(), appending string duplicate */       \
(                                                                                        \
    slist_append_uniq(list, ref, eq, (void* (*)(const void *))strdup)                    \
)

gint cmp_ptr(gconstpointer a, gconstpointer b);
// A trivial pointer comparator.

GString *tint2_g_string_replace(GString *s, const char *from, const char *to);

void get_image_mean_color(const Imlib_Image image, Color *mean_color);

void dump_image_data(const char *file_name, const char *name);

#define str_lequal_static(s1, s2, len)                                                   \
(                                                                                        \
    len == strlen_const(s2) && memcmp (s1, s2, sizeof(s2)) == 0                          \
)

#define strcpy_static(dst, src)                                                          \
    memcpy(dst, src, sizeof(src))

#define strdup_static( dst, src) (                                                       \
    dst = malloc( sizeof( src)),                                                         \
    strcpy_static( dst, src),                                                            \
    src                                                                                  \
)

#define startswith_static(s1, s2)                                                        \
(strncmp ((s1), (s2), strlen_const(s2)) == 0)

#define free_and_null(p) do {                                                            \
    free(p);                                                                             \
    p = NULL;                                                                            \
} while (0)

#define fd_set_unset_fd( fds, fdn, fd)                                                   \
/* Tries to clear fd from fds. On success - also decrements fdn.                         \
 * Returns 0 if fd is negative or not in fds. */                                         \
    (fd >= 0 && FD_ISSET( fd, fds) ? (FD_CLR( fd, fds), (*fdn)--, 1) : 0)

#if !GLIB_CHECK_VERSION(2, 33, 4)
GList *g_list_copy_deep(GList *list, GCopyFunc func, gpointer user_data);
#endif

#if !GLIB_CHECK_VERSION(2, 38, 0)
#define g_assert_null(expr) g_assert((expr) == NULL)
#endif

#if !GLIB_CHECK_VERSION(2, 40, 0)
#define g_assert_nonnull(expr) g_assert((expr) != NULL)
#endif

#endif
