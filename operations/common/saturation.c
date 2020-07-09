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
 * Copyright 2016 Red Hat, Inc.
 *           2019 Øyvind Kolås
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_saturation_type)
  enum_value (GEGL_SATURATION_TYPE_NATIVE,  "Native",  N_("Native"))
  enum_value (GEGL_SATURATION_TYPE_CIE_LAB, "CIE-Lab", N_("CIE Lab/Lch"))
  enum_value (GEGL_SATURATION_TYPE_CIE_YUV, "CIE-Yuv", N_("CIE Yuv"))
enum_end (GeglSaturationType)

property_double (scale, _("Scale"), 1.0)
    description(_("Scale, strength of effect"))
    value_range (0.0, 10.0)
    ui_range (0.0, 2.0)

property_enum (colorspace, _("Interpolation Color Space"),
    description(_("Set at Native if uncertain, the CIE based spaces might introduce hue shifts."))
    GeglSaturationType, gegl_saturation_type,
    GEGL_SATURATION_TYPE_NATIVE)

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     saturation
#define GEGL_OP_C_SOURCE saturation.c

#include "gegl-op.h"

typedef void (*ProcessFunc) (GeglOperation       *operation,
                             void                *in_buf,
                             void                *out_buf,
                             glong                n_pixels,
                             const GeglRectangle *roi,
                             gint                 level);

static void
process_lab (GeglOperation       *operation,
             void                *in_buf,
             void                *out_buf,
             glong                n_pixels,
             const GeglRectangle *roi,
             gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  glong i;

  for (i = 0; i < n_pixels; i++)
    {
      out[0] = in[0];
      out[1] = in[1] * o->scale;
      out[2] = in[2] * o->scale;

      in += 3;
      out += 3;
    }
}

static void
process_lab_alpha (GeglOperation       *operation,
                   void                *in_buf,
                   void                *out_buf,
                   glong                n_pixels,
                   const GeglRectangle *roi,
                   gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  glong i;

  for (i = 0; i < n_pixels; i++)
    {
      out[0] = in[0];
      out[1] = in[1] * o->scale;
      out[2] = in[2] * o->scale;
      out[3] = in[3];

      in += 4;
      out += 4;
    }
}

static void
process_lch (GeglOperation       *operation,
             void                *in_buf,
             void                *out_buf,
             glong                n_pixels,
             const GeglRectangle *roi,
             gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  glong i;

  for (i = 0; i < n_pixels; i++)
    {
      out[0] = in[0];
      out[1] = in[1] * o->scale;
      out[2] = in[2];

      in += 3;
      out += 3;
    }
}

static void
process_lch_alpha (GeglOperation       *operation,
                   void                *in_buf,
                   void                *out_buf,
                   glong                n_pixels,
                   const GeglRectangle *roi,
                   gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  glong i;

  for (i = 0; i < n_pixels; i++)
    {
      out[0] = in[0];
      out[1] = in[1] * o->scale;
      out[2] = in[2];
      out[3] = in[3];

      in += 4;
      out += 4;
    }
}


static void
process_cie_yuv_alpha (GeglOperation       *operation,
                       void                *in_buf,
                       void                *out_buf,
                       glong                n_pixels,
                       const GeglRectangle *roi,
                       gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  glong i;
  float scale = o->scale;

#define CIE_u_origin     (4/19.0f)
#define CIE_v_origin     (9/19.0f)

  for (i = 0; i < n_pixels; i++)
    {
      out[0] = in[0];
      out[1] = (in[1] - CIE_u_origin) * scale + CIE_u_origin;
      out[2] = (in[2] - CIE_v_origin) * scale + CIE_v_origin;
      out[3] = in[3];

      in += 4;
      out += 4;
    }
}

static void
process_rgb_alpha (GeglOperation       *operation,
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
  double luminance[3];
  float luminance_f[3];

  babl_space_get_rgb_luminance (space,
    &luminance[0], &luminance[1], &luminance[2]);
  for (int c = 0; c < 3; c ++)
    luminance_f[c] = luminance[c];

  for (i = 0; i < n_pixels; i++)
    {
      gfloat desaturated = (in[0] * luminance_f[0] +
                            in[1] * luminance_f[1] +
                            in[2] * luminance_f[2]) * rscale;
      for (int c = 0; c < 3; c ++)
        out[c] = desaturated + in[c] * scale;
      out[3] = in[3];

      in += 4;
      out += 4;
    }
}

#include <stdio.h>

