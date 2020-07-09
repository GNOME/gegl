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
 * Copyright 2020 Ell
 */

#include "config.h"
#include <glib/gi18n-lib.h>

/* #define MANUAL_CONTROL */

#define MAX_GAMMA 1000.0

#ifdef GEGL_PROPERTIES

enum_start (gegl_focus_blur_type)
  enum_value (GEGL_FOCUS_BLUR_TYPE_GAUSSIAN, "gaussian", N_("Gaussian Blur"))
  enum_value (GEGL_FOCUS_BLUR_TYPE_LENS,     "lens",     N_("Lens Blur"))
enum_end (GeglFocusBlurType)

enum_start (gegl_focus_blur_shape)
  enum_value (GEGL_FOCUS_BLUR_SHAPE_CIRCLE,     "circle",     N_("Circle"))
  enum_value (GEGL_FOCUS_BLUR_SHAPE_SQUARE,     "square",     N_("Square"))
  enum_value (GEGL_FOCUS_BLUR_SHAPE_DIAMOND,    "diamond",    N_("Diamond"))
  enum_value (GEGL_FOCUS_BLUR_SHAPE_HORIZONTAL, "horizontal", N_("Horizontal"))
  enum_value (GEGL_FOCUS_BLUR_SHAPE_VERTICAL,   "vertical",   N_("Vertical"))
enum_end (GeglFocusBlurShape)

property_enum (blur_type, _("Blur type"),
               GeglFocusBlurType, gegl_focus_blur_type,
               GEGL_FOCUS_BLUR_TYPE_GAUSSIAN)

property_double (blur_radius, _("Blur radius"), 25.0)
    description (_("Out-of-focus blur radius"))
    value_range (0.0, 1500.0)
    ui_range    (0.0, 100.0)
    ui_gamma    (2.0)
    ui_meta     ("unit", "pixel-distance")

property_double (highlight_factor, _("Highlight factor"), 0.0)
    description (_("Relative highlight strength"))
    value_range (0.0, 1.0)
    ui_meta     ("visible", "blur-type {lens}")

property_double (highlight_threshold_low, _("Highlight threshold (low)"), 0.0)
    ui_range    (0.0, 1.0)
    ui_meta     ("role", "range-start")
    ui_meta     ("unit", "luminance")
    ui_meta     ("range-label", _("Highlight threshold"))
    ui_meta     ("visible", "$highlight-factor.visible")

property_double (highlight_threshold_high, _("Highlight threshold (high)"), 1.0)
    ui_range    (0.0, 1.0)
    ui_meta     ("role", "range-end")
    ui_meta     ("unit", "luminance")
    ui_meta     ("visible", "$highlight-threshold-low.visible")

property_enum (shape, _("Shape"),
               GeglFocusBlurShape,
               gegl_focus_blur_shape,
               GEGL_FOCUS_BLUR_SHAPE_CIRCLE)

property_double (x, _("Center X"), 0.5)
    ui_range    (0, 1.0)
    ui_meta     ("unit", "relative-coordinate")
    ui_meta     ("axis", "x")

property_double (y, _("Center Y"), 0.5)
    ui_range    (0, 1.0)
    ui_meta     ("unit", "relative-coordinate")
    ui_meta     ("axis", "y")

property_double (radius, _("Radius"), 0.75)
    description (_("Focus-region outer radius"))
    value_range (0.0, G_MAXDOUBLE)
    ui_range    (0.0, 5.0)
    ui_meta     ("unit", "relative-distance")

property_double (focus, _("Sharpness"), 0.25)
    description (_("Focus-region inner limit"))
    value_range (0.0, 1.0)

property_double (midpoint, _("Midpoint"), 0.5)
    description (_("Focus-transition midpoint"))
    value_range (0.0, 1.0)

property_double (aspect_ratio, _("Aspect ratio"), 0.0)
    value_range (-1.0, +1.0)

property_double (rotation, _("Rotation"), 0.0)
    value_range (-180.0, +180.0)
    ui_meta     ("unit", "degree")
    ui_meta     ("direction", "cw")

#ifdef MANUAL_CONTROL

property_int (blur_levels, _("Blur levels"), 8)
    description (_("Number of blur levels"))
    value_range (2, 16)
    ui_meta     ("visible", "blur-type {gaussian}")

property_double (blur_gamma, _("Blur gamma"), 1.5)
    description (_("Gamma factor for blur-level spacing"))
    value_range (0.0, G_MAXDOUBLE)
    ui_range    (0.1, 10.0)
    ui_meta     ("visible", "blur-type {gaussian}")

#else

property_boolean (high_quality, _("High quality"), FALSE)
    description (_("Generate more accurate and consistent output (slower)"))
    ui_meta     ("visible", "blur-type {gaussian}")

#endif

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     focus_blur
#define GEGL_OP_C_SOURCE focus-blur.c

#include "gegl-op.h"

typedef struct
{
  GeglFocusBlurType  blur_type;

  GeglNode          *input;
  GeglNode          *output;

  GeglNode          *color;
  GeglNode          *crop;
  GeglNode          *vignette;

  GeglNode          *blur;
} Nodes;

