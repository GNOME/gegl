/* This file is an image processing operation for GEGL
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
 * Copyright 2019 Thomas Manni <thomas.manni@free.fr>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int (radius, _("Radius"), 4)
   description(_("Radius of square pixel region, (width and height will be radius*2+1)"))
   value_range (0, 1000)
   ui_range    (0, 100)
   ui_gamma   (1.5)

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     boxblur
#define GEGL_OP_C_SOURCE boxblur.c

#include "gegl-op.h"

static void
attach (GeglOperation *operation)
{
  GeglNode *gegl   = operation->node;
  GeglNode *output = gegl_node_get_output_proxy (gegl, "output");

  GeglNode *vblur  = gegl_node_new_child (gegl,
                                          "operation", "gegl:boxblur-1d",
                                          "orientation", 1,
                                          NULL);

  GeglNode *hblur  = gegl_node_new_child (gegl,
                                          "operation", "gegl:boxblur-1d",
                                          "orientation", 0,
                                          NULL);

  GeglNode *input  = gegl_node_get_input_proxy (gegl, "input");

  gegl_node_link_many (input, hblur, vblur, output, NULL);

  gegl_operation_meta_redirect (operation, "radius", hblur, "radius");
  gegl_operation_meta_redirect (operation, "radius", vblur, "radius");
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;
  operation_class->threaded = FALSE;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:boxblur",
    "categories",  "blur",
    "title",       _("BoxBlur"),
    "description", _("Blur resulting from averaging the colors of a square "
                     "neighborhood."),
    NULL);
}

#endif
