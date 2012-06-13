#include "gegl-editor-layer.h"

gint layer_connected_pads (gpointer host, GeglEditor* editor, gint from, gchar* output, gint to, gchar* input)
{
  GeglEditorLayer*	layer = (GeglEditorLayer*)host;

  GeglNode*	from_node = NULL;
  GeglNode*	to_node	  = NULL;
  GSList*	pair	  = layer->pairs;
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
  gboolean success = gegl_node_connect_to(from_node, output, to_node, input);
  g_print("connected: %s(%s) to %s(%s), %i\n", gegl_node_get_operation(from_node), output,
	  gegl_node_get_operation(to_node), input, success);  
}

gint layer_disconnected_pads (gpointer host, GeglEditor* editor, gint from, gchar* output, gint to, gchar* input)
{
  GeglEditorLayer*	layer = (GeglEditorLayer*)host;
  g_print("disconnected: %s to %s\n", output, input);
}
//gint (*disconnectedPads) (gpointer host, GeglEditor* editor, gint from, gchar* output, gint to, gchar* input)

GeglEditorLayer*	
layer_create(GeglEditor* editor, GeglNode* gegl)
{
  GeglEditorLayer*	layer = malloc(sizeof(GeglEditorLayer));
  editor->host		      = (gpointer)layer;
  editor->connectedPads	      = layer_connected_pads;
  editor->disconnectedPads    = layer_disconnected_pads;
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
