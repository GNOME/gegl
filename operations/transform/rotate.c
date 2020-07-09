/* This file is part of GEGL
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
 * Copyright 2006 Philip Lafleur
 *           2017 Øyvind Kolås
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (degrees, _("Degrees"), 0.0)
    description(_("Angle to rotate (counter-clockwise)"))
    value_range (-720.0, 720.0)
    ui_range (-180.0, 180.0)
    ui_meta ("unit", "degree")
    ui_meta ("direction", "ccw")

#else

#include "gegl-operation-filter.h"
#include "transform-core.h"
#define GEGL_OP_NO_SOURCE
#define GEGL_OP_Parent  OpTransform
#define GEGL_OP_PARENT  TYPE_OP_TRANSFORM
#define GEGL_OP_NAME    rotate
#define GEGL_OP_BUNDLE
#define GEGL_OP_C_FILE  "rotate.c"

#include "gegl-op.h"

static void
create_matrix (OpTransform *op,
               GeglMatrix3 *matrix)
{
  GeglProperties *o = GEGL_PROPERTIES (op);
  double radians = o->degrees * (2 * G_PI / 360.0);

  matrix->coeff [0][0] = matrix->coeff [1][1] = cos (radians);
  matrix->coeff [0][1] = sin (radians);
  matrix->coeff [1][0] = - matrix->coeff [0][1];
}

#include <stdio.h>

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;
  OpTransformClass   *transform_class;
  gchar              *composition =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "  <node operation='gegl:crop' width='200' height='200'/>"
    "  <node operation='gegl:over'>"
    "    <node operation='gegl:rotate'>"
    "      <params>"
    "        <param name='degrees'>30</param>"
    "        <param name='origin-x'>50</param>"
    "        <param name='origin-y'>50</param>"
    "      </params>"
    "    </node>"
    "    <node operation='gegl:load' path='standard-input.png'/>"
    "  </node>"
    "  <node operation='gegl:checkerboard'>"
    "    <params>"
    "      <param name='color1'>rgb(0.25,0.25,0.25)</param>"
    "      <param name='color2'>rgb(0.75,0.75,0.75)</param>"
    "    </params>"
    "  </node>"    
    "</gegl>";

  operation_class = GEGL_OPERATION_CLASS (klass);
  transform_class = OP_TRANSFORM_CLASS (klass);
  transform_class->create_matrix = create_matrix;

  gegl_operation_class_set_keys (operation_class,
    "name", "gegl:rotate",
    "title", _("Rotate"),
    "categories", "transform",
    "reference-hash", "577f8ff8bcdd46dc4835097a6afc914b",
    "reference-composition", composition,
    "description", _("Rotate the buffer around the specified origin."),
    NULL);
}

#endif
