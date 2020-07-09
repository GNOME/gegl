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
 * Copyright 2020 Ell
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <stdio.h>

/* #define MANUAL_CONTROL */

#define MAX_LEVELS 16
#define GAMMA      1.5

#ifdef GEGL_PROPERTIES

property_double (radius, _("Radius"), 10.0)
    description (_("Maximal blur radius"))
    value_range (0.0, 1500.0)
    ui_range    (0.0, 100.0)
    ui_gamma    (2.0)
    ui_meta     ("unit", "pixel-distance")

property_boolean (linear_mask, _("Linear mask"), FALSE)
    description (_("Use linear mask values"))

#ifdef MANUAL_CONTROL

property_int (levels, _("Blur levels"), 8)
    description (_("Number of blur levels"))
    value_range (2, MAX_LEVELS)

property_double (gamma, _("Blur gamma"), GAMMA)
    description (_("Gamma factor for blur-level spacing"))
    value_range (0.0, G_MAXDOUBLE)
    ui_range    (0.1, 10.0)

#else

property_boolean (high_quality, _("High quality"), FALSE)
    description (_("Generate more accurate and consistent output (slower)"))

#endif

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     variable_blur
#define GEGL_OP_C_SOURCE variable-blur.c

#include "gegl-op.h"

typedef struct
{
  GeglNode *input;
  GeglNode *aux;
  GeglNode *output;

  GeglNode *gaussian_blur[MAX_LEVELS];

  GeglNode *piecewise_blend;
} Nodes;

static void
update (GeglOperation *operation)
{
  GeglProperties *o     = GEGL_PROPERTIES (operation);
  Nodes          *nodes = o->user_data;

  gdouble gamma;
  gint    levels;
  gint    i;

#ifdef MANUAL_CONTROL
  levels = o->levels;
  gamma  = o->gamma;
#else
  if (o->high_quality)
    {
      levels = MAX_LEVELS;
    }
  else
    {
      levels = ceil (CLAMP (log (o->radius) / G_LN2 + 3,
                            2, MAX_LEVELS));
    }

  gamma = GAMMA;
#endif

  gegl_node_set (nodes->piecewise_blend,
                 "levels", levels,
                 "gamma",  gamma,
                 NULL);

  for (i = 1; i < levels; i++)
    {
      gdouble radius;

      gegl_node_link (nodes->input, nodes->gaussian_blur[i]);

      radius = o->radius * pow ((gdouble) i / (levels - 1), gamma);

      gegl_node_set (nodes->gaussian_blur[i],
                     "std-dev-x", radius,
                     "std-dev-y", radius,
                     NULL);
    }

  for (; i < MAX_LEVELS; i++)
    gegl_node_disconnect (nodes->gaussian_blur[i], "input");
}

static void
attach (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Nodes          *nodes;
  gint            i;

  if (! o->user_data)
    o->user_data = g_slice_new (Nodes);

  nodes = o->user_data;

  nodes->input  = gegl_node_get_input_proxy  (operation->node, "input");
  nodes->aux    = gegl_node_get_input_proxy  (operation->node, "aux");
  nodes->output = gegl_node_get_output_proxy (operation->node, "output");

  nodes->piecewise_blend = gegl_node_new_child (
    operation->node,
    "operation", "gegl:piecewise-blend",
    NULL);

  gegl_operation_meta_redirect (operation,              "linear-mask",
                                nodes->piecewise_blend, "linear-mask");

  gegl_node_connect_to (nodes->input,           "output",
                        nodes->piecewise_blend, "aux1");

  for (i = 1; i < MAX_LEVELS; i++)
    {
      gchar aux_name[32];

      nodes->gaussian_blur[i] = gegl_node_new_child (
        operation->node,
        "operation", "gegl:gaussian-blur",
        NULL);

      sprintf (aux_name, "aux%d", i + 1);

      gegl_node_connect_to (nodes->gaussian_blur[i], "output",
                            nodes->piecewise_blend,  aux_name);
    }

  gegl_node_link_many (nodes->aux,
                       nodes->piecewise_blend,
                       nodes->output,
                       NULL);
}

static void
dispose (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);

  if (o->user_data)
    {
      g_slice_free (Nodes, o->user_data);

      o->user_data = NULL;
    }

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
  operation_meta_class->update = update;

  gegl_operation_class_set_keys (operation_class,
    "name",           "gegl:variable-blur",
    "title",          _("Variable Blur"),
    "categories",     "blur",
    "reference-hash", "553023d2b937e2ebeb216a7999dd12b3",
    "description", _("Blur the image by a varying amount using a mask"),
    NULL);
}

#endif