static void
update (GeglOperation *operation)
{
  GeglProperties *o     = GEGL_PROPERTIES (operation);
  Nodes          *nodes = o->user_data;

  gdouble scale;
  gdouble squeeze;

  if (o->aspect_ratio >= 0.0)
    scale = 1.0 - o->aspect_ratio;
  else
    scale = 1.0 / (1.0 + o->aspect_ratio);

  if (scale <= 1.0)
    squeeze = +2.0 * atan (1.0 / scale - 1.0) / G_PI;
  else
    squeeze = -2.0 * atan (scale - 1.0) / G_PI;

  gegl_node_set (nodes->vignette,
                 "shape",    o->shape,
                 "radius",   o->radius,
                 "softness", 1.0 - o->focus,
                 "gamma",    o->midpoint < 1.0 ?
                               MIN (log (0.5) / log (o->midpoint),
                                    MAX_GAMMA) :
                               MAX_GAMMA,
                 "squeeze",  squeeze,
                 "x",        o->x,
                 "y",        o->y,
                 "rotation", fmod (o->rotation + 360.0, 360.0),
                 NULL);

  if (o->blur_type != nodes->blur_type)
    {
      nodes->blur_type = o->blur_type;

      switch (nodes->blur_type)
        {
        case GEGL_FOCUS_BLUR_TYPE_GAUSSIAN:
          gegl_node_set (nodes->blur,
                         "operation",   "gegl:variable-blur",
                         "linear-mask", TRUE,
                         NULL);

          gegl_operation_meta_redirect (operation,   "blur-radius",
                                        nodes->blur, "radius");
#ifdef MANUAL_CONTROLS
          gegl_operation_meta_redirect (operation,   "blur-levels",
                                        nodes->blur, "levels");
          gegl_operation_meta_redirect (operation,   "blur-gamma",
                                        nodes->blur, "gamma");
#else
          gegl_operation_meta_redirect (operation,   "high-quality",
                                        nodes->blur, "high-quality");
#endif
          break;

        case GEGL_FOCUS_BLUR_TYPE_LENS:
          gegl_node_set (nodes->blur,
                         "operation",   "gegl:lens-blur",
                         "linear-mask", TRUE,
                         NULL);

          gegl_operation_meta_redirect (operation,   "blur-radius",
                                        nodes->blur, "radius");
          gegl_operation_meta_redirect (operation,   "highlight-factor",
                                        nodes->blur, "highlight-factor");
          gegl_operation_meta_redirect (operation,   "highlight-threshold-low",
                                        nodes->blur, "highlight-threshold-low");
          gegl_operation_meta_redirect (operation,   "highlight-threshold-high",
                                        nodes->blur, "highlight-threshold-high");
          break;
        }
    }
}

static void
attach (GeglOperation *operation)
{
  GeglProperties *o     = GEGL_PROPERTIES (operation);
  Nodes          *nodes;
  GeglColor      *black = gegl_color_new ("black");
  GeglColor      *white = gegl_color_new ("white");

  if (! o->user_data)
    o->user_data = g_slice_new (Nodes);

  nodes = o->user_data;

  nodes->blur_type = -1;

  nodes->input  = gegl_node_get_input_proxy  (operation->node, "input");
  nodes->output = gegl_node_get_output_proxy (operation->node, "output");

  nodes->color = gegl_node_new_child (
    operation->node,
    "operation", "gegl:color",
    "value",     black,
    NULL);

  nodes->crop = gegl_node_new_child (
    operation->node,
    "operation", "gegl:crop",
    NULL);

  nodes->vignette = gegl_node_new_child (
    operation->node,
    "operation",  "gegl:vignette",
    "color",      white,
    "proportion", 0.0,
    NULL);

  nodes->blur = gegl_node_new_child (
    operation->node,
    "operation", "gegl:variable-blur",
    NULL);

  gegl_node_link_many (nodes->input,
                       nodes->blur,
                       nodes->output,
                       NULL);

  gegl_node_link_many (nodes->color,
                       nodes->crop,
                       nodes->vignette,
                       NULL);

  gegl_node_connect_to (nodes->input, "output",
                        nodes->crop,  "aux");

  gegl_node_connect_to (nodes->vignette, "output",
                        nodes->blur,     "aux");

  g_object_unref (white);
  g_object_unref (black);
}

static void
dispose (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);

  if (o->user_data)
    {
      g_slice_free (Nodes, o->user_data);

      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->dispose (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass           *object_class;
  GeglOperationClass     *operation_class;
  GeglOperationMetaClass *operation_meta_class;

  object_class         = G_OBJECT_CLASS (klass);
  operation_class      = GEGL_OPERATION_CLASS (klass);
  operation_meta_class = GEGL_OPERATION_META_CLASS (klass);

  object_class->dispose        = dispose;
  operation_class->attach      = attach;
  operation_meta_class->update = update;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:focus-blur",
    "title",       _("Focus Blur"),
    "categories",  "blur",
    "reference-hash", "a6f7a6425769c7d8b1d277a5c3f25973",
    "description", _("Blur the image around a focal point"),
    NULL);
}

#endif
