/**************************************************************************
*
* Tint2 : common windows function
*
* Copyright (C) 2007 Pål Staurland (staura@gmail.com)
* Modified (C) 2008 thierry lorthiois (lorthiois@bbsoft.fr) from Omega distribution
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
#include <X11/extensions/Xrender.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "common.h"
#include "server.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <errno.h>
#include <dirent.h>
#if !defined(__OpenBSD__)
#include <wordexp.h>
#endif

#ifdef HAVE_RSVG
#include <librsvg/rsvg.h>
#endif

#include "../panel.h"
#include "timer.h"
#include "signals.h"
#include "bt.h"
#include "strnatcmp.h"
#include "common.h"

const char *home_dir = NULL;
size_t home_dir_len = 0;

const char *user_config_dir = NULL;
size_t user_config_dir_len = 0;

char *strdup_printf (size_t *len, const char *fmt,...)
{
    va_list ap1, ap2;
    va_start( ap1, fmt);
    va_copy( ap2, ap1);

    size_t _len = vsnprintf( NULL, 0, fmt, ap1);
    if (len)
        *len = _len;

    char *result = malloc( _len + 1);
    vsprintf( result, fmt, ap2);

    va_end( ap1);
    va_end( ap2);

    return result;
}

void fetch_home_dir (void) {
    if (! home_dir) {
        home_dir = g_get_home_dir();
        home_dir_len = strlen (home_dir);
    }
}

void fetch_user_config_dir (void) {
    if (! user_config_dir) {
        user_config_dir = g_get_user_config_dir();
        user_config_dir_len = strlen (user_config_dir);
    }
}

void write_data(int fd, const char *s, int len)
// Reliable write() wrapper
{
    while (len > 0) {
        int count = write(fd, s, len);
        if (count >= 0) {
            s += count;
            len -= count;
        } else
            break;
    }
}

void write_string(int fd, const char *s)
{
    write_data (fd, s, strlen(s));
}

void log_string(int fd, const char *s)
{
    write_string(2, s);
    write_string(fd, s);
}

void dump_backtrace(int log_fd)
{
    struct backtrace bt;
    get_backtrace(&bt, 1);
    log_string(log_fd, "\n" YELLOW "Backtrace:" RESET "\n");
    for (size_t i = 0; i < bt.frame_count; i++)
    {
        log_string(log_fd, bt.frames[i].name);
        log_string(log_fd, "\n");
    }
}

// sleep() returns early when signals arrive. This function does not.
void safe_sleep(int seconds)
{
    double t = get_time();
    while (1) {
        if (get_time() > t + seconds)
            return;
        sleep(1);
    }
}

const char *signal_name(int sig)
{
    switch (sig) {
    case SIGHUP:    return "SIGHUP: Hangup (POSIX).";
    case SIGINT:    return "SIGINT: Interrupt (ANSI).";
    case SIGQUIT:   return "SIGQUIT: Quit (POSIX).";
    case SIGILL:    return "SIGILL: Illegal instruction (ANSI).";
    case SIGTRAP:   return "SIGTRAP: Trace trap (POSIX).";
    case SIGABRT:   return "SIGABRT/SIGIOT: Abort (ANSI) / IOT trap (4.2 BSD).";
    case SIGBUS:    return "SIGBUS: BUS error (4.2 BSD).";
    case SIGFPE:    return "SIGFPE: Floating-point exception (ANSI).";
    case SIGKILL:   return "SIGKILL: Kill, unblockable (POSIX).";
    case SIGUSR1:   return "SIGUSR1: User-defined signal 1 (POSIX).";
    case SIGSEGV:   return "SIGSEGV: Segmentation violation (ANSI).";
    case SIGUSR2:   return "SIGUSR2: User-defined signal 2 (POSIX).";
    case SIGPIPE:   return "SIGPIPE: Broken pipe (POSIX).";
    case SIGALRM:   return "SIGALRM: Alarm clock (POSIX).";
    case SIGTERM:   return "SIGTERM: Termination (ANSI).";
    // case SIGSTKFLT: return "SIGSTKFLT: Stack fault.";
    case SIGCHLD:   return "SIGCHLD: Child status has changed (POSIX).";
    case SIGCONT:   return "SIGCONT: Continue (POSIX).";
    case SIGSTOP:   return "SIGSTOP: Stop, unblockable (POSIX).";
    case SIGTSTP:   return "SIGTSTP: Keyboard stop (POSIX).";
    case SIGTTIN:   return "SIGTTIN: Background read from tty (POSIX).";
    case SIGTTOU:   return "SIGTTOU: Background write to tty (POSIX).";
    case SIGURG:    return "SIGURG: Urgent condition on socket (4.2 BSD).";
    case SIGXCPU:   return "SIGXCPU: CPU limit exceeded (4.2 BSD).";
    case SIGXFSZ:   return "SIGXFSZ: File size limit exceeded (4.2 BSD).";
    case SIGVTALRM: return "SIGVTALRM: Virtual alarm clock (4.2 BSD).";
    case SIGPROF:   return "SIGPROF: Profiling alarm clock (4.2 BSD).";
    // case SIGPWR: return "SIGPWR: Power failure restart (System V).";
    case SIGSYS:    return "SIGSYS: Bad system call.";
    }
    static char s[64];
    STRBUF_AUTO_PRINTF(s, "SIG=%d: Unknown", sig);
    return s;
}

const char *get_home_dir()
{
    const char *s = getenv("HOME");
    if (s)
        return s;
    struct passwd *pw = getpwuid(getuid());
    
    return pw ? pw->pw_dir : NULL;
}

void copy_file(const char *path_src, const char *path_dest)
{
    if (g_str_equal(path_src, path_dest))
        return;

    FILE *file_src, *file_dest;
    char buffer[4096];
    int nb;

    if ( !(file_src = fopen(path_src, "rb")) )
        goto r0;

    if ( !(file_dest = fopen(path_dest, "wb")) )
        goto r1;

    while ((nb = fread(buffer, 1, sizeof(buffer), file_src)) > 0)
        if (nb != fwrite(buffer, 1, nb, file_dest))
            fprintf(stderr, "tint2: Error while copying file %s to %s\n", path_src, path_dest);

    fclose(file_dest);
r1: fclose(file_src);
r0: return;
}

static int cmp_strp (const void *p1, const void *p2) {
    return strcmp( *(char **)p1, *(char **)p2 );
}

int str_index(const char *s, char *array[], int size) {
    char **p = bsearch (&s, array, size, sizeof(*array), cmp_strp);
    return p ? p - array : -1;
}

int str_array_sort_errors (char **a, int len)
{
    int result = 0;

    if (len > 1)
        for (int i = 1; i < len; i++)
            if (strcmp (a[i], a[i-1]) < 0)
            {
                printf ("sort error: [%i]=\"%s\" \t[%i]=\"%s\"\n", i-1, a[i-1], i, a[i]);
                result++;
            }
    return result;
}

int compare_strings(const void *a, const void *b)
{
    return strnatcasecmp((const char *)a, (const char *)b);
}

#define SPN_SPACE " \t\r\n"

int parse_line(const char *line, char **key, char **value)
{
    char *a, *b, c;
    int result = PARSED_OK;

    /* fail if comment or empty */
    if ((c = line[0]) == '#' || c == '\n' || c == '\0')
        return 0;

    /* skip leading whitespace */
    line += strspn(line, SPN_SPACE);
    if ( !(a = strchr(line, '=')) )
        return 0;

    /* set key null terminator */
    if (a == line)
        a[0] = '\0';
    else {
        b = a;
        while (g_ascii_isspace(* --b));
        *(++b) = '\0';
        if (b != a)
            result |= PARSED_KEY;
    }

    /* + skip value leading space */
    a++, a += strspn(a, SPN_SPACE);

    *key = (char *)line;
    *value = a;

    if ((c = a[0]) != '\n' || c != '\0') {
        /* drop '\n' if found
         * along with trailing space */
        b = strchr(a + 1, '\0');
        if (b[-1] == '\n')
            b--;
        while (g_ascii_isspace(* --b));
        b[1] = '\0';
    }

    return a != b   ? result | PARSED_VALUE
                    : result;
}

