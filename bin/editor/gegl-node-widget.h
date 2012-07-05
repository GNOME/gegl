/* gegl-node-widget.h */

#ifndef __NODEWIDGET_H__
#define __NODEWIDGET_H__

#include	<gtk/gtk.h>
#include <glib-object.h>
#include <stdlib.h>

#define GEGL_TYPE_EDITOR		(gegl_editor_get_type())
#define GEGL_EDITOR(obj)		(G_TYPE_CHECK_INSTANCE_CAST(obj, GEGL_TYPE_EDITOR, GeglEditor))
#define GEGL_EDITOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST (klass, GEGL_TYPE_EDITOR, GeglEditorClass))
#define GEGL_IS_EDITOR(obj)		(G_TYPE_CHECK_INSTANCE_TYPE(obj, GEGL_TYPE_EDITOR))
#define GEGL_IS_EDITOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), GEGL_TYPE_EDITOR))
#define GEGL_EDITOR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), GEGL_TYPE_EDITOR, NodeWidgetClass))

typedef struct _GeglEditor	GeglEditor;
typedef struct _GeglEditorClass	GeglEditorClass;

typedef struct _EditorNode	EditorNode;
typedef struct _NodePad		NodePad;
typedef struct _PadConnection   PadConnection;

struct _NodePad
{
  gchar*	 name;
  NodePad	*connected;	//the pad that this is connected to. NULL if none
  NodePad	*next;		//the next pad in the linked list
  EditorNode*	 node;
};

struct _PadConnection
{
  
};


struct _EditorNode
{
  gint			 id, x, y, width, height;
  const gchar*		 title;
  gint			 title_height;
  EditorNode		*next;
  NodePad*		 inputs;
  NodePad*		 outputs;
  gboolean		 show_image;
  cairo_surface_t	*image;
};

EditorNode*	new_editor_node(EditorNode* prev);


struct _GeglEditor
{
  GtkDrawingArea	parent;

  /* public */
  gint (*connectedPads) (gpointer host, GeglEditor* editor, gint from, gchar* output, gint to, gchar* input);
  gint (*disconnectedPads) (gpointer host, GeglEditor* editor, gint from, gchar* output, gint to, gchar* input);
  gint (*nodeSelected) (gpointer host, GeglEditor* editor, gint node);
  gint (*nodeDeselected) (gpointer host, GeglEditor* editor, gint node);
  gint (*nodeRemoved) (gpointer host, GeglEditor* editor, gint node);

  /* private */
  gint		px, py;		//current mouse coordinates
  gint		dx, dy;		//last mouse coordinates when mouse button pressed
  gint		next_id;
  gboolean	left_mouse_down;	//if left mouse button is pressed
  EditorNode*	first_node;
  EditorNode*	dragged_node;
  EditorNode*	resized_node;
  NodePad*	dragged_pad;
  gpointer	host;		//sent back through callbacks. Can really be whatever
  EditorNode*	selected_node;	//TODO: allow multiple nodes to be selected at once
};

struct _GeglEditorClass
{
  GtkDrawingAreaClass	parent_class;
};

GType		gegl_editor_get_type(void);
GtkWidget*	gegl_editor_new(void);

//public methods
gint	gegl_editor_add_node(GeglEditor* self, const gchar* title, gint ninputs, gchar** inputs, gint noutputs, gchar** outputs);
void	gegl_editor_set_node_position(GeglEditor* self, gint node, gint x, gint y);
void	gegl_editor_show_node_image(GeglEditor* self, gint node);
void	gegl_editor_hide_node_image(GeglEditor* self, gint node);
void	gegl_editor_set_node_image(GeglEditor* self, gint node, cairo_surface_t* image);

#endif
