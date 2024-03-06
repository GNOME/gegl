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
 *2022 Beaver GEGL Effects  *2023 beaver GEGL Styles a special fork of GEGL Effects intended for Gimp 2.99/3
 *2022 BarefootLiam (for helping give Inner Glow a disable checkbox) 



 If it was not for me recreating GEGL Drop Shadow like graph from scratch in the "All nodes relating to Outline here" this filter would have been watered down.

I hope Gimp's team is not overwhelmed by my complexity because this is the first GEGL operation to use 55+ or more nodes when everything is enabled


 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES
/* This is an ENUM list that divides GEGL Styles into three parts*/
enum_start (partoffiltertobedisplayed)
enum_value   (GEGL_UI_STROKESHADOW, "strokeshadow", N_IGNORED("Color, Outline and Shadow"))
enum_value   (GEGL_UI_INNERGLOWBEVEL, "innerglowbevel", N_IGNORED("Bevel and Inner Glow"))
enum_value   (GEGL_UI_IMAGEOUTLINEBEVEL, "imageoutlinebevel", N_IGNORED("Image file upload and Outline Bevel"))
  enum_end (endoflist)

    /* List of all GUI options */

    /* GUI Change list in GUI */

property_enum(guichange, _IGNORED("Part of filter to be displayed"),
    endoflist, partoffiltertobedisplayed,
    GEGL_UI_STROKESHADOW)
  description(_IGNORED("Display a different part of the GUI"))

    /* Color Overlays GUI options begin here */

property_color  (optioncolor, _IGNORED("Color Overlay"), "#ffffff")
  ui_meta ("visible", "guichange {strokeshadow}")
    description (_IGNORED("In default the color overlay is set to the multiply blend mode. Which means if your text/shape color is white it can become any color you select and white color fill is registered as transparent. When using multiply color overlay, images with colors other then white will cause undesired effects as if you applied a multiply color overlay on them. Solid color mode will change the color of the alpha channel defined image regardless of what its original color was."))

property_enum (policycolor, _IGNORED("Color Policy:"),
    description (_IGNORED("Change the blend mode of Color Overlay from Multiply to Solid Color. Multiply only works on white/textshapes and Solid Color will work on anything. No Color means there is no color overlay. "))
    GeglBlendColorOverlay, gegl_blend_color_overlay,
    GEGL_BLEND_MODE_TYPE_MULTIPLY_COLOR)
  ui_meta ("visible", "guichange {strokeshadow}")

enum_start (gegl_blend_color_overlay)
  enum_value (GEGL_BLEND_MODE_TYPE_NO_COLOR,      "nocolor",
              N_IGNORED("No Color"))
  enum_value (GEGL_BLEND_MODE_TYPE_MULTIPLY_COLOR,      "multiply",
              N_IGNORED("Multiply (white shapes are any color and white color fill is transparency)"))
  enum_value (GEGL_BLEND_MODE_TYPE_SOLID_COLOR,      "solidcolor",
              N_IGNORED("Solid Color"))
enum_end (GeglBlendColorOverlay)


    /* Outline's normal GUI options begin here */

property_boolean (enableoutline, _IGNORED("Enable Outline"), FALSE)
  description    (_IGNORED("Disable or Enable Outline"))
  ui_meta ("visible", "guichange {strokeshadow}")

property_double (outlineopacity, _IGNORED("Outline Opacity"), 1)
  value_range   (0.0, 1.0)
  ui_steps      (0.01, 0.10)
  description    (_IGNORED("Opacity of the outline"))
  ui_meta ("visible", "guichange {strokeshadow}")

property_double (outlinex, _IGNORED("Outline X"), 0.0)
  description   (_IGNORED("Horizontal outline offset"))
  ui_range      (-15.0, 15.0)
  ui_steps      (1, 10)
  ui_meta       ("axis", "x")
  ui_meta ("visible", "guichange {strokeshadow}")

property_double (outliney, _IGNORED("Outline Y"), 0.0)
  description   (_IGNORED("Vertical outline offset"))
  ui_range      (-15.0, 15.0)
  ui_steps      (1, 10)
  ui_meta       ("axis", "y")
  ui_meta ("visible", "guichange {strokeshadow}")

/* Should correspond to GeglMedianBlurNeighborhood in median-blur.c */
enum_start (gegl_effects_grow_shapes)
  enum_value (GEGL_stroke_GROW_SHAPE_SQUARE,  "square",  N_IGNORED("Square"))
  enum_value (GEGL_stroke_GROW_SHAPE_CIRCLE,  "circle",  N_IGNORED("Circle"))
  enum_value (GEGL_stroke_GROW_SHAPE_DIAMOND, "diamond", N_IGNORED("Diamond"))
enum_end (shapegrowend)

property_enum   (outlinegrowshape, _IGNORED("Outline Grow shape"),
                 shapegrowend, gegl_effects_grow_shapes,
                 GEGL_stroke_GROW_SHAPE_CIRCLE)
  description   (_IGNORED("The shape to expand or contract the stroke in"))
  ui_meta ("visible", "guichange {strokeshadow}")

