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
 * 2023 Sam Lester, GEGL Styles is based on a 2022 text styling plugin, BarefootLiam helped with inner glow's checkbox

If it was not for me recreating GEGL Drop Shadow like graph from scratch in the "All nodes relating to Outline here" this filter would have been watered down.
I hope Gimp's team is not overwhelmed by my complexity because this is the first GEGL operation to use 55+ or more nodes when everything is enabled
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES
/* This is an ENUM list that divides GEGL Styles into three parts*/
enum_start (partoffiltertobedisplayed)
enum_value   (GEGL_UI_OUTLINESHADOW, "outlineshadow", N_("Color, Outline and Shadow"))
enum_value   (GEGL_UI_INNERGLOWBEVEL, "innerglowbevel", N_("Bevel and Inner Glow"))
enum_value   (GEGL_UI_IMAGEOUTLINEBEVEL, "imageoutlinebevel", N_("Image upload and Outline Bevel"))
  enum_end (endoflist)

    /* List of all GUI options */

    /* GUI Change list in GUI */

property_enum(guichange, _("Part of filter to be displayed"),
    endoflist, partoffiltertobedisplayed,
    GEGL_UI_OUTLINESHADOW)
  description(_("Display a different part of the GUI"))

    /* Color Overlays GUI options begin here */

property_color  (color_fill, _("Color overlay"), "#ffffff")
  ui_meta ("visible", "guichange {outlineshadow}")
    description (_("Color overlay setting"))

property_enum (color_policy, _("Color policy"),
    description (_("Change the blend mode of the color overlay"))
    GeglBlendColorOverlay, gegl_blend_color_overlay,
    GEGL_BLEND_MODE_TYPE_MULTIPLY_COLOR)
  ui_meta ("visible", "guichange {outlineshadow}")

enum_start (gegl_blend_color_overlay)
  enum_value (GEGL_BLEND_MODE_TYPE_NO_COLOR,      "nocolor",
              N_("No Color"))
  enum_value (GEGL_BLEND_MODE_TYPE_MULTIPLY_COLOR,      "multiply",
              N_("Multiply"))
  enum_value (GEGL_BLEND_MODE_TYPE_SOLID_COLOR,      "solidcolor",
              N_("Solid Color"))
enum_end (GeglBlendColorOverlay)


    /* Outline's normal GUI options begin here */

property_boolean (enableoutline, _("Enable Outline"), FALSE)
  description    (_("Disable or enable the outline"))
  ui_meta ("visible", "guichange {outlineshadow}")

property_double (outline_opacity, _("Outline opacity"), 1)
  value_range   (0.0, 1.0)
  ui_steps      (0.01, 0.10)
  description    (_("Opacity of the outline"))
  ui_meta ("visible", "guichange {outlineshadow}")
  ui_meta     ("sensitive", " enableoutline")

property_double (outline_x, _("Outline X"), 0.0)
  description   (_("Horizontal outline offset"))
  ui_range      (-15.0, 15.0)
  ui_steps      (1, 10)
  ui_meta       ("axis", "x")
  ui_meta ("visible", "guichange {outlineshadow}")
  ui_meta     ("sensitive", " enableoutline")

property_double (outline_y, _("Outline Y"), 0.0)
  description   (_("Vertical outline offset"))
  ui_range      (-15.0, 15.0)
  ui_steps      (1, 10)
  ui_meta       ("axis", "y")
  ui_meta ("visible", "guichange {outlineshadow}")
  ui_meta     ("sensitive", " enableoutline")

/* Should correspond to GeglMedianBlurNeighborhood in median-blur.c */
enum_start (gegl_styles_grow_shapes)
  enum_value (GEGL_stroke_GROW_SHAPE_SQUARE,  "square",  N_("Square"))
  enum_value (GEGL_stroke_GROW_SHAPE_CIRCLE,  "circle",  N_("Circle"))
  enum_value (GEGL_stroke_GROW_SHAPE_DIAMOND, "diamond", N_("Diamond"))
enum_end (shapegrowend)

property_enum   (outline_grow_shape, _("Outline grow shape"),
                 shapegrowend, gegl_styles_grow_shapes,
                 GEGL_stroke_GROW_SHAPE_CIRCLE)
  description   (_("The shape to expand or contract the outline in"))
  ui_meta ("visible", "guichange {outlineshadow}")
  ui_meta     ("sensitive", " enableoutline")

