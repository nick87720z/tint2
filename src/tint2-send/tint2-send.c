#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Display  *display = NULL;
static XEvent event = {};
static char *execp_name = NULL;

enum action {
    ACTION_HIDE,
    ACTION_REFRESH_EXECP,
    ACTION_SHOW,
};

/* From wmctrl */
char *get_property(Window window, Atom xa_prop_type, const char *prop_name) {
    Atom xa_prop_name = XInternAtom(display, prop_name, False);

    Atom xa_ret_type;
    int ret_format;
    unsigned long ret_nitems;
    unsigned long ret_bytes_after;
    unsigned long tmp_size;
    unsigned char *ret_prop = NULL;

    if (XGetWindowProperty(display, window, xa_prop_name, 0, 1024,
                           False, xa_prop_type, &xa_ret_type, &ret_format,
                           &ret_nitems, &ret_bytes_after, &ret_prop) != Success)
        goto err0;

    if (xa_ret_type != xa_prop_type)
        goto err1;

    return ret_prop;

err1:
    XFree (ret_prop);
err0:
    return NULL;
}

int is_tint2(Window window)
{
    XWindowAttributes attr = {};
    if (!XGetWindowAttributes(display, window, &attr))
        return 0;
    // if (attr.map_state != IsViewable)
    //     return 0;

    char *wm_class = get_property(window, XA_STRING, "WM_NAME");
    if (!wm_class)
        return 0;

    int class_match = strcmp(wm_class, "tint2") == 0;
    XFree (wm_class);

    return class_match;
}

void handle_tint2_window(Window window, int action)
{
    if (!is_tint2(window))
        return;
    switch (action)
    {
        case ACTION_SHOW:
            fprintf(stderr, "Showing tint2 window: %lx\n", window);
            break;
        case ACTION_HIDE:
            fprintf(stderr, "Hiding tint2 window: %lx\n", window);
            break;
        case ACTION_REFRESH_EXECP:
            fprintf(stderr, "Refreshing execp '%s' for window: %lx\n", execp_name, window);
            break;
    }
    event.xcrossing.window = window;
    XSendEvent(display, window, False, 0, &event);
    XFlush(display);
}

typedef void window_callback_t(Window window, int action);

void walk_windows(Window node, window_callback_t *callback, int action)
{
    callback(node, action);

    Window root = 0;
    Window parent = 0;
    Window *children = 0;
    unsigned int nchildren = 0;
    if (!XQueryTree(display, node,
                    &root, &parent, &children, &nchildren))
        return;

    for (unsigned int i = 0; i < nchildren; i++)
        walk_windows(children[i], callback, action);
}

static int cmp_strp (const void *p1, const void *p2) {
    return strcmp( *(char **)p1, *(char **)p2 );
}

int main(int argc, char **argv)
{
    display = XOpenDisplay(NULL);
    if (!display ) {
        fprintf(stderr, "Failed to open X11 connection\n");
        exit(1);
    }

    argc--, argv++;
    if (!argc) {
        fprintf(stderr, "Usage: tint2-send [show|hide|refresh-execp]\n");
        exit(1);
    }

    char *array[] = { "hide", "refresh-execp", "show" };
    char **tmp = bsearch ( &argv[0], array, 3, sizeof(*array), cmp_strp);
    int action = tmp ? tmp - array : -1;

    switch (action)
    {
        case -1:
            fprintf (stderr, "Error: unknown action %s\n", argv[0]);
            exit(1);
        case ACTION_SHOW:
            event.xcrossing.type = EnterNotify;
            event.xcrossing.mode = NotifyNormal;
            event.xcrossing.detail = NotifyVirtual;
            event.xcrossing.same_screen = True;
            break;
        case ACTION_HIDE:
            event.xcrossing.type = LeaveNotify;
            event.xcrossing.mode = NotifyNormal;
            event.xcrossing.detail = NotifyVirtual;
            event.xcrossing.same_screen = True;
            break;
        case ACTION_REFRESH_EXECP:
            execp_name = argv[1];
            if (!execp_name) {
                fprintf(stderr, "Error: missing execp name\n");
                exit(1);
            }
            if (!execp_name[0]) {
                fprintf(stderr, "Error: empty execp name\n");
                exit(1);
            }
            if (strlen(execp_name) > sizeof(event.xclient.data.b)) {
                fprintf(stderr, "Error: execp name bigger than %ld bytes\n", sizeof(event.xclient.data.b));
                exit(1);
            }
            event.xclient.type = ClientMessage;
            event.xclient.send_event = True;
            event.xclient.message_type = XInternAtom(display, "_TINT2_REFRESH_EXECP", False);
            event.xclient.format = 8;
            strncpy(event.xclient.data.b, execp_name, sizeof(event.xclient.data.b));
            break;
    }
    walk_windows(DefaultRootWindow(display), handle_tint2_window, action);

    return 0;
}
