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
 *                 2023 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (chamfer_blend_mode)
  enum_value (CHAMFER_BLEND_HARDLIGHT, "hardlight",
              N_("HardLight"))
  enum_value (CHAMFER_BLEND_MULTIPLY,  "multiply",
              N_("Multiply"))
  //enum_value (CHAMFER_BLEND_COLORDODGE, "colordodge",
  //            N_("ColorDodge"))
  enum_value (CHAMFER_BLEND_DARKEN,    "darken",
              N_("Darken"))
  enum_value (CHAMFER_BLEND_LIGHTEN,   "lighten",
              N_("Lighten"))
//enum_value (CHAMFER_BLEND_OVERLAY,   "overlay",
//            N_("Overlay"))
  enum_value (CHAMFER_BLEND_SOFTLIGHT, "softlight",
              N_("Soft Light"))
enum_end (ChamferBlendMode)

enum_start (chamfer_median_neighborhood)
  enum_value (CHAMFER_MEDIAN_NEIGHBORHOOD_SQUARE,  "square",  N_("Square"))
  enum_value (CHAMFER_MEDIAN_NEIGHBORHOOD_CIRCLE,  "circle",  N_("Circle"))
  enum_value (CHAMFER_MEDIAN_NEIGHBORHOOD_DIAMOND, "diamond", N_("Diamond"))
enum_end (ChamferMedianNeighborhood)
 /*Changing the name of a ENUM list will result in all presets breaking*/

property_enum (blendmode, _("Blend Mode"),
    ChamferBlendMode, chamfer_blend_mode,
    CHAMFER_BLEND_HARDLIGHT)
  description (_("What blending mode a light map will be applied with."))
 /*Changing the name of a ENUM list will result in all presets breaking*/



property_double (strength, _("Strength"), 0.3)
    value_range (0, 1.0)

property_double (depth, _("Depth"), 0.5)
  value_range (0.01, 1.0)

property_double (curvature, _("Curvature"), 1.0)
  description (_("Curvature at 0.5 we are close to a circle shape, 1.0 is straight, values above 1.0 are concave."))
  value_range (0.0,8.0)
  ui_range (0.0,4.0)
  ui_gamma (2.0)

property_double (azimuth, _("Light direction"), 67.0)
    description (_("Light angle (degrees). For most blend modes this rotates lighting of the bevel."))
    value_range (0, 360)
    ui_meta ("unit", "degree")
    ui_meta ("direction", "ccw")


property_boolean(detailed_options, _("Detailed options"), FALSE)

property_boolean(mask_with_alpha, _("Mask with initial alpha"), TRUE)
ui_meta ("visible", "detailed_options")
    description (_("Keep alpha coverage, avoids growing of the shape beyond initial position."))

property_boolean(use_dt, ("Use distance-transform"), TRUE)
   ui_meta ("visible", "detailed_options")
    description (("Use gegl:distance-transform for computing base-shape, ideally the curvature and thickness controls also control the non-distance-transform case."))

property_double (elevation, _("Light Elevation"), 12.5)
    description (_("Elevation angle (degrees). For most blend modes this shifts the lightest colors of the bevel."))
    value_range (0, 90)
    ui_meta ("unit", "degree")
    ui_meta ("visible", "detailed_options")

property_int (emboss_depth, ("Emboss Depth"), 1)
    description (("Emboss Depth. For some blend modes it adds depth and for others adds detail to the bevel."))
  ui_range    (1, 70)
    value_range (1, 100)
   ui_meta ("visible", "detailed_options")

property_int  (dt_mcb_iterations, ("Distance transform smoothing"), 7)
  description (("Applies a mild mean curvature blur on the bevel"))
  value_range (0, 10)
  ui_range    (0, 10)
  ui_meta ("visible", "detailed_options")

property_double (gaus, ("Normal Bevel Effect"), 1)
   description (("Internal Gaussian Blur makes a bumpish bevel. Making this too high will create an undesirable effect. The larger the text the higher this slider should be."))
   value_range (0.0, 5.5)
   ui_range    (0.0, 4.0)
   ui_steps      (0.1, 1.0)
   ui_meta ("visible", "detailed_options")

property_int (box, ("Sharp Bevel Effect"), 3)
   description(("Internal Box Blur makes a sharp bevel. Making this too high will create an undesirable effect. The larger the text the higher this slider should be."))
   value_range (0, 9)
   ui_range    (0, 6)
   ui_steps      (1.0, 2.0)  
   ui_meta ("visible", "detailed_options")

property_enum (type, _("Choose Internal Median Shape"),
               ChamferMedianNeighborhood, chamfer_median_neighborhood,
               CHAMFER_MEDIAN_NEIGHBORHOOD_CIRCLE)
  description (("Base shape of the median blur for the bevel. This effect is only prominent on very thin bevels. "))
ui_meta ("visible", "detailed_options")

property_double (opacity_boost, ("Widen bevel by increasing internal opacity"), 8)
    description (("Opacity boost, for widening bevel."))
    value_range (3, 12)
    ui_range    (3, 10)
    ui_meta ("visible", "detailed_options")

property_int  (size, ("Internal Median Blur Radius"), 1)
  value_range (0, 7)
  ui_range    (0, 7)
  ui_meta     ("unit", "pixel-distance")
  description (("An internal median blur radius set to thin the bevel in default. If internal median blur alpha percentile is high it will make the bevel fat."))
ui_meta ("visible", "detailed_options")

property_double  (alphapercentile, ("Internal Median Blur Alpha percentile"), 0)
  value_range (0, 100)
  description (("Median Blur's alpha percentile being applied internally"))
  ui_meta ("visible", "detailed_options")


