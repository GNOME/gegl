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

property_double (x, _("X"), 0.0)
    description (_("Horizontal scale factor"))
    ui_range (0.0, 20.0)
    value_range (-9000.0, 9000.0)

property_double (y, _("Y"), 0.0)
    description (_("Vertical scale factor"))
    ui_range (0.0, 20.0)
    value_range (-9000.0, 9000.0)

#else

#include "gegl-operation-filter.h"
#include "scale.h"
#define GEGL_OP_NO_SOURCE
#define GEGL_OP_Parent  OpScale
#define GEGL_OP_PARENT  TYPE_OP_SCALE
#define GEGL_OP_NAME    scale_ratio
#define GEGL_OP_BUNDLE
#define GEGL_OP_C_FILE  "scale-ratio.c"

#include "gegl-op.h"

static void
create_matrix (OpTransform *op,
               GeglMatrix3 *matrix)
{
  GeglProperties *o = GEGL_PROPERTIES (op);

  matrix->coeff [0][0] = o->x;
  matrix->coeff [1][1] = o->y;
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
    "    <node operation='gegl:scale-ratio'>"
    "      <params>"
    "        <param name='origin-x'>100</param>"
    "        <param name='origin-y'>100</param>"
    "        <param name='x'>2.0</param>"
    "        <param name='y'>2.0</param>"
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
    "name", "gegl:scale-ratio",
    "title", _("Scale ratio"),
    "categories", "transform",
    "reference-hash", "ff701f40ea9e983b06712fb817df56b8",
    "reference-composition", composition,
    "description", _("Scales the buffer according to a ratio."),
    NULL);
}

#endif
