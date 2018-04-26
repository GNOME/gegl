/* This file is an image processing operation for GEGL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
#include <math.h>

static void prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl     *format;

  if (! o->srgb)
    format = babl_format ("RGBA float");
  else
    format = babl_format ("R'G'B'A float");

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
  const Babl     *format;
  gfloat          color[4];
  gfloat          alpha_c;

  if (! o->srgb)
    format = babl_format ("RaGaBaA float");
  else
    format = babl_format ("R'aG'aB'aA float");

  gegl_color_get_pixel (o->value, format, &color);

  alpha_c = 1.0f - color[3];

  if (fabs (alpha_c) <= EPSILON)
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

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->prepare    = prepare;
  operation_class->process    = operation_process;

  point_filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
      "name",        "gegl:color-overlay",
      "categories",  "color",
      "title",       _("Color Overlay"),
      "description", _("Paint a color overlay over the input, "
                       "preserving its transparency."),
      NULL);
}

#endif

