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



  EditorNode* node = editor->first_node;
  for(;node != NULL; node = node->next)
    {
      cairo_set_source_rgb(cr, 1, 1, 1);

      /*if(editor->px > node->x && editor->px < node->x+node->width &&
	 editor->py > node->y && editor->py < node->y+node->height) 
	{
	  cairo_set_source_rgb(cr, 1, 0, 0);
	  }*/

      if(node == editor->dragged_node)
	{
	  cairo_rectangle(cr, node->x+editor->px-editor->dx, node->y+editor->py-editor->dy, node->width, node->height);
	}
      else
	{
	  cairo_rectangle(cr, node->x, node->y, node->width, node->height);
	}

      cairo_fill(cr);
    }
  /*  cairo_arc(cr, editor->px, editor->py, 30, 0, 2*3.14159);

  cairo_stroke(cr);*/

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
  editor->px		       = (gint)event->x;
  editor->py		       = (gint)event->y;

  /* redraw */
  gtk_widget_queue_draw(widget);

  return FALSE;
}

static gboolean
gegl_node_widget_button_press(GtkWidget* widget, GdkEventButton* event)
{
  GeglNodeWidget*	editor = GEGL_NODE_WIDGET(widget);
  //TODO: check which mouse button was pressed
  editor->left_mouse_down = TRUE;
  editor->dx = editor->px;
  editor->dy = editor->py;

  EditorNode* node = editor->first_node;
  for(;node != NULL; node = node->next)
    {
      if(editor->px > node->x && editor->px < node->x+node->width &&
	 editor->py > node->y && editor->py < node->y+node->height) 
	{
	  editor->dragged_node = node;
	}
    }

  return FALSE;
}

static gboolean
gegl_node_widget_button_release(GtkWidget* widget, GdkEventButton* event)
{
  GeglNodeWidget*	editor = GEGL_NODE_WIDGET(widget);
  //TODO: check which mouse button was released
  editor->left_mouse_down = FALSE;

  if(editor->dragged_node)
    {
      editor->dragged_node->x += editor->px-editor->dx;
      editor->dragged_node->y += editor->py-editor->dy;
    }

  editor->dragged_node = NULL;

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
  widget_class->button_release_event    = gegl_node_widget_button_release;
}



static void
gegl_node_widget_init(GeglNodeWidget* self)
{
  gtk_widget_add_events (GTK_WIDGET(self),
			 GDK_POINTER_MOTION_MASK |
			 GDK_BUTTON_PRESS_MASK   |
			 GDK_BUTTON_RELEASE_MASK );

  self->first_node = NULL;
  self->dragged_node = NULL;

  self->first_node = new_editor_node(NULL);
  EditorNode* node = new_editor_node(self->first_node);
  node->x = 50;
  node = new_editor_node(node);
  node->x = 100;
  node->y = 14;
}

GtkWidget* 
gegl_node_widget_new ( void )
{
  return g_object_new (GEGL_TYPE_NODE_WIDGET, NULL);
}

EditorNode* new_editor_node(EditorNode* prev) {
  EditorNode* node = malloc(sizeof(EditorNode));
  node->next = NULL;
  node->x = node->y = 0;
  node->width = node->height = 25;
  if(prev != NULL)
    prev->next = node;
  return node;
}
