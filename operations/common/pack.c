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
 * Copyright 2020 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

property_double  (gap, _("Gap"), 0.0)
                  description (_("How many pixels of space between items"))
property_enum (orientation, _("Orientation"), GeglOrientation, gegl_orientation, GEGL_ORIENTATION_HORIZONTAL)

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     pack 
#define GEGL_OP_C_SOURCE pack.c

#include "gegl-op.h"

typedef struct
{
  GeglNode *over;
  GeglNode *translate;
  int width;
  int height;

} State;

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglNode *gegl = operation->node;
  State *state = o->user_data;
  if (!state)
     return;

  GeglNode *input = gegl_node_get_input_proxy (gegl, "input");
  GeglRectangle in_rect = gegl_node_get_bounding_box (input);

  if (o->orientation == GEGL_ORIENTATION_VERTICAL)
  {
    if (state->height != in_rect.height)
    {
      state->height = in_rect.height;
      gegl_node_set (state->translate, "x", 0.0f,
                                       "y", 1.0f * in_rect.height + o->gap, NULL); 
    }
  }
  else
  {
    if (state->width != in_rect.width)
    {
      state->width = in_rect.width;
      gegl_node_set (state->translate, "x", 1.0f * in_rect.width + o->gap,
                                       "y", 0.0f, NULL); 
    }
  }
}

static void
update_graph (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  State *state = o->user_data;

  GeglNode *gegl   = operation->node;
  GeglNode *input  = gegl_node_get_input_proxy  (gegl, "input");
  GeglNode *aux    = gegl_node_get_input_proxy  (gegl, "aux");
  GeglNode *output = gegl_node_get_output_proxy (gegl, "output");

  gegl_node_link_many (input, state->over, output, NULL);
  gegl_node_link_many (aux, state->translate, NULL);

  gegl_node_connect_from (state->over, "aux",
                          state->translate,  "output");
}

static void
attach (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglNode *gegl = operation->node;
  State *state = g_malloc0 (sizeof (State));
  o->user_data = state;

  state->over = gegl_node_new_child (gegl, "operation", "gegl:over", NULL);
  state->translate = gegl_node_new_child (gegl, "operation", "gegl:translate", NULL);
}

static void
dispose (GObject *object)
{
   GeglProperties  *o     = GEGL_PROPERTIES (object);
   g_clear_pointer (&o->user_data, g_free);
   G_OBJECT_CLASS (gegl_op_parent_class)->dispose (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass           *object_class;
  GeglOperationClass     *operation_class;
  GeglOperationMetaClass *operation_meta_class;

  object_class         = G_OBJECT_CLASS (klass);
  operation_class      = GEGL_OPERATION_CLASS (klass);
  operation_meta_class = GEGL_OPERATION_META_CLASS (klass);

  object_class->dispose        = dispose;
  operation_class->attach      = attach;
  operation_class->prepare     = prepare;
  operation_meta_class->update = update_graph;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:pack",
    "title",       _("Pack"),
    "categories",  "layout",
    "description", _("Packs an image horizontally or vertically next to each other with optional gap, aux right of input."),
    NULL);
}

#endif
