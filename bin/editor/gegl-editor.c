#include <glib.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include <gegl.h>

#include "gegl-node-widget.h"
#include "gegl-editor-layer.h"

GtkWidget	*window;

static void print_info(GeglNode* gegl)
{
  GSList *list = gegl_node_get_children(gegl);
  for(;list != NULL; list = list->next)
    {
      GeglNode* node = GEGL_NODE(list->data);
      g_print("Node %s\n", gegl_node_get_operation(node));

      GeglNode** nodes;
      const gchar** pads;
      gint num = gegl_node_get_consumers(node, "output", &nodes, &pads);

      int i;
      g_print("%s: %d consumer(s)\n", gegl_node_get_operation(node), num);
      for(i = 0; i < num; i++)
	{
	  g_print("Connection: (%s to %s)\n", gegl_node_get_operation(node), gegl_node_get_operation(nodes[0]), pads[0]);
	}
    }
}

GeglNode* getFinalNode(GeglNode* node)
{
  if(gegl_node_find_property(node, "output") == NULL)
    return node;

  GeglNode**	nodes;
  const gchar** pads;
  gint		num_consumers = gegl_node_get_consumers(node, "output", &nodes, &pads);
  
  if(num_consumers == 0)
    return node;
  else
    return getFinalNode(nodes[0]);
}


GeglNode* getFirstNode(GeglNode* node)
{
  GeglNode* prev_node = gegl_node_get_producer(node, "input", NULL);
  if(prev_node == NULL)
    return node;
  else
    return getFirstNode(prev_node);
}

GeglNode* gegl_node_get_nth_child(GeglNode*, gint);

void SaveAs(GeglEditorLayer* layer)
{
  print_info(layer->gegl);
  GtkFileChooserDialog	*file_select = GTK_FILE_CHOOSER_DIALOG(gtk_file_chooser_dialog_new("Save As", GTK_WINDOW(window), 
											   GTK_FILE_CHOOSER_ACTION_SAVE, 
											   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
											   GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
											   NULL));
  gint			 result	     = gtk_dialog_run(GTK_DIALOG(file_select));
  if(result == GTK_RESPONSE_ACCEPT)
    {
      const gchar	*filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_select));

      //Try to guess at an output
      GeglNode* first = gegl_node_get_nth_child(layer->gegl, 0);
      GeglNode* last = getFinalNode(first);

      g_print("Final node: %s\n", gegl_node_get_operation(last));
      const gchar	*xml = gegl_node_to_xml(last, "/");

      g_print("%s\n", filename);

      g_file_set_contents(filename, xml, -1, NULL);	//TODO: check for error
    }

  gtk_widget_destroy(GTK_WIDGET(file_select));
}

void Open(GeglEditorLayer* layer)
{
  GtkFileChooserDialog	*file_select = GTK_FILE_CHOOSER_DIALOG(gtk_file_chooser_dialog_new("Save As", GTK_WINDOW(window), 
											   GTK_FILE_CHOOSER_ACTION_OPEN, 
											   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
											   GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
											   NULL));
  gint			 result	     = gtk_dialog_run(GTK_DIALOG(file_select));
  if(result == GTK_RESPONSE_ACCEPT)
    {
      const gchar	*filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(file_select));
      GeglNode* gegl = gegl_node_new_from_file(filename);
      layer_set_graph(layer, gegl);
      //clear the current project, load in a new gegl object
    }

  gtk_widget_destroy(GTK_WIDGET(file_select));
}

void file_menu_item_activated(GtkMenuItem* item, gpointer data)
{
  const gchar*	label = gtk_menu_item_get_label(item);
  if(0 == g_strcmp0(label, "Save As"))
    {
      SaveAs((GeglEditorLayer*)data);
    }
  else if(0 == g_strcmp0( label, "Save"))
    {
      //Check to see if open graph is associated with a file. If it is save to that	file, otherwise, save as
      SaveAs((GeglEditorLayer*)data);
    }
  else if(0 == g_strcmp0(label, "Open"))
    {
      Open((GeglEditorLayer*)data);
    }
  else if(0 == g_strcmp0(label, "New Graph"))
    {
      
    }
  else if(0 == g_strcmp0(label, "Quit"))
    {
    }
}

void process_activated(GtkMenuItem* item, gpointer data)
{
}

void process_all_activated(GtkMenuItem* item, gpointer data)
{
}