property_double (outline_blur, _("Outline blur radius"), 0.0)
  value_range   (0.0, 3)
  ui_range      (0.0, 3.0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  description    (_("A mild blur for the outline"))
  ui_meta ("visible", "guichange {outlineshadow}")
  ui_meta     ("sensitive", " enableoutline")

property_double (outline, _("Outline grow radius"), 12.0)
  value_range   (0.0, G_MAXDOUBLE)
  ui_range      (0, 100.0)
  ui_digits     (0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  description (_("The distance to expand the outline"))
  ui_meta ("visible", "guichange {outlineshadow}")
  ui_meta     ("sensitive", " enableoutline")

    /* Shadow's GUI options begin here */
property_color (outline_color, _("Outline’s color"), "#000000")
  description    (_("Color of the outline (defaults to 'black') "))
    /* TRANSLATORS: the string 'black' should not be translated */
  ui_meta ("visible", "guichange {outlineshadow}")
  ui_meta     ("sensitive", " enableoutline")

property_double (shadow_opacity, _("Shadow/Glow opacity"), 0.0)
  value_range   (0.0, 1.0)
  ui_range      (0.0, 1.0)
  ui_steps      (0.01, 0.10)
  description    (_("Shadow opacity which will also enable or disable the shadow glow effect"))
  ui_meta ("visible", "guichange {outlineshadow}")


property_double (shadow_x, _("Shadow/Glow X"), 10.0)
  description   (_("Horizontal shadow offset"))
  ui_range      (-40.0, 40.0)
  ui_steps      (1, 10)
  ui_meta       ("unit", "pixel-distance")
  ui_meta       ("axis", "x")
  description    (_("Horizontal axis of the shadow glow"))
  ui_meta ("visible", "guichange {outlineshadow}")

property_double (shadow_y, _("Shadow/Glow Y"), 10.0)
  description   (_("Vertical shadow offset"))
  ui_range      (-40.0, 40.0)
  ui_steps      (1, 10)
  ui_meta       ("unit", "pixel-distance")
  ui_meta       ("axis", "y")
  description    (_("Vertical axis of the shadow glow"))
  ui_meta ("visible", "guichange {outlineshadow}")

property_color  (shadow_color, _("Shadow/Glow’s color"), "black")
    /* TRANSLATORS: the string 'black' should not be translated */
  description   (_("The shadow’s color (defaults to 'black')"))
  ui_meta ("visible", "guichange {outlineshadow}")

property_double (shadow_grow_radius, _("Shadow/Glow grow radius"), 0.0)
  value_range   (0.0, 100.0)
  ui_range      (0.0, 50.0)
  ui_digits     (0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  description (_("The distance to expand the shadow before blurring."))
  ui_meta ("visible", "guichange {outlineshadow}")

property_double (shadow_radius, _("Shadow/Glow blur radius"), 12.0)
  value_range   (0.0, G_MAXDOUBLE)
  ui_range      (0.0, 110.0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  description    (_("The shadow’s blur range"))
  ui_meta ("visible", "guichange {outlineshadow}")

    /* Bevel's GUI options begin here */

    /* This is the Bevel's ENUM List listed in the GUI */

/* This is the ENUM list for bevel's emboss blend modes */
property_boolean (enablebevel, _("Enable Bevel"), FALSE)
  description   (_("Whether to add a bevel effect"))
  ui_meta ("visible", "guichange {innerglowbevel}")


enum_start (gegl_blend_mode_for_bevel)
  enum_value (GEGL_BLEND_MODE_TYPE_MULTIPLY_BEVEL,      "multiply",
              N_("Multiply"))
  enum_value (GEGL_BLEND_MODE_TYPE_ADD_BEVEL,      "add",
              N_("Add"))
  enum_value (GEGL_BLEND_MODE_TYPE_HARDLIGHT_BEVEL,      "hardlight",
              N_("Hard Light"))
  enum_value (GEGL_BLEND_MODE_TYPE_DARKEN_BEVEL,      "darken",
              N_("Darken"))
  enum_value (GEGL_BLEND_MODE_TYPE_COLORDODGE_BEVEL,      "colordodge",
              N_("Color Dodge"))
enum_end (GeglBlendModeForBevel)

property_enum (bevel_blend, _("Bevel blend mode"),
    description (_("Blend mode of the bevel"))
    GeglBlendModeForBevel, gegl_blend_mode_for_bevel,
    GEGL_BLEND_MODE_TYPE_MULTIPLY_BEVEL)
  ui_meta ("visible", "guichange {innerglowbevel}")
  ui_meta     ("sensitive", " enablebevel")

enum_start (gbevel_listing)
  enum_value (GEGL_BEVEL_CHAMFER,      "chamferbevel",
              N_("Chamfer Bevel"))
  enum_value (GEGL_BEVEL_BUMP,      "bumpbevel",
              N_("Bump Bevel"))
enum_end (gbevellisting)

property_enum (bevel_type, _("Select Bevel"),
    gbevellisting, gbevel_listing,
    GEGL_BEVEL_BUMP)
    description (_("Type of bevel "))
  ui_meta ("visible", "guichange {innerglowbevel}")
  ui_meta     ("sensitive", " enablebevel")



property_int (bevel_depth, _("Bevel depth"), 65)
    description (_("Emboss depth for the bevel"))
    value_range (1, 100)
  ui_meta ("visible", "guichange {innerglowbevel}")
  ui_meta     ("sensitive", " enablebevel")


property_double (bevel_elevation, _("Bevel elevation"), 55.0)
    description (_("Emboss elevation of the bevel"))
    value_range (55, 125)
    ui_meta ("unit", "degree")
  ui_meta ("visible", "guichange {innerglowbevel}")
  ui_meta     ("sensitive", " enablebevel")


property_double (bevel_azimuth, _("Bevel azimuth"), 75.0)
    description (_("Emboss azimuth of the bevel"))
    value_range (0.0, 360.0)
  ui_steps      (0.01, 0.50)
  ui_meta ("visible", "guichange {innerglowbevel}")
  ui_meta     ("sensitive", " enablebevel")


property_double (bevel_radius, _("Bevel radius (bump only)"), 5.0)
  value_range (1.0, 8.0)
  ui_range (1.0, 8.0)
  ui_gamma (1.5)
  description    (_("Internal gaussian blur to inflate the bump bevel. This option does not work on chamfer"))
  ui_meta ("visible", "guichange {innerglowbevel}")
  ui_meta     ("sensitive", " enablebevel")



property_double (bevel_outlow, _("Levels low output lighting for Bevel"), 0.0)
    description (_("Levels low output as a light adjustment for the bevel"))
    ui_range    (0.0, 0.2)
    value_range    (0.0, 0.2)
  ui_steps      (0.01, 0.50)
  ui_meta ("visible", "guichange {innerglowbevel}")
  ui_meta     ("sensitive", " enablebevel")


property_double (bevel_outhigh, _("Levels high output lighting for Bevel"), 1.0)
    description (_("Levels high output as a light adjustment for the bevel"))
    ui_range    (1.0, 1.2)
    value_range    (1.0, 1.2)
  ui_steps      (0.01, 0.50)
  ui_meta ("visible", "guichange {innerglowbevel}")
  ui_meta     ("sensitive", " enablebevel")


property_double (bevel_dark, _("Dark Bevel/ignore image mode"), 0.00)
    description (_("This instructs the bevel to ignore image details if there is an image file overlay below it. It also allows bevel to work better when the user selects darker colors. The effect is noticeable on bump bevel but barely effects chamfer bevel"))
  value_range   (0.00, 1.0)
  ui_steps      (0.01, 0.50)
  ui_meta ("visible", "guichange {innerglowbevel}")
  ui_meta     ("sensitive", " enablebevel")

    /* This operation is extremely important for bevels with image file overlays */

    /* Inner Glow GUI options begin here */

property_boolean (enableinnerglow, _("Enable Inner Glow"), FALSE)
  description   (_("Whether to add the inner glow effect"))
  ui_meta ("visible", "guichange {innerglowbevel}")

    /* This is the Inner Glow's ENUM List  */

/* This is the ENUM list for inner glow's blend modes
ALT means their srgb=true is true  */
enum_start (gegl_blend_mode_type_innerglowblend)
  enum_value (GEGL_BLEND_MODE_TYPE_NORMALIG,      "normal",
              N_("Normal"))
  enum_value (GEGL_BLEND_MODE_TYPE_OVERLAYIG,      "overlay",
              N_("Overlay"))
  enum_value (GEGL_BLEND_MODE_TYPE_SCREENIG,      "screen",
              N_("Screen"))
  enum_value (GEGL_BLEND_MODE_TYPE_HARDLIGHTIG,      "hardlight",
              N_("Hard Light"))
  enum_value (GEGL_BLEND_MODE_TYPE_COLORDODGEIG,      "colordodge",
              N_("Color Dodge"))
  enum_value (GEGL_BLEND_MODE_TYPE_PLUSIG,      "plus",
              N_("Plus"))
enum_end (GeglBlendModeTypeigblend)

property_enum (ig_blend, _("Blend mode of Inner Glow"),
    GeglBlendModeTypeigblend, gegl_blend_mode_type_innerglowblend,
    GEGL_BLEND_MODE_TYPE_NORMALIG)
  ui_meta ("visible", "guichange {innerglowbevel}")
  ui_meta     ("sensitive", " enableinnerglow")

property_double (ig_radius, _("Inner Glow's blur radius"), 6.0)
  value_range   (0.0, 30.0)
  ui_range      (0.0, 30.0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  description (_("Blur control of the inner glow"))
  ui_meta ("visible", "guichange {innerglowbevel}")
  ui_meta     ("sensitive", " enableinnerglow")

property_double (ig_grow_radius, _("Inner Glow's grow radius"), 5)
  value_range   (1, 30.0)
  ui_range      (1, 30.0)
  ui_digits     (0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  description (_("The distance to expand the inner glow before blurring"))
  ui_meta ("visible", "guichange {innerglowbevel}")
  ui_meta     ("sensitive", " enableinnerglow")

property_double (ig_opacity, _("Inner Glow's opacity"), 1.00)
  value_range   (0.0, 1.00)
  ui_steps      (0.01, 0.10)
  description (_("Opacity of the inner glow"))
  ui_meta ("visible", "guichange {innerglowbevel}")
  ui_meta     ("sensitive", " enableinnerglow")

property_color (ig_value, _("Inner Glow's color"), "#ff8f00")
    description (_("The color of the inner glow"))
  ui_meta ("visible", "guichange {innerglowbevel}")
  ui_meta     ("sensitive", " enableinnerglow")

property_double  (ig_treatment, _("Inner Glow's unmodified pixel fix"), 60)
  value_range (50, 85)
  description (_("Cover pixels that inner glow might miss"))
  ui_meta ("visible", "guichange {innerglowbevel}")
  ui_meta     ("sensitive", " enableinnerglow")
 /* Without this Inner Glow would miss many pixels around alpha defined shapes edges'*/

    /* Image file overlay GUI options begin here */

property_boolean (enableimage, _("Enable image upload"), FALSE)
  description   (_("Whether to enable or disable the image file upload."))
ui_meta ("visible", "guichange {imageoutlinebevel}")

property_file_path(image_src, _("Image file overlay"), "")
  description (_("Upload an image file from your computer to be in the fill area. Allows (png, jpg, raw, svg, bmp, tif, ...)"))
ui_meta ("visible", "guichange {imageoutlinebevel}")
  ui_meta     ("sensitive", " enableimage")

property_double(image_opacity, _("Opacity of image file overlay"), 1.0)
    value_range (0.0, 1.0)
ui_meta ("visible", "guichange {imageoutlinebevel}")
  description (_("Opacity of the image file overlay that was uploaded. "))
  ui_meta     ("sensitive", " enableimage")

    /*
property_double (imagehue, _("Hue rotation"),  0.0)
   description  (_("Hue adjustment"))
   value_range  (-180.0, 180.0)
  description (_("Hue rotation of the uploaded image file or whatever is on canvas"))
ui_meta ("visible", "guichange {imageoutlinebevel}") */

property_double (image_saturation, _("Saturation"), 1.0)
   description  (_("Saturation adjustment of the uploaded image file or whatever is on canvas"))
   value_range  (0.0, 3.0)
ui_meta ("visible", "guichange {imageoutlinebevel}")

property_double (image_lightness, _("Lightness"), 0.0)
   description  (_("Lightness adjustment of the uploaded image file or whatever is on canvas"))
   value_range  (-20.0, 20.0)
ui_meta ("visible", "guichange {imageoutlinebevel}")

    /* Outline special options in GUI begin here */
property_boolean (enablespecialoutline, _("Enable advanced options on Outline (requires outline)"), FALSE)
  description    (_("Turn on advanced outline abilities"))
ui_meta ("visible", "guichange {imageoutlinebevel}")
  ui_meta     ("sensitive", " enableoutline")


property_boolean (enableoutlinebevel, _("Enable Outline Bevel (requires advanced options)"), TRUE)
  description    (_("Turn on outlines ability to bevel"))
ui_meta ("visible", "guichange {imageoutlinebevel}")
  ui_meta     ("sensitive", " enablespecialoutline")



/* This is the ENUM list for bevel outline's emboss blend modes */
enum_start (gegl_blend_mode_for_bevel_outline)
  enum_value (GEGL_BLEND_MODE_TYPE_MULTIPLY_BEVEL_OUTLINE,      "multiply",
              N_("Multiply"))
  enum_value (GEGL_BLEND_MODE_TYPE_ADD_BEVEL_OUTLINE,      "add",
              N_("Add"))
  enum_value (GEGL_BLEND_MODE_TYPE_HARDLIGHT_BEVEL_OUTLINE,      "hardlight",
              N_("Hard Light"))
  enum_value (GEGL_BLEND_MODE_TYPE_DARKEN_BEVEL_OUTLINE,      "darken",
              N_("Darken"))
  enum_value (GEGL_BLEND_MODE_TYPE_COLORDODGE_BEVEL_OUTLINE,      "colordodge",
              N_("Color Dodge"))
enum_end (GeglBlendModeForBevelOutline)


property_enum (os_blend, _("Outline Bevel blend mode"),
    description (_("Blend mode of the outline bevel"))
    GeglBlendModeForBevelOutline, gegl_blend_mode_for_bevel_outline,
    GEGL_BLEND_MODE_TYPE_MULTIPLY_BEVEL_OUTLINE)
ui_meta ("visible", "guichange {imageoutlinebevel}")
  ui_meta     ("sensitive", " enablespecialoutline")


property_int (os_depth, _("Outline Bevel depth"), 15)
    description (_("Emboss depth for the outline bevel"))
    value_range (1, 100)
ui_meta ("visible", "guichange {imageoutlinebevel}")
  ui_meta     ("sensitive", " enablespecialoutline")

property_double (os_elevation, _("Outline Bevel elevation"), 47.0)
    description (_("Emboss elevation for the outline bevel"))
    value_range (0, 180)
    ui_meta ("unit", "degree")
ui_meta ("visible", "guichange {imageoutlinebevel}")
  ui_meta     ("sensitive", " enablespecialoutline")

property_double (os_azimuth, _("Outline Bevel azimuth"), 55.0)
    description (_("Emboss azimuth for the outline bevel"))
    value_range (0.0, 360.0)
    ui_steps      (0.01, 0.50)
ui_meta ("visible", "guichange {imageoutlinebevel}")
  ui_meta     ("sensitive", " enablespecialoutline")


property_double (os_radius, _("Outline Bevel radius"), 3.0)
    description (_("Internal gaussian blur to inflate the outline bevel"))
  value_range (1.0, 8.0)
  ui_range (1.0, 8.0)
  ui_gamma (1.5)
ui_meta ("visible", "guichange {imageoutlinebevel}")
  ui_meta     ("sensitive", " enablespecialoutline")


property_boolean (enableimageoutline, _("Enable image upload on Outline (requires advanced options)"), FALSE)
  description   (_("Whether to enable or disable the image file upload."))
ui_meta ("visible", "guichange {imageoutlinebevel}")
  ui_meta     ("sensitive", " enablespecialoutline")

property_file_path(os_src, _("Outline image file overlay"), "")
  description (_("Upload an image file from your computer to be in the outline area. Allows (png, jpg, raw, svg, bmp, tif, ...)"))
ui_meta ("visible", "guichange {imageoutlinebevel}")
  ui_meta     ("sensitive", " enablespecialoutline")


property_double (os_src_opacity, _("Outline image opacity"), 1.0)
   description  (_("Outline image opacity adjustment"))
   value_range  (0.0, 1.0)
ui_meta ("visible", "guichange {imageoutlinebevel}")
  ui_meta     ("sensitive", " enablespecialoutline")

property_double (os_outlow, _("Levels low output lighting for Outline Bevel"), 0.0)
    description (_("Levels low output as a light adjustment for the outline bevel"))
    ui_range    (0.0, 0.2)
    value_range (0.0, 0.2)
  ui_steps      (0.01, 0.50)
ui_meta ("visible", "guichange {imageoutlinebevel}")
  ui_meta     ("sensitive", " enablespecialoutline")

property_double (os_outhigh, _("Levels high output lighting for Outline Bevel"), 1)
    description (_("Levels high output as a light adjustment for the outline bevel"))
    ui_range    (1.0, 1.2)
    value_range    (1.0, 1.2)
  ui_steps      (0.01, 0.50)
ui_meta ("visible", "guichange {imageoutlinebevel}")
  ui_meta     ("sensitive", " enablespecialoutline")


property_double (os_dark, _("Outline Dark Bevel/ignore image mode"), 0.0)
    description (_("This instructs the outline bevel to ignore image details if there is an image file overlay below it. It also allows outline bevel to work better when the user selects darker colors."))
  value_range   (0.00, 1.00)
  ui_steps      (0.01, 0.50)
ui_meta ("visible", "guichange {imageoutlinebevel}")
  ui_meta     ("sensitive", " enablespecialoutline")
    /* This operation is extremely important for bevels with image file overlays. Normal bevel's dark bevel slider is at 0% at default. But outline bevels is at 100%.   */

    /* end of list of all GUI options */

#else
#define GEGL_OP_META
#define GEGL_OP_NAME     styles
#define GEGL_OP_C_SOURCE styles.c


#include "gegl-op.h"
#include <stdio.h>

typedef struct
{
 /* Critical nodes*/
  GeglNode *input;
  GeglNode *output;
  GeglNode *nothing1;
  GeglNode *nothing2;
  GeglNode *nothing3;
  GeglNode *nothing4;
  GeglNode *nothing5;
  GeglNode *nothing6;
  GeglNode *nothing7;
  GeglNode *repairgeglgraph;
 /* Add nodes relating to color overlay start here'*/
  GeglNode *crop;
  GeglNode *thecoloroverlay;
  GeglNode *nopcolor;
  GeglNode *opacitycolor;
  GeglNode *nocolor;
  GeglNode *coloroverlaypolicy;
  GeglNode *beforecoloroverlaypolicy;
  GeglNode *solidcolor;
  GeglNode *multiplycolor;
 /* Add nodes relating to outline and its special ability start here'*/
  GeglNode *inputso;
  GeglNode *behindso;
  GeglNode *strokeso;
  GeglNode *opacityso;
  GeglNode *blurso;
  GeglNode *moveso;
  GeglNode *nopso;
  GeglNode *colorso;
  GeglNode *bevellightingso;
 /*Outline's special ability subsect*/
  GeglNode *bevelso;
  GeglNode *atopso;
  GeglNode *layerso;
  GeglNode *invisibleblend2;
  GeglNode *replaceontop2so;
  GeglNode *idrefbevelblendmodeso;
  GeglNode *beveladdso;
  GeglNode *bevelmultiplyso;
  GeglNode *bevelblendmodeso;
  GeglNode *nopb3so;
  GeglNode *bevelalphaso;
  GeglNode *darkbeveloutline;
  GeglNode *opacityload2;
  GeglNode *idrefbevelso;
 /* Add nodes relating to shadow start here'*/
  GeglNode *ds;
 /* Add nodes relating to image file overlay start here'*/
  GeglNode *atopi;
  GeglNode *image;
  GeglNode *imageadjustments;
  GeglNode *imageadjustments2;
  GeglNode *nopimage;
 /* Add nodes relating to bevel start here'*/
  GeglNode *bevelbump;
  GeglNode *bevelblendmode;
  GeglNode *bevelalpha;
  GeglNode *nopreplaceontop;
  GeglNode *invisibleblend;
  GeglNode *replaceontop;
  GeglNode *darkbevel;
  GeglNode *beveladd;
  GeglNode *bevelmultiply;
  GeglNode *nopb;
  GeglNode *bevellighting;
  GeglNode *opacityload;
 /* Add nodes relating to innerglow start here'*/
  GeglNode *innerglow;
  GeglNode *innerglowblend;
  GeglNode *nopig;
  GeglNode *invisibleblend3;
} State;


static void attach (GeglOperation *operation)
{

  GeglNode *gegl = operation->node;
  GeglProperties *o = GEGL_PROPERTIES (operation);
  /*All nodes relating to repair gegl graph*/

  GeglNode *output = gegl_node_get_output_proxy (gegl, "output");
  GeglNode *input  = gegl_node_get_input_proxy (gegl, "input");




  GeglNode *repairgeglgraph      = gegl_node_new_child (gegl, "operation", "gegl:median-blur", "radius", 0, "abyss-policy",     GEGL_ABYSS_NONE,
                                                        NULL);

  GeglNode *nothing1 = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);


  GeglNode *nothing2 = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);


  GeglNode *nothing3 = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  GeglNode *nothing4 = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);


  GeglNode *nothing5 = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);


  GeglNode *nothing6 = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  GeglNode *nothing7 = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  /*All nodes relating to color overlay go here*/

  GeglNode *thecoloroverlay = gegl_node_new_child (gegl,
                                  "operation", "gegl:color",
                                  NULL);


  GeglNode *nopcolor = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  GeglNode *crop = gegl_node_new_child (gegl,
                                  "operation", "gegl:crop",
                                  NULL);


  GeglNode *beforecoloroverlaypolicy = gegl_node_new_child (gegl,
                                  "operation", "gegl:src-atop",
                                  NULL);


  GeglNode *coloroverlaypolicy = gegl_node_new_child (gegl,
                                  "operation", "gegl:multiply",
                                  NULL);

  /*All nodes relating to outline and its special ability go here*/

  GeglNode *bevelblendmodeso = gegl_node_new_child (gegl,
                                  "operation", "gegl:multiply",
                                  NULL);

  GeglNode *inputso   = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  GeglNode *colorso   = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-overlay",
                                  NULL);

  GeglNode *nopso   = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  GeglNode *behindso   = gegl_node_new_child (gegl,
                                  "operation", "gegl:dst-over",
                                  NULL);

  GeglNode *opacityso    = gegl_node_new_child (gegl,
                                  "operation", "gegl:opacity",
                                  NULL);

  GeglNode *strokeso      = gegl_node_new_child (gegl, "operation", "gegl:median-blur",
                                         "percentile",       100.0,
                                         "alpha-percentile", 100.0,
                                         "abyss-policy",     GEGL_ABYSS_NONE,
                                         NULL);
  GeglNode *moveso    = gegl_node_new_child (gegl,
                                  "operation", "gegl:translate",
                                  NULL);

  GeglNode *blurso      = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur",
                                         "clip-extent", FALSE,
                                         "abyss-policy", 0,
                                         NULL);

  GeglNode *atopso    = gegl_node_new_child (gegl,
                                  "operation", "gegl:src-atop",
                                  NULL);


    GeglNode *invisibleblend2 = gegl_node_new_child (gegl,
                                  "operation", "gegl:dst",
                                  NULL);

    GeglNode *replaceontop2so   = gegl_node_new_child (gegl,
                                  "operation", "gegl:src-in",
                                  NULL);

    GeglNode *idrefbevelblendmodeso    = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

    GeglNode *idrefbevelso    = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

    GeglNode *layerso    = gegl_node_new_child (gegl,
                                  "operation", "gegl:load",
                                  NULL);

    GeglNode *opacityload2    = gegl_node_new_child (gegl,
                                  "operation", "gegl:opacity",
                                  NULL); 

    GeglNode *bevelso    = gegl_node_new_child (gegl,
                                  "operation", "gegl:bevel", "type", 1, "blendmode", 0,
                                  NULL);


    GeglNode *bevellightingso = gegl_node_new_child (gegl,
                                  "operation", "gegl:levels",
                                  NULL);


    GeglNode *nopb3so = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);
