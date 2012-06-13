#include "gegl-editor-layer.h"

gint layer_connected_pads (gpointer host, GeglEditor* editor, gint from, gchar* output, gint to, gchar* input)
{
  GeglEditorLayer* layer = (GeglEditorLayer*)host;
  g_print("connected: %s to %s\n", output, input);
}

gint layer_disconnected_pads (gpointer host, GeglEditor* editor, gint from, gchar* output, gint to, gchar* input)
{
  GeglEditorLayer* layer = (GeglEditorLayer*)host;
  g_print("disconnected: %s to %s\n", output, input);
}
//gint (*disconnectedPads) (gpointer host, GeglEditor* editor, gint from, gchar* output, gint to, gchar* input)

GeglEditorLayer*	
layer_create(GeglEditor* editor, GeglNode* gegl)
{
  GeglEditorLayer* layer = malloc(sizeof(GeglEditorLayer));
  editor->host = (gpointer)layer;
  editor->connectedPads = layer_connected_pads;
  editor->disconnectedPads = layer_disconnected_pads;
  layer->editor = editor;
  layer->gegl = gegl;
  layer->pairs = NULL;
  return layer;
}

void
layer_add_gegl_node(GeglEditorLayer* layer, GeglNode* node)
{
  //get input pads
  //gegl_pad_is_output
  GSList *pads = gegl_node_get_input_pads(node);
  guint num_inputs = g_slist_length(pads);
  gchar** inputs = malloc(sizeof(gchar*)*num_inputs);
  int i;
  for(i = 0; pads != NULL; pads = pads->next, i++)
    {
      inputs[i] = gegl_pad_get_name(pads->data);
    }

  gint id;
  if(gegl_node_get_pad(node, "output") == NULL)
    {
      id = gegl_editor_add_node(layer->editor, gegl_node_get_operation(node), num_inputs, inputs, 0, NULL);
    }
  else
    {
      gchar* output = "output";
      gchar* outputs[] = {output};
      id = gegl_editor_add_node(layer->editor, gegl_node_get_operation(node), num_inputs, inputs, 1, outputs);
    }

  node_id_pair* new_pair = malloc(sizeof(node_id_pair));
  new_pair->node = node;
  new_pair->id = id;
  g_slist_append(layer->pairs, new_pair);
}
