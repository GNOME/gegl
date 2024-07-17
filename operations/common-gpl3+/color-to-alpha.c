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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Color To Alpha plug-in v1.0 by Seth Burgess, sjburges@gimp.org 1999/05/14
 *  with algorithm by clahey
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 2011 Robert Sasu <sasu.robert@gmail.com>
 * Copyright (C) 2012 Øyvind Kolås <pippin@gimp.org>
 * Copyright (C) 2017 Ell
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_color (color, _("Color"), "white")
    description(_("The color to make transparent."))

property_double (transparency_threshold, _("Transparency threshold"), 0.0)
    description(_("The limit below which colors become transparent."))
    value_range (0.0, 1.0)

property_double (opacity_threshold, _("Opacity threshold"), 1.0)
    description(_("The limit above which colors remain opaque."))
    value_range (0.0, 1.0)

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     color_to_alpha
#define GEGL_OP_C_SOURCE color-to-alpha.c

#include "gegl-op.h"
#include <stdio.h>

#define EPSILON 0.00001

static void
prepare (GeglOperation *operation)
{
  const Babl     *space  = gegl_operation_get_source_space (operation, "input");
  gegl_operation_set_format (operation, "input",
                             babl_format_with_space ("R'G'B'A float", space));
  gegl_operation_set_format (operation, "output",
                             babl_format_with_space ("R'G'B'A float", space));
}

/*
 * An excerpt from a discussion on #gimp that sheds some light on the ideas
 * behind the algorithm that is being used here.
 *
 <clahey>   so if a1 > c1, a2 > c2, and a3 > c2 and a1 - c1 > a2-c2, a3-c3,
 then a1 = b1 * alpha + c1 * (1-alpha)
 So, maximizing alpha without taking b1 above 1 gives
 a1 = alpha + c1(1-alpha) and therefore alpha = (a1-c1) / (1-c1).
 <sjburges> clahey: btw, the ordering of that a2, a3 in the white->alpha didn't
 matter
 <clahey>   sjburges: You mean that it could be either a1, a2, a3 or
 a1, a3, a2?
 <sjburges> yeah
 <sjburges> because neither one uses the other
 <clahey>   sjburges: That's exactly as it should be.  They are both just
 getting reduced to the same amount, limited by the the darkest
 color.
 <clahey>   Then a2 = b2 * alpha + c2 * (1- alpha).  Solving for b2 gives
 b2 = (a1-c2)/alpha + c2.
 <sjburges> yeah
 <clahey>   That gives us are formula for if the background is darker than the
 foreground? Yep.
 <clahey>   Next if a1 < c1, a2 < c2, a3 < c3, and c1-a1 > c2-a2, c3-a3, and
 by our desired result a1 = b1 * alpha + c1 * (1-alpha),
 we maximize alpha without taking b1 negative gives
 alpha = 1 - a1 / c1.
 <clahey>   And then again, b2 = (a2-c2) / alpha + c2 by the same formula.
 (Actually, I think we can use that formula for all cases, though
 it may possibly introduce rounding error.
 <clahey>   sjburges: I like the idea of using floats to avoid rounding error.
 Good call.
*/

static void
color_to_alpha (const gfloat *color,
                const gfloat *src,
                gfloat       *dst,
                gfloat        transparency_threshold,
                gfloat        opacity_threshold)
{
  gint   i;
  gfloat dist  = 0.0f;
  gfloat alpha = 0.0f;

  for (i = 0; i < 4; i++)
    dst[i] = src[i];

  for (i = 0; i < 3; i++)
    {
      gfloat d;
      gfloat a;

      d = fabsf (dst[i] - color[i]);

      if (d < transparency_threshold + EPSILON)
        a = 0.0f;
      else if (d > opacity_threshold - EPSILON)
        a = 1.0f;
      else if (dst[i] < color[i])
        a = (d - transparency_threshold) / (MIN (opacity_threshold,        color[i]) - transparency_threshold);
      else
        a = (d - transparency_threshold) / (MIN (opacity_threshold, 1.0f - color[i]) - transparency_threshold);

      if (a > alpha)
        {
          alpha = a;
          dist  = d;
        }
    }

  if (alpha > EPSILON)
    {
      gfloat ratio     = transparency_threshold / dist;
      gfloat alpha_inv = 1.0f / alpha;

      for (i = 0; i < 3; i++)
        {
          gfloat c;

          c = color[i] + (dst[i] - color[i]) * ratio;

          dst[i] = c + (dst[i] - c) * alpha_inv;
        }
    }

  dst[3] *= alpha;
}