void add_operation_activated(GtkMenuItem* item, gpointer data)
{
  GeglEditorLayer	*layer = (GeglEditorLayer*)data;
  if(0 == strcmp("Add Operation", gtk_menu_item_get_label(item)))
    {
      g_print("Add an operation\n");
    }
  GtkWidget	*add_op_dialog = gtk_dialog_new_with_buttons("AddOperation", GTK_WINDOW(window), 
							     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							     GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL,GTK_RESPONSE_REJECT, NULL);
  /////list/////////
  GtkListStore	*store	       = gtk_list_store_new(1, G_TYPE_STRING);

  guint		n_ops;
  gchar**	ops = gegl_list_operations(&n_ops);

  GtkTreeIter	itr;

  int	i;
  for(i = 0; i < n_ops; i++) {
    gtk_list_store_append(store, &itr);
    gtk_list_store_set(store, &itr, 0, ops[i], -1);
  }

  /////////////////

  GtkWidget	*text_entry = gtk_entry_new();
  
  //  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(add_op_dialog))), text_entry);
  //  gtk_widget_show(text_entry);
  
  GtkWidget*	list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

  GtkCellRenderer	*renderer;
  GtkTreeViewColumn	*column;

  renderer = gtk_cell_renderer_text_new ();
  column   = gtk_tree_view_column_new_with_attributes ("Operation",
						       renderer,
						       "text", 0,
						       NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);

  GtkScrolledWindow*	scrolls = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
  gtk_widget_set_size_request(GTK_WIDGET(scrolls), 100, 150);
  gtk_widget_show(GTK_WIDGET(scrolls));
  gtk_container_add(GTK_CONTAINER(scrolls), list);

  gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(add_op_dialog))), GTK_WIDGET(scrolls));
  gtk_widget_show(list);

  //g_signal_connect(add_op_dialog, "response", add_operation_dialog, data);

  gint		 result = gtk_dialog_run(GTK_DIALOG(add_op_dialog));
  GeglNode	*node;
  if(result == GTK_RESPONSE_ACCEPT)
    {
      GtkTreeSelection	*selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
      GtkTreeModel	*model;
      if(gtk_tree_selection_get_selected(selection, &model, &itr))
	{
	  gchar*	operation;
	  gtk_tree_model_get(model, &itr, 0, &operation, -1);
	  //	  node = gegl_node_create_child (layer->gegl, gtk_entry_get_text(GTK_ENTRY(text_entry)));
	  node	       = gegl_node_create_child(layer->gegl, operation);
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

  GtkWidget*	property_box = gtk_vbox_new(FALSE, 0);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  //add some samples nodes
  GeglNode		*gegl  = gegl_node_new();
  GeglEditorLayer*	 layer = layer_create(node_editor, NULL, property_box);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
  g_signal_connect (window, "destroy", G_CALLBACK( gtk_main_quit), NULL);

  GtkWidget	*vbox = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox);
  gtk_container_add(GTK_CONTAINER(window), vbox);


  /////////////////////////////////////////////////BUILD MENUBAR////////////////////////////////////////////////
 
  GtkWidget	*menubar;

  GtkWidget	*file_menu;
  GtkWidget	*file;
  GtkWidget	*new;
  GtkWidget	*open;
  GtkWidget	*save;
  GtkWidget	*save_as;
  GtkWidget	*exit;
  
  GtkWidget	*graph_menu;
  GtkWidget	*graph;
  GtkWidget	*add_operation;
  GtkWidget	*process;
  GtkWidget	*process_all;

  menubar = gtk_menu_bar_new();

  //File Menu
  file_menu = gtk_menu_new();
  file	    = gtk_menu_item_new_with_label("File");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(file), file_menu);

  new = gtk_menu_item_new_with_label("New Graph");
  g_signal_connect(new, "activate", (GCallback)file_menu_item_activated, layer);
  gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), new);

  open = gtk_menu_item_new_with_label("Open");
  g_signal_connect(open, "activate", (GCallback)file_menu_item_activated, layer);
  gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), open);

  save = gtk_menu_item_new_with_label("Save");
  g_signal_connect(save, "activate", (GCallback)file_menu_item_activated, layer);
  gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), save);

  save_as = gtk_menu_item_new_with_label("Save As");
  g_signal_connect(save_as, "activate", (GCallback)file_menu_item_activated, layer);
  gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), save_as);

  //Graph Menu
  graph_menu = gtk_menu_new();
  graph	     = gtk_menu_item_new_with_label("Graph");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(graph), graph_menu);

  add_operation = gtk_menu_item_new_with_label("Add Operation");
  g_signal_connect(add_operation, "activate", (GCallback)add_operation_activated, layer);
  gtk_menu_shell_append(GTK_MENU_SHELL(graph_menu), add_operation);

  process = gtk_menu_item_new_with_label("Process");
  g_signal_connect(process, "activate", (GCallback)process_activated, layer);
  gtk_menu_shell_append(GTK_MENU_SHELL(graph_menu), process);

  process_all = gtk_menu_item_new_with_label("Process All");
  g_signal_connect(process_all, "activate", (GCallback)process_all_activated, layer);
  gtk_menu_shell_append(GTK_MENU_SHELL(graph_menu), process_all);

  //Add menues to menu bar

  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), graph);

  ////////////////////////////////////////////HORIZONTAL PANE///////////////////////////////////////////////////

  GtkWidget*	pane = gtk_hpaned_new();
  gtk_paned_set_position(GTK_PANED(pane), 150);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////

  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), pane, TRUE, TRUE, 0);

  gtk_paned_pack1(GTK_PANED(pane), property_box, TRUE, FALSE);
  gtk_paned_pack2(GTK_PANED(pane), editor, TRUE, TRUE);

  gtk_widget_show_all(window);

  ////////////////////////////////////////////GEGL OPERATIONS///////////////////////////////////////////////////
  //GeglNode	*display = gegl_node_create_child (gegl, "gegl:display");
  GeglNode	*over	 = gegl_node_new_child (gegl, "operation", "gegl:over", NULL);
  GeglNode	*load	 = gegl_node_new_child(gegl, "operation", "gegl:load", "path", "./surfer.png", NULL);
  GeglNode	*text	 = gegl_node_new_child(gegl, "operation", "gegl:text", "size", 10.0, "color", 
					       gegl_color_new("rgb(1.0,1.0,1.0)"), "text", "Hello world!", NULL);

  gegl_node_link(load, over);

  //layer_add_gegl_node(layer, display);
  /*  layer_add_gegl_node(layer, over);
  layer_add_gegl_node(layer, load);
  layer_add_gegl_node(layer, text);*/

  layer_set_graph(layer, gegl);


  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

  gtk_main();
  
  return 0;
}
