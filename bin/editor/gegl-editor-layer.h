#ifndef __EDITORLAYER_H__
#define __EDITORLAYER_H__

#include	"gegl-node-widget.h"
#include <gegl.h>
#include <glib.h>
#include <gegl-gtk.h>

/*	
Creates and removes connections between pads in the Gegl graph 
as they are created and removed by the user in the editor widget
Note: only one layer can be safely used per editor widget
The user should not link, unlink, add, or remove any operations outside of the layer interface
*/
typedef struct _GeglEditorLayer GeglEditorLayer;

typedef struct _node_id_pair
{
  GeglNode*	node;
  gint		id;
} node_id_pair;

struct _GeglEditorLayer
{
  GeglEditor	*editor;
  GeglNode	*gegl;
  GtkWidget	*prop_box;	//container for the property editor
  GSList	*pairs;
};

/* 
Editor and gegl graph should both be empty, but properly initialized 
*/
GeglEditorLayer*	layer_create(GeglEditor* editor, GeglNode* gegl, GtkWidget* property_editor_container);
void			layer_add_gegl_node(GeglEditorLayer* layer, GeglNode* node);
void layer_set_graph(GeglEditorLayer* layer, GeglNode* gegl); //will clear the current graph and load in the new one
//void layer_remove_gegl_node(GeglNode* node);
//link, unlink

void
gegl_node_disconnect_all_pads(GeglNode* node);

#endif
