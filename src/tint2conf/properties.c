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

#include "gui.h"
#include "background_gui.h"
#include "gradient_gui.h"
#include "strlcat.h"
#include "../util/common.h"

#include <stddef.h>

GtkWidget *scale_relative_to_dpi, *scale_relative_to_screen_height;
GtkWidget *panel_width, *panel_height, *panel_margin_x, *panel_margin_y, *panel_padding_x, *panel_padding_y,
    *panel_spacing;
GtkWidget *panel_wm_menu, *panel_dock, *panel_pivot_struts, *panel_autohide, *panel_autohide_show_time,
    *panel_autohide_hide_time, *panel_autohide_size;
GtkWidget *panel_combo_strut_policy, *panel_combo_layer, *panel_combo_width_type, *panel_combo_height_type,
    *panel_combo_monitor;
GtkWidget *panel_window_name, *disable_transparency;
GtkWidget *panel_mouse_effects;
GtkWidget *mouse_hover_icon_opacity, *mouse_hover_icon_saturation, *mouse_hover_icon_brightness;
GtkWidget *mouse_pressed_icon_opacity, *mouse_pressed_icon_saturation, *mouse_pressed_icon_brightness;
GtkWidget *panel_shrink;

GtkListStore *panel_items, *all_items;
GtkWidget *panel_items_view, *all_items_view;

GtkWidget *screen_position[12];
GSList *screen_position_group;
GtkWidget *panel_background;

GtkWidget *stack;

// taskbar
GtkWidget *taskbar_show_desktop, *taskbar_show_name, *taskbar_padding_x, *taskbar_padding_y, *taskbar_spacing;
GtkWidget *taskbar_hide_inactive_tasks, *taskbar_hide_diff_monitor, *taskbar_hide_diff_desktop;
GtkWidget *taskbar_name_padding_x, *taskbar_name_padding_y, *taskbar_name_inactive_color, *taskbar_name_active_color;
GtkWidget *taskbar_name_font, *taskbar_name_font_set;
GtkWidget *taskbar_active_background, *taskbar_inactive_background;
GtkWidget *taskbar_name_active_background, *taskbar_name_inactive_background;
GtkWidget *taskbar_distribute_size, *taskbar_sort_order, *taskbar_alignment, *taskbar_always_show_all_desktop_tasks;
GtkWidget *taskbar_hide_empty;

// task
GtkWidget *task_mouse_left, *task_mouse_middle, *task_mouse_right, *task_mouse_scroll_up, *task_mouse_scroll_down;
GtkWidget *task_show_icon, *task_show_text, *task_align_center, *font_shadow;
GtkWidget *task_maximum_width, *task_maximum_height, *task_padding_x, *task_padding_y, *task_spacing;
GtkWidget *task_font, *task_font_set;
GtkWidget *task_default_color, *task_default_color_set, *task_default_icon_opacity, *task_default_icon_osb_set,
    *task_default_icon_saturation, *task_default_icon_brightness, *task_default_background,
    *task_default_background_set;
GtkWidget *task_normal_color, *task_normal_color_set, *task_normal_icon_opacity, *task_normal_icon_osb_set,
    *task_normal_icon_saturation, *task_normal_icon_brightness, *task_normal_background, *task_normal_background_set;
GtkWidget *task_active_color, *task_active_color_set, *task_active_icon_opacity, *task_active_icon_osb_set,
    *task_active_icon_saturation, *task_active_icon_brightness, *task_active_background, *task_active_background_set;
GtkWidget *task_urgent_color, *task_urgent_color_set, *task_urgent_icon_opacity, *task_urgent_icon_osb_set,
    *task_urgent_icon_saturation, *task_urgent_icon_brightness, *task_urgent_background, *task_urgent_background_set;
GtkWidget *task_urgent_blinks;
GtkWidget *task_iconified_color, *task_iconified_color_set, *task_iconified_icon_opacity, *task_iconified_icon_osb_set,
    *task_iconified_icon_saturation, *task_iconified_icon_brightness, *task_iconified_background,
    *task_iconified_background_set;

// clock
GtkWidget *clock_format_line1, *clock_format_line2, *clock_tmz_line1, *clock_tmz_line2;
GtkWidget *clock_left_command, *clock_right_command;
GtkWidget *clock_mclick_command, *clock_rclick_command, *clock_uwheel_command, *clock_dwheel_command;
GtkWidget *clock_padding_x, *clock_padding_y;
GtkWidget *clock_font_line1, *clock_font_line1_set, *clock_font_line2, *clock_font_line2_set, *clock_font_color;
GtkWidget *clock_background;

// battery
GtkWidget *battery_hide_if_higher, *battery_alert_if_lower, *battery_alert_cmd, *battery_alert_full_cmd;
GtkWidget *battery_padding_x, *battery_padding_y;
GtkWidget *battery_font_line1, *battery_font_line1_set, *battery_font_line2, *battery_font_line2_set,
    *battery_font_color, *battery_format1, *battery_format2;
GtkWidget *battery_background;
GtkWidget *battery_tooltip;
GtkWidget *battery_left_command, *battery_mclick_command, *battery_right_command, *battery_uwheel_command,
    *battery_dwheel_command;
GtkWidget *ac_connected_cmd, *ac_disconnected_cmd;

// systray
GtkWidget *systray_icon_order, *systray_padding_x, *systray_padding_y, *systray_spacing;
GtkWidget *systray_icon_size, *systray_icon_opacity, *systray_icon_saturation, *systray_icon_brightness;
GtkWidget *systray_background, *systray_monitor, *systray_name_filter;

// tooltip
GtkWidget *tooltip_padding_x, *tooltip_padding_y, *tooltip_font, *tooltip_font_set, *tooltip_font_color;
GtkWidget *tooltip_task_show, *tooltip_show_after, *tooltip_hide_after, *tooltip_task_thumbnail, *tooltip_task_thumbnail_size;
GtkWidget *clock_format_tooltip, *clock_tmz_tooltip;
GtkWidget *tooltip_background;

// Separators
GArray *separators;

// Executors
GArray *executors;

// Buttons
GArray *buttons;

// launcher

GtkListStore *launcher_apps, *all_apps;
GtkWidget *launcher_apps_view, *all_apps_view;
GtkWidget *launcher_apps_dirs;

GtkWidget *launcher_icon_size, *launcher_icon_theme, *launcher_padding_x, *launcher_padding_y, *launcher_spacing;
GtkWidget *launcher_icon_opacity, *launcher_icon_saturation, *launcher_icon_brightness;
GtkWidget *margin_x, *margin_y;
GtkWidget *launcher_background, *launcher_icon_background;
GtkWidget *startup_notifications;
IconThemeWrapper *icon_theme;
GtkWidget *launcher_tooltip;
GtkWidget *launcher_icon_theme_override;

GtkWidget *addScrollBarToWidget(GtkWidget *widget);
gboolean gtk_tree_model_iter_prev_tint2(GtkTreeModel *model, GtkTreeIter *iter);

void create_general(GtkWidget *parent);
void init_panel_page (GtkWidget *parent);
void init_panel_items_page (GtkWidget *parent);
void init_launcher_page (GtkWidget *parent, GtkWindow *window);
gchar *get_default_theme_name();
void icon_theme_changed();
void load_icons(GtkListStore *apps);
void init_taskbar_page (GtkWidget *parent);
void init_task_page (GtkWidget *parent);
void create_task_status(GtkWidget *notebook,
                        char *name,
                        char *text,
                        GtkWidget **task_status_color,
                        GtkWidget **task_status_color_set,
                        GtkWidget **task_status_icon_opacity,
                        GtkWidget **task_status_icon_osb_set,
                        GtkWidget **task_status_icon_saturation,
                        GtkWidget **task_status_icon_brightness,
                        GtkWidget **task_status_background,
                        GtkWidget **task_status_background_set);
void create_separator(GtkWidget *parent, int i);
void create_execp(GtkWidget *parent, int i);
void create_button(GtkWidget *parent, int i);
void init_clock_page (GtkWidget *parent);
void init_systemtray_page (GtkWidget *parent);
void init_battery_page (GtkWidget *parent);
void init_tooltip_page (GtkWidget *parent);
void panel_add_item(GtkWidget *widget, gpointer data);
void panel_remove_item(GtkWidget *widget, gpointer data);
void panel_move_item_down(GtkWidget *widget, gpointer data);
void panel_move_item_up(GtkWidget *widget, gpointer data);

const gchar *get_default_font()
{
    GtkSettings *settings = gtk_settings_get_default();
    gchar *default_font;
    g_object_get(settings, "gtk-font-name", &default_font, NULL);
    if (default_font)
        return default_font;
    return "sans 10";
}

void applyClicked(GtkWidget *widget, gpointer data)
{
    gchar *filepath = get_current_theme_path();
    if (filepath) {
        if (config_is_manual(filepath)) {
            gchar *backup_path = g_strdup_printf("%s.backup.%lld", filepath, (long long)time(NULL));
            copy_file(filepath, backup_path);
            g_free(backup_path);
        }

        config_save_file(filepath);
    }
    int unused = system("killall -SIGUSR1 tint2 || pkill -SIGUSR1 -x tint2");
    (void)unused;
    g_free(filepath);
    refresh_current_theme();
}

void cancelClicked(GtkWidget *widget, gpointer data)
{
    GtkWidget *view = (GtkWidget *)data;
    gtk_widget_destroy(view);
}

void okClicked(GtkWidget *widget, gpointer data)
{
    applyClicked(widget, data);
    cancelClicked(widget, data);
}

void font_set_callback(GtkWidget *widget, gpointer data)
{
    gtk_widget_set_sensitive(data, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

GtkWidget *create_properties()
{
    GtkWidget *view, *dialog_vbox3, *button, *frame, *sidebar, *content_box;
    GtkWidget *page_panel, *page_panel_items, *page_launcher, *page_taskbar, *page_battery, *page_clock, *page_tooltip,
        *page_systemtray, *page_task, *page_background, *page_gradient;

    separators = g_array_new(FALSE, TRUE, sizeof(Separator));
    executors = g_array_new(FALSE, TRUE, sizeof(Executor));
    buttons = g_array_new(FALSE, TRUE, sizeof(Button));

    // global layer
    view = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(view), _("Properties"));
    gtk_window_set_modal(GTK_WINDOW(view), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(view), 920, 600);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(view), TRUE);
    gtk_window_set_type_hint(GTK_WINDOW(view), GDK_WINDOW_TYPE_HINT_DIALOG);
    
    #define create_dialog_stack_button(name, response, callback, data) \
    do {                                                                  \
        button = gtk_button_new_from_stock(name);                         \
        gtk_widget_show(button);                                          \
        gtk_dialog_add_action_widget(GTK_DIALOG(view), button, response); \
        g_signal_connect(button, "clicked", G_CALLBACK(callback), data);  \
        gtk_widget_set_can_default(button, TRUE);                         \
    } while (0)
    create_dialog_stack_button("gtk-apply",  GTK_RESPONSE_APPLY,   applyClicked, NULL);
    create_dialog_stack_button("gtk-cancel", GTK_RESPONSE_CANCEL, cancelClicked, view);
    create_dialog_stack_button("gtk-ok",     GTK_RESPONSE_OK,         okClicked, view);
    #undef create_dialog_stack_button
    
    // notebook
    
    stack = gtk_stack_new();
    gtk_stack_set_homogeneous(GTK_STACK(stack), FALSE);

    #define create_page(text, widget) \
    do {                               \
        widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, DEFAULT_HOR_SPACING);  \
        gtk_container_set_border_width(GTK_CONTAINER(widget), 10);            \
        gtk_stack_add_titled(GTK_STACK(stack), widget, NULL, text); \
        gtk_widget_show(widget);                                                          \
    } while (0)
    create_page(_("Gradients"),    page_gradient);
    create_page(_("Backgrounds"),  page_background);
    create_page(_("Panel"),        page_panel);
    create_page(_("Panel items"),  page_panel_items);
    create_page(_("Taskbar"),      page_taskbar);
    create_page(_("Task buttons"), page_task);
    create_page(_("Launcher"),     page_launcher);
    create_page(_("Clock"),        page_clock);
    create_page(_("System tray"),  page_systemtray);
    create_page(_("Battery"),      page_battery);
    create_page(_("Tooltip"),      page_tooltip);
    #undef create_page

    init_gradient_page (page_gradient);
    init_background_page (page_background);
    init_panel_page (page_panel);
    init_panel_items_page (page_panel_items);
    init_taskbar_page (page_taskbar);
    init_task_page (page_task);
    init_launcher_page (page_launcher, GTK_WINDOW(view));
    init_clock_page (page_clock);
    init_systemtray_page (page_systemtray);
    init_battery_page (page_battery);
    init_tooltip_page (page_tooltip);

    sidebar = gtk_stack_sidebar_new();
    gtk_stack_sidebar_set_stack(GTK_STACK_SIDEBAR(sidebar), GTK_STACK(stack));
    gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW(gtk_bin_get_child(GTK_BIN(sidebar))), GTK_CORNER_TOP_RIGHT);

    content_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(content_box), sidebar, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(content_box), addScrollBarToWidget(stack), TRUE, TRUE, 0);
    
    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), content_box);
    
    dialog_vbox3 = gtk_dialog_get_content_area(GTK_DIALOG(view));
    gtk_box_pack_start(GTK_BOX(dialog_vbox3), frame, TRUE, TRUE, 0);

    gtk_widget_show(sidebar);
    gtk_widget_show(stack);
    gtk_widget_show(content_box);
    gtk_widget_show(frame);
    gtk_widget_show(dialog_vbox3);

    return view;
}

void change_paragraph(GtkWidget *widget)
{
    GtkWidget *hbox;
    hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(widget), hbox, FALSE, FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);
}

// Color and GdkRGBA should be compatible
// as long as GDK doesn't change layout

void gdkRGBA2CairoColor(GdkRGBA *gcolor, Color *color)
{
    if (offsetof(Color, rgb[0]) == offsetof(GdkRGBA, red)   &&
        offsetof(Color, rgb[1]) == offsetof(GdkRGBA, green) &&
        offsetof(Color, rgb[2]) == offsetof(GdkRGBA, blue)  &&
        offsetof(Color, alpha) == offsetof(GdkRGBA, alpha))
    {
        *color = *(Color *)gcolor;
    } else {
        color->rgb[0] = gcolor->red;
        color->rgb[1] = gcolor->green;
        color->rgb[2] = gcolor->blue;
        color->alpha = gcolor->alpha;
    }
}

void cairoColor2GdkRGBA(Color *color, GdkRGBA *gcolor)
{
    if (offsetof(Color, rgb[0]) == offsetof(GdkRGBA, red)   &&
        offsetof(Color, rgb[1]) == offsetof(GdkRGBA, green) &&
        offsetof(Color, rgb[2]) == offsetof(GdkRGBA, blue)  &&
        offsetof(Color, alpha) == offsetof(GdkRGBA, alpha))
    {
        *gcolor = *(GdkRGBA *)color;
    } else {
        gcolor->red   = color->rgb[0];
        gcolor->green = color->rgb[1];
        gcolor->blue  = color->rgb[2];
        gcolor->alpha = color->alpha;
    }
}

int get_model_length(GtkTreeModel *model)
{
    int i;
    for (i = 0;; i++) {
        GtkTreePath *path;
        GtkTreeIter iter;

        path = gtk_tree_path_new_from_indices(i, -1);
        gboolean end = gtk_tree_model_get_iter(model, &iter, path);
        gtk_tree_path_free(path);

        if (!end) {
            return i;
        }
    }
}

