#include "gegl-editor-layer.h"

GeglEditorLayer*	
layer_create(GeglEditor* editor, GeglEditor* gegl)
{
  GeglEditorLayer* layer = malloc(sizeof(GeglEditorLayer));
  layer->editor = editor;
  layer->gegl = gegl;
  return layer;
}

void			
layer_add_gegl_op(GeglEditorLayer* layer, GeglOperation* node)
{
  gegl_editor_add_node(layer->editor, gegl_operation_get_name(operation), 0, NULL, 0, NULL);
}
