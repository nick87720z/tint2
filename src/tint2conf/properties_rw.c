#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>

#include "common.h"
#include "properties.h"
#include "properties_rw.h"
#include "gradient_gui.h"
#include "../util/color.h"
#include "config-keys.h"

void finalize_gradient();
void finalize_bg();
void add_entry(char *key, char *value);
void set_action(char *event, GtkWidget *combo);
char *get_action(GtkWidget *combo);

int config_has_panel_items;
int config_has_battery;
int config_has_systray;
int config_battery_enabled;
int config_systray_enabled;
int no_items_clock_enabled;
int no_items_systray_enabled;
int no_items_battery_enabled;

static int num_bg;
static int read_bg_color_hover;
static int read_border_color_hover;
static int read_bg_color_press;
static int read_border_color_press;

static int num_gr;

extern void background_page_finalize ();

void config_read_file(const char *path)
{
    num_bg = 0;
    background_create_new();
    gradient_create_new(GRADIENT_CONFIG_VERTICAL);

    config_has_panel_items = 0;
    config_has_battery = 0;
    config_battery_enabled = 0;
    config_has_systray = 0;
    config_systray_enabled = 0;
    no_items_clock_enabled = 0;
    no_items_systray_enabled = 0;
    no_items_battery_enabled = 0;

    FILE *fp = fopen(path, "r");
    if (fp) {
        char *line = NULL;
        size_t line_size = 0;
        while (getline(&line, &line_size, fp) >= 0) {
            char *key, *value;
            if (parse_line(line, &key, &value) & PARSED_KEY)
                add_entry(key, value);
        }
        free(line);
        fclose(fp);
    }

    finalize_gradient();
    finalize_bg();
    background_page_finalize ();
    gradient_page_finalize ();

    if (!config_has_panel_items) {
        char panel_items[256];
        panel_items[0] = '\0';

        strlcat(panel_items, "T", sizeof(panel_items));
        
        if (config_has_battery ? config_battery_enabled : no_items_battery_enabled)
            strlcat(panel_items, "B", sizeof(panel_items));
        
        if (config_has_systray ? config_systray_enabled : no_items_systray_enabled)
            strlcat(panel_items, "S", sizeof(panel_items));
        
        if (no_items_clock_enabled)
            strlcat(panel_items, "C", sizeof(panel_items));

        set_panel_items(panel_items);
    }
}

void config_write_color(FILE *fp, const char *name, GdkRGBA *color)
{
    double v;
    int p = (v = color->red   * 65535 /256 /16, v==floor(v)) &&
            (v = color->green * 65535 /256 /16, v==floor(v)) &&
            (v = color->blue  * 65535 /256 /16, v==floor(v)) ? 1 :
            (v = color->red   * 65535 /256, v==floor(v)) &&
            (v = color->green * 65535 /256, v==floor(v)) &&
            (v = color->blue  * 65535 /256, v==floor(v)) ? 2 : 4;
    switch (p) {
        case 1: fprintf(fp, "%s = #%1x%1x%1x %d\n", name,
                        (int)(65535 * color->red   / 256 / 16),
                        (int)(65535 * color->green / 256 / 16),
                        (int)(65535 * color->blue  / 256 / 16),
                        (int)(100 * color->alpha));
                break;
        case 2: fprintf(fp, "%s = #%02x%02x%02x %d\n", name,
                        (int)(65535 * color->red   / 256),
                        (int)(65535 * color->green / 256),
                        (int)(65535 * color->blue  / 256),
                        (int)(100 * color->alpha));
                break;
        case 4: fprintf(fp, "%s = #%04x%04x%04x %d\n", name,
                        (int)(65535 * color->red  ),
                        (int)(65535 * color->green),
                        (int)(65535 * color->blue ),
                        (int)(100   * color->alpha));
                break;
    }
}

void config_write_gradients(FILE *fp)
{
    fprintf(fp, "#-------------------------------------\n");
    fprintf(fp, "# Gradients\n");

    int index = 1;

    for (GList *gl = gradients ? gradients->next : NULL; gl; gl = gl->next, index++) {
        GradientConfig *g = (GradientConfig *)gl->data;
        GdkRGBA color;

        fprintf(fp, "# Gradient %d\n", index);
        fprintf(fp,
                "gradient = %s\n",
                g->type == GRADIENT_CONFIG_HORIZONTAL ? "horizontal" : g->type == GRADIENT_CONFIG_VERTICAL ? "vertical"
                                                                                                           : "radial");

        cairoColor2GdkRGBA(&g->start_color.color, &color);
        config_write_color(fp, "start_color", &color);

        cairoColor2GdkRGBA(&g->end_color.color, &color);
        config_write_color(fp, "end_color", &color);

        for (GList *l = g->extra_color_stops; l; l = l->next) {
            GradientConfigColorStop *stop = (GradientConfigColorStop *)l->data;
            // color_stop = percentage #rrggbb opacity
            cairoColor2GdkRGBA(&stop->color, &color);
            fprintf(fp,
                    "color_stop = %f #%02x%02x%02x %d\n",
                    stop->offset * 100,
                    (int)(0xff * color.red),
                    (int)(0xff * color.green),
                    (int)(0xff * color.blue),
                    (int)(color.alpha * 100));
        }
        fprintf(fp, "\n");
    }
}

void config_write_backgrounds(FILE *fp)
{
    fprintf(fp, "#-------------------------------------\n");
    fprintf(fp, "# Backgrounds\n");

    int index;
    for (index = 1;; index++) {
        GtkTreePath *path;
        GtkTreeIter iter;

        path = gtk_tree_path_new_from_indices(index, -1);
        gboolean found = gtk_tree_model_get_iter(GTK_TREE_MODEL(backgrounds), &iter, path);
        gtk_tree_path_free(path);

        if (!found) {
            break;
        }

        int r;
        int b;
        double fill_weight;
        double border_weight;
        gboolean sideTop;
        gboolean sideBottom;
        gboolean sideLeft;
        gboolean sideRight;
        gboolean roundTL;
        gboolean roundTR;
        gboolean roundBL;
        gboolean roundBR;
        GdkRGBA *fillColor;
        GdkRGBA *borderColor;
        int gradient_id;
        GdkRGBA *fillColorOver;
        GdkRGBA *borderColorOver;
        int gradient_id_over;
        GdkRGBA *fillColorPress;
        GdkRGBA *borderColorPress;
        int gradient_id_press;
        gchar *text;

        gtk_tree_model_get(GTK_TREE_MODEL(backgrounds),
                           &iter,
                           bgColFillColor,          &fillColor,
                           bgColBorderColor,        &borderColor,
                           bgColGradientId,         &gradient_id,
                           bgColFillColorOver,      &fillColorOver,
                           bgColBorderColorOver,    &borderColorOver,
                           bgColGradientIdOver,     &gradient_id_over,
                           bgColFillColorPress,     &fillColorPress,
                           bgColBorderColorPress,   &borderColorPress,
                           bgColGradientIdPress,    &gradient_id_press,
                           bgColBorderWidth,        &b,
                           bgColCornerRadius,       &r,
                           bgColText,               &text,
                           bgColBorderSidesTop,     &sideTop,
                           bgColBorderSidesBottom,  &sideBottom,
                           bgColBorderSidesLeft,    &sideLeft,
                           bgColBorderSidesRight,   &sideRight,
                           bgColCornerRoundTL,      &roundTL,
                           bgColCornerRoundTR,      &roundTR,
                           bgColCornerRoundBL,      &roundBL,
                           bgColCornerRoundBR,      &roundBR,
                           bgColFillWeight,         &fill_weight,
                           bgColBorderWeight,       &border_weight,
                           -1);
        fprintf(fp, "# Background %d: %s\n", index, text ? text : "");
        fprintf(fp, "rounded = %d\n", r);
        char corners[13]; {
            char *p = corners;
            if (roundTL) memcpy(p, "TL ", 3), p+=3;
            if (roundTR) memcpy(p, "TR ", 3), p+=3;
            if (roundBL) memcpy(p, "BL ", 3), p+=3;
            if (roundBR) memcpy(p, "BR ", 3), p+=3;
            *(p == corners ? p : p-1) = '\0';
        }
        fprintf(fp, "rounded_corners = %s\n", corners);
        fprintf(fp, "border_width = %d\n", b);
        char sides[5]; {
            char *p = sides;
            if (sideTop)    *p++ = 'T';
            if (sideBottom) *p++ = 'B';
            if (sideLeft)   *p++ = 'L';
            if (sideRight)  *p++ = 'R';
            *p = '\0';
        }
        fprintf(fp, "border_sides = %s\n", sides);

        fprintf(fp, "border_content_tint_weight = %d\n", (int)(border_weight));
        fprintf(fp, "background_content_tint_weight = %d\n", (int)(fill_weight));

        config_write_color(fp, "background_color", fillColor);
        config_write_color(fp, "border_color", borderColor);
        if (gradient_id >= 0)
            fprintf(fp, "gradient_id = %d\n", gradient_id);
        config_write_color(fp, "background_color_hover", fillColorOver);
        config_write_color(fp, "border_color_hover", borderColorOver);
        if (gradient_id_over >= 0)
            fprintf(fp, "gradient_id_hover = %d\n", gradient_id_over);
        config_write_color(fp, "background_color_pressed", fillColorPress);
        config_write_color(fp, "border_color_pressed", borderColorPress);
        if (gradient_id_press >= 0)
            fprintf(fp, "gradient_id_pressed = %d\n", gradient_id_press);
        fprintf(fp, "\n");
    }
}

