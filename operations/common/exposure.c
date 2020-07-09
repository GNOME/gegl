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
 * Copyright 2012,2013 Felix Ulber <felix.ulber@gmx.de>
 *           2013 Øyvind Kolås <pippin@gimp.org>
 *           2017 Red Hat, Inc.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (black_level, _("Black level"), 0.0)
    description (_("Adjust the black level"))
    value_range (-0.1, 0.1)

property_double (exposure, _("Exposure"), 0.0)
    description (_("Relative brightness change in stops"))
    ui_range    (-10.0, 10.0)

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     exposure
#define GEGL_OP_C_SOURCE exposure.c

#include "gegl-op.h"
#include "opencl/gegl-cl.h"

#ifdef _MSC_VER
#define exp2f (b) ((gfloat) pow (2.0, b))
#endif

typedef void (*ProcessFunc) (GeglOperation       *operation,
                             void                *in_buf,
                             void                *out_buf,
                             glong                n_pixels,
                             const GeglRectangle *roi,
                             gint                 level);

typedef struct
{
  GeglClRunData **cl_data_ptr;
  ProcessFunc process;
  const char *kernel_name;
  const char *kernel_source;
} EParamsType;

static GeglClRunData *cl_data_rgb = NULL;
static GeglClRunData *cl_data_rgba = NULL;
static GeglClRunData *cl_data_y = NULL;
static GeglClRunData *cl_data_ya = NULL;

static const char* kernel_source_rgb =
"__kernel void kernel_exposure_rgb(__global const float *in,           \n"
"                                  __global       float *out,          \n"
"                                  float                 black_level,  \n"
"                                  float                 gain)         \n"
"{                                                                     \n"
"  int gid = get_global_id(0);                                         \n"
"  int offset  = 3 * gid;                                              \n"
"  float3 in_v = (float3) (in[offset], in[offset + 1], in[offset+2]);  \n"
"  float3 out_v;                                                       \n"
"  out_v.xyz =  ((in_v.xyz - black_level) * gain);                     \n"
"  out[offset]     = out_v.x;                                          \n"
"  out[offset + 1] = out_v.y;                                          \n"
"  out[offset + 2] = out_v.z;                                          \n"
"}                                                                     \n";

static const char* kernel_source_rgba =
"__kernel void kernel_exposure_rgba(__global const float4 *in,          \n"
"                                   __global       float4 *out,         \n"
"                                   float                  black_level, \n"
"                                   float                  gain)        \n"
"{                                                                      \n"
"  int gid = get_global_id(0);                                          \n"
"  float4 in_v  = in[gid];                                              \n"
"  float4 out_v;                                                        \n"
"  out_v.xyz =  ((in_v.xyz - black_level) * gain);                      \n"
"  out_v.w   =  in_v.w;                                                 \n"
"  out[gid]  =  out_v;                                                  \n"
"}                                                                      \n";

static const char* kernel_source_y =
"__kernel void kernel_exposure_y(__global const float *in,             \n"
"                                __global       float *out,            \n"
"                                float                 black_level,    \n"
"                                float                 gain)           \n"
"{                                                                     \n"
"  int gid = get_global_id(0);                                         \n"
"  float in_v  = in[gid];                                              \n"
"  float out_v;                                                        \n"
"  out_v     =  ((in_v - black_level) * gain);                         \n"
"  out[gid]  =  out_v;                                                 \n"
"}                                                                     \n";

static const char* kernel_source_ya =
"__kernel void kernel_exposure_ya(__global const float2 *in,             \n"
"                                 __global       float2 *out,            \n"
"                                 float                  black_level,    \n"
"                                 float                  gain)           \n"
"{                                                                       \n"
"  int gid = get_global_id(0);                                           \n"
"  float2 in_v  = in[gid];                                               \n"
"  float2 out_v;                                                         \n"
"  out_v.x   =  ((in_v.x - black_level) * gain);                         \n"
"  out_v.y   =  in_v.y;                                                  \n"
"  out[gid]  =  out_v;                                                   \n"
"}                                                                       \n";

