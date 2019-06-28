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
 * Copyright 2019 Øyvind Kolås
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (scale, _("Scale"), 1.0)
    description(_("Scale, strength of effect"))
    value_range (0.0, 10.0)
    ui_range (0.0, 2.0)

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     saturation_hue_constant
#define GEGL_OP_C_SOURCE saturation-hue-constant.c

#include "gegl-op.h"

static void prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  const Babl *format;

  format = babl_format_with_space ("RGBA float", space);

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  glong i;
  float scale = o->scale;
  float rscale = 1.0f - o->scale;
  double red_luminance, green_luminance, blue_luminance;

  babl_space_get_rgb_luminance (space,
    &red_luminance, &green_luminance, &blue_luminance);

  for (i = 0; i < n_pixels; i++)
    {
      gfloat desaturated = (in[0] * red_luminance +
                            in[1] * green_luminance +
                            in[2] * green_luminance) * rscale;
      for (int c = 0; c < 3; c ++)
        out[c] = desaturated + in[c] * scale;
      out[3] = in[3];

      in += 4;
      out += 4;
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_filter_class =
    GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->prepare = prepare;
  operation_class->opencl_support = FALSE;

  point_filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:saturation-hue-constant",
    "title",       _("Saturation with constant hue"),
    "categories" , "color",
    "description", _("Changes the saturation"),
    NULL);
}

#endif