extern char *config_path;

int setenvd(const char *name, const int value)
{
    char buf[256];
    STRBUF_AUTO_PRINTF (buf, "%d", value);
    return setenv(name, buf, 1);
}

#ifndef TINT2CONF
pid_t tint_exec(const char *command,
                const char *dir,
                const char *tooltip,
                Time time,
                Area *area,
                int x,
                int y,
                gboolean terminal,
                gboolean startup_notification)
{
    if (!command || strlen(command) == 0)
        return -1;

    if (area) {
        Panel *panel = (Panel *)area->panel;

        int aligned_x, aligned_y, aligned_x1, aligned_y1, aligned_x2, aligned_y2;
        int panel_x1, panel_x2, panel_y1, panel_y2;
        if (panel_horizontal) {
            aligned_x1 = aligned_x2 = panel->posx;
            if (area_is_first(area))
                aligned_x1 += area->posx       ,
                aligned_x2 += panel->area.width,
                aligned_x = aligned_x1;
            else
                aligned_x2 += area->posx + area->width                   ,
                aligned_x = !area_is_last(area) ? aligned_x1 : aligned_x2;
            panel_x2 = (panel_x1 = panel->posx) + panel->area.width;

            aligned_y = panel->posy;
            if (panel_position & BOTTOM)
                aligned_y += panel->area.height;
            panel_y1 = panel_y2 = aligned_y1 = aligned_y2 = aligned_y;
        } else {
            aligned_y1 = aligned_y2 = panel->posy;
            if (area_is_first(area))
                aligned_y1 += area->posy        ,
                aligned_y2 += panel->area.height,
                aligned_y = aligned_y1;
            else
                aligned_y2 += area->posy + area->height                  ,
                aligned_y = !area_is_last(area) ? aligned_y1 : aligned_y2;
            panel_y2 = (panel_y1 = panel->posy) + panel->area.height;

            aligned_x = panel->posx;
            if (panel_position & RIGHT)
                aligned_x += panel->area.width;
            panel_x1 = panel_x2 = aligned_x1 = aligned_x2 = aligned_x;
        }

        setenv("TINT2_CONFIG", config_path, 1);
        setenvd("TINT2_BUTTON_X", x);
        setenvd("TINT2_BUTTON_Y", y);
        setenvd("TINT2_BUTTON_W", area->width);
        setenvd("TINT2_BUTTON_H", area->height);
        setenvd("TINT2_BUTTON_ALIGNED_X", aligned_x);
        setenvd("TINT2_BUTTON_ALIGNED_Y", aligned_y);
        setenvd("TINT2_BUTTON_ALIGNED_X1", aligned_x1);
        setenvd("TINT2_BUTTON_ALIGNED_Y1", aligned_y1);
        setenvd("TINT2_BUTTON_ALIGNED_X2", aligned_x2);
        setenvd("TINT2_BUTTON_ALIGNED_Y2", aligned_y2);
        setenvd("TINT2_BUTTON_PANEL_X1", panel_x1);
        setenvd("TINT2_BUTTON_PANEL_Y1", panel_y1);
        setenvd("TINT2_BUTTON_PANEL_X2", panel_x2);
        setenvd("TINT2_BUTTON_PANEL_Y2", panel_y2);
    } else
        setenv("TINT2_CONFIG", config_path, 1);

    if (!command)
        return -1;

    if (!tooltip)
        tooltip = command;

#if HAVE_SN
    SnLauncherContext *ctx = NULL;
    if (startup_notifications && startup_notification && time) {
        ctx = sn_launcher_context_new(server.sn_display, server.screen);
        sn_launcher_context_set_name(ctx, tooltip);
        sn_launcher_context_set_description(ctx, "Application launched from tint2");
        sn_launcher_context_set_binary_name(ctx, command);
        sn_launcher_context_initiate(ctx, "tint2", command, time);
    }
#endif /* HAVE_SN */
    pid_t pid;
    pid = fork();
    if (pid < 0)
        fprintf(stderr, "tint2: Could not fork\n");
    else if (pid == 0) {
// Child process
#if HAVE_SN
        if (startup_notifications && startup_notification && time)
            sn_launcher_context_setup_child_process(ctx);
#endif // HAVE_SN
        // Allow children to exist after parent destruction
        setsid();
        // Run the command
        if (dir)
            if (chdir(dir) != 0)
                fprintf(stderr, "tint2: failed to chdir to %s\n", dir);
        close_all_fds();
        reset_signals();
        if (terminal) {
#if !defined(__OpenBSD__)
            fprintf(stderr, "tint2: executing in x-terminal-emulator: %s\n", command);
            wordexp_t words;
            words.we_offs = 2;
            if (wordexp(command, &words, WRDE_DOOFFS | WRDE_SHOWERR) == 0) {
                words.we_wordv[0] = (char *)"x-terminal-emulator";
                words.we_wordv[1] = (char *)"-e";
                execvp("x-terminal-emulator", words.we_wordv);
            }
#endif
            fprintf(stderr,
                    "tint2: could not execute command in x-terminal-emulator: %s, executting in shell\n",
                    command);
        }
        execlp("sh", "sh", "-c", command, NULL);
        fprintf(stderr, "tint2: Failed to execute %s\n", command);
#if HAVE_SN
        if (startup_notifications && startup_notification && time)
            sn_launcher_context_unref(ctx);
#endif // HAVE_SN
        _exit(1);
    } else {
// Parent process
#if HAVE_SN
        if (startup_notifications && startup_notification && time)
            g_tree_insert(server.pids, GINT_TO_POINTER(pid), ctx);
#endif // HAVE_SN
    }

    unsetenv("TINT2_CONFIG");
    unsetenv("TINT2_BUTTON_X");
    unsetenv("TINT2_BUTTON_Y");
    unsetenv("TINT2_BUTTON_W");
    unsetenv("TINT2_BUTTON_H");
    unsetenv("TINT2_BUTTON_ALIGNED_X");
    unsetenv("TINT2_BUTTON_ALIGNED_Y");
    unsetenv("TINT2_BUTTON_ALIGNED_X1");
    unsetenv("TINT2_BUTTON_ALIGNED_Y1");
    unsetenv("TINT2_BUTTON_ALIGNED_X2");
    unsetenv("TINT2_BUTTON_ALIGNED_Y2");
    unsetenv("TINT2_BUTTON_PANEL_X1");
    unsetenv("TINT2_BUTTON_PANEL_Y1");
    unsetenv("TINT2_BUTTON_PANEL_X2");
    unsetenv("TINT2_BUTTON_PANEL_Y2");

    return pid;
}

