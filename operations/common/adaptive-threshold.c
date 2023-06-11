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
 * Copyright 2023 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (radius, _("Radius"), 200.0)
  value_range   (0.0, G_MAXDOUBLE)
  ui_range      (0.0, 1000.0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")

property_double (level, _("Level"), 0.5)
value_range   (0.0, 1.0) 

property_int (aa_factor, _("Antialias"), 1)
value_range   (1, 256) // this allows full 8bit resolution as a api-max
ui_range      (1, 16)

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     adaptive_threshold
#define GEGL_OP_C_SOURCE adaptive-threshold.c

#include "gegl-op.h"

typedef struct
{
  GeglNode *input;
  GeglNode *aa_grow;
  GeglNode *aa_grow2;
  GeglNode *blur;
  GeglNode *threshold;
  GeglNode *aa_shrink;
  GeglNode *output;
} State;

static void
update_graph (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  State *state = o->user_data;
  if (!state) return;

  if (o->aa_factor > 1.0001)
  {
    float factor = sqrt (o->aa_factor);
    float inv_factor = 1.0/factor;
    gegl_node_set (state->aa_grow, "x", factor, "y", factor, NULL);
    gegl_node_set (state->aa_grow2, "x", factor, "y", factor, NULL);
    gegl_node_set (state->aa_shrink, "x", inv_factor, "y", inv_factor, NULL);

    gegl_node_link_many (state->input, state->aa_grow, state->threshold, state->aa_shrink, state->output, NULL);
    gegl_node_connect_from (state->threshold, "aux", state->aa_grow2, "output");
  }
  else
  {
    gegl_node_link_many (state->input, state->threshold, state->output, NULL);
    gegl_node_connect_from (state->threshold, "aux", state->blur, "output");
  }
}


/* in attach we hook into graph adding the needed nodes */
static void
attach (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglNode  *gegl = operation->node;

  State *state = g_malloc0 (sizeof (State));
  o->user_data = state;

  state->aa_grow   = gegl_node_new_child (gegl, "operation", "gegl:scale-ratio", NULL);
  state->aa_grow2  = gegl_node_new_child (gegl, "operation", "gegl:scale-ratio", NULL);
  state->aa_shrink = gegl_node_new_child (gegl, "operation", "gegl:scale-ratio", NULL);
  state->input  = gegl_node_get_input_proxy (gegl, "input");
  state->output = gegl_node_get_output_proxy (gegl, "output");
  state->blur   = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur",
                                       "clip-extent", FALSE,
                                       "abyss-policy", 0,
                                       NULL);
  state->threshold = gegl_node_new_child (gegl, "operation", "gegl:threshold", NULL);

  gegl_node_link_many (state->input, state->aa_grow, state->threshold, state->aa_shrink, state->output,
                       NULL);
  gegl_node_connect_from (state->blur, "input", state->input, "output");
  gegl_node_connect_from (state->aa_grow2, "input", state->blur, "output");

  gegl_operation_meta_redirect (operation, "radius", state->blur, "std-dev-x");
  gegl_operation_meta_redirect (operation, "radius", state->blur, "std-dev-y");

  update_graph (operation);
}

static void
dispose (GObject *object)
{
   GeglProperties  *o = GEGL_PROPERTIES (object);
   g_clear_pointer (&o->user_data, g_free);
   G_OBJECT_CLASS (gegl_op_parent_class)->dispose (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass           *object_class;
  GeglOperationClass     *operation_class      = GEGL_OPERATION_CLASS (klass);
  GeglOperationMetaClass *operation_meta_class = GEGL_OPERATION_META_CLASS (klass);

  operation_class->attach      = attach;
  operation_meta_class->update = update_graph;

  object_class               = G_OBJECT_CLASS (klass); 
  object_class->dispose      = dispose; 

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:adaptive-threshold",
    "title",       _("Adaptive Threshold"),
    "description",
    _("Applies a threshold against the average of a spatial neighbourhood."),
    "gimp:menu-path", "<Image>/Colors",
    "gimp:menu-label", _("Adaptive Threshold..."),
    NULL);
}

#endif
