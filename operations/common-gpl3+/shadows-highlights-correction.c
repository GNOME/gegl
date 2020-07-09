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
 * This operation is a port of the Darktable Shadows Highlights filter
 * copyright (c) 2012--2015 Ulrich Pegelow.
 *
 * GEGL port: Thomas Manni <thomas.manni@free.fr>
 *
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (shadows, _("Shadows"), 0.0)
    description (_("Adjust exposure of shadows"))
    value_range (-100.0, 100.0)

property_double (highlights, _("Highlights"), 0.0)
    description (_("Adjust exposure of highlights"))
    value_range (-100.0, 100.0)

property_double (whitepoint, _("White point adjustment"), 0.0)
    description (_("Shift white point"))
    value_range (-10.0, 10.0)

property_double (compress, _("Compress"), 50.0)
    description (_("Compress the effect on shadows/highlights and preserve midtones"))
    value_range (0.0, 100.0)

property_double (shadows_ccorrect, _("Shadows color adjustment"), 100.0)
    description (_("Adjust saturation of shadows"))
    value_range (0.0, 100.0)

property_double (highlights_ccorrect, _("Highlights color adjustment"), 50.0)
    description (_("Adjust saturation of highlights"))
    value_range (0.0, 100.0)

#else

#define GEGL_OP_POINT_COMPOSER
#define GEGL_OP_NAME     shadows_highlights_correction
#define GEGL_OP_C_SOURCE shadows-highlights-correction.c

#include "gegl-op.h"
#include "gegl-debug.h"

#define SIGN(x) (((x) < 0) ? -1.f : 1.f)