void tint_exec_no_sn(const char *command)
{
    tint_exec(command, NULL, NULL, 0, NULL, 0, 0, FALSE, FALSE);
}
#endif

char *expand_tilde(const char *s)
{
    if (s[0] == '~' && (s[1] == '\0' || s[1] == '/') && (fetch_home_dir(), home_dir)) {
        size_t buf_size = home_dir_len + strlen(s);
        char *result = calloc (buf_size, 1);
        memcpy (result,                home_dir, home_dir_len);
        memcpy (result + home_dir_len, s + 1,    buf_size - home_dir_len);
        return result;
    } else
        return strdup(s);
}

char *contract_tilde(const char *s)
{
    fetch_home_dir ();
    if (!home_dir)
        return strdup(s);

    if (( strncmp (s, home_dir, home_dir_len) == 0            ) &&
        ( s [home_dir_len] == '\0' || s [home_dir_len] == '/' ))
    {
        size_t buf_size = strlen (s + home_dir_len) + 2;
        char *result = calloc (buf_size, 1);
        result[0] = '~';
        memcpy (result + 1, s + home_dir_len, buf_size - 1);
        return result;
    } else
        return strdup(s);
}

int hex_char_to_int(char c)
{
    switch (c) {
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'a': return 10;
    case 'b': return 11;
    case 'c': return 12;
    case 'd': return 13;
    case 'e': return 14;
    case 'f': return 15;
    case 'A': return 10;
    case 'B': return 11;
    case 'C': return 12;
    case 'D': return 13;
    case 'E': return 14;
    case 'F': return 15;
    default:  return 0;
    }
}

