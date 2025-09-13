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
 * Copyright 2006, 2010 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (std_dev, _("Radius"), 3.0)
    description(_("Expressed as standard deviation, in pixels"))
    value_range (0.0, 1500)
    ui_range    (0.0, 40.0)
    ui_gamma    (3.0)
    ui_meta     ("unit", "pixel-distance")

property_double (scale, _("Amount"), 0.5)
    description(_("Scaling factor for unsharp-mask, the strength of effect"))
    value_range (0.0, 300.0)
    ui_range    (0.0, 10.0)
    ui_gamma    (3.0)

property_double (threshold, _("Threshold"), 0.0)
    value_range (0.0, 1.0)
    ui_range    (0.0, 1.0)
    ui_gamma    (1.0)

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     unsharp_mask
#define GEGL_OP_C_SOURCE unsharp-mask.c

#include "gegl-op.h"

typedef struct
{
  GeglNode *aa;
  GeglNode *absolute;
  GeglNode *subtract;
  GeglNode *multiply_mask;
  GeglNode *multiply;
} State;

static void
update_graph (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  State *state = o->user_data;

  if (o->threshold > 0.0001)
  {
    gegl_node_connect(state->absolute, "input", state->subtract, "output");
    gegl_node_connect(state->multiply, "input", state->multiply_mask, "output");
  }
  else
  {
    gegl_node_connect (state->multiply, "input", state->subtract,  "output");
  }
}

static void
attach (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglNode *gegl, *input, *output, *add, *multiply, *subtract, *blur, *multiply2, *absolute, *threshold, *aa, *multiply_mask;
  State *state = g_malloc0 (sizeof (State));
  o->user_data = state;

  gegl = operation->node;

  input    = gegl_node_get_input_proxy (gegl, "input");
  output   = gegl_node_get_output_proxy (gegl, "output");
  add      = gegl_node_new_child (gegl, "operation", "gegl:add", NULL);
  multiply = gegl_node_new_child (gegl, "operation", "gegl:multiply", NULL);
  multiply_mask = gegl_node_new_child (gegl, "operation", "gegl:multiply", NULL);
  multiply2 = gegl_node_new_child (gegl, "operation", "gegl:multiply", "value", 2.0f, NULL);
  subtract = gegl_node_new_child (gegl, "operation", "gegl:subtract", NULL);
  absolute = gegl_node_new_child (gegl, "operation", "gegl:abs", NULL);
  threshold = gegl_node_new_child (gegl, "operation", "gegl:threshold", NULL);
  aa = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur", "std-dev-x", 1.0, "std-dev-y", 1.0, NULL);
  blur     = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur", NULL);
  state->aa = aa;
  state->absolute = absolute;
  state->subtract = subtract;
  state->multiply = multiply;
  state->multiply_mask = multiply_mask;

  gegl_node_link_many (input, subtract, multiply_mask, multiply, NULL);
  gegl_node_link (input, blur);
  gegl_node_link_many (multiply, add, output, NULL);

  gegl_node_link_many (subtract, absolute, multiply2, threshold, aa, NULL);
  gegl_node_connect (multiply_mask, "aux",  aa,  "output");

  gegl_node_connect (subtract, "aux",   blur,  "output");
  gegl_node_connect (add,      "aux",   input, "output");

  gegl_operation_meta_redirect (operation, "threshold", threshold, "value");
  gegl_operation_meta_redirect (operation, "scale", multiply, "value");
  gegl_operation_meta_redirect (operation, "std-dev", blur, "std-dev-x");
  gegl_operation_meta_redirect (operation, "std-dev", blur, "std-dev-y");
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

  object_class               = G_OBJECT_CLASS (klass);
  operation_class            = GEGL_OPERATION_CLASS (klass);
  operation_meta_class       = GEGL_OPERATION_META_CLASS (klass); 

  object_class->dispose        = dispose;
  operation_class->attach      = attach;
  operation_meta_class->update = update_graph;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:unsharp-mask",
    "title",       _("Sharpen (Unsharp Mask)"),
    "categories",  "enhance:sharpen",
    "reference-hash", "928da4281a4167d6349e7277ce72ff62",
    "description", _("Sharpen image, by adding difference to blurred image, a technique for sharpening originally used in darkrooms."),
    NULL);
}

#endif
