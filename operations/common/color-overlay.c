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
 * Copyright (C) 2018 Ell
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#define EPSILON 1e-6

#ifdef GEGL_PROPERTIES

property_color (value, _("Color"), "transparent")
    description (_("The color to paint over the input"))
    ui_meta     ("role", "color-primary")

property_boolean (srgb, _("sRGB"), FALSE)
    description (_("Use sRGB gamma instead of linear"))

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     color_overlay
#define GEGL_OP_C_SOURCE color-overlay.c

#include "gegl-op.h"

static void prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl     *format;

  if (! o->srgb)
    format = babl_format_with_space ("RGBA float", space);
  else
    format = babl_format_with_space ("R~G~B~A float", space);

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o   = GEGL_PROPERTIES (operation);
  gfloat         *in  = in_buf;
  gfloat         *out = out_buf;
  const Babl     *format = gegl_operation_get_format (operation, "output");
  gfloat          color[4];
  gfloat          alpha_c;
  gint            i;

  gegl_color_get_pixel (o->value, format, &color);

  for (i = 0; i < 3; i++)
    color[i] *= color[3];

  alpha_c = 1.0f - color[3];

  if (fabsf (alpha_c) <= EPSILON)
    {
      while (samples--)
        {
          gint i;

          for (i = 0; i < 3; i++)
            out[i] = color[i];

          out[3] = in[3];

          in  += 4;
          out += 4;
        }
    }
  else
    {
      while (samples--)
        {
          gint i;

          for (i = 0; i < 3; i++)
            out[i] = in[i] * alpha_c + color[i];

          out[3] = in[3];

          in  += 4;
          out += 4;
        }
    }

  return TRUE;
}

static gboolean
operation_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gdouble         alpha;

  gegl_color_get_rgba (o->value, NULL, NULL, NULL, &alpha);

  if (fabs (alpha) <= EPSILON)
    {
      /* pass input directly */
      gegl_operation_context_set_object (
        context,
        "output", gegl_operation_context_get_object (context, "input"));

      return TRUE;
    }

  return GEGL_OPERATION_CLASS (gegl_op_parent_class)->process (
    operation, context, output_prop, result, level);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;
  gchar                         *composition =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "  <node operation='gegl:crop' width='200' height='200'/>"
    "  <node operation='gegl:over'>"
    "    <node operation='gegl:color-overlay'>"
    "      <params>"
    "        <param name='value'>rgba(1.0,0.0,0.0,0.2)</param>"
    "      </params>"
    "    </node>"
    "    <node operation='gegl:load' path='standard-input.png'/>"
    "  </node>"
    "  <node operation='gegl:checkerboard'>"
    "    <params>"
    "      <param name='color1'>rgb(0.25,0.25,0.25)</param>"
    "      <param name='color2'>rgb(0.75,0.75,0.75)</param>"
    "    </params>"
    "  </node>"    
    "</gegl>";

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->prepare    = prepare;
  operation_class->process    = operation_process;

  point_filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
      "name",        "gegl:color-overlay",
      "categories",  "color",
      "title",       _("Color Overlay"),
      "reference-hash", "078b15ab732d02f2df2c5c111059d0ce",
      "reference-composition", composition,
      "description", _("Paint a color overlay over the input, "
                       "preserving its transparency."),
      NULL);
}

#endif