#define thresholdalphaso \
" id=0 dst-out aux=[ ref=0  component-extract component=alpha   levels in-low=0.15  color-to-alpha opacity-threshold=0.4  ] "\

    GeglNode *bevelalphaso = gegl_node_new_child (gegl,
                                  "operation", "gegl:gegl", "string", thresholdalphaso,
                                  NULL);

    GeglNode *ds = gegl_node_new_child (gegl,
                                  "operation", "gegl:dropshadow",
                                  NULL);

  /*All nodes relating to Inner Glow go here*/

    GeglNode *innerglow = gegl_node_new_child (gegl,
                                  "operation", "gegl:inner-glow",
                                  NULL);

      GeglNode *nopig = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

      GeglNode *innerglowblend = gegl_node_new_child (gegl,
                                  "operation", "gegl:src-atop",
                                  NULL);

      GeglNode *invisibleblend3 = gegl_node_new_child (gegl,
                                  "operation", "gegl:dst",
                                  NULL);

  /*All nodes relating to Image file overlay go here*/

      GeglNode *atopi = gegl_node_new_child (gegl,
                                  "operation", "gegl:src-atop",
                                  NULL);

       GeglNode *image = gegl_node_new_child (gegl,
                                  "operation", "gegl:load",
                                  NULL);

       GeglNode *opacityload = gegl_node_new_child (gegl,
                                  "operation", "gegl:opacity",
                                  NULL);

      GeglNode *nopimage = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

      GeglNode *imageadjustments = gegl_node_new_child (gegl,
                                  "operation", "gegl:hue-chroma",
                                  NULL);

      GeglNode *imageadjustments2 = gegl_node_new_child (gegl,
                                  "operation", "gegl:saturation",
                                  NULL);

  /*All nodes relating to bevel go here*/

      GeglNode *bevelblendmode = gegl_node_new_child (gegl,
                                  "operation", "gegl:multiply",
                                  NULL);