property_double (outlineblur, _IGNORED("Outline Blur radius"), 0.0)
  value_range   (0.0, 3)
  ui_range      (0.0, 3.0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  description    (_IGNORED("Apply a mild blur on the outline"))
  ui_meta ("visible", "guichange {strokeshadow}")

property_double (outline, _IGNORED("Outline Grow radius"), 12.0)
  value_range   (0.0, G_MAXDOUBLE)
  ui_range      (0, 100.0)
  ui_digits     (0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  description (_IGNORED("The distance to expand the outline"))
  ui_meta ("visible", "guichange {strokeshadow}")

    /* Shadow's GUI options begin here */
property_color (outlinecolor, _IGNORED("Ouline Color"), "#ffffff")
    ui_meta     ("role", "color-primary")
  description    (_IGNORED("Color of the outline"))
  ui_meta ("visible", "guichange {strokeshadow}")

property_double (shadowopacity, _IGNORED("Shadow/Glow Opacity --ENABLE SHADOW/GLOW"), 0.0)
  value_range   (0.0, 1.0)
  ui_range      (0.0, 1.0)
  ui_steps      (0.01, 0.10)
  description    (_IGNORED("Enable or disable the shadow/glow"))
  ui_meta ("visible", "guichange {strokeshadow}")

property_double (shadowx, _IGNORED("Shadow/Glow X"), 10.0)
  description   (_IGNORED("Horizontal shadow offset"))
  ui_range      (-40.0, 40.0)
  ui_steps      (1, 10)
  ui_meta       ("unit", "pixel-distance")
  ui_meta       ("axis", "x")
  description    (_IGNORED("Horizontal axis of the Shadow Glow"))
  ui_meta ("visible", "guichange {strokeshadow}")

property_double (shadowy, _IGNORED("Shadow/Glow Y"), 10.0)
  description   (_IGNORED("Vertical shadow offset"))
  ui_range      (-40.0, 40.0)
  ui_steps      (1, 10)
  ui_meta       ("unit", "pixel-distance")
  ui_meta       ("axis", "y")
  description    (_IGNORED("Vertical axis of the Shadow Glow"))
  ui_meta ("visible", "guichange {strokeshadow}")

property_color  (shadowcolor, _IGNORED("Shadow/Glow Color"), "black")
    /* TRANSLATORS: the string 'black' should not be translated */
  description   (_IGNORED("The shadow's color (defaults to 'black')"))
  ui_meta ("visible", "guichange {strokeshadow}")

property_double (shadowgrowradius, _IGNORED("Shadow/Glow Grow radius"), 0.0)
  value_range   (0.0, 100.0)
  ui_range      (0.0, 50.0)
  ui_digits     (0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  description (_IGNORED("The distance to expand the shadow before blurring."))
  ui_meta ("visible", "guichange {strokeshadow}")

property_double (shadowradius, _IGNORED("Shadow/Glow Blur radius"), 12.0)
  value_range   (0.0, G_MAXDOUBLE)
  ui_range      (0.0, 110.0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  description    (_IGNORED("Blur control of the shadow"))
  ui_meta ("visible", "guichange {strokeshadow}")

    /* Bevel's GUI options begin here */

    /* This is the Bevel's ENUM List listed in the GUI */

/* This is the ENUM list for bevel's emboss blend modes */
enum_start (gegl_blend_mode_for_bevel)
  enum_value (GEGL_BLEND_MODE_TYPE_BEVELOFF_BEVEL,      "beveloff",
              N_IGNORED("Bevel Off"))
  enum_value (GEGL_BLEND_MODE_TYPE_MULTIPLY_BEVEL,      "multiply",
              N_IGNORED("Multiply"))
  enum_value (GEGL_BLEND_MODE_TYPE_ADD_BEVEL,      "add",
              N_IGNORED("Add"))
  enum_value (GEGL_BLEND_MODE_TYPE_HARDLIGHT_BEVEL,      "hardlight",
              N_IGNORED("Hard Light"))
  enum_value (GEGL_BLEND_MODE_TYPE_DARKEN_BEVEL,      "darken",
              N_IGNORED("Darken"))
enum_end (GeglBlendModeForBevel)

property_enum (bevelblend, _IGNORED("Bevel blend mode and on/off switch:"),
    description (_IGNORED("This is both the bevel blend mode switcher and option to enable and disable the bevel."))
    GeglBlendModeForBevel, gegl_blend_mode_for_bevel,
    GEGL_BLEND_MODE_TYPE_BEVELOFF_BEVEL)
  ui_meta ("visible", "guichange {innerglowbevel}")

enum_start (gbevel_listing)
  enum_value (GEGL_BEVEL_CHAMFER,      "chamferbevel",
              N_IGNORED("Chamfer Bevel"))
  enum_value (GEGL_BEVEL_BUMP,      "bumpbevel",
              N_IGNORED("Bump Bevel"))
enum_end (gbevellisting)

property_enum (beveltype, _IGNORED("Select Bevel"),
    gbevellisting, gbevel_listing,
    GEGL_BEVEL_BUMP)
    description (_IGNORED("Bump Bevel is default followed by Chamfer. "))
  ui_meta ("visible", "guichange {innerglowbevel}")


property_int (beveldepth, _IGNORED("Bevel Depth"), 100)
    description (_IGNORED("Brings out depth and detail of the bevel"))
    value_range (1, 100)
  ui_meta ("visible", "guichange {innerglowbevel}")

property_double (bevelelevation, _IGNORED("Bevel Elevation"), 55.0)
    description (_IGNORED("Bevel Elevation angle (degrees). This appears to rotate the brightest pixels."))
    value_range (55, 125)
    ui_meta ("unit", "degree")
  ui_meta ("visible", "guichange {innerglowbevel}")

property_double (bevelazimuth, _IGNORED("Bevel Azimuth"), 75.0)
    description (_IGNORED("The bevel's light angle"))
    value_range (0.0, 360.0)
  ui_steps      (0.01, 0.50)
  ui_meta ("visible", "guichange {innerglowbevel}")

property_double (bevelradius, _IGNORED("Bump Bevel Radius"), 6.0)
  value_range (1.0, 12.0)
  ui_range (1.0, 12)
  ui_gamma (1.5)
  description    (_IGNORED("Internal Gaussian Blur to 'blow up' the bump bevel. This option does not work on chamfer."))
  ui_meta ("visible", "guichange {innerglowbevel}")


property_double (beveladd, _IGNORED("Bevel Light Adjustment 1"), 0.0)
    description (_IGNORED("GEGL operation 'add' is being used as a light adjustment for the bevel"))
    ui_range    (0.0, 0.2)
    value_range    (0.0, 0.2)
  ui_steps      (0.01, 0.50)
  ui_meta ("visible", "guichange {innerglowbevel}")

property_double (bevelmultiply, _IGNORED("Bevel Light Adjustment 2"), 1.0)
    description (_IGNORED("GEGL operation 'multiply' is being used as a light adjustment for the bevel"))
    ui_range    (1.0, 1.2)
    value_range    (1.0, 1.2)
  ui_steps      (0.01, 0.50)
  ui_meta ("visible", "guichange {innerglowbevel}")

property_double (beveldark, _IGNORED("Bump Dark Bevel/Bevel ignore image mode."), 0.00)
    description (_IGNORED("This instructs the bevel effect to ignore image details if there is an image file overlay below it, it also allows bevels to work better when users select darker colors. It doesn't work well on bump bevel but does work well on chamfer bevel."))
  value_range   (0.00, 1.0)
  ui_steps      (0.01, 0.50)
  ui_meta ("visible", "guichange {innerglowbevel}")
    /* This operation is extremely important for bevels with image file overlays */

    /* Inner Glow GUI options begin here */

property_boolean (enableinnerglow, _IGNORED("Enable Inner Glow"), FALSE)
  description   (_IGNORED("Whether to add an inner glow effect, which can be slow"))
  ui_meta ("visible", "guichange {innerglowbevel}")

    /* This is the Inner Glow's ENUM List  */

/* This is the ENUM list for inner glow's blend modes
ALT means their srgb=true is true  */
enum_start (gegl_blend_mode_type_innerglowblend)
  enum_value (GEGL_BLEND_MODE_TYPE_NORMALIG,      "normal",
              N_IGNORED("Normal"))
  enum_value (GEGL_BLEND_MODE_TYPE_OVERLAYIG,      "overlay",
              N_IGNORED("Overlay"))
  enum_value (GEGL_BLEND_MODE_TYPE_SCREENIG,      "screen",
              N_IGNORED("Screen"))
  enum_value (GEGL_BLEND_MODE_TYPE_HARDLIGHTIG,      "hardlight",
              N_IGNORED("Hardlight"))
  enum_value (GEGL_BLEND_MODE_TYPE_COLORDODGEIG,      "colordodge",
              N_IGNORED("Color Dodge"))
  enum_value (GEGL_BLEND_MODE_TYPE_PLUSIG,      "plus",
              N_IGNORED("Plus"))
enum_end (GeglBlendModeTypeigblend)

property_enum (innergblend, _IGNORED("Blend Mode of Inner Glow:"),
    GeglBlendModeTypeigblend, gegl_blend_mode_type_innerglowblend,
    GEGL_BLEND_MODE_TYPE_NORMALIG)
  ui_meta ("visible", "guichange {innerglowbevel}")

property_double (innergradius, _IGNORED("Inner Glow's Blur radius"), 6.0)
  value_range   (0.0, 30.0)
  ui_range      (0.0, 30.0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  description (_IGNORED("Blur control of the inner glow"))
  ui_meta ("visible", "guichange {innerglowbevel}")

property_double (innerggrowradius, _IGNORED("Inner Glow's Grow radius"), 5)
  value_range   (1, 30.0)
  ui_range      (1, 30.0)
  ui_digits     (0)
  ui_steps      (1, 5)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")
  description (_IGNORED("The distance to expand the innerglow before blurring"))
  ui_meta ("visible", "guichange {innerglowbevel}")

property_double (innergopacity, _IGNORED("Inner Glow's opacity"), 1.00)
  value_range   (0.0, 1.00)
  ui_steps      (0.01, 0.10)
  description (_IGNORED("Opacity of the inner glow"))
  ui_meta ("visible", "guichange {innerglowbevel}")

property_color (innergvalue, _IGNORED("Inner Glow's Color"), "#ff8f00")
    description (_IGNORED("The color of the innerglow"))
  ui_meta ("visible", "guichange {innerglowbevel}")

property_double  (innergtreatment, _IGNORED("Inner Glow's unmodified pixel fix"), 75)
  value_range (50, 85)
  description (_IGNORED("Median Blur Alpha percentile helps Inner Glow reach pixels in corners. The higher the value the more likely pixels in tight corners will be covered. But on lower values this is useful as it does not clip with the blend mode. For blend modes other then normal this should be low. On default (normal) make it high. "))
  ui_meta ("visible", "guichange {innerglowbevel}")
 /* Without this Inner Glow would miss many pixels around alpha defined shapes edges'*/

    /* Image file overlay GUI options begin here */

property_file_path(imagesrc, _IGNORED("Image file overlay upload "), "")
  description (_IGNORED("Upload an image with a file path from your computer to be in the fill area (png, jpg, raw, svg, bmp, tif, ...)"))
ui_meta ("visible", "guichange {imageoutlinebevel}")

property_double(imageopacity, _IGNORED("Opacity of Image File Overlay"), 1.0)
    value_range (0.0, 1.0)
ui_meta ("visible", "guichange {imageoutlinebevel}")
  description (_IGNORED("Opacity of the image file overlay that the user uploaded. This can be used to disable image file overlays quickly"))


property_double (imagehue, _IGNORED("Hue rotation"),  0.0)
   description  (_IGNORED("Hue adjustment"))
   value_range  (-180.0, 180.0)
  description (_IGNORED("Hue rotation of the uploaded image file or whatever is on canvas"))
ui_meta ("visible", "guichange {imageoutlinebevel}")

property_double (imagesaturation, _IGNORED("Saturation"), 1.0)
   description  (_IGNORED("Saturation adjustment of the uploaded image file or whatever is on canvas"))
   value_range  (0.0, 3.0)
ui_meta ("visible", "guichange {imageoutlinebevel}")

property_double (imagelightness, _IGNORED("Lightness"), 0.0)
   description  (_IGNORED("Lightness adjustment of the uploaded image file or whatever is on canvas"))
   value_range  (-20.0, 20.0)
ui_meta ("visible", "guichange {imageoutlinebevel}")

    /* Outline special options in GUI begin here */
property_boolean (enablespecialoutline, _IGNORED("Enable effects on Outline"), FALSE)
  description    (_IGNORED("Turn on special outline abilities"))
ui_meta ("visible", "guichange {imageoutlinebevel}")

/* This is the ENUM list for bevel outline's emboss blend modes */
enum_start (gegl_blend_mode_for_bevel_outline)
  enum_value (GEGL_BLEND_MODE_TYPE_BEVELOFF_OUTLINE,      "beveloff",
              N_IGNORED("Bevel Off"))
  enum_value (GEGL_BLEND_MODE_TYPE_MULTIPLY_BEVEL_OUTLINE,      "multiply",
              N_IGNORED("Multiply"))
  enum_value (GEGL_BLEND_MODE_TYPE_ADD_BEVEL_OUTLINE,      "add",
              N_IGNORED("Add"))
  enum_value (GEGL_BLEND_MODE_TYPE_HARDLIGHT_BEVEL_OUTLINE,      "hardlight",
              N_IGNORED("Hard Light"))
  enum_value (GEGL_BLEND_MODE_TYPE_DARKEN_BEVEL_OUTLINE,      "darken",
              N_IGNORED("Darken"))
enum_end (GeglBlendModeForBevelOutline)

property_enum (osblend, _IGNORED("Outline Bevel blend mode and on/off switch:"),
    description (_IGNORED("This is both the outline bevel blend mode switcher and option to enable and disable the outline bevel."))
    GeglBlendModeForBevelOutline, gegl_blend_mode_for_bevel_outline,
    GEGL_BLEND_MODE_TYPE_MULTIPLY_BEVEL_OUTLINE)
ui_meta ("visible", "guichange {imageoutlinebevel}")


property_int (osdepth, _IGNORED("Outline Bevel Depth"), 15)
    description (_IGNORED("Bring out depth and detail of bevel outline."))
    value_range (1, 100)
ui_meta ("visible", "guichange {imageoutlinebevel}")

property_double (oselevation, _IGNORED("Outline Bevel Elevation"), 47.0)
    description (_IGNORED("Rotate the brightest pixels on the bevel outline. 90 seems to work best on the 'multiply' blend mode but the others look better at lower values."))
    value_range (0, 180)
    ui_meta ("unit", "degree")
ui_meta ("visible", "guichange {imageoutlinebevel}")

property_double (osradius, _IGNORED("Outline Bevel Radius"), 3.0)
    description (_IGNORED("Internal Gaussian Blur to blow up the outline bevel"))
  value_range (1.0, 30.0)
  ui_range (1.0, 12)
  ui_gamma (1.5)
ui_meta ("visible", "guichange {imageoutlinebevel}")

property_double (osazimuth, _IGNORED("Outline Bevel Azimuth"), 55.0)
    description (_IGNORED("Outline Bevel's Light angle"))
    value_range (0.0, 360.0)
    ui_steps      (0.01, 0.50)
ui_meta ("visible", "guichange {imageoutlinebevel}")

property_file_path(ossrc, _IGNORED("Outline Image file overlay"), "")
  description (_IGNORED("Upload an image with a file path from your computer to be in the outline area (png, jpg, raw, svg, bmp, tif, ...)"))
ui_meta ("visible", "guichange {imageoutlinebevel}")

property_double (ossrcopacity, _IGNORED("Outline Image Opacity"), 1.0)
   description  (_IGNORED("Outline image opacity adjustment"))
   value_range  (0.0, 1.0)
ui_meta ("visible", "guichange {imageoutlinebevel}")

property_double (osadd, _IGNORED("Outline Bevel Light Adjustment 1"), 0.0)
    description (_IGNORED("GEGL exclusive blend mode 'add' is being used as a light adjustment for the bevel"))
    ui_range    (0.0, 0.2)
    value_range    (0.0, 0.2)
  ui_steps      (0.01, 0.50)
ui_meta ("visible", "guichange {imageoutlinebevel}")

property_double (osmultiply, _IGNORED("Outline Bevel Light Adjustment 2"), 1)
    description (_IGNORED("GEGL exclusive blend mode 'Multiply' is being used as a light adjustment for the bevel"))
    ui_range    (1.0, 1.2)
    value_range    (1.0, 1.2)
  ui_steps      (0.01, 0.50)
ui_meta ("visible", "guichange {imageoutlinebevel}")


property_double (osdark, _IGNORED("Outline Dark Bevel/Bevel ignore image mode."), 0.0)
    description (_IGNORED("This instructs the bevel effect to ignore image details if there is an image file overlay below it, it also allows bevels to work better when users select darker colors."))
  value_range   (0.00, 1.00)
  ui_steps      (0.01, 0.50)
ui_meta ("visible", "guichange {imageoutlinebevel}")
    /* This operation is extremely important for bevels with image file overlays. Normal bevel's dark bevel slider is at 0% at default. But outline bevels is at 100%.   */

    /* end of list of all GUI options */

#else
#define _IGNORED(str)   (str)
#define N_IGNORED(str)  (str)
#define GEGL_OP_META
#define GEGL_OP_NAME     styles
#define GEGL_OP_C_SOURCE styles.c

#include "gegl-op.h"

 /*Beginning of GEGL node list one FIRST NODE LIST'*/

typedef struct
{
 /* Critical nodes*/
  GeglNode *input;
  GeglNode *output;
  GeglNode *nothing;
  GeglNode *repairgeglgraph;
 /* Add nodes relating to color overlay start here'*/
  GeglNode *crop;
  GeglNode *thecoloroverlay;
  GeglNode *nopcolor;
  GeglNode *nocolor;
  GeglNode *coloroverlaypolicy;
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
 /*Outline's special ability subsect*/
  GeglNode *bevelso;
  GeglNode *atopso;
  GeglNode *layerso;
  GeglNode *invisibleblend2;
  GeglNode *opactyimageso;
  GeglNode *replaceontop2so;
  GeglNode *idrefbevelblendmodeso;
  GeglNode *bevelopactyimageso;
  GeglNode *addblendso;
  GeglNode *multiplyblendso;
  GeglNode *multiplyso;
  GeglNode *bevelblendmodeso;
  GeglNode *nopb2so;
  GeglNode *nopb3so;
  GeglNode *bevelalphaso;
  GeglNode *darkbeveloutline;
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
  GeglNode *addblend;
  GeglNode *multiplyblend;
  GeglNode *nopb;
  GeglNode *nopb2;
  GeglNode *nopb3;
  GeglNode *nopb4;
  GeglNode *bevellighting;
 /* Add nodes relating to innerglow start here'*/
  GeglNode *innerglow;
  GeglNode *innerglowblend;
  GeglNode *nopig;
}State;
 /* End of node list one FIRST NODE LIST*/

 /*Beginning of operations list */


static void attach (GeglOperation *operation)
{
  GeglNode *gegl = operation->node;
  GeglProperties *o = GEGL_PROPERTIES (operation);

  State *state = o->user_data = g_malloc0 (sizeof (State));

  state->input    = gegl_node_get_input_proxy (gegl, "input");
  state->output   = gegl_node_get_output_proxy (gegl, "output");

  state->nothing      = gegl_node_new_child (gegl, "operation", "gegl:nop", 
                                                        NULL);

  state->repairgeglgraph      = gegl_node_new_child (gegl, "operation", "gegl:median-blur", "radius", 0,
                                                        NULL);
 /*Repair GEGL Graph is a critical operation for Gimp's non-destructive future.
A median blur at zero radius is confirmed to make no changes to an image. 
This option resets gegl:opacity's value to prevent a known bug where
plugins like clay, glossy balloon and custom bevel glitch out when
drop shadow is applied in a gegl graph below them. 
as of June 16 2023 this is a better option then gegl:alpha-clip. 

 All nodes relating to color overlay go here.*/
  state->thecoloroverlay = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-overlay",
                                  NULL);

  state->nopcolor = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  state->crop = gegl_node_new_child (gegl,
                                  "operation", "gegl:crop",
                                  NULL);

  state->coloroverlaypolicy = gegl_node_new_child (gegl,
                                  "operation", "gegl:multiply",
                                  NULL);


  /*All nodes relating to outline and its special ability go here*/

  state->bevelblendmodeso = gegl_node_new_child (gegl,
                                  "operation", "gegl:multiply",
                                  NULL);


  state->inputso   = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  state->colorso   = gegl_node_new_child (gegl,
                                  "operation", "gegl:color-overlay",
                                  NULL);

  state->nopso   = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);


  state->behindso   = gegl_node_new_child (gegl,
                                  "operation", "gegl:dst-over",
                                  NULL);

  state->opacityso    = gegl_node_new_child (gegl,
                                  "operation", "gegl:opacity",
                                  NULL);


  state->strokeso      = gegl_node_new_child (gegl, "operation", "gegl:median-blur",
                                         "percentile",       100.0,
                                         "alpha-percentile", 100.0,
                                         "abyss-policy",     GEGL_ABYSS_NONE,
                                         NULL);
  state->moveso    = gegl_node_new_child (gegl,
                                  "operation", "gegl:translate",
                                  NULL);

  state->blurso      = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur",
                                         "clip-extent", FALSE,
                                         "abyss-policy", 0,
                                         NULL);            


  state->atopso    = gegl_node_new_child (gegl,
                                  "operation", "gegl:src-atop",
                                  NULL);

  state->idrefbevelblendmodeso   = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  state->invisibleblend2 = gegl_node_new_child (gegl,
                                  "operation", "gegl:dst",
                                  NULL);

  state->replaceontop2so   = gegl_node_new_child (gegl,
                                  "operation", "gegl:src-in",
                                  NULL);

  state->idrefbevelblendmodeso    = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);


  state->layerso    = gegl_node_new_child (gegl,
                                  "operation", "gegl:layer",
                                  NULL);

  state->bevelso    = gegl_node_new_child (gegl,
                                  "operation", "gegl:bevel", "type", 1, "blendmode", 0,
                                  NULL);

  state->opactyimageso    = gegl_node_new_child (gegl,
                                  "operation", "gegl:hue-chroma",
                                  NULL);

  state->multiplyso = gegl_node_new_child (gegl,
                                  "operation", "gegl:multiply",
                                  NULL);

  state->multiplyblendso = gegl_node_new_child (gegl,
                                  "operation", "gegl:multiply",
                                  NULL);

  state->addblendso = gegl_node_new_child (gegl,
                                  "operation", "gegl:add",
                                  NULL);

  state->nopb2so = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  state->nopb3so = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);


#define thresholdalphaso \
" id=0 dst-out aux=[ ref=0  component-extract component=alpha   levels in-low=0.15  color-to-alpha opacity-threshold=0.4  ] "\

  state->bevelalphaso = gegl_node_new_child (gegl,
                                  "operation", "gegl:gegl", "string", thresholdalphaso,
                                  NULL);


  state->bevelopactyimageso = gegl_node_new_child (gegl,
                                  "operation", "gegl:levels",
                                  NULL);


  state->ds = gegl_node_new_child (gegl,
                                  "operation", "gegl:dropshadow",
                                  NULL);

  /*All nodes relating to Inner Glow go here*/

  state->innerglow = gegl_node_new_child (gegl,
                                  "operation", "gegl:inner-glow",
                                  NULL);

    state->nopig = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

    state->innerglowblend = gegl_node_new_child (gegl,
                                  "operation", "gegl:src-atop",
                                  NULL);


  /*All nodes relating to Image file overlay go here*/

    state->atopi = gegl_node_new_child (gegl,
                                  "operation", "gegl:src-atop",
                                  NULL);

     state->image = gegl_node_new_child (gegl,
                                  "operation", "gegl:layer",
                                  NULL);

    state->nopimage = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

    state->imageadjustments = gegl_node_new_child (gegl,
                                  "operation", "gegl:hue-chroma",
                                  NULL);

    state->imageadjustments2 = gegl_node_new_child (gegl,
                                  "operation", "gegl:saturation",
                                  NULL);


  /*All nodes relating to bevel go here*/


    state->bevelblendmode = gegl_node_new_child (gegl,
                                  "operation", "gegl:multiply",
                                  NULL);


#define thresholdalpha \
" id=0 dst-out aux=[ ref=0  component-extract component=alpha   levels in-low=0.15  color-to-alpha opacity-threshold=0.4  ] "\

  state->bevelalpha = gegl_node_new_child (gegl,
                                  "operation", "gegl:gegl", "string", thresholdalpha,
                                  NULL);
  /*This is a GEGL Graph that does something like Gimp's threshold alpha filter or Curves on alpha channel. It gets rid of excesses on GEGL only blend modes.*/



    state->bevelbump = gegl_node_new_child (gegl,
                                  "operation", "gegl:bevel", "type", 1, "blendmode", 0,
                                  NULL);

    state->darkbevel = gegl_node_new_child (gegl,
                                  "operation", "gegl:levels",
                                  NULL);

    state->darkbeveloutline = gegl_node_new_child (gegl,
                                  "operation", "gegl:levels",
                                  NULL);

    state->replaceontop = gegl_node_new_child (gegl,
                                  "operation", "gegl:src-in",
                                  NULL);

    state->nopreplaceontop = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);


    state->invisibleblend = gegl_node_new_child (gegl,
                                  "operation", "gegl:dst",
                                  NULL);


    state->nopb = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);


    state->nopb2 = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);


    state->nopb3 = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);


  state->nopb4 = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);


    state->bevellighting      = gegl_node_new_child (gegl, "operation", "gegl:levels",
                                         NULL);


    state->addblend = gegl_node_new_child (gegl,
                                  "operation", "gegl:add",
                                  NULL);


    state->multiplyblend = gegl_node_new_child (gegl,
                                  "operation", "gegl:multiply",
                                  NULL);




  /*I know this DST doesn't turn it off but it hides it from the user.

End of Operations list*/


  /*Beginning of property configs*/

 /*Nodes relating to Color Overlay*/
  gegl_operation_meta_redirect (operation, "optioncolor", state->thecoloroverlay, "value");
  /*Nodes relating to Image File Overlay*/
  gegl_operation_meta_redirect (operation, "imagesrc",   state->image, "src");
  gegl_operation_meta_redirect (operation, "imageopacity",   state->image, "opacity");
  gegl_operation_meta_redirect (operation, "imagehue",   state->imageadjustments, "hue");
  gegl_operation_meta_redirect (operation, "imagesaturation",   state->imageadjustments2, "scale");
  gegl_operation_meta_redirect (operation, "imagelightness",   state->imageadjustments, "lightness");
  /*Nodes relating to Drop Shadow*/
  gegl_operation_meta_redirect (operation, "shadowx",   state->ds, "x");
  gegl_operation_meta_redirect (operation, "shadowy",   state->ds, "y");
  gegl_operation_meta_redirect (operation, "shadowopacity",   state->ds, "opacity");
  gegl_operation_meta_redirect (operation, "shadowgrowradius",   state->ds, "grow-radius");
  gegl_operation_meta_redirect (operation, "shadowradius",   state->ds, "radius");
  gegl_operation_meta_redirect (operation, "shadowcolor",   state->ds, "color");
  /*Nodes relating to Outline and its special ability to bevel and image file overlay itself*/
  gegl_operation_meta_redirect (operation, "outline",   state->strokeso, "radius");
  gegl_operation_meta_redirect (operation, "outlineblur",   state->blurso, "std-dev-x");
  gegl_operation_meta_redirect (operation, "outlineblur",   state->blurso, "std-dev-y");
  gegl_operation_meta_redirect (operation, "outlinex",   state->moveso, "x");
  gegl_operation_meta_redirect (operation, "outliney",   state->moveso, "y");
  gegl_operation_meta_redirect (operation, "outlinegrowshape",   state->strokeso, "neighborhood");
  gegl_operation_meta_redirect (operation, "outlineopacity",   state->opacityso, "value");
  gegl_operation_meta_redirect (operation, "outlinecolor",   state->colorso, "value");
  /*Outline's special ability subsection (named os for Outline Special)*/
  gegl_operation_meta_redirect (operation, "osradius",   state->bevelso, "radius");
  gegl_operation_meta_redirect (operation, "oselevation",   state->bevelso, "elevation");
  gegl_operation_meta_redirect (operation, "osdepth",   state->bevelso, "depth");
  gegl_operation_meta_redirect (operation, "osazimuth",   state->bevelso, "azimuth");
  gegl_operation_meta_redirect (operation, "osdark",     state->darkbeveloutline, "out-low");
  gegl_operation_meta_redirect (operation, "ossrc",   state->layerso, "src");
  gegl_operation_meta_redirect (operation, "ossrcopacity",   state->layerso, "opacity");
  gegl_operation_meta_redirect (operation, "osadd",   state->addblendso, "value");
  gegl_operation_meta_redirect (operation, "osmultiply",   state->multiplyblendso, "value");
  /*Nodes relating to Inner Glow*/
  gegl_operation_meta_redirect (operation, "innerggrowradius",   state->innerglow, "grow-radius");
  gegl_operation_meta_redirect (operation, "innergradius",   state->innerglow, "radius");
  gegl_operation_meta_redirect (operation, "innergopacity",   state->innerglow, "opacity");
  gegl_operation_meta_redirect (operation, "innergvalue",   state->innerglow, "value");
  gegl_operation_meta_redirect (operation, "innergtreatment",   state->innerglow, "cover");
  /*Nodes relating to Bevel*/
  gegl_operation_meta_redirect (operation, "beveldepth",   state->bevelbump, "depth");
  gegl_operation_meta_redirect (operation, "bevelradius",   state->bevelbump, "radius");
  gegl_operation_meta_redirect (operation, "bevelelevation",   state->bevelbump, "elevation");
  gegl_operation_meta_redirect (operation, "bevelazimuth",   state->bevelbump, "azimuth");
  gegl_operation_meta_redirect (operation, "beveldark",   state->darkbevel, "out-low");
  gegl_operation_meta_redirect (operation, "beveladd",   state->addblend, "value");
  gegl_operation_meta_redirect (operation, "beveltype",   state->bevelbump, "type");
  gegl_operation_meta_redirect (operation, "bevelmultiply",   state->multiplyblend, "value");
  /*End of property configs*/

}


