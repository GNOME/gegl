#include "gegl-editor-layer.h"

GeglEditorLayer*	
layer_create(GeglEditor* editor, GeglNode* gegl)
{
  GeglEditorLayer* layer = malloc(sizeof(GeglEditorLayer));
  layer->editor = editor;
  layer->gegl = gegl;
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
  gegl_editor_add_node(layer->editor, gegl_node_get_operation(node), num_inputs, inputs, 0, NULL);
}
