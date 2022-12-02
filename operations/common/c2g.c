/* STRESS, Spatio Temporal Retinex Envelope with Stochastic Sampling
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
 * Copyright 2007,2009,2018 Øyvind Kolås     <pippin@gimp.org>
 *           2007           Ivar Farup       <ivarf@hig.no>
 *           2007           Allesandro Rizzi <rizzi@dti.unimi.it>
 */


#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_int (radius, _("Radius"), 300)
  description(_("Neighborhood taken into account, this is the radius "
                     "in pixels taken into account when deciding which "
                     "colors map to which gray values"))
  value_range (2, 6000)
  ui_range    (2, 1000)
  ui_gamma    (1.6)
  ui_meta     ("unit", "pixel-distance")

property_int  (samples, _("Samples"), 4)
  description (_("Number of samples to do per iteration looking for the range of colors"))
  value_range (1, 1000)
  ui_range    (3, 17)

property_int (iterations, _("Iterations"), 10)
  description(_("Number of iterations, a higher number of iterations "
                     "provides less noisy results at a computational cost"))
  value_range (1, 1000)
  ui_range (1, 30)

property_boolean (enhance_shadows, _("Enhance Shadows"), FALSE)
    description(_("When enabled details in shadows are boosted at the expense of noise"))

/*
property_double (rgamma, _("Radial Gamma"), 0.0, 8.0, 2.0,
                _("Gamma applied to radial distribution"))
*/
#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     c2g
#define GEGL_OP_C_SOURCE c2g.c

#include "gegl-op.h"
#include <stdlib.h>
#include "envelopes.h"

#define RGAMMA 2.0

static void c2g (GeglOperation       *op,
                 GeglBuffer          *src,
                 const GeglRectangle *src_rect,
                 GeglBuffer          *dst,
                 const GeglRectangle *dst_rect,
                 gint                 radius,
                 gint                 samples,
                 gint                 iterations,
                 gdouble              rgamma,
                 gint                 level)
{
  const Babl *space = babl_format_get_space (gegl_operation_get_format (op, "output"));
  const Babl *format = babl_format_with_space ("RGBA float", space);

  if (dst_rect->width > 0 && dst_rect->height > 0)
  {
    /* XXX: compute total pixels and progress by consumption
     */
    GeglBufferIterator *i = gegl_buffer_iterator_new (dst, dst_rect, 0, babl_format_with_space ("YA float", space),
                                                      GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 1);
    GeglSampler *sampler = gegl_buffer_sampler_new_at_level (src, format, GEGL_SAMPLER_NEAREST, level);
    GeglSamplerGetFun getfun = gegl_sampler_get_fun (sampler);
#if 0
    float total_pix = dst_rect->width * dst_rect->height;
#endif

    while (gegl_buffer_iterator_next (i))
    {
      gint x,y;
      gint    dst_offset=0;
      gfloat *dst_buf = i->items[0].data;

      if (GEGL_PROPERTIES(op)->enhance_shadows)
      {
         GeglRectangle roi = i->items[0].roi;
      for (y=roi.y; y < roi.y + roi.height; y++)
        {
          for (x=roi.x; x < roi.x + roi.width; x++)
            {
              gfloat  min[4];
              gfloat  max[4];
              gfloat  pixel[4];

              compute_envelopes (src, sampler, getfun,
                                 x, y,
                                 radius, samples,
                                 iterations,
                                 FALSE, /* same spray */
                                 rgamma,
                                 min, max, pixel, format);
              {
                /* this should be replaced with a better/faster projection of
                 * pixel onto the vector spanned by min -> max, currently
                 * computed by comparing the distance to min with the sum
                 * of the distance to min/max.
                 */

                gfloat nominator = 0;
                gfloat denominator = 0;
                gint c;
                for (c=0; c<3; c++)
                  {
                    nominator   += (pixel[c] - min[c]) * (pixel[c] - min[c]);
                    denominator += (pixel[c] - max[c]) * (pixel[c] - max[c]);
                  }

                nominator = sqrtf (nominator);
                denominator = sqrtf (denominator);
                denominator = nominator + denominator;

                if (denominator>0.000)
                  {
                    dst_buf[dst_offset+0] = nominator/denominator;
                  }
                else
                  {
                    /* shouldn't happen */
                    dst_buf[dst_offset+0] = 0.5;
                  }
                dst_buf[dst_offset+1] = pixel[3];
                dst_offset+=2;
              }
            }
          }
      }
      else
      {
         GeglRectangle roi = i->items[0].roi;
        for (y=roi.y; y < roi.y + roi.height; y++)
          for (x=roi.x; x < roi.x + roi.width; x++)
            {
              gfloat  max[4];
              gfloat  pixel[4];

              compute_envelopes (src, sampler, getfun,
                                 x, y,
                                 radius, samples,
                                 iterations,
                                 FALSE, /* same spray */
                                 rgamma,
                                 NULL, max, pixel, format);
              {
                /* this should be replaced with a better/faster projection of
                 * pixel onto the vector spanned by min -> max, currently
                 * computed by comparing the distance to min with the sum
                 * of the distance to min/max.
                 */

                gfloat nominator = 0;
                gfloat denominator = 0;
                gint c;
                for (c=0; c<3; c++)
                  {
                    nominator   += pixel[c] * pixel[c];
                    denominator += (pixel[c] - max[c]) * (pixel[c] - max[c]);
                  }

                nominator = sqrtf (nominator);
                denominator = sqrtf (denominator);
                denominator = nominator + denominator;

                if (denominator>0.000)
                  {
                    dst_buf[dst_offset+0] = nominator/denominator;
                  }
                else
                  {
                    /* shouldn't happen */
                    dst_buf[dst_offset+0] = 0.5;
                  }
                dst_buf[dst_offset+1] = pixel[3];
                dst_offset+=2;
              }
            }
          }
      }
    g_object_unref (sampler);
  }
}

