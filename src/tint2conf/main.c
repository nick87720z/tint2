/**************************************************************************
*
* Tint2conf
*
* Copyright (C) 2009 Thierry lorthiois (lorthiois@bbsoft.fr) from Omega distribution
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

#include <time.h>
#include <unistd.h>

#ifdef HAVE_VERSION_H
#include "version.h"
#endif

#include "main.h"
#include "common.h"
#include "theme_view.h"
#include "properties.h"
#include "properties_rw.h"

void refresh_theme(const char *given_path);
void remove_theme(const char *given_path);
static void theme_selection_changed(GtkWidget *treeview, gpointer userdata);
static gchar *find_theme_in_system_dirs(const gchar *given_name);
static void load_specific_themes(char **paths, int count);

// ====== Utilities ======

gchar *get_home_config_path()
// Returns ~/.config/tint2/tint2rc (or equivalent).
{
    return strdup_printf( NULL, "%s/tint2/tint2rc", (fetch_user_config_dir(), user_config_dir));
}

gchar *get_etc_config_path()
// Returns /etc/xdg/tint2/tint2rc (or equivalent).
// Needs gfree.
{
    gchar *path = NULL;
    const gchar *const *system_dirs = g_get_system_config_dirs();
    for (int i = 0; system_dirs[i]; i++) {
        path = g_build_filename(system_dirs[i], "tint2", "tint2rc", NULL);
        if (g_file_test(path, G_FILE_TEST_EXISTS))
            return path;
        g_free(path);
        path = NULL;
    }
    strdup_static(path, "/dev/null");
    return path;
}

gboolean startswith(const char *str, const char *prefix)
{
    return strstr(str, prefix) == str;
}

gboolean endswith(const char *str, const char *suffix)
{
    size_t  slen = strlen(str),
            suf_len = strlen(suffix);
    return slen >= suf_len && g_str_equal(str + slen - suf_len, suffix);
}

gboolean theme_is_editable(const char *filepath)
// Returns TRUE if the theme file is in ~/.config.
{
    return access(filepath, W_OK) == 0;
}

gboolean theme_is_default(const char *filepath)
// Returns TRUE if the theme file is ~/.config/tint2/tint2rc.
{
    gchar *default_path = get_home_config_path();
    gboolean result = g_str_equal(default_path, filepath);
    g_free(default_path);
    return result;
}

char *file_name_from_path(const char *filepath)
// Extracts the file name from the path. Do not free!
{
    char *name = strrchr(filepath, '/');
    if (!name)
        return NULL;
    name++;
    if (!*name)
        return NULL;
    return name;
}

void make_backup(const char *filepath)
{
    gchar *backup_path = g_strdup_printf("%s.backup.%lld", filepath, (long long)time(NULL));
    copy_file(filepath, backup_path);
    g_free(backup_path);
}

gchar *import_no_overwrite(const char *filepath)
// Imports a file to ~/.config/tint2/.
// If a file with the same name exists, it does not overwrite it.
// Takes care of updating the theme list in the GUI.
// Returns the new location. Needs gfree.
{
    gchar *filename = file_name_from_path(filepath);
    if (!filename)
        return NULL;

    gchar *newpath = g_build_filename((fetch_user_config_dir(), user_config_dir), "tint2", filename, NULL);
    if (!g_file_test(newpath, G_FILE_TEST_EXISTS)) {
        copy_file(filepath, newpath);
        theme_list_append(newpath);
        g_timeout_add(SNAPSHOT_TICK, update_snapshot, NULL);
    }

    return newpath;
}

void import_with_overwrite(const char *filepath, const char *newpath)
// Copies a theme file from filepath to newpath.
// Takes care of updating the theme list in the GUI.
{
    gboolean theme_existed = g_file_test(newpath, G_FILE_TEST_EXISTS);
    if (theme_existed)
        make_backup(newpath);

    copy_file(filepath, newpath);

    if (theme_is_editable(newpath)) {
        if (!theme_existed) {
            theme_list_append(newpath);
            g_timeout_add(SNAPSHOT_TICK, update_snapshot, NULL);
        } else {
            int unused = system("killall -SIGUSR1 tint2 || pkill -SIGUSR1 -x tint2");
            (void)unused;
            refresh_theme(newpath);
        }
    }
}

static void menuImportFile();
static void menuSaveAs();
static void menuDelete();
static void menuReset();
static gboolean edit_theme(gpointer ignored);
static void make_selected_theme_default();
static void menuAbout();
static gboolean view_onPopupMenu(GtkWidget *treeview, gpointer userdata);
static gboolean view_onButtonPressed(GtkWidget *treeview, GdkEventButton *event, gpointer userdata);
static void viewRowActivated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data);

static void select_first_theme();
static void load_all_themes();
static void reload_all_themes();

// ====== Globals ======

//~ GtkBuilder *tint2conf_ui;
GtkWidget *g_window;
GtkWidget *tint_cmd;
GtkWidget *view_menu;

GSimpleActionGroup *actionGroup;
GSimpleAction
    *ThemeImportFile, *ThemeSaveAs, *ThemeDelete, *ThemeReset, *ThemeEdit, *ThemeMakeDefault, *ThemeRefresh,
    *RefreshAll, *Quit, *HelpAbout;

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    GtkWidget *vBox = NULL, *scrollbar = NULL;
    gtk_init(&argc, &argv);

#if !GLIB_CHECK_VERSION(2, 31, 0)
    g_thread_init(NULL);
#endif

    {
        gchar *tint2_config_dir = g_build_filename((fetch_user_config_dir(), user_config_dir), "tint2", NULL);
        if (!g_file_test(tint2_config_dir, G_FILE_TEST_IS_DIR))
            g_mkdir_with_parents(tint2_config_dir, 0700);
        g_free(tint2_config_dir);
    }

    g_set_application_name(_("tint2conf"));
    gtk_window_set_default_icon_name("tint2conf");

    // config file uses '.' as decimal separator
    setlocale(LC_NUMERIC, "POSIX");

    // Actions

    actionGroup = g_simple_action_group_new ();

    #define add_action(act, name, callback) do {                                         \
        act = g_simple_action_new ((name), NULL);                                        \
        g_signal_connect (G_OBJECT(act), "activate", G_CALLBACK(callback), NULL);        \
        g_simple_action_group_insert (actionGroup, G_ACTION(act));                       \
    } while (0)
    add_action (ThemeImportFile, "ThemeImportFile",  menuImportFile);
    add_action (ThemeSaveAs,     "ThemeSaveAs",      menuSaveAs);
    add_action (ThemeDelete,     "ThemeDelete",      menuDelete);
    add_action (ThemeReset,      "ThemeReset",       menuReset);
    add_action (ThemeEdit,       "ThemeEdit",        edit_theme);
    add_action (ThemeMakeDefault,"ThemeMakeDefault", make_selected_theme_default);
    add_action (ThemeRefresh,    "ThemeRefresh",     refresh_current_theme);
    add_action (RefreshAll,      "RefreshAll",       reload_all_themes);
    add_action (Quit,            "Quit",             gtk_main_quit);
    add_action (HelpAbout,       "HelpAbout",        menuAbout);
    #undef add_action

    // Keyboard accelerators

    GClosure *cl_import = g_cclosure_new (G_CALLBACK(menuImportFile), NULL, NULL);
    GClosure *cl_about  = g_cclosure_new (G_CALLBACK(menuAbout),      NULL, NULL);
    GClosure *cl_quit   = g_cclosure_new (G_CALLBACK(gtk_main_quit),  NULL, NULL);
    GtkAccelGroup *accelGroup = gtk_accel_group_new ();
    gtk_accel_group_connect (accelGroup, 'n', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE, cl_import);
    gtk_accel_group_connect (accelGroup, 'a', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE, cl_about);
    gtk_accel_group_connect (accelGroup, 'q', GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE, cl_quit);

    // define main layout : container, menubar, toolbar
    g_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (g_window), _("Tint2 panel themes"));
    gtk_window_resize (GTK_WINDOW (g_window), 920, 600);
    gtk_window_add_accel_group (GTK_WINDOW (g_window), accelGroup);
    gtk_widget_insert_action_group (g_window, "", G_ACTION_GROUP (actionGroup));
    g_signal_connect (G_OBJECT (g_window), "destroy", G_CALLBACK (gtk_main_quit), NULL);
    vBox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add (GTK_CONTAINER (g_window), vBox);

    // Menus and toolbar

    GtkWidget *main_menu, *toolbar;

    #define add_menu_item(menu, text, icon_name) do {                                    \
        menu_item = gtk_menu_item_new ();                                                \
        icon = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);             \
        label = gtk_accel_label_new (text);                                              \
        gtk_label_set_text_with_mnemonic (GTK_LABEL (label), text);                      \
        gtk_label_set_xalign (GTK_LABEL (label), 0.0);                                   \
        box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);                               \
        gtk_box_pack_start (GTK_BOX (box), icon,  FALSE, TRUE, 0);                       \
        gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);                        \
        gtk_container_add (GTK_CONTAINER (menu_item), box);                              \
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);                        \
    } while (0)
    #define add_menu_sub(menu, text, icon_name) do {                                     \
        add_menu_item (menu, text, icon_name);                                           \
        submenu = gtk_menu_new ();                                                       \
        gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), submenu);                  \
    } while (0)
    #define add_menu_act(menu, text, icon_name, act, accel) do {                         \
        add_menu_item (menu, text, icon_name);                                           \
        gtk_actionable_set_action_name (GTK_ACTIONABLE (menu_item), act);                \
        if (accel) {                                                                     \
            gtk_accel_label_set_accel_closure (GTK_ACCEL_LABEL (label), accel);          \
        }                                                                                \
    } while (0)
    #define add_menu_sep(menu) do {                                                      \
        menu_item = gtk_separator_menu_item_new ();                                      \
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);                        \
    } while (0)
    #define add_tool_button(action, icon) do {                                           \
        t_item = gtk_tool_button_new (NULL, NULL);                                       \
        gtk_actionable_set_action_name (GTK_ACTIONABLE (t_item), (action));              \
        gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (t_item), (icon));                \
        gtk_toolbar_insert (GTK_TOOLBAR (toolbar), t_item, -1);                          \
    } while (0)

    {
        GtkWidget *submenu, *menu_item, *label, *icon, *box;

        view_menu = gtk_menu_new();
        add_menu_act (view_menu, _("_Edit theme..."), "document-properties",  ".ThemeEdit",    NULL);
        add_menu_act (view_menu, _("Refresh"),        "view-refresh",         ".ThemeRefresh", NULL);
        add_menu_sep (view_menu);
        add_menu_act (view_menu, _("_Reset"),  "document-revert", ".ThemeReset",  NULL);
        add_menu_act (view_menu, _("_Delete"), "edit-delete",     ".ThemeDelete", NULL);

        main_menu = gtk_menu_bar_new();
        add_menu_sub (main_menu, _("_Theme"), NULL);
            add_menu_act (submenu, _("_Import theme..."), "list-add",             ".ThemeImportFile", cl_import);
            add_menu_act (submenu, _("_Edit theme..."),   "document-properties",  ".ThemeEdit",       NULL);
            add_menu_act (submenu, _("_Save as..."),      "document-save-as",     ".ThemeSaveAs",     NULL);
            add_menu_act (submenu, _("_Make default"),    "gtk-apply",            ".ThemeMakeDefault", NULL);
            add_menu_act (submenu, _("_Reset"),           "document-revert",      ".ThemeReset",      NULL);
            add_menu_act (submenu, _("_Delete"),          "edit-delete",          ".ThemeDelete",     NULL);
            add_menu_sep (submenu);
            add_menu_act (submenu, _("Refresh"),      "view-refresh", ".ThemeRefresh", NULL);
            add_menu_act (submenu, _("Refresh all"),  "view-refresh", ".RefreshAll",   NULL);
            add_menu_sep (submenu);
            add_menu_act (submenu, _("_Quit"), "application-exit", ".Quit", cl_quit);
        add_menu_sub (main_menu, _("_Help"), NULL);
            add_menu_act (submenu, _("_About"), "help-about", ".HelpAbout", cl_about);
    }

    gtk_widget_insert_action_group (GTK_WIDGET (view_menu), "", G_ACTION_GROUP (actionGroup));

    GtkToolItem *t_item;
    toolbar = gtk_toolbar_new ();
    add_tool_button (".ThemeEdit",        "document-properties");
    add_tool_button (".ThemeMakeDefault", "gtk-apply");

    #undef add_menu_sub
    #undef add_menu_act
    #undef add_menu_sep
    #undef add_menu_item
    #undef add_tool_button

    gtk_box_pack_start (GTK_BOX(vBox), main_menu, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX(vBox), toolbar,   FALSE, TRUE, 0);

    GtkWidget *box, *label;

    label = gtk_label_new(_("Command to run tint2: "));
    gtk_label_set_xalign (GTK_LABEL(label), 0);

    tint_cmd = gtk_entry_new ();
    gtk_entry_set_text(GTK_ENTRY(tint_cmd), "");

    box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_set_spacing (GTK_BOX(box), 8);
    gtk_box_pack_start (GTK_BOX(box), label, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX(box), tint_cmd, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX(vBox), box, FALSE, TRUE, 0);

    scrollbar = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vBox), scrollbar, TRUE, TRUE, 0);

    // define theme view
    create_view();
    gtk_container_add(GTK_CONTAINER(scrollbar), g_theme_view);
    g_signal_connect(g_theme_view, "button-press-event", (GCallback)view_onButtonPressed, NULL);
    g_signal_connect(g_theme_view, "popup-menu", (GCallback)view_onPopupMenu, NULL);
    g_signal_connect(g_theme_view, "row-activated", G_CALLBACK(viewRowActivated), NULL);
    g_signal_connect(g_theme_view, "cursor-changed", G_CALLBACK(theme_selection_changed), NULL);

    // load themes
    load_all_themes();
    argc--, argv++;
    if (argc > 0) {
        load_specific_themes(argv, argc);
        g_timeout_add(SNAPSHOT_TICK, edit_theme, NULL);
    } else {
        char *themes[2] = {getenv("TINT2_CONFIG"), NULL};
        if (themes[0] && themes[0][0] != '\0') {
            load_specific_themes(themes, 1);
            g_timeout_add(SNAPSHOT_TICK, edit_theme, NULL);
        }
    }

    gtk_widget_show_all(g_window);
    gtk_widget_show_all(view_menu);
    gtk_main();

    icon_theme_common_cleanup ();

    return 0;
}

static void menuAbout()
{
    const char *authors[] = {"Thierry Lorthiois <lorthiois@bbsoft.fr>",
                             "Andreas Fink <andreas.fink85@googlemail.com>",
                             "Christian Ruppert <Spooky85@gmail.com> (Build system)",
                             "Euan Freeman <euan04@gmail.com> (tintwizard http://code.google.com/p/tintwizard)",
                             (NULL)};

    gtk_show_about_dialog(GTK_WINDOW(g_window),
                          "name",
                          g_get_application_name(),
                          "comments",
                          _("Theming tool for tint2 panel"),
                          "version",
                          VERSION_STRING,
                          "copyright",
                          _("Copyright 2009-2017 tint2 team\nTint2 License GNU GPL version 2\nTintwizard License GNU "
                            "GPL version 3"),
                          "logo-icon-name",
                          "tint2conf",
                          "authors",
                          authors,
                          /* Translators: translate "translator-credits" as your name to have it appear in the credits
                             in the "About" dialog */
                          "translator-credits",
                          _("translator-credits"),
                          NULL);
}