static void update_graph (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  State *state = o->user_data;
  if (!state) return;
  GeglNode *replaceontop;
  GeglNode *replaceontop2so;


const char *blend_bevel = "gegl:nop";
  switch (o->bevelblend) {
    case GEGL_BLEND_MODE_TYPE_BEVELOFF_BEVEL:  blend_bevel = "gegl:dst"; break;
    case GEGL_BLEND_MODE_TYPE_MULTIPLY_BEVEL_OUTLINE:  blend_bevel = "gegl:multiply"; break;
    case GEGL_BLEND_MODE_TYPE_ADD_BEVEL_OUTLINE:  blend_bevel = "gegl:add"; break;
    case GEGL_BLEND_MODE_TYPE_HARDLIGHT_BEVEL_OUTLINE:  blend_bevel = "gegl:hard-light"; break;
    case GEGL_BLEND_MODE_TYPE_DARKEN_BEVEL_OUTLINE: blend_bevel = "gegl:darken"; break;
  }
  gegl_node_set (state->bevelblendmode, "operation", blend_bevel, NULL);

  const char *inner_glow_blend = "gegl:nop"; /* This is where inner glow's blend mode is swapped */
  switch (o->innergblend) {
    case GEGL_BLEND_MODE_TYPE_NORMALIG: inner_glow_blend = "gegl:src-atop"; break;
    case GEGL_BLEND_MODE_TYPE_OVERLAYIG: inner_glow_blend = "gegl:overlay"; break;
    case GEGL_BLEND_MODE_TYPE_SCREENIG: inner_glow_blend = "gegl:screen"; break;
    case GEGL_BLEND_MODE_TYPE_HARDLIGHTIG: inner_glow_blend =  "gegl:hard-light"; break;
    case GEGL_BLEND_MODE_TYPE_COLORDODGEIG: inner_glow_blend = "gegl:color-dodge"; break;
    case GEGL_BLEND_MODE_TYPE_PLUSIG: inner_glow_blend = "gegl:plus"; break;

  }
  gegl_node_set (state->innerglowblend, "operation", inner_glow_blend, NULL); 

  const char *blend_outline_bevel = "gegl:nop";
  switch (o->osblend) {
   case GEGL_BLEND_MODE_TYPE_BEVELOFF_BEVEL:  blend_outline_bevel = "gegl:dst"; break;
    case GEGL_BLEND_MODE_TYPE_MULTIPLY_BEVEL_OUTLINE:  blend_outline_bevel = "gegl:multiply"; break;
    case GEGL_BLEND_MODE_TYPE_ADD_BEVEL_OUTLINE:  blend_outline_bevel = "gegl:add"; break;
    case GEGL_BLEND_MODE_TYPE_HARDLIGHT_BEVEL_OUTLINE:  blend_outline_bevel = "gegl:hard-light"; break;
    case GEGL_BLEND_MODE_TYPE_DARKEN_BEVEL_OUTLINE: blend_outline_bevel = "gegl:darken"; break;
  }
  gegl_node_set (state->bevelblendmodeso, "operation", blend_outline_bevel, NULL);

  const char *blend_color = "gegl:nop";
  switch (o->policycolor) {
    case GEGL_BLEND_MODE_TYPE_NO_COLOR:  blend_color = "gegl:dst"; break;
    case GEGL_BLEND_MODE_TYPE_MULTIPLY_COLOR:  blend_color = "gegl:multiply"; break;
    case GEGL_BLEND_MODE_TYPE_SOLID_COLOR:   blend_color = "gegl:src"; break;
  }
  gegl_node_set (state->coloroverlaypolicy, "operation", blend_color, NULL);

  /* This is here so in default GEGL Styles does nothing on launch. That is how I like it. src-in node does a stack and this disables that by swapping it with dst (the invisible blend mode) */
if (o->bevelblend > 0)
replaceontop = state->replaceontop;
else
replaceontop = state->invisibleblend;

if (o->osblend > 0)
replaceontop2so = state->replaceontop2so;
else
replaceontop2so = state->invisibleblend2;

  if (o->enableinnerglow)
  if (o->enableoutline)
  if (o->enablespecialoutline)
  {
  /* INNERGLOW + SPECIAL OUTLINE */

         gegl_node_link_many (state->input, state->nopimage, state->atopi, state->nopcolor, state->coloroverlaypolicy, state->crop,  state->nopreplaceontop, replaceontop,   state->nopig, state->innerglowblend, state->inputso, state->behindso, state->ds, state->repairgeglgraph, state->output, NULL);
/* All nodes relating to Outline here*/
    gegl_node_link_many (state->inputso, state->strokeso, state->blurso, state->moveso, state->colorso, state->atopso, state->idrefbevelblendmodeso, replaceontop2so, state->opacityso,  NULL);
    gegl_node_connect_from (state->behindso, "aux", state->opacityso, "output");
    gegl_node_connect_from (state->bevelblendmodeso, "aux", state->nopb3so, "output");
    gegl_node_link_many (state->atopso, state->darkbeveloutline, state->bevelso, state->addblendso, state->multiplyblendso, state->bevelalphaso, state->nopb3so,  NULL);
    gegl_node_connect_from (state->atopso, "aux", state->opactyimageso, "output");
    gegl_node_link_many (state->nopso, state->layerso, state->opactyimageso,  NULL); 
    gegl_node_connect_from (replaceontop2so, "aux", state->bevelblendmodeso, "output");
    gegl_node_link_many (state->idrefbevelblendmodeso, state->bevelblendmodeso,  NULL); 
/* All nodes relating to Inner Glow here*/
      gegl_node_link_many (state->nopig, state->innerglow, NULL);
      gegl_node_connect_from (state->innerglowblend, "aux", state->innerglow, "output");
/* All nodes relating to image file upload here*/
      gegl_node_link_many (state->nopimage, state->image, state->imageadjustments, state->imageadjustments2, NULL);
      gegl_node_connect_from (state->atopi, "aux", state->imageadjustments2, "output");
/* All nodes relating to bevel here*/
      gegl_node_link_many ( state->nopreplaceontop, state->nopb, state->bevelblendmode, NULL); 
      gegl_node_link_many (state->nopb, state->darkbevel, state->bevelbump, state->addblend, state->multiplyblend, state->bevelalpha,  NULL);
      gegl_node_connect_from (state->bevelblendmode, "aux", state->bevelalpha, "output");
      gegl_node_connect_from (replaceontop, "aux", state->bevelblendmode, "output");
/* All nodes relating to color overlay here*/
      gegl_node_link_many (state->nopcolor, state->thecoloroverlay, NULL);
      gegl_node_connect_from (state->coloroverlaypolicy, "aux", state->thecoloroverlay, "output");

  }
  else 
  { 

  /* INNERGLOW + NORMAL OUTLINE */
         gegl_node_link_many (state->input, state->nopimage, state->atopi, state->nopcolor, state->coloroverlaypolicy, state->crop,  state->nopreplaceontop, replaceontop,  state->nopig, state->innerglowblend, state->inputso, state->behindso, state->ds, state->repairgeglgraph, state->output, NULL);
/* All nodes relating to Outline here*/
    gegl_node_link_many (state->inputso, state->strokeso, state->blurso, state->moveso, state->colorso, state->opacityso, NULL);
     gegl_node_connect_from (state->behindso, "aux", state->opacityso, "output");
/* All nodes relating to Inner Glow here*/
      gegl_node_link_many (state->nopig, state->innerglow, NULL);
      gegl_node_connect_from (state->innerglowblend, "aux", state->innerglow, "output");
/* All nodes relating to image file upload here*/
      gegl_node_link_many (state->nopimage, state->image, state->imageadjustments, state->imageadjustments2, NULL);
      gegl_node_connect_from (state->atopi, "aux", state->imageadjustments2, "output");
/* All nodes relating to bevel here*/
      gegl_node_link_many ( state->nopreplaceontop, state->nopb, state->bevelblendmode, NULL); 
      gegl_node_link_many (state->nopb, state->darkbevel, state->bevelbump, state->addblend, state->multiplyblend, state->bevelalpha,  NULL);
      gegl_node_connect_from (state->bevelblendmode, "aux", state->bevelalpha, "output");
      gegl_node_connect_from (replaceontop, "aux", state->bevelblendmode, "output");
/* All nodes relating to color overlay here*/
      gegl_node_link_many (state->nopcolor, state->thecoloroverlay, NULL);
      gegl_node_connect_from (state->coloroverlaypolicy, "aux", state->thecoloroverlay, "output");
  }
  else 
  { 
 /*NO OUTLINE INNER GLOW */
         gegl_node_link_many (state->input, state->nopimage, state->atopi, state->nopcolor, state->coloroverlaypolicy, state->crop, state->nopreplaceontop, replaceontop,  state->nopig, state->innerglowblend,  state->ds, state->repairgeglgraph, state->output, NULL);
/* All nodes relating to Inner Glow here*/
      gegl_node_link_many (state->nopig, state->innerglow, NULL);
      gegl_node_connect_from (state->innerglowblend, "aux", state->innerglow, "output");
/* All nodes relating to image file upload here*/
      gegl_node_link_many (state->nopimage, state->image, state->imageadjustments, state->imageadjustments2, NULL);
      gegl_node_connect_from (state->atopi, "aux", state->imageadjustments2, "output");
/* All nodes relating to bevel here*/
      gegl_node_link_many ( state->nopreplaceontop, state->nopb, state->bevelblendmode, NULL); 
      gegl_node_link_many (state->nopb, state->darkbevel, state->bevelbump, state->addblend, state->multiplyblend, state->bevelalpha,  NULL);
      gegl_node_connect_from (state->bevelblendmode, "aux", state->bevelalpha, "output");
      gegl_node_connect_from (replaceontop, "aux", state->bevelblendmode, "output");
/* All nodes relating to color overlay here*/
      gegl_node_link_many (state->nopcolor, state->thecoloroverlay, NULL);
      gegl_node_connect_from (state->coloroverlaypolicy, "aux", state->thecoloroverlay, "output");
    }

else 
 /*SPECIAL OUTLINE*/
  if (o->enableoutline)
  if (o->enablespecialoutline)
  {
         gegl_node_link_many (state->input, state->nopimage, state->atopi, state->nopcolor, state->coloroverlaypolicy, state->crop, state->nopreplaceontop, replaceontop, state->inputso, state->behindso, state->ds, state->repairgeglgraph, state->output, NULL);
/* All nodes relating to Outline here*/
    gegl_node_link_many (state->inputso, state->strokeso, state->blurso, state->moveso, state->colorso, state->atopso, state->idrefbevelblendmodeso, replaceontop2so, state->opacityso,  NULL);
    gegl_node_connect_from (state->behindso, "aux", state->opacityso, "output");
    gegl_node_connect_from (state->bevelblendmodeso, "aux", state->nopb3so, "output");
    gegl_node_link_many (state->atopso, state->darkbeveloutline, state->bevelso, state->addblendso, state->multiplyblendso, state->bevelalphaso, state->nopb3so,  NULL);
    gegl_node_connect_from (state->atopso, "aux", state->opactyimageso, "output");
    gegl_node_link_many (state->nopso, state->layerso, state->opactyimageso,  NULL); 
    gegl_node_connect_from (replaceontop2so, "aux", state->bevelblendmodeso, "output");
    gegl_node_link_many (state->idrefbevelblendmodeso, state->bevelblendmodeso,  NULL); 
/* All nodes relating to image file upload here*/
      gegl_node_link_many (state->nopimage, state->image, state->imageadjustments, state->imageadjustments2, NULL);
      gegl_node_connect_from (state->atopi, "aux", state->imageadjustments2, "output");
/* All nodes relating to bevel here*/
      gegl_node_link_many ( state->nopreplaceontop, state->nopb, state->bevelblendmode, NULL); 
      gegl_node_link_many (state->nopb, state->darkbevel, state->bevelbump, state->addblend, state->multiplyblend, state->bevelalpha,  NULL);
      gegl_node_connect_from (state->bevelblendmode, "aux", state->bevelalpha, "output");
      gegl_node_connect_from (replaceontop, "aux", state->bevelblendmode, "output");
/* All nodes relating to color overlay here*/
      gegl_node_link_many (state->nopcolor, state->thecoloroverlay, NULL);
      gegl_node_connect_from (state->coloroverlaypolicy, "aux", state->thecoloroverlay, "output");
 /*NORMAL OUTLINE*/
  }
  else 
  { 
         gegl_node_link_many (state->input, state->nopimage, state->atopi, state->nopcolor, state->coloroverlaypolicy, state->crop, state->nopreplaceontop, replaceontop, state->inputso, state->behindso, state->ds, state->repairgeglgraph, state->output, NULL);
/* All nodes relating to Outline here*/
    gegl_node_link_many (state->inputso, state->strokeso, state->blurso, state->moveso, state->colorso, state->opacityso, NULL);
     gegl_node_connect_from (state->behindso, "aux", state->opacityso, "output");
/* All nodes relating to image file upload here*/
      gegl_node_link_many (state->nopimage, state->image, state->imageadjustments, state->imageadjustments2, NULL);
      gegl_node_connect_from (state->atopi, "aux", state->imageadjustments2, "output");
/* All nodes relating to bevel here*/
      gegl_node_link_many ( state->nopreplaceontop, state->nopb, state->bevelblendmode, NULL); 
      gegl_node_link_many (state->nopb, state->darkbevel, state->bevelbump, state->addblend, state->multiplyblend, state->bevelalpha,  NULL);
      gegl_node_connect_from (state->bevelblendmode, "aux", state->bevelalpha, "output");
      gegl_node_connect_from (replaceontop, "aux", state->bevelblendmode, "output");
/* All nodes relating to color overlay here*/
      gegl_node_link_many (state->nopcolor, state->thecoloroverlay, NULL);
      gegl_node_connect_from (state->coloroverlaypolicy, "aux", state->thecoloroverlay, "output");
  }
  else  
  { 
 /*NO OUTLINE*/
       gegl_node_link_many (state->input, state->nopimage, state->atopi, state->nopcolor, state->coloroverlaypolicy, state->crop, state->nopreplaceontop, replaceontop,  state->ds, state->repairgeglgraph, state->output, NULL);
/* All nodes relating to image file upload here*/
      gegl_node_link_many (state->nopimage, state->image, state->imageadjustments, state->imageadjustments2, NULL);
      gegl_node_connect_from (state->atopi, "aux", state->imageadjustments2, "output");
/* All nodes relating to bevel here*/
      gegl_node_link_many ( state->nopreplaceontop, state->nopb, state->bevelblendmode, NULL); 
      gegl_node_link_many (state->nopb, state->darkbevel, state->bevelbump, state->addblend, state->multiplyblend, state->bevelalpha,  NULL);
      gegl_node_connect_from (state->bevelblendmode, "aux", state->bevelalpha, "output");
      gegl_node_connect_from (replaceontop, "aux", state->bevelblendmode, "output");
/* All nodes relating to color overlay here*/
      gegl_node_link_many (state->nopcolor, state->thecoloroverlay, NULL);
      gegl_node_connect_from (state->coloroverlaypolicy, "aux", state->thecoloroverlay, "output");
}
    }
/*END OF SIX GEGL GRAPHS*/

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
    "title",       _IGNORED("GEGL Styles"),
    "categories",  "Generic",
    "reference-hash", "129945ed565h8500fca01b2ac",
    "description", _IGNORED("A text styling engine capable of making thousands of unique text styles. This also works as a special tool for outlining and adding effects to images with alpha channels."
                     ""),
    "gimp:menu-path", "<Image>/Filters/Generic/",
    "gimp:menu-label", _IGNORED("Style text and add popular effects to alpha channel images"),
    NULL);
}

#endif