#include "opencl/gegl-cl.h"
#include "opencl/color-to-alpha.cl.h"

static GeglClRunData * cl_data = NULL;

static gboolean
cl_process (GeglOperation       *operation,
            cl_mem              in,
            cl_mem              out,
            size_t              global_worksize,
            const GeglRectangle *roi,
            gint                level)
{
  GeglProperties *o  = GEGL_PROPERTIES (operation);
  gfloat          tt = o->transparency_threshold;
  gfloat          ot = o->opacity_threshold;
  gfloat          color[4];
  gegl_color_get_pixel (o->color, gegl_operation_get_format (operation, "output"), color);

  if (!cl_data)
    {
      const char *kernel_name[] = {"cl_color_to_alpha",NULL};
      cl_data = gegl_cl_compile_and_build (color_to_alpha_cl_source, kernel_name);
    }
  if (!cl_data)
    return TRUE;
  else
  {
    cl_int cl_err = 0;
    cl_float4 f_color;
    f_color.s[0] = color[0];
    f_color.s[1] = color[1];
    f_color.s[2] = color[2];
    f_color.s[3] = color[3];

    cl_err = gegl_clSetKernelArg (cl_data->kernel[0], 0,
                                  sizeof (cl_mem),    (void*) &in);
    CL_CHECK;
    cl_err = gegl_clSetKernelArg (cl_data->kernel[0], 1,
                                  sizeof (cl_mem),    (void*) &out);
    CL_CHECK;
    cl_err = gegl_clSetKernelArg (cl_data->kernel[0], 2,
                                  sizeof (cl_float4), (void*) &f_color);
    CL_CHECK;
    cl_err = gegl_clSetKernelArg (cl_data->kernel[0], 3,
                                  sizeof (float),     (void*) &tt);
    CL_CHECK;
    cl_err = gegl_clSetKernelArg (cl_data->kernel[0], 4,
                                  sizeof (float),     (void*) &ot);
    CL_CHECK;

    cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                         cl_data->kernel[0], 1,
                                         NULL, &global_worksize, NULL,
                                         0, NULL, NULL);
    CL_CHECK;
  }

  return  FALSE;

 error:
  return TRUE;
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o                      = GEGL_PROPERTIES (operation);
  const Babl     *format                 = gegl_operation_get_format (operation, "output");
  gfloat          color[4];
  gfloat          transparency_threshold = o->transparency_threshold;
  gfloat          opacity_threshold      = o->opacity_threshold;
  gint            x;

  gfloat *in_buff = in_buf;
  gfloat *out_buff = out_buf;

  gegl_color_get_pixel (o->color, format, color);

  for (x = 0; x < n_pixels; x++)
    {
      color_to_alpha (color, in_buff, out_buff,
                      transparency_threshold, opacity_threshold);
      in_buff  += 4;
      out_buff += 4;
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *filter_class;
  gchar                         *composition =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "<node operation='svg:dst-over'>"
    "  <node operation='gegl:crop'>"
    "    <params>"
    "      <param name='width'>200.0</param>"
    "      <param name='height'>200.0</param>"
    "    </params>"
    "  </node>"
    "  <node operation='gegl:checkerboard'>"
    "    <params><param name='color1'>rgb(0.5, 0.5, 0.5)</param></params>"
    "  </node>"
    "</node>"
    "<node operation='gegl:color-to-alpha'>"
    "</node>"
    "<node operation='gegl:load'>"
    "  <params>"
    "    <param name='path'>standard-input.png</param>"
    "  </params>"
    "</node>"
    "</gegl>";

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  filter_class->process    = process;
  filter_class->cl_process = cl_process;

  operation_class->prepare = prepare;
  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:color-to-alpha",
    "title",       _("Color to Alpha"),
    "categories",  "color",
    "needs-alpha",  "true", /* hint for GIMP that layers this op is performed on
                               need to have alpha added */
    "license",     "GPL3+",
    "reference-hash", "f110613097308e0fe96ac29f54ca4c2e",
    "description", _("Convert a specified color to transparency, works best with white."),
    "reference-composition", composition,
    NULL);
}

#endif
