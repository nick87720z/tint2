#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Display  *display = NULL;
static XEvent event = {};
static char *execp_name = NULL;
Atom xa_prop_name;

enum action {
    ACTION_HIDE,
    ACTION_REFRESH_EXECP,
    ACTION_SHOW,
};

/* From wmctrl (optimized) */
char *get_property(Window window, Atom xa_prop_type, Atom *prop)
{
    Atom xa_ret_type;
    int ret_format;
    unsigned long ret_nitems;
    unsigned long ret_bytes_after;
    unsigned long tmp_size;
    unsigned char *ret_prop = NULL;

    if (XGetWindowProperty(display, window, *prop, 0, 1024,
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

    char *wm_class = get_property(window, XA_STRING, &xa_prop_name);
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
    int status = 0;

    display = XOpenDisplay(NULL);
    if (!display ) {
        fprintf(stderr, "Failed to open X11 connection\n");
        exit(1);
    }

    argc--, argv++;
    if (!argc) {
        fprintf(stderr,
                "Usage:\n"
                "    tint2-send COMMAND\n"
                "    tint2-send --stdin\n"
                "Commands:\n"
                "    show\n"
                "    hide\n"
                "    refresh-execp EXECP_NAME\n"
        );
        exit(1);
    }
    xa_prop_name = XInternAtom(display, "WM_NAME", False);
    event.xclient.message_type = XInternAtom (display, "_TINT2_REFRESH_EXECP", False);

    char *array[] = { "hide", "refresh-execp", "show" };

    int use_stdin = strcmp (argv[0], "--stdin") == 0;
    char *line = NULL;
    size_t line_avail = 0;
    while (1)
    {
        char *args[2];

        if (!use_stdin)
            args[0] = argv[0],
            args[1] = argv[1];
        else {
            fputs (">", stdout);
            if (getline (&line, &line_avail, stdin) == -1)
                break;
            char *saveptr;
            args[0] = strtok_r (line, " \t\n", &saveptr);
            args[1] = strtok_r (NULL, " \t\n", &saveptr);
        }

        char **p = bsearch ( &args[0], array, 3, sizeof(*array), cmp_strp);
        int action = p ? p - array : -1;

        switch (action)
        {
            case -1:
                fprintf (stderr, "Error: unknown action %s\n", args[0]);
                status = 1;
                goto ret;
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
                execp_name = args[1];
                if (!execp_name) {
                    fprintf(stderr, "Error: missing execp name\n");
                    status = 1;
                    goto ret;
                }
                if (!execp_name[0]) {
                    fprintf(stderr, "Error: empty execp name\n");
                    status = 1;
                    goto ret;
                }
                if (strlen(execp_name) > sizeof(event.xclient.data.b)) {
                    fprintf(stderr, "Error: execp name bigger than %ld bytes\n", sizeof(event.xclient.data.b));
                    status = 1;
                    goto ret;
                }
                event.xclient.type = ClientMessage;
                event.xclient.send_event = True;
                event.xclient.format = 8;
                strncpy(event.xclient.data.b, execp_name, sizeof(event.xclient.data.b));
                break;
        }
        walk_windows(DefaultRootWindow(display), handle_tint2_window, action);

        if (!use_stdin)
            break;
    }

ret:
    if (line)
        free (line);
    return status;
}
