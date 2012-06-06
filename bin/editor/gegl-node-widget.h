/* gegl-node-widget.h */

#ifndef __NODEWIDGET_H__
#define __NODEWIDGET_H__

#include	<gtk/gtk.h>
#include <glib-object.h>
#include <stdlib.h>

#define GEGL_TYPE_NODE_WIDGET			(gegl_node_widget_get_type())
#define GEGL_NODE_WIDGET(obj)			(G_TYPE_CHECK_INSTANCE_CAST(obj, GEGL_TYPE_NODE_WIDGET, GeglNodeWidget))
#define GEGL_NODE_WIDGET_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST (klass, GEGL_TYPE_NODE_WIDGET, GeglNodeWidgetClass))
#define GEGL_IS_NODE_WIDGET(obj)		(G_TYPE_CHECK_INSTANCE_TYPE(obj, GEGL_TYPE_NODE_WIDGET))
#define GEGL_IS_NODE_WIDGET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), GEGL_TYPE_NODE_WIDGET))
#define GEGL_NODE_WIDGET_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), GEGL_TYPE_NODE_WIDGET, NodeWidgetClass))

typedef struct _GeglNodeWidget		GeglNodeWidget;
typedef struct _GeglNodeWidgetClass	GeglNodeWidgetClass;

typedef struct _EditorNode	EditorNode;
typedef struct _NodePad		NodePad;

struct _NodePad
{
  gchar*	 name;
  NodePad	*connected;	//the pad that this is connected to. NULL if none
  NodePad	*next;		//the next pad in the linked list
  EditorNode*	 node;
};

struct _EditorNode
{
  gint		 x, y, width, height;
  gchar*	 title;
  gint		 title_height;
  EditorNode	*next;
  NodePad*	 inputs;
  NodePad*	 outputs;
};

EditorNode*	new_editor_node(EditorNode* prev);


struct _GeglNodeWidget
{
  GtkDrawingArea	parent;

  /* private */
  gint		px, py;		//current mouse coordinates
  gint		dx, dy;		//last mouse coordinates when mouse button pressed
  gboolean	left_mouse_down;	//if left mouse button is pressed
  EditorNode*	first_node;
  EditorNode*	dragged_node;
  EditorNode*	resized_node;
  NodePad*	dragged_pad;
};

struct _GeglNodeWidgetClass
{
  GtkDrawingAreaClass	parent_class;
};

GType		gegl_node_widget_get_type(void);
GtkWidget*	gegl_node_widget_new(void);

#endif
