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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 * 2022 Sam Lester (GEGL inner glow) *2023 InnerShadow for GEGL Styles
 */

 /* This filter is a stand alone meant to be fused with Gimp's blending options. But it also is meant to be called with GEGL Styles
    From a technical perspective this is literally inverted transparency a drop shadow then removal of the color fill that drop shadow applied too*/


#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_innerglow_grow_shape)
  enum_value (GEGL_INNERGLOW_GROW_SHAPE_SQUARE,  "squareig",  N_("Square"))
  enum_value (GEGL_INNERGLOW_GROW_SHAPE_CIRCLE,  "circleig",  N_("Circle"))
  enum_value (GEGL_INNERGLOW_GROW_SHAPE_DIAMOND, "diamondig", N_("Diamond"))
enum_end (innerglowshape)


property_enum   (grow_shape, _("Grow shape"),
                 innerglowshape, gegl_innerglow_grow_shape,
                 GEGL_INNERGLOW_GROW_SHAPE_CIRCLE)
  description   (_("The shape to expand or contract the shadow in"))


property_double (x, _("X"), 0.0)
  description   (_("Horizontal shadow offset"))
  ui_range      (-30.0, 30.0)
  ui_steps      (1, 2)
  ui_meta       ("unit", "pixel-distance")
  ui_meta       ("axis", "x")

property_double (y, _("Y"), 0.0)
  description   (_("Vertical shadow offset"))
  ui_range      (-30.0, 30.0)
  ui_steps      (1, 2)
  ui_meta       ("unit", "pixel-distance")
  ui_meta       ("axis", "y")



property_double (radius, _("Blur radius"), 7.5)
  value_range   (0.0, 40.0)
  ui_range      (0.0, 30.0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")


property_double (grow_radius, _("Grow radius"), 4.0)
  value_range   (1, 30.0)
  ui_range      (1, 30.0)
  ui_digits     (0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  description (_("The distance to expand the shadow before blurring; a negative value will contract the shadow instead"))

property_double (opacity, _("Opacity"), 1.2)
  value_range   (0.0, 2.0)
  ui_steps      (0.01, 0.10)


property_color (value, _("Color"), "#fbff00")
    description (_("The color to paint over the input"))
    ui_meta     ("role", "color-primary")


property_double  (cover, _("Median fix for non-affected pixels on edges"), 60)
  value_range (50, 100)
  description (_("Median Blur covers unaffected pixels. Making this slider too high will make it outline-like. So only slide it as high as you need to cover thin shape corners."))
 /* This option has to be visible as certain inner shadows will look bad if it is locked at 100. And if it is disabled all the pixels on the text's edges will be unmodified.  */

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     gegl_inner_glow
#define GEGL_OP_C_SOURCE inner-glow.c

#include "gegl-op.h"

static void attach (GeglOperation *operation)
{
  GeglNode *gegl = operation->node;
  GeglNode *input, *color, *translate, *out, *median, *medianfix, *crop, *opacity, *gaussian,  *output;

  input    = gegl_node_get_input_proxy (gegl, "input");
  output   = gegl_node_get_output_proxy (gegl, "output");

 gaussian    = gegl_node_new_child (gegl,
                                  "operation", "gegl:gaussian-blur", "abyss-policy", 0, "clip-extent", FALSE,  
                                  NULL);

 opacity    = gegl_node_new_child (gegl,
                                  "operation", "gegl:opacity", 
                                  NULL);

 translate    = gegl_node_new_child (gegl,
                                  "operation", "gegl:translate",
                                  NULL);

 median     = gegl_node_new_child (gegl, "operation", "gegl:median-blur",
                                         "radius",       1,
                                         "alpha-percentile", 0.0,
                                         NULL);

 crop    = gegl_node_new_child (gegl,
                                  "operation", "gegl:crop",
                                  NULL);

 color    = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-overlay",
                                  NULL);


 medianfix     = gegl_node_new_child (gegl, "operation", "gegl:median-blur",
                                         "radius",       1,
                                         "abyss-policy",     GEGL_ABYSS_NONE,
                                         NULL);

 out    = gegl_node_new_child (gegl,
                                  "operation", "gegl:src-out",
                                  NULL);
 /* This is meant so inner glow to reach pixels in tight corners */

gegl_operation_meta_redirect (operation, "grow_radius", median, "radius");
gegl_operation_meta_redirect (operation, "radius", gaussian, "std-dev-x");
gegl_operation_meta_redirect (operation, "radius", gaussian, "std-dev-y");
gegl_operation_meta_redirect (operation, "opacity", opacity, "value");
gegl_operation_meta_redirect (operation, "grow_shape", median, "neighborhood");
gegl_operation_meta_redirect (operation, "value", color, "value");
gegl_operation_meta_redirect (operation, "x", translate, "x");
gegl_operation_meta_redirect (operation, "y", translate, "y");
gegl_operation_meta_redirect (operation, "cover", medianfix, "alpha-percentile");


 gegl_node_link_many (input, median, gaussian, translate, out, color, opacity, medianfix, crop, output, NULL);
 gegl_node_connect (out, "aux", input, "output");
 gegl_node_connect (crop, "aux", input, "output");


}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:inner-glow",
    "title",       _("Inner Glow"),
    "reference-hash", "8a1319fb8f04ae1bc086721abf25419a",
    "description", _("GEGL does an inner shadow glow effect; for more interesting use different blend mode than the default, Replace."),
    "gimp:menu-path", "<Image>/Filters/Light and Shadow/",
    "gimp:menu-label", _("Inner Glow..."),
    NULL);
}

#endif