// ====== Theme import/copy/delete ======

static void free_data(gpointer data, gpointer userdata)
{
    g_free(data);
}

static void menuImportFile()
// Shows open dialog and copies the selected files to ~ without overwrite.
{
    GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Import theme(s)"),
                                                    GTK_WINDOW(g_window),
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    "gtk-cancel",
                                                    GTK_RESPONSE_CANCEL,
                                                    "gtk-add",
                                                    GTK_RESPONSE_ACCEPT,
                                                    NULL);
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
    gtk_file_chooser_set_select_multiple(chooser, TRUE);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
        gtk_widget_destroy(dialog);
        return;
    }

    GSList *list = gtk_file_chooser_get_filenames(chooser);
    for (GSList *l = list; l; l = l->next) {
        gchar *newpath = import_no_overwrite(l->data);
        g_free(newpath);
    }
    g_slist_foreach(list, free_data, NULL);
    g_slist_free(list);
    gtk_widget_destroy(dialog);
}

gchar *get_current_theme_path()
// Returns the path to the currently selected theme, or NULL if no theme is selected. Needs gfree.
{
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view));
    GtkTreeIter iter;
    GtkTreeModel *model;
    if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(sel), &model, &iter)) {
        gchar *filepath;
        gtk_tree_model_get(model, &iter, COL_THEME_FILE, &filepath, -1);
        return filepath;
    }
    return NULL;
}