void init_panel_page (GtkWidget *parent)
{
    int i;
    GtkWidget *table, *hbox, *position;
    GtkWidget *label;
    int row, col;

    label = gtk_label_new(_("<b>Geometry</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);
    hbox = gtk_hbox_new(FALSE, 20);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(parent), hbox, FALSE, FALSE, 0);

    table = gtk_table_new(2, 10, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

    row = 0;
    col = 2;
    label = gtk_label_new(_("Position"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    position = gtk_table_new(5, 5, FALSE);
    gtk_widget_show(position);
    for (i = 0; i < 12; ++i) {
        GSList *group = i == 0 ? NULL : gtk_radio_button_get_group(GTK_RADIO_BUTTON(screen_position[0]));
        screen_position[i] = gtk_radio_button_new(group);
        g_object_set(screen_position[i], "draw-indicator", FALSE, NULL);
        gtk_widget_show(screen_position[i]);

        if (i <= 2 || i >= 9) {
            gtk_widget_set_size_request(screen_position[i], 30, 15);
        } else {
            gtk_widget_set_size_request(screen_position[i], 15, 25);
        }
    }
    gtk_table_attach_defaults(GTK_TABLE(position), screen_position[0], 1, 2, 0, 1);
    gtk_widget_set_tooltip_text(screen_position[0], _("Position on screen: top-left, horizontal panel"));
    gtk_table_attach_defaults(GTK_TABLE(position), screen_position[1], 2, 3, 0, 1);
    gtk_widget_set_tooltip_text(screen_position[1], _("Position on screen: top-center, horizontal panel"));
    gtk_table_attach_defaults(GTK_TABLE(position), screen_position[2], 3, 4, 0, 1);
    gtk_widget_set_tooltip_text(screen_position[2], _("Position on screen: top-right, horizontal panel"));
    gtk_table_attach_defaults(GTK_TABLE(position), screen_position[3], 0, 1, 1, 2);
    gtk_widget_set_tooltip_text(screen_position[3], _("Position on screen: top-left, vertical panel"));
    gtk_table_attach_defaults(GTK_TABLE(position), screen_position[4], 0, 1, 2, 3);
    gtk_widget_set_tooltip_text(screen_position[4], _("Position on screen: center-left, vertical panel"));
    gtk_table_attach_defaults(GTK_TABLE(position), screen_position[5], 0, 1, 3, 4);
    gtk_widget_set_tooltip_text(screen_position[5], _("Position on screen: bottom-left, vertical panel"));
    gtk_table_attach_defaults(GTK_TABLE(position), screen_position[6], 4, 5, 1, 2);
    gtk_widget_set_tooltip_text(screen_position[6], _("Position on screen: top-right, vertical panel"));
    gtk_table_attach_defaults(GTK_TABLE(position), screen_position[7], 4, 5, 2, 3);
    gtk_widget_set_tooltip_text(screen_position[7], _("Position on screen: center-right, vertical panel"));
    gtk_table_attach_defaults(GTK_TABLE(position), screen_position[8], 4, 5, 3, 4);
    gtk_widget_set_tooltip_text(screen_position[8], _("Position on screen: bottom-right, vertical panel"));
    gtk_table_attach_defaults(GTK_TABLE(position), screen_position[9], 1, 2, 4, 5);
    gtk_widget_set_tooltip_text(screen_position[9], _("Position on screen: bottom-left, horizontal panel"));
    gtk_table_attach_defaults(GTK_TABLE(position), screen_position[10], 2, 3, 4, 5);
    gtk_widget_set_tooltip_text(screen_position[10], _("Position on screen: bottom-center, horizontal panel"));
    gtk_table_attach_defaults(GTK_TABLE(position), screen_position[11], 3, 4, 4, 5);
    gtk_widget_set_tooltip_text(screen_position[11], _("Position on screen: bottom-right, horizontal panel"));
    gtk_table_attach(GTK_TABLE(table), position, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);

    row++;
    col = 2;
    label = gtk_label_new(_("Monitor"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    panel_combo_monitor = gtk_combo_box_text_new();
    gtk_widget_show(panel_combo_monitor);
    gtk_table_attach(GTK_TABLE(table), panel_combo_monitor, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(panel_combo_monitor), _("All"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(panel_combo_monitor), _("Primary"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(panel_combo_monitor), _("1"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(panel_combo_monitor), _("2"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(panel_combo_monitor), _("3"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(panel_combo_monitor), _("4"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(panel_combo_monitor), _("5"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(panel_combo_monitor), _("6"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_monitor), 0);
    gtk_widget_set_tooltip_text(panel_combo_monitor, _("The monitor on which the panel is placed"));

    row++;
    col = 2;
    label = gtk_label_new(_("Length"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    panel_width = gtk_spin_button_new_with_range(0, 9000, 1);
    gtk_widget_show(panel_width);
    gtk_table_attach(GTK_TABLE(table), panel_width, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(panel_width,
                         _("The length of the panel (width for horizontal panels, height for vertical panels)"));

    panel_combo_width_type = gtk_combo_box_text_new();
    gtk_widget_show(panel_combo_width_type);
    gtk_table_attach(GTK_TABLE(table), panel_combo_width_type, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(panel_combo_width_type), _("Percent"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(panel_combo_width_type), _("Pixels"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_width_type), 0);
    gtk_widget_set_tooltip_text(panel_combo_width_type,
                         _("The units used to specify the length of the panel: pixels or percentage of the monitor "
                           "size"));

    row++;
    col = 2;
    label = gtk_label_new(_("Compact"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    panel_shrink = gtk_check_button_new();
    gtk_widget_show(panel_shrink);
    gtk_table_attach(GTK_TABLE(table), panel_shrink, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    row++;
    col = 2;
    label = gtk_label_new(_("Size"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    panel_height = gtk_spin_button_new_with_range(0, 9000, 1);
    gtk_widget_show(panel_height);
    gtk_table_attach(GTK_TABLE(table), panel_height, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(panel_height,
                         _("The size of the panel (height for horizontal panels, width for vertical panels)"));

    panel_combo_height_type = gtk_combo_box_text_new();
    gtk_widget_show(panel_combo_height_type);
    gtk_table_attach(GTK_TABLE(table), panel_combo_height_type, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(panel_combo_height_type), _("Percent"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(panel_combo_height_type), _("Pixels"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_height_type), 0);
    gtk_widget_set_tooltip_text(panel_combo_height_type,
                         _("The units used to specify the size of the panel: pixels or percentage of the monitor size"));

    row++;
    col = 2;
    label = gtk_label_new(_("Horizontal margin"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    panel_margin_x = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(panel_margin_x);
    gtk_table_attach(GTK_TABLE(table), panel_margin_x, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(panel_margin_x,
                         _("Creates a space between the panel and the edge of the monitor. "
                           "For left-aligned panels, the space is created on the right of the panel; "
                           "for right-aligned panels, it is created on the left; "
                           "for centered panels, it is evenly distributed on both sides of the panel."));

    row++;
    col = 2;
    label = gtk_label_new(_("Vertical margin"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    panel_margin_y = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(panel_margin_y);
    gtk_table_attach(GTK_TABLE(table), panel_margin_y, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(panel_margin_y,
                         _("Creates a space between the panel and the edge of the monitor. "
                           "For top-aligned panels, the space is created on the bottom of the panel; "
                           "for bottom-aligned panels, it is created on the top; "
                           "for centered panels, it is evenly distributed on both sides of the panel."));

    row++;
    col = 2;
    label = gtk_label_new(_("Scale relative to DPI"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    scale_relative_to_dpi = gtk_spin_button_new_with_range(0, 9000, 1);
    gtk_widget_show(scale_relative_to_dpi);
    gtk_table_attach(GTK_TABLE(table), scale_relative_to_dpi, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    row++;
    col = 2;
    label = gtk_label_new(_("Scale relative to screen height"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    scale_relative_to_screen_height = gtk_spin_button_new_with_range(0, 9000, 1);
    gtk_widget_show(scale_relative_to_screen_height);
    gtk_table_attach(GTK_TABLE(table), scale_relative_to_screen_height, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    change_paragraph(parent);

    label = gtk_label_new(_("<b>Appearance</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(2, 10, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

    row = 0;
    col = 2;
    label = gtk_label_new(_("Background"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    panel_background = create_background_combo(_("Panel"));
    gtk_widget_show(panel_background);
    gtk_table_attach(GTK_TABLE(table), panel_background, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(panel_background,
                         _("Selects the background used to display the panel. "
                           "Backgrounds can be edited in the Backgrounds tab."));

    row++;
    col = 2;
    label = gtk_label_new(_("Horizontal padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    panel_padding_x = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(panel_padding_x);
    gtk_table_attach(GTK_TABLE(table), panel_padding_x, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(panel_padding_x,
                         _("Specifies the horizontal padding of the panel. "
                           "This is the space between the border of the panel and the elements inside."));

    row++;
    col = 2;
    label = gtk_label_new(_("Vertical padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    panel_padding_y = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(panel_padding_y);
    gtk_table_attach(GTK_TABLE(table), panel_padding_y, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(panel_padding_y,
                         _("Specifies the vertical padding of the panel. "
                           "This is the space between the border of the panel and the elements inside."));

    row++;
    col = 2;
    label = gtk_label_new(_("Spacing"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    panel_spacing = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(panel_spacing);
    gtk_table_attach(GTK_TABLE(table), panel_spacing, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(panel_spacing, _("Specifies the spacing between elements inside the panel."));

    row++;
    col = 2;
    label = gtk_label_new(_("Ignore compositor"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    disable_transparency = gtk_check_button_new();
    gtk_widget_show(disable_transparency);
    gtk_table_attach(GTK_TABLE(table), disable_transparency, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(disable_transparency,
                         _("If enabled, the compositor will not be used to draw a transparent panel. "
                           "May fix display corruption problems on broken graphics stacks."));

    row++, col = 2;
    label = gtk_label_new(_("Font shadows"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    font_shadow = gtk_check_button_new();
    gtk_widget_show(font_shadow);
    gtk_table_attach(GTK_TABLE(table), font_shadow, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(font_shadow,
                         _("If enabled, a shadow will be drawn behind text. "
                           "This may improve legibility on transparent panels."));

    row++;
    col = 2;
    label = gtk_label_new(_("Mouse effects"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    panel_mouse_effects = gtk_check_button_new();
    gtk_widget_show(panel_mouse_effects);
    gtk_table_attach(GTK_TABLE(table), panel_mouse_effects, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(panel_mouse_effects,
                         _("Clickable interface items change appearance when the mouse is moved over them."));

    row++;
    col = 2;
    label = gtk_label_new(_("Icon opacity (hovered)"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    mouse_hover_icon_opacity = gtk_spin_button_new_with_range(0, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(mouse_hover_icon_opacity), 100);
    gtk_widget_show(mouse_hover_icon_opacity);
    gtk_table_attach(GTK_TABLE(table), mouse_hover_icon_opacity, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(mouse_hover_icon_opacity,
                         _("Specifies the opacity adjustment of the icons under the mouse, in percent."));

    row++;
    col = 2;
    label = gtk_label_new(_("Icon saturation (hovered)"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    mouse_hover_icon_saturation = gtk_spin_button_new_with_range(-100, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(mouse_hover_icon_saturation), 0);
    gtk_widget_show(mouse_hover_icon_saturation);
    gtk_table_attach(GTK_TABLE(table), mouse_hover_icon_saturation, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(mouse_hover_icon_saturation,
                         _("Specifies the saturation adjustment of the icons under the mouse, in percent."));

    row++;
    col = 2;
    label = gtk_label_new(_("Icon brightness (hovered)"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    mouse_hover_icon_brightness = gtk_spin_button_new_with_range(-100, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(mouse_hover_icon_brightness), 10);
    gtk_widget_show(mouse_hover_icon_brightness);
    gtk_table_attach(GTK_TABLE(table), mouse_hover_icon_brightness, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(mouse_hover_icon_brightness,
                         _("Specifies the brightness adjustment of the icons under the mouse, in percent."));

    row++;
    col = 2;
    label = gtk_label_new(_("Icon opacity (pressed)"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    mouse_pressed_icon_opacity = gtk_spin_button_new_with_range(0, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(mouse_pressed_icon_opacity), 100);
    gtk_widget_show(mouse_pressed_icon_opacity);
    gtk_table_attach(GTK_TABLE(table), mouse_pressed_icon_opacity, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(mouse_pressed_icon_opacity,
                                _("Specifies the opacity adjustment of the icons on mouse button press, in percent."));

    row++;
    col = 2;
    label = gtk_label_new(_("Icon saturation (pressed)"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    mouse_pressed_icon_saturation = gtk_spin_button_new_with_range(-100, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(mouse_pressed_icon_saturation), 0);
    gtk_widget_show(mouse_pressed_icon_saturation);
    gtk_table_attach(GTK_TABLE(table), mouse_pressed_icon_saturation, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(mouse_pressed_icon_saturation,
                                _("Specifies the saturation adjustment of the icons on mouse button press, in percent."));

    row++;
    col = 2;
    label = gtk_label_new(_("Icon brightness (pressed)"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    mouse_pressed_icon_brightness = gtk_spin_button_new_with_range(-100, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(mouse_pressed_icon_brightness), 0);
    gtk_widget_show(mouse_pressed_icon_brightness);
    gtk_table_attach(GTK_TABLE(table), mouse_pressed_icon_brightness, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(mouse_pressed_icon_brightness,
                                _("Specifies the brightness adjustment of the icons on mouse button press, in percent."));

    change_paragraph(parent);

    label = gtk_label_new(_("<b>Autohide</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(2, 10, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

    row = 0;
    col = 2;
    label = gtk_label_new(_("Autohide"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    panel_autohide = gtk_check_button_new();
    gtk_widget_show(panel_autohide);
    gtk_table_attach(GTK_TABLE(table), panel_autohide, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(panel_autohide,
                                _("If enabled, the panel is hidden when the mouse cursor leaves the panel."));

    row++;
    col = 2;
    label = gtk_label_new(_("Show panel after"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    panel_autohide_show_time = gtk_spin_button_new_with_range(0, 10000, 0.1);
    gtk_widget_show(panel_autohide_show_time);
    gtk_table_attach(GTK_TABLE(table), panel_autohide_show_time, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(panel_autohide_show_time,
                                _("Specifies a delay after which the panel is shown when the mouse cursor enters the panel."));

    label = gtk_label_new(_("seconds"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    row++;
    col = 2;
    label = gtk_label_new(_("Hidden size"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    panel_autohide_size = gtk_spin_button_new_with_range(1, 500, 1);
    gtk_widget_show(panel_autohide_size);
    gtk_table_attach(GTK_TABLE(table), panel_autohide_size, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(panel_autohide_size,
                                _("Specifies the size of the panel when hidden, in pixels."));

    row++;
    col = 2;
    label = gtk_label_new(_("Hide panel after"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    panel_autohide_hide_time = gtk_spin_button_new_with_range(0, 10000, 0.1);
    gtk_widget_show(panel_autohide_hide_time);
    gtk_table_attach(GTK_TABLE(table), panel_autohide_hide_time, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(panel_autohide_hide_time,
                                _("Specifies a delay after which the panel is hidden when the mouse cursor leaves the panel."));

    label = gtk_label_new(_("seconds"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    change_paragraph(parent);

    label = gtk_label_new(_("<b>Window manager interaction</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(2, 12, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

    row = 0;
    col = 2;
    label = gtk_label_new(_("Forward mouse events"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    panel_wm_menu = gtk_check_button_new();
    gtk_widget_show(panel_wm_menu);
    gtk_table_attach(GTK_TABLE(table), panel_wm_menu, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(panel_wm_menu,
                                _("If enabled, mouse events not handled by panel elements are forwarded to the desktop. "
                                  "Useful on desktop environments that show a start menu when right clicking the desktop, "
                                  "or switch the desktop when rotating the mouse wheel over the desktop."));

    row++;
    col = 2;
    label = gtk_label_new(_("Place panel in dock"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    panel_dock = gtk_check_button_new();
    gtk_widget_show(panel_dock);
    gtk_table_attach(GTK_TABLE(table), panel_dock, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(panel_dock,
                                _("If enabled, places the panel in the dock area of the window manager. "
                                  "Windows placed in the dock are usually treated differently than normal windows. "
                                  "The exact behavior depends on the window manager and its configuration."));

    row++;
    col = 2;
    label = gtk_label_new(_("Pivot reserved space"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    panel_pivot_struts = gtk_check_button_new();
    gtk_widget_show(panel_pivot_struts);
    gtk_table_attach(GTK_TABLE(table), panel_pivot_struts, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(panel_pivot_struts,
                                _("If enabled, the space reserved for the panel (if \"Maximised windows\" is set to "
                                  "match the panel size or hidden panel size) will be given to the window manager as if "
                                  "the panel were the opposite orientation (horizontal vs vertical) than it actually is. "
                                  "On some window managers this allows placing a panel in the middle of the virtual screen, "
                                  "e.g. on the bottom edge of the top screen in a vertical dual-monitor setup."));

    row++;
    col = 2;
    label = gtk_label_new(_("Panel layer"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    panel_combo_layer = gtk_combo_box_text_new();
    gtk_widget_show(panel_combo_layer);
    gtk_table_attach(GTK_TABLE(table), panel_combo_layer, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(panel_combo_layer), _("Top"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(panel_combo_layer), _("Normal"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(panel_combo_layer), _("Bottom"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_layer), 1);
    gtk_widget_set_tooltip_text(panel_combo_layer,
                                _("Specifies the layer on which the panel window should be placed. \n"
                                  "Top means the panel should always cover other windows. \n"
                                  "Bottom means other windows should always cover the panel. \n"
                                  "Normal means that other windows may or may not cover the panel, depending on which has "
                                  "focus. \n"
                                  "Note that some window managers prevent this option from working correctly if the panel is "
                                  "placed in the dock."));

    row++;
    col = 2;
    label = gtk_label_new(_("Maximized windows"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    panel_combo_strut_policy = gtk_combo_box_text_new();
    gtk_widget_show(panel_combo_strut_policy);
    gtk_table_attach(GTK_TABLE(table), panel_combo_strut_policy, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(panel_combo_strut_policy), _("Match the panel size"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(panel_combo_strut_policy), _("Match the hidden panel size"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(panel_combo_strut_policy), _("Fill the screen"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_strut_policy), 0);
    gtk_widget_set_tooltip_text(panel_combo_strut_policy,
                                _("Specifies the size of maximized windows. \n"
                                  "Match the panel size means that maximized windows should extend to the edge of the panel. "
                                  "\n"
                                  "Match the hidden panel size means that maximized windows should extend to the edge of the "
                                  "panel when hidden; "
                                  "when visible, the panel and the windows will overlap. \n"
                                  "Fill the screen means that maximized windows will always have the same size as the screen. "
                                  "\n"
                                  "\n"
                                  "Note: on multi-monitor (Xinerama) setups, the panel must be placed at the edge (not in the "
                                  "middle) "
                                  "of the virtual screen for this to work correctly."));

    row++;
    col = 2;
    label = gtk_label_new(_("Window name"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    panel_window_name = gtk_entry_new();
    gtk_widget_show(panel_window_name);
    gtk_entry_set_width_chars(GTK_ENTRY(panel_window_name), 28);
    gtk_entry_set_text(GTK_ENTRY(panel_window_name), "tint2");
    gtk_table_attach(GTK_TABLE(table), panel_window_name, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(panel_window_name,
                                _("Specifies the name of the panel window. "
                                  "This is useful if you want to configure special treatment of tint2 windows in your "
                                  "window manager or compositor."));

    change_paragraph(parent);
}

void init_panel_items_page (GtkWidget *parent)
{
    GtkWidget *table, *label, *button, *image;

    panel_items = gtk_list_store_new(itemsNumCols, G_TYPE_STRING, G_TYPE_STRING);
    all_items = gtk_list_store_new(itemsNumCols, G_TYPE_STRING, G_TYPE_STRING);

    GtkTreeIter iter;
    gtk_list_store_append(all_items, &iter);
    gtk_list_store_set(all_items, &iter, itemsColName, _("Battery"), itemsColValue, "B", -1);
    gtk_list_store_append(all_items, &iter);
    gtk_list_store_set(all_items, &iter, itemsColName, _("Clock"), itemsColValue, "C", -1);
    gtk_list_store_append(all_items, &iter);
    gtk_list_store_set(all_items, &iter, itemsColName, _("System tray"), itemsColValue, "S", -1);
    gtk_list_store_append(all_items, &iter);
    gtk_list_store_set(all_items, &iter, itemsColName, _("Taskbar"), itemsColValue, "T", -1);
    gtk_list_store_append(all_items, &iter);
    gtk_list_store_set(all_items, &iter, itemsColName, _("Launcher"), itemsColValue, "L", -1);
    gtk_list_store_append(all_items, &iter);
    gtk_list_store_set(all_items, &iter, itemsColName, _("Free space"), itemsColValue, "F", -1);
    gtk_list_store_append(all_items, &iter);
    gtk_list_store_set(all_items, &iter, itemsColName, _("Separator"), itemsColValue, ":", -1);
    gtk_list_store_append(all_items, &iter);
    gtk_list_store_set(all_items, &iter, itemsColName, _("Executor"), itemsColValue, "E", -1);
    gtk_list_store_append(all_items, &iter);
    gtk_list_store_set(all_items, &iter, itemsColName, _("Button"), itemsColValue, "P", -1);

    panel_items_view = gtk_tree_view_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(panel_items_view),
                                                -1,
                                                "",
                                                gtk_cell_renderer_text_new(),
                                                "text",
                                                itemsColName,
                                                NULL);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(panel_items_view), FALSE);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(panel_items_view)), GTK_SELECTION_SINGLE);
    gtk_tree_view_set_model(GTK_TREE_VIEW(panel_items_view), GTK_TREE_MODEL(panel_items));
    g_object_unref(panel_items);
    gtk_widget_set_tooltip_text(panel_items_view,
                                _("Specifies the elements that will appear in the panel and their order. "
                                  "Elements can be added by selecting them in the list of available elements, then clicking "
                                  "on "
                                  "the add left button."));

    all_items_view = gtk_tree_view_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(all_items_view),
                                                -1,
                                                "",
                                                gtk_cell_renderer_text_new(),
                                                "text",
                                                itemsColName,
                                                NULL);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(all_items_view), FALSE);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(all_items_view)), GTK_SELECTION_SINGLE);
    gtk_tree_view_set_model(GTK_TREE_VIEW(all_items_view), GTK_TREE_MODEL(all_items));
    g_object_unref(all_items);
    gtk_widget_set_tooltip_text(all_items_view,
                                _("Lists all the possible elements that can appear in the panel. "
                                  "Elements can be added to the panel by selecting them, then clicking on "
                                  "the add left button."));

    table = gtk_table_new(2, 3, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

    label = gtk_label_new(_("<b>Elements selected</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

    label = gtk_label_new(_("<b>Elements available</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

    GtkWidget *vbox;
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 0);

    button = gtk_button_new();
    image = gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button), image);
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(panel_move_item_up), NULL);
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
    gtk_widget_set_tooltip_text(button, _("Moves up the current element in the list of selected elements."));

    button = gtk_button_new();
    image = gtk_image_new_from_stock(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button), image);
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(panel_move_item_down), NULL);
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
    gtk_widget_set_tooltip_text(button, _("Moves down the current element in the list of selected elements."));

    label = gtk_label_new(_(" "));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    button = gtk_button_new();
    image = gtk_image_new_from_stock(GTK_STOCK_GO_BACK, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button), image);
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(panel_add_item), NULL);
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
    gtk_widget_set_tooltip_text(button,
                                _("Copies the current element in the list of available elements to the list of selected "
                                  "elements."));

    button = gtk_button_new();
    image = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button), image);
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(panel_remove_item), NULL);
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
    gtk_widget_set_tooltip_text(button, _("Removes the current element from the list of selected elements."));

    gtk_table_attach(GTK_TABLE(table), vbox, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

    gtk_widget_show(panel_items_view);
    gtk_table_attach(GTK_TABLE(table),
                     addScrollBarToWidget(panel_items_view),
                     0,
                     1,
                     1,
                     2,
                     GTK_FILL | GTK_EXPAND,
                     GTK_FILL,
                     0,
                     0);

    gtk_widget_show(all_items_view);
    gtk_table_attach(GTK_TABLE(table),
                     addScrollBarToWidget(all_items_view),
                     2,
                     3,
                     1,
                     2,
                     GTK_FILL | GTK_EXPAND,
                     GTK_FILL,
                     0,
                     0);

    change_paragraph(parent);
}

gboolean panel_contains(const char *value)
{
    GtkTreeModel *model = GTK_TREE_MODEL(panel_items);

    GtkTreeIter i;
    if (!gtk_tree_model_get_iter_first(model, &i)) {
        return FALSE;
    }

    while (1) {
        gchar *v;
        gtk_tree_model_get(model, &i, itemsColValue, &v, -1);
        if (g_str_equal(value, v)) {
            return TRUE;
        }

        if (!gtk_tree_model_iter_next(model, &i)) {
            break;
        }
    }

    return FALSE;
}

char *get_panel_items()
{
    size_t buf_size = 256;
    char *result = calloc(buf_size, 1);
    char *p = result;
    GtkTreeModel *model = GTK_TREE_MODEL(panel_items);

    GtkTreeIter i;
    if (!gtk_tree_model_get_iter_first(model, &i)) {
        return FALSE;
    }

    while (1) {
        gchar *v;
        gtk_tree_model_get(model, &i, itemsColValue, &v, -1);
        *p++ = *v;

        if (!gtk_tree_model_iter_next(model, &i)) {
            break;
        }
    }
    *p = '\0';

    return result;
}

void set_panel_items(const char *items)
{
    gtk_list_store_clear(panel_items);

    if (!items)
        return;

    int separator_index = 0;
    int execp_index = 0;
    int button_index = 0;

    STRBUF_AUTO (buffer, 256);
    for (; *items; items++) {
        const char *value;
        const char *name;

        char v = *items;
        switch (v) {
            case 'B':   value = "B";
                        name = _("Battery");
                        break;
            case 'C':   value = "C";
                        name = _("Clock");
                        break;
            case 'S':   value = "S";
                        name = _("System tray");
                        break;
            case 'T':   value = "T";
                        name = _("Taskbar");
                        break;
            case 'L':   value = "L";
                        name = _("Launcher");
                        break;
            case 'F':   value = "F";
                        name = _("Free space");
                        break;
            case ':':   snprintf(buffer, strlen_const(buffer), "%s %d", _("Separator"), ++separator_index);
                        name = buffer;
                        value = ":";
                        break;
            case 'E':   snprintf(buffer, strlen_const(buffer), "%s %d", _("Executor"), ++execp_index);
                        name = buffer;
                        value = "E";
                        break;
            case 'P':   snprintf(buffer, strlen_const(buffer), "%s %d", _("Button"), ++button_index);
                        name = buffer;
                        value = "P";
                        break;
            default:    value = NULL;
                        name = NULL;
                        continue;
        }

        GtkTreeIter iter;
        gtk_list_store_append(panel_items, &iter);
        gtk_list_store_set(panel_items, &iter, itemsColName, name, itemsColValue, value, -1);
    }
}

void panel_add_item(GtkWidget *widget, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;

    if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(all_items_view)), &model, &iter)) {
        gchar *name;
        gchar *value;

        gtk_tree_model_get(model, &iter, itemsColName, &name, itemsColValue, &value, -1);

        if (!panel_contains(value)  || (*value && strchr(":EFP", *value))) {
            GtkTreeIter iter;
            gtk_list_store_append(panel_items, &iter);
            gtk_list_store_set(panel_items, &iter, itemsColName, g_strdup(name), itemsColValue, g_strdup(value), -1);
            switch (*value) {
            case ':':   separator_create_new();
                        separator_update_indices();
                        break;
            case 'E':   execp_create_new();
                        execp_update_indices();
                        break;
            case 'P':   button_create_new();
                        button_update_indices();
                        break;
            }
        }
    }
}

void panel_remove_item(GtkWidget *widget, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;

    if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(panel_items_view)), &model, &iter)) {
        gchar *name;
        gchar *value;

        gtk_tree_model_get(model, &iter, itemsColName, &name, itemsColValue, &value, -1);

        switch (*value) {
        case ':':   for (int i = 0; i < separators->len; i++) {
                        Separator *separator = &g_array_index(separators, Separator, i);
                        if (g_str_equal(name, separator->name)) {
                            separator_remove(i);
                            gtk_list_store_remove(panel_items, &iter);
                            separator_update_indices();
                            break;
                        }
                    }
                    break;
        case 'E':   for (int i = 0; i < executors->len; i++) {
                        Executor *executor = &g_array_index(executors, Executor, i);
                        if (g_str_equal(name, executor->name)) {
                            execp_remove(i);
                            gtk_list_store_remove(panel_items, &iter);
                            execp_update_indices();
                            break;
                        }
                    }
                    break;
        case 'P':   for (int i = 0; i < buttons->len; i++) {
                        Button *button = &g_array_index(buttons, Button, i);
                        if (g_str_equal(name, button->name)) {
                            button_remove(i);
                            gtk_list_store_remove(panel_items, &iter);
                            button_update_indices();
                            break;
                        }
                    }
                    break;
        default:    gtk_list_store_remove(panel_items, &iter);
                    break;
        }
    }
}

void panel_move_item_down(GtkWidget *widget, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;

    if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(panel_items_view)),
                                        &model,
                                        &iter))
    {
        GtkTreeIter next = iter;
        if (gtk_tree_model_iter_next(model, &next)) {
            gchar *name1;
            gchar *value1;
            gtk_tree_model_get(model, &iter, itemsColName, &name1, itemsColValue, &value1, -1);
            gchar *name2;
            gchar *value2;
            gtk_tree_model_get(model, &next, itemsColName, &name2, itemsColValue, &value2, -1);

            if (*value1 && *value2 == *value1)
            switch (*value1) {
            case ':': { Separator *separator1 = NULL;
                        Separator *separator2 = NULL;
                        for (int i = 0; i < separators->len; i++) {
                            Separator *separator = &g_array_index(separators, Separator, i);
                            if (g_str_equal(name1, separator->name)) {
                                separator1 = separator;
                            }
                            if (g_str_equal(name2, separator->name)) {
                                separator2 = separator;
                            }
                        }
                        Separator tmp = *separator1;
                        *separator1 = *separator2;
                        *separator2 = tmp;
                        gtk_list_store_swap(panel_items, &iter, &next);
                        separator_update_indices();
                        break; }
            case 'E': { Executor *executor1 = NULL;
                        Executor *executor2 = NULL;
                        for (int i = 0; i < executors->len; i++) {
                            Executor *executor = &g_array_index(executors, Executor, i);
                            if (g_str_equal(name1, executor->name)) {
                                executor1 = executor;
                            }
                            if (g_str_equal(name2, executor->name)) {
                                executor2 = executor;
                            }
                        }
                        Executor tmp = *executor1;
                        *executor1 = *executor2;
                        *executor2 = tmp;
                        gtk_list_store_swap(panel_items, &iter, &next);
                        execp_update_indices();
                        break; }
            case 'P': { Button *button1 = NULL;
                        Button *button2 = NULL;
                        for (int i = 0; i < buttons->len; i++) {
                            Button *button = &g_array_index(buttons, Button, i);
                            if (g_str_equal(name1, button->name)) {
                                button1 = button;
                            }
                            if (g_str_equal(name2, button->name)) {
                                button2 = button;
                            }
                        }
                        Button tmp = *button1;
                        *button1 = *button2;
                        *button2 = tmp;
                        gtk_list_store_swap(panel_items, &iter, &next);
                        button_update_indices();
                        break; }
            }
            else
                gtk_list_store_swap(panel_items, &iter, &next);
        }
    }
}

void panel_move_item_up(GtkWidget *widget, gpointer data)
{
    {
        GtkTreeIter iter;
        GtkTreeModel *model;

        if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(panel_items_view)),
                                            &model,
                                            &iter))
        {
            GtkTreeIter prev = iter;
            if (gtk_tree_model_iter_prev_tint2(model, &prev)) {
                gchar *name1;
                gchar *value1;
                gtk_tree_model_get(model, &iter, itemsColName, &name1, itemsColValue, &value1, -1);
                gchar *name2;
                gchar *value2;
                gtk_tree_model_get(model, &prev, itemsColName, &name2, itemsColValue, &value2, -1);

                if (*value1 && *value2 == *value1)
                switch (*value1) {
                case ':': { Separator *separator1 = NULL;
                            Separator *separator2 = NULL;
                            for (int i = 0; i < separators->len; i++)
                            {
                                Separator *separator = &g_array_index(separators, Separator, i);
                                if (g_str_equal(name1, separator->name)) {
                                    separator1 = separator;
                                }
                                if (g_str_equal(name2, separator->name)) {
                                    separator2 = separator;
                                }
                            }
                            Separator tmp = *separator1;
                            *separator1 = *separator2;
                            *separator2 = tmp;
                            gtk_list_store_swap (panel_items, &iter, &prev);
                            separator_update_indices();
                            break; }
                case 'E': { Executor *executor1 = NULL;
                            Executor *executor2 = NULL;
                            for (int i = 0; i < executors->len; i++)
                            {
                                Executor *executor = &g_array_index(executors, Executor, i);
                                if (g_str_equal(name1, executor->name)) {
                                    executor1 = executor;
                                }
                                if (g_str_equal(name2, executor->name)) {
                                    executor2 = executor;
                                }
                            }
                            Executor tmp = *executor1;
                            *executor1 = *executor2;
                            *executor2 = tmp;
                            gtk_list_store_swap (panel_items, &iter, &prev);
                            execp_update_indices();
                            break; }
                case 'P': { Button *button1 = NULL;
                            Button *button2 = NULL;
                            for (int i = 0; i < buttons->len; i++)
                            {
                                Button *button = &g_array_index(buttons, Button, i);
                                if (g_str_equal(name1, button->name)) {
                                    button1 = button;
                                }
                                if (g_str_equal(name2, button->name)) {
                                    button2 = button;
                                }
                            }
                            Button tmp = *button1;
                            *button1 = *button2;
                            *button2 = tmp;
                            gtk_list_store_swap (panel_items, &iter, &prev);
                            button_update_indices();
                            break; }
                }
                else
                    gtk_list_store_swap (panel_items, &iter, &prev);
            }
        }
    }
}

enum { iconsColName = 0, iconsColDescr, iconsNumCols };
GtkListStore *icon_themes;

void launcher_add_app(GtkWidget *widget, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;

    if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(all_apps_view)), &model, &iter)) {
        GdkPixbuf *pixbuf;
        gchar *name;
        gchar *path;
        gchar *iconName;

        gtk_tree_model_get(model, &iter,
                           appsColIcon,     &pixbuf,
                           appsColText,     &name,
                           appsColPath,     &path,
                           appsColIconName, &iconName,
                           -1);

        GtkTreeIter iter;
        gtk_list_store_append(launcher_apps, &iter);
        gtk_list_store_set(launcher_apps,
                           &iter,
                           appsColIcon,     pixbuf,
                           appsColIconName, g_strdup(iconName),
                           appsColText,     g_strdup(name),
                           appsColPath,     g_strdup(path),
                           -1);
        if (pixbuf)
            g_object_unref(pixbuf);
    }
}

void launcher_remove_app(GtkWidget *widget, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;

    if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(launcher_apps_view)), &model, &iter)) {
        gtk_list_store_remove(launcher_apps, &iter);
    }
}

void launcher_move_app_down(GtkWidget *widget, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;

    if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(launcher_apps_view)), &model, &iter)) {
        GtkTreeIter next = iter;
        if (gtk_tree_model_iter_next(model, &next)) {
            gtk_list_store_swap(launcher_apps, &iter, &next);
        }
    }
}

void launcher_move_app_up(GtkWidget *widget, gpointer data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;

    if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(launcher_apps_view)), &model, &iter)) {
        GtkTreeIter prev = iter;
        if (gtk_tree_model_iter_prev_tint2(model, &prev)) {
            gtk_list_store_swap(launcher_apps, &iter, &prev);
        }
    }
}

gboolean gtk_tree_model_iter_prev_tint2(GtkTreeModel *model, GtkTreeIter *iter)
{
    GtkTreeIter i;
    if (!gtk_tree_model_get_iter_first(model, &i)) {
        return FALSE;
    }
    GtkTreePath *piter = gtk_tree_model_get_path(model, iter);
    if (!piter)
        return FALSE;

    while (1) {
        GtkTreeIter next = i;
        if (gtk_tree_model_iter_next(model, &next)) {
            GtkTreePath *pn = gtk_tree_model_get_path(model, &next);
            if (!pn)
                continue;
            if (gtk_tree_path_compare(piter, pn) == 0) {
                gtk_tree_path_free(piter);
                gtk_tree_path_free(pn);
                *iter = i;
                return TRUE;
            }
            gtk_tree_path_free(pn);
            i = next;
        } else {
            break;
        }
    }
    gtk_tree_path_free(piter);
    return FALSE;
}

// Note: the returned pointer must be released with g_free!
gchar *get_current_icon_theme()
{
    int index = gtk_combo_box_get_active(GTK_COMBO_BOX(launcher_icon_theme));
    if (index <= 0) {
        return NULL;
    }

    GtkTreePath *path;
    GtkTreeIter iter;

    path = gtk_tree_path_new_from_indices(index, -1);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(icon_themes), &iter, path);
    gtk_tree_path_free(path);

    gchar *name;
    gtk_tree_model_get(GTK_TREE_MODEL(icon_themes), &iter, iconsColName, &name, -1);
    return name;
}

void set_current_icon_theme(const char *theme)
{
    int i;
    for (i = 0;; i++) {
        GtkTreePath *path;
        GtkTreeIter iter;

        path = gtk_tree_path_new_from_indices(i, -1);
        gboolean end = gtk_tree_model_get_iter(GTK_TREE_MODEL(icon_themes), &iter, path);
        gtk_tree_path_free(path);

        if (!end) {
            break;
        }

        gchar *name;
        gtk_tree_model_get(GTK_TREE_MODEL(icon_themes), &iter, iconsColName, &name, -1);
        if (g_str_equal(name, theme)) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(launcher_icon_theme), i);
            g_free(name);
            break;
        }
        g_free(name);
    }
}

void icon_theme_changed(gpointer data)
{
    create_please_wait(GTK_WINDOW(data));
    process_events();

    if (icon_theme)
        free_themes(icon_theme);

    gchar *icon_theme_name = get_current_icon_theme();
    if (!icon_theme_name || g_str_equal(icon_theme_name, "")) {
        g_free(icon_theme_name);
        icon_theme_name = get_default_theme_name();
    }
    icon_theme = load_themes(icon_theme_name);
    g_free(icon_theme_name);

    load_icons(launcher_apps);
    load_icons(all_apps);
    save_icon_cache(icon_theme);

    destroy_please_wait();
}

void launcher_icon_theme_changed(GtkWidget *widget, gpointer data)
{
    icon_theme_changed(data);
}

GdkPixbuf *load_icon(const gchar *name)
{
    process_events();

    int size = 22;
    char *path = get_icon_path(icon_theme, name, size, FALSE);
    GdkPixbuf *pixbuf = path ? gdk_pixbuf_new_from_file_at_size(path, size, size, NULL) : NULL;
    free(path);
    return pixbuf;
}

void load_icons(GtkListStore *apps)
{
    int i;
    for (i = 0;; i++) {
        GtkTreePath *path;
        GtkTreeIter iter;

        path = gtk_tree_path_new_from_indices(i, -1);
        gboolean found = gtk_tree_model_get_iter(GTK_TREE_MODEL(apps), &iter, path);
        gtk_tree_path_free(path);

        if (!found)
            break;

        gchar *iconName;
        gtk_tree_model_get(GTK_TREE_MODEL(apps), &iter, appsColIconName, &iconName, -1);
        GdkPixbuf *pixbuf = load_icon(iconName);
        gtk_list_store_set(apps, &iter, appsColIcon, pixbuf, -1);
        if (pixbuf)
            g_object_unref(pixbuf);
        g_free(iconName);
    }
}

void load_desktop_file(const char *file, gboolean selected)
{
    char *file_contracted = contract_tilde(file);

    GtkListStore *store = selected ? launcher_apps : all_apps;
    gboolean duplicate = FALSE;
    for (int index = 0;; index++) {
        GtkTreePath *path = gtk_tree_path_new_from_indices(index, -1);
        GtkTreeIter iter;
        gboolean found = gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path);
        gtk_tree_path_free(path);
        if (!found)
            break;

        gchar *app_path;
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, appsColPath, &app_path, -1);
        char *contracted = contract_tilde(app_path);
        if (strcmp(contracted, file_contracted) == 0) {
            duplicate = TRUE;
            break;
        }
        free(contracted);
        g_free(app_path);
    }

    if (!duplicate) {
        DesktopEntry entry;
        if (read_desktop_file(file, &entry)) {
            int index;
            gboolean stop = FALSE;
            for (index = 0; !stop || selected; index++) {
                GtkTreePath *path = gtk_tree_path_new_from_indices(index, -1);
                GtkTreeIter iter;
                gboolean found = gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path);
                gtk_tree_path_free(path);
                if (!found)
                    break;

                gchar *app_name;
                gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, appsColText, &app_name, -1);
                if (strnatcasecmp(app_name, entry.name) >= 0)
                    stop = TRUE;
                g_free(app_name);
            }

            GdkPixbuf *pixbuf = load_icon(entry.icon);
            GtkTreeIter iter;
            gtk_list_store_insert(store, &iter, index);
            gtk_list_store_set(store,
                               &iter,
                               appsColIcon,     pixbuf,
                               appsColIconName, g_strdup(entry.icon),
                               appsColText,     g_strdup(entry.name),
                               appsColPath,     g_strdup(file),
                               -1);
            if (pixbuf)
                g_object_unref(pixbuf);
        } else {
            fprintf(stderr, "tint2: Could not load %s\n", file);
            GdkPixbuf *pixbuf = load_icon(DEFAULT_ICON);
            GtkTreeIter iter;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store,
                               &iter,
                               appsColIcon,     pixbuf,
                               appsColIconName, g_strdup(""),
                               appsColText,     g_strdup(file),
                               appsColPath,     g_strdup(file),
                               -1);
            if (pixbuf)
                g_object_unref(pixbuf);
        }
        free_desktop_entry(&entry);
    }

    free(file_contracted);
}

void populate_from_entries(GList *entries, gboolean selected)
{
    for (GList *l = entries; l; l = l->next) {
        DesktopEntry *entry = (DesktopEntry *)l->data;
        GdkPixbuf *pixbuf = load_icon(entry->icon);
        GtkTreeIter iter;
        gtk_list_store_append(selected ? launcher_apps : all_apps, &iter);
        gtk_list_store_set(selected ? launcher_apps : all_apps,
                           &iter,
                           appsColIcon,     pixbuf,
                           appsColIconName, g_strdup(entry->icon),
                           appsColText,     g_strdup(entry->name),
                           appsColPath,     g_strdup(entry->path),
                           -1);
        if (pixbuf)
            g_object_unref(pixbuf);
    }
}

static gint compare_entries(gconstpointer a, gconstpointer b)
{
    return strnatcasecmp(((DesktopEntry *)a)->name, ((DesktopEntry *)b)->name);
}

void load_desktop_entry(const char *file, GList **entries)
{
    process_events();

    DesktopEntry *entry = calloc(1, sizeof(DesktopEntry));
    if (!read_desktop_file(file, entry))
        fprintf(stderr, "tint2: Could not load %s\n", file);
    if (entry->hidden_from_menus) {
        free(entry);
        return;
    }
    if (!entry->name)
        entry->name = strdup(file);
    if (!entry->icon)
        strdup_static(entry->icon, "");

    // Don't worry about order, it has to be sorter later anyway
    *entries = g_list_prepend(*entries, entry);
}

void load_desktop_entries(const char *path, GList **entries)
{
    process_events();

    GList *subdirs = NULL;
    GList *files = NULL;

    GDir *d = g_dir_open(path, 0, NULL);
    if (d) {
        const gchar *name;
        while ((name = g_dir_read_name(d))) {
            gchar *file = g_build_filename(path, name, NULL);
            if (!g_file_test(file, G_FILE_TEST_IS_DIR) && g_str_has_suffix(file, ".desktop")) {
                files = g_list_prepend(files, file);
            } else if (g_file_test(file, G_FILE_TEST_IS_DIR)) {
                subdirs = g_list_prepend(subdirs, file);
            } else {
                g_free(file);
            }
        }
        g_dir_close(d);
    }

    subdirs = g_list_sort(subdirs, compare_strings);
    GList *l;
    for (l = subdirs; l; l = l->next) {
        gchar *dir = (gchar *)l->data;
        load_desktop_entries(dir, entries);
        g_free(dir);
    }
    g_list_free(subdirs);

    files = g_list_sort(files, compare_strings);
    for (l = files; l; l = l->next) {
        gchar *file = (gchar *)l->data;
        load_desktop_entry(file, entries);
        g_free(file);
    }
    g_list_free(files);
}

static gint compare_themes(gconstpointer a, gconstpointer b)
{
    gint result = strnatcasecmp(((IconTheme *)a)->description, ((IconTheme *)b)->description);
    if (result)
        return result;
    return strnatcasecmp(((IconTheme *)a)->name, ((IconTheme *)b)->name);
}

void load_theme_file(const char *file_name, const char *theme_name, GList **themes)
{
    process_events();

    if (!file_name || !theme_name) {
        return;
    }

    FILE *f;
    if ((f = fopen(file_name, "rt")) == NULL) {
        return;
    }

    char *line = NULL;
    size_t line_size;

    ssize_t line_len;
    while ((line_len = getline(&line, &line_size, f)) >= 0) {
        if (line[0] == '\n' || line[0] == '\0')
            continue;

        char *key, *value;

        str_strip_newline (line, line_len);

        if (parse_theme_line(line, &key, &value) &&
            str_lequal_static (key, "Name", value - key - 1))
        {
            IconTheme *theme = calloc(1, sizeof(IconTheme));
            theme->name = strdup(theme_name);
            theme->description = strdup(value);
            *themes = g_list_prepend(*themes, theme);
            break;
        }

        if (line[0] == '[' && line[line_len - 1] == ']' &&
            !str_lequal_static (line, "[Icon Theme]", line_len)
        ) break;
    }
    fclose(f);
    free(line);
}

void load_icon_themes(const gchar *path, const gchar *parent, GList **themes)
{
    process_events();

    GDir *d = g_dir_open(path, 0, NULL);
    if (!d)
        return;
    int path_len = strlen(path);
    int name_cap = 0;
    const gchar *name;

    char *fpath_buf = NULL;
    int fpath_cap = 0;

    while ((name = g_dir_read_name(d)))
    {
        // Initial path buffer setup (copy path argument)
        int name_len = strlen(name);
        if (name_len > name_cap)
        {
            char *buf;
            name_cap = name_len;
            fpath_cap = path_len + 1 + name_len + 1 + sizeof("index.theme");
            buf = realloc(fpath_buf, fpath_cap);
            if (!fpath_buf) {
                memcpy(buf, path, path_len);
                buf[path_len] = G_DIR_SEPARATOR;
            }
            fpath_buf = buf;
        }
        // Must be directory and not symbolic link for theme in same directory
        char *p = fpath_buf + path_len + 1;
        memcpy(p, name, name_len + 1);
        if (!g_file_test(fpath_buf, G_FILE_TEST_IS_DIR))
            continue;
        if (g_file_test(fpath_buf, G_FILE_TEST_IS_SYMLINK)) {
#ifdef PATH_MAX
            char real_path[PATH_MAX];
#else
            char real_path[65536];
#endif
            if (realpath(fpath_buf, real_path) &&
                strstr(real_path, path) == real_path &&
                !strchr(real_path + path_len + 1, G_DIR_SEPARATOR))
                continue;
        }
        // Must contain index.theme
        p += name_len;
        *p++ = G_DIR_SEPARATOR;
        memcpy(p, "index.theme", sizeof("index.theme"));
        if (!g_file_test(fpath_buf, G_FILE_TEST_IS_REGULAR))
            continue;

        load_theme_file(fpath_buf, name, themes);
    }
    free(fpath_buf);
    g_dir_close(d);
}

gchar *get_default_theme_name()
{
    gchar *name = NULL;
    g_object_get(gtk_settings_get_default(), "gtk-icon-theme-name", &name, NULL);
    if (!name) {
        strdup_static(name, "hicolor");
    }
    return name;
}

GtkWidget *addScrollBarToWidget(GtkWidget *widget)
{
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), widget);

    gtk_widget_set_size_request(scrolled_window, 100, 300);

    gtk_widget_show(widget);
    gtk_widget_show(scrolled_window);

    return scrolled_window;
}

void init_launcher_page (GtkWidget *parent, GtkWindow *window)
{
    GtkWidget *image;

    icon_theme = NULL;

    launcher_apps = gtk_list_store_new(appsNumCols, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    all_apps = gtk_list_store_new(appsNumCols, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    icon_themes = gtk_list_store_new(iconsNumCols, G_TYPE_STRING, G_TYPE_STRING);

    launcher_apps_view = gtk_tree_view_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(launcher_apps_view),
                                                -1,
                                                "",
                                                gtk_cell_renderer_pixbuf_new(),
                                                "pixbuf",
                                                appsColIcon,
                                                NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(launcher_apps_view),
                                                -1,
                                                "",
                                                gtk_cell_renderer_text_new(),
                                                "text",
                                                appsColText,
                                                NULL);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(launcher_apps_view), FALSE);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(launcher_apps_view)), GTK_SELECTION_SINGLE);
    gtk_tree_view_set_model(GTK_TREE_VIEW(launcher_apps_view), GTK_TREE_MODEL(launcher_apps));
    g_object_unref(launcher_apps);
    gtk_widget_set_tooltip_text(launcher_apps_view,
                                _("Specifies the application launchers that will appear in the launcher and their order. "
                                  "Launchers can be added by selecting an item in the list of available applications, then "
                                  "clicking on "
                                  "the add left button."));

    all_apps_view = gtk_tree_view_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(all_apps_view),
                                                -1,
                                                "",
                                                gtk_cell_renderer_pixbuf_new(),
                                                "pixbuf",
                                                appsColIcon,
                                                NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(all_apps_view),
                                                -1,
                                                "",
                                                gtk_cell_renderer_text_new(),
                                                "text",
                                                appsColText,
                                                NULL);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(all_apps_view), FALSE);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(all_apps_view)), GTK_SELECTION_SINGLE);
    gtk_tree_view_set_model(GTK_TREE_VIEW(all_apps_view), GTK_TREE_MODEL(all_apps));
    g_object_unref(all_apps);
    gtk_widget_set_tooltip_text(all_apps_view,
                                _("Lists all the applications detected on the system. "
                                  "Launchers can be added to the launcher by selecting an application, then clicking on "
                                  "the add left button."));

    GtkWidget *table, *label, *button;
    int row, col;

    table = gtk_table_new(2, 3, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

    label = gtk_label_new(_("<b>Applications selected</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

    label = gtk_label_new(_("<b>Applications available</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);

    GtkWidget *vbox;
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(vbox);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 0);

    button = gtk_button_new();
    image = gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button), image);
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(launcher_move_app_up), NULL);
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
    gtk_widget_set_tooltip_text(button,
                                _("Moves up the current launcher in the list of selected applications."));

    button = gtk_button_new();
    image = gtk_image_new_from_stock(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button), image);
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(launcher_move_app_down), NULL);
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
    gtk_widget_set_tooltip_text(button,
                                _("Moves down the current launcher in the list of selected applications."));

    label = gtk_label_new(_(" "));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

    button = gtk_button_new();
    image = gtk_image_new_from_stock(GTK_STOCK_GO_BACK, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button), image);
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(launcher_add_app), NULL);
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
    gtk_widget_set_tooltip_text(button,
                                _("Copies the current application in the list of available applications to the list of "
                                  "selected applications."));

    button = gtk_button_new();
    image = gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(button), image);
    g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(launcher_remove_app), NULL);
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
    gtk_widget_set_tooltip_text(button,
                                _("Removes the current application from the list of selected application."));

    gtk_table_attach(GTK_TABLE(table), vbox, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

    gtk_widget_show(launcher_apps_view);
    gtk_table_attach(GTK_TABLE(table),
                     addScrollBarToWidget(launcher_apps_view),
                     0,
                     1,
                     1,
                     2,
                     GTK_FILL | GTK_EXPAND,
                     GTK_FILL,
                     0,
                     0);

    gtk_widget_show(all_apps_view);
    gtk_table_attach(GTK_TABLE(table),
                     addScrollBarToWidget(all_apps_view),
                     2,
                     3,
                     1,
                     2,
                     GTK_FILL | GTK_EXPAND,
                     GTK_FILL,
                     0,
                     0);

    change_paragraph(parent);

    label = gtk_label_new(_("<b>Additional application directories</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    launcher_apps_dirs = gtk_entry_new();
    gtk_widget_show(launcher_apps_dirs);
    gtk_box_pack_start(GTK_BOX(parent), launcher_apps_dirs, FALSE, FALSE, 0);
    gtk_widget_set_tooltip_text(launcher_apps_dirs,
                                _("Specifies a path to a directory from which the launcher is loading all .desktop files (all "
                                  "subdirectories are explored recursively). "
                                  "Can be used multiple times, in which case the paths must be separated by commas. Leading ~ "
                                  "is expaned to the path of the user's home directory."));

    label = gtk_label_new(_("<b>Appearance</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(7, 10, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

    row = 0, col = 2;
    label = gtk_label_new(_("Background"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    launcher_background = create_background_combo(_("Launcher"));
    gtk_widget_show(launcher_background);
    gtk_table_attach(GTK_TABLE(table), launcher_background, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(launcher_background,
                                _("Selects the background used to display the launcher. "
                                  "Backgrounds can be edited in the Backgrounds tab."));

    row++, col = 2;
    label = gtk_label_new(_("Icon background"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    launcher_icon_background = create_background_combo(_("Launcher icon"));
    gtk_widget_show(launcher_icon_background);
    gtk_table_attach(GTK_TABLE(table), launcher_icon_background, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(launcher_icon_background,
                                _("Selects the background used to display the launcher icon. "
                                  "Backgrounds can be edited in the Backgrounds tab."));

    row++, col = 2;
    label = gtk_label_new(_("Horizontal padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    launcher_padding_x = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(launcher_padding_x);
    gtk_table_attach(GTK_TABLE(table), launcher_padding_x, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(launcher_padding_x,
                                _("Specifies the horizontal padding of the launcher. "
                                  "This is the space between the border and the elements inside."));

    row++, col = 2;
    label = gtk_label_new(_("Vertical padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    launcher_padding_y = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(launcher_padding_y);
    gtk_table_attach(GTK_TABLE(table), launcher_padding_y, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(launcher_padding_y,
                                _("Specifies the vertical padding of the launcher. "
                                  "This is the space between the border and the elements inside."));

    row++, col = 2;
    label = gtk_label_new(_("Spacing"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    launcher_spacing = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(launcher_spacing);
    gtk_table_attach(GTK_TABLE(table), launcher_spacing, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(launcher_spacing,
                                _("Specifies the spacing between the elements inside the launcher."));

    row++, col = 2;
    label = gtk_label_new(_("Icon size"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    launcher_icon_size = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(launcher_icon_size);
    gtk_table_attach(GTK_TABLE(table), launcher_icon_size, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(launcher_icon_size, _("Specifies the size of the launcher icons, in pixels."));

    row++;
    col = 2;
    label = gtk_label_new(_("Icon opacity"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    launcher_icon_opacity = gtk_spin_button_new_with_range(0, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(launcher_icon_opacity), 100);
    gtk_widget_show(launcher_icon_opacity);
    gtk_table_attach(GTK_TABLE(table), launcher_icon_opacity, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(launcher_icon_opacity,
                                _("Specifies the opacity of the launcher icons, in percent."));

    row++;
    col = 2;
    label = gtk_label_new(_("Icon saturation"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    launcher_icon_saturation = gtk_spin_button_new_with_range(-100, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(launcher_icon_saturation), 0);
    gtk_widget_show(launcher_icon_saturation);
    gtk_table_attach(GTK_TABLE(table), launcher_icon_saturation, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(launcher_icon_saturation,
                                _("Specifies the saturation adjustment of the launcher icons, in percent."));

    row++;
    col = 2;
    label = gtk_label_new(_("Icon brightness"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    launcher_icon_brightness = gtk_spin_button_new_with_range(-100, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(launcher_icon_brightness), 0);
    gtk_widget_show(launcher_icon_brightness);
    gtk_table_attach(GTK_TABLE(table), launcher_icon_brightness, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(launcher_icon_brightness,
                                _("Specifies the brightness adjustment of the launcher icons, in percent."));

    row++, col = 2;
    label = gtk_label_new(_("Icon theme"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 3, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    launcher_icon_theme = gtk_combo_box_new_with_model(GTK_TREE_MODEL(icon_themes));
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(launcher_icon_theme), renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(launcher_icon_theme), renderer, "text", iconsColDescr, NULL);
    g_signal_connect(G_OBJECT(launcher_icon_theme), "changed", G_CALLBACK(launcher_icon_theme_changed), window);
    gtk_widget_show(launcher_icon_theme);
    gtk_table_attach(GTK_TABLE(table), launcher_icon_theme, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(launcher_icon_theme,
                                _("The icon theme used to display launcher icons. If left blank, "
                                  "tint2 will detect and use the icon theme of your desktop as long as you have "
                                  "an XSETTINGS manager running (most desktop environments do)."));

    launcher_icon_theme_override = gtk_check_button_new_with_label(_("Overrides XSETTINGS"));
    gtk_widget_show(launcher_icon_theme_override);
    gtk_table_attach(GTK_TABLE(table), launcher_icon_theme_override, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(launcher_icon_theme_override,
                                _("If enabled, the icon theme selected here will override the one provided by XSETTINGS."));

    row++, col = 2;
    label = gtk_label_new(_("Startup notifications"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    startup_notifications = gtk_check_button_new();
    gtk_widget_show(startup_notifications);
    gtk_table_attach(GTK_TABLE(table), startup_notifications, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(startup_notifications,
                                _("If enabled, startup notifications are shown when starting applications from the launcher. "
                                  "The appearance may vary depending on your desktop environment configuration; normally, a "
                                  "busy mouse cursor is displayed until the application starts."));

    row++, col = 2;
    label = gtk_label_new(_("Tooltips"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    launcher_tooltip = gtk_check_button_new();
    gtk_widget_show(launcher_tooltip);
    gtk_table_attach(GTK_TABLE(table), launcher_tooltip, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(launcher_tooltip,
                                _("If enabled, shows a tooltip with the application name when the mouse is moved over an "
                                  "application launcher."));

    change_paragraph(parent);

    fprintf(stderr, "tint2: Loading icon themes\n");
    GList *themes = NULL;
    const GSList *location;
    for (location = get_icon_locations(); location; location = location->next) {
        const gchar *path = (gchar *)location->data;
        load_icon_themes(path, NULL, &themes);
    }
    if (themes) {
        themes = g_list_sort(themes, compare_themes);
        GList *p = themes, *n;
        while ((n = p->next))
        {
            IconTheme *tp, *tn;
            char *sp, *sn;
            sp = (tp = p->data)->name;
            sn = (tn = n->data)->name;
            if (strcmp(sp, sn) != 0) {
                p = n;
                continue;
            }
            themes = g_list_delete_link(themes, n);
            free_icon_theme(tn);
            free(tn);
        }
    }

    GtkTreeIter iter;
    gtk_list_store_append(icon_themes, &iter);
    gtk_list_store_set(icon_themes, &iter, 0, "", -1);
    for (GList *l = themes; l; l = l->next) {
        IconTheme *theme = (IconTheme *)l->data;
        GtkTreeIter iter;
        gtk_list_store_append(icon_themes, &iter);
        gtk_list_store_set(icon_themes, &iter,
                           iconsColName,    g_strdup(theme->name),
                           iconsColDescr,   g_strdup(theme->description),
                           -1);
    }

    for (GList *l = themes; l; l = l->next) {
        free_icon_theme((IconTheme *)l->data);
    }
    g_list_free(themes);
    fprintf(stderr, "tint2: Icon themes loaded\n");

    fprintf(stderr, "tint2: Loading .desktop files\n");
    GList *entries = NULL;
    for (location = get_apps_locations(); location; location = location->next) {
        const gchar *path = (gchar *)location->data;
        load_desktop_entries(path, &entries);
    }
    entries = g_list_sort(entries, compare_entries);
    populate_from_entries(entries, FALSE);

    for (GList *l = entries; l; l = l->next) {
        free_desktop_entry((DesktopEntry *)l->data);
    }
    g_list_free(entries);

    icon_theme_changed(window);
    load_icons(launcher_apps);
    load_icons(all_apps);
    fprintf(stderr, "tint2: Desktop files loaded\n");
}

void init_taskbar_page (GtkWidget *parent)
{
    GtkWidget *table, *label;
    int row, col;

    label = gtk_label_new(_("<b>Options</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(4, 2, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = col = 0;

    col = 2;
    row++;
    label = gtk_label_new(_("Show a taskbar for each desktop"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    taskbar_show_desktop = gtk_check_button_new();
    gtk_widget_show(taskbar_show_desktop);
    gtk_table_attach(GTK_TABLE(table), taskbar_show_desktop, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(taskbar_show_desktop,
                                _("If enabled, the taskbar is split into multiple smaller taskbars, one for each virtual "
                                  "desktop."));

    col = 2;
    row++;
    label = gtk_label_new(_("Hide taskbars for empty desktops"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    taskbar_hide_empty = gtk_check_button_new();
    gtk_widget_show(taskbar_hide_empty);
    gtk_table_attach(GTK_TABLE(table), taskbar_hide_empty, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    col = 2;
    row++;
    label = gtk_label_new(_("Distribute size between taskbars"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    taskbar_distribute_size = gtk_check_button_new();
    gtk_widget_show(taskbar_distribute_size);
    gtk_table_attach(GTK_TABLE(table), taskbar_distribute_size, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(taskbar_distribute_size,
                                _("If enabled and 'Show a taskbar for each desktop' is also enabled, "
                                  "the available size is distributed between taskbars proportionally to the number of tasks."));

    col = 2;
    row++;
    label = gtk_label_new(_("Hide inactive tasks"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    taskbar_hide_inactive_tasks = gtk_check_button_new();
    gtk_widget_show(taskbar_hide_inactive_tasks);
    gtk_table_attach(GTK_TABLE(table), taskbar_hide_inactive_tasks, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(taskbar_hide_inactive_tasks,
                                _("If enabled, only the active task will be shown in the taskbar."));

    col = 2;
    row++;
    label = gtk_label_new(_("Hide tasks from different monitors"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    taskbar_hide_diff_monitor = gtk_check_button_new();
    gtk_widget_show(taskbar_hide_diff_monitor);
    gtk_table_attach(GTK_TABLE(table), taskbar_hide_diff_monitor, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(taskbar_hide_diff_monitor,
                                _("If enabled, tasks that are not on the same monitor as the panel will not be displayed. "
                                  "This behavior is enabled automatically if the panel monitor is set to 'All'."));

    col = 2;
    row++;
    label = gtk_label_new(_("Hide tasks from different desktops"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    taskbar_hide_diff_desktop = gtk_check_button_new();
    gtk_widget_show(taskbar_hide_diff_desktop);
    gtk_table_attach(GTK_TABLE(table), taskbar_hide_diff_desktop, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    col = 2;
    row++;
    label = gtk_label_new(_("Always show all desktop tasks"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    taskbar_always_show_all_desktop_tasks = gtk_check_button_new();
    gtk_widget_show(taskbar_always_show_all_desktop_tasks);
    gtk_table_attach(GTK_TABLE(table),
                     taskbar_always_show_all_desktop_tasks,
                     col,
                     col + 1,
                     row,
                     row + 1,
                     GTK_FILL,
                     0,
                     0,
                     0);
    col++;
    gtk_widget_set_tooltip_text(taskbar_always_show_all_desktop_tasks,
                                _("Has effect only if 'Show a taskbar for each desktop' is enabled. "
                                  "If enabled, tasks that appear on all desktops are shown on all taskbars. "
                                  "Otherwise, they are shown only on the taskbar of the current desktop."));

    row++;
    col = 2;
    label = gtk_label_new(_("Task sorting"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    taskbar_sort_order = gtk_combo_box_text_new();
    gtk_widget_show(taskbar_sort_order);
    gtk_table_attach(GTK_TABLE(table), taskbar_sort_order, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(taskbar_sort_order), _("None"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(taskbar_sort_order), _("By title"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(taskbar_sort_order), _("By application"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(taskbar_sort_order), _("By center"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(taskbar_sort_order), _("Most recently used first"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(taskbar_sort_order), _("Most recently used last"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_sort_order), 0);
    gtk_widget_set_tooltip_text(taskbar_sort_order,
                                _("Specifies how tasks should be sorted on the taskbar. \n"
                                  "'None' means that new tasks are added to the end, and the user can also reorder task "
                                  "buttons by mouse dragging. \n"
                                  "'By title' means that tasks are sorted by their window titles. \n"
                                  "'By application' means that tasks are sorted by their application names. \n"
                                  "'By center' means that tasks are sorted geometrically by their window centers."));

    row++;
    col = 2;
    label = gtk_label_new(_("Task alignment"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    taskbar_alignment = gtk_combo_box_text_new();
    gtk_widget_show(taskbar_alignment);
    gtk_table_attach(GTK_TABLE(table), taskbar_alignment, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(taskbar_alignment), _("Left"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(taskbar_alignment), _("Center"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(taskbar_alignment), _("Right"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_alignment), 0);
    gtk_widget_set_tooltip_text(taskbar_alignment,
                                _("Specifies how tasks should be positioned on the taskbar."));

    change_paragraph(parent);

    label = gtk_label_new(_("<b>Appearance</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(3, 12, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = col = 0;

    col = 2;
    label = gtk_label_new(_("Horizontal padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    taskbar_padding_x = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(taskbar_padding_x);
    gtk_table_attach(GTK_TABLE(table), taskbar_padding_x, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(taskbar_padding_x,
                                _("Specifies the horizontal padding of the taskbar. "
                                  "This is the space between the border and the elements inside."));

    col = 2;
    row++;
    label = gtk_label_new(_("Vertical padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    taskbar_padding_y = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(taskbar_padding_y);
    gtk_table_attach(GTK_TABLE(table), taskbar_padding_y, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(taskbar_padding_y,
                                _("Specifies the vertical padding of the taskbar. "
                                  "This is the space between the border and the elements inside."));

    col = 2;
    row++;
    label = gtk_label_new(_("Spacing"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    taskbar_spacing = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(taskbar_spacing);
    gtk_table_attach(GTK_TABLE(table), taskbar_spacing, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(taskbar_spacing,
                                _("Specifies the spacing between the elements inside the taskbar."));

    col = 2;
    row++;
    label = gtk_label_new(_("Active background"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    taskbar_active_background = create_background_combo(_("Active taskbar"));
    gtk_widget_show(taskbar_active_background);
    gtk_table_attach(GTK_TABLE(table), taskbar_active_background, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(taskbar_active_background,
                                _("Selects the background used to display the taskbar of the current desktop. "
                                  "Backgrounds can be edited in the Backgrounds tab."));
    col = 2;
    row++;
    label = gtk_label_new(_("Inactive background"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    taskbar_inactive_background = create_background_combo(_("Inactive taskbar"));
    gtk_widget_show(taskbar_inactive_background);
    gtk_table_attach(GTK_TABLE(table), taskbar_inactive_background, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(taskbar_inactive_background,
                                _("Selects the background used to display taskbars of inactive desktops. "
                                  "Backgrounds can be edited in the Backgrounds tab."));

    change_paragraph(parent);

    label = gtk_label_new(_("<b>Desktop name</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(6, 22, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = col = 0;

    col = 2;
    row++;
    label = gtk_label_new(_("Show desktop name"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    taskbar_show_name = gtk_check_button_new();
    gtk_widget_show(taskbar_show_name);
    gtk_table_attach(GTK_TABLE(table), taskbar_show_name, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(taskbar_show_name,
                                _("If enabled, displays the name of the desktop at the top/left of the taskbar. "
                                  "The name is set by your window manager; you might be able to configure it there."));

    col = 2;
    row++;
    label = gtk_label_new(_("Horizontal padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    taskbar_name_padding_x = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(taskbar_name_padding_x);
    gtk_table_attach(GTK_TABLE(table), taskbar_name_padding_x, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(taskbar_name_padding_x,
                                _("Specifies the horizontal padding of the desktop name. "
                                  "This is the space between the border and the text inside."));

    col = 2;
    row++;
    label = gtk_label_new(_("Vertical padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    taskbar_name_padding_y = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(taskbar_name_padding_y);
    gtk_table_attach(GTK_TABLE(table), taskbar_name_padding_y, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(taskbar_name_padding_y,
                                _("Specifies the vertical padding of the desktop name. "
                                  "This is the space between the border and the text inside."));

    col = 2;
    row++;
    label = gtk_label_new(_("Active font color"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    taskbar_name_active_color = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(taskbar_name_active_color), TRUE);
    gtk_widget_show(taskbar_name_active_color);
    gtk_table_attach(GTK_TABLE(table), taskbar_name_active_color, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(taskbar_name_active_color,
                                _("Specifies the font color used to display the name of the current desktop."));

    col = 2;
    row++;
    label = gtk_label_new(_("Inactive font color"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    taskbar_name_inactive_color = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(taskbar_name_inactive_color), TRUE);
    gtk_widget_show(taskbar_name_inactive_color);
    gtk_table_attach(GTK_TABLE(table), taskbar_name_inactive_color, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(taskbar_name_inactive_color,
                                _("Specifies the font color used to display the name of inactive desktops."));

    col = 1;
    row++;
    taskbar_name_font_set = gtk_check_button_new();
    gtk_widget_show(taskbar_name_font_set);
    gtk_table_attach(GTK_TABLE(table), taskbar_name_font_set, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_set_tooltip_text(taskbar_name_font_set,
                                _("If not checked, the desktop theme font is used. If checked, the custom font specified here "
                                  "is used."));
    col++;

    label = gtk_label_new(_("Font"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    PangoFontDescription *taskbar_name_font_desc = pango_font_description_from_string(get_default_font());
    pango_font_description_set_weight(taskbar_name_font_desc, PANGO_WEIGHT_BOLD);
    taskbar_name_font = gtk_font_button_new_with_font(pango_font_description_to_string(taskbar_name_font_desc));
    pango_font_description_free(taskbar_name_font_desc);
    gtk_widget_show(taskbar_name_font);
    gtk_table_attach(GTK_TABLE(table), taskbar_name_font, col, col + 3, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_font_button_set_show_style(GTK_FONT_BUTTON(taskbar_name_font), TRUE);
    gtk_widget_set_tooltip_text(taskbar_name_font, _("Specifies the font used to display the desktop name."));
    g_signal_connect(G_OBJECT(taskbar_name_font_set),
                       "toggled",
                       G_CALLBACK(font_set_callback),
                       taskbar_name_font);
    font_set_callback(taskbar_name_font_set, taskbar_name_font);

    col = 2;
    row++;
    label = gtk_label_new(_("Active background"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    taskbar_name_active_background = create_background_combo(_("Active desktop name"));
    gtk_widget_show(taskbar_name_active_background);
    gtk_table_attach(GTK_TABLE(table), taskbar_name_active_background, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(taskbar_name_active_background,
                                _("Selects the background used to display the name of the current desktop. "
                                  "Backgrounds can be edited in the Backgrounds tab."));

    col = 2;
    row++;
    label = gtk_label_new(_("Inactive background"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    taskbar_name_inactive_background = create_background_combo(_("Inactive desktop name"));
    gtk_widget_show(taskbar_name_inactive_background);
    gtk_table_attach(GTK_TABLE(table), taskbar_name_inactive_background, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(taskbar_name_inactive_background,
                                _("Selects the background used to display the name of inactive desktops. "
                                  "Backgrounds can be edited in the Backgrounds tab."));

    change_paragraph(parent);
}

void init_task_page (GtkWidget *parent)
{
    GtkWidget *table, *label, *notebook;
    int row, col;

    label = gtk_label_new(_("<b>Mouse events</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(3, 10, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("Left click"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    task_mouse_left = gtk_combo_box_text_new();
    gtk_widget_show(task_mouse_left);
    gtk_table_attach(GTK_TABLE(table), task_mouse_left, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_left), _("None"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_left), _("Close"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_left), _("Toggle"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_left), _("Iconify"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_left), _("Shade"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_left), _("Toggle or iconify"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_left), _("Maximize or restore"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_left), _("Desktop left"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_left), _("Desktop right"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_left), _("Next task"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_left), _("Previous task"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(task_mouse_left), 5);
    gtk_widget_set_tooltip_text(task_mouse_left,
                                _("Specifies the action performed when task buttons receive a left click event: \n"
                                  "'None' means that no action is taken. \n"
                                  "'Close' closes the task. \n"
                                  "'Toggle' toggles the task. \n"
                                  "'Iconify' iconifies (minimizes) the task. \n"
                                  "'Shade' shades (collapses) the task. \n"
                                  "'Toggle or iconify' toggles or iconifies the task. \n"
                                  "'Maximize or restore' maximizes or minimizes the task. \n"
                                  "'Desktop left' sends the task to the previous desktop. \n"
                                  "'Desktop right' sends the task to the next desktop. \n"
                                  "'Next task' sends the focus to the next task. \n"
                                  "'Previous task' sends the focus to the previous task."));

    row++, col = 2;
    label = gtk_label_new(_("Wheel scroll up"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    task_mouse_scroll_up = gtk_combo_box_text_new();
    gtk_widget_show(task_mouse_scroll_up);
    gtk_table_attach(GTK_TABLE(table), task_mouse_scroll_up, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_up), _("None"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_up), _("Close"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_up), _("Toggle"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_up), _("Iconify"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_up), _("Shade"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_up), _("Toggle or iconify"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_up), _("Maximize or restore"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_up), _("Desktop left"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_up), _("Desktop right"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_up), _("Next task"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_up), _("Previous task"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(task_mouse_scroll_up), 0);
    gtk_widget_set_tooltip_text(task_mouse_scroll_up,
                                _("Specifies the action performed when task buttons receive a scroll up event: \n"
                                  "'None' means that no action is taken. \n"
                                  "'Close' closes the task. \n"
                                  "'Toggle' toggles the task. \n"
                                  "'Iconify' iconifies (minimizes) the task. \n"
                                  "'Shade' shades (collapses) the task. \n"
                                  "'Toggle or iconify' toggles or iconifies the task. \n"
                                  "'Maximize or restore' maximizes or minimizes the task. \n"
                                  "'Desktop left' sends the task to the previous desktop. \n"
                                  "'Desktop right' sends the task to the next desktop. \n"
                                  "'Next task' sends the focus to the next task. \n"
                                  "'Previous task' sends the focus to the previous task."));

    row++, col = 2;
    label = gtk_label_new(_("Middle click"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    task_mouse_middle = gtk_combo_box_text_new();
    gtk_widget_show(task_mouse_middle);
    gtk_table_attach(GTK_TABLE(table), task_mouse_middle, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_middle), _("None"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_middle), _("Close"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_middle), _("Toggle"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_middle), _("Iconify"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_middle), _("Shade"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_middle), _("Toggle or iconify"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_middle), _("Maximize or restore"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_middle), _("Desktop left"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_middle), _("Desktop right"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_middle), _("Next task"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_middle), _("Previous task"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(task_mouse_middle), 0);
    gtk_widget_set_tooltip_text(task_mouse_middle,
                                _("Specifies the action performed when task buttons receive a middle click event: \n"
                                  "'None' means that no action is taken. \n"
                                  "'Close' closes the task. \n"
                                  "'Toggle' toggles the task. \n"
                                  "'Iconify' iconifies (minimizes) the task. \n"
                                  "'Shade' shades (collapses) the task. \n"
                                  "'Toggle or iconify' toggles or iconifies the task. \n"
                                  "'Maximize or restore' maximizes or minimizes the task. \n"
                                  "'Desktop left' sends the task to the previous desktop. \n"
                                  "'Desktop right' sends the task to the next desktop. \n"
                                  "'Next task' sends the focus to the next task. \n"
                                  "'Previous task' sends the focus to the previous task."));

    row++, col = 2;
    label = gtk_label_new(_("Wheel scroll down"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    task_mouse_scroll_down = gtk_combo_box_text_new();
    gtk_widget_show(task_mouse_scroll_down);
    gtk_table_attach(GTK_TABLE(table), task_mouse_scroll_down, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_down), _("None"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_down), _("Close"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_down), _("Toggle"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_down), _("Iconify"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_down), _("Shade"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_down), _("Toggle or iconify"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_down), _("Maximize or restore"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_down), _("Desktop left"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_down), _("Desktop right"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_down), _("Next task"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_scroll_down), _("Previous task"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(task_mouse_scroll_down), 0);
    gtk_widget_set_tooltip_text(task_mouse_scroll_down,
                                _("Specifies the action performed when task buttons receive a scroll down event: \n"
                                  "'None' means that no action is taken. \n"
                                  "'Close' closes the task. \n"
                                  "'Toggle' toggles the task. \n"
                                  "'Iconify' iconifies (minimizes) the task. \n"
                                  "'Shade' shades (collapses) the task. \n"
                                  "'Toggle or iconify' toggles or iconifies the task. \n"
                                  "'Maximize or restore' maximizes or minimizes the task. \n"
                                  "'Desktop left' sends the task to the previous desktop. \n"
                                  "'Desktop right' sends the task to the next desktop. \n"
                                  "'Next task' sends the focus to the next task. \n"
                                  "'Previous task' sends the focus to the previous task."));

    row++, col = 2;
    label = gtk_label_new(_("Right click"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    task_mouse_right = gtk_combo_box_text_new();
    gtk_widget_show(task_mouse_right);
    gtk_table_attach(GTK_TABLE(table), task_mouse_right, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_right), _("None"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_right), _("Close"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_right), _("Toggle"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_right), _("Iconify"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_right), _("Shade"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_right), _("Toggle or iconify"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_right), _("Maximize or restore"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_right), _("Desktop left"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_right), _("Desktop right"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_right), _("Next task"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(task_mouse_right), _("Previous task"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(task_mouse_right), 1);
    gtk_widget_set_tooltip_text(task_mouse_right,
                                _("Specifies the action performed when task buttons receive a right click event: \n"
                                  "'None' means that no action is taken. \n"
                                  "'Close' closes the task. \n"
                                  "'Toggle' toggles the task. \n"
                                  "'Iconify' iconifies (minimizes) the task. \n"
                                  "'Shade' shades (collapses) the task. \n"
                                  "'Toggle or iconify' toggles or iconifies the task. \n"
                                  "'Maximize or restore' maximizes or minimizes the task. \n"
                                  "'Desktop left' sends the task to the previous desktop. \n"
                                  "'Desktop right' sends the task to the next desktop. \n"
                                  "'Next task' sends the focus to the next task. \n"
                                  "'Previous task' sends the focus to the previous task."));

    change_paragraph(parent);

    label = gtk_label_new(_("<b>Appearance</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(4, 13, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("Show icon"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    task_show_icon = gtk_check_button_new();
    gtk_widget_show(task_show_icon);
    gtk_table_attach(GTK_TABLE(table), task_show_icon, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(task_show_icon, _("If enabled, the window icon is shown on task buttons."));

    row++, col = 2;
    label = gtk_label_new(_("Show text"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    task_show_text = gtk_check_button_new();
    gtk_widget_show(task_show_text);
    gtk_table_attach(GTK_TABLE(table), task_show_text, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(task_show_text, _("If enabled, the window title is shown on task buttons."));

    row++, col = 2;
    label = gtk_label_new(_("Center text"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    task_align_center = gtk_check_button_new();
    gtk_widget_show(task_align_center);
    gtk_table_attach(GTK_TABLE(table), task_align_center, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(task_align_center,
                                _("If enabled, the text is centered on task buttons. Otherwise, it is left-aligned."));

    row++, col = 2;
    label = gtk_label_new(_("Show tooltips"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    tooltip_task_show = gtk_check_button_new();
    gtk_widget_show(tooltip_task_show);
    gtk_table_attach(GTK_TABLE(table), tooltip_task_show, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(tooltip_task_show,
                                _("If enabled, a tooltip showing the window title is displayed when the mouse cursor moves "
                                  "over task buttons."));

    row++, col = 2;
    label = gtk_label_new(_("Thumbnails"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    tooltip_task_thumbnail = gtk_check_button_new();
    gtk_widget_show(tooltip_task_thumbnail);
    gtk_table_attach(GTK_TABLE(table), tooltip_task_thumbnail, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(tooltip_task_thumbnail,
                                _("If enabled, a tooltip showing the window thumbnail is displayed when the mouse cursor moves "
                                  "over task buttons."));

    row++, col = 2;
    label = gtk_label_new(_("Thumbnail size"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    tooltip_task_thumbnail_size = gtk_spin_button_new_with_range(8, 9000, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(tooltip_task_thumbnail_size), 210);
    gtk_widget_show(tooltip_task_thumbnail_size);
    gtk_table_attach(GTK_TABLE(table), tooltip_task_thumbnail_size, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    row++, col = 2;
    label = gtk_label_new(_("Maximum width"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    task_maximum_width = gtk_spin_button_new_with_range(0, 9000, 1);
    gtk_widget_show(task_maximum_width);
    gtk_table_attach(GTK_TABLE(table), task_maximum_width, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(task_maximum_width, _("Specifies the maximum width of the task buttons."));

    row++, col = 2;
    label = gtk_label_new(_("Maximum height"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    task_maximum_height = gtk_spin_button_new_with_range(0, 9000, 1);
    gtk_widget_show(task_maximum_height);
    gtk_table_attach(GTK_TABLE(table), task_maximum_height, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(task_maximum_height, _("Specifies the maximum height of the task buttons."));

    row++, col = 2;
    label = gtk_label_new(_("Horizontal padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    task_padding_x = gtk_spin_button_new_with_range(0, 9000, 1);
    gtk_widget_show(task_padding_x);
    gtk_table_attach(GTK_TABLE(table), task_padding_x, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(task_padding_x,
                                _("Specifies the horizontal padding of the task buttons. "
                                  "This is the space between the border and the content inside."));

    row++, col = 2;
    label = gtk_label_new(_("Vertical padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    task_padding_y = gtk_spin_button_new_with_range(0, 9000, 1);
    gtk_widget_show(task_padding_y);
    gtk_table_attach(GTK_TABLE(table), task_padding_y, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(task_padding_y,
                                _("Specifies the vertical padding of the task buttons. "
                                  "This is the space between the border and the content inside."));

    row++, col = 2;
    label = gtk_label_new(_("Spacing"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    task_spacing = gtk_spin_button_new_with_range(-9000, 9000, 1);
    gtk_widget_show(task_spacing);
    gtk_table_attach(GTK_TABLE(table), task_spacing, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(task_spacing, _("Specifies the spacing between the icon and the text."
                                                "Set to negative for overlap (text above)."));

    row++, col = 1;
    task_font_set = gtk_check_button_new();
    gtk_widget_show(task_font_set);
    gtk_table_attach(GTK_TABLE(table), task_font_set, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_set_tooltip_text(task_font_set,
                                _("If not checked, the desktop theme font is used. If checked, the custom font specified here "
                                  "is used."));
    col++;

    label = gtk_label_new(_("Font"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    task_font = gtk_font_button_new_with_font(get_default_font());
    gtk_widget_show(task_font);
    gtk_table_attach(GTK_TABLE(table), task_font, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_font_button_set_show_style(GTK_FONT_BUTTON(task_font), TRUE);
    gtk_widget_set_tooltip_text(task_font, _("Specifies the font used to display the task button text."));
    g_signal_connect(G_OBJECT(task_font_set), "toggled", G_CALLBACK(font_set_callback), task_font);
    font_set_callback(task_font_set, task_font);

    change_paragraph(parent);

    notebook = gtk_notebook_new();
    gtk_widget_show(notebook);
    gtk_container_set_border_width(GTK_CONTAINER(notebook), 0);
    gtk_box_pack_start(GTK_BOX(parent), notebook, TRUE, TRUE, 0);

    create_task_status(notebook,
                       _("Default style"),
                       _("Default task"),
                       &task_default_color,
                       &task_default_color_set,
                       &task_default_icon_opacity,
                       &task_default_icon_osb_set,
                       &task_default_icon_saturation,
                       &task_default_icon_brightness,
                       &task_default_background,
                       &task_default_background_set);
    create_task_status(notebook,
                       _("Normal task"),
                       _("Normal task"),
                       &task_normal_color,
                       &task_normal_color_set,
                       &task_normal_icon_opacity,
                       &task_normal_icon_osb_set,
                       &task_normal_icon_saturation,
                       &task_normal_icon_brightness,
                       &task_normal_background,
                       &task_normal_background_set);
    create_task_status(notebook,
                       _("Active task"),
                       _("Active task"),
                       &task_active_color,
                       &task_active_color_set,
                       &task_active_icon_opacity,
                       &task_active_icon_osb_set,
                       &task_active_icon_saturation,
                       &task_active_icon_brightness,
                       &task_active_background,
                       &task_active_background_set);
    create_task_status(notebook,
                       _("Urgent task"),
                       _("Urgent task"),
                       &task_urgent_color,
                       &task_urgent_color_set,
                       &task_urgent_icon_opacity,
                       &task_urgent_icon_osb_set,
                       &task_urgent_icon_saturation,
                       &task_urgent_icon_brightness,
                       &task_urgent_background,
                       &task_urgent_background_set);
    create_task_status(notebook,
                       _("Iconified task"),
                       _("Iconified task"),
                       &task_iconified_color,
                       &task_iconified_color_set,
                       &task_iconified_icon_opacity,
                       &task_iconified_icon_osb_set,
                       &task_iconified_icon_saturation,
                       &task_iconified_icon_brightness,
                       &task_iconified_background,
                       &task_iconified_background_set);
}

void task_status_toggle_button_callback(GtkWidget *widget, gpointer data)
{
    GtkWidget *child1, *child2, *child3;
    child1 = child2 = child3 = NULL;

    if (widget == task_default_color_set) {
        child1 = task_default_color;
    } else if (widget == task_default_icon_osb_set) {
        child1 = task_default_icon_opacity;
        child2 = task_default_icon_saturation;
        child3 = task_default_icon_brightness;
    } else if (widget == task_default_background_set) {
        child1 = task_default_background;
    } else if (widget == task_normal_color_set) {
        child1 = task_normal_color;
    } else if (widget == task_normal_icon_osb_set) {
        child1 = task_normal_icon_opacity;
        child2 = task_normal_icon_saturation;
        child3 = task_normal_icon_brightness;
    } else if (widget == task_normal_background_set) {
        child1 = task_normal_background;
    } else if (widget == task_active_color_set) {
        child1 = task_active_color;
    } else if (widget == task_active_icon_osb_set) {
        child1 = task_active_icon_opacity;
        child2 = task_active_icon_saturation;
        child3 = task_active_icon_brightness;
    } else if (widget == task_active_background_set) {
        child1 = task_active_background;
    } else if (widget == task_urgent_color_set) {
        child1 = task_urgent_color;
    } else if (widget == task_urgent_icon_osb_set) {
        child1 = task_urgent_icon_opacity;
        child2 = task_urgent_icon_saturation;
        child3 = task_urgent_icon_brightness;
    } else if (widget == task_urgent_background_set) {
        child1 = task_urgent_background;
    } else if (widget == task_iconified_color_set) {
        child1 = task_iconified_color;
    } else if (widget == task_iconified_icon_osb_set) {
        child1 = task_iconified_icon_opacity;
        child2 = task_iconified_icon_saturation;
        child3 = task_iconified_icon_brightness;
    } else if (widget == task_iconified_background_set) {
        child1 = task_iconified_background;
    }

    if (child1)
        gtk_widget_set_sensitive(child1, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
    if (child2)
        gtk_widget_set_sensitive(child2, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
    if (child3)
        gtk_widget_set_sensitive(child3, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

void create_task_status(GtkWidget *notebook,
                        char *name,
                        char *text,
                        GtkWidget **task_status_color,
                        GtkWidget **task_status_color_set,
                        GtkWidget **task_status_icon_opacity,
                        GtkWidget **task_status_icon_osb_set,
                        GtkWidget **task_status_icon_saturation,
                        GtkWidget **task_status_icon_brightness,
                        GtkWidget **task_status_background,
                        GtkWidget **task_status_background_set)
{
    GtkWidget *label = gtk_label_new(_(name));
    gtk_widget_show(label);
    GtkWidget *page_task = gtk_vbox_new(FALSE, DEFAULT_HOR_SPACING);
    gtk_container_set_border_width(GTK_CONTAINER(page_task), 10);
    gtk_widget_show(page_task);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_task, label);

    GtkWidget *table = gtk_table_new(6, 3, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(page_task), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

    *task_status_color_set = gtk_check_button_new();
    gtk_widget_show(*task_status_color_set);
    gtk_table_attach(GTK_TABLE(table), *task_status_color_set, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);
    g_signal_connect(G_OBJECT(*task_status_color_set),
                       "toggled",
                       G_CALLBACK(task_status_toggle_button_callback),
                       NULL);
    gtk_widget_set_tooltip_text(*task_status_color_set,
                                _("If enabled, a custom font color is used to display the task text."));

    label = gtk_label_new(_("Font color"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);

    *task_status_color = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(*task_status_color), TRUE);
    gtk_widget_show(*task_status_color);
    gtk_table_attach(GTK_TABLE(table), *task_status_color, 2, 3, 0, 1, GTK_FILL, 0, 0, 0);
    gtk_widget_set_tooltip_text(*task_status_color,
                                _("Specifies the font color used to display the task text."));

    *task_status_icon_osb_set = gtk_check_button_new();
    gtk_widget_show(*task_status_icon_osb_set);
    gtk_table_attach(GTK_TABLE(table), *task_status_icon_osb_set, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);
    g_signal_connect(G_OBJECT(*task_status_icon_osb_set),
                       "toggled",
                       G_CALLBACK(task_status_toggle_button_callback),
                       NULL);
    gtk_widget_set_tooltip_text(*task_status_icon_osb_set,
                                _("If enabled, a custom opacity/saturation/brightness is used to display the task icon."));

    label = gtk_label_new(_("Icon opacity"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);

    *task_status_icon_opacity = gtk_spin_button_new_with_range(0, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(*task_status_icon_opacity), 100);
    gtk_widget_show(*task_status_icon_opacity);
    gtk_table_attach(GTK_TABLE(table), *task_status_icon_opacity, 2, 3, 1, 2, GTK_FILL, 0, 0, 0);
    gtk_widget_set_tooltip_text(*task_status_icon_opacity,
                                _("Specifies the opacity (in %) used to display the task icon."));

    label = gtk_label_new(_("Icon saturation"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, 2, 3, GTK_FILL, 0, 0, 0);

    *task_status_icon_saturation = gtk_spin_button_new_with_range(-100, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(*task_status_icon_saturation), 0);
    gtk_widget_show(*task_status_icon_saturation);
    gtk_table_attach(GTK_TABLE(table), *task_status_icon_saturation, 2, 3, 2, 3, GTK_FILL, 0, 0, 0);
    gtk_widget_set_tooltip_text(*task_status_icon_saturation,
                                _("Specifies the saturation adjustment (in %) used to display the task icon."));

    label = gtk_label_new(_("Icon brightness"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, 3, 4, GTK_FILL, 0, 0, 0);

    *task_status_icon_brightness = gtk_spin_button_new_with_range(-100, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(*task_status_icon_brightness), 0);
    gtk_widget_show(*task_status_icon_brightness);
    gtk_table_attach(GTK_TABLE(table), *task_status_icon_brightness, 2, 3, 3, 4, GTK_FILL, 0, 0, 0);
    gtk_widget_set_tooltip_text(*task_status_icon_brightness,
                                _("Specifies the brightness adjustment (in %) used to display the task icon."));

    *task_status_background_set = gtk_check_button_new();
    gtk_widget_show(*task_status_background_set);
    gtk_table_attach(GTK_TABLE(table), *task_status_background_set, 0, 1, 4, 5, GTK_FILL, 0, 0, 0);
    g_signal_connect(G_OBJECT(*task_status_background_set),
                       "toggled",
                       G_CALLBACK(task_status_toggle_button_callback),
                       NULL);
    gtk_widget_set_tooltip_text(*task_status_background_set,
                                _("If enabled, a custom background is used to display the task."));

    label = gtk_label_new(_("Background"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, 1, 2, 4, 5, GTK_FILL, 0, 0, 0);

    *task_status_background = create_background_combo(text);
    gtk_widget_show(*task_status_background);
    gtk_table_attach(GTK_TABLE(table), *task_status_background, 2, 3, 4, 5, GTK_FILL, 0, 0, 0);
    gtk_widget_set_tooltip_text(*task_status_background,
                                _("Selects the background used to display the task. "
                                  "Backgrounds can be edited in the Backgrounds tab."));

    if (*task_status_color == task_urgent_color) {
        label = gtk_label_new(_("Blinks"));
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
        gtk_widget_show(label);
        gtk_table_attach(GTK_TABLE(table), label, 1, 2, 5, 6, GTK_FILL, 0, 0, 0);

        task_urgent_blinks = gtk_spin_button_new_with_range(0, 1000000, 1);
        gtk_widget_show(task_urgent_blinks);
        gtk_table_attach(GTK_TABLE(table), task_urgent_blinks, 2, 3, 5, 6, GTK_FILL, 0, 0, 0);
        gtk_widget_set_tooltip_text(task_urgent_blinks, _("Specifies how many times urgent tasks blink."));
    }

    task_status_toggle_button_callback(*task_status_color_set, NULL);
    task_status_toggle_button_callback(*task_status_icon_osb_set, NULL);
    task_status_toggle_button_callback(*task_status_background_set, NULL);
}

void init_clock_page (GtkWidget *parent)
{
    GtkWidget *table;
    GtkWidget *label;
    int row, col;

    table = gtk_table_new(1, 2, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("<b>Format</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(3, 10, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("First line format"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    clock_format_line1 = gtk_entry_new();
    gtk_widget_show(clock_format_line1);
    gtk_entry_set_width_chars(GTK_ENTRY(clock_format_line1), 16);
    gtk_table_attach(GTK_TABLE(table), clock_format_line1, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(clock_format_line1,
                                _("Specifies the format used to display the first line of the clock text. "
                                  "See 'man date' for all the available options."));

    row++, col = 2;
    label = gtk_label_new(_("Second line format"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    clock_format_line2 = gtk_entry_new();
    gtk_widget_show(clock_format_line2);
    gtk_entry_set_width_chars(GTK_ENTRY(clock_format_line2), 16);
    gtk_table_attach(GTK_TABLE(table), clock_format_line2, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(clock_format_line2,
                                _("Specifies the format used to display the second line of the clock text. "
                                  "See 'man date' for all the available options."));

    row++, col = 2;
    label = gtk_label_new(_("First line timezone"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    clock_tmz_line1 = gtk_entry_new();
    gtk_widget_show(clock_tmz_line1);
    gtk_entry_set_width_chars(GTK_ENTRY(clock_tmz_line1), 16);
    gtk_table_attach(GTK_TABLE(table), clock_tmz_line1, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(clock_tmz_line1,
                                _("Specifies the timezone used to display the first line of the clock text. If empty, the "
                                  "current timezone is used. "
                                  "Otherwise, it must be set to a valid value of the TZ environment variable."));

    row++, col = 2;
    label = gtk_label_new(_("Second line timezone"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    clock_tmz_line2 = gtk_entry_new();
    gtk_widget_show(clock_tmz_line2);
    gtk_entry_set_width_chars(GTK_ENTRY(clock_tmz_line2), 16);
    gtk_table_attach(GTK_TABLE(table), clock_tmz_line2, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(clock_tmz_line2,
                                _("Specifies the timezone used to display the second line of the clock text. If empty, the "
                                  "current timezone is used. "
                                  "Otherwise, it must be set to a valid value of the TZ environment variable."));

    change_paragraph(parent);

    label = gtk_label_new(_("<b>Mouse events</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(5, 10, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("Left click command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    clock_left_command = gtk_entry_new();
    gtk_widget_show(clock_left_command);
    gtk_entry_set_width_chars(GTK_ENTRY(clock_left_command), 50);
    gtk_table_attach(GTK_TABLE(table), clock_left_command, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(clock_left_command,
                                _("Specifies a command that will be executed when the clock receives a left click."));

    row++, col = 2;
    label = gtk_label_new(_("Right click command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    clock_right_command = gtk_entry_new();
    gtk_widget_show(clock_right_command);
    gtk_entry_set_width_chars(GTK_ENTRY(clock_right_command), 50);
    gtk_table_attach(GTK_TABLE(table), clock_right_command, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(clock_right_command,
                                _("Specifies a command that will be executed when the clock receives a right click."));

    row++, col = 2;
    label = gtk_label_new(_("Middle click command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    clock_mclick_command = gtk_entry_new();
    gtk_widget_show(clock_mclick_command);
    gtk_entry_set_width_chars(GTK_ENTRY(clock_mclick_command), 50);
    gtk_table_attach(GTK_TABLE(table), clock_mclick_command, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(clock_mclick_command,
                                _("Specifies a command that will be executed when the clock receives a middle click."));

    row++, col = 2;
    label = gtk_label_new(_("Wheel scroll up command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    clock_uwheel_command = gtk_entry_new();
    gtk_widget_show(clock_uwheel_command);
    gtk_entry_set_width_chars(GTK_ENTRY(clock_uwheel_command), 50);
    gtk_table_attach(GTK_TABLE(table), clock_uwheel_command, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(clock_uwheel_command,
                                _("Specifies a command that will be executed when the clock receives a mouse scroll up."));

    row++, col = 2;
    label = gtk_label_new(_("Wheel scroll down command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    clock_dwheel_command = gtk_entry_new();
    gtk_widget_show(clock_dwheel_command);
    gtk_entry_set_width_chars(GTK_ENTRY(clock_dwheel_command), 50);
    gtk_table_attach(GTK_TABLE(table), clock_dwheel_command, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(clock_dwheel_command,
                                _("Specifies a command that will be executed when the clock receives a mouse scroll down."));

    change_paragraph(parent);

    label = gtk_label_new(_("<b>Appearance</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(3, 22, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("Background"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    clock_background = create_background_combo(_("Clock"));
    gtk_widget_show(clock_background);
    gtk_table_attach(GTK_TABLE(table), clock_background, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(clock_background,
                                _("Selects the background used to display the clock. "
                                  "Backgrounds can be edited in the Backgrounds tab."));

    row++, col = 2;
    label = gtk_label_new(_("Horizontal padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    clock_padding_x = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(clock_padding_x);
    gtk_table_attach(GTK_TABLE(table), clock_padding_x, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(clock_padding_x,
                                _("Specifies the horizontal padding of the clock. "
                                  "This is the space between the border and the content inside."));

    row++, col = 2;
    label = gtk_label_new(_("Vertical padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    clock_padding_y = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(clock_padding_y);
    gtk_table_attach(GTK_TABLE(table), clock_padding_y, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(clock_padding_y,
                                _("Specifies the vertical padding of the clock. "
                                  "This is the space between the border and the content inside."));

    row++, col = 1;
    clock_font_line1_set = gtk_check_button_new();
    gtk_widget_show(clock_font_line1_set);
    gtk_table_attach(GTK_TABLE(table), clock_font_line1_set, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_set_tooltip_text(clock_font_line1_set,
                                _("If not checked, the desktop theme font is used. If checked, the custom font specified here "
                                  "is used."));
    col++;

    label = gtk_label_new(_("Font first line"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    PangoFontDescription *time1_font_desc = pango_font_description_from_string(get_default_font());
    pango_font_description_set_weight(time1_font_desc, PANGO_WEIGHT_BOLD);
    pango_font_description_set_size(time1_font_desc, pango_font_description_get_size(time1_font_desc));
    clock_font_line1 = gtk_font_button_new_with_font(pango_font_description_to_string(time1_font_desc));
    pango_font_description_free(time1_font_desc);
    gtk_widget_show(clock_font_line1);
    gtk_table_attach(GTK_TABLE(table), clock_font_line1, col, col + 3, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_font_button_set_show_style(GTK_FONT_BUTTON(clock_font_line1), TRUE);
    gtk_widget_set_tooltip_text(clock_font_line1,
                                _("Specifies the font used to display the first line of the clock."));
    g_signal_connect(G_OBJECT(clock_font_line1_set),
                       "toggled",
                       G_CALLBACK(font_set_callback),
                       clock_font_line1);
    font_set_callback(clock_font_line1_set, clock_font_line1);

    row++, col = 1;
    clock_font_line2_set = gtk_check_button_new();
    gtk_widget_show(clock_font_line2_set);
    gtk_table_attach(GTK_TABLE(table), clock_font_line2_set, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_set_tooltip_text(clock_font_line2_set,
                                _("If not checked, the desktop theme font is used. If checked, the custom font specified here "
                                  "is used."));
    col++;

    label = gtk_label_new(_("Font second line"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    PangoFontDescription *time2_font_desc = pango_font_description_from_string(get_default_font());
    pango_font_description_set_size(time2_font_desc, pango_font_description_get_size(time2_font_desc) - PANGO_SCALE);
    clock_font_line2 = gtk_font_button_new_with_font(pango_font_description_to_string(time2_font_desc));
    ;
    pango_font_description_free(time2_font_desc);
    gtk_widget_show(clock_font_line2);
    gtk_table_attach(GTK_TABLE(table), clock_font_line2, col, col + 3, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_font_button_set_show_style(GTK_FONT_BUTTON(clock_font_line2), TRUE);
    gtk_widget_set_tooltip_text(clock_font_line2,
                                _("Specifies the font used to display the second line of the clock."));
    g_signal_connect(G_OBJECT(clock_font_line2_set),
                       "toggled",
                       G_CALLBACK(font_set_callback),
                       clock_font_line2);
    font_set_callback(clock_font_line2_set, clock_font_line2);

    row++, col = 2;
    label = gtk_label_new(_("Font color"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    clock_font_color = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(clock_font_color), TRUE);
    gtk_widget_show(clock_font_color);
    gtk_table_attach(GTK_TABLE(table), clock_font_color, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(clock_font_color, _("Specifies the font color used to display the clock."));

    change_paragraph(parent);

    label = gtk_label_new(_("<b>Tooltip</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(3, 10, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("Format"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    clock_format_tooltip = gtk_entry_new();
    gtk_widget_show(clock_format_tooltip);
    gtk_entry_set_width_chars(GTK_ENTRY(clock_format_tooltip), 30);
    gtk_table_attach(GTK_TABLE(table), clock_format_tooltip, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(clock_format_tooltip,
                                _("Specifies the format used to display the clock tooltip. "
                                  "See 'man date' for the available options."));

    row++, col = 2;
    label = gtk_label_new(_("Timezone"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    clock_tmz_tooltip = gtk_entry_new();
    gtk_widget_show(clock_tmz_tooltip);
    gtk_entry_set_width_chars(GTK_ENTRY(clock_tmz_tooltip), 16);
    gtk_table_attach(GTK_TABLE(table), clock_tmz_tooltip, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(clock_tmz_tooltip,
                                _("Specifies the timezone used to display the clock tooltip. If empty, the current timezone "
                                  "is used. "
                                  "Otherwise, it must be set to a valid value of the TZ environment variable."));

    change_paragraph(parent);
}

void create_separator(GtkWidget *stack, int i)
{
    GtkWidget *label;
    GtkWidget *table;
    int row, col;
    Separator *separator = &g_array_index(separators, Separator, i);
    STRBUF_AUTO_PRINTF (separator->name, "%s %d", _("Separator"), i + 1);
    separator->page = gtk_box_new(GTK_ORIENTATION_VERTICAL, DEFAULT_HOR_SPACING);
    gtk_container_set_border_width(GTK_CONTAINER(separator->page), 10);
    gtk_widget_show(separator->page);
    gtk_stack_add_titled(GTK_STACK(stack), separator->page, NULL, separator->name);

    GtkWidget *parent = separator->page;

    table = gtk_table_new(1, 2, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("<b>Appearance</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(3, 10, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("Background"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    separator->bg = create_background_combo(_("Separator"));
    gtk_widget_show(separator->bg);
    gtk_table_attach(GTK_TABLE(table), separator->bg, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    row++, col = 2;
    label = gtk_label_new(_("Foreground color"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    separator->color = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(separator->color), TRUE);
    gtk_widget_show(separator->color);
    GdkRGBA color = {   .red   = 0x77 / 255.0,
                        .green = 0x77 / 255.0,
                        .blue  = 0x77 / 255.0,
                        .alpha = 0.9 };
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(separator->color), &color);
    gtk_table_attach(GTK_TABLE(table), separator->color, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    row++, col = 2;
    label = gtk_label_new(_("Style"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    separator->style = gtk_combo_box_text_new();
    gtk_widget_show(separator->style);
    gtk_table_attach(GTK_TABLE(table), separator->style, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(separator->style), _("Empty"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(separator->style), _("Line"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(separator->style), _("Dots"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(separator->style), 2);

    row++, col = 2;
    label = gtk_label_new(_("Size"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    separator->size = gtk_spin_button_new_with_range(0, 10000, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(separator->size), 3);
    gtk_widget_show(separator->size);
    gtk_table_attach(GTK_TABLE(table), separator->size, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    row++, col = 2;
    label = gtk_label_new(_("Horizontal padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    separator->padx = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(separator->padx), 1);
    gtk_widget_show(separator->padx);
    gtk_table_attach(GTK_TABLE(table), separator->padx, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    row++, col = 2;
    label = gtk_label_new(_("Vertical padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    separator->pady = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(separator->pady);
    gtk_table_attach(GTK_TABLE(table), separator->pady, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    change_paragraph(parent);
}

void create_execp(GtkWidget *stack, int i)
{
    GtkWidget *label;
    GtkWidget *table;
    int row, col;

    Executor *executor = &g_array_index(executors, Executor, i);

    STRBUF_AUTO_PRINTF (executor->name, "%s %d", _("Executor"), i + 1);
    executor->page = gtk_box_new(GTK_ORIENTATION_VERTICAL, DEFAULT_HOR_SPACING);
    gtk_container_set_border_width(GTK_CONTAINER(executor->page), 10);
    gtk_widget_show(executor->page);
    gtk_stack_add_titled(GTK_STACK(stack), executor->page, NULL, executor->name);

    GtkWidget *parent = executor->page;

    table = gtk_table_new(1, 2, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("<b>Format</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(3, 10, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("Name"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->id = gtk_entry_new();
    gtk_widget_show(executor->id);
    gtk_entry_set_width_chars(GTK_ENTRY(executor->id), 20);
    gtk_entry_set_max_length(GTK_ENTRY(executor->id), 20);
    gtk_table_attach(GTK_TABLE(table), executor->id, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->id,
                                _("Specifies a name that can be used with `tint2-send refresh-execp` to re-execute "
                                  "the command."));

    row++, col = 2;
    label = gtk_label_new(_("Command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->cmd = gtk_entry_new();
    gtk_widget_show(executor->cmd);
    gtk_entry_set_width_chars(GTK_ENTRY(executor->cmd), 50);
    gtk_table_attach(GTK_TABLE(table), executor->cmd, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->cmd, _("Specifies the command to execute."));

    row++, col = 2;
    label = gtk_label_new(_("Interval"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->interval = gtk_spin_button_new_with_range(0, 1000000, 1);
    gtk_widget_show(executor->interval);
    gtk_table_attach(GTK_TABLE(table), executor->interval, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->interval,
                                _("Specifies the interval at which the command is executed, in seconds. "
                                  "If zero, the command is executed only once."));

    row++, col = 2;
    label = gtk_label_new(_("Show icon"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->has_icon = gtk_check_button_new();
    gtk_widget_show(executor->has_icon);
    gtk_table_attach(GTK_TABLE(table), executor->has_icon, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->has_icon,
                                _("If enabled, the first line printed by the command is interpreted "
                                  "as a path to an image file."));

    row++, col = 2;
    label = gtk_label_new(_("Cache icon"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->cache_icon = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(executor->cache_icon), 1);
    gtk_widget_show(executor->cache_icon);
    gtk_table_attach(GTK_TABLE(table), executor->cache_icon, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->cache_icon,
                                _("If enabled, the image is not reloaded from disk every time the command is executed if the "
                                  "path remains unchanged. Enabling this is recommended."));

    row++, col = 2;
    label = gtk_label_new(_("Continuous output"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->cont = gtk_spin_button_new_with_range(0, 1000000, 1);
    gtk_widget_show(executor->cont);
    gtk_table_attach(GTK_TABLE(table), executor->cont, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->cont,
                                _("If non-zero, the last execp_continuous lines from the output of "
                                  "the command are displayed, every execp_continuous lines; this is "
                                  "useful for showing the output of commands that run indefinitely, "
                                  "such as 'ping 127.0.0.1'. If zero, the output of the command is "
                                  "displayed after it finishes executing."));
    row++, col = 2;
    label = gtk_label_new(_("Display markup"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->markup = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(executor->markup), 1);
    gtk_widget_show(executor->markup);
    gtk_table_attach(GTK_TABLE(table), executor->markup, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->markup,
                                _("If enabled, the output of the command is treated as Pango markup, "
                                  "which allows rich text formatting. Note that using this with commands "
                                  "that print data downloaded from the Internet is a potential security risk."));

    row++, col = 2;
    label = gtk_label_new(_("Monitor"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->mon = gtk_combo_box_text_new();
    gtk_widget_show(executor->mon);
    gtk_table_attach(GTK_TABLE(table), executor->mon, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(executor->mon), _("All"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(executor->mon), _("Primary"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(executor->mon), _("1"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(executor->mon), _("2"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(executor->mon), _("3"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(executor->mon), _("4"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(executor->mon), _("5"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(executor->mon), _("6"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(executor->mon), 0);
    gtk_widget_set_tooltip_text(executor->mon,
                                _("Specifies the monitor on which to place the executor."));

    change_paragraph(parent);

    label = gtk_label_new(_("<b>Mouse events</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(5, 10, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("Left click command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->cmd_lclick = gtk_entry_new();
    gtk_widget_show(executor->cmd_lclick);
    gtk_entry_set_width_chars(GTK_ENTRY(executor->cmd_lclick), 50);
    gtk_table_attach(GTK_TABLE(table), executor->cmd_lclick, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->cmd_lclick,
                                _("Specifies a command that will be executed when the executor receives a left click."));

    label = gtk_label_new(_("Sink"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->cmd_lclick_sink = gtk_spin_button_new_with_range(-1, 65535, 1);
    gtk_widget_show(executor->cmd_lclick_sink);
    gtk_table_attach(GTK_TABLE(table), executor->cmd_lclick_sink, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->cmd_lclick_sink,
                                _("Specifies receiver of this command.\n"
                                  "If -1, command is executed in the default shell.\n"
                                  "If 0, command is sent to executor process (only continuous executors).\n"
                                  "Positive values are used for global command sinks (not implemented)."));

    row++, col = 2;
    label = gtk_label_new(_("Right click command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->cmd_rclick = gtk_entry_new();
    gtk_widget_show(executor->cmd_rclick);
    gtk_entry_set_width_chars(GTK_ENTRY(executor->cmd_rclick), 50);
    gtk_table_attach(GTK_TABLE(table), executor->cmd_rclick, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->cmd_rclick,
                                _("Specifies a command that will be executed when the executor receives a right click."));

    label = gtk_label_new(_("Sink"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->cmd_rclick_sink = gtk_spin_button_new_with_range(-1, 65535, 1);
    gtk_widget_show(executor->cmd_rclick_sink);
    gtk_table_attach(GTK_TABLE(table), executor->cmd_rclick_sink, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->cmd_rclick_sink,
                                _("Specifies receiver of this command.\n"
                                  "If -1, command is executed in the default shell.\n"
                                  "If 0, command is sent to executor process (only continuous executors).\n"
                                  "Positive values are used for global command sinks (not implemented)."));

    row++, col = 2;
    label = gtk_label_new(_("Middle click command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->cmd_mclick = gtk_entry_new();
    gtk_widget_show(executor->cmd_mclick);
    gtk_entry_set_width_chars(GTK_ENTRY(executor->cmd_mclick), 50);
    gtk_table_attach(GTK_TABLE(table), executor->cmd_mclick, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->cmd_mclick,
                                _("Specifies a command that will be executed when the executor receives a middle click."));

    label = gtk_label_new(_("Sink"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->cmd_mclick_sink = gtk_spin_button_new_with_range(-1, 65535, 1);
    gtk_widget_show(executor->cmd_mclick_sink);
    gtk_table_attach(GTK_TABLE(table), executor->cmd_mclick_sink, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->cmd_mclick_sink,
                                _("Specifies receiver of this command.\n"
                                  "If -1, command is executed in the default shell.\n"
                                  "If 0, command is sent to executor process (only continuous executors).\n"
                                  "Positive values are used for global command sinks (not implemented)."));

    row++, col = 2;
    label = gtk_label_new(_("Wheel scroll up command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->cmd_uwheel = gtk_entry_new();
    gtk_widget_show(executor->cmd_uwheel);
    gtk_entry_set_width_chars(GTK_ENTRY(executor->cmd_uwheel), 50);
    gtk_table_attach(GTK_TABLE(table), executor->cmd_uwheel, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->cmd_uwheel,
                                _("Specifies a command that will be executed when the executor receives a mouse scroll up."));

    label = gtk_label_new(_("Sink"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->cmd_uwheel_sink = gtk_spin_button_new_with_range(-1, 65535, 1);
    gtk_widget_show(executor->cmd_uwheel_sink);
    gtk_table_attach(GTK_TABLE(table), executor->cmd_uwheel_sink, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->cmd_uwheel_sink,
                                _("Specifies receiver of this command.\n"
                                  "If -1, command is executed in the default shell.\n"
                                  "If 0, command is sent to executor process (only continuous executors).\n"
                                  "Positive values are used for global command sinks (not implemented)."));

    row++, col = 2;
    label = gtk_label_new(_("Wheel scroll down command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->cmd_dwheel = gtk_entry_new();
    gtk_widget_show(executor->cmd_dwheel);
    gtk_entry_set_width_chars(GTK_ENTRY(executor->cmd_dwheel), 50);
    gtk_table_attach(GTK_TABLE(table), executor->cmd_dwheel, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->cmd_dwheel,
                                _("Specifies a command that will be executed when the executor receives a mouse scroll down."));

    label = gtk_label_new(_("Sink"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->cmd_dwheel_sink = gtk_spin_button_new_with_range(-1, 65535, 1);
    gtk_widget_show(executor->cmd_dwheel_sink);
    gtk_table_attach(GTK_TABLE(table), executor->cmd_dwheel_sink, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->cmd_dwheel_sink,
                                _("Specifies receiver of this command.\n"
                                  "If -1, command is executed in the default shell.\n"
                                  "If 0, command is sent to executor process (only continuous executors).\n"
                                  "Positive values are used for global command sinks (not implemented)."));

    change_paragraph(parent);

    label = gtk_label_new(_("<b>Appearance</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(3, 22, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("Background"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->bg = create_background_combo(_("Executor"));
    gtk_widget_show(executor->bg);
    gtk_table_attach(GTK_TABLE(table), executor->bg, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->bg,
                                _("Selects the background used to display the executor. "
                                  "Backgrounds can be edited in the Backgrounds tab."));

    row++, col = 2;
    label = gtk_label_new(_("Horizontal padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->padx = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(executor->padx);
    gtk_table_attach(GTK_TABLE(table), executor->padx, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->padx,
                                _("Specifies the horizontal padding of the executor. "
                                  "This is the space between the border and the content inside."));

    row++, col = 2;
    label = gtk_label_new(_("Vertical padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->pady = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(executor->pady);
    gtk_table_attach(GTK_TABLE(table), executor->pady, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->pady,
                                _("Specifies the vertical padding of the executor. "
                                  "This is the space between the border and the content inside."));

    row++, col = 2;
    label = gtk_label_new(_("Spacing"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->spacing = gtk_spin_button_new_with_range(-500, 500, 1);
    gtk_widget_show(executor->spacing);
    gtk_table_attach(GTK_TABLE(table), executor->spacing, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->spacing,
                                _("Specifies distance between icon and text."
                                  "Set to negative for overlap (text above)."));

    row++, col = 1;
    executor->font_use = gtk_check_button_new();
    gtk_widget_show(executor->font_use);
    gtk_table_attach(GTK_TABLE(table), executor->font_use, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_set_tooltip_text(executor->font_use,
                                _("If not checked, the desktop theme font is used. If checked, the custom font specified here "
                                  "is used."));
    col++;

    label = gtk_label_new(_("Font"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->font = gtk_font_button_new_with_font(get_default_font());
    gtk_widget_show(executor->font);
    gtk_table_attach(GTK_TABLE(table), executor->font, col, col + 3, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_font_button_set_show_style(GTK_FONT_BUTTON(executor->font), TRUE);
    g_signal_connect(G_OBJECT(executor->font_use),
                       "toggled",
                       G_CALLBACK(font_set_callback),
                       executor->font);
    font_set_callback(executor->font_use, executor->font);

    row++, col = 2;
    label = gtk_label_new(_("Font color"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->font_color = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(executor->font_color), TRUE);
    gtk_widget_show(executor->font_color);
    gtk_table_attach(GTK_TABLE(table), executor->font_color, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    row++, col = 2;
    label = gtk_label_new(_("Centered"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->centered = gtk_check_button_new();
    gtk_widget_show(executor->centered);
    gtk_table_attach(GTK_TABLE(table), executor->centered, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    row++, col = 2;
    label = gtk_label_new(_("Icon width"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->iw = gtk_spin_button_new_with_range(0, 1000000, 1);
    gtk_widget_show(executor->iw);
    gtk_table_attach(GTK_TABLE(table), executor->iw, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->iw, _("If non-zero, the image is resized to this width."));

    row++, col = 2;
    label = gtk_label_new(_("Icon height"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->ih = gtk_spin_button_new_with_range(0, 1000000, 1);
    gtk_widget_show(executor->ih);
    gtk_table_attach(GTK_TABLE(table), executor->ih, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->ih, _("If non-zero, the image is resized to this height."));

    row++, col = 2;
    label = gtk_label_new(_("Tooltip"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->show_tooltip = gtk_check_button_new();
    gtk_widget_show(executor->show_tooltip);
    gtk_table_attach(GTK_TABLE(table), executor->show_tooltip, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(executor->show_tooltip), 1);
    col++;

    row++, col = 2;
    label = gtk_label_new(_("Tooltip text"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    executor->tooltip = gtk_entry_new();
    gtk_widget_show(executor->tooltip);
    gtk_entry_set_width_chars(GTK_ENTRY(executor->tooltip), 50);
    gtk_table_attach(GTK_TABLE(table), executor->tooltip, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(executor->tooltip,
                                _("The tooltip text to display. Leave this empty to display an automatically generated "
                                  "tooltip with information about when the command was last executed."));

    change_paragraph(parent);
}

void create_button(GtkWidget *stack, int i) {
    GtkWidget *label;
    GtkWidget *table;
    int row, col;

    Button *button = &g_array_index(buttons, Button, i);

    STRBUF_AUTO_PRINTF (button->name, "%s %d", _("Button"), i + 1);
    button->page = gtk_box_new(GTK_ORIENTATION_VERTICAL, DEFAULT_HOR_SPACING);
    gtk_container_set_border_width(GTK_CONTAINER(button->page), 10);
    gtk_widget_show(button->page);
    gtk_stack_add_titled(GTK_STACK(stack), button->page, NULL, button->name);

    GtkWidget *parent = button->page;

    table = gtk_table_new(1, 2, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("<b>Format</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(3, 10, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("Icon"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    button->icon = gtk_entry_new();
    gtk_widget_show(button->icon);
    gtk_entry_set_width_chars(GTK_ENTRY(button->icon), 50);
    gtk_table_attach(GTK_TABLE(table), button->icon, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    row++, col = 2;
    label = gtk_label_new(_("Text"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    button->text = gtk_entry_new();
    gtk_widget_show(button->text);
    gtk_entry_set_width_chars(GTK_ENTRY(button->text), 50);
    gtk_table_attach(GTK_TABLE(table), button->text, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    row++, col = 2;
    label = gtk_label_new(_("Tooltip"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    button->tooltip = gtk_entry_new();
    gtk_widget_show(button->tooltip);
    gtk_entry_set_width_chars(GTK_ENTRY(button->tooltip), 50);
    gtk_table_attach(GTK_TABLE(table), button->tooltip, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    change_paragraph(parent);

    label = gtk_label_new(_("<b>Mouse events</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(5, 10, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("Left click command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    button->cmd_lclick = gtk_entry_new();
    gtk_widget_show(button->cmd_lclick);
    gtk_entry_set_width_chars(GTK_ENTRY(button->cmd_lclick), 50);
    gtk_table_attach(GTK_TABLE(table), button->cmd_lclick, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(button->cmd_lclick,
                                _("Specifies a command that will be executed when the button receives a left click."));

    row++, col = 2;
    label = gtk_label_new(_("Right click command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    button->cmd_rclick = gtk_entry_new();
    gtk_widget_show(button->cmd_rclick);
    gtk_entry_set_width_chars(GTK_ENTRY(button->cmd_rclick), 50);
    gtk_table_attach(GTK_TABLE(table), button->cmd_rclick, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(button->cmd_rclick,
                                _("Specifies a command that will be executed when the button receives a right click."));

    row++, col = 2;
    label = gtk_label_new(_("Middle click command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    button->cmd_mclick = gtk_entry_new();
    gtk_widget_show(button->cmd_mclick);
    gtk_entry_set_width_chars(GTK_ENTRY(button->cmd_mclick), 50);
    gtk_table_attach(GTK_TABLE(table), button->cmd_mclick, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(button->cmd_mclick,
                                _("Specifies a command that will be executed when the button receives a middle click."));

    row++, col = 2;
    label = gtk_label_new(_("Wheel scroll up command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    button->cmd_uwheel = gtk_entry_new();
    gtk_widget_show(button->cmd_uwheel);
    gtk_entry_set_width_chars(GTK_ENTRY(button->cmd_uwheel), 50);
    gtk_table_attach(GTK_TABLE(table), button->cmd_uwheel, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(button->cmd_uwheel,
                                _("Specifies a command that will be executed when the button receives a mouse scroll up."));

    row++, col = 2;
    label = gtk_label_new(_("Wheel scroll down command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    button->cmd_dwheel = gtk_entry_new();
    gtk_widget_show(button->cmd_dwheel);
    gtk_entry_set_width_chars(GTK_ENTRY(button->cmd_dwheel), 50);
    gtk_table_attach(GTK_TABLE(table), button->cmd_dwheel, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(button->cmd_dwheel,
                                _("Specifies a command that will be executed when the button receives a mouse scroll down."));

    change_paragraph(parent);

    label = gtk_label_new(_("<b>Appearance</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(3, 22, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("Background"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    button->bg = create_background_combo(_("Button"));
    gtk_widget_show(button->bg);
    gtk_table_attach(GTK_TABLE(table), button->bg, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(button->bg,
                                _("Selects the background used to display the button. "
                                  "Backgrounds can be edited in the Backgrounds tab."));

    row++, col = 2;
    label = gtk_label_new(_("Horizontal padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    button->padx = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(button->padx);
    gtk_table_attach(GTK_TABLE(table), button->padx, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(button->padx,
                                _("Specifies the horizontal padding of the button. "
                                  "This is the space between the border and the content inside."));

    row++, col = 2;
    label = gtk_label_new(_("Vertical padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    button->pady = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(button->pady);
    gtk_table_attach(GTK_TABLE(table), button->pady, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(button->pady,
                                _("Specifies the vertical padding of the button. "
                                  "This is the space between the border and the content inside."));

    row++, col = 2;
    label = gtk_label_new(_("Spacing"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    button->spacing = gtk_spin_button_new_with_range(-500, 500, 1);
    gtk_widget_show(button->spacing);
    gtk_table_attach(GTK_TABLE(table), button->spacing, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(button->spacing,
                                _("Specifies distance between icon and text."
                                  "Set to negative for overlap (text above)."));

    row++, col = 1;
    button->font_use = gtk_check_button_new();
    gtk_widget_show(button->font_use);
    gtk_table_attach(GTK_TABLE(table), button->font_use, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_set_tooltip_text(button->font_use,
                                _("If not checked, the desktop theme font is used. If checked, the custom font specified here "
                                  "is used."));
    col++;

    label = gtk_label_new(_("Font"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    button->font = gtk_font_button_new_with_font(get_default_font());
    gtk_widget_show(button->font);
    gtk_table_attach(GTK_TABLE(table), button->font, col, col + 3, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_font_button_set_show_style(GTK_FONT_BUTTON(button->font), TRUE);
    g_signal_connect(G_OBJECT(button->font_use),
                       "toggled",
                       G_CALLBACK(font_set_callback),
                       button->font);
    font_set_callback(button->font_use, button->font);

    row++, col = 2;
    label = gtk_label_new(_("Font color"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    button->font_color = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(button->font_color), TRUE);
    gtk_widget_show(button->font_color);
    gtk_table_attach(GTK_TABLE(table), button->font_color, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    row++, col = 2;
    label = gtk_label_new(_("Centered"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    button->centered = gtk_check_button_new();
    gtk_widget_show(button->centered);
    gtk_table_attach(GTK_TABLE(table), button->centered, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    row++, col = 2;
    label = gtk_label_new(_("Maximum icon size"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    button->max_icon_size = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(button->max_icon_size);
    gtk_table_attach(GTK_TABLE(table), button->max_icon_size, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    change_paragraph(parent);
}

void separator_create_new()
{
    g_array_set_size(separators, separators->len + 1);
    create_separator(stack, separators->len - 1);
}

void execp_create_new()
{
    g_array_set_size(executors, executors->len + 1);
    create_execp(stack, executors->len - 1);
}

void button_create_new()
{
    g_array_set_size(buttons, buttons->len + 1);
    create_button(stack, buttons->len - 1);
}

Separator *separator_get_last()
{
    if (separators->len <= 0)
        separator_create_new();
    return &g_array_index(separators, Separator, separators->len - 1);
}

Executor *execp_get_last()
{
    if (executors->len <= 0)
        execp_create_new();
    return &g_array_index(executors, Executor, executors->len - 1);
}

Button *button_get_last()
{
    if (buttons->len <= 0)
        button_create_new();
    return &g_array_index(buttons, Button, buttons->len - 1);
}

void separator_remove(int i)
{
    GtkWidget *page = g_array_index(separators, Separator, i).page;
    gtk_widget_hide(page);
    gtk_container_remove(GTK_CONTAINER(stack), page);
    separators = g_array_remove_index(separators, i);
}

void execp_remove(int i)
{
    GtkWidget *page = g_array_index(executors, Executor, i).page;
    gtk_widget_hide(page);
    gtk_container_remove(GTK_CONTAINER(stack), page);
    executors = g_array_remove_index(executors, i);
}

void button_remove(int i)
{
    GtkWidget *page = g_array_index(buttons, Button, i).page;
    gtk_widget_hide(page);
    gtk_container_remove(GTK_CONTAINER(stack), page);
    buttons = g_array_remove_index(buttons, i);
}

void separator_update_indices()
{
    GValue name_value = G_VALUE_INIT;
    g_value_init(&name_value, G_TYPE_STRING);
    for (int i = 0; i < separators->len; i++) {
        Separator *separator = &g_array_index(separators, Separator, i);
        snprintf(separator->name, strlen_const(separator->name), "%s %d", _("Separator"), i + 1);
        g_value_set_static_string(&name_value, separator->name);
        gtk_container_child_set_property(GTK_CONTAINER(stack), separator->page, "title", &name_value);
    }

    GtkTreeModel *model = GTK_TREE_MODEL(panel_items);
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter_first(model, &iter))
        return;
    int separator_index = -1;
    do {
        gchar *name;
        gchar *value;
        gtk_tree_model_get(model, &iter, itemsColName, &name, itemsColValue, &value, -1);

        if (g_str_equal(value, ":")) {
            separator_index++;
            char buffer[256];
            STRBUF_AUTO_PRINTF (buffer, "%s %d", _("Separator"), separator_index + 1);
            gtk_list_store_set(panel_items, &iter, itemsColName, buffer, -1);
        }
    } while (gtk_tree_model_iter_next(model, &iter));
}

void execp_update_indices()
{
    GValue name_value = G_VALUE_INIT;
    g_value_init(&name_value, G_TYPE_STRING);
    for (int i = 0; i < executors->len; i++) {
        Executor *executor = &g_array_index(executors, Executor, i);
        snprintf(executor->name, strlen_const(executor->name), "%s %d", _("Executor"), i + 1);
        g_value_set_static_string(&name_value, executor->name);
        gtk_container_child_set_property(GTK_CONTAINER(stack), executor->page, "title", &name_value);
    }

    GtkTreeModel *model = GTK_TREE_MODEL(panel_items);
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter_first(model, &iter))
        return;
    int execp_index = -1;
    do {
        gchar *name;
        gchar *value;
        gtk_tree_model_get(model, &iter, itemsColName, &name, itemsColValue, &value, -1);

        if (g_str_equal(value, "E")) {
            execp_index++;
            char buffer[256];
            STRBUF_AUTO_PRINTF (buffer, "%s %d", _("Executor"), execp_index + 1);
            gtk_list_store_set(panel_items, &iter, itemsColName, buffer, -1);
        }
    } while (gtk_tree_model_iter_next(model, &iter));
}

void button_update_indices()
{
    GValue name_value = G_VALUE_INIT;
    g_value_init(&name_value, G_TYPE_STRING);
    for (int i = 0; i < buttons->len; i++) {
        Button *button = &g_array_index(buttons, Button, i);
        snprintf(button->name, strlen_const(button->name), "%s %d", _("Button"), i + 1);
        g_value_set_static_string(&name_value, button->name);
        gtk_container_child_set_property(GTK_CONTAINER(stack), button->page, "title", &name_value);
    }

    GtkTreeModel *model = GTK_TREE_MODEL(panel_items);
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter_first(model, &iter))
        return;
    int button_index = -1;
    do {
        gchar *name;
        gchar *value;
        gtk_tree_model_get(model, &iter, itemsColName, &name, itemsColValue, &value, -1);

        if (g_str_equal(value, "P")) {
            button_index++;
            char buffer[256];
            STRBUF_AUTO_PRINTF (buffer, "%s %d", _("Button"), button_index + 1);
            gtk_list_store_set(panel_items, &iter, itemsColName, buffer, -1);
        }
    } while (gtk_tree_model_iter_next(model, &iter));
}

void init_systemtray_page (GtkWidget *parent)
{
    GtkWidget *table;
    GtkWidget *label;
    int row, col;

    label = gtk_label_new(_("<b>Options</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(2, 2, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0;
    col = 2;

    label = gtk_label_new(_("Icon ordering"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    systray_icon_order = gtk_combo_box_text_new();
    gtk_widget_show(systray_icon_order);
    gtk_table_attach(GTK_TABLE(table), systray_icon_order, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(systray_icon_order), _("Ascending"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(systray_icon_order), _("Descending"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(systray_icon_order), _("Left to right"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(systray_icon_order), _("Right to left"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(systray_icon_order), 0);
    gtk_widget_set_tooltip_text(systray_icon_order,
                                _("Specifies the order used to arrange the system tray icons. \n"
                                  "'Ascending' means that icons are sorted in ascending order of their window names. \n"
                                  "'Descending' means that icons are sorted in descending order of their window names. \n"
                                  "'Left to right' means that icons are always added to the right. \n"
                                  "'Right to left' means that icons are always added to the left."));

    row++;
    col = 2;
    label = gtk_label_new(_("Monitor"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    systray_monitor = gtk_combo_box_text_new();
    gtk_widget_show(systray_monitor);
    gtk_table_attach(GTK_TABLE(table), systray_monitor, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(systray_monitor), _("Primary"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(systray_monitor), _("1"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(systray_monitor), _("2"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(systray_monitor), _("3"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(systray_monitor), _("4"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(systray_monitor), _("5"));
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(systray_monitor), _("6"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(systray_monitor), 0);
    gtk_widget_set_tooltip_text(systray_monitor,
                                _("Specifies the monitor on which to place the system tray. "
                                  "Due to technical limitations, the system tray cannot be displayed on multiple monitors."));

    change_paragraph(parent);

    label = gtk_label_new(_("<b>Appearance</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(6, 10, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("Background"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    systray_background = create_background_combo(_("Systray"));
    gtk_widget_show(systray_background);
    gtk_table_attach(GTK_TABLE(table), systray_background, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(systray_background,
                                _("Selects the background used to display the system tray. "
                                  "Backgrounds can be edited in the Backgrounds tab."));

    row++;
    col = 2;
    label = gtk_label_new(_("Horizontal padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    systray_padding_x = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(systray_padding_x);
    gtk_table_attach(GTK_TABLE(table), systray_padding_x, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(systray_padding_x,
                                _("Specifies the horizontal padding of the system tray. "
                                  "This is the space between the border and the content inside."));

    row++;
    col = 2;
    label = gtk_label_new(_("Vertical padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    systray_padding_y = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(systray_padding_y);
    gtk_table_attach(GTK_TABLE(table), systray_padding_y, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(systray_padding_y,
                                _("Specifies the vertical padding of the system tray. "
                                  "This is the space between the border and the content inside."));

    row++;
    col = 2;
    label = gtk_label_new(_("Spacing"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    systray_spacing = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(systray_spacing);
    gtk_table_attach(GTK_TABLE(table), systray_spacing, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(systray_spacing, _("Specifies the spacing between system tray icons."));

    row++;
    col = 2;
    label = gtk_label_new(_("Icon size"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    systray_icon_size = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(systray_icon_size);
    gtk_table_attach(GTK_TABLE(table), systray_icon_size, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(systray_icon_size,
                                _("Specifies the size of the system tray icons, in pixels."));

    row++;
    col = 2;
    label = gtk_label_new(_("Icon opacity"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    systray_icon_opacity = gtk_spin_button_new_with_range(0, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(systray_icon_opacity), 100);
    gtk_widget_show(systray_icon_opacity);
    gtk_table_attach(GTK_TABLE(table), systray_icon_opacity, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(systray_icon_opacity,
                                _("Specifies the opacity of the system tray icons, in percent."));

    row++;
    col = 2;
    label = gtk_label_new(_("Icon saturation"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    systray_icon_saturation = gtk_spin_button_new_with_range(-100, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(systray_icon_saturation), 0);
    gtk_widget_show(systray_icon_saturation);
    gtk_table_attach(GTK_TABLE(table), systray_icon_saturation, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(systray_icon_saturation,
                                _("Specifies the saturation adjustment of the system tray icons, in percent."));

    row++;
    col = 2;
    label = gtk_label_new(_("Icon brightness"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    systray_icon_brightness = gtk_spin_button_new_with_range(-100, 100, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(systray_icon_brightness), 0);
    gtk_widget_show(systray_icon_brightness);
    gtk_table_attach(GTK_TABLE(table), systray_icon_brightness, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(systray_icon_brightness,
                                _("Specifies the brightness adjustment of the system tray icons, in percent."));

    row++, col = 2;
    label = gtk_label_new(_("Name filter"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    systray_name_filter = gtk_entry_new();
    gtk_widget_show(systray_name_filter);
    gtk_entry_set_width_chars(GTK_ENTRY(systray_name_filter), 50);
    gtk_table_attach(GTK_TABLE(table), systray_name_filter, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
}

void init_battery_page (GtkWidget *parent)
{
    GtkWidget *table, *label;
    int row, col;

    table = gtk_table_new(1, 2, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("<b>Thresholds</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(2, 10, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("Hide if charge higher than"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    battery_hide_if_higher = gtk_spin_button_new_with_range(0, 101, 1);
    gtk_widget_show(battery_hide_if_higher);
    gtk_table_attach(GTK_TABLE(table), battery_hide_if_higher, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(battery_hide_if_higher,
                                _("Minimum battery level for which to hide the batter applet. Use 101 to always show the "
                                  "batter applet."));

    label = gtk_label_new(_("%"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    row++, col = 2;
    label = gtk_label_new(_("Alert if charge lower than"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    battery_alert_if_lower = gtk_spin_button_new_with_range(0, 100, 1);
    gtk_widget_show(battery_alert_if_lower);
    gtk_table_attach(GTK_TABLE(table), battery_alert_if_lower, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(battery_alert_if_lower, _("Battery level for which to display an alert."));

    label = gtk_label_new(_("%"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    row++, col = 2;
    label = gtk_label_new(_("Alert command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    battery_alert_cmd = gtk_entry_new();
    gtk_widget_show(battery_alert_cmd);
    gtk_entry_set_width_chars(GTK_ENTRY(battery_alert_cmd), 50);
    gtk_table_attach(GTK_TABLE(table), battery_alert_cmd, col, col + 3, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(battery_alert_cmd,
                                _("Command to be executed when the alert threshold is reached."));

    row++, col = 2;
    label = gtk_label_new(_("Battery full command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    battery_alert_full_cmd = gtk_entry_new();
    gtk_widget_show(battery_alert_full_cmd);
    gtk_entry_set_width_chars(GTK_ENTRY(battery_alert_full_cmd), 50);
    gtk_table_attach(GTK_TABLE(table), battery_alert_full_cmd, col, col + 3, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    change_paragraph(parent);

    label = gtk_label_new(_("<b>AC connection events</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(2, 10, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

    row = 0, col = 2;
    label = gtk_label_new(_("AC connected command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    ac_connected_cmd = gtk_entry_new();
    gtk_widget_show(ac_connected_cmd);
    gtk_entry_set_width_chars(GTK_ENTRY(ac_connected_cmd), 50);
    gtk_table_attach(GTK_TABLE(table), ac_connected_cmd, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(ac_connected_cmd,
                                _("Specifies a command that will be executed when AC is connected to the system."));

    row++, col = 2;
    label = gtk_label_new(_("AC disconnected command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    ac_disconnected_cmd = gtk_entry_new();
    gtk_widget_show(ac_disconnected_cmd);
    gtk_entry_set_width_chars(GTK_ENTRY(ac_disconnected_cmd), 50);
    gtk_table_attach(GTK_TABLE(table), ac_disconnected_cmd, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(ac_disconnected_cmd,
                                _("Specifies a command that will be executed when AC is disconnected to the system."));

    change_paragraph(parent);

    label = gtk_label_new(_("<b>Mouse events</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(5, 10, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("Tooltips"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    battery_tooltip = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(battery_tooltip), 1);
    gtk_widget_show(battery_tooltip);
    gtk_table_attach(GTK_TABLE(table), battery_tooltip, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(battery_tooltip,
                                _("If enabled, shows a tooltip with detailed battery information when the mouse is moved over "
                                  "the battery widget."));

    row++, col = 2;
    label = gtk_label_new(_("Left click command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    battery_left_command = gtk_entry_new();
    gtk_widget_show(battery_left_command);
    gtk_entry_set_width_chars(GTK_ENTRY(battery_left_command), 50);
    gtk_table_attach(GTK_TABLE(table), battery_left_command, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(battery_left_command,
                                _("Specifies a command that will be executed when the battery receives a left click."));

    row++, col = 2;
    label = gtk_label_new(_("Right click command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    battery_right_command = gtk_entry_new();
    gtk_widget_show(battery_right_command);
    gtk_entry_set_width_chars(GTK_ENTRY(battery_right_command), 50);
    gtk_table_attach(GTK_TABLE(table), battery_right_command, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(battery_right_command,
                                _("Specifies a command that will be executed when the battery receives a right click."));

    row++, col = 2;
    label = gtk_label_new(_("Middle click command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    battery_mclick_command = gtk_entry_new();
    gtk_widget_show(battery_mclick_command);
    gtk_entry_set_width_chars(GTK_ENTRY(battery_mclick_command), 50);
    gtk_table_attach(GTK_TABLE(table), battery_mclick_command, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(battery_mclick_command,
                                _("Specifies a command that will be executed when the battery receives a middle click."));

    row++, col = 2;
    label = gtk_label_new(_("Wheel scroll up command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    battery_uwheel_command = gtk_entry_new();
    gtk_widget_show(battery_uwheel_command);
    gtk_entry_set_width_chars(GTK_ENTRY(battery_uwheel_command), 50);
    gtk_table_attach(GTK_TABLE(table), battery_uwheel_command, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(battery_uwheel_command,
                                _("Specifies a command that will be executed when the battery receives a mouse scroll up."));

    row++, col = 2;
    label = gtk_label_new(_("Wheel scroll down command"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    battery_dwheel_command = gtk_entry_new();
    gtk_widget_show(battery_dwheel_command);
    gtk_entry_set_width_chars(GTK_ENTRY(battery_dwheel_command), 50);
    gtk_table_attach(GTK_TABLE(table), battery_dwheel_command, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(battery_dwheel_command,
                                _("Specifies a command that will be executed when the battery receives a mouse scroll down."));

    change_paragraph(parent);

    label = gtk_label_new(_("<b>Appearance</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(4, 22, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("Background"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    battery_background = create_background_combo(_("Battery"));
    gtk_widget_show(battery_background);
    gtk_table_attach(GTK_TABLE(table), battery_background, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(battery_background,
                                _("Selects the background used to display the battery. "
                                  "Backgrounds can be edited in the Backgrounds tab."));

    row++, col = 2;
    label = gtk_label_new(_("Horizontal padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    battery_padding_x = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(battery_padding_x);
    gtk_table_attach(GTK_TABLE(table), battery_padding_x, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(battery_padding_x,
                                _("Specifies the horizontal padding of the battery. "
                                  "This is the space between the border and the content inside."));

    row++, col = 2;
    label = gtk_label_new(_("Vertical padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    battery_padding_y = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(battery_padding_y);
    gtk_table_attach(GTK_TABLE(table), battery_padding_y, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(battery_padding_y,
                                _("Specifies the vertical padding of the battery. "
                                  "This is the space between the border and the content inside."));

    row++, col = 1;
    battery_font_line1_set = gtk_check_button_new();
    gtk_widget_show(battery_font_line1_set);
    gtk_table_attach(GTK_TABLE(table), battery_font_line1_set, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_set_tooltip_text(battery_font_line1_set,
                                _("If not checked, the desktop theme font is used. If checked, the custom font specified here "
                                  "is used."));
    col++;

    label = gtk_label_new(_("Font first line"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    PangoFontDescription *bat1_font_desc = pango_font_description_from_string(get_default_font());
    pango_font_description_set_size(bat1_font_desc, pango_font_description_get_size(bat1_font_desc) - PANGO_SCALE);
    battery_font_line1 = gtk_font_button_new_with_font(pango_font_description_to_string(bat1_font_desc));
    gtk_widget_show(battery_font_line1);
    pango_font_description_free(bat1_font_desc);
    gtk_table_attach(GTK_TABLE(table), battery_font_line1, col, col + 3, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_font_button_set_show_style(GTK_FONT_BUTTON(battery_font_line1), TRUE);
    gtk_widget_set_tooltip_text(battery_font_line1,
                                _("Specifies the font used to display the first line of the battery text."));
    g_signal_connect(G_OBJECT(battery_font_line1_set),
                       "toggled",
                       G_CALLBACK(font_set_callback),
                       battery_font_line1);
    font_set_callback(battery_font_line1_set, battery_font_line1);

    row++, col = 1;
    battery_font_line2_set = gtk_check_button_new();
    gtk_widget_show(battery_font_line2_set);
    gtk_table_attach(GTK_TABLE(table), battery_font_line2_set, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_set_tooltip_text(battery_font_line2_set,
                                _("If not checked, the desktop theme font is used. If checked, the custom font specified here "
                                  "is used."));
    col++;

    label = gtk_label_new(_("Font second line"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    PangoFontDescription *bat2_font_desc = pango_font_description_from_string(get_default_font());
    pango_font_description_set_size(bat2_font_desc, pango_font_description_get_size(bat2_font_desc) - PANGO_SCALE);
    battery_font_line2 = gtk_font_button_new_with_font(pango_font_description_to_string(bat2_font_desc));
    pango_font_description_free(bat2_font_desc);
    gtk_widget_show(battery_font_line2);
    gtk_table_attach(GTK_TABLE(table), battery_font_line2, col, col + 3, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_font_button_set_show_style(GTK_FONT_BUTTON(battery_font_line2), TRUE);
    gtk_widget_set_tooltip_text(battery_font_line2,
                                _("Specifies the font used to display the second line of the battery text."));
    g_signal_connect(G_OBJECT(battery_font_line2_set),
                       "toggled",
                       G_CALLBACK(font_set_callback),
                       battery_font_line2);
    font_set_callback(battery_font_line2_set, battery_font_line2);

    row++, col = 2;
    label = gtk_label_new(_("Font color"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    battery_font_color = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(battery_font_color), TRUE);
    gtk_widget_show(battery_font_color);
    gtk_table_attach(GTK_TABLE(table), battery_font_color, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(battery_font_color,
                                _("Specifies the font clor used to display the battery text."));

    row++, col = 2;
    label = gtk_label_new(_("First line format"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    battery_format1 = gtk_entry_new();
    gtk_widget_show(battery_format1);
    gtk_entry_set_width_chars(GTK_ENTRY(battery_format1), 16);
    gtk_table_attach(GTK_TABLE(table), battery_format1, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    char *bat_format_spec = "Format specification:\n"
                            " %s: State (charging, discharging, full, unknown).\n"
                            " %m: Minutes left until completely charged/discharged (estimated).\n"
                            " %h: Hours left until completely charged/discharged (estimated).\n"
                            " %t: Time left. Shows \"hrs:mins\" when charging/discharging, or \"Full\".\n"
                            " %p: Percentage. Includes the % sign.\n"
                            " %p: Percentage. Without the % sign.";
    gtk_widget_set_tooltip_text(battery_format1,
                                _(bat_format_spec));

    row++, col = 2;
    label = gtk_label_new(_("Second line format"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    battery_format2 = gtk_entry_new();
    gtk_widget_show(battery_format2);
    gtk_entry_set_width_chars(GTK_ENTRY(battery_format2), 16);
    gtk_table_attach(GTK_TABLE(table), battery_format2, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(battery_format2,
                                _(bat_format_spec));

    change_paragraph(parent);
}

void init_tooltip_page (GtkWidget *parent)
{
    GtkWidget *table;
    GtkWidget *label;
    int row, col;

    label = gtk_label_new(_("<b>Timing</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(2, 22, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("Show delay"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    tooltip_show_after = gtk_spin_button_new_with_range(0, 10000, 0.1);
    gtk_widget_show(tooltip_show_after);
    gtk_table_attach(GTK_TABLE(table), tooltip_show_after, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(tooltip_show_after,
                         _("Specifies a delay after which to show the tooltip when moving the mouse over an element."));

    label = gtk_label_new(_("seconds"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    row++, col = 2;
    label = gtk_label_new(_("Hide delay"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    tooltip_hide_after = gtk_spin_button_new_with_range(0, 10000, 0.1);
    gtk_widget_show(tooltip_hide_after);
    gtk_table_attach(GTK_TABLE(table), tooltip_hide_after, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_set_tooltip_text(tooltip_hide_after,
                         _("Specifies a delay after which to hide the tooltip when moving the mouse outside an "
                           "element."));
    col++;

    label = gtk_label_new(_("seconds"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    change_paragraph(parent);

    label = gtk_label_new(_("<b>Appearance</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    table = gtk_table_new(3, 10, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);
    row = 0, col = 2;

    label = gtk_label_new(_("Background"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    tooltip_background = create_background_combo(_("Tooltip"));
    gtk_widget_show(tooltip_background);
    gtk_table_attach(GTK_TABLE(table), tooltip_background, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(tooltip_background,
                         _("Selects the background used to display the tooltip. "
                           "Backgrounds can be edited in the Backgrounds tab."));

    row++, col = 2;
    label = gtk_label_new(_("Horizontal padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    tooltip_padding_x = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(tooltip_padding_x);
    gtk_table_attach(GTK_TABLE(table), tooltip_padding_x, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(tooltip_padding_x,
                         _("Specifies the horizontal padding of the tooltip. "
                           "This is the space between the border and the content inside."));

    row++, col = 2;
    label = gtk_label_new(_("Vertical padding"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    tooltip_padding_y = gtk_spin_button_new_with_range(0, 500, 1);
    gtk_widget_show(tooltip_padding_y);
    gtk_table_attach(GTK_TABLE(table), tooltip_padding_y, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(tooltip_padding_y,
                         _("Specifies the vertical padding of the tooltip. "
                           "This is the space between the border and the content inside."));

    row++, col = 1;
    tooltip_font_set = gtk_check_button_new();
    gtk_widget_show(tooltip_font_set);
    gtk_table_attach(GTK_TABLE(table), tooltip_font_set, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    gtk_widget_set_tooltip_text(tooltip_font_set, _("If not checked, the desktop theme font is used. "
                                               "If checked, the custom font specified here is used."));
    col++;

    label = gtk_label_new(_("Font"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    tooltip_font = gtk_font_button_new_with_font(get_default_font());
    gtk_widget_show(tooltip_font);
    gtk_table_attach(GTK_TABLE(table), tooltip_font, col, col + 3, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_font_button_set_show_style(GTK_FONT_BUTTON(tooltip_font), TRUE);
    gtk_widget_set_tooltip_text(tooltip_font, _("Specifies the font used to display the text of the tooltip."));
    g_signal_connect(G_OBJECT(tooltip_font_set), "toggled", G_CALLBACK(font_set_callback), tooltip_font);
    font_set_callback(tooltip_font_set, tooltip_font);

    row++, col = 2;
    label = gtk_label_new(_("Font color"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    tooltip_font_color = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(tooltip_font_color), TRUE);
    gtk_widget_show(tooltip_font_color);
    gtk_table_attach(GTK_TABLE(table), tooltip_font_color, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(tooltip_font_color, _("Specifies the font color used to display the text of the tooltip."));

    change_paragraph(parent);
}

static GtkWidget *please_wait_dialog = NULL;
void create_please_wait(GtkWindow *parent)
{
    if (please_wait_dialog)
        return;

    please_wait_dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(please_wait_dialog), "Center");
    gtk_window_set_default_size(GTK_WINDOW(please_wait_dialog), 300, 150);
    gtk_window_set_position(GTK_WINDOW(please_wait_dialog), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(please_wait_dialog), 15);
    gtk_window_set_title(GTK_WINDOW(please_wait_dialog), _("Please wait..."));
    gtk_window_set_deletable(GTK_WINDOW(please_wait_dialog), FALSE);

    GtkWidget *label = gtk_label_new(_("Loading..."));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);

    GtkWidget *halign = gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_container_add(GTK_CONTAINER(halign), label);
    gtk_container_add(GTK_CONTAINER(please_wait_dialog), halign);

    gtk_widget_show_all(please_wait_dialog);
    gtk_window_set_modal(GTK_WINDOW(please_wait_dialog), TRUE);
    // gtk_window_set_keep_above(GTK_WINDOW(please_wait_dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(please_wait_dialog), parent);
}

void process_events()
{
    while (gtk_events_pending())
        gtk_main_iteration_do(FALSE);
}

void destroy_please_wait()
{
    if (!please_wait_dialog)
        return;
    gtk_widget_destroy(please_wait_dialog);
    please_wait_dialog = NULL;
}