static void
process_rgb (GeglOperation       *op,
             void                *in_buf,
             void                *out_buf,
             glong                n_pixels,
             const GeglRectangle *roi,
             gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (op);
  gfloat     *in_pixel;
  gfloat     *out_pixel;
  gfloat      black_level = (gfloat) o->black_level;
  gfloat      diff;
  gfloat      exposure_negated = (gfloat) -o->exposure;
  gfloat      gain;
  gfloat      white;

  glong       i;

  in_pixel = in_buf;
  out_pixel = out_buf;

  white = exp2f (exposure_negated);
  diff = MAX (white - black_level, 0.000001);
  gain = 1.0f / diff;

  for (i=0; i<n_pixels; i++)
    {
      out_pixel[0] = (in_pixel[0] - black_level) * gain;
      out_pixel[1] = (in_pixel[1] - black_level) * gain;
      out_pixel[2] = (in_pixel[2] - black_level) * gain;

      out_pixel += 3;
      in_pixel  += 3;
    }
}

static void
process_rgba (GeglOperation       *op,
              void                *in_buf,
              void                *out_buf,
              glong                n_pixels,
              const GeglRectangle *roi,
              gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (op);
  gfloat     *in_pixel;
  gfloat     *out_pixel;
  gfloat      black_level = (gfloat) o->black_level;
  gfloat      diff;
  gfloat      exposure_negated = (gfloat) -o->exposure;
  gfloat      gain;
  gfloat      white;
  
  glong       i;

  in_pixel = in_buf;
  out_pixel = out_buf;
  
  white = exp2f (exposure_negated);
  diff = MAX (white - black_level, 0.000001);
  gain = 1.0f / diff;

  for (i=0; i<n_pixels; i++)
    {
      out_pixel[0] = (in_pixel[0] - black_level) * gain;
      out_pixel[1] = (in_pixel[1] - black_level) * gain;
      out_pixel[2] = (in_pixel[2] - black_level) * gain;
      out_pixel[3] = in_pixel[3];
      
      out_pixel += 4;
      in_pixel  += 4;
    }
}

static void
process_y (GeglOperation       *op,
           void                *in_buf,
           void                *out_buf,
           glong                n_pixels,
           const GeglRectangle *roi,
           gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (op);
  gfloat     *in_pixel;
  gfloat     *out_pixel;
  gfloat      black_level = (gfloat) o->black_level;
  gfloat      diff;
  gfloat      exposure_negated = (gfloat) -o->exposure;
  gfloat      gain;
  gfloat      white;

  glong       i;

  in_pixel = in_buf;
  out_pixel = out_buf;

  white = exp2f (exposure_negated);
  diff = MAX (white - black_level, 0.000001);
  gain = 1.0f / diff;

  for (i=0; i<n_pixels; i++)
    {
      out_pixel[0] = (in_pixel[0] - black_level) * gain;

      out_pixel++;
      in_pixel++;
    }
}

static void
process_ya (GeglOperation       *op,
            void                *in_buf,
            void                *out_buf,
            glong                n_pixels,
            const GeglRectangle *roi,
            gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (op);
  gfloat     *in_pixel;
  gfloat     *out_pixel;
  gfloat      black_level = (gfloat) o->black_level;
  gfloat      diff;
  gfloat      exposure_negated = (gfloat) -o->exposure;
  gfloat      gain;
  gfloat      white;

  glong       i;

  in_pixel = in_buf;
  out_pixel = out_buf;

  white = exp2f (exposure_negated);
  diff = MAX (white - black_level, 0.000001);
  gain = 1.0f / diff;

  for (i=0; i<n_pixels; i++)
    {
      out_pixel[0] = (in_pixel[0] - black_level) * gain;
      out_pixel[1] = in_pixel[1];

      out_pixel += 2;
      in_pixel  += 2;
    }
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  EParamsType *params;
  const Babl *format;
  const Babl *input_format;
  const Babl *input_model;
  const Babl *y_model;

  if (o->user_data == NULL)
    o->user_data = g_slice_new0 (EParamsType);

  params = (EParamsType *) o->user_data;

  input_format = gegl_operation_get_source_format (operation, "input");
  if (input_format == NULL)
    {
      format = babl_format ("RGBA float");

      params->process = process_rgba;

      params->cl_data_ptr = &cl_data_rgba;
      params->kernel_name = "kernel_exposure_rgba";
      params->kernel_source = kernel_source_rgba;
      goto out;
    }

  input_model = babl_format_get_model (input_format);

  if (babl_format_has_alpha (input_format))
    {
      y_model = babl_model_with_space ("YA", space);
      if (input_model == y_model)
        {
          format = babl_format_with_space ("YA float", space);

          params->process = process_ya;

          params->cl_data_ptr = &cl_data_ya;
          params->kernel_name = "kernel_exposure_ya";
          params->kernel_source = kernel_source_ya;
        }
      else
        {
          format = babl_format_with_space ("RGBA float", space);

          params->process = process_rgba;

          params->cl_data_ptr = &cl_data_rgba;
          params->kernel_name = "kernel_exposure_rgba";
          params->kernel_source = kernel_source_rgba;
        }
    }
  else
    {
      y_model = babl_model_with_space ("Y", space);
      if (input_model == y_model)
        {
          format = babl_format_with_space ("Y float", space);

          params->process = process_y;

          params->cl_data_ptr = &cl_data_y;
          params->kernel_name = "kernel_exposure_y";
          params->kernel_source = kernel_source_y;
        }
      else
        {
          format = babl_format_with_space ("RGB float", space);

          params->process = process_rgb;

          params->cl_data_ptr = &cl_data_rgb;
          params->kernel_name = "kernel_exposure_rgb";
          params->kernel_source = kernel_source_rgb;
        }
    }

 out:
  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

/* GeglOperationPointFilter gives us a linear buffer to operate on
 * in our requested pixel format
 */
static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  EParamsType *params = (EParamsType *) o->user_data;

  params->process (operation, in_buf, out_buf, n_pixels, roi, level);
  return TRUE;
}

