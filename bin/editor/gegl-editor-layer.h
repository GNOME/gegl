#ifndef __EDITORLAYER_H__
#define __EDITORLAYER_H__

#include "gegl-node-widget.h"
#include <gegl.h>
typedef struct _GeglOperation GeglOperation;
#include <operation/gegl-operation.h>

/*	
Creates and removes connections between pads in the Gegl graph 
as they are created and removed by the user in the editor widget
Note: only one layer can be safely used per editor widget
The user should not link, unlink, add, or remove any operations outside of the layer interface
*/
typedef struct _GeglEditorLayer GeglEditorLayer;

struct _GeglEditorLayer
{
  GeglEditor	*editor;
  GeglNode	*gegl;
};

/* 
Editor and gegl graph should both be empty, but properly initialized 
*/
GeglEditorLayer*	layer_create(GeglEditor* editor, GeglEditor* gegl);
void			layer_add_gegl_op(GeglEditorLayer* layer, GeglOperation* op);
//void layer_remove_gegl_node(GeglNode* node);
//link, unlink

#endif