static void prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  const Babl *format_rgba = babl_format_with_space ("RGBA float", space);
  const Babl *format_ya = babl_format_with_space ("YA float", space);

  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  area->left = area->right = area->top = area->bottom =
      ceil (GEGL_PROPERTIES (operation)->radius);

  gegl_operation_set_format (operation, "input", format_rgba);
  gegl_operation_set_format (operation, "output", format_ya);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation,
                                                                     "input");
  if (!in_rect)
    return result;
  return *in_rect;
}

#include "opencl/gegl-cl.h"
#include "gegl-buffer-cl-iterator.h"

#include "opencl/c2g.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
cl_c2g (cl_mem                in_tex,
        cl_mem                out_tex,
        size_t                global_worksize,
        const GeglRectangle  *src_roi,
        const GeglRectangle  *roi,
        gint                  radius,
        gint                  samples,
        gint                  iterations,
        gdouble               rgamma)
{
  cl_int cl_err = 0;
  cl_mem cl_lut_cos = NULL;
  cl_mem cl_lut_sin = NULL;
  cl_mem cl_radiuses = NULL;
  const size_t gbl_size[2] = {roi->width, roi->height};

  if (!cl_data)
    {
      const char *kernel_name[] ={"c2g", NULL};
      cl_data = gegl_cl_compile_and_build(c2g_cl_source, kernel_name);
    }
  if (!cl_data) return TRUE;

  compute_luts(rgamma);

  cl_lut_cos = gegl_clCreateBuffer(gegl_cl_get_context(),
                                   CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                   ANGLE_PRIME * sizeof(cl_float), lut_cos, &cl_err);
  CL_CHECK;

  cl_lut_sin = gegl_clCreateBuffer(gegl_cl_get_context(),
                                   CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                   ANGLE_PRIME * sizeof(cl_float), lut_sin, &cl_err);

  cl_radiuses = gegl_clCreateBuffer(gegl_cl_get_context(),
                                    CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                                    RADIUS_PRIME * sizeof(cl_float), radiuses, &cl_err);
  CL_CHECK;

  {
  cl_int cl_src_width  = src_roi->width;
  cl_int cl_src_height = src_roi->height;
  cl_int cl_radius     = radius;
  cl_int cl_samples    = samples;
  cl_int cl_iterations = iterations;

  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem), (void*)&in_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_int), (void*)&cl_src_width);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 2, sizeof(cl_int), (void*)&cl_src_height);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 3, sizeof(cl_mem), (void*)&cl_radiuses);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 4, sizeof(cl_mem), (void*)&cl_lut_cos);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 5, sizeof(cl_mem), (void*)&cl_lut_sin);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 6, sizeof(cl_mem), (void*)&out_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 7, sizeof(cl_int), (void*)&cl_radius);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 8, sizeof(cl_int), (void*)&cl_samples);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 9, sizeof(cl_int), (void*)&cl_iterations);
  CL_CHECK;
  }

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue(), cl_data->kernel[0],
                                       2, NULL, gbl_size, NULL,
                                       0, NULL, NULL);
  CL_CHECK;

  cl_err = gegl_clFinish(gegl_cl_get_command_queue ());
  CL_CHECK;

  cl_err = gegl_clReleaseMemObject (cl_radiuses);
  CL_CHECK_ONLY (cl_err);
  cl_err = gegl_clReleaseMemObject (cl_lut_cos);
  CL_CHECK_ONLY (cl_err);
  cl_err = gegl_clReleaseMemObject (cl_lut_sin);
  CL_CHECK_ONLY (cl_err);

  return FALSE;

