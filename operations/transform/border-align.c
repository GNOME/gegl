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

property_double (x, _("X"), 0.5)
    description (_("Horizontal justification 0.0 is left 0.5 centered and 1.0 right."))
    value_range (-2.0, 3.0)
    ui_range (0.0, 1.0)
    ui_meta ("axis", "x")

property_double (y, _("Y"), 0.5)
    description (_("Vertical justification 0.0 is top 0.5 middle and 1.0 bottom."))
    value_range (-2.0, 3.0)
    ui_range (0.0, 1.0)
    ui_meta ("axis", "y")

property_double (horizontal_margin, "Horizontal Margin", 0.0)
property_double (vertical_margin, "Vertical Margin", 0.0)

property_boolean (snap_integer, "snap to integer position", TRUE)


#else

#include "gegl-operation-filter.h"
#include "transform-core.h"
#define GEGL_OP_NO_SOURCE
#define GEGL_OP_Parent  OpTransform
#define GEGL_OP_PARENT  TYPE_OP_TRANSFORM
#define GEGL_OP_NAME    border_align 
#define GEGL_OP_BUNDLE
#define GEGL_OP_C_FILE  "border-align.c"

#include "gegl-op.h"

#include <stdio.h>

static GeglNode *gegl_node_get_consumer_no (GeglNode *node,
                                            const char *output_pad,
                                            const char **consumer_pad,
                                            int no)
{
  GeglNode *consumer = NULL;
  GeglNode **nodes = NULL;
  const gchar **consumer_names = NULL;
  int count;
  if (node == NULL)
    return NULL;

  count = gegl_node_get_consumers (node, "output", &nodes, &consumer_names);
  if (count > no){
     /* XXX : look into inverting list in get_consumers */
     //consumer = nodes[count-no-1];
     consumer = nodes[no];
     if (consumer_pad)
       *consumer_pad = g_intern_string (consumer_names[count-no-1]);
  }
  g_free (nodes);
  g_free (consumer_names);
  return consumer;
}

static GeglNode *gegl_node_find_composite_target (GeglNode *node)
{
  const char *dest_pad = NULL;
  GeglNode *iter = node;
  iter = gegl_node_get_consumer_no (iter, "output", &dest_pad, 0);
  while (iter && dest_pad && g_str_equal (dest_pad, "input"))
  {
    iter = gegl_node_get_consumer_no (iter, "output", &dest_pad, 0);
  }
  if (dest_pad && !strcmp (dest_pad, "aux"))
  {
    return gegl_node_get_producer (iter, "input", NULL);
  }
  return NULL;
}

static void
create_matrix (OpTransform *op,
               GeglMatrix3 *matrix)
{
  GeglOperation *operation = GEGL_OPERATION (op);
  GeglProperties *o = GEGL_PROPERTIES (op);

  gdouble x = 0.0;
  gdouble y = 0.0;

  GeglNode *border_node = gegl_operation_get_source_node (operation, "aux");
  GeglNode *box_node    = gegl_operation_get_source_node (operation, "input");

  GeglRectangle box_rect = {0,};
  GeglRectangle border_rect = {0,};
 
  if (box_node)
    box_rect = gegl_node_get_bounding_box (box_node);

  if (border_node)
  {
    border_rect = gegl_node_get_bounding_box (border_node);
  }
  else
  {
     border_node = gegl_node_find_composite_target (operation->node);
     if (border_node)
       border_rect = gegl_node_get_bounding_box (border_node);
  }

  x = o->x * (border_rect.width - box_rect.width - o->horizontal_margin * 2) +
             o->horizontal_margin;
  y = o->y * (border_rect.height - box_rect.height - o->vertical_margin * 2)+
             o->vertical_margin;

  x -= box_rect.x;
  y -= box_rect.y;

  if (o->snap_integer)
  {
    x = roundf (x);
    y = roundf (y);
  }

  matrix->coeff [0][2] = x;
  matrix->coeff [1][2] = y;
}

static void attach (GeglOperation *operation)
{
  GeglOperationComposerClass *klass = GEGL_OPERATION_COMPOSER_GET_CLASS (operation); 
  GParamSpec    *pspec;
  GeglOperationClass *parent_class = g_type_class_peek_parent (klass);

  if (parent_class->attach)
    parent_class->attach (operation);

  pspec = g_param_spec_object ("aux",
      klass->aux_label?klass->aux_label:"Aux",
      klass->aux_description?klass->aux_description:_("Auxiliary image buffer input pad."),
      GEGL_TYPE_BUFFER,
      G_PARAM_READWRITE |
      GEGL_PARAM_PAD_INPUT);
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);

}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;
  OpTransformClass   *transform_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  transform_class = OP_TRANSFORM_CLASS (klass);

  operation_class->attach = attach;
  transform_class->create_matrix = create_matrix;

  gegl_operation_class_set_keys (operation_class,
    "name", "gegl:border-align",
    "title", _("Border Align"),
    "categories", "transform",
    "reference-hash", "109c3f3685488a9952ca07ef18387850",
    "description", _("Aligns box of input rectangle with border of compositing target or aux' bounding-box border, if aux pad is not connected the op tries to figure out which bounding box' border applies."),
    NULL);
}

#endif