void config_write_panel(FILE *fp)
{
    fprintf(fp, "#-------------------------------------\n");
    fprintf(fp, "# Panel\n");
    char *items = get_panel_items();
    fprintf(fp, "panel_items = %s\n", items);
    free(items);
    fprintf(fp,
            "panel_size = %d%s %d%s\n",
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(panel_width)),
            gtk_combo_box_get_active(GTK_COMBO_BOX(panel_combo_width_type)) == 0 ? "%" : "",
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(panel_height)),
            gtk_combo_box_get_active(GTK_COMBO_BOX(panel_combo_height_type)) == 0 ? "%" : "");
    fprintf(fp,
            "panel_margin = %d %d\n",
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(panel_margin_x)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(panel_margin_y)));
    fprintf(fp,
            "panel_padding = %d %d %d\n",
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(panel_padding_x)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(panel_padding_y)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(panel_spacing)));
    fprintf(fp, "panel_background_id = %d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(panel_background)));
    fprintf(fp, "wm_menu = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(panel_wm_menu)) ? 1 : 0);
    fprintf(fp, "panel_dock = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(panel_dock)) ? 1 : 0);
    fprintf(fp, "panel_pivot_struts = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(panel_pivot_struts)) ? 1 : 0);

    fprintf(fp, "panel_position = ");
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_BLH]))) {
        fprintf(fp, "bottom left horizontal");
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_BCH]))) {
        fprintf(fp, "bottom center horizontal");
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_BRH]))) {
        fprintf(fp, "bottom right horizontal");
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_TLH]))) {
        fprintf(fp, "top left horizontal");
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_TCH]))) {
        fprintf(fp, "top center horizontal");
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_TRH]))) {
        fprintf(fp, "top right horizontal");
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_TLV]))) {
        fprintf(fp, "top left vertical");
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_CLV]))) {
        fprintf(fp, "center left vertical");
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_BLV]))) {
        fprintf(fp, "bottom left vertical");
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_TRV]))) {
        fprintf(fp, "top right vertical");
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_CRV]))) {
        fprintf(fp, "center right vertical");
    } else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(screen_position[POS_BRV]))) {
        fprintf(fp, "bottom right vertical");
    }
    fprintf(fp, "\n");

    fprintf(fp, "panel_layer = ");
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(panel_combo_layer)) == 0) {
        fprintf(fp, "top");
    } else if (gtk_combo_box_get_active(GTK_COMBO_BOX(panel_combo_layer)) == 1) {
        fprintf(fp, "normal");
    } else {
        fprintf(fp, "bottom");
    }
    fprintf(fp, "\n");

    fprintf(fp, "panel_monitor = ");
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(panel_combo_monitor)) <= 0) {
        fprintf(fp, "all");
    } else if (gtk_combo_box_get_active(GTK_COMBO_BOX(panel_combo_monitor)) == 1) {
        fprintf(fp, "primary");
    } else {
        fprintf(fp, "%d", gtk_combo_box_get_active(GTK_COMBO_BOX(panel_combo_monitor)) - 1);
    }
    fprintf(fp, "\n");

    fprintf(fp, "panel_shrink = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(panel_shrink)) ? 1 : 0);

    fprintf(fp, "autohide = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(panel_autohide)) ? 1 : 0);
    fprintf(fp, "autohide_show_timeout = %g\n", gtk_spin_button_get_value(GTK_SPIN_BUTTON(panel_autohide_show_time)));
    fprintf(fp, "autohide_hide_timeout = %g\n", gtk_spin_button_get_value(GTK_SPIN_BUTTON(panel_autohide_hide_time)));
    fprintf(fp, "autohide_height = %d\n", (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(panel_autohide_size)));

    fprintf(fp, "strut_policy = ");
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(panel_combo_strut_policy)) == 0) {
        fprintf(fp, "follow_size");
    } else if (gtk_combo_box_get_active(GTK_COMBO_BOX(panel_combo_strut_policy)) == 1) {
        fprintf(fp, "minimum");
    } else if (gtk_combo_box_get_active(GTK_COMBO_BOX(panel_combo_strut_policy)) == 2) {
        fprintf(fp, "none");
    }
    fprintf(fp, "\n");

    fprintf(fp, "panel_window_name = %s\n", gtk_entry_get_text(GTK_ENTRY(panel_window_name)));
    fprintf(fp,
            "disable_transparency = %d\n",
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(disable_transparency)) ? 1 : 0);
    fprintf(fp, "mouse_effects = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(panel_mouse_effects)) ? 1 : 0);
    fprintf(fp, "font_shadow = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(font_shadow)) ? 1 : 0);
    fprintf(fp,
            "mouse_hover_icon_asb = %d %d %d\n",
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(mouse_hover_icon_opacity)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(mouse_hover_icon_saturation)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(mouse_hover_icon_brightness)));
    fprintf(fp,
            "mouse_pressed_icon_asb = %d %d %d\n",
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(mouse_pressed_icon_opacity)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(mouse_pressed_icon_saturation)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(mouse_pressed_icon_brightness)));

    fprintf(fp,
            "scale_relative_to_dpi = %d\n",
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(scale_relative_to_dpi)));
    fprintf(fp,
            "scale_relative_to_screen_height = %d\n",
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(scale_relative_to_screen_height)));

    fprintf(fp, "\n");
}

void config_write_taskbar(FILE *fp)
{
    fprintf(fp, "#-------------------------------------\n");
    fprintf(fp, "# Taskbar\n");

    fprintf(fp,
            "taskbar_mode = %s\n",
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(taskbar_show_desktop)) ? "multi_desktop" : "single_desktop");
    fprintf(fp,
            "taskbar_hide_if_empty = %d\n",
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(taskbar_hide_empty)) ? 1 : 0);
    fprintf(fp,
            "taskbar_padding = %d %d %d\n",
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(taskbar_padding_x)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(taskbar_padding_y)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(taskbar_spacing)));
    fprintf(fp, "taskbar_background_id = %d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_inactive_background)));
    fprintf(fp,
            "taskbar_active_background_id = %d\n",
            gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_active_background)));
    fprintf(fp, "taskbar_name = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(taskbar_show_name)) ? 1 : 0);
    fprintf(fp,
            "taskbar_hide_inactive_tasks = %d\n",
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(taskbar_hide_inactive_tasks)) ? 1 : 0);
    fprintf(fp,
            "taskbar_hide_different_monitor = %d\n",
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(taskbar_hide_diff_monitor)) ? 1 : 0);
    fprintf(fp,
            "taskbar_hide_different_desktop = %d\n",
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(taskbar_hide_diff_desktop)) ? 1 : 0);
    fprintf(fp,
            "taskbar_always_show_all_desktop_tasks = %d\n",
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(taskbar_always_show_all_desktop_tasks)) ? 1 : 0);
    fprintf(fp,
            "taskbar_name_padding = %d %d\n",
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(taskbar_name_padding_x)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(taskbar_name_padding_y)));
    fprintf(fp,
            "taskbar_name_background_id = %d\n",
            gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_name_inactive_background)));
    fprintf(fp,
            "taskbar_name_active_background_id = %d\n",
            gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_name_active_background)));
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(taskbar_name_font_set)))
        fprintf(fp, "taskbar_name_font = %s\n", gtk_font_button_get_font_name(GTK_FONT_BUTTON(taskbar_name_font)));

    GdkRGBA color;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(taskbar_name_inactive_color), &color);
    config_write_color(fp, "taskbar_name_font_color", &color);

    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(taskbar_name_active_color), &color);
    config_write_color(fp, "taskbar_name_active_font_color", &color);

    fprintf(fp,
            "taskbar_distribute_size = %d\n",
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(taskbar_distribute_size)) ? 1 : 0);

    fprintf(fp, "taskbar_sort_order = ");
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_sort_order)) <= 0) {
        fprintf(fp, "none");
    } else if (gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_sort_order)) == 1) {
        fprintf(fp, "title");
    } else if (gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_sort_order)) == 2) {
        fprintf(fp, "application");
    } else if (gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_sort_order)) == 3) {
        fprintf(fp, "center");
    } else if (gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_sort_order)) == 4) {
        fprintf(fp, "mru");
    } else if (gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_sort_order)) == 5) {
        fprintf(fp, "lru");
    } else {
        fprintf(fp, "none");
    }
    fprintf(fp, "\n");

    fprintf(fp, "task_align = ");
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_alignment)) <= 0) {
        fprintf(fp, "left");
    } else if (gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_alignment)) == 1) {
        fprintf(fp, "center");
    } else {
        fprintf(fp, "right");
    }
    fprintf(fp, "\n");

    fprintf(fp, "\n");
}

void config_write_task_font_color(FILE *fp, char *name, GtkWidget *task_color)
{
    GdkRGBA color;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(task_color), &color);
    char full_name[128];
    STRBUF_AUTO_PRINTF (full_name, "task%s_font_color", name);
    config_write_color(fp, full_name, &color);
}

void config_write_task_icon_osb(FILE *fp,
                                char *name,
                                GtkWidget *widget_opacity,
                                GtkWidget *widget_saturation,
                                GtkWidget *widget_brightness)
{
    char full_name[128];
    STRBUF_AUTO_PRINTF (full_name, "task%s_icon_asb", name);
    fprintf(fp,
            "%s = %d %d %d\n",
            full_name,
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget_opacity)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget_saturation)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget_brightness)));
}

void config_write_task_background(FILE *fp, char *name, GtkWidget *task_background)
{
    char full_name[128];
    STRBUF_AUTO_PRINTF (full_name, "task%s_background_id", name);
    fprintf(fp, "%s = %d\n", full_name, gtk_combo_box_get_active(GTK_COMBO_BOX(task_background)));
}

void config_write_task(FILE *fp)
{
    fprintf(fp, "#-------------------------------------\n");
    fprintf(fp, "# Task\n");

    fprintf(fp, "task_text = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_show_text)) ? 1 : 0);
    fprintf(fp, "task_icon = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_show_icon)) ? 1 : 0);
    fprintf(fp, "task_centered = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_align_center)) ? 1 : 0);
    fprintf(fp, "urgent_nb_of_blink = %d\n", (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(task_urgent_blinks)));
    fprintf(fp,
            "task_maximum_size = %d %d\n",
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(task_maximum_width)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(task_maximum_height)));
    fprintf(fp,
            "task_padding = %d %d %d\n",
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(task_padding_x)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(task_padding_y)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(task_spacing)));
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_font_set)))
        fprintf(fp, "task_font = %s\n", gtk_font_button_get_font_name(GTK_FONT_BUTTON(task_font)));
    fprintf(fp, "task_tooltip = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tooltip_task_show)) ? 1 : 0);
    fprintf(fp, "task_thumbnail = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tooltip_task_thumbnail)) ? 1 : 0);
    fprintf(fp,
            "task_thumbnail_size = %d\n",
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(tooltip_task_thumbnail_size)));


    // same for: "" _normal _active _urgent _iconified
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_default_color_set))) {
        config_write_task_font_color(fp, "", task_default_color);
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_normal_color_set))) {
        config_write_task_font_color(fp, "_normal", task_normal_color);
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_active_color_set))) {
        config_write_task_font_color(fp, "_active", task_active_color);
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_urgent_color_set))) {
        config_write_task_font_color(fp, "_urgent", task_urgent_color);
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_iconified_color_set))) {
        config_write_task_font_color(fp, "_iconified", task_iconified_color);
    }

    // same for: "" _normal _active _urgent _iconified
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_default_icon_osb_set))) {
        config_write_task_icon_osb(fp,
                                   "",
                                   task_default_icon_opacity,
                                   task_default_icon_saturation,
                                   task_default_icon_brightness);
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_normal_icon_osb_set))) {
        config_write_task_icon_osb(fp,
                                   "_normal",
                                   task_normal_icon_opacity,
                                   task_normal_icon_saturation,
                                   task_normal_icon_brightness);
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_active_icon_osb_set))) {
        config_write_task_icon_osb(fp,
                                   "_active",
                                   task_active_icon_opacity,
                                   task_active_icon_saturation,
                                   task_active_icon_brightness);
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_urgent_icon_osb_set))) {
        config_write_task_icon_osb(fp,
                                   "_urgent",
                                   task_urgent_icon_opacity,
                                   task_urgent_icon_saturation,
                                   task_urgent_icon_brightness);
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_iconified_icon_osb_set))) {
        config_write_task_icon_osb(fp,
                                   "_iconified",
                                   task_iconified_icon_opacity,
                                   task_iconified_icon_saturation,
                                   task_iconified_icon_brightness);
    }

    // same for: "" _normal _active _urgent _iconified
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_default_background_set))) {
        config_write_task_background(fp, "", task_default_background);
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_normal_background_set))) {
        config_write_task_background(fp, "_normal", task_normal_background);
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_active_background_set))) {
        config_write_task_background(fp, "_active", task_active_background);
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_urgent_background_set))) {
        config_write_task_background(fp, "_urgent", task_urgent_background);
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(task_iconified_background_set))) {
        config_write_task_background(fp, "_iconified", task_iconified_background);
    }

    fprintf(fp, "mouse_left = %s\n", get_action(task_mouse_left));
    fprintf(fp, "mouse_middle = %s\n", get_action(task_mouse_middle));
    fprintf(fp, "mouse_right = %s\n", get_action(task_mouse_right));
    fprintf(fp, "mouse_scroll_up = %s\n", get_action(task_mouse_scroll_up));
    fprintf(fp, "mouse_scroll_down = %s\n", get_action(task_mouse_scroll_down));

    fprintf(fp, "\n");
}