gchar *get_selected_theme_or_warn()
// Returns the path to the currently selected theme, or NULL if no theme is selected. Needs gfree.
// Shows an error box if not theme is selected.
{
    gchar *filepath = get_current_theme_path();
    if (!filepath) {
        GtkWidget *w = gtk_message_dialog_new(GTK_WINDOW(g_window),
                                              GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_MESSAGE_ERROR,
                                              GTK_BUTTONS_CLOSE,
                                              _("Please select a theme."));
        g_signal_connect_swapped(w, "response", G_CALLBACK(gtk_widget_destroy), w);
        gtk_widget_show(w);
    }
    return filepath;
}

static void menuSaveAs()
// For the selected theme, shows save dialog (with overwrite confirmation) and copies to the selected file with
// overwrite.
// Shows error box if no theme is selected.
{
    gchar *filepath = get_selected_theme_or_warn();
    if (!filepath)
        return;

    gchar *filename = file_name_from_path(filepath);
    GtkWidget *dialog = gtk_file_chooser_dialog_new(_("Save theme as"),
                                                    GTK_WINDOW(g_window),
                                                    GTK_FILE_CHOOSER_ACTION_SAVE,
                                                    "gtk-cancel",
                                                    GTK_RESPONSE_CANCEL,
                                                    "gtk-save",
                                                    GTK_RESPONSE_ACCEPT,
                                                    NULL);
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
    gtk_file_chooser_set_do_overwrite_confirmation(chooser, FALSE);
    gchar *config_dir = g_build_filename(g_get_home_dir(), ".config", "tint2", NULL);
    gtk_file_chooser_set_current_folder(chooser, config_dir);
    g_free(config_dir);
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.tint2rc");
    gtk_file_filter_add_pattern(filter, "tint2rc");
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
    gtk_file_chooser_set_current_name(chooser, filename);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *newpath = gtk_file_chooser_get_filename(chooser);
        if (!endswith(newpath, ".tint2rc") && !endswith(newpath, "/tint2rc")) {
            gchar *proper_path = g_strdup_printf("%s.tint2rc", newpath);
            g_free(newpath);
            newpath = proper_path;
        }
        if (g_file_test(newpath, G_FILE_TEST_EXISTS)) {
            GtkWidget *w = gtk_message_dialog_new(GTK_WINDOW(g_window),
                                                  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                  GTK_MESSAGE_QUESTION,
                                                  GTK_BUTTONS_YES_NO,
                                                  _("A file named \"%s\" already exists.  Do you want to replace it?"),
                                                  newpath);
            gint response = gtk_dialog_run(GTK_DIALOG(w));
            gtk_widget_destroy(w);
            if (response != GTK_RESPONSE_YES) {
                g_free(newpath);
                gtk_widget_destroy(dialog);
                g_free(filepath);
                return;
            }
        }
        import_with_overwrite(filepath, newpath);
        g_free(newpath);
    }
    gtk_widget_destroy(dialog);
    g_free(filepath);
}