error:
  if (cl_radiuses)
    gegl_clReleaseMemObject (cl_radiuses);
  if (cl_lut_cos)
    gegl_clReleaseMemObject (cl_lut_cos);
  if (cl_lut_sin)
    gegl_clReleaseMemObject (cl_lut_sin);

  return TRUE;
}

static gboolean
cl_process (GeglOperation       *operation,
            GeglBuffer          *input,
            GeglBuffer          *output,
            const GeglRectangle *result)
{
  const Babl *out_format = gegl_operation_get_format (operation, "output");
  const Babl *in_format  = babl_format_with_space ("RGBA float", babl_format_get_space (out_format));
  gint err;

  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties *o = GEGL_PROPERTIES (operation);

  GeglBufferClIterator *i = gegl_buffer_cl_iterator_new (output,
                                                         result,
                                                         out_format,
                                                         GEGL_CL_BUFFER_WRITE);

  gint read = gegl_buffer_cl_iterator_add_2 (i,
                                             input,
                                             result,
                                             in_format,
                                             GEGL_CL_BUFFER_READ,
                                             op_area->left,
                                             op_area->right,
                                             op_area->top,
                                             op_area->bottom,
                                             GEGL_ABYSS_NONE);

  while (gegl_buffer_cl_iterator_next (i, &err))
    {
      if (err) return FALSE;

      err = cl_c2g(i->tex[read],
                   i->tex[0],
                   i->size[0],
                   &i->roi[read],
                   &i->roi[0],
                   o->radius,
                   o->samples,
                   o->iterations,
                   RGAMMA);

      if (err) return FALSE;
    }

  return TRUE;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle compute;
  compute = gegl_operation_get_required_for_output (operation, "input",result);

  if (o->radius < 500 && gegl_operation_use_opencl (operation))
    if(cl_process(operation, input, output, result))
      return TRUE;

  c2g (operation, input, &compute, output, result,
       o->radius,
       o->samples,
       o->iterations,
       /*o->rgamma*/RGAMMA,
       level);

  return  TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;
  gchar                    *composition = 
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "  <node operation='gegl:crop' width='200' height='200'/>"
    "  <node operation='gegl:over'>"
    "    <node operation='gegl:c2g'>"
    "      <params>"
    "        <param name='radius'>200</param>"
    "        <param name='iterations'>90</param>"
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

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process    = process;
  operation_class->prepare = prepare;

  /* we override defined region to avoid growing the size of what is defined
   * by the filter. This also allows the tricks used to treat alpha==0 pixels
   * in the image as source data not to be skipped by the stochastic sampling
   * yielding correct edge behavior.
   */
  operation_class->get_bounding_box = get_bounding_box;

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name",                  "gegl:c2g",
    "categories",            "grayscale:color",
    "title",                 "Color to Grayscale",
    /* Reference hash is not consistent from run to run
    "reference-hash",        "992ec85d36a5ad598a2533c4593d89d3",
    */
    "reference-hash",        "unstable",
    "reference-composition", composition,
    "description",
    _("Color to grayscale conversion, uses envelopes formed with the STRESS approach "
      "to perform local color-difference preserving grayscale generation."),
    NULL);
}

#endif