int hex_to_rgb(char *hex, int *rgb)
{
    if (hex == NULL || hex[0] != '#')
        return (0);

    int len = strlen(++hex);
    switch (len) {
    case 3:
        rgb[0] = hex_char_to_int(hex[0]) << 12;
        rgb[1] = hex_char_to_int(hex[1]) << 12;
        rgb[2] = hex_char_to_int(hex[2]) << 12;
        return 1;
    case 6:
        rgb[0] = hex_char_to_int(hex[0]) << 12 | hex_char_to_int(hex[1]) << 8;
        rgb[1] = hex_char_to_int(hex[2]) << 12 | hex_char_to_int(hex[3]) << 8;
        rgb[2] = hex_char_to_int(hex[4]) << 12 | hex_char_to_int(hex[5]) << 8;
        return 1;
    case 12:
        rgb[0] = hex_char_to_int(hex[0]) << 12 | hex_char_to_int(hex[1]) << 8 | hex_char_to_int(hex[2])  << 4 | hex_char_to_int(hex[3]);
        rgb[1] = hex_char_to_int(hex[4]) << 12 | hex_char_to_int(hex[5]) << 8 | hex_char_to_int(hex[6])  << 4 | hex_char_to_int(hex[7]);
        rgb[2] = hex_char_to_int(hex[8]) << 12 | hex_char_to_int(hex[9]) << 8 | hex_char_to_int(hex[10]) << 4 | hex_char_to_int(hex[11]);
        return 1;
    default:
        return 0;
    }
}

void get_color(char *hex, double *rgb)
{
    int rgbi[3];
    if (hex_to_rgb(hex, rgbi))
        rgb[0] = rgbi[0] / ((1 << 16) - 1.0),
        rgb[1] = rgbi[1] / ((1 << 16) - 1.0),
        rgb[2] = rgbi[2] / ((1 << 16) - 1.0);
    else
        rgb[0] = rgb[1] = rgb[2] = 0;
}

int extract_values(char *str, char **tvec, unsigned tnum)
{
    int ti = 0;
    char *saveptr, *tok;
    for (tok = strtok_r (str, " ", &saveptr);
         ti < tnum && tok;
         tok = strtok_r (NULL, " ", &saveptr), ti++)
    {
        *tvec++ = tok;
    }
    return ti;
}

void adjust_asb(DATA32 *data, int w, int h, float alpha_adjust, float satur_adjust, float bright_adjust)
{
    for (int id = 0; id < w * h; id++)
    {
        unsigned int argb = data[id];
        int a = (argb >> 24) & 0xff;
        // transparent => nothing to do.
        if (a == 0)
            continue;

        int r = (argb >> 16) & 0xff;
        int g = (argb >>  8) & 0xff;
        int b = (argb      ) & 0xff;

        // Convert RGB to HSV
        int cmax = MAX3(r, g, b);
        int cmin = MIN3(r, g, b);
        int delta = cmax - cmin;
        float brightness = cmax / 255.0f;
        float saturation = (cmax != 0) ? delta / (float)cmax : 0;
        float hue;
        if (saturation == 0) hue = 0;
        else {
            float redc   = (cmax - r) / (float)delta;
            float greenc = (cmax - g) / (float)delta;
            float bluec  = (cmax - b) / (float)delta;
            hue = (r == cmax) ? bluec - greenc
                : (g == cmax) ? redc - bluec + 2.0f
                :               greenc - redc + 4.0f;
            hue /= 6.0f;
            if (hue < 0) hue += 1.0f;
        }

        // Adjust H, S
        saturation += satur_adjust;
        saturation = CLAMP(saturation, 0.0, 1.0);

        a *= alpha_adjust;
        a = CLAMP(a, 0, 255);

        // Convert HSV to RGB
        if (saturation == 0)
            r = g = b = (int)(brightness * 255.0f + 0.5f);
        else {
            float h2 = (hue - (int)hue) * 6.0f;
            float f = h2 - (int)(h2);
            float p = brightness * (1.0f - saturation);
            float q = brightness * (1.0f - saturation * f);
            float t = brightness * (1.0f - (saturation * (1.0f - f)));

            switch ((int)h2) {
            case 0: r = (int)(brightness * 255.0f + 0.5f);
                    g = (int)(t * 255.0f + 0.5f);
                    b = (int)(p * 255.0f + 0.5f);
                    break;
            case 1: r = (int)(q * 255.0f + 0.5f);
                    g = (int)(brightness * 255.0f + 0.5f);
                    b = (int)(p * 255.0f + 0.5f);
                    break;
            case 2: r = (int)(p * 255.0f + 0.5f);
                    g = (int)(brightness * 255.0f + 0.5f);
                    b = (int)(t * 255.0f + 0.5f);
                    break;
            case 3: r = (int)(p * 255.0f + 0.5f);
                    g = (int)(q * 255.0f + 0.5f);
                    b = (int)(brightness * 255.0f + 0.5f);
                    break;
            case 4: r = (int)(t * 255.0f + 0.5f);
                    g = (int)(p * 255.0f + 0.5f);
                    b = (int)(brightness * 255.0f + 0.5f);
                    break;
            case 5: r = (int)(brightness * 255.0f + 0.5f);
                    g = (int)(p * 255.0f + 0.5f);
                    b = (int)(q * 255.0f + 0.5f);
                    break;
            }
        }

        r += bright_adjust * 255;
        g += bright_adjust * 255;
        b += bright_adjust * 255;

        r = CLAMP(r, 0, 255);
        g = CLAMP(g, 0, 255);
        b = CLAMP(b, 0, 255);

        argb = a;
        argb = (argb << 8) + r;
        argb = (argb << 8) + g;
        argb = (argb << 8) + b;
        data[id] = argb;
    }
}