static void menuDelete()
// Deletes the selected theme with confirmation.
{
    gchar *filepath = get_selected_theme_or_warn();
    if (!filepath)
        return;

    GtkWidget *w = gtk_message_dialog_new(GTK_WINDOW(g_window),
                                          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_MESSAGE_QUESTION,
                                          GTK_BUTTONS_YES_NO,
                                          _("Do you really want to delete the selected theme?"));
    gint response = gtk_dialog_run(GTK_DIALOG(w));
    gtk_widget_destroy(w);
    if (response != GTK_RESPONSE_YES) {
        g_free(filepath);
        return;
    }

    GFile *file = g_file_new_for_path(filepath);
    if (g_file_trash(file, NULL, NULL)) {
        remove_theme(filepath);
    }
    g_object_unref(G_OBJECT(file));
    g_free(filepath);
}

static void menuReset()
{
    gchar *filepath = get_selected_theme_or_warn();
    if (!filepath)
        return;
    gchar *filename = file_name_from_path(filepath);
    if (!filename) {
        g_free(filepath);
        return;
    }

    gchar *syspath = find_theme_in_system_dirs(filename);
    if (!syspath)
        syspath = find_theme_in_system_dirs("tint2rc");
    if (!syspath) {
        g_free(filepath);
        return;
    }

    GtkWidget *w = gtk_message_dialog_new(GTK_WINDOW(g_window),
                                          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                          GTK_MESSAGE_QUESTION,
                                          GTK_BUTTONS_YES_NO,
                                          _("Do you really want to reset the selected theme to default?"));
    gint response = gtk_dialog_run(GTK_DIALOG(w));
    gtk_widget_destroy(w);
    if (response != GTK_RESPONSE_YES) {
        g_free(filepath);
        g_free(syspath);
        return;
    }

    import_with_overwrite(syspath, filepath);
    g_free(filepath);
    g_free(syspath);
}