#define thresholdalpha \
" id=0 dst-out aux=[ ref=0  component-extract component=alpha   levels in-low=0.15  color-to-alpha opacity-threshold=0.4  ] "\

      GeglNode *bevelalpha = gegl_node_new_child (gegl,
                                  "operation", "gegl:gegl", "string", thresholdalpha,
                                  NULL);
  /*This is a GEGL Graph that does something like Gimp's threshold alpha filter or Curves on alpha channel. It gets rid of excesses on GEGL only blend modes.*/

      GeglNode *bevelbump = gegl_node_new_child (gegl,     "operation", "gegl:bevel", "type", 1, "blendmode", 0,

                                  NULL);

      GeglNode *darkbevel = gegl_node_new_child (gegl,
                                  "operation", "gegl:levels",
                                  NULL);

      GeglNode *darkbeveloutline = gegl_node_new_child (gegl,
                                  "operation", "gegl:levels",
                                  NULL);

      GeglNode *replaceontop = gegl_node_new_child (gegl,
                                  "operation", "gegl:src-in",
                                  NULL);

      GeglNode *nopreplaceontop = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

      GeglNode *invisibleblend = gegl_node_new_child (gegl,
                                  "operation", "gegl:dst",
                                  NULL);

      GeglNode *nopb = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

      GeglNode *bevellighting      = gegl_node_new_child (gegl, "operation", "gegl:levels",
                                         NULL);
 /*Nodes relating to Color Overlay*/
  gegl_operation_meta_redirect (operation, "color_fill", thecoloroverlay, "value");
  /*Nodes relating to Image File Overlay*/
  gegl_operation_meta_redirect (operation, "image_src",   image, "path");
  gegl_operation_meta_redirect (operation, "image_opacity",   opacityload, "value");
