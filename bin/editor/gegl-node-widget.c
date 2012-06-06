#include "gegl-node-widget.h"

G_DEFINE_TYPE (GeglNodeWidget, gegl_node_widget, GTK_TYPE_DRAWING_AREA);

enum {
  PROP_0,
  N_PROPERTIES,
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

static void
gegl_node_widget_set_property (GObject	*object,
			 guint		 property_id,
			 const GValue	*value,
			 GParamSpec	*pspec)
{
  GeglNodeWidget	*self = GEGL_NODE_WIDGET(object);

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
    }
}

static void
gegl_node_widget_get_property (GObject	*object,
			 guint		 property_id,
			 GValue		*value,
			 GParamSpec	*pspec)
{
  GeglNodeWidget*	self = GEGL_NODE_WIDGET(object);
  
  switch(property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
      break;
    }
}

static gboolean
gegl_node_widget_draw(GtkWidget *widget, cairo_t *cr)
{
  GeglNodeWidget*	editor = GEGL_NODE_WIDGET(widget);

  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_paint(cr);

  cairo_arc(cr, editor->p_x, editor->p_y, 30, 0, 2*3.14159);
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_stroke(cr);

  return FALSE;
}

#if GTK_MAJOR_VERSION == (2)
static gboolean
gegl_node_widget_expose (GtkWidget *widget, GdkEventExpose *event)
{
  cairo_t	*cr = gdk_cairo_create(widget->window);

  gegl_node_widget_draw(widget, cr);
  cairo_destroy(cr);

  return FALSE;
}
#endif

static gboolean
gegl_node_widget_motion(GtkWidget* widget, GdkEventMotion* event)
{
  GeglNodeWidget*	editor = GEGL_NODE_WIDGET(widget);
  editor->p_x		       = (gint)event->x;
  editor->p_y		       = (gint)event->y;

  /* redraw */
  gtk_widget_queue_draw(widget);

  return FALSE;
}

static gboolean
gegl_node_widget_button_press(GtkWidget* widget, GdkEventButton* event)
{
  g_print("button_press\n");
  return FALSE;
}

static void
gegl_node_widget_class_init(GeglNodeWidgetClass *klass)
{
  GtkWidgetClass	*widget_class = GTK_WIDGET_CLASS(klass);
#if GTK_MAJOR_VERSION == (3)
  widget_class->draw		      = gegl_node_widget_draw;
#else
  widget_class->expose_event	      = gegl_node_widget_expose;
#endif
  widget_class->motion_notify_event   = gegl_node_widget_motion;
  widget_class->button_press_event    = gegl_node_widget_button_press;
}



static void
gegl_node_widget_init(GeglNodeWidget* self)
{
  gtk_widget_add_events (GTK_WIDGET(self),
			 GDK_POINTER_MOTION_MASK |
			 GDK_BUTTON_PRESS_MASK   |
			 GDK_BUTTON_RELEASE_MASK );
}

GtkWidget* 
gegl_node_widget_new ( void )
{
  return g_object_new (GEGL_TYPE_NODE_WIDGET, NULL);
}


