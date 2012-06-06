/* gegl-node-widget.h */

#ifndef __NODEWIDGET_H__
#define __NODEWIDGET_H__

#include	<gtk/gtk.h>
#include <glib-object.h>

#define GEGL_TYPE_NODE_WIDGET			(gegl_node_widget_get_type())
#define GEGL_NODE_WIDGET(obj)			(G_TYPE_CHECK_INSTANCE_CAST(obj, GEGL_TYPE_NODE_WIDGET, GeglNodeWidget))
#define GEGL_NODE_WIDGET_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST (klass, GEGL_TYPE_NODE_WIDGET, GeglNodeWidgetClass))
#define GEGL_IS_NODE_WIDGET(obj)		(G_TYPE_CHECK_INSTANCE_TYPE(obj, GEGL_TYPE_NODE_WIDGET))
#define GEGL_IS_NODE_WIDGET_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), GEGL_TYPE_NODE_WIDGET))
#define GEGL_NODE_WIDGET_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), GEGL_TYPE_NODE_WIDGET, NodeWidgetClass))

typedef struct _GeglNodeWidget		GeglNodeWidget;
typedef struct _GeglNodeWidgetClass	GeglNodeWidgetClass;

struct _GeglNodeWidget
{
  GtkDrawingArea	parent;

  /* private */
  gint	p_x, p_y;
};

struct _GeglNodeWidgetClass
{
  GtkDrawingAreaClass	parent_class;
};

GType		gegl_node_widget_get_type(void);
GtkWidget*	gegl_node_widget_new(void);

#endif
