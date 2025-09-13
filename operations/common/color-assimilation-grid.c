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
 * Original idea and concept by Øyvind Kolås
 * also Copyright 2019 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (grid_size, _("Grid size"), 23)
    value_range (0.0, 150)
    ui_range    (0.0, 40.0)
    ui_gamma    (3.0)
    ui_meta     ("unit", "pixel-distance")

property_double (saturation, _("Saturation"), 2.5)
    value_range (0.0, 30.0)
    ui_range    (0.0, 10.0)

property_double (angle, _("Angle"), 45.0)
    value_range (-180.0, 180.0)
    ui_range    (-180.0, 180.0)
    ui_gamma    (1.0)

property_double (line_thickness, _("Line thickness"), 0.4)
    value_range (0.0, 1.0)
    ui_range    (0.0, 1.0)
    ui_gamma    (1.0)

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     color_assimilation_grid
#define GEGL_OP_C_SOURCE color-assimilation-grid.c

#include "gegl-op.h"

typedef struct
{
  GeglNode *desaturate;
  GeglNode *saturate;
  GeglNode *over;
  GeglNode *opacity;
  GeglNode *mask;
  GeglNode *color;
  double    old_line_thickness;
} State;

static void
update_graph (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  State *s = o->user_data;
  if (s && s->old_line_thickness != o->line_thickness)
  {
    GeglColor *color = gegl_color_new (NULL);
    gegl_color_set_rgba (color, o->line_thickness,
                                o->line_thickness,
                                o->line_thickness, 1.0);
    gegl_node_set (s->color, "value", color, NULL);
    g_object_unref (color);
    s->old_line_thickness = o->line_thickness;
  }
}

static void
attach (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  State          *s = g_malloc0 (sizeof (State));
  GeglNode       *gegl, *input, *output;

  o->user_data = s;
  gegl = operation->node;

  s->desaturate   = gegl_node_new_child (gegl, "operation", "gegl:saturation",
                                               "scale", 0.0, NULL);
  s->saturate     = gegl_node_new_child (gegl, "operation", "gegl:saturation",
                                               "scale", 0.0, NULL);
  s->over         = gegl_node_new_child (gegl, "operation", "gegl:over", NULL);
  s->opacity      = gegl_node_new_child (gegl, "operation", "gegl:opacity",
                                               "value", 1.0, NULL);
  s->mask = gegl_node_new_child (gegl, "operation", "gegl:newsprint",
                                               "pattern", 4,
                                                "angle",  o->angle,
                                                "period", o->grid_size,
                                                "color-model", 0, NULL);
  s->color        = gegl_node_new_child (gegl, "operation", "gegl:color",
                                               NULL);

  input           = gegl_node_get_input_proxy  (gegl, "input");
  output          = gegl_node_get_output_proxy (gegl, "output");

  gegl_node_link_many (input, s->desaturate, s->over, output, NULL);
  gegl_node_link_many (input, s->saturate, s->opacity, NULL);
  gegl_node_link_many (s->color, s->mask, NULL);

  gegl_node_connect (s->opacity, "aux", s->mask,  "output");
  gegl_node_connect (s->over,    "aux", s->opacity,  "output");

  gegl_operation_meta_redirect (operation, "grid-size", s->mask, "period");
  gegl_operation_meta_redirect (operation, "angle", s->mask, "angle");
  gegl_operation_meta_redirect (operation, "saturation", s->saturate, "scale");
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

  object_class          = G_OBJECT_CLASS (klass);
  operation_class       = GEGL_OPERATION_CLASS (klass);
  operation_meta_class  = GEGL_OPERATION_META_CLASS (klass);
  object_class->dispose        = dispose;
  operation_class->attach      = attach;
  operation_meta_class->update = update_graph;

  gegl_operation_class_set_keys (operation_class,
    "name",           "gegl:color-assimilation-grid",
    "title",          _("Color Assimilation Grid"),
    "categories",     "illusions",
    "reference-hash", "19c0eab029aefaf6a3d0ac01d4932117",
    "description", _("Turn image grayscale and overlay an oversaturated grid - through color assimilation happening in the human visual system, for some grid scales this produces the illusion that the grayscale grid cells themselves also have color."),
    NULL);
}

#endif