// ====== Theme popup menu ======

static void show_popup_menu(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
    gtk_menu_popup_at_pointer(GTK_MENU(view_menu), (GdkEvent *)event);
}

static gboolean view_onButtonPressed(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
    // single click with the right mouse button?
    if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
        GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
        if (gtk_tree_selection_count_selected_rows(selection) <= 1) {
            // Get tree path for row that was clicked
            GtkTreePath *path;
            if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview),
                                              (gint)event->x,
                                              (gint)event->y,
                                              &path,
                                              NULL,
                                              NULL,
                                              NULL)) {
                gtk_tree_selection_unselect_all(selection);
                gtk_tree_selection_select_path(selection, path);
                gtk_tree_path_free(path);
            }
        }
        show_popup_menu(treeview, event, userdata);
    }
    return FALSE;
}

static gboolean view_onPopupMenu(GtkWidget *treeview, gpointer userdata)
{
    show_popup_menu(treeview, NULL, userdata);
    return TRUE;
}

static void theme_selection_changed(GtkWidget *treeview, gpointer userdata)
{
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view));
    GtkTreeIter iter;
    GtkTreeModel *model;
    if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(sel), &model, &iter)) {
        gchar *filepath;
        gtk_tree_model_get(model, &iter, COL_THEME_FILE, &filepath, -1);
        gboolean isdefault = theme_is_default(filepath);
        gchar *text = isdefault ? strdup_printf( NULL, "tint2") : strdup_printf( NULL, "tint2 -c %s", filepath);
        gtk_entry_set_text(GTK_ENTRY(tint_cmd), text);
        free( text);
        gboolean editable = theme_is_editable(filepath);
        g_free(filepath);
        g_simple_action_set_enabled(ThemeSaveAs,        TRUE);
        g_simple_action_set_enabled(ThemeDelete,        editable);
        g_simple_action_set_enabled(ThemeReset,         editable);
        g_simple_action_set_enabled(ThemeMakeDefault,   !isdefault);
        g_simple_action_set_enabled(ThemeEdit,          TRUE);
        g_simple_action_set_enabled(ThemeRefresh,       TRUE);
    } else {
        gtk_entry_set_text(GTK_ENTRY(tint_cmd), "");
        g_simple_action_set_enabled(ThemeSaveAs,        FALSE);
        g_simple_action_set_enabled(ThemeDelete,        FALSE);
        g_simple_action_set_enabled(ThemeReset,         FALSE);
        g_simple_action_set_enabled(ThemeMakeDefault,   FALSE);
        g_simple_action_set_enabled(ThemeEdit,          FALSE);
        g_simple_action_set_enabled(ThemeRefresh,       FALSE);
    }
}