void config_write_systray(FILE *fp)
{
    fprintf(fp, "#-------------------------------------\n");
    fprintf(fp, "# System tray (notification area)\n");

    fprintf(fp,
            "systray_padding = %d %d %d\n",
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(systray_padding_x)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(systray_padding_y)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(systray_spacing)));
    fprintf(fp, "systray_background_id = %d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(systray_background)));

    fprintf(fp, "systray_sort = ");
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(systray_icon_order)) == 0) {
        fprintf(fp, "ascending");
    } else if (gtk_combo_box_get_active(GTK_COMBO_BOX(systray_icon_order)) == 1) {
        fprintf(fp, "descending");
    } else if (gtk_combo_box_get_active(GTK_COMBO_BOX(systray_icon_order)) == 2) {
        fprintf(fp, "left2right");
    } else if (gtk_combo_box_get_active(GTK_COMBO_BOX(systray_icon_order)) == 3) {
        fprintf(fp, "right2left");
    }
    fprintf(fp, "\n");

    fprintf(fp, "systray_icon_size = %d\n", (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(systray_icon_size)));
    fprintf(fp,
            "systray_icon_asb = %d %d %d\n",
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(systray_icon_opacity)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(systray_icon_saturation)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(systray_icon_brightness)));

    fprintf(fp, "systray_monitor = ");
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(systray_monitor)) <= 0) {
        fprintf(fp, "primary");
    } else {
        fprintf(fp, "%d", MAX(1, gtk_combo_box_get_active(GTK_COMBO_BOX(systray_monitor))));
    }
    fprintf(fp, "\n");

    fprintf(fp, "systray_name_filter = %s\n", gtk_entry_get_text(GTK_ENTRY(systray_name_filter)));

    fprintf(fp, "\n");
}

void config_write_launcher(FILE *fp)
{
    fprintf(fp, "#-------------------------------------\n");
    fprintf(fp, "# Launcher\n");

    fprintf(fp,
            "launcher_padding = %d %d %d\n",
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(launcher_padding_x)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(launcher_padding_y)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(launcher_spacing)));
    fprintf(fp, "launcher_background_id = %d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(launcher_background)));
    fprintf(fp,
            "launcher_icon_background_id = %d\n",
            gtk_combo_box_get_active(GTK_COMBO_BOX(launcher_icon_background)));
    fprintf(fp, "launcher_icon_size = %d\n", (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(launcher_icon_size)));
    fprintf(fp,
            "launcher_icon_asb = %d %d %d\n",
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(launcher_icon_opacity)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(launcher_icon_saturation)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(launcher_icon_brightness)));
    gchar *icon_theme = get_current_icon_theme();
    if (icon_theme && !g_str_equal(icon_theme, "")) {
        fprintf(fp, "launcher_icon_theme = %s\n", icon_theme);
        g_free(icon_theme);
        icon_theme = NULL;
    }
    fprintf(fp,
            "launcher_icon_theme_override = %d\n",
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(launcher_icon_theme_override)) ? 1 : 0);
    fprintf(fp,
            "startup_notifications = %d\n",
            gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(startup_notifications)) ? 1 : 0);
    fprintf(fp, "launcher_tooltip = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(launcher_tooltip)) ? 1 : 0);

    int index;
    for (index = 0;; index++) {
        GtkTreePath *path;
        GtkTreeIter iter;

        path = gtk_tree_path_new_from_indices(index, -1);
        gboolean found = gtk_tree_model_get_iter(GTK_TREE_MODEL(launcher_apps), &iter, path);
        gtk_tree_path_free(path);

        if (!found) {
            break;
        }

        gchar *app_path;
        gtk_tree_model_get(GTK_TREE_MODEL(launcher_apps), &iter, appsColPath, &app_path, -1);
        char *contracted = contract_tilde(app_path);
        fprintf(fp, "launcher_item_app = %s\n", contracted);
        free(contracted);
        g_free(app_path);
    }

    gchar **app_dirs = g_strsplit(gtk_entry_get_text(GTK_ENTRY(launcher_apps_dirs)), ",", 0);
    for (index = 0; app_dirs[index]; index++) {
        gchar *dir = app_dirs[index];
        g_strstrip(dir);
        if (strlen(dir) > 0) {
            char *contracted = contract_tilde(dir);
            fprintf(fp, "launcher_apps_dir = %s\n", contracted);
            free(contracted);
        }
    }
    g_strfreev(app_dirs);

    fprintf(fp, "\n");
}

void config_write_clock(FILE *fp)
{
    fprintf(fp, "#-------------------------------------\n");
    fprintf(fp, "# Clock\n");

    fprintf(fp, "time1_format = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_format_line1)));
    fprintf(fp, "time2_format = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_format_line2)));
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(clock_font_line1_set)))
        fprintf(fp, "time1_font = %s\n", gtk_font_button_get_font_name(GTK_FONT_BUTTON(clock_font_line1)));
    fprintf(fp, "time1_timezone = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_tmz_line1)));
    fprintf(fp, "time2_timezone = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_tmz_line2)));
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(clock_font_line2_set)))
        fprintf(fp, "time2_font = %s\n", gtk_font_button_get_font_name(GTK_FONT_BUTTON(clock_font_line2)));

    GdkRGBA color;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(clock_font_color), &color);
    config_write_color(fp, "clock_font_color", &color);

    fprintf(fp,
            "clock_padding = %d %d\n",
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(clock_padding_x)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(clock_padding_y)));
    fprintf(fp, "clock_background_id = %d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(clock_background)));
    fprintf(fp, "clock_tooltip = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_format_tooltip)));
    fprintf(fp, "clock_tooltip_timezone = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_tmz_tooltip)));
    fprintf(fp, "clock_lclick_command = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_left_command)));
    fprintf(fp, "clock_rclick_command = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_right_command)));
    fprintf(fp, "clock_mclick_command = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_mclick_command)));
    fprintf(fp, "clock_uwheel_command = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_uwheel_command)));
    fprintf(fp, "clock_dwheel_command = %s\n", gtk_entry_get_text(GTK_ENTRY(clock_dwheel_command)));

    fprintf(fp, "\n");
}

void config_write_battery(FILE *fp)
{
    fprintf(fp, "#-------------------------------------\n");
    fprintf(fp, "# Battery\n");

    fprintf(fp, "battery_tooltip = %d\n", gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(battery_tooltip)) ? 1 : 0);
    fprintf(fp, "battery_low_status = %g\n", gtk_spin_button_get_value(GTK_SPIN_BUTTON(battery_alert_if_lower)));
    fprintf(fp, "battery_low_cmd = %s\n", gtk_entry_get_text(GTK_ENTRY(battery_alert_cmd)));
    fprintf(fp, "battery_full_cmd = %s\n", gtk_entry_get_text(GTK_ENTRY(battery_alert_full_cmd)));
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(battery_font_line1_set)))
        fprintf(fp, "bat1_font = %s\n", gtk_font_button_get_font_name(GTK_FONT_BUTTON(battery_font_line1)));
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(battery_font_line2_set)))
        fprintf(fp, "bat2_font = %s\n", gtk_font_button_get_font_name(GTK_FONT_BUTTON(battery_font_line2)));
    GdkRGBA color;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(battery_font_color), &color);
    config_write_color(fp, "battery_font_color", &color);
    fprintf(fp, "bat1_format = %s\n", gtk_entry_get_text(GTK_ENTRY(battery_format1)));
    fprintf(fp, "bat2_format = %s\n", gtk_entry_get_text(GTK_ENTRY(battery_format2)));
    fprintf(fp,
            "battery_padding = %d %d\n",
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(battery_padding_x)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(battery_padding_y)));
    fprintf(fp, "battery_background_id = %d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(battery_background)));
    fprintf(fp, "battery_hide = %d\n", (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(battery_hide_if_higher)));
    fprintf(fp, "battery_lclick_command = %s\n", gtk_entry_get_text(GTK_ENTRY(battery_left_command)));
    fprintf(fp, "battery_rclick_command = %s\n", gtk_entry_get_text(GTK_ENTRY(battery_right_command)));
    fprintf(fp, "battery_mclick_command = %s\n", gtk_entry_get_text(GTK_ENTRY(battery_mclick_command)));
    fprintf(fp, "battery_uwheel_command = %s\n", gtk_entry_get_text(GTK_ENTRY(battery_uwheel_command)));
    fprintf(fp, "battery_dwheel_command = %s\n", gtk_entry_get_text(GTK_ENTRY(battery_dwheel_command)));

    fprintf(fp, "ac_connected_cmd = %s\n", gtk_entry_get_text(GTK_ENTRY(ac_connected_cmd)));
    fprintf(fp, "ac_disconnected_cmd = %s\n", gtk_entry_get_text(GTK_ENTRY(ac_disconnected_cmd)));

    fprintf(fp, "\n");
}