void create_heuristic_mask(DATA32 *data, int w, int h)
{
    // first we need to find the mask color, therefore we check all 4 edge pixel and take the color which
    // appears most often (we only need to check three edges, the 4th is implicitly clear)
    unsigned int topLeft = data[0], topRight = data[w - 1], bottomLeft = data[w * h - w], bottomRight = data[w * h - 1];
    int max = (topLeft == topRight) + (topLeft == bottomLeft) + (topLeft == bottomRight);
    int maskPos = 0;
    if (max < (topRight == topLeft) + (topRight == bottomLeft) + (topRight == bottomRight)) {
        max = (topRight == topLeft) + (topRight == bottomLeft) + (topRight == bottomRight);
        maskPos = w - 1;
    }
    if (max < (bottomLeft == topRight) + (bottomLeft == topLeft) + (bottomLeft == bottomRight))
        maskPos = w * h - w;

    // now mask out every pixel which has the same color as the edge pixels
    unsigned char *udata = (unsigned char *)data;
    unsigned char b = udata[4 * maskPos];
    unsigned char g = udata[4 * maskPos + 1];
    unsigned char r = udata[4 * maskPos + 1];
    for (int i = 0; i < h * w; ++i)
    {
        if (b - udata[0] == 0 && g - udata[1] == 0 && r - udata[2] == 0)
            udata[3] = 0;
        udata += 4;
    }
}

void render_image(Drawable d, int x, int y)
{
    if (!server.real_transparency) {
        imlib_context_set_blend(1);
        imlib_context_set_drawable(d);
        imlib_render_image_on_drawable(x, y);
        return;
    }

    int w = imlib_image_get_width() ,
        h = imlib_image_get_height();

    Pixmap pixmap = XCreatePixmap(server.display, server.root_win, w, h, 32);
    imlib_context_set_drawable(pixmap);
    imlib_context_set_blend(0);
    imlib_render_image_on_drawable(0, 0);

    Pixmap mask = XCreatePixmap(server.display, server.root_win, w, h, 32);
    imlib_context_set_drawable(mask);
    imlib_context_set_blend(0);
    imlib_render_image_on_drawable(0, 0);

    Picture pict = XRenderCreatePicture(server.display, pixmap,
                                        XRenderFindStandardFormat(server.display, PictStandardARGB32),
                                        0, 0);
    Picture pict_drawable = XRenderCreatePicture (server.display, d,
                                                  XRenderFindVisualFormat(server.display, server.visual),
                                                  0, 0);
    Picture pict_mask = XRenderCreatePicture (server.display, mask,
                                              XRenderFindStandardFormat(server.display, PictStandardARGB32),
                                              0, 0);
    XRenderComposite(server.display, PictOpOver, pict, pict_mask, pict_drawable, 0, 0, 0, 0, x, y, w, h);

    XRenderFreePicture(server.display, pict_mask);
    XRenderFreePicture(server.display, pict_drawable);
    XRenderFreePicture(server.display, pict);
    XFreePixmap(server.display, mask);
    XFreePixmap(server.display, pixmap);
}

gboolean is_color_attribute(PangoAttribute *attr, gpointer user_data)
{
    switch (attr->klass->type) {
        case PANGO_ATTR_FOREGROUND:
        case PANGO_ATTR_BACKGROUND:
        case PANGO_ATTR_UNDERLINE_COLOR:
        case PANGO_ATTR_STRIKETHROUGH_COLOR:
        case PANGO_ATTR_FOREGROUND_ALPHA:
        case PANGO_ATTR_BACKGROUND_ALPHA:
            return 1;
        default:
            return 0;
    }
}

gboolean layout_set_markup_strip_colors(PangoLayout *layout, const char *markup)
{
    PangoAttrList *attrs = NULL;
    char *text = NULL;
    GError *error = NULL;
    if (!pango_parse_markup(markup, -1, 0, &attrs, &text, NULL, &error)) {
        g_error_free(error);
        return FALSE;
    }

    pango_layout_set_text(layout, text, -1);
    g_free(text);

    pango_attr_list_filter(attrs, is_color_attribute, NULL);
    pango_layout_set_attributes(layout, attrs);
    pango_attr_list_unref(attrs);
    return TRUE;
}

void draw_shadow(cairo_t *c, int posx, int posy, PangoLayout *shadow_layout)
{
    const int shadow_size = 3;
    const double shadow_edge_alpha = 0.0;
    int i, j;
    for (i = -shadow_size; i <= shadow_size; i++) {
        for (j = -shadow_size; j <= shadow_size; j++)
        {
            double r = sqrt(i*i + j*j);
            if (r <= 0) continue;
            cairo_set_source_rgba(c,
                                  0.0, 0.0, 0.0,
                                  1.0 - (1.0 - shadow_edge_alpha) * r / shadow_size);
            pango_cairo_update_layout(c, shadow_layout);
            cairo_move_to(c, posx + i, posy + j);
            pango_cairo_show_layout(c, shadow_layout);
        }
    }
}

void draw_text(PangoLayout *layout, cairo_t *c, int posx, int posy, Color *color, PangoLayout *shadow_layout)
{
    if (shadow_layout)
        draw_shadow(c, posx, posy, shadow_layout);
    cairo_set_source_rgba(c, color->rgb[0], color->rgb[1], color->rgb[2], color->alpha);
    pango_cairo_update_layout(c, layout);
    cairo_move_to(c, posx, posy);
    pango_cairo_show_layout(c, layout);
}

