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
 * This operation is a port of the Darktable Shadows Highlights filter
 * copyright (c) 2012--2015 Ulrich Pegelow.
 *
 * GEGL port: Thomas Manni <thomas.manni@free.fr>
 *
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (shadows, _("Shadows"), 50.0)
    value_range (-100.0, 100.0)

property_double (highlights, _("Highlights"), -50.0)
    value_range (-100.0, 100.0)

property_double (whitepoint, _("White point adjustment"), 0.0)
    value_range (-10.0, 10.0)

property_double (compress, _("Compress"), 50.0)
    value_range (0.0, 100.0)

property_double (shadows_ccorrect, _("Shadows color adjustment"), 100.0)
    value_range (0.0, 100.0)

property_double (highlights_ccorrect, _("Highlights color adjustment"), 50.0)
    value_range (0.0, 100.0)

#else

#define GEGL_OP_POINT_COMPOSER
#define GEGL_OP_NAME     shadows_highlights_correction
#define GEGL_OP_C_SOURCE shadows-highlights-correction.c

#include "gegl-op.h"
#include "gegl-debug.h"
#include <math.h>

#define SIGN(x) (((x) < 0) ? -1.f : 1.f)

static void
prepare (GeglOperation *operation)
{
  const Babl *laba = babl_format ("CIE Lab alpha float");
  const Babl *lab  = babl_format ("CIE Lab float");

  gegl_operation_set_format (operation, "input",  laba);
  gegl_operation_set_format (operation, "aux",    lab);
  gegl_operation_set_format (operation, "output", laba);
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *aux_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  gfloat *src  = in_buf;
  gfloat *dst  = out_buf;
  gfloat *aux  = aux_buf;

  gfloat shadows    = 2.f * fminf (fmaxf (-1.0, (o->shadows / 100.f)), 1.f);
  gfloat highlights = 2.f * fminf (fmaxf (-1.0, (o->highlights / 100.f)), 1.f);
  gfloat whitepoint = fmaxf (1.f - o->whitepoint / 100.f, 0.01f);
  gfloat compress   = fminf (fmaxf (0, (o->compress / 100.f)), 0.99f);

  gfloat shadows_ccorrect = (fminf (fmaxf (0.0f, (o->shadows_ccorrect / 100.f)), 1.f) - 0.5f)
                             * SIGN(shadows) + 0.5f;

  gfloat highlights_ccorrect = (fminf (fmaxf (0.0f, (o->highlights_ccorrect / 100.f)), 1.f) - 0.5f)
                                * SIGN(-highlights) + 0.5f;

  gfloat max[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
  gfloat min[4] = { 0.0f, -1.0f, -1.0f, 0.0f };
  gfloat lmax = max[0] + fabs(min[0]);
  gfloat halfmax = lmax / 2.0;
  gfloat doublemax = lmax * 2.0;
  gfloat low_approximation = 0.01f;

  if (!aux)
    {
      memcpy (out_buf, in_buf, sizeof (gfloat) * 4 * n_pixels);
      return TRUE;
    }

  while (n_pixels--)
    {
      gfloat ta[3];
      gfloat tb[3];
      gfloat highlights2;
      gfloat highlights_xform;
      gfloat shadows2;
      gfloat shadows_xform;

      ta[0] = src[0] / 100.f;
      ta[1] = src[1] / 128.f;
      ta[2] = src[2] / 128.f;

      tb[0] = (100.f - aux[0]) / 100.f;
      tb[1] = 0.f;
      tb[2] = 0.f;

      ta[0] = ta[0] > 0.0f ? ta[0] / whitepoint : ta[0];
      tb[0] = tb[0] > 0.0f ? tb[0] / whitepoint : tb[0];

      highlights2 = highlights * highlights;
      highlights_xform = CLAMP(1.0f - tb[0] / (1.0f - compress), 0.0f, 1.0f);

      while (highlights2 > 0.0f)
        {
          gfloat lref, href;
          gfloat chunk, optrans;

          gfloat la = ta[0];
          gfloat lb = (tb[0] - halfmax) * SIGN(-highlights) * SIGN(lmax - la) + halfmax;

          lref = copysignf(fabs(la) > low_approximation ? 1.0f / fabs(la) : 1.0f / low_approximation, la);
          href = copysignf(
              fabs(1.0f - la) > low_approximation ? 1.0f / fabs(1.0f - la) : 1.0f / low_approximation, 1.0f - la);

          chunk = highlights2 > 1.0f ? 1.0f : highlights2;
          optrans = chunk * highlights_xform;
          highlights2 -= 1.0f;

          ta[0] = la * (1.0 - optrans)
                  + (la > halfmax ? lmax - (lmax - doublemax * (la - halfmax)) * (lmax - lb) : doublemax * la
                                                                                               * lb) * optrans;

          ta[1] = ta[1] * (1.0f - optrans)
                  + (ta[1] + tb[1]) * (ta[0] * lref * (1.0f - highlights_ccorrect)
                                       + (1.0f - ta[0]) * href * highlights_ccorrect) * optrans;

          ta[2] = ta[2] * (1.0f - optrans)
                  + (ta[2] + tb[2]) * (ta[0] * lref * (1.0f - highlights_ccorrect)
                                       + (1.0f - ta[0]) * href * highlights_ccorrect) * optrans;
        }

    shadows2 = shadows * shadows;
    shadows_xform = CLAMP(tb[0] / (1.0f - compress) - compress / (1.0f - compress), 0.0f, 1.0f);

    while (shadows2 > 0.0f)
      {
        gfloat lref, href;
        gfloat chunk, optrans;

        gfloat la = ta[0];
        gfloat lb = (tb[0] - halfmax) * SIGN(shadows) * SIGN(lmax - la) + halfmax;
        lref = copysignf(fabs(la) > low_approximation ? 1.0f / fabs(la) : 1.0f / low_approximation, la);
        href = copysignf(
            fabs(1.0f - la) > low_approximation ? 1.0f / fabs(1.0f - la) : 1.0f / low_approximation, 1.0f - la);

        chunk = shadows2 > 1.0f ? 1.0f : shadows2;
        optrans = chunk * shadows_xform;
        shadows2 -= 1.0f;

        ta[0] = la * (1.0 - optrans)
                + (la > halfmax ? lmax - (lmax - doublemax * (la - halfmax)) * (lmax - lb) : doublemax * la
                                                                                             * lb) * optrans;

        ta[1] = ta[1] * (1.0f - optrans)
                + (ta[1] + tb[1]) * (ta[0] * lref * shadows_ccorrect
                                     + (1.0f - ta[0]) * href * (1.0f - shadows_ccorrect)) * optrans;

        ta[2] = ta[2] * (1.0f - optrans)
                + (ta[2] + tb[2]) * (ta[0] * lref * shadows_ccorrect
                                     + (1.0f - ta[0]) * href * (1.0f - shadows_ccorrect)) * optrans;
      }

      dst[0] = ta[0] * 100.f;
      dst[1] = ta[1] * 128.f;
      dst[2] = ta[2] * 128.f;
      dst[3] = src[3];

      src += 4;
      dst += 4;
      aux += 3;
    }

  return TRUE;
}

#include "opencl/gegl-cl.h"

#include "opencl/shadows-highlights-correction.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
cl_process (GeglOperation       *op,
            cl_mem               in_tex,
            cl_mem               aux_tex,
            cl_mem               out_tex,
            size_t               global_worksize,
            const GeglRectangle *roi,
            gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (op);

  cl_int cl_err = 0;

  gfloat shadows    = 2.f * fminf (fmaxf (-1.0, (o->shadows / 100.f)), 1.f);
  gfloat highlights = 2.f * fminf (fmaxf (-1.0, (o->highlights / 100.f)), 1.f);
  gfloat whitepoint = fmaxf (1.f - o->whitepoint / 100.f, 0.01f);
  gfloat compress   = fminf (fmaxf (0, (o->compress / 100.f)), 0.99f);

  gfloat shadows_ccorrect = (fminf (fmaxf (0.0f, (o->shadows_ccorrect / 100.f)), 1.f) - 0.5f)
                             * SIGN(shadows) + 0.5f;

  gfloat highlights_ccorrect = (fminf (fmaxf (0.0f, (o->highlights_ccorrect / 100.f)), 1.f) - 0.5f)
                                * SIGN(-highlights) + 0.5f;

  if (!cl_data)
    {
      const char *kernel_name[] = {"shadows_highlights", NULL};
      cl_data = gegl_cl_compile_and_build (shadows_highlights_correction_cl_source, kernel_name);
    }
  if (!cl_data) return TRUE;

  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem),   (void*)&in_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_mem),   (aux_tex)? (void*)&aux_tex : NULL);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 2, sizeof(cl_mem),   (void*)&out_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 3, sizeof(cl_float), (void*)&shadows);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 4, sizeof(cl_float), (void*)&highlights);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 5, sizeof(cl_float), (void*)&compress);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 6, sizeof(cl_float), (void*)&shadows_ccorrect);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 7, sizeof(cl_float), (void*)&highlights_ccorrect);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 8, sizeof(cl_float), (void*)&whitepoint);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                        cl_data->kernel[0], 1,
                                        NULL, &global_worksize, NULL,
                                        0, NULL, NULL);
  CL_CHECK;

  return FALSE;

error:
  return TRUE;
}

static GeglRectangle
get_bounding_box (GeglOperation *self)
{
  GeglRectangle  result  = { 0, 0, 0, 0 };
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (self, "input");

  if (! in_rect)
    return result;

  return *in_rect;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass              *operation_class;
  GeglOperationPointComposerClass *point_composer_class;

  operation_class      = GEGL_OPERATION_CLASS (klass);
  point_composer_class = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  operation_class->prepare          = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->opencl_support   = TRUE;

  point_composer_class->process     = process;
  point_composer_class->cl_process  = cl_process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:shadows-highlights-correction",
    "categories",  "hidden",
    "license",     "GPL3+",
    "description", _("Lighten shadows and darken highlights"),
    NULL);
}

#endif