property_int  (mcb_iterations, ("Smooth rough pixels on the Bevel"), 0)
  description (("Applies a mild mean curvature blur on the bevel"))
  value_range (0, 2)
  ui_range    (0, 2)
  ui_meta ("visible", "detailed_options")


#else

#define GEGL_OP_META
#define GEGL_OP_NAME     chamfer 
#define GEGL_OP_C_SOURCE chamfer.c

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

  GeglNode *distance_transform;
  GeglNode *dt_mcb;
  GeglNode *gamma;
  GeglNode *divide;
  GeglNode *mul;
  GeglNode *rgb_clip;
  GeglNode *src_in;
  GeglNode *white;
  GeglNode *crop;

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
    case CHAMFER_BLEND_HARDLIGHT:  blend_op = "gegl:hard-light"; break;
    case CHAMFER_BLEND_MULTIPLY:   blend_op = "gegl:multiply"; break;
    //case CHAMFER_BLEND_COLORDODGE: blend_op = "gegl:color-dodge"; break;
    case CHAMFER_BLEND_DARKEN:     blend_op = "gegl:darken"; break;
    case CHAMFER_BLEND_LIGHTEN:    blend_op = "gegl:lighten"; break;
    //case CHAMFER_BLEND_OVERLAY:    blend_op = "gegl:overlay"; break;
    case CHAMFER_BLEND_SOFTLIGHT:  blend_op = "gegl:soft-light"; break;
  }
  gegl_node_set (state->blend, "operation", blend_op, NULL);


  if (o->use_dt)
  {
    gegl_node_link_many (state->input, state->blend, state->mask, state->output,  NULL);
    gegl_node_connect (state->crop, "input", state->white, "output");
    gegl_node_connect (state->crop, "aux", state->input, "output");
    gegl_node_connect (state->src_in, "aux", state->crop, "output");
    gegl_node_connect (state->emboss, "output", state->blend, "aux");
    gegl_node_link_many (state->input, state->src_in, state->distance_transform, state->divide, state->rgb_clip, state->mul, state->dt_mcb, state->gamma, state->emboss, NULL);
  }
  else
  {
    gegl_node_link_many (state->gaussian, state->blend, state->opacity,
                         state->mcb, state->mask, state->alpha_clip,
                         state->output,  NULL);
    gegl_node_link_many (state->gaussian, state->emboss, NULL);
  }

  gegl_node_set_passthrough (state->mask, !o->mask_with_alpha);
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

  //state->dt_gaussian   = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur", NULL);
  state->alpha_clip = gegl_node_new_child (gegl, "operation", "gegl:alpha-clip",
                                           NULL);
  state->dt_mcb     = gegl_node_new_child (gegl, "operation", "gegl:mean-curvature-blur",
"iterations", 0,
                                           NULL);
  state->gamma = gegl_node_new_child (gegl, "operation", "gegl:gamma",
                                           NULL);
  state->divide = gegl_node_new_child (gegl, "operation", "gegl:divide",
                                           NULL);
  state->mul = gegl_node_new_child (gegl, "operation", "gegl:multiply",
                                        "value", 0.25,
                                           NULL);
  state->rgb_clip = gegl_node_new_child (gegl, "operation", "gegl:rgb-clip",
                                           NULL);
  state->white = gegl_node_new_child (gegl, "operation", "gegl:color", NULL);
  state->crop = gegl_node_new_child (gegl, "operation", "gegl:crop",
                                     NULL);

  {GeglColor *color = gegl_color_new("white");
   gegl_node_set (state->white, "value", color, NULL);
   g_object_unref (color);
  }
  state->src_in = gegl_node_new_child (gegl, "operation", "gegl:dst-in", NULL);
  state->distance_transform = gegl_node_new_child (gegl, "operation", "gegl:distance-transform",
                                           NULL);

  gegl_node_link_many (state->input, state->median, state->box,
                       state->gaussian, state->alpha_clip, state->blend, state->opacity,
                       state->mcb,  state->mask,
                       state->output,  NULL);
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
  gegl_operation_meta_redirect (operation, "emboss-depth",
                                state->emboss, "depth");
  gegl_operation_meta_redirect (operation, "alphapercentile",
                                state->median, "alpha-percentile");
  gegl_operation_meta_redirect (operation, "opacity-boost",
                                state->opacity, "value");
  gegl_operation_meta_redirect (operation, "mcb-iterations",
                                state->mcb, "iterations");
  gegl_operation_meta_redirect (operation, "dt-mcb-iterations",
                                state->dt_mcb, "iterations");
  gegl_operation_meta_redirect (operation, "box",
                                state->box, "radius");
  gegl_operation_meta_redirect (operation, "type",
                                state->median, "neighborhood");
  gegl_operation_meta_redirect (operation, "curvature",
                                state->gamma, "value");
  gegl_operation_meta_redirect (operation, "depth",
                                state->divide, "value");
  gegl_operation_meta_redirect (operation, "strength",
                                state->mul, "value");

}

static void
dispose (GObject *object)
{
   GeglProperties  *o = GEGL_PROPERTIES (object);
   g_clear_pointer (&o->user_data, g_free);
   G_OBJECT_CLASS (gegl_op_parent_class)->dispose (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass           *object_class = G_OBJECT_CLASS (klass);
  GeglOperationClass     *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationMetaClass *operation_meta_class = GEGL_OPERATION_META_CLASS (klass);

  object_class->dispose        = dispose;
  operation_class->attach      = attach;
  operation_meta_class->update = update_graph;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:chamfer",
    "title",       _("Chamfer"),
    "reference-hash", "11lighth3do6akv00vyeefjf25sb2ac",
    "description", _("Simulate lighting of a chamfered 3D-edges for an alpha-defined shape."),
    "gimp:menu-path", "<Image>/Filters/Light and Shadow",
    NULL);
}

#endif