static void
prepare (GeglOperation *operation)
{
  const Babl *space    = gegl_operation_get_source_space (operation, "input");
  const Babl *cie_laba = babl_format_with_space ("CIE Lab alpha float", space);
  const Babl *cie_l    = babl_format_with_space ("CIE L float", space);

  gegl_operation_set_format (operation, "input",  cie_laba);
  gegl_operation_set_format (operation, "aux",    cie_l);
  gegl_operation_set_format (operation, "output", cie_laba);
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

  gfloat shadows;
  gfloat shadows_100 = (gfloat) o->shadows / 100.0f;
  gfloat shadows_sign;
  gfloat highlights;
  gfloat highlights_100 = (gfloat) o->highlights / 100.0f;
  gfloat highlights_sign_negated;
  gfloat whitepoint = 1.0f - (gfloat) o->whitepoint / 100.0f;
  gfloat compress;

  gfloat shadows_ccorrect;
  gfloat shadows_ccorrect_100 = (gfloat) o->shadows_ccorrect / 100.0f;

  gfloat highlights_ccorrect;
  gfloat highlights_ccorrect_100 = (gfloat) o->highlights_ccorrect / 100.0f;

  gfloat low_approximation = 0.01f;

  compress = fminf ((gfloat) o->compress / 100.0f, 0.99f);
  g_return_val_if_fail (compress >= 0.0f, FALSE);

  g_return_val_if_fail (-1.0f <= highlights_100 && highlights_100 <= 1.0f, FALSE);
  highlights = 2.0f * highlights_100;
  highlights_sign_negated = copysignf (1.0f, -highlights);

  g_return_val_if_fail (0.0f <= highlights_ccorrect_100 && highlights_ccorrect_100 <= 1.0f, FALSE);
  highlights_ccorrect = (highlights_ccorrect_100 - 0.5f) * highlights_sign_negated + 0.5f;

  g_return_val_if_fail (-1.0f <= shadows_100 && shadows_100 <= 1.0f, FALSE);
  shadows = 2.0f * shadows_100;
  shadows_sign = copysignf (1.0f, shadows);

  g_return_val_if_fail (0.0f <= shadows_ccorrect_100 && shadows_ccorrect_100 <= 1.0f, FALSE);
  shadows_ccorrect = (shadows_ccorrect_100 - 0.5f) * shadows_sign + 0.5f;

  g_return_val_if_fail (whitepoint >= 0.01f, FALSE);

  if (!aux)
    {
      memcpy (out_buf, in_buf, sizeof (gfloat) * 4 * n_pixels);
      return TRUE;
    }

  while (n_pixels--)
    {
      gfloat ta[3];
      gfloat tb0;

      ta[0] = src[0] / 100.f;
      ta[1] = src[1] / 128.f;
      ta[2] = src[2] / 128.f;

      tb0 = (100.f - aux[0]) / 100.f;

      ta[0] = ta[0] > 0.0f ? ta[0] / whitepoint : ta[0];
      tb0 = tb0 > 0.0f ? tb0 / whitepoint : tb0;

      if (tb0 < 1.0f - compress)
        {
          gfloat highlights2 = highlights * highlights;
          gfloat highlights_xform;

          highlights_xform = fminf(1.0f - tb0 / (1.0f - compress), 1.0f);

          while (highlights2 > 0.0f)
            {
              gfloat lref, href;
              gfloat chunk, optrans;

              gfloat la = ta[0];
              gfloat la_abs;
              gfloat la_inverted = 1.0f - la;
              gfloat la_inverted_abs;
              gfloat lb = (tb0 - 0.5f) * highlights_sign_negated * SIGN(la_inverted) + 0.5f;

              la_abs = fabsf (la);
              lref = copysignf(la_abs > low_approximation ? 1.0f / la_abs : 1.0f / low_approximation, la);

              la_inverted_abs = fabsf (la_inverted);
              href = copysignf(la_inverted_abs > low_approximation ? 1.0f / la_inverted_abs : 1.0f / low_approximation,
                               la_inverted);

              chunk = highlights2 > 1.0f ? 1.0f : highlights2;
              optrans = chunk * highlights_xform;
              highlights2 -= 1.0f;

              ta[0] = la * (1.0 - optrans)
                      + (la > 0.5f ? 1.0f - (1.0f - 2.0f * (la - 0.5f)) * (1.0f - lb) : 2.0f * la * lb) * optrans;

              ta[1] = ta[1] * (1.0f - optrans)
                      + ta[1] * (ta[0] * lref * (1.0f - highlights_ccorrect)
                                 + (1.0f - ta[0]) * href * highlights_ccorrect) * optrans;

              ta[2] = ta[2] * (1.0f - optrans)
                      + ta[2] * (ta[0] * lref * (1.0f - highlights_ccorrect)
                                 + (1.0f - ta[0]) * href * highlights_ccorrect) * optrans;
            }
        }

    if (tb0 > compress)
      {
        gfloat shadows2 = shadows * shadows;
        gfloat shadows_xform;

        shadows_xform = fminf(tb0 / (1.0f - compress) - compress / (1.0f - compress), 1.0f);

        while (shadows2 > 0.0f)
          {
            gfloat lref, href;
            gfloat chunk, optrans;

            gfloat la = ta[0];
            gfloat la_abs;
            gfloat la_inverted = 1.0f - la;
            gfloat la_inverted_abs;
            gfloat lb = (tb0 - 0.5f) * shadows_sign * SIGN(la_inverted) + 0.5f;

            la_abs = fabsf (la);
            lref = copysignf(la_abs > low_approximation ? 1.0f / la_abs : 1.0f / low_approximation, la);

            la_inverted_abs = fabsf (la_inverted);
            href = copysignf(la_inverted_abs > low_approximation ? 1.0f / la_inverted_abs : 1.0f / low_approximation,
                             la_inverted);

            chunk = shadows2 > 1.0f ? 1.0f : shadows2;
            optrans = chunk * shadows_xform;
            shadows2 -= 1.0f;

            ta[0] = la * (1.0 - optrans)
                    + (la > 0.5f ? 1.0f - (1.0f - 2.0f * (la - 0.5f)) * (1.0f - lb) : 2.0f * la * lb) * optrans;

            ta[1] = ta[1] * (1.0f - optrans)
                    + ta[1] * (ta[0] * lref * shadows_ccorrect
                               + (1.0f - ta[0]) * href * (1.0f - shadows_ccorrect)) * optrans;

            ta[2] = ta[2] * (1.0f - optrans)
                    + ta[2] * (ta[0] * lref * shadows_ccorrect
                               + (1.0f - ta[0]) * href * (1.0f - shadows_ccorrect)) * optrans;
          }
      }

      dst[0] = ta[0] * 100.f;
      dst[1] = ta[1] * 128.f;
      dst[2] = ta[2] * 128.f;
      dst[3] = src[3];

      src += 4;
      dst += 4;
      aux += 1;
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

  gfloat shadows;
  gfloat shadows_100 = (gfloat) o->shadows / 100.0f;
  gfloat highlights;
  gfloat highlights_100 = (gfloat) o->highlights / 100.0f;
  gfloat whitepoint = 1.0f - (gfloat) o->whitepoint / 100.f;
  gfloat compress;

  gfloat shadows_ccorrect;
  gfloat shadows_ccorrect_100 = (gfloat) o->shadows_ccorrect / 100.0f;

  gfloat highlights_ccorrect;
  gfloat highlights_ccorrect_100 = (gfloat) o->highlights_ccorrect / 100.0f;

  compress = fminf ((gfloat) o->compress / 100.0f, 0.99f);
  g_return_val_if_fail (compress >= 0.0f, TRUE);

  g_return_val_if_fail (-1.0f <= highlights_100 && highlights_100 <= 1.0f, TRUE);
  highlights = 2.0f * highlights_100;

  g_return_val_if_fail (0.0f <= highlights_ccorrect_100 && highlights_ccorrect_100 <= 1.0f, TRUE);
  highlights_ccorrect = (highlights_ccorrect_100 - 0.5f) * SIGN (-highlights) + 0.5f;

  g_return_val_if_fail (-1.0f <= shadows_100 && shadows_100 <= 1.0f, TRUE);
  shadows = 2.0f * shadows_100;

  g_return_val_if_fail (0.0f <= shadows_ccorrect_100 && shadows_ccorrect_100 <= 1.0f, TRUE);
  shadows_ccorrect = (shadows_ccorrect_100 - 0.5f) * SIGN (shadows) + 0.5f;

  g_return_val_if_fail (whitepoint >= 0.01f, TRUE);

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
    "name",           "gegl:shadows-highlights-correction",
    "categories",     "hidden",
    "license",        "GPL3+",
    "reference-hash", "26edcb1732f29cbbd3ca543c76e57c9b",
    "description", _("Lighten shadows and darken highlights"),
    NULL);
}

#endif
