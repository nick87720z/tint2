#include "background_gui.h"
#include "gradient_gui.h"

GtkListStore *backgrounds;
GtkWidget *current_background, *background_fill_color, *background_border_color, *background_gradient,
    *background_fill_color_over, *background_border_color_over, *background_gradient_over, *background_fill_color_press,
    *background_border_color_press, *background_gradient_press, *background_border_width, *background_corner_radius,
    *background_border_sides_top, *background_border_sides_bottom, *background_border_sides_left,
    *background_border_sides_right, *background_border_content_tint_weight, *background_fill_content_tint_weight;

GtkWidget *create_background_combo(const char *label)
{
    GtkWidget *combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(backgrounds));
    GtkCellRenderer *renderer = gtk_cell_renderer_pixbuf_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "pixbuf", bgColPixbuf, NULL);
    renderer = gtk_cell_renderer_text_new();
    g_object_set(renderer, "wrap-mode", PANGO_WRAP_WORD, NULL);
    g_object_set(renderer, "wrap-width", 300, NULL);
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "text", bgColText, NULL);
    g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(background_combo_changed), (void *)label);
    return combo;
}

void background_combo_changed(GtkWidget *widget, gpointer data)
{
    gchar *combo_text = (gchar *)data;
    if (!combo_text || g_str_equal(combo_text, ""))
        return;
    int selected_index = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));

    int index;
    for (index = 0;; index++) {
        GtkTreePath *path;
        GtkTreeIter iter;

        path = gtk_tree_path_new_from_indices(index, -1);
        gboolean found = gtk_tree_model_get_iter(GTK_TREE_MODEL(backgrounds), &iter, path);
        gtk_tree_path_free(path);

        if (!found) {
            break;
        }

        gchar *text;
        gtk_tree_model_get(GTK_TREE_MODEL(backgrounds), &iter, bgColText, &text, -1);
        gchar **parts = g_strsplit(text, ", ", -1);
        int ifound;
        for (ifound = 0; parts[ifound]; ifound++) {
            if (g_str_equal(parts[ifound], combo_text))
                break;
        }
        if (parts[ifound] && index != selected_index) {
            for (; parts[ifound + 1]; ifound++) {
                gchar *tmp = parts[ifound];
                parts[ifound] = parts[ifound + 1];
                parts[ifound + 1] = tmp;
            }
            g_free(parts[ifound]);
            parts[ifound] = NULL;
            text = g_strjoinv(", ", parts);
            g_strfreev(parts);
            gtk_list_store_set(backgrounds, &iter, bgColText, text, -1);
            g_free(text);
        } else if (!parts[ifound] && index == selected_index) {
            if (!ifound) {
                text = g_strdup(combo_text);
            } else {
                for (ifound = 0; parts[ifound]; ifound++) {
                    if (compare_strings(combo_text, parts[ifound]) < 0)
                        break;
                }
                if (parts[ifound]) {
                    gchar *tmp = parts[ifound];
                    parts[ifound] = g_strconcat(combo_text, ", ", tmp, NULL);
                    g_free(tmp);
                } else {
                    ifound--;
                    gchar *tmp = parts[ifound];
                    parts[ifound] = g_strconcat(tmp, ", ", combo_text, NULL);
                    g_free(tmp);
                }
                text = g_strjoinv(", ", parts);
                g_strfreev(parts);
            }
            gtk_list_store_set(backgrounds, &iter, bgColText, text, -1);
            g_free(text);
        }
    }
}

