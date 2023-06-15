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
 * Copyright 2022, 2023 LinuxBeaver
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES


enum_start (gegl_blend_mode_typedesignerlite)
  enum_value (GEGL_BLEND_MODE_TYPE_HARDLIGHT, "hardlight",
              N_("HardLight"))
  enum_value (GEGL_BLEND_MODE_TYPE_MULTIPLY,  "multiply",
              N_("Multiply"))
  enum_value (GEGL_BLEND_MODE_TYPE_COLORDODGE, "colordodge",
              N_("ColorDodge"))
  enum_value (GEGL_BLEND_MODE_TYPE_PLUS,      "plus",
              N_("Plus"))
  enum_value (GEGL_BLEND_MODE_TYPE_DARKEN,    "darken",
              N_("Darken"))
  enum_value (GEGL_BLEND_MODE_TYPE_LIGHTEN,   "lighten",
              N_("Lighten"))
  enum_value (GEGL_BLEND_MODE_TYPE_OVERLAY,   "overlay",
              N_("Overlay"))
  enum_value (GEGL_BLEND_MODE_TYPE_SOFTLIGHT, "softlight",
              N_("Soft Light"))
  enum_value (GEGL_BLEND_MODE_TYPE_ADDITION,  "addition",
              N_("Addition"))
enum_end (GeglBlendModeTypedesignerlite)

property_enum (blendmode, _("Blend Mode of Internal Emboss"),
    GeglBlendModeTypedesignerlite, gegl_blend_mode_typedesignerlite,
    GEGL_BLEND_MODE_TYPE_HARDLIGHT)
  description (_("What blend mode the bevel's emboss will fuse with. Different blend modes will have radically different appearances and will effect the way sliders behave. "))
 /*Changing the name of a ENUM list will result in all presets breaking*/

enum_start (gegl_median_blur_neighborhooddlite)
  enum_value (GEGL_MEDIAN_BLUR_NEIGHBORHOOD_SQUAREcblite,  "squarecb",  N_("Square"))
  enum_value (GEGL_MEDIAN_BLUR_NEIGHBORHOOD_CIRCLEcblite,  "circlecb",  N_("Circle"))
  enum_value (GEGL_MEDIAN_BLUR_NEIGHBORHOOD_DIAMONDcblite, "diamondcb", N_("Diamond"))
enum_end (GeglMedianBlurNeighborhooddlite)
 /*Changing the name of a ENUM list will result in all presets breaking*/

property_double (azimuth, _("Emboss Azimuth"), 67.0)
    description (_("Light angle (degrees). For most blend modes this rotates lighting of the bevel."))
    value_range (30, 90)
    ui_meta ("unit", "degree")
    ui_meta ("direction", "ccw")


property_double (elevation, _("Emboss Elevation"), 25.0)
    description (_("Elevation angle (degrees). For most blend modes this shifts the lightest colors of the bevel."))
    value_range (7, 90)
    ui_meta ("unit", "degree")

property_int (depth, _("Emboss Depth"), 24)
    description (_("Emboss Depth. For some blend modes it adds depth and for others adds detail to the bevel."))
  ui_range    (1, 70)
    value_range (1, 100)

property_double (gaus, _("Normal Bevel Effect"), 1)
   description (_("Internal Gaussian Blur makes a bumpish bevel. Making this too high will create an undesirable effect. The larger the text the higher this slider should be."))
   value_range (0.0, 5.5)
   ui_range    (0.0, 4.0)
  ui_steps      (0.1, 1.0)

property_int (box, _("Sharp Bevel Effect"), 3)
   description(_("Internal Box Blur makes a sharp bevel. Making this too high will create an undesirable effect. The larger the text the higher this slider should be."))
   value_range (0, 9)
   ui_range    (0, 6)
  ui_steps      (1.0, 2.0)  

property_boolean(guichange, _("Detailed options"), FALSE)

property_boolean(mask_with_alpha, _("Mask with initial alpha"), TRUE)
ui_meta ("visible", "guichange")

property_enum (type, _("Choose Internal Median Shape"),
               GeglMedianBlurNeighborhooddlite, gegl_median_blur_neighborhooddlite,
               GEGL_MEDIAN_BLUR_NEIGHBORHOOD_CIRCLEcblite)
  description (_("Base shape of the median blur for the bevel. This effect is only prominent on very thin bevels. "))
ui_meta ("visible", "guichange")

property_double (opacity, _("Widen bevel by increasing internal opacity"), 8)
    description (_("Opacity boost, for widening bevel."))
    value_range (3, 10)
    ui_range    (4, 10)
ui_meta ("visible", "guichange")

property_int  (size, _("Internal Median Blur Radius"), 1)
  value_range (0, 7)
  ui_range    (0, 7)
  ui_meta     ("unit", "pixel-distance")
  description (_("An internal median blur radius set to thin the bevel in default. If internal median blur alpha percentile is high it will make the bevel fat."))
ui_meta ("visible", "guichange")

property_double  (alphapercentile, _("Internal Median Blur Alpha percentile"), 0)
  value_range (0, 100)
  description (_("Median Blur's alpha percentile being applied internally"))
ui_meta ("visible", "guichange")


property_int  (mcb, _("Smooth rough pixels on the Bevel"), 0)
  description (_("Applies a mild mean curvature blur on the bevel"))
  value_range (0, 2)
  ui_range    (0, 2)
ui_meta ("visible", "guichange")


#else

#define GEGL_OP_META
#define GEGL_OP_NAME     bevel
#define GEGL_OP_C_SOURCE bevel.c

#include "gegl-op.h"

