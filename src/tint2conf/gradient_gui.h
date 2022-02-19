#ifndef GRADIENT_GUI_H
#define GRADIENT_GUI_H

#include "gui.h"

int gradient_index_safe(int index);
void create_gradient(GtkWidget *parent);
void gradient_page_finalize ();
GtkWidget *create_gradient_combo();
void gradient_duplicate(GtkWidget *widget, gpointer data);
void gradient_delete(GtkWidget *widget, gpointer data);
void gradient_update_image(int index);
void gradient_update(GtkWidget *widget, gpointer data);
void gradient_force_update();
void current_gradient_changed(GtkWidget *widget, gpointer data);
void background_update_for_gradient(int gradient_id);

GtkWidget *create_gradient_stop_combo();
void gradient_stop_duplicate(GtkWidget *widget, gpointer data);
void gradient_stop_delete(GtkWidget *widget, gpointer data);
void gradient_stop_update(GtkWidget *widget, gpointer data);
void gradient_stop_update_image(int index);
void current_gradient_stop_changed(GtkWidget *widget, gpointer data);

typedef enum GradientConfigType {
    GRADIENT_CONFIG_VERTICAL,
    GRADIENT_CONFIG_HORIZONTAL,
    GRADIENT_CONFIG_RADIAL
} GradientConfigType;

typedef struct GradientConfigColorStop {
    Color color;
    double offset;  // offset in 0-1
} GradientConfigColorStop;

typedef struct GradientConfig {
    GradientConfigType type;
    GradientConfigColorStop start_color;
    GradientConfigColorStop end_color;
    GList *extra_color_stops;   // Each element is a GradientConfigColorStop
} GradientConfig;

void gradient_create_new(GradientConfigType t);
void gradient_draw(cairo_t *c, GradientConfig *g, int w, int h, gboolean preserve);

#endif