Imlib_Image load_image(const char *path, int cached)
{
    Imlib_Image image;
    if (debug_icons)
        fprintf(stderr, "tint2: loading icon %s\n", path);
    image = imlib_load_image(path);
#ifdef HAVE_RSVG
    if (!image && g_str_has_suffix(path, ".svg")) {
        int pipe_fd_stdout[2];

        if (pipe (pipe_fd_stdout))
            fprintf (stderr, "tint2: load_image: Creating output pipe for SVG loader failed!\n");
        else {
            // We fork here because librsvg allocates memory like crazy
            pid_t pid = fork();
            if (pid == 0) {
                // Child
                close (pipe_fd_stdout[0]);
                dup2  (pipe_fd_stdout[1], 1); // 1 is stdout
                close (pipe_fd_stdout[1]);

                GError *err = NULL;
                RsvgHandle *svg = rsvg_handle_new_from_file(path, &err);

                if (err != NULL) {
                    fprintf(stderr, "tint2: Could not load svg image!: %s", err->message);
                    g_error_free(err);
                } else {
                    GdkPixbuf *pixbuf = rsvg_handle_get_pixbuf(svg);
                    int dim[] = {
                        gdk_pixbuf_get_width (pixbuf),
                        gdk_pixbuf_get_height (pixbuf),
                    };
                    bool has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);

                    // Convert from GdkPixbuf RGBA byte array to DATA32 bitfield
                    const guint8 *data = gdk_pixbuf_read_pixels (pixbuf);   // Modifying pixbuf internal data
                    write_data (1, (const char *)dim, sizeof(dim));
                    if (has_alpha)
                        for (int i = 0, I = dim[0] * dim[1]; i < I; i++)
                        {
                            guint8 *p = (guint8 *)data + i * 4;
                            *(DATA32 *)p = ((DATA32)p[3] << 24) | ((DATA32)p[0] << 16) | ((DATA32)p[1] << 8) | (DATA32)p[2];
                        }
                    else
                        for (int i = 0, I = dim[0] * dim[1]; i < I; i++)
                        {
                            guint8 *p = (guint8 *)data + i * 4;
                            *(DATA32 *)p = 0xFF000000 | ((DATA32)p[0] << 16) | ((DATA32)p[1] << 8) | (DATA32)p[2];
                        }
                    write_data (1, (const char *)data, dim[0] * dim[1] * 4);
                    g_object_unref( svg);
                    g_object_unref( pixbuf);
                }
                _exit(0);
            } else {
                // Parent
                static unsigned long counter = 0;
                char tmp_filename[128];
                STRBUF_AUTO_PRINTF (tmp_filename, "/tmp/tint2-%d-%lu.png", (int)getpid(), counter++);
                int fd = open(tmp_filename, O_CREAT | O_EXCL, 0600);
                if (fd >= 0)
                    close (fd);

                int dim[2], size, ret;
                close (pipe_fd_stdout[1]);

                ret = read (pipe_fd_stdout[0], dim, sizeof(dim));
                image = imlib_create_image (dim[0], dim[1]);
                imlib_context_set_image (image);
                imlib_image_set_has_alpha (1);
                if (fd >= 0)
                    imlib_image_set_format ("png");
                else
                    imlib_image_set_irrelevant_format (1);

                DATA32 *data = imlib_image_get_data ();
                size = dim[0] * dim[1] * sizeof(DATA32);
                char *p = (char *)data;
                while (size)
                {
                    ret = read (pipe_fd_stdout[0], p, size);
                    if (ret > 0) {
                        size -= ret;
                        p += ret;
                    }
                    else if (ret == 0 || (ret == -1 && errno != EINTR))
                        break;
                }
                imlib_image_put_back_data (data);
                if (fd >= 0)
                    imlib_save_image (tmp_filename);
                close (pipe_fd_stdout[0]);
            }
        }
    }
#endif
    imlib_context_set_image(image);
    imlib_image_set_changes_on_disk();
    return image;
}

Imlib_Image adjust_icon(Imlib_Image original, int alpha, int saturation, int brightness)
{
    if (!original)
        return NULL;

    imlib_context_set_image(original);
    Imlib_Image copy = imlib_clone_image();

    imlib_context_set_image(copy);
    imlib_image_set_has_alpha(1);
    DATA32 *data = imlib_image_get_data();
    adjust_asb(data,
               imlib_image_get_width(),
               imlib_image_get_height(),
               alpha / 100.0,
               saturation / 100.0,
               brightness / 100.0);
    imlib_image_put_back_data(data);
    return copy;
}

void draw_rect(cairo_t *c, double x, double y, double w, double h, double r, int corner_mask)
{
    draw_rect_on_sides(c, x, y, w, h, r, corner_mask);
}

void draw_rect_on_sides(cairo_t *c, double x, double y, double w, double h, double r, int corner_mask)
{
    double c1;
    r = MIN(MIN(w, h) / 2, r);
    c1 = 0.5522847498 * r;
    int rtl = corner_mask & CORNER_TL ? r : 0,
        rtr = corner_mask & CORNER_TR ? r : 0,
        rbr = corner_mask & CORNER_BR ? r : 0,
        rbl = corner_mask & CORNER_BL ? r : 0;

    cairo_move_to(c, x+rtl, y);
    cairo_line_to (c, x+w-rtr, y);                                  // Top line
    if (rtr > 0)                                                    // Top right corner
        cairo_curve_to (c, x+w-r+c1, y, x+w, y+r-c1, x+w, y+r);
    cairo_line_to (c, x+w, y+h-rbr);                                // Right line
    if (rbr > 0)                                                    // Bottom right corner
        cairo_curve_to (c, x+w, y+h-r+c1, x+w-r+c1, y+h, x+w-r, y+h);
    cairo_line_to (c, x+rbl, y+h);                                  // Bottom line
    if (rbl > 0)                                                    // Bottom left corner
        cairo_curve_to (c, x+r-c1, y+h, x, y+h-r+c1, x, y+h-r);
    cairo_line_to (c, x, y+rtl);                                    // Left line
    if (rtl > 0)                                                    // Top left corner
        cairo_curve_to (c, x, y+r-c1, x+r-c1, y, x+r, y);
}