void select_first_theme()
{
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_theme_view));
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(model, &iter)) {
        GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
        gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view)), &iter);
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(g_theme_view), path, NULL, FALSE, 0, 0);
        gtk_tree_path_free(path);
    }
    theme_selection_changed(NULL, NULL);
}

void select_theme(const char *given_path)
{
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_theme_view));
    GtkTreeIter iter;

    gboolean have_iter = gtk_tree_model_get_iter_first(model, &iter);
    while (have_iter) {
        gchar *filepath;
        gtk_tree_model_get(model, &iter, COL_THEME_FILE, &filepath, -1);
        if (g_str_equal(filepath, given_path)) {
            GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
            gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view)), &iter);
            gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(g_theme_view), path, NULL, FALSE, 0, 0);
            gtk_tree_path_free(path);
            g_free(filepath);
            break;
        }
        g_free(filepath);
        have_iter = gtk_tree_model_iter_next(model, &iter);
    }
    theme_selection_changed(NULL, NULL);
}

static gboolean edit_theme(gpointer ignored)
// Edits the selected theme. If it is read-only, it copies first to ~.
{
    gchar *filepath = get_selected_theme_or_warn();
    if (!filepath)
        return FALSE;

    gboolean editable = theme_is_editable(filepath);
    if (!editable) {
        gchar *newpath = import_no_overwrite(filepath);
        g_free(filepath);
        filepath = newpath;
        select_theme(filepath);
    }
    create_please_wait(GTK_WINDOW(g_window));
    process_events();
    GtkWidget *prop = create_properties();
    config_read_file(filepath);
    save_icon_cache(icon_theme);
    gtk_window_present(GTK_WINDOW(prop));
    g_free(filepath);

    destroy_please_wait();

    return FALSE;
}