void config_write_separator(FILE *fp)
{
    for (int i = 0; i < separators->len; i++) {
        fprintf(fp, "#-------------------------------------\n");
        fprintf(fp, "# Separator %d\n", i + 1);

        Separator *separator = &g_array_index(separators, Separator, i);

        fprintf(fp, "separator = new\n");
        fprintf(fp,
                "separator_background_id = %d\n",
                gtk_combo_box_get_active(GTK_COMBO_BOX(separator->bg)));
        GdkRGBA color;
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(separator->color), &color);
        config_write_color(fp, "separator_color", &color);
        // fprintf(fp, "separator_style = %d\n",
        // (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(separator->style)));
        fprintf(fp,
                "separator_style = %s\n",
                gtk_combo_box_get_active(GTK_COMBO_BOX(separator->style)) == 0
                    ? "empty"
                    : gtk_combo_box_get_active(GTK_COMBO_BOX(separator->style)) == 1 ? "line" : "dots");
        fprintf(fp,
                "separator_size = %d\n",
                (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(separator->size)));
        fprintf(fp,
                "separator_padding = %d %d\n",
                (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(separator->padx)),
                (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(separator->pady)));
        fprintf(fp, "\n");
    }
}

void config_write_execp(FILE *fp)
{
    for (int i = 0; i < executors->len; i++) {
        fprintf(fp, "#-------------------------------------\n");
        fprintf(fp, "# Executor %d\n", i + 1);

        Executor *executor = &g_array_index(executors, Executor, i);

        fprintf(fp, "execp = new\n");
        fprintf(fp, "execp_name = %s\n", gtk_entry_get_text(GTK_ENTRY(executor->id)));
        fprintf(fp, "execp_command = %s\n", gtk_entry_get_text(GTK_ENTRY(executor->cmd)));
        fprintf(fp, "execp_interval = %d\n", (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(executor->interval)));
        fprintf(fp,
                "execp_has_icon = %d\n",
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(executor->has_icon)) ? 1 : 0);
        fprintf(fp,
                "execp_cache_icon = %d\n",
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(executor->cache_icon)) ? 1 : 0);
        fprintf(fp,
                "execp_continuous = %d\n",
                (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(executor->cont)));
        fprintf(fp,
                "execp_markup = %d\n",
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(executor->markup)) ? 1 : 0);
        fprintf(fp, "execp_monitor = ");
        if (gtk_combo_box_get_active(GTK_COMBO_BOX(executor->mon)) <= 0) {
            fprintf(fp, "all");
        } else if (gtk_combo_box_get_active(GTK_COMBO_BOX(executor->mon)) == 1) {
            fprintf(fp, "primary");
        } else {
            fprintf(fp, "%d", MAX(1, gtk_combo_box_get_active(GTK_COMBO_BOX(executor->mon)) - 1));
        }
        fprintf(fp, "\n");

        if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(executor->show_tooltip))) {
            fprintf(fp, "execp_tooltip = \n");
        } else {
            const gchar *text = gtk_entry_get_text(GTK_ENTRY(executor->tooltip));
            if (strlen(text) > 0)
                fprintf(fp, "execp_tooltip = %s\n", text);
        }

        fprintf(fp, "execp_lclick_command = %s\n", gtk_entry_get_text(GTK_ENTRY(executor->cmd_lclick)));
        fprintf(fp, "execp_rclick_command = %s\n", gtk_entry_get_text(GTK_ENTRY(executor->cmd_rclick)));
        fprintf(fp, "execp_mclick_command = %s\n", gtk_entry_get_text(GTK_ENTRY(executor->cmd_mclick)));
        fprintf(fp, "execp_uwheel_command = %s\n", gtk_entry_get_text(GTK_ENTRY(executor->cmd_uwheel)));
        fprintf(fp, "execp_dwheel_command = %s\n", gtk_entry_get_text(GTK_ENTRY(executor->cmd_dwheel)));
        fprintf(fp, "execp_lclick_command_sink = %d\n",
                    (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(executor->cmd_lclick_sink)));
        fprintf(fp, "execp_mclick_command_sink = %d\n",
                    (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(executor->cmd_mclick_sink)));
        fprintf(fp, "execp_rclick_command_sink = %d\n",
                    (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(executor->cmd_rclick_sink)));
        fprintf(fp, "execp_uwheel_command_sink = %d\n",
                    (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(executor->cmd_uwheel_sink)));
        fprintf(fp, "execp_dwheel_command_sink = %d\n",
                    (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(executor->cmd_dwheel_sink)));

        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(executor->font_use)))
            fprintf(fp, "execp_font = %s\n", gtk_font_button_get_font_name(GTK_FONT_BUTTON(executor->font)));
        GdkRGBA color;
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(executor->font_color), &color);
        config_write_color(fp, "execp_font_color", &color);
        fprintf(fp,
                "execp_padding = %d %d %d\n",
                (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(executor->padx)),
                (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(executor->pady)),
                (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(executor->spacing)));
        fprintf(fp, "execp_background_id = %d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(executor->bg)));
        fprintf(fp,
                "execp_centered = %d\n",
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(executor->centered)) ? 1 : 0);
        fprintf(fp, "execp_icon_w = %d\n", (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(executor->iw)));
        fprintf(fp, "execp_icon_h = %d\n", (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(executor->ih)));

        fprintf(fp, "\n");
    }
}

void config_write_button(FILE *fp)
{
    for (int i = 0; i < buttons->len; i++) {
        fprintf(fp, "#-------------------------------------\n");
        fprintf(fp, "# Button %d\n", i + 1);

        Button *button = &g_array_index(buttons, Button, i);

        fprintf(fp, "button = new\n");
        if (strlen(gtk_entry_get_text(GTK_ENTRY(button->icon))))
            fprintf(fp, "button_icon = %s\n", gtk_entry_get_text(GTK_ENTRY(button->icon)));
        if (gtk_entry_get_text(GTK_ENTRY(button->text)))
            fprintf(fp, "button_text = %s\n", gtk_entry_get_text(GTK_ENTRY(button->text)));
        if (strlen(gtk_entry_get_text(GTK_ENTRY(button->tooltip))))
            fprintf(fp, "button_tooltip = %s\n", gtk_entry_get_text(GTK_ENTRY(button->tooltip)));

        fprintf(fp, "button_lclick_command = %s\n", gtk_entry_get_text(GTK_ENTRY(button->cmd_lclick)));
        fprintf(fp, "button_rclick_command = %s\n", gtk_entry_get_text(GTK_ENTRY(button->cmd_rclick)));
        fprintf(fp, "button_mclick_command = %s\n", gtk_entry_get_text(GTK_ENTRY(button->cmd_mclick)));
        fprintf(fp, "button_uwheel_command = %s\n", gtk_entry_get_text(GTK_ENTRY(button->cmd_uwheel)));
        fprintf(fp, "button_dwheel_command = %s\n", gtk_entry_get_text(GTK_ENTRY(button->cmd_dwheel)));

        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button->font_use)))
            fprintf(fp, "button_font = %s\n", gtk_font_button_get_font_name(GTK_FONT_BUTTON(button->font)));
        GdkRGBA color;
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button->font_color), &color);
        config_write_color(fp, "button_font_color", &color);
        fprintf(fp,
                "button_padding = %d %d %d\n",
                (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(button->padx)),
                (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(button->pady)),
                (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(button->spacing)));
        fprintf(fp, "button_background_id = %d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(button->bg)));
        fprintf(fp,
                "button_centered = %d\n",
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button->centered)) ? 1 : 0);
        fprintf(fp,
                "button_max_icon_size = %d\n",
                (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(button->max_icon_size)));

        fprintf(fp, "\n");
    }
}

void config_write_tooltip(FILE *fp)
{
    fprintf(fp, "#-------------------------------------\n");
    fprintf(fp, "# Tooltip\n");

    fprintf(fp, "tooltip_show_timeout = %g\n", gtk_spin_button_get_value(GTK_SPIN_BUTTON(tooltip_show_after)));
    fprintf(fp, "tooltip_hide_timeout = %g\n", gtk_spin_button_get_value(GTK_SPIN_BUTTON(tooltip_hide_after)));
    fprintf(fp,
            "tooltip_padding = %d %d\n",
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(tooltip_padding_x)),
            (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(tooltip_padding_y)));
    fprintf(fp, "tooltip_background_id = %d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(tooltip_background)));

    GdkRGBA color;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(tooltip_font_color), &color);
    config_write_color(fp, "tooltip_font_color", &color);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tooltip_font_set)))
        fprintf(fp, "tooltip_font = %s\n", gtk_font_button_get_font_name(GTK_FONT_BUTTON(tooltip_font)));

    fprintf(fp, "\n");
}

// Similar to BSD checksum, except we skip the first line (metadata)
unsigned short checksum_txt(FILE *f)
{
    unsigned int checksum = 0;
    fseek(f, 0, SEEK_SET);

    // Skip the first line
    int c;
    do {
        c = getc(f);
    } while (c != EOF && c != '\n');

    while ((c = getc(f)) != EOF) {
        // Rotate right
        checksum = (checksum >> 1) + ((checksum & 1) << 15);
        // Update checksum
        checksum += c;
        // Truncate to 16 bits
        checksum &= 0xffff;
    }
    return checksum;
}

void config_save_file(const char *path)
{
    fprintf(stderr, "tint2: config_save_file : %s\n", path);

    FILE *fp;
    if ((fp = fopen(path, "w+t")) == NULL)
        return;

    unsigned short checksum = 0;
    fprintf(fp, "#---- Generated by tint2conf %04x ----\n", checksum);
    fprintf(fp, "# See https://gitlab.com/o9000/tint2/wikis/Configure for \n");
    fprintf(fp, "# full documentation of the configuration options.\n");

    config_write_gradients(fp);
    config_write_backgrounds(fp);
    config_write_panel(fp);
    config_write_taskbar(fp);
    config_write_task(fp);
    config_write_systray(fp);
    config_write_launcher(fp);
    config_write_clock(fp);
    config_write_battery(fp);
    config_write_separator(fp);
    config_write_execp(fp);
    config_write_button(fp);
    config_write_tooltip(fp);

    checksum = checksum_txt(fp);
    fseek(fp, 0, SEEK_SET);
    fflush(fp);
    fprintf(fp, "#---- Generated by tint2conf %04x ----\n", checksum);

    fclose(fp);
}

