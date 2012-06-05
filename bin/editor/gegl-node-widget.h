/* gegl-node-widget.h */

#ifndef __NODEWIDGET_H__
#define __NODEWIDGET_H__

#include	<gtk/gtk.h>

#define NODEWIDGET(obj)		GTK_CHECK_CAST (obj, nodewidget_get_type(), NodeWidget)
#define NODEWIDGET_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, nodewidget_get_type(), NodeWidgetClass)
#define IS_NODEWIDGET(obj)	GTK_CHECK_TYPE(obj, nodewidget_get_type())

typedef struct _NodeWidget NodeWidget;
typedef struct _NodeWidgetClass NodeWidgetClass;

struct _NodeWidget
{
  GtkWidget widget;

  
};

struct _NodeWidgetClass
{
  GtkWidgetClass parent_class;
};

GtkType nodewidget_get_type(void);
NodeWidget* nodewdiget_new();

#endif
