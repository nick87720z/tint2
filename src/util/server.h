/**************************************************************************
* server :
* -
*
* Check COPYING file for Copyright
*
**************************************************************************/

#ifndef SERVER_H
#define SERVER_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xinerama.h>

#ifdef HAVE_SN
#include <libsn/sn.h>
#endif
#include <glib.h>

extern gboolean primary_monitor_first;

enum atom {
    _XROOTPMAP_ID,
    _XROOTMAP_ID,
    _NET_CURRENT_DESKTOP,
    _NET_NUMBER_OF_DESKTOPS,
    _NET_DESKTOP_NAMES,
    _NET_DESKTOP_GEOMETRY,
    _NET_DESKTOP_VIEWPORT,
    _NET_WORKAREA,
    _NET_ACTIVE_WINDOW,
    _NET_WM_WINDOW_TYPE,
    _NET_WM_STATE_SKIP_PAGER,
    _NET_WM_STATE_SKIP_TASKBAR,
    _NET_WM_STATE_STICKY,
    _NET_WM_STATE_DEMANDS_ATTENTION,
    _NET_WM_WINDOW_TYPE_DOCK,
    _NET_WM_WINDOW_TYPE_DESKTOP,
    _NET_WM_WINDOW_TYPE_TOOLBAR,
    _NET_WM_WINDOW_TYPE_MENU,
    _NET_WM_WINDOW_TYPE_SPLASH,
    _NET_WM_WINDOW_TYPE_DIALOG,
    _NET_WM_WINDOW_TYPE_NORMAL,
    _NET_WM_DESKTOP,
    WM_STATE,
    _NET_WM_STATE,
    _NET_WM_STATE_MAXIMIZED_VERT,
    _NET_WM_STATE_MAXIMIZED_HORZ,
    _NET_WM_STATE_SHADED,
    _NET_WM_STATE_HIDDEN,
    _NET_WM_STATE_BELOW,
    _NET_WM_STATE_ABOVE,
    _NET_WM_STATE_MODAL,
    _NET_CLIENT_LIST,
    _NET_WM_NAME,
    _NET_WM_VISIBLE_NAME,
    _NET_WM_STRUT,
    _NET_WM_ICON,
    _NET_WM_ICON_GEOMETRY,
    _NET_WM_ICON_NAME,
    _NET_CLOSE_WINDOW,
    UTF8_STRING,
    _NET_SUPPORTING_WM_CHECK,
    _NET_WM_CM_S0,
    _NET_WM_STRUT_PARTIAL,
    WM_NAME,
    __SWM_VROOT,
    _MOTIF_WM_HINTS,
    WM_HINTS,

    // systray protocol
    _NET_SYSTEM_TRAY_SCREEN,
    _NET_SYSTEM_TRAY_OPCODE,
    MANAGER,
    _NET_SYSTEM_TRAY_MESSAGE_DATA,
    _NET_SYSTEM_TRAY_ORIENTATION,
    _NET_SYSTEM_TRAY_ICON_SIZE,
    _NET_SYSTEM_TRAY_PADDING,
    _XEMBED,
    _XEMBED_INFO,
    _NET_WM_PID,

    // XSettings
    _XSETTINGS_SCREEN,
    _XSETTINGS_SETTINGS,

    // drag 'n' drop
    XdndAware,
    XdndEnter,
    XdndPosition,
    XdndStatus,
    XdndDrop,
    XdndLeave,
    XdndSelection,
    XdndTypeList,
    XdndActionCopy,
    XdndFinished,
    TARGETS,

    // tint2 atoms
    _TINT2_REFRESH_EXECP,

    NUM_ATOMS
};

typedef struct Property {
    unsigned char *data;
    int format, nitems;
    Atom type;
} Property;

const char *GetAtomName(Display *disp, Atom a);
// Returns the name of an Atom as string. Do not free the string.

typedef struct Monitor {
    int x;
    int y;
    int width;
    int height;
    int dpi;
    gboolean primary;
    gchar **names;
} Monitor;

typedef struct Viewport {
    int x;
    int y;
    int width;
    int height;
} Viewport;

typedef struct Server {
    Display *display;
    int x11_fd;
    GQueue *errors; // Similar to signal handler, this is way for more meaningful error report for user
    int err_n;

    Window root_win;
    Window composite_manager;
    gboolean real_transparency;
    gboolean disable_transparency;
    int desktop;        // current desktop
    int screen;
    int depth;
    int num_desktops;   // number of monitor (without monitor included into another one)
    int num_monitors;

    Viewport *viewports;
    // Non-null only if WM uses viewports (compiz) and number of viewports > 1.
    // In that case there are num_desktops viewports.

    Monitor *monitors;
    gboolean got_root_win;
    Visual *visual;
    Visual *visual32;
    Pixmap root_pmap;   // root background
    GC gc;
    Colormap colormap;
    Colormap colormap32;
    Atom atom [NUM_ATOMS];
    int xdamage_event_type;
    int xdamage_event_error_type;
    gboolean has_shm;
#ifdef HAVE_SN
    SnDisplay *sn_display;
    GTree *pids;
#endif // HAVE_SN
} Server;

extern Server server;

#define GET_COMPOSITE_MANAGER() XGetSelectionOwner(server.display, server.atom [_NET_WM_CM_S0])

void cleanup_server();
// freed memory

void send_event32(Window win, Atom at, long data1, long data2, long data3);
int get_property32(Window win, Atom at, Atom type);
void *get_property(Window win, Atom at, Atom type, int *num_results);
Atom server_get_atom(char *atom_name);
int server_catch_error(Display *d, XErrorEvent *ev);
void server_init_atoms();
void server_init_visual();
void server_init_xdamage();

int x11_io_error(Display *display);
void handle_crash(const char *reason);

void get_root_pixmap();
// detect root background

void get_monitors();
// detect monitors and desktops

void sort_monitors();
void print_monitors();
void get_desktops();
void server_get_number_of_desktops();
GSList *get_desktop_names();
int get_current_desktop();
void change_desktop(int desktop);

void forward_click(XEvent *e);
// Forward mouse click to the desktop window

#ifdef HAVE_SN
void error_trap_push(SnDisplay *display, Display *xdisplay);
void error_trap_pop(SnDisplay *display, Display *xdisplay);
#endif

#endif
