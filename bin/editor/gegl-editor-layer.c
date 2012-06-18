#include "gegl-editor-layer.h"
#include <stdlib.h>
#include <stdio.h>

void refresh_images(GeglEditorLayer* self)
{
  GSList*	pair = self->pairs;
  for(;pair != NULL; pair = pair->next)
    {
      node_id_pair	*data = pair->data;

      /*      if(node->image != NULL)
	      cairo_surface_destroy(node->image);	//TODO: only destory if it has changed*/

      const Babl	*cairo_argb32 = babl_format("cairo-ARGB32");

      const GeglRectangle	roi = gegl_node_get_bounding_box(GEGL_NODE(data->node));
      if(roi.width == 0 || roi.height == 0)
	{
	  g_print("Empty rectangle: %s\n", gegl_node_get_operation(GEGL_NODE(data->node)));
	  continue;		//skip
	}

      gegl_editor_show_node_image(self->editor, data->id);

      gint	stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, roi.width);
      guchar*	buf    = malloc(stride*roi.height);

      //make buffer in memory
      gegl_node_blit(GEGL_NODE(data->node),
		     1.0,
		     &roi,
		     cairo_argb32,
		     buf,
		     GEGL_AUTO_ROWSTRIDE,
		     GEGL_BLIT_CACHE);

      cairo_surface_t*	image = 
	cairo_image_surface_create_for_data(buf, CAIRO_FORMAT_ARGB32, 
				      roi.width, roi.height, 
				      stride);
      gegl_editor_set_node_image(self->editor, data->id, image);
    }
}

gint layer_connected_pads (gpointer host, GeglEditor* editor, gint from, gchar* output, gint to, gchar* input)
{
  GeglEditorLayer*	self = (GeglEditorLayer*)host;

  GeglNode*	from_node = NULL;
  GeglNode*	to_node	  = NULL;
  GSList*	pair	  = self->pairs;
  for(;pair != NULL; pair = pair->next)
    {
      node_id_pair*	data = pair->data;
      if(data->id == from)
	from_node	     = data->node;
      if(data->id == to)
	to_node		     = data->node;
      if(from_node != NULL && to_node != NULL)
	break;
    }
  
  g_assert(from_node != NULL && to_node != NULL);
  g_assert(from_node != to_node);
  gboolean	success = gegl_node_connect_to(from_node, output, to_node, input);
  g_print("connected: %s(%s) to %s(%s), %i\n", gegl_node_get_operation(from_node), output,
	  gegl_node_get_operation(to_node), input, success);  
  refresh_images(self);
}

gint layer_disconnected_pads (gpointer host, GeglEditor* editor, gint from, gchar* output, gint to, gchar* input)
{
  GeglEditorLayer*	layer = (GeglEditorLayer*)host;
  g_print("disconnected: %s to %s\n", output, input);
  //TODO: disconnect in GEGL as well
}

struct text_prop_data
{
  GeglNode* node;
  gchar* property;
  GType prop_type;
  GeglEditorLayer* layer;
};

void text_property_changed(GtkEntry* entry, gpointer data)
{
  struct text_prop_data *dat = (struct text_prop_data*)data;
  gchar *text = gtk_entry_get_text(entry);

  GeglNode* node = dat->node;
  gchar* property = dat->property;
  GType prop_type = dat->prop_type;
  GeglEditorLayer *layer = dat->layer;

  g_print("%s -> %s\n", property, text);

  gint i_value;
  gdouble d_value;
  gchar *str_value;
  GValue value = { 0, }; //different than = 0?
  g_value_init(&value, prop_type);
  g_print("%s\n", G_VALUE_TYPE_NAME(&value));

  switch(prop_type)
    {
    case G_TYPE_INT:
      i_value = (gint)strtod(text, NULL);
      gegl_node_set(node, property, i_value, NULL);
      break;
    case G_TYPE_DOUBLE:
      d_value = strtod(text, NULL);
      gegl_node_set(node, property, d_value, NULL);
      break;
    case G_TYPE_STRING:
      gegl_node_set(node, property, text, NULL);
      break;
    default:
      g_print("Unknown property type: %s (%s)\n", property, g_type_name(prop_type));
    }

  refresh_images(layer);
}

