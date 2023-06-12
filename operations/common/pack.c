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

#ifdef GEGL_PROPERTIES

property_double  (gap, _("Gap"), 0.0)
                  description (_("How many pixels of space between items"))
property_double  (align, _("Align"), 0.0)
                  description (_("How to align items, 0.0 is start 0.5 middle and 1.0 end."))
property_enum (orientation, _("Orientation"), GeglOrientation, gegl_orientation, GEGL_ORIENTATION_HORIZONTAL)

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     pack 
#define GEGL_OP_C_SOURCE pack.c

#include "gegl-op.h"

typedef struct
{
  GeglNode *reset_origin_input;
  GeglNode *reset_origin_aux;
  GeglNode *over;
  GeglNode *translate;
  int in_width;
  int in_height;
  int aux_width;
  int aux_height;
  float gap;
  float align;

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

  GeglNode *aux = gegl_node_get_input_proxy (gegl, "aux");
  GeglRectangle aux_rect = gegl_node_get_bounding_box (aux);

  if (o->orientation == GEGL_ORIENTATION_VERTICAL)
  {
    if (state->in_height != in_rect.height ||
        state->in_width  != in_rect.width  ||
        state->aux_width  != aux_rect.width ||
        state->aux_height != aux_rect.height ||
        state->gap != o->gap ||
        state->align != o->align)
    {
      double x = 0.0;
      double y = 1.0f * in_rect.height + o->gap;

      if (in_rect.width < aux_rect.width)
      {
        x = -(aux_rect.width - in_rect.width) * o->align;
      }
      else
      {
        x = (in_rect.width - aux_rect.width) * o->align;
      }

      gegl_node_set (state->translate, "x", round(x), "y", y, NULL);
    }
  }
  else
  {
    if (state->in_width  != in_rect.width ||
        state->in_height != in_rect.height ||
        state->aux_width  != aux_rect.width ||
        state->aux_height != aux_rect.height ||
        state->gap != o->gap ||
        state->align != o->align)
    {
      double x = in_rect.width + o->gap;
      double y = 0.0;

      if (in_rect.height < aux_rect.height)
      {
        y = -(aux_rect.height - in_rect.height) * o->align;
      }
      else
      {
        y = (in_rect.height - aux_rect.height) * o->align;
      }

      gegl_node_set (state->translate, "x", x, "y", round(y), NULL);
    }
  }
  state->in_width = in_rect.width;
  state->in_height= in_rect.height;
  state->aux_width = aux_rect.width;
  state->aux_height= aux_rect.height;
  state->gap = o->gap;
  state->align = o->align;
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

  gegl_node_link_many (input, state->reset_origin_input, state->over, output, NULL);
  gegl_node_link_many (aux, state->reset_origin_aux, state->translate, NULL);

  gegl_node_connect (state->over, "aux", state->translate,  "output");
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
  state->reset_origin_input = gegl_node_new_child (gegl, "operation", "gegl:reset-origin", NULL);
  state->reset_origin_aux = gegl_node_new_child (gegl, "operation", "gegl:reset-origin", NULL);
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