static void
process_cmyk_alpha (GeglOperation       *operation,
                    void                *in_buf,
                    void                *out_buf,
                    glong                n_pixels,
                    const GeglRectangle *roi,
                    gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  const Babl *in_format = gegl_operation_get_format (operation, "input");
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  glong i;
  float scale = o->scale;
  float rscale = 1.0f - o->scale;
  const Babl *fish1 = babl_fish (in_format, babl_format_with_space ("YA float", space));
  const Babl *fish2 = babl_fish (babl_format_with_space ("YA float", space),
                                 babl_format_with_space ("CMYKA float", space));
  float *grayA = gegl_malloc (n_pixels * 2 * sizeof (float));
  float *cmykA = gegl_malloc (n_pixels * 5 * sizeof (float));
  gfloat *desaturated = cmykA;

  babl_process (fish1, in, grayA, n_pixels);
  babl_process (fish2, grayA, cmykA, n_pixels);
  gegl_free (grayA);

  for (i = 0; i < n_pixels; i++)
    {
      for (int c = 0; c < 4; c ++)
        out[c] = desaturated[c] * rscale + in[c] * scale;
      out[4] = in[4];

      in += 5;
      out += 5;
      desaturated += 5;
    }
  gegl_free (cmykA);
}


static void prepare (GeglOperation *operation)
{
  const Babl *input_format = NULL;
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *format;
  BablModelFlag model_flags;

  input_format = gegl_operation_get_source_format (operation, "input");

  switch (o->colorspace)
  {
    default:
    case GEGL_SATURATION_TYPE_NATIVE:
        format = babl_format_with_space ("RGBA float", space);
        o->user_data = process_rgb_alpha;
        if (input_format)
          {
            model_flags = babl_get_model_flags (input_format);
            if (model_flags & BABL_MODEL_FLAG_CMYK && o->scale < 1.0)
            {
            /* we only use the CMYK code path when desaturating, it provides
               the expected result, and retains the separation - wheras for
               increasing saturation - to achieve expected behavior we need
               to fall back to RGBA to achieve the desired result. */
              format = babl_format_with_space ("CMYKA float", space);
              o->user_data = process_cmyk_alpha;
            }
            else if (model_flags & BABL_MODEL_FLAG_CIE)
            {
              format = babl_format_with_space ("CIE Lab alpha float", space);
              o->user_data = process_lab_alpha;
            }
            /* otherwise we use the RGB default */
          }
      break;
    case GEGL_SATURATION_TYPE_CIE_YUV:
      format = babl_format_with_space ("CIE Yuv alpha float", space);
      o->user_data = process_cie_yuv_alpha;
      break;
    case GEGL_SATURATION_TYPE_CIE_LAB:
      {
        const Babl *lch_model;
        const Babl *input_model;

        if (input_format == NULL)
          {
            format = babl_format_with_space ("CIE Lab alpha float", space);
            o->user_data = process_lab_alpha;
            goto out;
          }

        input_model = babl_format_get_model (input_format);

        if (babl_format_has_alpha (input_format))
          {
            lch_model = babl_model_with_space ("CIE LCH(ab) alpha", space);
            if (input_model == lch_model)
              {
                format = babl_format_with_space ("CIE LCH(ab) alpha float", space);
                o->user_data = process_lch_alpha;
              }
            else
              {
                format = babl_format_with_space ("CIE Lab alpha float", space);
                o->user_data = process_lab_alpha;
              }
          }
        else
          {
            lch_model = babl_model_with_space ("CIE LCH(ab)", space);
            if (input_model == lch_model)
              {
                format = babl_format_with_space ("CIE LCH(ab) float", space);
                o->user_data = process_lch;
              }
            else
              {
                format = babl_format_with_space ("CIE Lab float", space);
                o->user_data = process_lab;
              }
          }
      }
   break;
  }
 out:
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
  ProcessFunc real_process = (ProcessFunc) o->user_data;

  real_process (operation, in_buf, out_buf, n_pixels, roi, level);
  return TRUE;
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
    "    <node operation='gegl:saturation'>"
    "      <params>"
    "        <param name='scale'>2.0</param>"
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

  operation_class->prepare = prepare;
  operation_class->opencl_support = FALSE;

  point_filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:saturation",
    "title",       _("Saturation"),
    "categories" , "color",
    "opi",         "1:0",
    "reference-hash", "c93c29f810f7743c454e3d8171878eee",
    "reference-composition", composition,
    "description", _("Changes the saturation"),
    NULL);
}

#endif