void clear_pixmap(Pixmap p, int x, int y, int w, int h)
{
    Picture pict = XRenderCreatePicture (server.display, p,
                                         XRenderFindVisualFormat(server.display, server.visual),
                                         0, 0);
    XRenderColor col;
    XRenderFillRectangle(server.display, PictOpClear, pict, &col, x, y, w, h);
    XRenderFreePicture(server.display, pict);
}

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
                    double scale)
{
    PangoRectangle rect;

    available_width = MAX(0, available_width);
    available_height = MAX(0, available_height);

    Pixmap pmap = XCreatePixmap(server.display, server.root_win,
                                available_height, available_width, server.depth);
    cairo_surface_t *cs = cairo_xlib_surface_create(server.display, pmap, server.visual,
                                                    available_height, available_width);
    cairo_t *c = cairo_create(cs);

    PangoContext *context = pango_cairo_create_context(c); // Leak source, unref (below) is useless
    pango_cairo_context_set_resolution(context, 96 * scale);
    PangoLayout *layout = pango_layout_new(context);
    pango_layout_set_width(layout, available_width * PANGO_SCALE);
    pango_layout_set_height(layout, available_height * PANGO_SCALE);
    pango_layout_set_alignment(layout, alignment);
    pango_layout_set_wrap(layout, wrap);
    pango_layout_set_ellipsize(layout, ellipsis);
    pango_layout_set_font_description(layout, font);
    text_len = MAX(0, text_len);
    if (!markup)
        pango_layout_set_text(layout, text, text_len);
    else
        pango_layout_set_markup(layout, text, text_len);

    pango_layout_get_extents(layout, NULL, &rect); // Leak source
    // Hope, this reduces chance of wrong pixel extents - if obscure extents_to_pixels() conversion was reason
    *width  = ceil((rect.x + rect.width ) / (double)PANGO_SCALE) - floor(rect.x / (double)PANGO_SCALE);
    *height = ceil((rect.y + rect.height) / (double)PANGO_SCALE) - floor(rect.y / (double)PANGO_SCALE);

    g_object_unref(layout);
    g_object_unref(context);
    cairo_destroy(c);
    cairo_surface_destroy(cs);
    XFreePixmap(server.display, pmap);
}

#if !GLIB_CHECK_VERSION(2, 34, 0)
GList *g_list_copy_deep(GList *list, GCopyFunc func, gpointer user_data)
{
    list = g_list_copy(list);

    if (func)
        for (GList *l = list; l; l = l->next)
            l->data = func(l->data, user_data);

    return list;
}
#endif

GSList *load_locations_from_dir(GSList *locations, const char *dir, ...) {
    int buf_cap = 0;
    char *buf = NULL;
    //~ fprintf(stderr, "%s: '%s'\n", __FUNCTION__, dir);
    
    int dir_len = strlen(dir);
    if (dir_len > buf_cap)
        buf = realloc(buf, (buf_cap = dir_len) + 1);
    memcpy(buf, dir, dir_len);
    buf[dir_len] = G_DIR_SEPARATOR;
    
    va_list ap;
    va_start(ap, dir);
    dir_len++;
    for (const char *suffix = va_arg(ap, const char *);
         suffix;
         suffix = va_arg(ap, const char *))
    {
        int suf_len, buf_size_req;
        suf_len = strlen(suffix);
        buf_size_req = dir_len + suf_len;
        if (buf_size_req > buf_cap)
            buf = realloc(buf, (buf_cap = buf_size_req) + 1);
        memcpy(buf + dir_len, suffix, suf_len);
        buf[buf_size_req] = '\0';
        
        //~ fprintf(stderr, " location: '%s'\n", buf);
        locations = slist_append_uniq_dup(locations, buf, g_str_equal);
    }
    va_end(ap);
    free(buf);

    return locations;
}

GSList *load_locations_from_env(GSList *locations, const char *var, ...)
{
    int buf_cap = 0;
    char *buf = NULL;
    char *value = getenv (var);
    if (value)
    {
        //~ fprintf (stderr, "%s: %s = '%s'\n", __FUNCTION__, var, value);
        value = strdup (value);
        char *t;
        for (char *token = strtok_r (value, ":", &t);
             token;
             token = strtok_r (NULL, ":", &t))
        {
            int tok_len = strlen (token);
            if (tok_len > buf_cap)
                buf = realloc (buf, (buf_cap = tok_len) + 1);
            memcpy (buf, token, tok_len);
            buf [tok_len] = G_DIR_SEPARATOR;

            va_list ap;
            va_start (ap, var);
            tok_len++;
            for (const char *suffix = va_arg (ap, const char *);
                 suffix;
                 suffix = va_arg (ap, const char *))
            {
                int suf_len, buf_size_req;
                suf_len = strlen (suffix);
                buf_size_req = tok_len + suf_len;
                if (buf_size_req > buf_cap)
                    buf = realloc (buf, (buf_cap = buf_size_req) + 1);
                memcpy (buf + tok_len, suffix, suf_len);
                buf [buf_size_req] = '\0';
                
                //~ fprintf (stderr, " location: '%s'\n", buf);
                locations = slist_append_uniq_dup (locations, buf, g_str_equal);
            }
            va_end (ap);
        }
        free (value);
        free (buf);
    }
    return locations;
}