gboolean config_is_manual(const char *path)
{
    FILE *fp;
    char line[512];
    gboolean result;

    if ((fp = fopen(path, "r")) == NULL)
        return FALSE;

    result = TRUE;
    if (fgets(line, sizeof(line), fp) != NULL) {
        if (!g_regex_match_simple("^#---- Generated by tint2conf [0-9a-f][0-9a-f][0-9a-f][0-9a-f] ----\n$",
                                  line,
                                  0,
                                  0)) {
            result = TRUE;
        } else {
            unsigned short checksum1 = checksum_txt(fp);
            unsigned short checksum2 = 0;
            if (sscanf(line, "#---- Generated by tint2conf %hxu", &checksum2) == 1) {
                result = checksum1 != checksum2;
            } else {
                result = TRUE;
            }
        }
    }
    fclose(fp);
    return result;
}

void finalize_bg()
{
    if (num_bg > 0) {
        GdkRGBA color;
        if (!read_bg_color_hover) {
            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(background_fill_color), &color);
            gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(background_fill_color_over), &color);
        }
        if (!read_border_color_hover) {
            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(background_border_color), &color);
            gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(background_border_color_over), &color);
        }
        if (!read_bg_color_press) {
            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(background_fill_color), &color);
            gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(background_fill_color_press), &color);
        }
        if (!read_border_color_press) {
            gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(background_border_color_over), &color);
            gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(background_border_color_press), &color);
        }
    }
    background_force_update();
}

void finalize_gradient()
{
    if (num_gr > 0) {
        gradient_force_update();
    }
}

