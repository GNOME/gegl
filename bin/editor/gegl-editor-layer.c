#include "gegl-editor-layer.h"

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

  guint n_props;
  GParamSpec** properties = gegl_operation_list_properties(gegl_node_get_operation(node), &n_props);
  int i;
  for(i = 0; i < n_props; i++)
    {
      GParamSpec* prop = properties[i];
      g_print("%s (%s)\n", prop->name, g_type_name(prop->value_type));
    }
}

/*  gint (*nodeSelected) (gpointer host, GeglEditor* editor, gint node);
    gint (*nodeDeselected) (gpointer host, GeglEditor* editor, gint node);*/

GeglEditorLayer*	
layer_create(GeglEditor* editor, GeglNode* gegl)
{
  GeglEditorLayer*	layer = malloc(sizeof(GeglEditorLayer));
  editor->host		      = (gpointer)layer;
  editor->connectedPads	      = layer_connected_pads;
  editor->disconnectedPads    = layer_disconnected_pads;
  editor->nodeSelected	      = layer_node_selected;
  layer->editor		      = editor;
  layer->gegl		      = gegl;
  layer->pairs		      = NULL;
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