GSList *slist_append_uniq(GSList *list, gconstpointer ref, GCompareFunc eq, void* (*assign)(const void *))
// Append to list if not found, via assign function. Use value directly with NULL assign.
{
    if (!list) {
        list = g_slist_alloc();
        list->next = NULL;
        list->data = g_strdup(ref);
    }
    else for (GSList *e = list; e; e = e->next)
    {
        if (eq(e->data, ref))
            break;
        if (e->next == NULL) {
            e = e->next = g_slist_alloc();
            e->next = NULL;
            e->data = assign ? assign(ref) : (gpointer)ref;
            break;
        }
    }
    return list;
}

gint cmp_ptr(gconstpointer a, gconstpointer b)
{
    return a < b ? -1 : a != b;
}

void close_all_fds()
{
    long maxfd = sysconf(_SC_OPEN_MAX);
    for (int fd = 3; fd < maxfd; fd++)
        close(fd);
}

GString *tint2_g_string_replace(GString *s, const char *from, const char *to)
{
    GString *result = g_string_new("");
    for (char *p = s->str; *p;) {
        if (strstr(p, from) == p) {
            g_string_append(result, to);
            p += strlen(from);
        } else {
            g_string_append_c(result, *p);
            p += 1;
        }
    }
    g_string_assign(s, result->str);
    g_string_free(result, TRUE);
    return s;
}

void get_image_mean_color(const Imlib_Image image, Color *mean_color)
{
    memset(mean_color, 0, sizeof(*mean_color));

    if (!image)
        return;
    imlib_context_set_image(image);
    imlib_image_set_has_alpha(1);
    size_t size = (size_t)imlib_image_get_width() * (size_t)imlib_image_get_height();
    DATA32 *data = imlib_image_get_data_for_reading_only();
    DATA32 sum_r, sum_g, sum_b, count;
    sum_r = sum_g = sum_b = count = 0;
    for (size_t i = 0; i < size; i++)
    {
        DATA32 argb, a, r, g, b;
        argb = data[i];
        a = (argb >> 24) & 0xff;
        r = (argb >> 16) & 0xff;
        g = (argb >>  8) & 0xff;
        b = (argb      ) & 0xff;
        if (a)
            sum_r += r,
            sum_g += g,
            sum_b += b,
            count++;
    }

    if (!count)
        count = 1;
    mean_color->alpha = 1.0;
    mean_color->rgb[0] = sum_r / 255.0 / count;
    mean_color->rgb[1] = sum_g / 255.0 / count;
    mean_color->rgb[2] = sum_b / 255.0 / count;
}

void adjust_color(Color *color, int alpha, int saturation, int brightness)
{
    if (alpha == 100 && saturation == 0 && brightness == 0)
        return;

    DATA32 argb =   (((DATA32)(color->alpha  * 255) & 0xff) << 24) |
                    (((DATA32)(color->rgb[0] * 255) & 0xff) << 16) |
                    (((DATA32)(color->rgb[1] * 255) & 0xff) <<  8) |
                    (((DATA32)(color->rgb[2] * 255) & 0xff) <<  0);
    adjust_asb (&argb, 1, 1, alpha / 100.0, saturation / 100.0, brightness / 100.0);
    DATA32 a = (argb >> 24) & 0xff;
    DATA32 r = (argb >> 16) & 0xff;
    DATA32 g = (argb >>  8) & 0xff;
    DATA32 b = (argb      ) & 0xff;
    color->alpha  = a / 255.0;
    color->rgb[0] = r / 255.0;
    color->rgb[1] = g / 255.0;
    color->rgb[2] = b / 255.0;
}

void dump_image_data(const char *image_file, const char *name)
{
    Imlib_Image image = load_image(image_file, false);
    if (!image) {
        fprintf(stderr, "tint2: Could not load image from file\n");
        return;
    }

    size_t name_len;

    char *file_name = strdup_printf( &name_len, "%s.h", name);
    FILE *header = fopen( file_name, "wt");
    fprintf(header,
            "#ifndef %s_h\n"
            "#define %s_h\n"
            "\n"
            "#include <Imlib2.h>\n"
            "\n"
            "extern int %s_width;\n"
            "extern int %s_height;\n"
            "extern DATA32 %s_data[];\n"
            "\n"
            "#endif\n",
            name, name,
            name, name, name);
    fclose(header);

    imlib_context_set_image(image);

    int height = imlib_image_get_height(),
        width = imlib_image_get_width();
    file_name [name_len - 1] = 'c';
    FILE *source = fopen( file_name, "wt");

    fprintf(source,
            "#include <%s.h>\n"
            "\n"
            "int %s_width = %d;\n"
            "int %s_height = %d;\n"
            "DATA32 %s_data[] = {\n",
            name,
            name, width,
            name, height,
            name);

    DATA32 *data = imlib_image_get_data_for_reading_only();
    int *col_widths = malloc (width * sizeof (*col_widths));
    for (size_t c = 0; c < width; c++)
    {
        int col_width = 0;
        for (size_t l = 0; l < height; l++)
        {
            DATA32 value = data [width * l + c];
            int w = value ? (int) floor (log10 (value)) + 1 : 1;
            if (col_width < w)
                col_width = w;
        }
        col_widths [c] = col_width;
    }
    for (size_t l = 0; l < height; l++)
    {
        fprintf (source, "\t");
        for (size_t c = 0; c < width; c++)
        {
            fprintf(source, "%*u%s", col_widths[c], data[width * l + c], (l == width-1 && c == height-1) ? "" : ", ");
        }
        fprintf (source, "\n");
    }
    fprintf(source, "};\n");
    free (col_widths);
    fclose(source);
    free( file_name);

    imlib_free_image();
}