void add_entry(char *key, char *value)
{
    char *values[4] = {NULL, NULL, NULL, NULL};
    cfg_key_t key_i = str_index (key, cfg_keys, DICT_KEYS_NUM);

    switch (key_i) {
    /* Gradients */
    case key_scale_relative_to_dpi:
        extract_values(value, values, 3);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(scale_relative_to_dpi), atoi(values[0]));
        break;
    case key_scale_relative_to_screen_height:
        extract_values(value, values, 3);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(scale_relative_to_screen_height), atoi(values[0]));
        break;
    case key_gradient: {
        finalize_gradient();
        GradientConfigType t;
        if (g_str_equal(value, "horizontal"))
            t = GRADIENT_CONFIG_HORIZONTAL;
        else if (g_str_equal(value, "vertical"))
            t = GRADIENT_CONFIG_VERTICAL;
        else
            t = GRADIENT_CONFIG_RADIAL;
        gradient_create_new(t);
        num_gr++;
        break; }
    case key_start_color: {
        extract_values(value, values, 3);
        GdkRGBA col;
        hex2gdk(values[0], &col);
        col.alpha = (values[1] ? atoi(values[1]) : 50) / 100.0;
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(gradient_start_color), &col);
        break; }
    case key_end_color: {
        extract_values(value, values, 3);
        GdkRGBA col;
        hex2gdk(values[0], &col);
        col.alpha = (values[1] ? atoi(values[1]) : 50) / 100.0;
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(gradient_end_color), &col);
        break; }
    case key_color_stop: {
        GradientConfig *g = (GradientConfig *)g_list_last(gradients)->data;
        extract_values(value, values, 3);
        GradientConfigColorStop *color_stop = (GradientConfigColorStop *)calloc(1, sizeof(GradientConfigColorStop));
        color_stop->offset = atof(values[0]) / 100.0;
        get_color(values[1], color_stop->color.rgb);
        if (values[2])
            color_stop->color.alpha = (atoi(values[2]) / 100.0);
        else
            color_stop->color.alpha = 0.5;
        g->extra_color_stops = g_list_append(g->extra_color_stops, color_stop);
        current_gradient_changed(NULL, NULL);
        break; }

    /* Background and border */
    case key_rounded:
        // 'rounded' is the first parameter => alloc a new background
        finalize_bg();
        background_create_new();
        num_bg++;
        read_bg_color_hover = 0;
        read_border_color_hover = 0;
        read_bg_color_press = 0;
        read_border_color_press = 0;
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(background_corner_radius), atoi(value));
        break;
    case key_rounded_corners: {
        unsigned int rmask = 0;
        if (extract_values(value, values, 4))
            for (int i=0; i<4 && values[i]; i++) {
                if (strcmp(values[i], "tl") == 0 || strcmp(values[i], "TL") == 0)
                    rmask |= CORNER_TL;
                if (strcmp(values[i], "tr") == 0 || strcmp(values[i], "TR") == 0)
                    rmask |= CORNER_TR;
                if (strcmp(values[i], "br") == 0 || strcmp(values[i], "BR") == 0)
                    rmask |= CORNER_BR;
                if (strcmp(values[i], "bl") == 0 || strcmp(values[i], "BL") == 0)
                    rmask |= CORNER_BL;
            }
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(background_corner_round_tleft),  !!(rmask & CORNER_TL));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(background_corner_round_tright), !!(rmask & CORNER_TR));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(background_corner_round_bleft),  !!(rmask & CORNER_BL));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(background_corner_round_bright), !!(rmask & CORNER_BR));
        break; }
    case key_border_width:
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(background_border_width), atoi(value));
        break;
    case key_background_color: {
        extract_values(value, values, 3);
        GdkRGBA col;
        hex2gdk(values[0], &col);
        col.alpha = (values[1] ? atoi(values[1]) : 50) / 100.0;
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(background_fill_color), &col);
        break; }
    case key_border_color: {
        extract_values(value, values, 3);
        GdkRGBA col;
        hex2gdk(values[0], &col);
        col.alpha = (values[1] ? atoi(values[1]) : 50) / 100.0;
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(background_border_color), &col);
        break; }
    case key_background_color_hover: {
        extract_values(value, values, 3);
        GdkRGBA col;
        hex2gdk(values[0], &col);
        col.alpha = (values[1] ? atoi(values[1]) : 50) / 100.0;
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(background_fill_color_over), &col);
        read_bg_color_hover = 1;
        break; }
    case key_border_color_hover: {
        extract_values(value, values, 3);
        GdkRGBA col;
        hex2gdk(values[0], &col);
        col.alpha = (values[1] ? atoi(values[1]) : 50) / 100.0;
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(background_border_color_over), &col);
        read_border_color_hover = 1;
        break; }
    case key_background_color_pressed: {
        extract_values(value, values, 3);
        GdkRGBA col;
        hex2gdk(values[0], &col);
        col.alpha = (values[1] ? atoi(values[1]) : 50) / 100.0;
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(background_fill_color_press), &col);
        read_bg_color_press = 1;
        break; }
    case key_border_color_pressed: {
        extract_values(value, values, 3);
        GdkRGBA col;
        hex2gdk(values[0], &col);
        col.alpha = (values[1] ? atoi(values[1]) : 50) / 100.0;
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(background_border_color_press), &col);
        read_border_color_press = 1;
        break; }
    case key_border_sides:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(background_border_sides_top),
                                     strchr(value, 't') || strchr(value, 'T'));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(background_border_sides_bottom),
                                     strchr(value, 'b') || strchr(value, 'B'));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(background_border_sides_left),
                                     strchr(value, 'l') || strchr(value, 'L'));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(background_border_sides_right),
                                     strchr(value, 'r') || strchr(value, 'R'));
        break;
    case key_gradient_id: {
        int id = gradient_index_safe(atoi(value));
        gtk_combo_box_set_active(GTK_COMBO_BOX(background_gradient), id);
        break; }
    case key_gradient_id_hover: {
        int id = gradient_index_safe(atoi(value));
        gtk_combo_box_set_active(GTK_COMBO_BOX(background_gradient_over), id);
        break; }
    case key_gradient_id_pressed: {
        int id = gradient_index_safe(atoi(value));
        gtk_combo_box_set_active(GTK_COMBO_BOX(background_gradient_press), id);
        break; }
    case key_border_content_tint_weight:
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(background_border_content_tint_weight), atoi(value));
        break;
    case key_background_content_tint_weight:
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(background_fill_content_tint_weight), atoi(value));
        break;

    /* Panel */
    case key_panel_size: {
        extract_values(value, values, 3);
        char *b;
        if ((b = strchr(values[0], '%'))) {
            b[0] = '\0';
            gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_width_type), 0);
        } else
            gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_width_type), 1);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_width), atoi(values[0]));
        if (atoi(values[0]) == 0) {
            // full width mode
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_width), 100);
            gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_width_type), 0);
        }
        if ((b = strchr(values[1], '%'))) {
            b[0] = '\0';
            gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_height_type), 0);
        } else
            gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_height_type), 1);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_height), atoi(values[1]));
        break; }
    case key_panel_items:
        config_has_panel_items = 1;
        set_panel_items(value);
        break;
    case key_panel_margin:
        extract_values(value, values, 3);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_margin_x), atoi(values[0]));
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_margin_y), atoi(values[1]));
        break;
    case key_panel_padding:
        extract_values(value, values, 3);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_padding_x), atoi(values[0]));
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_spacing), atoi(values[0]));
        if (values[1])
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_padding_y), atoi(values[1]));
        if (values[2])
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_spacing), atoi(values[2]));
        break;
    case key_panel_position: {
        extract_values(value, values, 3);

        char vpos, hpos, orientation;

        vpos = 'B';
        hpos = 'C';
        orientation = 'H';

        if (values[0]) {
            if (strcmp(values[0], "top") == 0)
                vpos = 'T';
            if (strcmp(values[0], "bottom") == 0)
                vpos = 'B';
            if (strcmp(values[0], "center") == 0)
                vpos = 'C';
        }

        if (values[1]) {
            if (strcmp(values[1], "left") == 0)
                hpos = 'L';
            if (strcmp(values[1], "right") == 0)
                hpos = 'R';
            if (strcmp(values[1], "center") == 0)
                hpos = 'C';
        }

        if (values[2]) {
            if (strcmp(values[2], "horizontal") == 0)
                orientation = 'H';
            if (strcmp(values[2], "vertical") == 0)
                orientation = 'V';
        }

        if (vpos == 'T' && hpos == 'L' && orientation == 'H')
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_TLH]), 1);
        if (vpos == 'T' && hpos == 'C' && orientation == 'H')
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_TCH]), 1);
        if (vpos == 'T' && hpos == 'R' && orientation == 'H')
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_TRH]), 1);

        if (vpos == 'B' && hpos == 'L' && orientation == 'H')
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_BLH]), 1);
        if (vpos == 'B' && hpos == 'C' && orientation == 'H')
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_BCH]), 1);
        if (vpos == 'B' && hpos == 'R' && orientation == 'H')
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_BRH]), 1);

        if (vpos == 'T' && hpos == 'L' && orientation == 'V')
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_TLV]), 1);
        if (vpos == 'C' && hpos == 'L' && orientation == 'V')
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_CLV]), 1);
        if (vpos == 'B' && hpos == 'L' && orientation == 'V')
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_BLV]), 1);

        if (vpos == 'T' && hpos == 'R' && orientation == 'V')
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_TRV]), 1);
        if (vpos == 'C' && hpos == 'R' && orientation == 'V')
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_CRV]), 1);
        if (vpos == 'B' && hpos == 'R' && orientation == 'V')
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(screen_position[POS_BRV]), 1);
        break; }
    case key_panel_background_id: {
        int id = background_index_safe(atoi(value));
        gtk_combo_box_set_active(GTK_COMBO_BOX(panel_background), id);
        break; }
    case key_panel_window_name:
        gtk_entry_set_text(GTK_ENTRY(panel_window_name), value);
        break;
    case key_disable_transparency:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(disable_transparency), atoi(value));
        break;
    case key_mouse_effects:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(panel_mouse_effects), atoi(value));
        break;
    case key_mouse_hover_icon_asb:
        extract_values(value, values, 3);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(mouse_hover_icon_opacity), atoi(values[0]));
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(mouse_hover_icon_saturation), atoi(values[1]));
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(mouse_hover_icon_brightness), atoi(values[2]));
        break;
    case key_mouse_pressed_icon_asb:
        extract_values(value, values, 3);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(mouse_pressed_icon_opacity), atoi(values[0]));
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(mouse_pressed_icon_saturation), atoi(values[1]));
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(mouse_pressed_icon_brightness), atoi(values[2]));
        break;
    case key_font_shadow:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(font_shadow), atoi(value));
        break;
    case key_wm_menu:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(panel_wm_menu), atoi(value));
        break;
    case key_panel_dock:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(panel_dock), atoi(value));
        break;
    case key_panel_pivot_struts:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(panel_pivot_struts), atoi(value));
        break;
    case key_panel_layer:
        if (strcmp(value, "bottom") == 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_layer), 2);
        else if (strcmp(value, "top") == 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_layer), 0);
        else
            gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_layer), 1);
        break;
    case key_panel_monitor:
        if (strcmp(value, "all") == 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_monitor), 0);
        else if (strcmp(value, "primary") == 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_monitor), 1);
        else if (strcmp(value, "1") == 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_monitor), 2);
        else if (strcmp(value, "2") == 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_monitor), 3);
        else if (strcmp(value, "3") == 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_monitor), 4);
        else if (strcmp(value, "4") == 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_monitor), 5);
        else if (strcmp(value, "5") == 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_monitor), 6);
        else if (strcmp(value, "6") == 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_monitor), 7);
        break;
    case key_panel_shrink:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(panel_shrink), atoi(value));
        break;

    /* autohide options */
    case key_autohide:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(panel_autohide), atoi(value));
        break;
    case key_autohide_show_timeout:
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_autohide_show_time), atof(value));
        break;
    case key_autohide_hide_timeout:
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_autohide_hide_time), atof(value));
        break;
    case key_strut_policy:
        if (strcmp(value, "follow_size") == 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_strut_policy), 0);
        else if (strcmp(value, "none") == 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_strut_policy), 2);
        else
            gtk_combo_box_set_active(GTK_COMBO_BOX(panel_combo_strut_policy), 1);
        break;
    case key_autohide_height:
        if (atoi(value) <= 0) {
            // autohide need height > 0
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_autohide_size), 1);
        } else {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(panel_autohide_size), atoi(value));
        }
        break;


    /* Battery */
    case key_battery:
        // Obsolete option
        config_has_battery = 1;
        config_battery_enabled = atoi(value);
        break;
    case key_battery_tooltip:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(battery_tooltip), atoi(value));
        break;
    case key_battery_low_status:
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(battery_alert_if_lower), atof(value));
        break;
    case key_battery_low_cmd:
        gtk_entry_set_text(GTK_ENTRY(battery_alert_cmd), value);
        break;
    case key_battery_full_cmd:
        gtk_entry_set_text(GTK_ENTRY(battery_alert_full_cmd), value);
        break;
    case key_bat1_font:
        gtk_font_button_set_font_name(GTK_FONT_BUTTON(battery_font_line1), value);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(battery_font_line1_set), TRUE);
        break;
    case key_bat2_font:
        gtk_font_button_set_font_name(GTK_FONT_BUTTON(battery_font_line2), value);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(battery_font_line2_set), TRUE);
        break;
    case key_bat1_format:
        gtk_entry_set_text(GTK_ENTRY(battery_format1), value);
        break;
    case key_bat2_format:
        gtk_entry_set_text(GTK_ENTRY(battery_format2), value);
        break;
    case key_battery_font_color: {
        extract_values(value, values, 3);
        GdkRGBA col;
        hex2gdk(values[0], &col);
        if (values[1]) {
            col.alpha = atoi(values[1]) / 100.0;
        }
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(battery_font_color), &col);
        break; }
    case key_battery_padding:
        extract_values(value, values, 3);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(battery_padding_x), atoi(values[0]));
        if (values[1])
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(battery_padding_y), atoi(values[1]));
        break;
    case key_battery_background_id: {
        int id = background_index_safe(atoi(value));
        gtk_combo_box_set_active(GTK_COMBO_BOX(battery_background), id);
        break; }
    case key_battery_hide: {
        int percentage_hide = atoi(value);
        if (percentage_hide == 0)
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(battery_hide_if_higher), 101.0);
        else
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(battery_hide_if_higher), atoi(value));
        break; }
    case key_battery_lclick_command:
        gtk_entry_set_text(GTK_ENTRY(battery_left_command), value);
        break;
    case key_battery_rclick_command:
        gtk_entry_set_text(GTK_ENTRY(battery_right_command), value);
        break;
    case key_battery_mclick_command:
        gtk_entry_set_text(GTK_ENTRY(battery_mclick_command), value);
        break;
    case key_battery_uwheel_command:
        gtk_entry_set_text(GTK_ENTRY(battery_uwheel_command), value);
        break;
    case key_battery_dwheel_command:
        gtk_entry_set_text(GTK_ENTRY(battery_dwheel_command), value);
        break;
    case key_ac_connected_cmd:
        gtk_entry_set_text(GTK_ENTRY(ac_connected_cmd), value);
        break;
    case key_ac_disconnected_cmd:
        gtk_entry_set_text(GTK_ENTRY(ac_disconnected_cmd), value);
        break;

    /* Clock */
    case key_time1_format:
        gtk_entry_set_text(GTK_ENTRY(clock_format_line1), value);
        no_items_clock_enabled = strlen(value) > 0;
        break;
    case key_time2_format:
        gtk_entry_set_text(GTK_ENTRY(clock_format_line2), value);
        break;
    case key_time1_font:
        gtk_font_button_set_font_name(GTK_FONT_BUTTON(clock_font_line1), value);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(clock_font_line1_set), TRUE);
        break;
    case key_time1_timezone:
        gtk_entry_set_text(GTK_ENTRY(clock_tmz_line1), value);
        break;
    case key_time2_timezone:
        gtk_entry_set_text(GTK_ENTRY(clock_tmz_line2), value);
        break;
    case key_time2_font:
        gtk_font_button_set_font_name(GTK_FONT_BUTTON(clock_font_line2), value);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(clock_font_line2_set), TRUE);
        break;
    case key_clock_font_color: {
        extract_values(value, values, 3);
        GdkRGBA col;
        hex2gdk(values[0], &col);
        if (values[1]) {
            col.alpha = atoi(values[1]) / 100.0;
        }
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(clock_font_color), &col);
        break; }
    case key_clock_padding:
        extract_values(value, values, 3);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(clock_padding_x), atoi(values[0]));
        if (values[1])
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(clock_padding_y), atoi(values[1]));
        break;
    case key_clock_background_id: {
        int id = background_index_safe(atoi(value));
        gtk_combo_box_set_active(GTK_COMBO_BOX(clock_background), id);
        break; }
    case key_clock_tooltip:
        gtk_entry_set_text(GTK_ENTRY(clock_format_tooltip), value);
        break;
    case key_clock_tooltip_timezone:
        gtk_entry_set_text(GTK_ENTRY(clock_tmz_tooltip), value);
        break;
    case key_clock_lclick_command:
        gtk_entry_set_text(GTK_ENTRY(clock_left_command), value);
        break;
    case key_clock_rclick_command:
        gtk_entry_set_text(GTK_ENTRY(clock_right_command), value);
        break;
    case key_clock_mclick_command:
        gtk_entry_set_text(GTK_ENTRY(clock_mclick_command), value);
        break;
    case key_clock_uwheel_command:
        gtk_entry_set_text(GTK_ENTRY(clock_uwheel_command), value);
        break;
    case key_clock_dwheel_command:
        gtk_entry_set_text(GTK_ENTRY(clock_dwheel_command), value);
        break;

    /* Taskbar */
    case key_taskbar_mode:
        if (strcmp(value, "multi_desktop") == 0)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(taskbar_show_desktop), 1);
        else
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(taskbar_show_desktop), 0);
        break;
    case key_taskbar_hide_if_empty:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(taskbar_hide_empty), atoi(value));
        break;
    case key_taskbar_distribute_size:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(taskbar_distribute_size), atoi(value));
        break;
    case key_taskbar_sort_order:
        if (strcmp(value, "none") == 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_sort_order), 0);
        else if (strcmp(value, "title") == 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_sort_order), 1);
        else if (strcmp(value, "application") == 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_sort_order), 2);
        else if (strcmp(value, "center") == 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_sort_order), 3);
        else if (strcmp(value, "mru") == 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_sort_order), 4);
        else if (strcmp(value, "lru") == 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_sort_order), 5);
        else
            gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_sort_order), 0);
        break;
    case key_task_align:
        if (strcmp(value, "left") == 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_alignment), 0);
        else if (strcmp(value, "center") == 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_alignment), 1);
        else if (strcmp(value, "right") == 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_alignment), 2);
        else
            gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_alignment), 0);
        break;
    case key_taskbar_padding: {
        extract_values(value, values, 3);
        int padx = atoi(values[0]);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(taskbar_padding_x), padx);
        if (values[1])
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(taskbar_padding_y), atoi(values[1]));
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(taskbar_spacing), values[2] ? atoi(values[2]) : padx);
        break; }
    case key_taskbar_background_id: {
        int id = background_index_safe(atoi(value));
        gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_inactive_background), id);
        if (gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_active_background)) < 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_active_background), id);
        break; }
    case key_taskbar_active_background_id: {
        int id = background_index_safe(atoi(value));
        gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_active_background), id);
        break; }
    case key_taskbar_name:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(taskbar_show_name), atoi(value));
        break;
    case key_taskbar_hide_inactive_tasks:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(taskbar_hide_inactive_tasks), atoi(value));
        break;
    case key_taskbar_hide_different_monitor:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(taskbar_hide_diff_monitor), atoi(value));
        break;
    case key_taskbar_hide_different_desktop:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(taskbar_hide_diff_desktop), atoi(value));
        break;
    case key_taskbar_always_show_all_desktop_tasks:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(taskbar_always_show_all_desktop_tasks), atoi(value));
        break;
    case key_taskbar_name_padding:
        extract_values(value, values, 3);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(taskbar_name_padding_x), atoi(values[0]));
        if (values[1])
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(taskbar_name_padding_y), atoi(values[1]));
        break;
    case key_taskbar_name_background_id: {
        int id = background_index_safe(atoi(value));
        gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_name_inactive_background), id);
        if (gtk_combo_box_get_active(GTK_COMBO_BOX(taskbar_name_active_background)) < 0)
            gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_name_active_background), id);
        break; }
    case key_taskbar_name_active_background_id: {
        int id = background_index_safe(atoi(value));
        gtk_combo_box_set_active(GTK_COMBO_BOX(taskbar_name_active_background), id);
        break; }
    case key_taskbar_name_font:
        gtk_font_button_set_font_name(GTK_FONT_BUTTON(taskbar_name_font), value);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(taskbar_name_font_set), TRUE);
        break;
    case key_taskbar_name_font_color: {
        extract_values(value, values, 3);
        GdkRGBA col;
        hex2gdk(values[0], &col);
        if (values[1]) {
            col.alpha = atoi(values[1]) / 100.0;
        }
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(taskbar_name_inactive_color), &col);
        break; }
    case key_taskbar_name_active_font_color: {
        extract_values(value, values, 3);
        GdkRGBA col;
        hex2gdk(values[0], &col);
        if (values[1]) {
            col.alpha = atoi(values[1]) / 100.0;
        }
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(taskbar_name_active_color), &col);
        break; }

    /* Task */
    case key_task_text:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_show_text), atoi(value));
        break;
    case key_task_icon:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_show_icon), atoi(value));
        break;
    case key_task_centered:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_align_center), atoi(value));
        break;
    case key_urgent_nb_of_blink:
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(task_urgent_blinks), atoi(value));
        break;
    case key_task_width:
        // old parameter : just for backward compatibility
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(task_maximum_width), atoi(value));
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(task_maximum_height), 30.0);
        break;
    case key_task_maximum_size:
        extract_values(value, values, 3);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(task_maximum_width), atoi(values[0]));
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(task_maximum_height), 30.0);
        if (values[1])
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(task_maximum_height), atoi(values[1]));
        break;
    case key_task_padding:
        extract_values(value, values, 3);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(task_padding_x), atoi(values[0]));
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(task_spacing), atoi(values[0]));
        if (values[1])
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(task_padding_y), atoi(values[1]));
        if (values[2])
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(task_spacing), atoi(values[2]));
        break;
    case key_task_font:
        gtk_font_button_set_font_name(GTK_FONT_BUTTON(task_font), value);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_font_set), TRUE);
        break;

    // "tooltip" is deprecated but here for backwards compatibility
    case key_task_tooltip:
    case key_tooltip:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tooltip_task_show), atoi(value));
        break;
    case key_task_thumbnail:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tooltip_task_thumbnail), atoi(value));
        break;
    case key_task_thumbnail_size:
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(tooltip_task_thumbnail_size), MAX(8, atoi(value)));
        break;

    /* Systray */
    case key_systray:
        // Obsolete option
        config_has_systray = 1;
        config_systray_enabled = atoi(value);
        break;
    case key_systray_padding:
        no_items_systray_enabled = 1;

        extract_values(value, values, 3);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(systray_padding_x), atoi(values[0]));
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(systray_spacing), atoi(values[0]));
        if (values[1])
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(systray_padding_y), atoi(values[1]));
        if (values[2])
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(systray_spacing), atoi(values[2]));
        break;
    case key_systray_background_id: {
        int id = background_index_safe(atoi(value));
        gtk_combo_box_set_active(GTK_COMBO_BOX(systray_background), id);
        break; }
    case key_systray_sort:
        gtk_combo_box_set_active(
            GTK_COMBO_BOX(systray_icon_order),
            // default to left2right
            (int[]) {2,0,1,3} [ 1 + str_index(value, (char *[]){"ascending", "descending", "right2left"}, 3) ]
        );
        break;
    case key_systray_icon_size:
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(systray_icon_size), atoi(value));
        break;
    case key_systray_monitor:
        gtk_combo_box_set_active(
            GTK_COMBO_BOX(systray_monitor),
            (int[]) {1,2,3,4,5,6,0} [ str_index(value, (char *[]){"1", "2", "3", "4", "5", "6", "primary"}, 7) ]
        );
        break;
    case key_systray_icon_asb:
        extract_values(value, values, 3);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(systray_icon_opacity), atoi(values[0]));
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(systray_icon_saturation), atoi(values[1]));
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(systray_icon_brightness), atoi(values[2]));
        break;
    case key_systray_name_filter:
        gtk_entry_set_text(GTK_ENTRY(systray_name_filter), value);
        break;

    /* Launcher */
    case key_launcher_padding:
        extract_values(value, values, 3);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(launcher_padding_x), atoi(values[0]));
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(launcher_spacing), atoi(values[0]));
        if (values[1])
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(launcher_padding_y), atoi(values[1]));
        if (values[2])
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(launcher_spacing), atoi(values[2]));
        break;
    case key_launcher_background_id: {
        int id = background_index_safe(atoi(value));
        gtk_combo_box_set_active(GTK_COMBO_BOX(launcher_background), id);
        break; }
    case key_launcher_icon_background_id: {
        int id = background_index_safe(atoi(value));
        gtk_combo_box_set_active(GTK_COMBO_BOX(launcher_icon_background), id);
        break; }
    case key_launcher_icon_size:
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(launcher_icon_size), atoi(value));
        break;
    case key_launcher_item_app: {
        char *path = expand_tilde(value);
        load_desktop_file(path, TRUE);
        load_desktop_file(path, FALSE);
        free(path);
        break; }
    case key_launcher_apps_dir: {
        char *path = expand_tilde(value);

        int position = gtk_entry_get_text_length(GTK_ENTRY(launcher_apps_dirs));
        if (position > 0) {
            gtk_editable_insert_text(GTK_EDITABLE(launcher_apps_dirs), ",", 1, &position);
        }
        position = gtk_entry_get_text_length(GTK_ENTRY(launcher_apps_dirs));
        gtk_editable_insert_text(GTK_EDITABLE(launcher_apps_dirs), path, strlen(path), &position);

        free(path);
        break; }
    case key_launcher_icon_theme:
        set_current_icon_theme(value);
        break;
    case key_launcher_icon_theme_override:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(launcher_icon_theme_override), atoi(value));
        break;
    case key_launcher_tooltip:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(launcher_tooltip), atoi(value));
        break;
    case key_startup_notifications:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(startup_notifications), atoi(value));
        break;
    case key_launcher_icon_asb:
        extract_values(value, values, 3);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(launcher_icon_opacity), atoi(values[0]));
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(launcher_icon_saturation), atoi(values[1]));
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(launcher_icon_brightness), atoi(values[2]));
        break;

    /* Tooltip */
    case key_tooltip_show_timeout:
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(tooltip_show_after), atof(value));
        break;
    case key_tooltip_hide_timeout:
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(tooltip_hide_after), atof(value));
        break;
    case key_tooltip_padding:
        extract_values(value, values, 3);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(tooltip_padding_x), atoi(values[0]));
        if (values[1])
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(tooltip_padding_y), atoi(values[1]));
        break;
    case key_tooltip_background_id: {
        int id = background_index_safe(atoi(value));
        gtk_combo_box_set_active(GTK_COMBO_BOX(tooltip_background), id);
        break; }
    case key_tooltip_font_color: {
        extract_values(value, values, 3);
        GdkRGBA col;
        hex2gdk(values[0], &col);
        if (values[1]) {
            col.alpha = atoi(values[1]) / 100.0;
        }
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(tooltip_font_color), &col);
        break; }
    case key_tooltip_font:
        gtk_font_button_set_font_name(GTK_FONT_BUTTON(tooltip_font), value);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tooltip_font_set), TRUE);
        break;

    /* Mouse actions */
    case key_mouse_left:
        set_action(value, task_mouse_left);
        break;
    case key_mouse_middle:
        set_action(value, task_mouse_middle);
        break;
    case key_mouse_right:
        set_action(value, task_mouse_right);
        break;
    case key_mouse_scroll_up:
        set_action(value, task_mouse_scroll_up);
        break;
    case key_mouse_scroll_down:
        set_action(value, task_mouse_scroll_down);
        break;

    /* Separator */
    case key_separator:
        separator_create_new();
        break;
    case key_separator_background_id: {
        int id = background_index_safe(atoi(value));
        gtk_combo_box_set_active(GTK_COMBO_BOX(separator_get_last()->bg), id);
        break; }
    case key_separator_color: {
        extract_values(value, values, 3);
        GdkRGBA col;
        hex2gdk(values[0], &col);
        if (values[1]) {
            col.alpha = atoi(values[1]) / 100.0;
        }
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(separator_get_last()->color), &col);
        break; }
    case key_separator_style: {
        int i = str_index(value, (char *[]){"dots", "empty", "line"}, 3);
        if (i != -1)
            gtk_combo_box_set_active(
                GTK_COMBO_BOX(separator_get_last()->style),
                (int []) {2,0,1} [i]
            );
        break; }
    case key_separator_size:
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(separator_get_last()->size), atoi(value));
        break;
    case key_separator_padding:
        extract_values(value, values, 3);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(separator_get_last()->padx), atoi(values[0]));
        if (values[1])
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(separator_get_last()->pady), atoi(values[1]));
        break;

    /* Executor */
    case key_execp:
        execp_create_new();
        break;
    case key_execp_name:
        gtk_entry_set_text(GTK_ENTRY(execp_get_last()->id), value);
        break;
    case key_execp_command:
        gtk_entry_set_text(GTK_ENTRY(execp_get_last()->cmd), value);
        break;
    case key_execp_interval:
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(execp_get_last()->interval), atoi(value));
        break;
    case key_execp_has_icon:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(execp_get_last()->has_icon), atoi(value));
        break;
    case key_execp_cache_icon:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(execp_get_last()->cache_icon), atoi(value));
        break;
    case key_execp_continuous:
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(execp_get_last()->cont), atoi(value));
        break;
    case key_execp_markup:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(execp_get_last()->markup), atoi(value));
        break;
    case key_execp_monitor:
        gtk_combo_box_set_active(
            GTK_COMBO_BOX(execp_get_last()->mon),
            (int[]) {1,2,3,4,5,6,0,1} [ str_index(value, (char *[]){"1", "2", "3", "4", "5", "6", "all", "primary"}, 8) ]
        );
        break;
    case key_execp_tooltip:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(execp_get_last()->show_tooltip), strlen(value) > 0);
        gtk_entry_set_text(GTK_ENTRY(execp_get_last()->tooltip), value);
        break;
    case key_execp_lclick_command:
        gtk_entry_set_text(GTK_ENTRY(execp_get_last()->cmd_lclick), value);
        break;
    case key_execp_rclick_command:
        gtk_entry_set_text(GTK_ENTRY(execp_get_last()->cmd_rclick), value);
        break;
    case key_execp_mclick_command:
        gtk_entry_set_text(GTK_ENTRY(execp_get_last()->cmd_mclick), value);
        break;
    case key_execp_uwheel_command:
        gtk_entry_set_text(GTK_ENTRY(execp_get_last()->cmd_uwheel), value);
        break;
    case key_execp_dwheel_command:
        gtk_entry_set_text(GTK_ENTRY(execp_get_last()->cmd_dwheel), value);
        break;
    case key_execp_lclick_command_sink:
        if (value && *value)
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(execp_get_last()->cmd_lclick_sink), atoi(value));
        break;
    case key_execp_mclick_command_sink:
        if (value && *value)
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(execp_get_last()->cmd_mclick_sink), atoi(value));
        break;
    case key_execp_rclick_command_sink:
        if (value && *value)
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(execp_get_last()->cmd_rclick_sink), atoi(value));
        break;
    case key_execp_uwheel_command_sink:
        if (value && *value)
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(execp_get_last()->cmd_uwheel_sink), atoi(value));
        break;
    case key_execp_dwheel_command_sink:
        if (value && *value)
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(execp_get_last()->cmd_dwheel_sink), atoi(value));
        break;
    case key_execp_font:
        gtk_font_button_set_font_name(GTK_FONT_BUTTON(execp_get_last()->font), value);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(execp_get_last()->font_use), TRUE);
        break;
    case key_execp_font_color: {
        extract_values(value, values, 3);
        GdkRGBA col;
        hex2gdk(values[0], &col);
        if (values[1]) {
            col.alpha = atoi(values[1]) / 100.0;
        }
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(execp_get_last()->font_color), &col);
        break; }
    case key_execp_padding: {
        extract_values(value, values, 3);
        int padx = atoi(values[0]);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(execp_get_last()->padx), atoi(values[0]));
        if (values[1])
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(execp_get_last()->pady), atoi(values[1]));
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(execp_get_last()->spacing), values[2] ? atoi(values[2]) : padx);
        break; }
    case key_execp_background_id: {
        int id = background_index_safe(atoi(value));
        gtk_combo_box_set_active(GTK_COMBO_BOX(execp_get_last()->bg), id);
        break; }
    case key_execp_icon_w:
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(execp_get_last()->iw), atoi(value));
        break;
    case key_execp_icon_h:
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(execp_get_last()->ih), atoi(value));
        break;
    case key_execp_centered:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(execp_get_last()->centered), atoi(value));
        break;

    /* Button */
    case key_button:
        button_create_new();
        break;
    case key_button_icon:
        gtk_entry_set_text(GTK_ENTRY(button_get_last()->icon), value);
        break;
    case key_button_text:
        gtk_entry_set_text(GTK_ENTRY(button_get_last()->text), value);
        break;
    case key_button_tooltip:
        gtk_entry_set_text(GTK_ENTRY(button_get_last()->tooltip), value);
        break;
    case key_button_lclick_command:
        gtk_entry_set_text(GTK_ENTRY(button_get_last()->cmd_lclick), value);
        break;
    case key_button_rclick_command:
        gtk_entry_set_text(GTK_ENTRY(button_get_last()->cmd_rclick), value);
        break;
    case key_button_mclick_command:
        gtk_entry_set_text(GTK_ENTRY(button_get_last()->cmd_mclick), value);
        break;
    case key_button_uwheel_command:
        gtk_entry_set_text(GTK_ENTRY(button_get_last()->cmd_uwheel), value);
        break;
    case key_button_dwheel_command:
        gtk_entry_set_text(GTK_ENTRY(button_get_last()->cmd_dwheel), value);
        break;
    case key_button_font:
        gtk_font_button_set_font_name(GTK_FONT_BUTTON(button_get_last()->font), value);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_get_last()->font_use), TRUE);
        break;
    case key_button_font_color: {
        extract_values(value, values, 3);
        GdkRGBA col;
        hex2gdk(values[0], &col);
        if (values[1]) {
            col.alpha = atoi(values[1]) / 100.0;
        }
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(button_get_last()->font_color), &col);
        break; }
    case key_button_padding:
        extract_values(value, values, 3);
        int padx = atoi(values[0]);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(button_get_last()->padx), padx);
        if (values[1])
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(button_get_last()->pady), atoi(values[1]));
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(button_get_last()->spacing), values[2] ? atoi(values[2]) : padx);
        break;
    case key_button_background_id: {
        int id = background_index_safe(atoi(value));
        gtk_combo_box_set_active(GTK_COMBO_BOX(button_get_last()->bg), id);
        break; }
    case key_button_centered:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button_get_last()->centered), atoi(value));
        break;
    case key_button_max_icon_size:
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(button_get_last()->max_icon_size), atoi(value));
        break;
    default:
        if (g_regex_match_simple("task.*_font_color", key, 0, 0)) {
            gchar **split = g_regex_split_simple("_", key, 0, 0);
            GtkWidget *widget = NULL;
            if (strcmp(split[1], "normal") == 0) {
                widget = task_normal_color;
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_normal_color_set), 1);
            } else if (strcmp(split[1], "active") == 0) {
                widget = task_active_color;
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_active_color_set), 1);
            } else if (strcmp(split[1], "urgent") == 0) {
                widget = task_urgent_color;
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_urgent_color_set), 1);
            } else if (strcmp(split[1], "iconified") == 0) {
                widget = task_iconified_color;
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_iconified_color_set), 1);
            } else if (strcmp(split[1], "font") == 0) {
                widget = task_default_color;
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_default_color_set), 1);
            }
            g_strfreev(split);
            if (widget) {
                extract_values(value, values, 3);
                GdkRGBA col;
                hex2gdk(values[0], &col);
                if (values[1]) {
                    col.alpha = atoi(values[1]) / 100.0;
                }
                gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(widget), &col);
            }
        } else if (g_regex_match_simple("task.*_icon_asb", key, 0, 0)) {
            gchar **split = g_regex_split_simple("_", key, 0, 0);
            GtkWidget *widget_opacity = NULL;
            GtkWidget *widget_saturation = NULL;
            GtkWidget *widget_brightness = NULL;
            if (strcmp(split[1], "normal") == 0) {
                widget_opacity = task_normal_icon_opacity;
                widget_saturation = task_normal_icon_saturation;
                widget_brightness = task_normal_icon_brightness;
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_normal_icon_osb_set), 1);
            } else if (strcmp(split[1], "active") == 0) {
                widget_opacity = task_active_icon_opacity;
                widget_saturation = task_active_icon_saturation;
                widget_brightness = task_active_icon_brightness;
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_active_icon_osb_set), 1);
            } else if (strcmp(split[1], "urgent") == 0) {
                widget_opacity = task_urgent_icon_opacity;
                widget_saturation = task_urgent_icon_saturation;
                widget_brightness = task_urgent_icon_brightness;
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_urgent_icon_osb_set), 1);
            } else if (strcmp(split[1], "iconified") == 0) {
                widget_opacity = task_iconified_icon_opacity;
                widget_saturation = task_iconified_icon_saturation;
                widget_brightness = task_iconified_icon_brightness;
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_iconified_icon_osb_set), 1);
            } else if (strcmp(split[1], "icon") == 0) {
                widget_opacity = task_default_icon_opacity;
                widget_saturation = task_default_icon_saturation;
                widget_brightness = task_default_icon_brightness;
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_default_icon_osb_set), 1);
            }
            g_strfreev(split);
            if (widget_opacity && widget_saturation && widget_brightness) {
                extract_values(value, values, 3);
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget_opacity), atoi(values[0]));
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget_saturation), atoi(values[1]));
                gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget_brightness), atoi(values[2]));
            }
        } else if (g_regex_match_simple("task.*_background_id", key, 0, 0)) {
            gchar **split = g_regex_split_simple("_", key, 0, 0);
            GtkWidget *widget = NULL;
            if (strcmp(split[1], "normal") == 0) {
                widget = task_normal_background;
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_normal_background_set), 1);
            } else if (strcmp(split[1], "active") == 0) {
                widget = task_active_background;
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_active_background_set), 1);
            } else if (strcmp(split[1], "urgent") == 0) {
                widget = task_urgent_background;
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_urgent_background_set), 1);
            } else if (strcmp(split[1], "iconified") == 0) {
                widget = task_iconified_background;
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_iconified_background_set), 1);
            } else if (strcmp(split[1], "background") == 0) {
                widget = task_default_background;
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(task_default_background_set), 1);
            }
            g_strfreev(split);
            if (widget) {
                int id = background_index_safe(atoi(value));
                gtk_combo_box_set_active(GTK_COMBO_BOX(widget), id);
            }
        }
    }
}