void create_background(GtkWidget *parent)
{
    backgrounds = gtk_list_store_new(bgNumCols,
                                     GDK_TYPE_PIXBUF,
                                     GDK_TYPE_RGBA,
                                     GDK_TYPE_RGBA,
                                     G_TYPE_INT,
                                     G_TYPE_INT,
                                     G_TYPE_INT,
                                     G_TYPE_STRING,
                                     GDK_TYPE_RGBA,
                                     GDK_TYPE_RGBA,
                                     G_TYPE_INT,
                                     GDK_TYPE_RGBA,
                                     GDK_TYPE_RGBA,
                                     G_TYPE_INT,
                                     G_TYPE_BOOLEAN,
                                     G_TYPE_BOOLEAN,
                                     G_TYPE_BOOLEAN,
                                     G_TYPE_BOOLEAN,
                                     G_TYPE_DOUBLE,
                                     G_TYPE_DOUBLE);

    GtkWidget *table, *label, *button;
    int row, col;

    table = gtk_table_new(1, 4, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

    row = 0, col = 0;
    label = gtk_label_new(_("<b>Background</b>"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    current_background = create_background_combo(NULL);
    gtk_widget_show(current_background);
    gtk_table_attach(GTK_TABLE(table), current_background, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(current_background, _("Selects the background you would like to modify"));

    button = gtk_button_new_from_stock("gtk-add");
    g_signal_connect(button, "clicked", G_CALLBACK(background_duplicate), NULL);
    gtk_widget_show(button);
    gtk_table_attach(GTK_TABLE(table), button, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(button, _("Creates a copy of the current background"));

    button = gtk_button_new_from_stock("gtk-remove");
    g_signal_connect(button, "clicked", G_CALLBACK(background_delete), NULL);
    gtk_widget_show(button);
    gtk_table_attach(GTK_TABLE(table), button, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(button, _("Deletes the current background"));

    table = gtk_table_new(4, 4, FALSE);
    gtk_widget_show(table);
    gtk_box_pack_start(GTK_BOX(parent), table, FALSE, FALSE, 0);
    gtk_table_set_row_spacings(GTK_TABLE(table), ROW_SPACING);
    gtk_table_set_col_spacings(GTK_TABLE(table), COL_SPACING);

    row++, col = 2;
    label = gtk_label_new(_("Fill color"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    background_fill_color = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(background_fill_color), TRUE);
    gtk_widget_show(background_fill_color);
    gtk_table_attach(GTK_TABLE(table), background_fill_color, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(background_fill_color, _("The fill color of the current background"));

    row++, col = 2;
    label = gtk_label_new(_("Fill tint"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    background_fill_content_tint_weight = gtk_spin_button_new_with_range(0, 100, 1);
    gtk_widget_show(background_fill_content_tint_weight);
    gtk_table_attach(GTK_TABLE(table), background_fill_content_tint_weight, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(background_fill_content_tint_weight, _("How much the border color should be tinted with the content color"));

    row++, col = 2;
    label = gtk_label_new(_("Border color"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    background_border_color = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(background_border_color), TRUE);
    gtk_widget_show(background_border_color);
    gtk_table_attach(GTK_TABLE(table), background_border_color, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(background_border_color, _("The border color of the current background"));

    row++, col = 2;
    label = gtk_label_new(_("Border tint"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    background_border_content_tint_weight = gtk_spin_button_new_with_range(0, 100, 1);
    gtk_widget_show(background_border_content_tint_weight);
    gtk_table_attach(GTK_TABLE(table), background_border_content_tint_weight, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(background_border_content_tint_weight, _("How much the border color should be tinted with the content color"));

    row++, col = 2;
    label = gtk_label_new(_("Gradient"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    background_gradient = create_gradient_combo();
    gtk_widget_show(background_gradient);
    gtk_table_attach(GTK_TABLE(table), background_gradient, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    row++, col = 2;
    label = gtk_label_new(_("Fill color (mouse over)"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    background_fill_color_over = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(background_fill_color_over), TRUE);
    gtk_widget_show(background_fill_color_over);
    gtk_table_attach(GTK_TABLE(table), background_fill_color_over, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(background_fill_color_over,
                         _("The fill color of the current background on mouse over"));

    row++, col = 2;
    label = gtk_label_new(_("Border color (mouse over)"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    background_border_color_over = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(background_border_color_over), TRUE);
    gtk_widget_show(background_border_color_over);
    gtk_table_attach(GTK_TABLE(table), background_border_color_over, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(
                         background_border_color_over,
                         _("The border color of the current background on mouse over"));

    row++, col = 2;
    label = gtk_label_new(_("Gradient (mouse over)"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    background_gradient_over = create_gradient_combo();
    gtk_widget_show(background_gradient_over);
    gtk_table_attach(GTK_TABLE(table), background_gradient_over, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    row++, col = 2;
    label = gtk_label_new(_("Fill color (pressed)"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    background_fill_color_press = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(background_fill_color_press), TRUE);
    gtk_widget_show(background_fill_color_press);
    gtk_table_attach(GTK_TABLE(table), background_fill_color_press, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(background_fill_color_press,
                         _("The fill color of the current background on mouse button press"));

    row++, col = 2;
    label = gtk_label_new(_("Border color (pressed)"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    background_border_color_press = gtk_color_button_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(background_border_color_press), TRUE);
    gtk_widget_show(background_border_color_press);
    gtk_table_attach(GTK_TABLE(table), background_border_color_press, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(background_border_color_press,
                         _("The border color of the current background on mouse button press"));

    row++, col = 2;
    label = gtk_label_new(_("Gradient (pressed)"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    background_gradient_press = create_gradient_combo();
    gtk_widget_show(background_gradient_press);
    gtk_table_attach(GTK_TABLE(table), background_gradient_press, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    row++, col = 2;
    label = gtk_label_new(_("Border width"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    background_border_width = gtk_spin_button_new_with_range(0, 100, 1);
    gtk_widget_show(background_border_width);
    gtk_table_attach(GTK_TABLE(table), background_border_width, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(background_border_width,
                         _("The width of the border of the current background, in pixels"));

    row++, col = 2;
    label = gtk_label_new(_("Corner radius"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    background_corner_radius = gtk_spin_button_new_with_range(0, 100, 1);
    gtk_widget_show(background_corner_radius);
    gtk_table_attach(GTK_TABLE(table), background_corner_radius, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;
    gtk_widget_set_tooltip_text(background_corner_radius, _("The corner radius of the current background"));

    row++;
    col = 2;
    label = gtk_label_new(_("Border sides"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0);
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    background_border_sides_top = gtk_check_button_new_with_label(_("Top"));
    gtk_widget_show(background_border_sides_top);
    gtk_table_attach(GTK_TABLE(table), background_border_sides_top, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    background_border_sides_bottom = gtk_check_button_new_with_label(_("Bottom"));
    gtk_widget_show(background_border_sides_bottom);
    gtk_table_attach(GTK_TABLE(table), background_border_sides_bottom, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    background_border_sides_left = gtk_check_button_new_with_label(_("Left"));
    gtk_widget_show(background_border_sides_left);
    gtk_table_attach(GTK_TABLE(table), background_border_sides_left, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    background_border_sides_right = gtk_check_button_new_with_label(_("Right"));
    gtk_widget_show(background_border_sides_right);
    gtk_table_attach(GTK_TABLE(table), background_border_sides_right, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);
    col++;

    g_signal_connect(G_OBJECT(current_background), "changed", G_CALLBACK(current_background_changed), table);
    g_signal_connect(G_OBJECT(background_fill_color), "color-set", G_CALLBACK(background_update), NULL);
    g_signal_connect(G_OBJECT(background_border_color), "color-set", G_CALLBACK(background_update), NULL);
    g_signal_connect(G_OBJECT(background_gradient), "changed", G_CALLBACK(background_update), NULL);
    g_signal_connect(G_OBJECT(background_fill_color_over), "color-set", G_CALLBACK(background_update), NULL);
    g_signal_connect(G_OBJECT(background_border_color_over), "color-set", G_CALLBACK(background_update), NULL);
    g_signal_connect(G_OBJECT(background_gradient_over), "changed", G_CALLBACK(background_update), NULL);
    g_signal_connect(G_OBJECT(background_fill_color_press), "color-set", G_CALLBACK(background_update), NULL);
    g_signal_connect(G_OBJECT(background_border_color_press), "color-set", G_CALLBACK(background_update), NULL);
    g_signal_connect(G_OBJECT(background_gradient_press), "changed", G_CALLBACK(background_update), NULL);
    g_signal_connect(G_OBJECT(background_border_width), "value-changed", G_CALLBACK(background_update), NULL);
    g_signal_connect(G_OBJECT(background_corner_radius), "value-changed", G_CALLBACK(background_update), NULL);
    g_signal_connect(G_OBJECT(background_border_sides_top), "toggled", G_CALLBACK(background_update), NULL);
    g_signal_connect(G_OBJECT(background_border_sides_bottom), "toggled", G_CALLBACK(background_update), NULL);
    g_signal_connect(G_OBJECT(background_border_sides_left), "toggled", G_CALLBACK(background_update), NULL);
    g_signal_connect(G_OBJECT(background_border_sides_right), "toggled", G_CALLBACK(background_update), NULL);
    g_signal_connect(G_OBJECT(background_border_content_tint_weight), "value-changed", G_CALLBACK(background_update), NULL);
    g_signal_connect(G_OBJECT(background_fill_content_tint_weight), "value-changed", G_CALLBACK(background_update), NULL);

    change_paragraph(parent);
}

int background_index_safe(int index)
{
    if (index <= 0)
        index = 0;
    if (index >= get_model_length(GTK_TREE_MODEL(backgrounds)))
        index = 0;
    return index;
}

void background_create_new()
{
    int r = 0;
    int b = 0;
    gboolean sideTop = TRUE;
    gboolean sideBottom = TRUE;
    gboolean sideLeft = TRUE;
    gboolean sideRight = TRUE;
    
    GdkRGBA newColor = {0, 0, 0, 0};

    int index = 0;
    GtkTreeIter iter;

    gtk_list_store_append(backgrounds, &iter);
    gtk_list_store_set(backgrounds, &iter,
                       bgColPixbuf,             NULL,
                       bgColFillColor,          &newColor,
                       bgColBorderColor,        &newColor,
                       bgColGradientId,         -1,
                       bgColBorderWidth,        b,
                       bgColCornerRadius,       r,
                       bgColText,               "",
                       bgColFillColorOver,      &newColor,
                       bgColBorderColorOver,    &newColor,
                       bgColGradientIdOver,     -1,
                       bgColFillColorPress,     &newColor,
                       bgColBorderColorPress,   &newColor,
                       bgColGradientIdPress,    -1,
                       bgColBorderSidesTop,     sideTop,
                       bgColBorderSidesBottom,  sideBottom,
                       bgColBorderSidesLeft,    sideLeft,
                       bgColBorderSidesRight,   sideRight,
                       -1);

    background_update_image(index);
    gtk_combo_box_set_active(GTK_COMBO_BOX(current_background), get_model_length(GTK_TREE_MODEL(backgrounds)) - 1);
    current_background_changed(0, 0);
}

void background_duplicate(GtkWidget *widget, gpointer data)
{
    int index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_background));
    if (index < 0) {
        background_create_new();
        return;
    }

    GtkTreePath *path;
    GtkTreeIter iter;

    path = gtk_tree_path_new_from_indices(index, -1);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(backgrounds), &iter, path);
    gtk_tree_path_free(path);

    int r;
    int b;
    gboolean sideTop;
    gboolean sideBottom;
    gboolean sideLeft;
    gboolean sideRight;
    GdkRGBA *fillColor;
    GdkRGBA *borderColor;
    GdkRGBA *fillColorOver;
    GdkRGBA *borderColorOver;
    GdkRGBA *fillColorPress;
    GdkRGBA *borderColorPress;
    int gradient_id, gradient_id_over, gradient_id_press;

    gtk_tree_model_get(GTK_TREE_MODEL(backgrounds),
                       &iter,
                       bgColFillColor,          &fillColor,
                       bgColBorderColor,        &borderColor,
                       bgColFillColorOver,      &fillColorOver,
                       bgColBorderColorOver,    &borderColorOver,
                       bgColFillColorPress,     &fillColorPress,
                       bgColBorderColorPress,   &borderColorPress,
                       bgColBorderWidth,        &b,
                       bgColCornerRadius,       &r,
                       bgColBorderSidesTop,     &sideTop,
                       bgColBorderSidesBottom,  &sideBottom,
                       bgColBorderSidesLeft,    &sideLeft,
                       bgColBorderSidesRight,   &sideRight,
                       bgColGradientId,         &gradient_id,
                       bgColGradientIdOver,     &gradient_id_over,
                       bgColGradientIdPress,    &gradient_id_press,
                       -1);

    gtk_list_store_append(backgrounds, &iter);
    gtk_list_store_set(backgrounds, &iter,
                       bgColPixbuf,             NULL,
                       bgColFillColor,          fillColor,
                       bgColBorderColor,        borderColor,
                       bgColGradientId,         gradient_id,
                       bgColText,               "",
                       bgColFillColorOver,      fillColorOver,
                       bgColBorderColorOver,    borderColorOver,
                       bgColGradientIdOver,     gradient_id_over,
                       bgColFillColorPress,     fillColorPress,
                       bgColBorderColorPress,   borderColorPress,
                       bgColGradientIdPress,    gradient_id_press,
                       bgColBorderWidth,        b,
                       bgColCornerRadius,       r,
                       bgColBorderSidesTop,     sideTop,
                       bgColBorderSidesBottom,  sideBottom,
                       bgColBorderSidesLeft,    sideLeft,
                       bgColBorderSidesRight,   sideRight,
                       -1);
    g_boxed_free(GDK_TYPE_RGBA, fillColor);
    g_boxed_free(GDK_TYPE_RGBA, borderColor);
    g_boxed_free(GDK_TYPE_RGBA, fillColorOver);
    g_boxed_free(GDK_TYPE_RGBA, borderColorOver);
    g_boxed_free(GDK_TYPE_RGBA, fillColorPress);
    g_boxed_free(GDK_TYPE_RGBA, borderColorPress);
    background_update_image(get_model_length(GTK_TREE_MODEL(backgrounds)) - 1);
    gtk_combo_box_set_active(GTK_COMBO_BOX(current_background), get_model_length(GTK_TREE_MODEL(backgrounds)) - 1);
}

void background_delete(GtkWidget *widget, gpointer data)
{
    int index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_background));
    if (index < 0)
        return;

    if (get_model_length(GTK_TREE_MODEL(backgrounds)) <= 1)
        return;

    GtkTreePath *path;
    GtkTreeIter iter;

    path = gtk_tree_path_new_from_indices(index, -1);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(backgrounds), &iter, path);
    gtk_tree_path_free(path);

    gtk_list_store_remove(backgrounds, &iter);

    if (index == get_model_length(GTK_TREE_MODEL(backgrounds)))
        index--;
    gtk_combo_box_set_active(GTK_COMBO_BOX(current_background), index);
}

void background_update_image(int index)
{
    GtkTreePath *path;
    GtkTreeIter iter;

    path = gtk_tree_path_new_from_indices(index, -1);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(backgrounds), &iter, path);
    gtk_tree_path_free(path);

    int w = 70;
    int h = 30;
    int r;
    int b;
    GdkPixbuf *pixbuf;
    GdkRGBA *fillColor;
    GdkRGBA *borderColor;
    int gradient_id;

    gtk_tree_model_get(GTK_TREE_MODEL(backgrounds), &iter,
                       bgColFillColor,      &fillColor,
                       bgColBorderColor,    &borderColor,
                       bgColBorderWidth,    &b,
                       bgColCornerRadius,   &r,
                       bgColGradientId,     &gradient_id,
                       -1);

    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_RGB24, w, h);
    cairo_t *cr = cairo_create(s);
    cairo_set_line_width(cr, b);

    cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    cairo_rectangle(cr, 0, 0, w, h);
    cairo_fill(cr);

    double degrees = G_PI_4 / 45.0;
    double c = r + b,
           rb = r - b;

    // Trying to reproduce exactly same drawing sequence as in panel.
    // Original look is very different from panel look.

    // Blending target
    cairo_push_group (cr);
    cairo_move_to (cr, b,   b);
    cairo_line_to (cr, w-b, b);
    cairo_line_to (cr, w-b, h-b);
    cairo_line_to (cr, b,   h-b);
    cairo_close_path (cr);
    cairo_set_source_rgba(cr, fillColor->red, fillColor->green, fillColor->blue, fillColor->alpha);
    cairo_fill_preserve(cr);
    if (index >= 1 && gradient_id >= 1) {
        GradientConfig *g = (GradientConfig *)g_list_nth(gradients, (guint)gradient_id)->data;
        gradient_draw(cr, g, w, h, TRUE);
    }
    // Clip & draw complete mix
    cairo_pop_group_to_source (cr);
    cairo_new_path (cr);
    cairo_arc(cr, w - c, c,     rb, -90 * degrees,   0 * degrees);
    cairo_arc(cr, w - c, h - c, rb,   0 * degrees,  90 * degrees);
    cairo_arc(cr, c,     h - c, rb,  90 * degrees, 180 * degrees);
    cairo_arc(cr, c,     c,     rb, 180 * degrees, 270 * degrees);
    cairo_fill_preserve (cr);

    // Stich border area
    cairo_new_sub_path (cr);
    cairo_arc(cr, w - c, c,     r, -90 * degrees,   0 * degrees);
    cairo_arc(cr, w - c, h - c, r,   0 * degrees,  90 * degrees);
    cairo_arc(cr, c,     h - c, r,  90 * degrees, 180 * degrees);
    cairo_arc(cr, c,     c,     r, 180 * degrees, 270 * degrees);
    cairo_set_source_rgba(cr, borderColor->red, borderColor->green, borderColor->blue, borderColor->alpha);
    cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
    cairo_set_operator (cr, CAIRO_OPERATOR_ADD);
    cairo_fill(cr);
    cairo_destroy(cr);
    cr = NULL;

    pixbuf = gdk_pixbuf_get_from_surface(s, 0, 0, w, h);

    gtk_list_store_set(backgrounds, &iter, bgColPixbuf, pixbuf, -1);
    if (pixbuf)
        g_object_unref(pixbuf);
    cairo_surface_destroy(s);
    
    g_boxed_free(GDK_TYPE_RGBA, fillColor);
    g_boxed_free(GDK_TYPE_RGBA, borderColor);
}

void background_force_update()
{
    background_update(NULL, NULL);
}

static gboolean background_updates_disabled = FALSE;
void background_update(GtkWidget *widget, gpointer data)
{
    if (background_updates_disabled)
        return;
    int index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_background));
    if (index < 0)
        return;

    GtkTreePath *path;
    GtkTreeIter iter;

    path = gtk_tree_path_new_from_indices(index, -1);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(backgrounds), &iter, path);
    gtk_tree_path_free(path);

    int r;
    int b;

    r = gtk_spin_button_get_value(GTK_SPIN_BUTTON(background_corner_radius));
    b = gtk_spin_button_get_value(GTK_SPIN_BUTTON(background_border_width));

    double fill_weight = gtk_spin_button_get_value(GTK_SPIN_BUTTON(background_fill_content_tint_weight));
    double border_weight = gtk_spin_button_get_value(GTK_SPIN_BUTTON(background_border_content_tint_weight));

    gboolean sideTop = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(background_border_sides_top));
    gboolean sideBottom = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(background_border_sides_bottom));
    gboolean sideLeft = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(background_border_sides_left));
    gboolean sideRight = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(background_border_sides_right));

    GdkRGBA fillColor;
    GdkRGBA borderColor;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(background_fill_color), &fillColor);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(background_border_color), &borderColor);
    int gradient_id = gtk_combo_box_get_active(GTK_COMBO_BOX(background_gradient));

    GdkRGBA fillColorOver;
    GdkRGBA borderColorOver;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(background_fill_color_over), &fillColorOver);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(background_border_color_over), &borderColorOver);
    int gradient_id_over = gtk_combo_box_get_active(GTK_COMBO_BOX(background_gradient_over));

    GdkRGBA fillColorPress;
    GdkRGBA borderColorPress;
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(background_fill_color_press), &fillColorPress);
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(background_border_color_press), &borderColorPress);
    int gradient_id_press = gtk_combo_box_get_active(GTK_COMBO_BOX(background_gradient_press));

    gtk_list_store_set(backgrounds, &iter,
                       bgColPixbuf,             NULL,
                       bgColFillColor,          &fillColor,
                       bgColBorderColor,        &borderColor,
                       bgColGradientId,         gradient_id,
                       bgColFillColorOver,      &fillColorOver,
                       bgColBorderColorOver,    &borderColorOver,
                       bgColGradientIdOver,     gradient_id_over,
                       bgColFillColorPress,     &fillColorPress,
                       bgColBorderColorPress,   &borderColorPress,
                       bgColGradientIdPress,    gradient_id_press,
                       bgColBorderWidth,        b,
                       bgColCornerRadius,       r,
                       bgColBorderSidesTop,     sideTop,
                       bgColBorderSidesBottom,  sideBottom,
                       bgColBorderSidesLeft,    sideLeft,
                       bgColBorderSidesRight,   sideRight,
                       bgColFillWeight,         fill_weight,
                       bgColBorderWeight,       border_weight,
                       -1);
    background_update_image(index);
}

void current_background_changed(GtkWidget *widget, gpointer data)
{
    int index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_background));
    if (index < 0)
        return;

    if (data)
        gtk_widget_set_sensitive(data, index > 0);

    background_updates_disabled = TRUE;

    GtkTreePath *path;
    GtkTreeIter iter;

    path = gtk_tree_path_new_from_indices(index, -1);
    gtk_tree_model_get_iter(GTK_TREE_MODEL(backgrounds), &iter, path);
    gtk_tree_path_free(path);

    int r;
    int b;

    double fill_weight;
    double border_weight;

    gboolean sideTop;
    gboolean sideBottom;
    gboolean sideLeft;
    gboolean sideRight;

    GdkRGBA *fillColor;
    GdkRGBA *borderColor;
    GdkRGBA *fillColorOver;
    GdkRGBA *borderColorOver;
    GdkRGBA *fillColorPress;
    GdkRGBA *borderColorPress;
    int gradient_id, gradient_id_over, gradient_id_press;

    gtk_tree_model_get(GTK_TREE_MODEL(backgrounds), &iter,
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
                       bgColBorderSidesTop,     &sideTop,
                       bgColBorderSidesBottom,  &sideBottom,
                       bgColBorderSidesLeft,    &sideLeft,
                       bgColBorderSidesRight,   &sideRight,
                       bgColFillWeight,         &fill_weight,
                       bgColBorderWeight,       &border_weight,
                       -1);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(background_border_sides_top), sideTop);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(background_border_sides_bottom), sideBottom);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(background_border_sides_left), sideLeft);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(background_border_sides_right), sideRight);

    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(background_fill_color), fillColor);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(background_border_color), borderColor);
    gtk_combo_box_set_active(GTK_COMBO_BOX(background_gradient), gradient_id);

    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(background_fill_color_over), fillColorOver);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(background_border_color_over), borderColorOver);
    gtk_combo_box_set_active(GTK_COMBO_BOX(background_gradient_over), gradient_id_over);

    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(background_fill_color_press), fillColorPress);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(background_border_color_press), borderColorPress);
    gtk_combo_box_set_active(GTK_COMBO_BOX(background_gradient_press), gradient_id_press);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(background_border_width), b);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(background_corner_radius), r);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(background_fill_content_tint_weight), fill_weight);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(background_border_content_tint_weight), border_weight);

    g_boxed_free(GDK_TYPE_RGBA, fillColor);
    g_boxed_free(GDK_TYPE_RGBA, borderColor);
    g_boxed_free(GDK_TYPE_RGBA, fillColorOver);
    g_boxed_free(GDK_TYPE_RGBA, borderColorOver);
    g_boxed_free(GDK_TYPE_RGBA, fillColorPress);
    g_boxed_free(GDK_TYPE_RGBA, borderColorPress);

    background_updates_disabled = FALSE;
    background_update_image(index);
}

void background_update_for_gradient(int gradient_id_changed)
{
    gboolean deleted = gradient_id_changed >= get_model_length(GTK_TREE_MODEL(gradient_ids));
    int current_bg_index = gtk_combo_box_get_active(GTK_COMBO_BOX(current_background));
    for (int index = 1;; index++) {
        GtkTreePath *path;
        GtkTreeIter iter;

        path = gtk_tree_path_new_from_indices(index, -1);
        gboolean found = gtk_tree_model_get_iter(GTK_TREE_MODEL(backgrounds), &iter, path);
        gtk_tree_path_free(path);

        if (!found) {
            break;
        }

        int gradient_id, gradient_id_over, gradient_id_press;

        gtk_tree_model_get(GTK_TREE_MODEL(backgrounds), &iter,
                           bgColGradientId,     &gradient_id,
                           bgColGradientIdOver, &gradient_id_over,
                           bgColGradientIdPress,&gradient_id_press,
                           -1);
        gboolean changed = FALSE;
        if (gradient_id == gradient_id_changed && deleted)
            gradient_id = -1, changed = TRUE;
        if (gradient_id_over == gradient_id_changed && deleted)
            gradient_id_over = -1, changed = TRUE;
        if (gradient_id_press == gradient_id_changed && deleted)
            gradient_id_press = -1, changed = TRUE;
        if (changed) {
            gtk_list_store_set(GTK_LIST_STORE(backgrounds), &iter,
                               bgColGradientId,     gradient_id,
                               bgColGradientIdOver, gradient_id_over,
                               bgColGradientIdPress,gradient_id_press,
                               -1);
            if (index == current_bg_index) {
                gtk_combo_box_set_active(GTK_COMBO_BOX(background_gradient), gradient_id);
                gtk_combo_box_set_active(GTK_COMBO_BOX(background_gradient_over), gradient_id_over);
                gtk_combo_box_set_active(GTK_COMBO_BOX(background_gradient_press), gradient_id_press);
                background_force_update();
            } else {
                background_update_image(index);
            }
        } else {
            background_update_image(index);
        }
    }
}
