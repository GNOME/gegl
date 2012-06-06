#include <glib.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include <gegl.h>

#include "gegl-node-widget.h"


gint
main (gint	  argc,
      gchar	**argv)
{
  GtkWidget	*window;
  GtkWidget	*editor;

  gtk_init(&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  editor = gegl_node_widget_new ();
  gtk_container_add(GTK_CONTAINER(window), editor);

  g_signal_connect (window, "destroy", G_CALLBACK( gtk_main_quit), NULL);

  gtk_widget_show(editor);
  gtk_widget_show(window);

  gtk_main();
  
  return 0;
}
