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

property_double (radius1, _("Detail band"), 1.1)
    description(_("Features size for detail band, used for noise removal."))
    value_range (0.0, 100.0)
    ui_range    (0.5, 2.0)
    ui_meta     ("unit", "pixel-distance")

property_double (scale1, _("Detail scale, negative values dimnishes signal in detail band, postivie values increases signal."), -1.6)
    description(_("Scaling factor for image features at radius, -1 cancels them out 1.0 edge enhances"))
    value_range (-3.0, 100.0)
    ui_range    (-3.0, 3.0)

property_double (radius2, _("Feature band"), 4.0)
    description(_("Expressed as standard deviation, in pixels"))
    value_range (0.0, 100.0)
    ui_range    (0.5, 20.0)
    ui_meta     ("unit", "pixel-distance")

property_double (scale2, _("Feature scale"), 0.0)
    description(_("Scaling factor for image features at radius, -1 cancels them out 1.0 edge enhances, when set to 0.0 performance of op is higher."))
    value_range (-3.0, 100.0)
    ui_range    (-3.0, 3.0)

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     edges_and_details
#define GEGL_OP_C_SOURCE edges-and-details.c

#include "gegl-op.h"

typedef struct
{
  GeglNode *input;
  GeglNode *add1;
  GeglNode *dog1;
  GeglNode *mul1;
  GeglNode *add2;
  GeglNode *dog2;
  GeglNode *mul2;
  GeglNode *output;
} State;

static float compute_radius2 (float center_radius, float bw)
{
  return center_radius / (bw + 1);
}
static float compute_radius1 (float center_radius, float bw)
{
  return compute_radius2(center_radius, bw) * bw;
}

static void
update_graph (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  State *state = o->user_data;
  if (!state) return;
  float bw = 0.625;
  GeglNode *iter = state->input;

  if (fabs(o->scale1) > 0.01)
  {
    float radius1 = compute_radius1(o->radius1, bw);
    float radius2 = compute_radius2(o->radius1, bw);
    gegl_node_set (state->dog1, "radius1", radius1,
                                "radius2", radius2,
                                NULL);
    gegl_node_set (state->mul1, "value", o->scale1, NULL);
    gegl_node_connect_from (state->add1, "input",
                            iter, "output");
    gegl_node_connect_from (state->dog1, "input",
                            iter, "output");
    gegl_node_connect_from (state->mul1, "input",
                            state->dog1, "output");
    gegl_node_connect_from (state->add1, "aux",
                            state->mul1, "output");
    iter = state->add1;
  }
  if (fabs (o->scale2) > 0.01)
  {
    float radius1 = compute_radius1(o->radius2, bw);
    float radius2 = compute_radius2(o->radius2, bw);
    gegl_node_set (state->dog2, "radius1", radius1,
                                "radius2", radius2,
                                NULL);
    gegl_node_set (state->mul2, "value", o->scale2, NULL);
    gegl_node_connect_from (state->add2, "input",
                            iter, "output");
    gegl_node_connect_from (state->dog2, "input",
                            iter, "output");
    gegl_node_connect_from (state->mul2, "input",
                            state->dog2, "output");
    gegl_node_connect_from (state->add2, "aux",
                            state->mul2, "output");
    iter = state->add2;
  }
  gegl_node_connect_from (state->output, "input",
                          iter,  "output");
}

static void
attach (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglNode *gegl = operation->node;
  State *state = g_malloc0 (sizeof (State));
  o->user_data = state;

  state->input    = gegl_node_get_input_proxy (gegl, "input");
  state->output   = gegl_node_get_output_proxy (gegl, "output");
  state->add1     = gegl_node_new_child (gegl, "operation", "gegl:add", NULL);
  state->mul1     = gegl_node_new_child (gegl, "operation", "gegl:multiply", "value", 0.0, NULL);
  state->dog1     = gegl_node_new_child (gegl, "operation", "gegl:difference-of-gaussians", NULL);

  state->add2     = gegl_node_new_child (gegl, "operation", "gegl:add", NULL);
  state->mul2     = gegl_node_new_child (gegl, "operation", "gegl:multiply", "value", 0.0, NULL);
  state->dog2     = gegl_node_new_child (gegl, "operation", "gegl:difference-of-gaussians", NULL);

  update_graph (operation);
}

static void
my_set_property (GObject      *object,
                 guint         property_id,
                 const GValue *value,
                 GParamSpec   *pspec)
{
  GeglProperties  *o     = GEGL_PROPERTIES (object);

  set_property (object, property_id, value, pspec);

  if (o)
    update_graph ((void*)object);
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
  GObjectClass       *object_class;
  GeglOperationClass *operation_class;

  object_class    = G_OBJECT_CLASS (klass);
  object_class->dispose      = dispose;
  object_class->set_property = my_set_property;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:edges-and-details",
    "title",       _("Edges and Details"),
    "categories",  "enhance:sharpen:denoise",
    "description", _("Two band parametric equalizer, for noise reduction and edge enhancement."),
    NULL);
}

#endif
