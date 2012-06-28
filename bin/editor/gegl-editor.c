#include <glib.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include <gegl.h>

#include "gegl-node-widget.h"
#include "gegl-editor-layer.h"

GtkWidget	*window;

/*void add_operation_dialog (GtkDialog* dialog, gint response_id, gpointer user_data)
{
  if(response_id == GTK_RESPONSE_ACCEPT) {
    g_print("add\n");
  }
  }*/

void menuitem_activated(GtkMenuItem* item, gpointer data)
{
  GeglEditorLayer	*layer = (GeglEditorLayer*)data;
  if(0 == strcmp("Add Operation", gtk_menu_item_get_label(item)))
    {
      g_print("Add an operation\n");
    }
  GtkWidget *add_op_dialog = gtk_dialog_new_with_buttons("AddOperation", GTK_WINDOW(window), 
							 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							 GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT, NULL);

  /////list/////////
  GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);

  guint n_ops;
  gchar** ops = gegl_list_operations(&n_ops);

  GtkTreeIter itr;

  int i;
  for(i = 0; i < n_ops; i++) {
    gtk_list_store_append(store, &itr);
    gtk_list_store_set(store, &itr, 0, ops[i], -1);
  }

  /////////////////

  GtkWidget *text_entry = gtk_entry_new();
  
  //  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(add_op_dialog))), text_entry);
  //  gtk_widget_show(text_entry);
  
  GtkWidget* list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Operation",
						     renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  GtkScrolledWindow* scrolls = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_size_request(GTK_WIDGET(scrolls), 100, 150);
  gtk_widget_show(scrolls);
  gtk_container_add(GTK_CONTAINER(scrolls), list);

  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(add_op_dialog))), scrolls);
  gtk_widget_show(list);

  //g_signal_connect(add_op_dialog, "response", add_operation_dialog, data);

  gint result = gtk_dialog_run(GTK_DIALOG(add_op_dialog));
  GeglNode *node;
  if(result == GTK_RESPONSE_ACCEPT)
    {
      GtkTreeSelection *selection = gtk_tree_view_get_selection(list);
      GtkTreeModel *model;
      if(gtk_tree_selection_get_selected(selection, &model, &itr))
	{
	  gchar* operation;
	  gtk_tree_model_get(model, &itr, 0, &operation, -1);
	  //	  node = gegl_node_create_child (layer->gegl, gtk_entry_get_text(GTK_ENTRY(text_entry)));
	  node = gegl_node_create_child(layer->gegl, operation);
	  layer_add_gegl_node(layer, node);
	}
    }
  gtk_widget_destroy(add_op_dialog);
}

gint
main (gint	  argc,
      gchar	**argv)
{
  gtk_init(&argc, &argv);

  GtkWidget	*editor	     = gegl_editor_new ();
  GeglEditor*	 node_editor = GEGL_EDITOR(editor);

  gegl_init(&argc, &argv);

////////////////////////////////////////////CREATE OPERATION DIALOG///////////////////////////////////////////

  GtkWidget* property_box = gtk_vbox_new(TRUE, 0);


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  //add some samples nodes
  GeglNode		*gegl  = gegl_node_new();
  GeglEditorLayer*	 layer = layer_create(node_editor, gegl, property_box);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
  g_signal_connect (window, "destroy", G_CALLBACK( gtk_main_quit), NULL);

  GtkWidget	*vbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(window), vbox);


/////////////////////////////////////////////////BUILD MENUBAR////////////////////////////////////////////////
 
 GtkWidget	*menubar;
  
  GtkWidget	*process_menu;
  GtkWidget	*process;
  GtkWidget	*process_all;

  GtkWidget	*graph_menu;
  GtkWidget	*graph;
  GtkWidget	*add_operation;

  menubar      = gtk_menu_bar_new();
  process_menu = gtk_menu_new();
  graph_menu   = gtk_menu_new();

  process     = gtk_menu_item_new_with_label("Process");
  process_all = gtk_menu_item_new_with_label("All");

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(process), process_menu);
  gtk_menu_shell_append(GTK_MENU_SHELL(process_menu), process_all);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), process);

  graph		= gtk_menu_item_new_with_label("Graph");
  add_operation = gtk_menu_item_new_with_label("Add Operation");
  g_signal_connect(add_operation, "activate", (GCallback)menuitem_activated, layer);

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(graph), graph_menu);
  gtk_menu_shell_append(GTK_MENU_SHELL(graph_menu), add_operation);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), graph);


////////////////////////////////////////////HORIZONTAL PANE///////////////////////////////////////////////////

  GtkWidget* pane = gtk_hpaned_new();
  gtk_paned_set_position(GTK_PANED(pane), 150);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), pane, TRUE, TRUE, 0);

  gtk_paned_pack1(GTK_PANED(pane), property_box, TRUE, FALSE);
  gtk_paned_pack2(GTK_PANED(pane), editor, TRUE, TRUE);

  gtk_widget_show_all(window);

////////////////////////////////////////////GEGL OPERATIONS///////////////////////////////////////////////////

  GeglNode	*display = gegl_node_create_child (gegl, "gegl:display");
  GeglNode	*over = gegl_node_new_child (gegl, "operation", "gegl:over", NULL);
  GeglNode	*load = gegl_node_new_child(gegl, "operation", "gegl:load", "path", "./surfer.png", NULL);
  GeglNode	*text = gegl_node_new_child(gegl, "operation", "gegl:text", "size", 10.0, "color", 
					    gegl_color_new("rgb(1.0,1.0,1.0)"), "text", "Hello world!", NULL);

  layer_add_gegl_node(layer, display);
  layer_add_gegl_node(layer, over);
  layer_add_gegl_node(layer, load);
  layer_add_gegl_node(layer, text);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

  g_print("%s\n", G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(load)));
  
  gtk_main();
  
  return 0;
}