/* OpenCL processing function */
static cl_int
cl_process (GeglOperation       *op,
            cl_mem               in_tex,
            cl_mem               out_tex,
            size_t               global_worksize,
            const GeglRectangle *roi,
            gint                 level)
{
  /* Retrieve a pointer to GeglProperties structure which contains all the
   * chanted properties
   */

  GeglProperties *o = GEGL_PROPERTIES (op);
  EParamsType *params = (EParamsType *) o->user_data;

  gfloat      black_level = (gfloat) o->black_level;
  gfloat      diff;
  gfloat      exposure_negated = (gfloat) -o->exposure;
  gfloat      gain;
  gfloat      white;
  
  GeglClRunData *cl_data_local;
  cl_int cl_err = 0;

  if (*params->cl_data_ptr == NULL)
    {
      const char *kernel_name[] = {NULL, NULL};

      kernel_name[0] = params->kernel_name;
      *params->cl_data_ptr = gegl_cl_compile_and_build (params->kernel_source, kernel_name);
    }
  if (*params->cl_data_ptr == NULL) return 1;

  cl_data_local = *params->cl_data_ptr;

  white = exp2f (exposure_negated);
  diff = MAX (white - black_level, 0.000001);
  gain = 1.0f / diff;

  cl_err |= gegl_clSetKernelArg(cl_data_local->kernel[0], 0, sizeof(cl_mem),   (void*)&in_tex);
  cl_err |= gegl_clSetKernelArg(cl_data_local->kernel[0], 1, sizeof(cl_mem),   (void*)&out_tex);
  cl_err |= gegl_clSetKernelArg(cl_data_local->kernel[0], 2, sizeof(cl_float), (void*)&black_level);
  cl_err |= gegl_clSetKernelArg(cl_data_local->kernel[0], 3, sizeof(cl_float), (void*)&gain);
  if (cl_err != CL_SUCCESS) return cl_err;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                        cl_data_local->kernel[0], 1,
                                        NULL, &global_worksize, NULL,
                                        0, NULL, NULL);
  if (cl_err != CL_SUCCESS) return cl_err;

  return cl_err;
}

static void
finalize (GObject *object)
{
  GeglOperation *op = GEGL_OPERATION (object);
  GeglProperties *o = GEGL_PROPERTIES (op);

  if (o->user_data)
    g_slice_free (EParamsType, o->user_data);

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass                  *object_class;
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;
  gchar                         *composition =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "  <node operation='gegl:crop' width='200' height='200'/>"
    "  <node operation='gegl:over'>"
    "    <node operation='gegl:exposure'>"
    "      <params>"
    "        <param name='exposure'>1.5</param>"
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

  object_class       = G_OBJECT_CLASS (klass);
  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->finalize = finalize;

  operation_class->opencl_support = TRUE;
  operation_class->prepare        = prepare;

  point_filter_class->process    = process;
  point_filter_class->cl_process = cl_process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:exposure",
    "title",       _("Exposure"),
    "categories",  "color",
    "reference-hash", "a4ae5d7f933046aa462e0f7659bd1261",
    "reference-composition", composition,
    "description", _("Change exposure of an image in shutter speed stops"),
    "op-version",  "1:0",
    NULL);
}

#endif
