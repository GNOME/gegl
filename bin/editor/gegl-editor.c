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

  editor = gegl_editor_new ();
  gtk_container_add(GTK_CONTAINER(window), editor);

  g_signal_connect (window, "destroy", G_CALLBACK( gtk_main_quit), NULL);

  gtk_widget_show(editor);
  gtk_widget_show(window);

  GeglEditor* node_editor = GEGL_EDITOR(editor);
  gchar *inputs[2];
  inputs[0] = "Input1";
  inputs[1] = "Input2";

  gint my_node = gegl_editor_add_node(node_editor, "New Node", 2, inputs, 2, inputs);
  gegl_editor_set_node_position(node_editor, my_node, 100, 0);

  gtk_main();
  
  return 0;
}