static void make_selected_theme_default()
{
    gchar *filepath = get_selected_theme_or_warn();
    if (!filepath)
        return;

    gchar *default_path = get_home_config_path();
    if (g_str_equal(filepath, default_path)) {
        g_free(filepath);
        free( default_path);
        return;
    }

    GtkWidget *w =
        gtk_message_dialog_new(GTK_WINDOW(g_window),
                               GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                               GTK_MESSAGE_QUESTION,
                               GTK_BUTTONS_YES_NO,
                               _("Do you really want to replace the default theme with the selected theme?"));
    gint response = gtk_dialog_run(GTK_DIALOG(w));
    gtk_widget_destroy(w);
    if (response != GTK_RESPONSE_YES) {
        g_free(filepath);
        free( default_path);
        return;
    }

    import_with_overwrite(filepath, default_path);
    select_first_theme();

    g_free(filepath);
    free( default_path);
}

static void viewRowActivated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
    edit_theme(NULL);
}

// ====== Theme load/reload ======

static void ensure_default_theme_exists()
{
    // Without a user tint2rc file, copy the default
    gchar *path_home = strdup_printf( NULL, "%s/tint2/tint2rc", (fetch_user_config_dir(), user_config_dir));
    if (!g_file_test(path_home, G_FILE_TEST_EXISTS)) {
        const gchar *const *system_dirs = g_get_system_config_dirs();
        for (int i = 0; system_dirs[i]; i++)
        {
            gchar *path = strdup_printf( NULL, "%s/tint2/tint2rc", system_dirs[i]);
            if (g_file_test(path, G_FILE_TEST_EXISTS)) {
                copy_file(path, path_home);
                break;
            }
            free( path);
        }
    }
    free( path_home);
}

static int theme_file_valid (const char *file_name)
{
    return  !g_file_test(file_name, G_FILE_TEST_IS_DIR) &&
            !strstr(file_name, "backup") &&
            !strstr(file_name, "copy") &&
            !strchr(file_name, '~') &&
            (endswith(file_name, "tint2rc") || endswith(file_name, ".conf"));
}

static gboolean load_user_themes()
{
    // Load configs from home directory
    gchar *tint2_config_dir = strdup_printf( NULL, "%s/tint2", (fetch_user_config_dir(), user_config_dir));
    GDir *dir = g_dir_open(tint2_config_dir, 0, NULL);
    if (dir == NULL) {
        free( tint2_config_dir);
        return FALSE;
    }
    gboolean found_theme = FALSE;

    const gchar *file_name;
    while ((file_name = g_dir_read_name(dir))) {
        if (theme_file_valid (file_name))
        {
            found_theme = TRUE;
            gchar *path = strdup_printf( NULL, "%s/%s", tint2_config_dir, file_name);
            theme_list_append(path);
            free( path);
        }
    }
    g_dir_close(dir);
    free( tint2_config_dir);

    return found_theme;
}

static gboolean load_themes_from_dirs(const gchar *const *dirs)
{
    gboolean found_theme = FALSE;
    for (int i = 0; dirs[i]; i++) {
        gchar *path_tint2 = strdup_printf( NULL, "%s/tint2", dirs[i]);
        GDir *dir = g_dir_open(path_tint2, 0, NULL);
        if (dir) {
            const gchar *file_name;
            while ((file_name = g_dir_read_name(dir))) {
                if (theme_file_valid (file_name))
                {
                    found_theme = TRUE;
                    gchar *path = strdup_printf( NULL, "%s/%s", path_tint2, file_name);
                    theme_list_append(path);
                    free( path);
                }
            }
            g_dir_close(dir);
        }
        free( path_tint2);
    }
    return found_theme;
}

static gboolean load_system_themes()
{
    gboolean found_theme = FALSE;
    if (load_themes_from_dirs(g_get_system_config_dirs()))
        found_theme = TRUE;
    if (load_themes_from_dirs(g_get_system_data_dirs()))
        found_theme = TRUE;
    return found_theme;
}

static gchar *find_theme_in_dirs(const gchar *const *dirs, const gchar *given_name)
{
    for (int i = 0; dirs[i]; i++) {
        gchar *filepath = strdup_printf( NULL, "%s/tint2/%s", dirs[i], given_name);
        if (g_file_test(filepath, G_FILE_TEST_EXISTS))
            return filepath;
        free( filepath);
    }
    return NULL;
}

static gchar *find_theme_in_system_dirs(const gchar *given_name)
{
    gchar *result = find_theme_in_dirs(g_get_system_config_dirs(), given_name);
    if (result)
        return result;
    return find_theme_in_dirs(g_get_system_data_dirs(), given_name);
}