gint layer_node_selected (gpointer host, GeglEditor* editor, gint node_id)
{
  GeglEditorLayer*	self = (GeglEditorLayer*)host;
  GeglNode*		node = NULL;
  GSList*		pair = self->pairs;
  for(;pair != NULL; pair = pair->next)
    {
      node_id_pair*	data = pair->data;
      if(data->id == node_id)
	{
	  node = data->node;
	  break;
	}
    }

  g_assert(node != NULL);

  g_print("selected: %s\n", gegl_node_get_operation(node));

  guint		n_props;
  GParamSpec**	properties = gegl_operation_list_properties(gegl_node_get_operation(node), &n_props);

  //prop_box *should* only have one child which was added by the editor layer, but clear it anyway
  /*GList *children, *iter;

  children = gtk_container_get_children(GTK_CONTAINER(self->prop_box));
  for(iter = children; iter != NULL; iter = g_list_next(iter))
    gtk_widget_destroy(GTK_WIDGET(iter->data));
    g_list_free(children);*/

  //TODO: only create enough columns for the properties which will actually be included (i.e. ignoring GeglBuffer props)
  GtkTable	*prop_table = gtk_table_new(2, n_props, FALSE);

  int	i;
  for(i = 0; i < n_props; i++)
    {
      GParamSpec*	prop = properties[i];
      GType		type = prop->value_type;
      gchar*		name = prop->name;

      GtkWidget*	name_label = gtk_label_new(name);
      gtk_misc_set_alignment(GTK_MISC(name_label), 0, 0.5);
      gtk_table_attach(prop_table, name_label, 0, 1, i, i+1, GTK_FILL, GTK_FILL, 1, 1);

      GtkWidget*	value_entry = gtk_entry_new(); //TODO: populate this with the existing property value

      gchar buf[256] = "*"; //can probably be smaller; In fact, can probably do this without sprintf and a buffer. TODO: look at g_string

      gint i_value;
      gdouble d_value;
      gchar* str_value;

      switch(type)
	{
	case G_TYPE_INT:
	  gegl_node_get(node, name, &i_value, NULL);
	  sprintf(buf, "%d", i_value);
	  break;
	case G_TYPE_DOUBLE:
	  gegl_node_get(node, name, &d_value, NULL);
	  sprintf(buf, "%f", d_value);
	  break;
	case G_TYPE_STRING:
	  gegl_node_get(node, name, &str_value, NULL);
	  sprintf(buf, "%s", str_value);
	  break;
	  //	default:
	  //	  g_print("Unknown property type: %s (%s)\n", property, g_type_name(prop_type));
	}

      gtk_entry_set_text(value_entry, buf);

      gtk_entry_set_width_chars(GTK_ENTRY(value_entry), 2);
      struct text_prop_data *data = malloc(sizeof(struct text_prop_data)); //TODO store this in a list and free it when the node is deselected
      data->node = node;
      data->property = name;
      data->prop_type = type;
      data->layer = self;
      g_signal_connect(value_entry, "activate", text_property_changed, data);

      gtk_table_attach(prop_table, value_entry, 1, 2, i, i+1, GTK_EXPAND | GTK_FILL | GTK_SHRINK, GTK_FILL, 1, 1);
    }

  gtk_box_pack_start(GTK_BOX(self->prop_box), prop_table, TRUE, TRUE, 0);
  gtk_widget_show_all(self->prop_box);
}

gint layer_node_deselected(gpointer host, GeglEditor* editor, gint node)
{
  GeglEditorLayer*	 self = (GeglEditorLayer*)host;
  GList			*children, *iter;

  children = gtk_container_get_children(GTK_CONTAINER(self->prop_box));
  for(iter = children; iter != NULL; iter = g_list_next(iter))
    gtk_widget_destroy(GTK_WIDGET(iter->data));
  g_list_free(children);
}

/*  gint (*nodeSelected) (gpointer host, GeglEditor* editor, gint node);
    gint (*nodeDeselected) (gpointer host, GeglEditor* editor, gint node);*/

GeglEditorLayer*	
layer_create(GeglEditor* editor, GeglNode* gegl, GtkWidget* prop_box)
{
  GeglEditorLayer*	layer = malloc(sizeof(GeglEditorLayer));
  editor->host		      = (gpointer)layer;
  editor->connectedPads	      = layer_connected_pads;
  editor->disconnectedPads    = layer_disconnected_pads;
  editor->nodeSelected	      = layer_node_selected;
  editor->nodeDeselected      = layer_node_deselected;
  layer->editor		      = editor;
  layer->gegl		      = gegl;
  layer->pairs		      = NULL;
  layer->prop_box	      = prop_box;
  return layer;
}

void
layer_add_gegl_node(GeglEditorLayer* layer, GeglNode* node)
{
  //get input pads
  //gegl_pad_is_output
  GSList	*pads	    = gegl_node_get_input_pads(node);
  guint		 num_inputs = g_slist_length(pads);
  gchar**	 inputs	    = malloc(sizeof(gchar*)*num_inputs);
  int		 i;
  for(i = 0; pads != NULL; pads = pads->next, i++)
    {
      inputs[i] = gegl_pad_get_name(pads->data);
    }

  gint	id;
  if(gegl_node_get_pad(node, "output") == NULL)
    {
      id = gegl_editor_add_node(layer->editor, gegl_node_get_operation(node), num_inputs, inputs, 0, NULL);
    }
  else
    {
      gchar*	output = "output";
      gchar* outputs[] = {output};
      id	       = gegl_editor_add_node(layer->editor, gegl_node_get_operation(node), num_inputs, inputs, 1, outputs);
    }

  node_id_pair* new_pair = malloc(sizeof(node_id_pair));
  new_pair->node	 = node;
  new_pair->id		 = id;
  layer->pairs		 = g_slist_append(layer->pairs, new_pair);
}
