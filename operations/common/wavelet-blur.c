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
 * Copyright 2017 Thomas Manni <thomas.manni@free.fr>
 *
 * Wavelet blur used in wavelet decompose filter
 *  theory is from original wavelet plugin
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (radius, _("Radius"), 1.0)
    description (_("Radius of the wavelet blur"))
    value_range (0.0, 1500.0)
    ui_range    (0.0, 256.0)
    ui_gamma    (3.0)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("radius", "blur")

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     wavelet_blur
#define GEGL_OP_C_SOURCE wavelet-blur.c

#include "gegl-op.h"

static void
attach (GeglOperation *operation)
{
  GeglNode *gegl   = operation->node;
  GeglNode *input  = gegl_node_get_input_proxy (gegl, "input");
  GeglNode *output = gegl_node_get_output_proxy (gegl, "output");

  GeglNode *vblur  = gegl_node_new_child (gegl,
                                          "operation", "gegl:wavelet-blur-1d",
                                          "orientation", 1,
                                          NULL);

  GeglNode *hblur  = gegl_node_new_child (gegl,
                                          "operation", "gegl:wavelet-blur-1d",
                                          "orientation", 0,
                                          NULL);

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

  gegl_operation_class_set_keys (operation_class,
    "name",           "gegl:wavelet-blur",
    "title",          "Wavelet Blur",
    "categories",     "blur",
    "reference-hash", "841190ad242df6eacc0c39423db15cc1",
    "description", _("This blur is used for the wavelet decomposition filter, "
                     "each pixel is computed from another by the HAT transform"),
    NULL);
}

#endif