/*  gegl_operation_meta_redirect (operation, "imagehue",   imageadjustments, "hue"); */
  gegl_operation_meta_redirect (operation, "image_saturation",   imageadjustments2, "scale");
  gegl_operation_meta_redirect (operation, "image_lightness",   imageadjustments, "lightness");
  /*Nodes relating to Drop Shadow*/
  gegl_operation_meta_redirect (operation, "shadow_x",   ds, "x");
  gegl_operation_meta_redirect (operation, "shadow_y",   ds, "y");
  gegl_operation_meta_redirect (operation, "shadow_opacity",   ds, "opacity");
  gegl_operation_meta_redirect (operation, "shadow_grow_radius",   ds, "grow-radius");
  gegl_operation_meta_redirect (operation, "shadow_radius",   ds, "radius");
  gegl_operation_meta_redirect (operation, "shadow_color",   ds, "color");
  /*Nodes relating to Outline and its special ability to bevel and image file overlay itself*/
  gegl_operation_meta_redirect (operation, "outline",   strokeso, "radius");
  gegl_operation_meta_redirect (operation, "outline_blur",   blurso, "std-dev-x");
  gegl_operation_meta_redirect (operation, "outline_blur",   blurso, "std-dev-y");
  gegl_operation_meta_redirect (operation, "outline_x",   moveso, "x");
  gegl_operation_meta_redirect (operation, "outline_y",   moveso, "y");
  gegl_operation_meta_redirect (operation, "outline_grow_shape",   strokeso, "neighborhood");
  gegl_operation_meta_redirect (operation, "outline_opacity",   opacityso, "value");
  gegl_operation_meta_redirect (operation, "outline_color",   colorso, "value");
  /*Outline's special ability subsection (named os for Outline Special)*/
  gegl_operation_meta_redirect (operation, "os_radius",   bevelso, "radius");
  gegl_operation_meta_redirect (operation, "os_elevation",   bevelso, "elevation");
  gegl_operation_meta_redirect (operation, "os_depth",   bevelso, "depth");
  gegl_operation_meta_redirect (operation, "os_azimuth",   bevelso, "azimuth");
  gegl_operation_meta_redirect (operation, "os_dark",     darkbeveloutline, "out-low");
  gegl_operation_meta_redirect (operation, "os_src",   layerso, "path");
  gegl_operation_meta_redirect (operation, "os_src_opacity",   opacityload2, "value"); 
  gegl_operation_meta_redirect (operation, "os_outlow",   bevellightingso, "out-low");
  gegl_operation_meta_redirect (operation, "os_outhigh",   bevellightingso, "out-high");
  /*Nodes relating to Inner Glow*/
  gegl_operation_meta_redirect (operation, "ig_grow_radius",   innerglow, "grow-radius");
  gegl_operation_meta_redirect (operation, "ig_radius",   innerglow, "radius");
  gegl_operation_meta_redirect (operation, "ig_opacity",   innerglow, "opacity");
  gegl_operation_meta_redirect (operation, "ig_value",   innerglow, "value");
  gegl_operation_meta_redirect (operation, "ig_treatment",   innerglow, "cover");
  /*Nodes relating to Bevel*/
  gegl_operation_meta_redirect (operation, "bevel_depth",   bevelbump, "depth");
  gegl_operation_meta_redirect (operation, "bevel_radius",   bevelbump, "radius");
  gegl_operation_meta_redirect (operation, "bevel_elevation",   bevelbump, "elevation");
  gegl_operation_meta_redirect (operation, "bevel_azimuth",   bevelbump, "azimuth");
  gegl_operation_meta_redirect (operation, "bevel_dark",   darkbevel, "out-low");
  gegl_operation_meta_redirect (operation, "bevel_outlow",   bevellighting, "out-low");
  gegl_operation_meta_redirect (operation, "bevel_type",   bevelbump, "type");
  gegl_operation_meta_redirect (operation, "bevel_outhigh",   bevellighting, "out-high");

/*These are the nodes that launch at startup but afterward the state-> nodes kick in.*/
         gegl_node_link_many (input, nopimage, atopi, nopcolor, beforecoloroverlaypolicy, crop,  nopreplaceontop, replaceontop, nopig, innerglowblend, inputso, behindso, ds,  repairgeglgraph,  output, NULL);
/* All nodes relating to Outline here
    gegl_node_link_many (inputso, strokeso, blurso, moveso, colorso, atopso, idrefbevelblendmodeso, replaceontop2so, opacityso,  NULL);
    gegl_node_connect (behindso, "aux", opacityso, "output");
    gegl_node_connect (bevelblendmodeso, "aux", nopb3so, "output");
    gegl_node_link_many (atopso, darkbeveloutline, bevelso, bevellightingso, bevelalphaso, nopb3so,  NULL);
    gegl_node_connect (atopso, "aux", layerso, "output");
    gegl_node_link_many (nopso, layerso,  NULL);
    gegl_node_connect (replaceontop2so, "aux", bevelblendmodeso, "output");
    gegl_node_link_many (idrefbevelblendmodeso, bevelblendmodeso,  NULL);
 All nodes relating to Inner Glow here
      gegl_node_link_many (nopig, innerglow, NULL);
      gegl_node_connect (innerglowblend, "aux", innerglow, "output");
 All nodes relating to image file upload here*/
   gegl_node_link_many (nopimage, /*image,*/ imageadjustments, imageadjustments2, opacityload, NULL);
      gegl_node_connect (atopi, "aux", opacityload, "output");
