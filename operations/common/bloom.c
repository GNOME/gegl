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

#define EPSILON 1e-6

#ifdef GEGL_PROPERTIES

property_double (threshold, _("Threshold"), 50.0)
    description (_("Glow-area brightness threshold"))
    ui_range    (0.0, 100.0)

property_double (softness, _("Softness"), 25.0)
    description (_("Glow-area edge softness"))
    value_range (0.0, G_MAXDOUBLE)
    ui_range    (0.0, 100.0)

property_double (radius, _("Radius"), 10.0)
    description (_("Glow radius"))
    value_range (0.0, 1500.0)
    ui_range    (0.0, 100.0)
    ui_gamma    (2.0)
    ui_meta     ("unit", "pixel-distance")

property_double (strength, _("Strength"), 50.0)
    description (_("Glow strength"))
    value_range (0.0, G_MAXDOUBLE)
    ui_range    (0.0, 100.0)

property_boolean (limit_exposure, _("Limit exposure"), FALSE)
    description (_("Don't over-expose highlights"))

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     bloom
#define GEGL_OP_C_SOURCE bloom.c

#include "gegl-op.h"

typedef struct
{
  GeglNode *convert_format;
  GeglNode *cast_format;
  GeglNode *levels;
  GeglNode *rgb_clip;
  GeglNode *multiply;
  GeglNode *gaussian_blur;
  GeglNode *combine;
} Nodes;

static void
update (GeglOperation *operation)
{
  GeglProperties *o     = GEGL_PROPERTIES (operation);
  Nodes          *nodes = o->user_data;

  if (nodes)
    {
      gegl_node_set (nodes->levels,
                     "in-low",   (o->threshold - o->softness) / 100.0,
                     "in-high",  (o->threshold + o->softness) / 100.0,
                     "out-high", o->strength / 100.0,
                     NULL);

      gegl_node_set (nodes->rgb_clip,
                     "high-limit", o->strength / 100.0,
                     NULL);

      gegl_node_set (nodes->combine,
                     "operation", o->limit_exposure ? "gegl:screen" :
                                                      "gegl:add",
                     NULL);
    }
}

static void
attach (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglNode       *input;
  GeglNode       *output;
  Nodes          *nodes;

  input  = gegl_node_get_input_proxy (operation->node, "input");
  output = gegl_node_get_output_proxy (operation->node, "output");

  if (! o->user_data)
    o->user_data = g_slice_new (Nodes);

  nodes = o->user_data;

  nodes->convert_format = gegl_node_new_child (
    operation->node,
    "operation",     "gegl:convert-format",
    "format",        babl_format ("Y float"),
    NULL);

  nodes->cast_format    = gegl_node_new_child (
    operation->node,
    "operation",     "gegl:cast-format",
    "input-format",  babl_format ("Y' float"),
    "output-format", babl_format ("Y float"),
    NULL);

  nodes->levels         = gegl_node_new_child (
    operation->node,
    "operation",     "gegl:levels",
    NULL);

  nodes->rgb_clip       = gegl_node_new_child (
    operation->node,
    "operation",     "gegl:rgb-clip",
    NULL);

  nodes->multiply       = gegl_node_new_child (
    operation->node,
    "operation",     "gegl:multiply",
    NULL);

  nodes->gaussian_blur  = gegl_node_new_child (
    operation->node,
    "operation",     "gegl:gaussian-blur",
    NULL);

  nodes->combine        = gegl_node_new_child (
    operation->node,
    "operation",     "gegl:add",
    NULL);

  gegl_node_link_many (input,
                       nodes->convert_format,
                       nodes->cast_format,
                       nodes->levels,
                       nodes->rgb_clip,
                       NULL);

  gegl_node_connect_to (input,           "output",
                        nodes->multiply, "input");
  gegl_node_connect_to (nodes->rgb_clip, "output",
                        nodes->multiply, "aux");

  gegl_node_link (nodes->multiply, nodes->gaussian_blur);

  gegl_node_connect_to (input,                "output",
                        nodes->combine,       "input");
  gegl_node_connect_to (nodes->gaussian_blur, "output",
                        nodes->combine,       "aux");

  gegl_node_link (nodes->combine, output);

  gegl_operation_meta_redirect (operation,            "radius",
                                nodes->gaussian_blur, "std-dev-x");
  gegl_operation_meta_redirect (operation,            "radius",
                                nodes->gaussian_blur, "std-dev-y");
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
    "name",        "gegl:bloom",
    "title",       _("Bloom"),
    "categories",  "light",
    "reference-hash", "ab23acffc881bde3fa22458bba89e9ed",
    "description", _("Add glow around bright areas"),
    NULL);
}

#endif
