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
 * Copyright 2026 Akash Bora <contact@akascape.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (vibrance, _("Vibrance"), 0.0)
    description (_("Vibrance (chroma) adjustment"))
    value_range (-100.0, 100.0)
    ui_range    (-5.0, 5.0)
    ui_steps    (0.1, 1.0)
    
property_double (saturation, _("Saturation"), 1.0)
    description (_("Saturation scale factor"))
    value_range (0.0, 10.0)
    ui_range    (0.0, 2.0)
    ui_steps    (0.1, 1.0)

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     vibrance
#define GEGL_OP_C_SOURCE vibrance.c

#include "gegl-op.h"

static void
attach (GeglOperation *operation)
{
  GeglNode *gegl = operation->node;
  GeglNode *input, *output;
  GeglNode *saturation, *hue_chroma;

  input  = gegl_node_get_input_proxy  (gegl, "input");
  output = gegl_node_get_output_proxy (gegl, "output");

  saturation = gegl_node_new_child (gegl,
                                    "operation", "gegl:saturation",
                                    NULL);

  hue_chroma = gegl_node_new_child (gegl,
                                    "operation", "gegl:hue-chroma",
                                    NULL);

  gegl_node_link_many (input, saturation, hue_chroma, output, NULL);

  gegl_operation_meta_redirect (operation, "saturation", saturation, "scale");
  gegl_operation_meta_redirect (operation, "vibrance",   hue_chroma, "chroma");
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:vibrance",
    "title",       _("Vibrance"),
    "categories",  "color",
    "description", _("Adjusts the saturation and vibrance of the image."),
    NULL);
}

#endif