#include <glib.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include <gegl.h>

#include "gegl-node-widget.h"
#include "gegl-editor-layer.h"

gint
main (gint	  argc,
      gchar	**argv)
{
  gtk_init(&argc, &argv);

  GtkWidget	*window;
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

  GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(window), vbox);


  //create the menubar
  GtkWidget *menubar;
  GtkWidget *process_menu;
  GtkWidget *process;
  GtkWidget *process_all;

  menubar = gtk_menu_bar_new();
  process_menu = gtk_menu_new();

  process = gtk_menu_item_new_with_label("Process");
  process_all = gtk_menu_item_new_with_label("All");

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(process), process_menu);
  gtk_menu_shell_append(GTK_MENU_SHELL(process_menu), process_all);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), process);

  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
  //  gtk_widget_show(menubar);

  GtkWidget	*editor;
  editor = gegl_editor_new ();
  //  gtk_container_add(GTK_CONTAINER(vbox), editor);
  gtk_box_pack_start(GTK_BOX(vbox), editor, TRUE, TRUE, 0);

  g_signal_connect (window, "destroy", G_CALLBACK( gtk_main_quit), NULL);

  //  gtk_widget_show(editor);
  gtk_widget_show_all(window);

  GeglEditor* node_editor = GEGL_EDITOR(editor);

  gegl_init(&argc, &argv);
  
  //add some samples nodes
  GeglNode *gegl = gegl_node_new();
  GeglEditorLayer* layer = layer_create(node_editor, gegl);

  GeglNode *display    = gegl_node_create_child (gegl, "gegl:display");
  layer_add_gegl_node(layer, display);

  GeglNode *over       = gegl_node_new_child (gegl,
                                 "operation", "gegl:over",
                                 NULL);
  layer_add_gegl_node(layer, over);


  gtk_main();
  
  return 0;
}
