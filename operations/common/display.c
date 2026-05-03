/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2011 Jon Nordby <jononor@gmail.com>
 */

#include "config.h"

#include <gegl.h>
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES
property_string  (window_title, _("Window title"), "window_title")
    description(_("Title to be given to output window"))
#else

typedef struct DisplayOpData {
  GeglNode *input; /* The node to show the output of. */
  GeglNode *display; /* The node actually acting as the display op. */
} DisplayOpData;

#define GEGL_OP_SINK
#define GEGL_OP_NAME        display
#define GEGL_OP_C_SOURCE    display.c
#define GEGL_OP_STRUCT_DATA DisplayOpData
#include "gegl-op.h"

/* gegl:display
 * Meta operation for displaying the output of a buffer.
 * Will use one of several well-known display operations 
 * to actually display the output. */

/* Set the correct display handler operation. */
static void
set_display_handler (GeglOperation *operation)
{
  const gchar     *known_handlers[] = {"gegl-gtk3:display", 
                                       "gegl-gtk2:display",
                                       "gegl:sdl2-display",
                                       "gegl:sdl-display"};
  GeglProperties  *props            = GEGL_PROPERTIES (operation);
  DisplayOpData   *data             = (DisplayOpData *) (props->user_data);
  char            *handler          = NULL;
  gchar          **operations       = NULL;
  guint            n_operations;
  gint             i, j;

  /* FIXME: Allow operations to register as a display handler. */
  operations = gegl_list_operations (&n_operations);

  for (i=0; !handler && i < G_N_ELEMENTS(known_handlers); i++)
    {
      for (j=0; j < n_operations; j++)
      {
         if (g_strcmp0(operations[j], known_handlers[i]) == 0)
           {
             handler = operations[j];
             break;
           }
      }
    }

  if (handler)
      gegl_node_set (data->display, "operation", handler,
                     "window-title", props->window_title, NULL);
  else
      g_warning ("No display handler operation found for gegl:display");

  g_free (operations);
}

/* Create an input proxy, and initial display operation, and link together.
 * These will be passed control when process is called later. */
static void
attach (GeglOperation *operation)
{
  GeglProperties *props = GEGL_PROPERTIES (operation);
  DisplayOpData  *data  = (DisplayOpData *) (props->user_data);

  if (props->user_data == NULL)
    props->user_data = g_new0 (DisplayOpData, 1);
  data = props->user_data;

  g_assert (!data->input);
  g_assert (!data->display);

  data->input   = gegl_node_get_input_proxy (operation->node, "input");
  data->display = gegl_node_new_child (operation->node,
                                            "operation", "gegl:nop",
                                            NULL);
  gegl_node_link (data->input, data->display);

  set_display_handler (operation);
}


/* Forward processing to the handler operation. */
static gboolean
process (GeglOperation        *operation,
         GeglOperationContext *context,
         const gchar          *output_pad,
         const GeglRectangle  *roi,
         gint                  level)
{
  GeglProperties *props = GEGL_PROPERTIES (operation);
  DisplayOpData  *data  = (DisplayOpData *) (props->user_data);

  return gegl_operation_process (gegl_node_get_gegl_operation (data->display),
                                 context, output_pad, roi, level);
}

static void
finalize (GObject *object)
{
  GeglProperties *props = GEGL_PROPERTIES (object);
  DisplayOpData  *data  = (DisplayOpData *) (props->user_data);

  g_free (data);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass           *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass     *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationSinkClass *sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  object_class->finalize = finalize;

  operation_class->attach  = attach;
  operation_class->process = process;

  sink_class->needs_full = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"        , "gegl:display",
    "categories"  , "meta:display",
    "title"       , _("Display"),
    "description" ,
    _("Display the input buffer in a window."),
    NULL);
}
#endif
