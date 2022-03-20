#ifndef EXECPLUGIN_H
#define EXECPLUGIN_H

#include <sys/time.h>
#include <pango/pangocairo.h>

#include "area.h"
#include "common.h"
#include "timer.h"

extern bool debug_executors;

// Architecture:
// Panel panel_config contains an array of Execp, each storing all config options and all the state variables.
// Only these run commands.
//
// Tint2 maintains an array of Panels, one for each monitor. Each stores an array of Execp which was initially copied
// from panel_config. Each works as a frontend to the corresponding Execp in panel_config as backend, using the
// backend's config and state variables.

typedef struct ExecpBackend {
// Config:
    char name[21];
    char *command;  // Command to execute at a specified interval
    int interval;   // Interval in seconds
    int monitor;
    gboolean has_icon;  // 1 if first line of output is an icon path
    gboolean cache_icon;
    int icon_w;
    int icon_h;
    gboolean has_user_tooltip;
    char *tooltip;
    gboolean centered;
    gboolean has_font;
    PangoFontDescription *font_desc;
    Color font_color;
    int continuous;
    gboolean has_markup;
    char *lclick_command;
    char *mclick_command;
    char *rclick_command;
    char *uwheel_command;
    char *dwheel_command;
    int lclick_command_sink,    // Custom command sink ID
        mclick_command_sink,    // -1 - no sink (fork and exec as usually)
        rclick_command_sink,    //  0 - executor stdin (exclusive sink)
        uwheel_command_sink,    //>=1 - TODO: externally configured sink (shared)
        dwheel_command_sink;
    int paddingx, // horizontal padding left/right
        spacing,   // horizontal padding between childs
        paddingy;
    Background *bg;

    // Backend state:
    Timer timer;
    int child_pipe_stdin;
    int child_pipe_stdout;
    int child_pipe_stderr;
    pid_t child;

    // Command output buffer
    char *buf_stdout;
    ssize_t buf_stdout_length;
    ssize_t buf_stdout_capacity;
    char *buf_stderr;
    ssize_t buf_stderr_length;
    ssize_t buf_stderr_capacity;

    char *text;         // Text extracted from the output buffer
    char *icon_path;    // Icon path extracted from the output buffer
    Imlib_Image icon;
    gchar tooltip_text[512]; // Default tooltip when user tooltip is disabled

    time_t last_update_start_time;  // The time the last command was started
    time_t last_update_finish_time; // The time the last output was obtained
    time_t last_update_duration;    // The time it took to execute last command

    GList *instances;
    // List of Execp which are frontends for this backend, one for each panel

    GTree *cmd_pids;
} ExecpBackend;

typedef struct ExecpFrontend {
// Frontend state:
    int iconx;
    int icony;
    int textx;
    int texty;
    int textw;
    int texth;
} ExecpFrontend;

typedef struct Execp {
    Area area;

    ExecpBackend *backend;      // All elements have the backend pointer set.
                                // However only backend elements have ownership.

    ExecpFrontend *frontend;    // Set only for frontend Execp items.
    bool dummy;
} Execp;

void default_execp();
// Called before the config is read and panel_config/panels are created.
// Afterwards, the config parsing code creates the array of Execp in panel_config and populates the configuration fields
// in the backend.
// Probably does nothing.

Execp *create_execp();
// Creates a new Execp item with only the backend field set. The state is NOT initialized. The config is initialized to
// the default values.
// This will be used by the config code to populate its backedn config fields.

void destroy_execp(void *obj);

void init_execp();
// Called after the config is read and panel_config is populated, but before panels are created.
// Initializes the state of the backend items.
// panel_config.panel_items is used to determine which backend items are enabled. The others should be destroyed and
// removed from panel_config.execp_list.

void init_execp_panel(void *panel);
// Called after each on-screen panel is created, with a pointer to the panel.
// Initializes the state of the frontend items. Also adds a pointer to it in backend->instances.
// At this point the Area has not been added yet to the GUI tree, but it will be added right away.

void cleanup_execp();
// Called just before the panels are destroyed. Afterwards, tint2 exits or restarts and reads the config again.
// Releases all frontends and then all the backends.
// The frontend items are not freed by this function, only their members. The items are Areas which are freed in the
// GUI element tree cleanup function (remove_area).

void draw_execp(void *obj, cairo_t *c);
// Called on draw, obj = pointer to the front-end Execp item.

gboolean resize_execp(void *obj);
// Called on resize, obj = pointer to the front-end Execp item.
// Returns 1 if the new size is different than the previous size.

void execp_action(void *obj, int button, int x, int y, Time time);
// Called on mouse click event.

void execp_cmd_completed(Execp *obj, pid_t pid);

gboolean read_execp(void *obj);
// Called to check if new output from the command can be read.
// No command might be running.
// Returns 1 if the output has been updated and a redraw is needed.

void execp_update_post_read(Execp *execp);
// Called for Execp front elements when the command output has changed.

void execp_default_font_changed();

void handle_execp_events( fd_set *fds, int *fdn);

void execp_force_update(Execp *execp);

#endif // EXECPLUGIN_H
