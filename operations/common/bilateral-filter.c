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
 * Copyright 2005 Øyvind Kolås <pippin@gimp.org>,
 *           2007 Øyvind Kolås <oeyvindk@hig.no>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_double (blur_radius, _("Blur radius"), 4.0)
  description(_("Radius of square pixel region, (width and height will be radius*2+1)."))
  value_range   (0.0, 1000.0)
  ui_range      (0.0, 100.0)
  ui_gamma      (1.5)

property_double (edge_preservation, _("Edge preservation"), 8.0)
  description   (_("Amount of edge preservation"))
  value_range   (0.0, 100.0)

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     bilateral_filter
#define GEGL_OP_C_SOURCE bilateral-filter.c

#include "gegl-op.h"

static void
bilateral_filter (GeglBuffer          *src,
                  const GeglRectangle *src_rect,
                  GeglBuffer          *dst,
                  const GeglRectangle *dst_rect,
                  gdouble              radius,
                  gdouble              preserve,
                  const Babl          *format);

#include <stdio.h>

static void prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  const Babl *format = babl_format_with_space ("RGBA float", space);
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties              *o = GEGL_PROPERTIES (operation);

  area->left = area->right = area->top = area->bottom = ceil (o->blur_radius);
  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

#include "opencl/gegl-cl.h"
#include "gegl-buffer-cl-iterator.h"

#include "opencl/bilateral-filter.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
cl_bilateral_filter (cl_mem                in_tex,
                     cl_mem                out_tex,
                     size_t                global_worksize,
                     const GeglRectangle  *roi,
                     gfloat                radius,
                     gfloat                preserve)
{
  cl_int cl_err = 0;
  size_t global_ws[2];

  if (!cl_data)
    {
      const char *kernel_name[] = {"bilateral_filter", NULL};
      cl_data = gegl_cl_compile_and_build (bilateral_filter_cl_source, kernel_name);
    }
  if (!cl_data) return TRUE;

  global_ws[0] = roi->width;
  global_ws[1] = roi->height;

  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem),   (void*)&in_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_mem),   (void*)&out_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 2, sizeof(cl_float), (void*)&radius);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 3, sizeof(cl_float), (void*)&preserve);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                       cl_data->kernel[0], 2,
                                       NULL, global_ws, NULL,
                                       0, NULL, NULL);
  CL_CHECK;

  return FALSE;

error:
  return TRUE;
}

static gboolean
cl_process (GeglOperation       *operation,
            GeglBuffer          *input,
            GeglBuffer          *output,
            const GeglRectangle *result)
{
  const Babl *in_format  = gegl_operation_get_format (operation, "input");
  const Babl *out_format = gegl_operation_get_format (operation, "output");
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

      err = cl_bilateral_filter(i->tex[read],
                                i->tex[0],
                                i->size[0],
                                &i->roi[0],
                                ceil(o->blur_radius),
                                o->edge_preservation);

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
  const Babl *format = gegl_operation_get_format (operation, "output");

  if (o->blur_radius >= 1.0 && gegl_operation_use_opencl (operation))
    if (cl_process (operation, input, output, result))
      return TRUE;

  compute = gegl_operation_get_required_for_output (operation, "input",result);

  if (o->blur_radius < 1.0)
    {
      gegl_buffer_copy (input, result, GEGL_ABYSS_NONE,
                        output, result);
    }
  else
    {
      bilateral_filter (input, &compute, output, result, o->blur_radius, o->edge_preservation, format);
    }

  return  TRUE;
}

static void
bilateral_filter (GeglBuffer          *src,
                  const GeglRectangle *src_rect,
                  GeglBuffer          *dst,
                  const GeglRectangle *dst_rect,
                  gdouble              radius,
                  gdouble              preserve,
                  const Babl          *format)
{
  gfloat *gauss;
  gint x,y;
  gint offset;
  gfloat *src_buf;
  gfloat *dst_buf;
  gint width = (gint) radius * 2 + 1;
  gint iradius = radius;
  gint src_width = src_rect->width;
  gint src_height = src_rect->height;

  gauss = g_newa (gfloat, width * width);
  src_buf = g_new0 (gfloat, src_rect->width * src_rect->height * 4);
  dst_buf = g_new0 (gfloat, dst_rect->width * dst_rect->height * 4);

  gegl_buffer_get (src, src_rect, 1.0, format, src_buf, GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  offset = 0;

#define POW2(a) ((a)*(a))
  for (y=-iradius;y<=iradius;y++)
    for (x=-iradius;x<=iradius;x++)
      {
        gauss[x+(int)radius + (y+(int)radius)*width] = exp(- 0.5*(POW2(x)+POW2(y))/radius   );
      }

  for (y=0; y<dst_rect->height; y++)
    for (x=0; x<dst_rect->width; x++)
      {
        gint u,v;
        gfloat *center_pix = src_buf + ((x+iradius)+((y+iradius) * src_width)) * 4;
        gfloat  accumulated[4]={0,0,0,0};
        gfloat  count=0.0;

        for (v=-iradius;v<=iradius;v++)
          for (u=-iradius;u<=iradius;u++)
            {
              gint i,j;
              i = x + radius + u;
              j = y + radius + v;
              if (i >= 0 && i < src_width &&
                  j >= 0 && j < src_height)
                {
                  gint c;

                  gfloat *src_pix = src_buf + (i + j * src_width) * 4;

                  gfloat diff_map   = exp (- (POW2(center_pix[0] - src_pix[0])+
                                              POW2(center_pix[1] - src_pix[1])+
                                              POW2(center_pix[2] - src_pix[2])) * preserve
                                          );
                  gfloat gaussian_weight;
                  gfloat weight;

                  gaussian_weight = gauss[u+(int)radius+(v+(int)radius)*width];

                  weight = diff_map * gaussian_weight;

                  for (c=0;c<4;c++)
                    {
                      accumulated[c] += src_pix[c] * weight;
                    }
                  count += weight;
                }
            }

        for (u=0; u<4;u++)
          dst_buf[offset*4+u] = accumulated[u]/count;
        offset++;
      }
  gegl_buffer_set (dst, dst_rect, 0, format, dst_buf,
                   GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class  = GEGL_OPERATION_CLASS (klass);
  filter_class     = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process   = process;
  operation_class->prepare = prepare;

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
           "name", "gegl:bilateral-filter",
           "title", _("Bilateral Filter"),
           "categories", "enhance:noise-reduction",
           "reference-hash", "5cfcdea9b2f5917f48c54a8972374d8a",
           "description",
           _("Like a gaussian blur; but where the contribution for each neighborhood "
          "pixel is also weighted by the color difference with the original center pixel."),
           NULL);
}

#endif
