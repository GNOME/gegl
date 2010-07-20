
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
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006, 2010 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

/* Brush shape parameters */
gegl_chant_double (scale,    _("Scale Factor"),  0.0, 10.0, 1.0,
                             _("Brush Scale Factor"))
gegl_chant_double (aspect,   _("Aspect Ratio"),   0.1, 10.0, 1.0,
                             _("Brush Aspect, -10 for pancake 10.0 for spike."))
gegl_chant_double (angle,    _("Angle"),   0.0, 360.0, 0.0,
                             _("Brush Angle."))
gegl_chant_double (hardness, _("Hardness"),   0.0, 1.0, 0.6,
                             _("Brush Hardness, 0.0 for soft 1.0 for hard."))
gegl_chant_double (force,    _("Force"),   0.0, 1.0, 0.6,
                             _("Brush Force."))

gegl_chant_color  (color,    _("Color"),      "rgb(0.0,0.0,0.0)",
                             _("Color of paint to use for stroking."))
gegl_chant_double (opacity,  _("Opacity"),  -2.0, 2.0, 1.0,
                             _("Opacity."))
/* dab position parameters */
gegl_chant_double (x, _("X"), -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                   _("Horizontal offset."))
gegl_chant_double (y, _("Y"), -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                   _("Vertical offset."))

gegl_chant_double (radius, _("Radius"), -G_MAXDOUBLE, G_MAXDOUBLE, 10.0,
                   _("Blur radius."))

#else

#define GEGL_CHANT_C_FILE       "brush-dab.c"

#include "gegl-plugin.h"

struct _GeglChant
{
  GeglOperationMeta parent_instance;
  gpointer          properties;

  GeglNode *input;
  GeglNode *output;

  GeglNode *scale;
  GeglNode *rotate;
  GeglNode *translate;

  GeglNode *hardness;
  GeglNode *force;
  
  GeglNode *opacity;
  GeglNode *color;
};

typedef struct
{
  GeglOperationMetaClass parent_class;
} GeglChantClass;

#include "gegl-chant.h"
GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_TYPE_OPERATION_META)

/* in attach we hook into graph adding the needed nodes */
static void attach (GeglOperation *operation)
{
  GeglChant *self = GEGL_CHANT (operation);
  GeglNode *gegl  = operation->node;

  self->input     = gegl_node_get_input_proxy (gegl, "input");
  self->output    = gegl_node_get_output_proxy (gegl, "output");

  self->scale     = gegl_node_new_child (gegl, "operation", "gegl:scale", NULL);
  self->rotate    = gegl_node_new_child (gegl, "operation", "gegl:rotate", NULL);
  self->translate = gegl_node_new_child (gegl, "operation", "gegl:translate", NULL);

  self->opacity   = gegl_node_new_child (gegl, "operation", "gegl:opacity", NULL);

  self->color     = gegl_node_new_child (gegl, "operation", "gegl:color", NULL);

  gegl_node_link_many (self->input,
                       self->translate, NULL);
  gegl_node_link_many (self->color, self->opacity, self->output, NULL);

  gegl_node_connect_from (self->opacity, "aux", self->translate, "output");
  gegl_operation_meta_redirect (operation, "angle", self->rotate, "degrees");
#if 0
  gegl_operation_meta_redirect (operation, "hardness", self->hardness, "");
  gegl_operation_meta_redirect (operation, "force", self->force, "");
#endif
  gegl_operation_meta_redirect (operation, "x", self->translate, "x");
  gegl_operation_meta_redirect (operation, "y", self->translate, "y");
  gegl_operation_meta_redirect (operation, "opacity", self->opacity, "value");
  gegl_operation_meta_redirect (operation, "color", self->color, "value");
}

static void prepare (GeglOperation *operation)
{
  GeglChant *self = GEGL_CHANT (operation);
  GeglChantO  *o = GEGL_CHANT_PROPERTIES (operation);
  gdouble ar     = o->aspect;
  gdouble scale     = o->scale;

  gdouble scalex = (ar>0.0)?scale*ar:scale;
  gdouble scaley = (ar<0.0)?scale*ar:scale;

  gegl_node_set (self->scale, "x", scalex, NULL);
  gegl_node_set (self->scale, "y", scaley, NULL);
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GObjectClass       *object_class;
  GeglOperationClass *operation_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;
  operation_class->prepare = prepare;

  operation_class->name        = "gegl:brush-dab";
  operation_class->categories  = "meta:render";
  operation_class->description =
        _("Transform a brush dab.");
}

#endif