static void load_all_themes()
{
    ensure_default_theme_exists();

    gtk_list_store_clear(GTK_LIST_STORE(theme_list_store));
    theme_selection_changed(NULL, NULL);

    gboolean found_themes = FALSE;
    if (load_user_themes())
        found_themes = TRUE;
    if (load_system_themes())
        found_themes = TRUE;

    if (found_themes) {
        select_first_theme();

        GtkTreeIter iter;
        GtkTreeModel *model;
        gboolean have_iter;

        model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_theme_view));
        have_iter = gtk_tree_model_get_iter_first(model, &iter);
        while (have_iter) {
            gtk_list_store_set(theme_list_store, &iter, COL_SNAPSHOT, NULL, -1);
            have_iter = gtk_tree_model_iter_next(model, &iter);
        }

        g_timeout_add(SNAPSHOT_TICK, update_snapshot, NULL);
    }
}

static void reload_all_themes()
{
    ensure_default_theme_exists();

    gtk_list_store_clear(GTK_LIST_STORE(theme_list_store));
    theme_selection_changed(NULL, NULL);

    gboolean found_themes = FALSE;
    if (load_user_themes())
        found_themes = TRUE;
    if (load_system_themes())
        found_themes = TRUE;

    if (found_themes) {
        select_first_theme();

        GtkTreeIter iter;
        GtkTreeModel *model;
        gboolean have_iter;

        model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_theme_view));
        have_iter = gtk_tree_model_get_iter_first(model, &iter);
        while (have_iter) {
            gtk_list_store_set(theme_list_store, &iter, COL_SNAPSHOT, NULL, COL_FORCE_REFRESH, TRUE, -1);
            have_iter = gtk_tree_model_iter_next(model, &iter);
        }

        g_timeout_add(SNAPSHOT_TICK, update_snapshot, NULL);
    }
}

static void load_specific_themes(char **paths, int count)
{
    ensure_default_theme_exists();

    gboolean found_themes = FALSE;
    while (count > 0) {
        // Load configs
        const char *file_name = paths[0];
        paths++, count--;
        if (g_file_test(file_name, G_FILE_TEST_IS_REGULAR) || g_file_test(file_name, G_FILE_TEST_IS_SYMLINK)) {
            theme_list_append(file_name);
            if (!found_themes) {
                select_theme(file_name);
                found_themes = TRUE;
            }
        }
    }

    if (found_themes) {
        GtkTreeIter iter;
        GtkTreeModel *model;
        gboolean have_iter;

        model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_theme_view));
        have_iter = gtk_tree_model_get_iter_first(model, &iter);
        while (have_iter) {
            gtk_list_store_set(theme_list_store, &iter, COL_SNAPSHOT, NULL, -1);
            have_iter = gtk_tree_model_iter_next(model, &iter);
        }

        g_timeout_add(SNAPSHOT_TICK, update_snapshot, NULL);
    }
}

void refresh_current_theme()
{
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view));
    GtkTreeIter iter;
    GtkTreeModel *model;
    if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(sel), &model, &iter)) {
        gtk_list_store_set(theme_list_store, &iter, COL_SNAPSHOT, NULL, -1);
        g_timeout_add(SNAPSHOT_TICK, update_snapshot, NULL);
    }
}

void refresh_theme(const char *given_path)
{
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_theme_view));
    GtkTreeIter iter;

    gboolean have_iter = gtk_tree_model_get_iter_first(model, &iter);
    while (have_iter) {
        gchar *filepath;
        gtk_tree_model_get(model, &iter, COL_THEME_FILE, &filepath, -1);
        if (g_str_equal(filepath, given_path)) {
            gtk_list_store_set(theme_list_store, &iter, COL_SNAPSHOT, NULL, -1);
            g_timeout_add(SNAPSHOT_TICK, update_snapshot, NULL);
            g_free(filepath);
            break;
        }
        g_free(filepath);
        have_iter = gtk_tree_model_iter_next(model, &iter);
    }
}

void remove_theme(const char *given_path)
{
    GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(g_theme_view));
    GtkTreeIter iter;

    gboolean have_iter = gtk_tree_model_get_iter_first(model, &iter);
    while (have_iter) {
        gchar *filepath;
        gtk_tree_model_get(model, &iter, COL_THEME_FILE, &filepath, -1);
        if (g_str_equal(filepath, given_path)) {
            gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
            theme_selection_changed(NULL, NULL);
            g_free(filepath);
            break;
        }
        g_free(filepath);
        have_iter = gtk_tree_model_iter_next(model, &iter);
    }
}
