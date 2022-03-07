#include "init.h"

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/XShm.h>

#include "config.h"
#include "default_icon.h"
#include "drag_and_drop.h"
#include "fps_distribution.h"
#include "panel.h"
#include "server.h"
#include "signals.h"
#include "test.h"
#include "tooltip.h"
#include "tracing.h"
#include "uevent.h"
#include "version.h"

void print_usage()
{
    fprintf(stdout,
            "Usage: tint2 [OPTION...] [path_to_config_file]\n"
            "\n"
            "Options:\n"
            "      --battery-sys-prefix          Battery system prefix.\n"
            "                                    Linux default is \"/sys/class/power_supply\".\n"
            "  -c, --config path_to_config_file  Loads the configuration file from a\n"
            "                                    custom location.\n"
            "  -s, --snapshot path_to_snapshot   Save panel snapshot to file and quit\n"
            "  -v, --version                     Prints version information and exits.\n"
            "  -h, --help                        Display this help and exits.\n"
            "\n"
            "Developer options:\n"
            "      --test                                 Run built-in self-tests.\n"
            "      --test-verbose                         Same as --tests, but with verbose errors report.\n"
            "      --dump-image-data image output_prefix  Wraps image file into resource in the simplest possible form.\n"
            "\n"
            "For more information, run `man tint2` or visit the project page\n"
            "<https://gitlab.com/nick87720z/tint2>.\n");
}

// Data with their keys for str_index().
// Must be sorted with "LANG=C sort" command.

enum {  help_key_battery_sys_prefix,
        help_key_config,
        help_key_dump_image_data,
        help_key_help,
        help_key_snapshot,
        help_key_test,
        help_key_test_verbose,
        help_key_version,
        help_key_c,
        help_key_h,
        help_key_s,
        help_key_v,
        HELP_KEYS
};
static char *help_opt_sv[] = {  "--battery-sys-prefix",
                                "--config",
                                "--dump-image-data",
                                "--help",
                                "--snapshot",
                                "--test",
                                "--test-verbose",
                                "--version",
                                "-c",
                                "-h",
                                "-s",
                                "-v", };

void handle_cli_arguments(int argc, char **argv)
{
    // Read command line arguments
    for (int i = 1; i < argc; ++i) {
        gboolean error = FALSE;

        switch (str_index ( argv[i], help_opt_sv, HELP_KEYS ))
        {
            case help_key_h: case help_key_help:
                print_usage();
                exit(0);
                break;
            case help_key_v: case help_key_version:
                fprintf(stdout, "tint2 version %s\n", VERSION_STRING);
                exit(0);
                break;
            case help_key_test:
                run_all_tests(false);
                exit(0);
                break;
            case help_key_test_verbose:
                run_all_tests(true);
                exit(0);
                break;
            case help_key_dump_image_data:
                dump_image_data(argv[i+1], argv[i+2]);
                exit(0);
                break;
            case help_key_c: case help_key_config:
                if (i + 1 < argc) {
                    i++;
                    config_path = strdup(argv[i]);
                } else {
                    error = TRUE;
                }
                break;
            case help_key_s: case help_key_snapshot:
                if (i + 1 < argc) {
                    i++;
                    snapshot_path = strdup(argv[i]);
                } else {
                    error = TRUE;
                }
                break;
        #ifdef ENABLE_BATTERY
            case help_key_battery_sys_prefix:
                if (i + 1 < argc) {
                    i++;
                    battery_sys_prefix = strdup(argv[i]);
                } else {
                    error = TRUE;
                }
                break;
        #endif
            default:
                if (i + 1 == argc)
                    config_path = strdup(argv[i]);
                else
                    error = TRUE;
        }
        if (error) {
            print_usage();
            exit(EXIT_FAILURE);
        }
    }
}

void handle_env_vars()
{
    char *tmp;

    #define _load_env_flag(name)                                                         \
    ( (tmp = getenv(name)) && tmp[0] && str_index(tmp, (char *[]){"0", "no", "off"}, 3) == -1 )

    debug_geometry  = _load_env_flag("DEBUG_GEOMETRY");
    debug_gradients = _load_env_flag("DEBUG_GRADIENTS");
    debug_icons     = _load_env_flag("DEBUG_ICONS");
    debug_fps       = _load_env_flag("DEBUG_FPS");
    debug_frames    = _load_env_flag("DEBUG_FRAMES");
    debug_dnd       = _load_env_flag("DEBUG_DND");
    debug_thumbnails = _load_env_flag("DEBUG_THUMBNAILS");
    debug_timers    = _load_env_flag("DEBUG_TIMERS");
    debug_executors = _load_env_flag("DEBUG_EXECUTORS");
    debug_blink     = _load_env_flag("DEBUG_BLINK");
    thumb_use_shm   = _load_env_flag("TINT2_THUMBNAIL_SHM");
    if (debug_fps)
    {
        init_fps_distribution();
        char *s = getenv("TRACING_FPS_THRESHOLD");
        if (!s || sscanf(s, "%lf", &tracing_fps_threshold) != 1) {
            tracing_fps_threshold = 60;
        }
    }
    #undef _load_env_flag
}

static Timer detect_compositor_timer = DEFAULT_TIMER;
static int detect_compositor_timer_counter = 0;

