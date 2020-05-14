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
#include <math.h>
#include <stdio.h>

#define MAX_LEVELS 16

#ifdef GEGL_PROPERTIES

property_double (radius, _("Radius"), 10.0)
    description (_("Maximal blur radius"))
    value_range (0.0, 1500.0)
    ui_range    (0.0, 100.0)
    ui_gamma    (2.0)
    ui_meta     ("unit", "pixel-distance")

property_int (levels, _("Levels"), 8)
    description (_("Number of blur levels"))
    value_range (2, MAX_LEVELS)

property_boolean (linear_mask, _("Linear mask"), FALSE)
    description (_("Use linear mask values"))

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

  if (nodes)
    {
      gint i;

      for (i = 1; i < o->levels; i++)
        {
          gegl_node_link (nodes->input, nodes->gaussian_blur[i]);

          gegl_node_set (nodes->gaussian_blur[i],
                         "std-dev-x", i * o->radius / (o->levels - 1),
                         "std-dev-y", i * o->radius / (o->levels - 1),
                         NULL);
        }

      for (; i < MAX_LEVELS; i++)
        gegl_node_disconnect (nodes->gaussian_blur[i], "input");
    }
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

  gegl_operation_meta_redirect (operation,              "levels",
                                nodes->piecewise_blend, "levels");
  gegl_operation_meta_redirect (operation,              "linear-mask",
                                nodes->piecewise_blend, "linear-mask");

  gegl_operation_meta_watch_node (operation, nodes->piecewise_blend);

  gegl_node_connect_to (nodes->input,           "output",
                        nodes->piecewise_blend, "aux1");

  for (i = 1; i < MAX_LEVELS; i++)
    {
      gchar aux_name[32];

      nodes->gaussian_blur[i] = gegl_node_new_child (
        operation->node,
        "operation", "gegl:gaussian-blur",
        NULL);

      gegl_operation_meta_watch_node (operation, nodes->gaussian_blur[i]);

      sprintf (aux_name, "aux%d", i + 1);

      gegl_node_connect_to (nodes->gaussian_blur[i], "output",
                            nodes->piecewise_blend,  aux_name);
    }

  gegl_node_link_many (nodes->aux,
                       nodes->piecewise_blend,
                       nodes->output,
                       NULL);

  update (operation);
}

static void
my_set_property (GObject      *object,
                 guint         property_id,
                 const GValue *value,
                 GParamSpec   *pspec)
{
  set_property (object, property_id, value, pspec);

  update (GEGL_OPERATION (object));
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
  GObjectClass       *object_class;
  GeglOperationClass *operation_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->dispose      = dispose;
  object_class->set_property = my_set_property;

  operation_class->attach    = attach;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:variable-blur",
    "title",       _("Variable Blur"),
    "categories",  "blur",
    "description", _("Blur the image by a varying amount using a mask"),
    NULL);
}

#endif