/* All nodes relating to bevel here
      gegl_node_link_many ( nopreplaceontop, nopb, bevelblendmode, NULL);
      gegl_node_link_many (nopb, darkbevel, bevelbump, bevellighting, bevelalpha,  NULL);
      gegl_node_connect (bevelblendmode, "aux", bevelalpha, "output");
      gegl_node_connect (replaceontop, "aux", bevelblendmode, "output"); */
 /*All nodes relating to color overlay here*/
     gegl_node_link_many (nopcolor, coloroverlaypolicy, NULL);
      gegl_node_connect (beforecoloroverlaypolicy, "aux", coloroverlaypolicy, "output");
      gegl_node_connect (coloroverlaypolicy, "aux", thecoloroverlay, "output");
      gegl_node_connect (crop, "aux", input, "output");

 /* Now save points to the various gegl nodes so we can rewire them in
   * update_graph() later
   */
  State *state = g_malloc0 (sizeof (State));
  o->user_data = state;
  state->input = input;
  state->output = output;
  state->nopimage = nopimage;
  state->atopi = atopi;
  state->crop = crop;
  state->nopreplaceontop = nopreplaceontop;
  state->replaceontop = replaceontop;
  state->nopig = nopig;
  state->innerglowblend = innerglowblend;
  state->inputso = inputso;
  state->strokeso = strokeso;
  state->behindso = behindso;
/*  state->ds = ds; */
  state->blurso = blurso;
  state->moveso = moveso;
  state->colorso = colorso;
  state->atopso = atopso;
  state->idrefbevelblendmodeso = idrefbevelblendmodeso;
  state->idrefbevelso = idrefbevelso;
  state->replaceontop2so = replaceontop2so;
  state->opacityso = opacityso;
  state->behindso = behindso;
  state->bevelblendmodeso = bevelblendmodeso;
  state->nopb3so = nopb3so;
  state->darkbeveloutline = darkbeveloutline;
  state->bevelso = bevelso;
  state->bevellightingso = bevellightingso;
  state->bevelalphaso = bevelalphaso;
  state->nopso = nopso;
  state->layerso = layerso;
  state->innerglow = innerglow;
  state->image = image;
  state->imageadjustments = imageadjustments;
  state->imageadjustments2 = imageadjustments2;
  state->nopb = nopb;
  state->darkbevel = darkbevel;
  state->bevelbump = bevelbump;
  state->bevellighting = bevellighting;
  state->bevelalpha = bevelalpha;
  state->bevelblendmode = bevelblendmode;
  state->invisibleblend = invisibleblend;
  state->invisibleblend2 = invisibleblend2;
  state->invisibleblend3 = invisibleblend3;
  state->nothing1 = nothing1;
  state->nothing2 = nothing2;
  state->nothing3 = nothing3;
  state->nothing4 = nothing4;
  state->nothing5 = nothing5;
  state->nothing6 = nothing6;
  state->nothing7 = nothing7;
  state->coloroverlaypolicy = coloroverlaypolicy;
  state->nopcolor = nopcolor;
  state->opacityload = opacityload;
  state->opacityload2 = opacityload2;
/*  state->thecoloroverlay = thecoloroverlay; */
/*   state->repairgeglgraph = repairgeglgraph; */
  state->beforecoloroverlaypolicy = beforecoloroverlaypolicy;

}

