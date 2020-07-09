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
 * Copyright 2020 Øyvind Kolås
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

#else

#include "gegl-operation-filter.h"
#include "transform-core.h"
#define GEGL_OP_NO_SOURCE
#define GEGL_OP_Parent  OpTransform
#define GEGL_OP_PARENT  TYPE_OP_TRANSFORM
#define GEGL_OP_NAME    reset_origin
#define GEGL_OP_BUNDLE
#define GEGL_OP_C_FILE  "reset-origin.c"

#include "gegl-op.h"

#include <stdio.h>

static void
create_matrix (OpTransform *op,
               GeglMatrix3 *matrix)
{
  GeglOperation *operation = GEGL_OPERATION (op);

  gdouble x = 0.0;
  gdouble y = 0.0;

  GeglNode *box_node    = gegl_operation_get_source_node (operation, "input");

  GeglRectangle box_rect = {0,};
 
  if (box_node)
    box_rect = gegl_node_get_bounding_box (box_node);

  x = -box_rect.x;
  y = -box_rect.y;

  matrix->coeff [0][2] = x;
  matrix->coeff [1][2] = y;
}

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
    "    <node operation='gegl:reset-origin'>"
    "      <params>"
    "        <param name='origin-x'>100</param>"
    "        <param name='origin-y'>100</param>"
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
    "name", "gegl:reset-origin",
    "title", _("Reset origin"),
    "categories", "transform",
    "reference-composition", composition,
    "description", _("Translate top-left to 0,0."),
    NULL);
}

#endif