void detect_compositor(void *arg)
{
    if (server.composite_manager) {
        stop_timer(&detect_compositor_timer);
        return;
    }

    detect_compositor_timer_counter--;
    if (detect_compositor_timer_counter < 0) {
        stop_timer(&detect_compositor_timer);
        return;
    }

    // No compositor, check for one
    if (XGetSelectionOwner(server.display, server.atom [_NET_WM_CM_S0]) != None) {
        stop_timer(&detect_compositor_timer);
        // Restart tint2
        fprintf(stderr, "tint2: Detected compositor, restarting tint2...\n");
        kill(getpid(), SIGUSR1);
    }
}

void start_detect_compositor()
{
    // Already have a compositor, nothing to do
    if (server.composite_manager)
        return;

    // Check every 0.5 seconds for up to 30 seconds
    detect_compositor_timer_counter = 60;
    INIT_TIMER(detect_compositor_timer);
    change_timer(&detect_compositor_timer, true, 500, 500, detect_compositor, 0);
}

void create_default_elements()
{
    default_timers();
    default_systray();
    memset(&server, 0, sizeof(server));
#ifdef ENABLE_BATTERY
    default_battery();
#endif
    default_clock();
    default_launcher();
    default_taskbar();
    default_tooltip();
    default_execp();
    default_button();
    default_panel();
}

void load_default_task_icon()
{
    const gchar *const *data_dirs = g_get_system_data_dirs();
    for (int i = 0; data_dirs[i] != NULL; i++) {
        gchar *path = g_build_filename(data_dirs[i], "tint2", "default_icon.png", NULL);
        if (g_file_test(path, G_FILE_TEST_EXISTS))
            default_icon = load_image(path, TRUE);
        g_free(path);
    }
    if (!default_icon) {
        default_icon = imlib_create_image_using_data(default_icon_width,
                                                     default_icon_height,
                                                     default_icon_data);
    }
}

void init_post_config()
{
    server_init_visual();
    server_init_xdamage();

    imlib_context_set_display(server.display);
    imlib_context_set_visual(server.visual);
    imlib_context_set_colormap(server.colormap);

    init_signals_postconfig();
    load_default_task_icon();

    XSync(server.display, False);
}

void init_X11_pre_config()
{
    server.display = XOpenDisplay(NULL);
    if (!server.display) {
        fprintf(stderr, "tint2: could not open display!\n");
        exit(EXIT_FAILURE);
    }
    server.x11_fd = ConnectionNumber(server.display);
    server.errors = g_queue_new ();
    XSetErrorHandler(server_catch_error);
    XSetIOErrorHandler(x11_io_error);

    server.screen = DefaultScreen(server.display);
    server_init_atoms ();
    server.root_win = RootWindow(server.display, server.screen);
    server.has_shm = XShmQueryExtension(server.display);

    // This line adds dependency on env variables to be handled first
    fprintf (stderr, "tint2: xShm: %s\n", !server.has_shm ? "Unavailable" : thumb_use_shm ? "Enabled" : "Disabled");

    // Needed since the config file uses '.' as decimal separator
    setlocale(LC_ALL, "");
    setlocale(LC_NUMERIC, "POSIX");

    /* Catch events */
    XSelectInput(server.display, server.root_win, PropertyChangeMask | StructureNotifyMask);

    // get monitor and desktop config
    get_monitors();
    get_desktops();
    server.desktop = get_current_desktop();

    server.disable_transparency = FALSE;

    xsettings_client = xsettings_client_new(server.display, server.screen, xsettings_notify_cb, NULL, NULL);
}

void init(int argc, char **argv)
{
    setlinebuf(stdout);
    setlinebuf(stderr);

    default_config();
    handle_env_vars();
    handle_cli_arguments(argc, argv);
    create_default_elements();
    init_signals();

    init_X11_pre_config();
    if (!config_read()) {
        fprintf(stderr, "tint2: Could not read config file.\n");
        print_usage();
        timers_warnings = false;
        cleanup();
        exit(EXIT_FAILURE);
    }

    init_post_config();
    start_detect_compositor();
    init_panel();
}

void cleanup()
{
#ifdef HAVE_SN
    if (startup_notifications) {
        sn_display_unref(server.sn_display);
        server.sn_display = NULL;
    }
#endif // HAVE_SN

    cleanup_button();
    cleanup_execp();
    cleanup_systray();
    cleanup_tooltip();
    cleanup_clock();
    cleanup_launcher();
#ifdef ENABLE_BATTERY
    cleanup_battery();
#endif
    cleanup_separator();
    cleanup_taskbar();
    cleanup_panel();
    cleanup_config();

    if (default_icon) {
        imlib_context_set_image(default_icon);
        imlib_free_image();
        default_icon = NULL;
    }
    imlib_context_disconnect_display();

    xsettings_client_destroy(xsettings_client);
    xsettings_client = NULL;

    cleanup_server();
    cleanup_timers();
    icon_theme_common_cleanup ();

    if (server.display)
        XCloseDisplay(server.display);
    if (server.errors)
        g_queue_free (server.errors);
    server.display = NULL;
    server.errors = NULL;

    if (sigchild_pipe_valid) {
        sigchild_pipe_valid = FALSE;
        close(sigchild_pipe[1]);
        close(sigchild_pipe[0]);
    }

    uevent_cleanup();
    cleanup_fps_distribution();

#ifdef HAVE_TRACING
    cleanup_tracing();
#endif
}

// TESTS

STR_ARRAY_TEST_SORTED (help_opt_sv, ARRAY_SIZE(help_opt_sv));