void hex2gdk(char *hex, GdkRGBA *color)
{
    int rgbi[3];
    if (hex_to_rgb(hex, rgbi))
        color->red   = rgbi[0] / 65535.0,
        color->green = rgbi[1] / 65535.0,
        color->blue  = rgbi[2] / 65535.0;
    else
        color->red = color->green = color->blue = 0;
    color->alpha = 1.0;
}

void set_action(char *event, GtkWidget *combo)
{
    if (strcmp(event, "none") == 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
    else if (strcmp(event, "close") == 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 1);
    else if (strcmp(event, "toggle") == 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 2);
    else if (strcmp(event, "iconify") == 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 3);
    else if (strcmp(event, "shade") == 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 4);
    else if (strcmp(event, "toggle_iconify") == 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 5);
    else if (strcmp(event, "maximize_restore") == 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 6);
    else if (strcmp(event, "desktop_left") == 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 7);
    else if (strcmp(event, "desktop_right") == 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 8);
    else if (strcmp(event, "next_task") == 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 9);
    else if (strcmp(event, "prev_task") == 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 10);
}

char *get_action(GtkWidget *combo)
{
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 0)
        return "none";
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 1)
        return "close";
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 2)
        return "toggle";
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 3)
        return "iconify";
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 4)
        return "shade";
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 5)
        return "toggle_iconify";
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 6)
        return "maximize_restore";
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 7)
        return "desktop_left";
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 8)
        return "desktop_right";
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 9)
        return "next_task";
    if (gtk_combo_box_get_active(GTK_COMBO_BOX(combo)) == 10)
        return "prev_task";
    return "none";
}