static void update_graph (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  State *state = o->user_data;
  if (!state) return;
  GeglNode *swapreplaceontop=NULL;
  GeglNode *swapreplaceontop2so=NULL;
  GeglNode *swapbevelalpha=NULL;
  GeglNode *swapbevelbump=NULL;
  GeglNode *swapdarkbevel=NULL;
  GeglNode *swapimage=NULL;
  GeglNode *swaplayerso=NULL;
/*  GeglNode *swapds; */
  GeglNode *swapbevelso=NULL;
  GeglNode *swapbevelblendmodeso=NULL;

const char *blend_bevel = "gegl:nop";
  switch (o->bevel_blend) {
    case GEGL_BLEND_MODE_TYPE_MULTIPLY_BEVEL_OUTLINE:  blend_bevel = "gegl:multiply"; break;
    case GEGL_BLEND_MODE_TYPE_ADD_BEVEL_OUTLINE:  blend_bevel = "gegl:add"; break;
    case GEGL_BLEND_MODE_TYPE_HARDLIGHT_BEVEL_OUTLINE:  blend_bevel = "gegl:hard-light"; break;
    case GEGL_BLEND_MODE_TYPE_DARKEN_BEVEL_OUTLINE: blend_bevel = "gegl:darken"; break;
    case GEGL_BLEND_MODE_TYPE_COLORDODGE_BEVEL: blend_bevel = "gegl:color-dodge"; break;
  if (o->bevel_blend == GEGL_BLEND_MODE_TYPE_COLORDODGE_BEVEL) gegl_node_set (state->bevelblendmode, "srgb", TRUE, NULL);
  }
  gegl_node_set (state->bevelblendmode, "operation", blend_bevel, NULL);

  const char *inner_glow_blend = "gegl:nop"; /* This is where inner glow's blend mode is swapped */
  switch (o->ig_blend) {
    case GEGL_BLEND_MODE_TYPE_NORMALIG: inner_glow_blend = "gegl:src-atop"; break;
    case GEGL_BLEND_MODE_TYPE_OVERLAYIG: inner_glow_blend = "gegl:overlay"; break;
    case GEGL_BLEND_MODE_TYPE_SCREENIG: inner_glow_blend = "gegl:screen"; break;
    case GEGL_BLEND_MODE_TYPE_HARDLIGHTIG: inner_glow_blend =  "gegl:hard-light"; break;
    case GEGL_BLEND_MODE_TYPE_COLORDODGEIG: inner_glow_blend = "gegl:color-dodge"; break;
    case GEGL_BLEND_MODE_TYPE_PLUSIG: inner_glow_blend = "gegl:plus"; break;
  if (o->ig_blend == GEGL_BLEND_MODE_TYPE_OVERLAYIG) gegl_node_set (state->innerglowblend, "srgb", TRUE, NULL);
  if (o->ig_blend == GEGL_BLEND_MODE_TYPE_PLUSIG) gegl_node_set (state->innerglowblend, "srgb", TRUE, NULL);

  }
  gegl_node_set (state->innerglowblend, "operation", inner_glow_blend, NULL);

  const char *blend_outline_bevel = "gegl:nop";
  switch (o->os_blend) {
    case GEGL_BLEND_MODE_TYPE_MULTIPLY_BEVEL_OUTLINE:  blend_outline_bevel = "gegl:multiply"; break;
    case GEGL_BLEND_MODE_TYPE_ADD_BEVEL_OUTLINE:  blend_outline_bevel = "gegl:add"; break;
    case GEGL_BLEND_MODE_TYPE_HARDLIGHT_BEVEL_OUTLINE:  blend_outline_bevel = "gegl:hard-light"; break;
    case GEGL_BLEND_MODE_TYPE_DARKEN_BEVEL_OUTLINE: blend_outline_bevel = "gegl:darken"; break;
    case GEGL_BLEND_MODE_TYPE_COLORDODGE_BEVEL_OUTLINE: blend_outline_bevel = "gegl:color-dodge"; break;
  if (o->os_blend == GEGL_BLEND_MODE_TYPE_COLORDODGE_BEVEL_OUTLINE) gegl_node_set (state->bevelblendmodeso, "srgb", TRUE, NULL);
  }
  gegl_node_set (state->bevelblendmodeso, "operation", blend_outline_bevel, NULL);

  const char *blend_color = "gegl:nop";
  switch (o->color_policy) {
    case GEGL_BLEND_MODE_TYPE_NO_COLOR:  blend_color = "gegl:dst"; break;
    case GEGL_BLEND_MODE_TYPE_MULTIPLY_COLOR:  blend_color = "gegl:multiply"; break;
    case GEGL_BLEND_MODE_TYPE_SOLID_COLOR:   blend_color = "gegl:src"; break;
  }
  gegl_node_set (state->coloroverlaypolicy, "operation", blend_color, NULL);

  /* This is here so in default GEGL Styles does nothing on launch. That is how I like it. src-in node does a stack and this disables that by swapping it with dst (the invisible blend mode) */

/*Nodes relating to special outline bevel's switching to the invisible blend mode (gegl:dst)
 so GEGL Styles does nothing on startup. */
  if (!o->enablespecialoutline) gegl_node_disconnect(state->replaceontop2so, "aux");
  if (!o->enablespecialoutline) swapreplaceontop2so = state->invisibleblend2;
  if (o->enablespecialoutline) swapreplaceontop2so = state->replaceontop2so;

/*Nodes relating to bevel being enabled/disabled*/

  if (!o->enablebevel) gegl_node_disconnect(state->replaceontop, "aux");
  if (o->enablebevel)  gegl_node_connect(state->bevelblendmode, "output", state->replaceontop, "aux");
  if (!o->enablebevel) swapbevelbump = state->nothing1;
  if (o->enablebevel) swapbevelbump = state->bevelbump;
  /*if (!o->enablebevel) swapreplaceontop = invisibleblend;
  if (o->enablebevel) swapreplaceontop = state->replaceontop;*/
  if (o->enablebevel) swapbevelalpha  = state->bevelalpha;
  if (!o->enablebevel) swapbevelalpha  = state->nothing2;
  if (fabs (o->bevel_dark) > 0.0001) swapdarkbevel = state->darkbevel;
  else swapdarkbevel = state->nothing3;

/*Nodes relating to image file upload being enabled disabled*/
  if (!o->enableimage) swapimage = state->nothing4;
  if (o->enableimage) swapimage = state->image;

/*Nodes relating to outline image file upload being enabled disabled*/
  if (!o->enableimageoutline) swaplayerso = state->nothing5;
  if (o->enableimageoutline) swaplayerso = state->layerso;

/*Nodes relating to dropshadow being enabled/disabled

For some odd reason this was slowing things down. So it got the boot.
  if (fabs (o->shadowopacity) > 0.0001) swapds = state->ds;
  else swapds = state->nothing6;*/

/*Nodes relating to outline being enabled/disabled*/
  if (!o->enableoutline) gegl_node_disconnect(state->behindso, "aux");

/*Nodes relating to inner glow being enabled/disabled*/
  if (!o->enableinnerglow) gegl_node_disconnect(state->innerglowblend, "aux");

/*Nodes relating to outline bevel being enabled/disabled*/
  if (o->enableoutlinebevel) swapbevelso = state->bevelso;
  if (!o->enableoutlinebevel) swapbevelso = state->nothing7;
  if (o->enableoutlinebevel) swapbevelblendmodeso = state->bevelblendmodeso;
  if (!o->enableoutlinebevel) swapbevelblendmodeso = state->invisibleblend3;

/*to fix a bug with replace on top and prevent on a trivial warning and blend bug.
If this is removed bevel's blend mode will run in default and blend the composition.
I choose enable outline only because I had to choose one checkbox. */
  if (o->enableoutline) swapreplaceontop = NULL;

  if (o->enableinnerglow)
  if (o->enableoutline)
  if (o->enablespecialoutline)

 {                                                   /*Six GEGL graphs exist here and I did test with one GEGL Graph with more if (0->enable)'s and it was a ton slower.*/
  /* INNERGLOW + SPECIAL OUTLINE */

         gegl_node_link_many (state->input,  state->atopi, state->nopcolor, state->beforecoloroverlaypolicy, state->crop,  state->nopreplaceontop, swapreplaceontop,  state->nopig, state->innerglowblend, state->inputso, state->behindso, /*state->ds state->repairgeglgraph,*/ state->output, NULL);
/* All nodes relating to Outline here*/
    gegl_node_link_many (state->inputso, state->strokeso, state->blurso, state->moveso, state->colorso,  state->idrefbevelso,  state->atopso, state->idrefbevelblendmodeso, swapreplaceontop2so, state->opacityso,  NULL);
    gegl_node_connect (state->behindso, "aux", state->opacityso, "output");
    gegl_node_connect (swapbevelblendmodeso, "aux", state->nopb3so, "output");
    gegl_node_link_many (state->idrefbevelso, state->darkbeveloutline, swapbevelso, state->bevellightingso, state->bevelalphaso, state->nopb3so,  NULL);
    gegl_node_connect (state->atopso, "aux", state->opacityload2, "output");
    gegl_node_link_many (swaplayerso, state->opacityload2,  NULL);
    gegl_node_connect (swapreplaceontop2so, "aux", swapbevelblendmodeso, "output");
    gegl_node_link_many (state->idrefbevelblendmodeso, swapbevelblendmodeso,  NULL);
/* All nodes relating to Inner Glow here*/
      gegl_node_link_many (state->nopig, state->innerglow, NULL);
      gegl_node_connect (state->innerglowblend, "aux", state->innerglow, "output");
/* All nodes relating to image file upload here*/
      gegl_node_link_many (swapimage, state->imageadjustments, state->imageadjustments2, state->opacityload, NULL);
      gegl_node_connect (state->atopi, "aux", state->opacityload, "output");
/* All nodes relating to bevel here*/
      gegl_node_link_many ( state->nopreplaceontop, state->nopb, state->bevelblendmode, NULL);
      gegl_node_link_many (state->nopb, swapdarkbevel, swapbevelbump, state->bevellighting,  swapbevelalpha,  NULL);
      gegl_node_connect (state->bevelblendmode, "aux", swapbevelalpha, "output");
      gegl_node_connect (swapreplaceontop, "aux", state->bevelblendmode, "output");
 /*All nodes relating to color overlay here */
      gegl_node_link_many (state->nopcolor, state->coloroverlaypolicy, NULL);
      gegl_node_connect (state->beforecoloroverlaypolicy, "aux", state->coloroverlaypolicy, "output");
/*      gegl_node_connect (state->coloroverlaypolicy, "aux", state->thecoloroverlay, "output"); */
      gegl_node_connect (state->crop, "aux", state->input, "output");
  }
  else
  {

  /* INNERGLOW + NORMAL OUTLINE */
         gegl_node_link_many (state->input,  state->atopi, state->nopcolor, state->beforecoloroverlaypolicy, state->crop,  state->nopreplaceontop, swapreplaceontop,  state->nopig, state->innerglowblend, state->inputso, state->behindso, /*state->ds state->repairgeglgraph,*/ state->output, NULL);
/* All nodes relating to Outline here*/
    gegl_node_link_many (state->inputso, state->strokeso, state->blurso, state->moveso, state->colorso, state->opacityso, NULL);
     gegl_node_connect (state->behindso, "aux", state->opacityso, "output");
/* All nodes relating to Inner Glow here*/
      gegl_node_link_many (state->nopig, state->innerglow, NULL);
      gegl_node_connect (state->innerglowblend, "aux", state->innerglow, "output");
/* All nodes relating to image file upload here*/
      gegl_node_link_many (swapimage, state->imageadjustments, state->imageadjustments2, state->opacityload, NULL);
      gegl_node_connect (state->atopi, "aux", state->opacityload, "output");
/* All nodes relating to bevel here*/
      gegl_node_link_many ( state->nopreplaceontop, state->nopb, state->bevelblendmode, NULL);
      gegl_node_link_many (state->nopb, swapdarkbevel, swapbevelbump, state->bevellighting, swapbevelalpha,  NULL);
      gegl_node_connect (state->bevelblendmode, "aux", swapbevelalpha, "output");
      gegl_node_connect (swapreplaceontop, "aux", state->bevelblendmode, "output");
/* All nodes relating to color overlay here*/
      gegl_node_link_many (state->nopcolor, state->coloroverlaypolicy, NULL);
      gegl_node_connect (state->beforecoloroverlaypolicy, "aux", state->coloroverlaypolicy, "output");
/*      gegl_node_connect (state->coloroverlaypolicy, "aux", state->thecoloroverlay, "output"); */
      gegl_node_connect (state->crop, "aux", state->input, "output");
  }
  else
  {
 /*NO OUTLINE INNER GLOW */
         gegl_node_link_many (state->input, state->atopi, state->nopcolor, state->beforecoloroverlaypolicy, state->crop, state->nopreplaceontop, swapreplaceontop,  state->nopig, state->innerglowblend,  /*state->ds state->repairgeglgraph, */ state->output, NULL);
/* All nodes relating to Inner Glow here*/
      gegl_node_link_many (state->nopig, state->innerglow, NULL);
      gegl_node_connect (state->innerglowblend, "aux", state->innerglow, "output");
/* All nodes relating to image file upload here*/
      gegl_node_link_many (swapimage, state->imageadjustments, state->opacityload, NULL);
      gegl_node_connect (state->atopi, "aux", state->opacityload, "output");
/* All nodes relating to bevel here*/
      gegl_node_link_many ( state->nopreplaceontop, state->nopb, state->bevelblendmode, NULL);
      gegl_node_link_many (state->nopb, swapdarkbevel, swapbevelbump, state->bevellighting, swapbevelalpha,  NULL);
      gegl_node_connect (state->bevelblendmode, "aux", swapbevelalpha, "output");
      gegl_node_connect (swapreplaceontop, "aux", state->bevelblendmode, "output");
/* All nodes relating to color overlay here*/
      gegl_node_link_many (state->nopcolor, state->coloroverlaypolicy, NULL);
      gegl_node_connect (state->beforecoloroverlaypolicy, "aux", state->coloroverlaypolicy, "output");
/*      gegl_node_connect (state->coloroverlaypolicy, "aux", state->thecoloroverlay, "output"); */
      gegl_node_connect (state->crop, "aux", state->input, "output");
    }

else
 /*SPECIAL OUTLINE*/
  if (o->enableoutline)
  if (o->enablespecialoutline)
  {
         gegl_node_link_many (state->input, state->atopi, state->nopcolor, state->beforecoloroverlaypolicy, state->crop, state->nopreplaceontop, swapreplaceontop, state->inputso, state->behindso, /*state->ds, state->repairgeglgraph, */ state->output, NULL);
/* All nodes relating to Outline here*/
    gegl_node_link_many (state->inputso, state->strokeso, state->blurso, state->moveso, state->colorso, state->idrefbevelso, state->atopso,  state->idrefbevelblendmodeso, swapreplaceontop2so, state->opacityso,  NULL);
    gegl_node_connect (state->behindso, "aux", state->opacityso, "output");
    gegl_node_connect (swapbevelblendmodeso, "aux", state->nopb3so, "output");
    gegl_node_link_many (state->idrefbevelso, state->darkbeveloutline, swapbevelso, state->bevellightingso,  state->bevelalphaso, state->nopb3so,  NULL);
    gegl_node_connect (state->atopso, "aux", state->opacityload2, "output");
    gegl_node_link_many (swaplayerso, state->opacityload2,  NULL);
    gegl_node_connect (swapreplaceontop2so, "aux", swapbevelblendmodeso, "output");
    gegl_node_link_many (state->idrefbevelblendmodeso, swapbevelblendmodeso,  NULL);
/* All nodes relating to image file upload here*/
      gegl_node_link_many (swapimage, state->imageadjustments, state->imageadjustments2, state->opacityload, NULL);
      gegl_node_connect (state->atopi, "aux", state->opacityload, "output");
/* All nodes relating to bevel here*/
      gegl_node_link_many ( state->nopreplaceontop, state->nopb, state->bevelblendmode, NULL);
      gegl_node_link_many (state->nopb, swapdarkbevel, swapbevelbump, state->bevellighting,  swapbevelalpha,  NULL);
      gegl_node_connect (state->bevelblendmode, "aux", swapbevelalpha, "output");
      gegl_node_connect (swapreplaceontop, "aux", state->bevelblendmode, "output");
/* All nodes relating to color overlay here*/
      gegl_node_link_many (state->nopcolor, state->coloroverlaypolicy, NULL);
      gegl_node_connect (state->beforecoloroverlaypolicy, "aux", state->coloroverlaypolicy, "output");
/*      gegl_node_connect (state->coloroverlaypolicy, "aux", state->thecoloroverlay, "output"); */
      gegl_node_connect (state->crop, "aux", state->input, "output");

  }
  else
  {
 /*NORMAL OUTLINE*/
         gegl_node_link_many (state->input, state->atopi, state->nopcolor, state->beforecoloroverlaypolicy, state->crop, state->nopreplaceontop, swapreplaceontop, state->inputso, state->behindso, /*state->ds state->repairgeglgraph,*/ state->output, NULL);
/* All nodes relating to Outline here*/
    gegl_node_link_many (state->inputso, state->strokeso, state->blurso, state->moveso, state->colorso, state->opacityso, NULL);
     gegl_node_connect (state->behindso, "aux", state->opacityso, "output");
/* All nodes relating to image file upload here*/
      gegl_node_link_many (swapimage, state->imageadjustments, state->imageadjustments2, state->opacityload, NULL);
      gegl_node_connect (state->atopi, "aux", state->opacityload, "output");
/* All nodes relating to bevel here*/
      gegl_node_link_many ( state->nopreplaceontop, state->nopb, state->bevelblendmode, NULL);
      gegl_node_link_many (state->nopb, swapdarkbevel, swapbevelbump, state->bevellighting,  swapbevelalpha,  NULL);
      gegl_node_connect (state->bevelblendmode, "aux", swapbevelalpha, "output");
      gegl_node_connect (swapreplaceontop, "aux", state->bevelblendmode, "output");
/* All nodes relating to color overlay here*/
      gegl_node_link_many (state->nopcolor, state->coloroverlaypolicy, NULL);
      gegl_node_connect (state->beforecoloroverlaypolicy, "aux", state->coloroverlaypolicy, "output");
/*      gegl_node_connect (state->coloroverlaypolicy, "aux", state->thecoloroverlay, "output"); */
      gegl_node_connect (state->crop, "aux", state->input, "output");
  }
  else
  {
 /*NO OUTLINE*/
       gegl_node_link_many (state->input, state->atopi, state->nopcolor, state->beforecoloroverlaypolicy, state->crop, state->nopreplaceontop, swapreplaceontop, /*state->ds, state->repairgeglgraph,*/ state->output, NULL);
/* All nodes relating to image file upload here*/
      gegl_node_link_many (swapimage, state->imageadjustments, state->imageadjustments2, state->opacityload, NULL);
      gegl_node_connect (state->atopi, "aux", state->opacityload, "output");
/* All nodes relating to bevel here*/
      gegl_node_link_many ( state->nopreplaceontop, state->nopb, state->bevelblendmode, NULL);
      gegl_node_link_many (state->nopb, swapdarkbevel, swapbevelbump, state->bevellighting,  swapbevelalpha,  NULL);
      gegl_node_connect (state->bevelblendmode, "aux", swapbevelalpha, "output");
      gegl_node_connect (swapreplaceontop, "aux", state->bevelblendmode, "output");
/* All nodes relating to color overlay here*/
      gegl_node_link_many (state->nopcolor, state->coloroverlaypolicy, NULL);
      gegl_node_connect (state->beforecoloroverlaypolicy, "aux", state->coloroverlaypolicy, "output");
/*      gegl_node_connect (state->coloroverlaypolicy, "aux", state->thecoloroverlay, "output"); */
      gegl_node_connect (state->crop, "aux", state->input, "output");
  }
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
    "name",        "gegl:styles",
    "title",       _("GEGL Styles"),
    "categories",  "Generic",
    "reference-hash", "ffb9e86edb25bc92e8d4e68f59bbb04b",
    "description", _("An engine to style text and add popular effects to alpha channel images. Effects include outline, bevel, shadow, and inner glow all in one place."
                     ""),
    "gimp:menu-path", "<Image>/Filters/Generic/",
    "gimp:menu-label", _("Text Styling..."),
    NULL);
}

#endif
