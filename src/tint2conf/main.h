#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <locale.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#ifdef GETTEXT_PACKAGE
#include <glib/gi18n-lib.h>
#else
#define _(String) String
#define GETTEXT_PACKAGE "tint2conf"
#endif

#ifndef GTK_TYPE_INT
#define GTK_TYPE_INT G_TYPE_INT
#endif

#ifndef GTK_TYPE_STRING
#define GTK_TYPE_STRING G_TYPE_STRING
#endif

#ifndef GTK_TYPE_BOOL
#define GTK_TYPE_BOOL G_TYPE_BOOLEAN
#endif

#ifndef GTK_TYPE_DOUBLE
#define GTK_TYPE_DOUBLE G_TYPE_DOUBLE
#endif

#define gtk_tooltips_set_tip(t, widget, txt, arg) gtk_widget_set_tooltip_text(widget, txt)

#define GTK_OBJECT(x) (x)
#define GTK_SIGNAL_FUNC G_CALLBACK

#define SNAPSHOT_TICK 190
gboolean update_snapshot(gpointer ignored);
void menuApply();
void refresh_current_theme();
extern GtkWidget *g_window;