typedef struct
{
  GeglNode *input;
  GeglNode *median;
  GeglNode *box;
  GeglNode *gaussian;
  GeglNode *blend;
  GeglNode *emboss;
  GeglNode *opacity;
  GeglNode *extract_alpha;
  GeglNode *mask;
  GeglNode *mcb;
  GeglNode *alpha_clip;
  GeglNode *output;
}State;

static void
update_graph (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  State *state = o->user_data;

  if (!state)
    return;

  const char *blend_op = "gegl:nop";
  switch (o->blendmode) {
    case GEGL_BLEND_MODE_TYPE_HARDLIGHT: blend_op = "gegl:hard-light"; break;
    case GEGL_BLEND_MODE_TYPE_MULTIPLY: blend_op = "gegl:multiply"; break;
    case GEGL_BLEND_MODE_TYPE_COLORDODGE: blend_op = "gegl:color-dodge"; break;
    case GEGL_BLEND_MODE_TYPE_PLUS: blend_op = "gegl:plus"; break;
    case GEGL_BLEND_MODE_TYPE_DARKEN: blend_op = "gegl:darken"; break;
    case GEGL_BLEND_MODE_TYPE_LIGHTEN: blend_op = "gegl:lighten"; break;
    case GEGL_BLEND_MODE_TYPE_OVERLAY: blend_op = "gegl:overlay"; break;
    case GEGL_BLEND_MODE_TYPE_SOFTLIGHT: blend_op = "gegl:soft-light"; break;
    case GEGL_BLEND_MODE_TYPE_ADDITION: blend_op = "gegl:add"; break;
  }
  gegl_node_set (state->blend, "operation", blend_op, NULL);

  if (o->mask_with_alpha)
    gegl_node_link_many (state->alpha_clip, state->output,  NULL);
  else
    gegl_node_link_many (state->alpha_clip,  state->mask,
                         state->output,  NULL);
}

static void attach (GeglOperation *operation)
{
  GeglNode *gegl = operation->node;
  GeglProperties *o = GEGL_PROPERTIES (operation);

  State *state = o->user_data = g_malloc0 (sizeof (State));

  state->input     = gegl_node_get_input_proxy  (gegl, "input");
  state->output    = gegl_node_get_output_proxy (gegl, "output");
  state->median    = gegl_node_new_child (gegl, "operation", "gegl:median-blur",
                                                "percentile", 53.0,
                                          NULL);
  state->blend      = gegl_node_new_child (gegl, "operation", "gegl:hard-light",
                                           NULL);
  state->opacity    = gegl_node_new_child (gegl, "operation", "gegl:opacity",
                                           NULL);
  state->extract_alpha = gegl_node_new_child (gegl, "operation", "gegl:component-extract", NULL);
  state->mask = gegl_node_new_child (gegl, "operation", "gegl:opacity",
                                           NULL);
  gegl_node_set_enum_as_string (state->extract_alpha, "component", "alpha");
  state->gaussian   = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur",
                                                 "filter",    1,
                                           NULL);
 /*Filter 1 is code for gaussian-blur filter=fir. Which makes bevel less puffy. */

  state->emboss     = gegl_node_new_child (gegl, "operation", "gegl:emboss",
                                           NULL);
  state->box        = gegl_node_new_child (gegl, "operation", "gegl:box-blur",
                                           NULL);
  state->mcb        = gegl_node_new_child (gegl, "operation", "gegl:mean-curvature-blur",
                                           NULL);
  state->alpha_clip = gegl_node_new_child (gegl, "operation", "gegl:alpha-clip",
                                           NULL);

  gegl_node_link_many (state->input, state->median, state->box,
                       state->gaussian, state->blend, state->opacity,
                       state->mcb,  state->alpha_clip,  state->mask,
                       state->output,  NULL);
  gegl_node_link_many (state->gaussian, state->emboss, NULL);
  gegl_node_connect (state->emboss, "output", state->blend, "aux");
  gegl_node_link (state->input, state->extract_alpha);
  gegl_node_connect (state->extract_alpha, "output", state->mask, "aux");

  gegl_operation_meta_redirect (operation, "size",
                                state->median, "radius");
  gegl_operation_meta_redirect (operation, "gaus",
                                state->gaussian, "std-dev-x");
  gegl_operation_meta_redirect (operation, "gaus",
                                state->gaussian, "std-dev-y");
  gegl_operation_meta_redirect (operation, "azimuth",
                                state->emboss, "azimuth");
  gegl_operation_meta_redirect (operation, "elevation",
                                state->emboss, "elevation");
  gegl_operation_meta_redirect (operation, "depth",
                                state->emboss, "depth");
  gegl_operation_meta_redirect (operation, "alphapercentile",
                                state->median, "alpha-percentile");
  gegl_operation_meta_redirect (operation, "opacity",
                                state->opacity, "value");
  gegl_operation_meta_redirect (operation, "mcb",
                                state->mcb, "iterations");
  gegl_operation_meta_redirect (operation, "box",
                                state->box, "radius");
  gegl_operation_meta_redirect (operation, "type",
                                state->median, "neighborhood");

}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;
  GeglOperationMetaClass *operation_meta_class = GEGL_OPERATION_META_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;
  operation_meta_class->update = update_graph;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:bevel",
    "title",       _("Bevel"),
    "categories",  "Artistic",
    "reference-hash", "11lighth3do6akv00vyeefjf25sb2ac",
    "description", _("Design a custom bevel or bump effect. This filter is meant for shapes and text."),
    "gimp:menu-path", "<Image>/Filters/Light and Shadow",
    "gimp:menu-label", _("Bevel..."),
    NULL);
}

#endif
