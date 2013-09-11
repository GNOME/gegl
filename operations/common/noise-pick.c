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
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 1997 Miles O'Neal <meo@rru.com>  http://www.rru.com/~meo/
 * Copyright 2012 Maxime Nicco <maxime.nicco@gmail.com>
 */

/*
 *  PICK Operation
 *     We pick a pixel at random from the neighborhood of the current pixel.
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_seed   (seed, _("Seed"),
                   _("Random seed"))

gegl_chant_double (pct_random, _("Randomization (%)"),
                   0.0, 100.0, 50.0,
                   _("Randomization"))

gegl_chant_int    (repeat, _("Repeat"),
                   1, 100, 1,
                   _("Repeat"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE "noise-pick.c"

#include "gegl-chant.h"

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);

  op_area->left   = 1;
  op_area->right  = 1;
  op_area->top    = 1;
  op_area->bottom = 1;

  gegl_operation_set_format (operation, "input" , babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"
#include "gegl/gegl-config.h"
#include "opencl/noise-pick.cl.h"

GEGL_CL_STATIC

static gboolean
cl_noise_pick (cl_mem               in_tex,
               cl_mem               aux_tex,
               cl_mem               out_tex,
               const GeglRectangle  *src_roi,
               const GeglRectangle  *roi,
               const GeglRectangle  *wr,
               gint                 radius,
               gint                 seed,
               gfloat               pct_random,
               gint                 repeat)
{
  cl_int cl_err           = 0;
  cl_mem cl_random_data   = NULL;
  cl_mem cl_random_primes = NULL;
  const size_t gbl_size[2] = {roi->width, roi->height};
  const size_t src_size = src_roi->width*src_roi->height;

  GEGL_CL_BUILD(noise_pick, "cl_noise_pick", "copy_out_to_aux")

  cl_random_data = gegl_cl_load_random_data(&cl_err);
  CL_CHECK;
  cl_random_primes = gegl_cl_load_random_primes(&cl_err);
  CL_CHECK;

  {
  cl_int   cl_roi_x      = roi->x;
  cl_int   cl_roi_y      = roi->y;
  cl_int   cl_src_width  = src_roi->width;
  cl_int   cl_wr_width   = wr->width;
  cl_int   cl_radius     = radius;
  cl_int   cl_seed       = seed;
  cl_float cl_pct_random = pct_random;
  cl_int   offset;
  int wr_size = wr->width*wr->height;
  int it;

  cl_err = gegl_clEnqueueCopyBuffer(gegl_cl_get_command_queue(),
                                    in_tex , aux_tex , 0 , 0 ,
                                    src_size * sizeof(cl_float4),
                                    0, NULL, NULL);
  CL_CHECK;

  /*Arguments for the first kernel*/
  GEGL_CL_ARG_START(cl_data->kernel[0])
  GEGL_CL_ARG(cl_mem,   aux_tex)
  GEGL_CL_ARG(cl_mem,   out_tex)
  GEGL_CL_ARG(cl_mem,   cl_random_data)
  GEGL_CL_ARG(cl_mem,   cl_random_primes)
  GEGL_CL_ARG(cl_int,   cl_roi_x)
  GEGL_CL_ARG(cl_int,   cl_roi_y)
  GEGL_CL_ARG(cl_int,   cl_src_width)
  GEGL_CL_ARG(cl_int,   cl_wr_width)
  GEGL_CL_ARG(cl_int,   cl_radius)
  GEGL_CL_ARG(cl_int,   cl_seed)
  GEGL_CL_ARG(cl_float, cl_pct_random)
  GEGL_CL_ARG_END

  /*Arguments for the second kernel*/
  GEGL_CL_ARG_START(cl_data->kernel[1])
  GEGL_CL_ARG(cl_mem,   out_tex)
  GEGL_CL_ARG(cl_mem,   aux_tex)
  GEGL_CL_ARG(cl_int,   cl_src_width)
  GEGL_CL_ARG(cl_int,   cl_radius)
  GEGL_CL_ARG_END

  offset = 0;

  for(it = 0; it < repeat; ++it)
    {
      cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 11, sizeof(cl_int),
                                   (void*)&offset);
      CL_CHECK;
      cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                           cl_data->kernel[0], 2,
                                           NULL, gbl_size, NULL,
                                           0, NULL, NULL);
      CL_CHECK;
      if(it < repeat -1) /*No need for the last copy*/
        {
          cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                              cl_data->kernel[1], 2,
                                              NULL, gbl_size, NULL,
                                               0, NULL, NULL);
          CL_CHECK;
        }

      offset += wr_size;
    }

  cl_err = gegl_clFinish(gegl_cl_get_command_queue ());
  CL_CHECK;

  GEGL_CL_RELEASE(cl_random_data)
  GEGL_CL_RELEASE(cl_random_primes)
  }

  return  FALSE;

error:
  if(cl_random_data)
    GEGL_CL_RELEASE(cl_random_data)
  if(cl_random_primes)
    GEGL_CL_RELEASE(cl_random_primes)
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
  GeglRectangle *wr      = gegl_operation_source_get_bounding_box (operation,
                                                                   "input");

  gint err;

  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

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

  gint aux  = gegl_buffer_cl_iterator_add_2 (i,
                                             NULL,
                                             result,
                                             in_format,
                                             GEGL_CL_BUFFER_AUX,
                                             op_area->left,
                                             op_area->right,
                                             op_area->top,
                                             op_area->bottom,
                                             GEGL_ABYSS_NONE);

  GEGL_CL_BUFFER_ITERATE_START(i, err)
    {
      err = cl_noise_pick (i->tex[read],
                           i->tex[aux],
                           i->tex[0],
                           &i->roi[read],
                           &i->roi[0],
                           wr,
                           1,
                           o->seed,
                           o->pct_random,
                           o->repeat);
    }
  GEGL_CL_BUFFER_ITERATE_END(err)

  return TRUE;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO              *o       = GEGL_CHANT_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglBuffer              *tmp;
  gfloat                  *src_buf;
  gfloat                  *dst_buf;
  gfloat                  *in_pixel;
  gfloat                  *out_pixel;
  gint                     n_pixels = result->width * result->height;
  gint                     width    = result->width;
  GeglRectangle            src_rect;
  GeglRectangle            whole_region;
  gint                     total_pixels;
  gint                     whole_region_size;
  gint                     offset;
  gint                     i;

  if (gegl_cl_is_accelerated ())
    {
      if (cl_process (operation, input, output, result))
        return TRUE;
      else
        gegl_cl_disable();
    }

  tmp = gegl_buffer_new (result, babl_format ("RGBA float"));

  src_rect.x      = result->x - op_area->left;
  src_rect.width  = result->width + op_area->left + op_area->right;
  src_rect.y      = result->y - op_area->top;
  src_rect.height = result->height + op_area->top + op_area->bottom;

  total_pixels = src_rect.height * src_rect.width;

  src_buf = g_slice_alloc (4 * total_pixels * sizeof (gfloat));
  dst_buf = g_slice_alloc (4 * n_pixels * sizeof (gfloat));

  gegl_buffer_copy (input, NULL, tmp, NULL);

  whole_region        = *(gegl_operation_source_get_bounding_box (operation,
                                                                  "input"));
  whole_region_size   = whole_region.width*whole_region.height;
  offset              = 0;

  for (i = 0; i < o->repeat; i++)
    {
      gint x, y;

      x = result->x;
      y = result->y;

      n_pixels = result->width * result->height;

      gegl_buffer_get (tmp, &src_rect, 1.0,
                       babl_format ("RGBA float"), src_buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

      in_pixel  = src_buf + (src_rect.width + 1) * 4;
      out_pixel = dst_buf;

      while (n_pixels--)
        {
          gint b, n, idx;

          /* n is independent from the roi, but from the whole image*/
          idx = x + whole_region.width * y;
          n = 2 * (idx + offset);

          if (gegl_random_float_range (o->seed, x, y, 0, n, 0.0, 100.0) <=
              o->pct_random)
            {
              gint k = gegl_random_int_range (o->seed, x, y, 0, n+1, 0, 9);

              for (b = 0; b < 4; b++)
                {
                  switch (k)
                    {
                    case 0:
                      out_pixel[b] = in_pixel[b - src_rect.width * 4 - 4];
                      break;
                    case 1:
                      out_pixel[b] = in_pixel[b - src_rect.width * 4];
                      break;
                    case 2:
                      out_pixel[b] = in_pixel[b - src_rect.width * 4 + 4];
                      break;
                    case 3:
                      out_pixel[b] = in_pixel[b - 4];
                      break;
                    case 4:
                      out_pixel[b] = in_pixel[b];
                      break;
                    case 5:
                      out_pixel[b] = in_pixel[b + 4];
                      break;
                    case 6:
                      out_pixel[b] = in_pixel[b + src_rect.width * 4 - 4];
                      break;
                    case 7:
                      out_pixel[b] = in_pixel[b + src_rect.width * 4];
                      break;
                    case 8:
                      out_pixel[b] = in_pixel[b + src_rect.width * 4 + 4];
                      break;
                    }
                }
            }
          else
            {
              for (b = 0; b < 4; b++)
                {
                  out_pixel[b] = in_pixel[b];
                }
            }

          if (n_pixels % width == 0)
            in_pixel += 12;
          else
            in_pixel += 4;

          out_pixel += 4;

          x++;
          if (x >= result->x + result->width)
            {
              x = result->x;
              y++;
            }
        }

      offset += whole_region_size;

      gegl_buffer_set (tmp, result, 0,
                       babl_format ("RGBA float"), dst_buf,
                       GEGL_AUTO_ROWSTRIDE);
    }

  gegl_buffer_copy (tmp, NULL, output, NULL);

  g_slice_free1 (4 * total_pixels * sizeof (gfloat), src_buf);
  g_slice_free1 (4 * n_pixels * sizeof (gfloat), dst_buf);

  g_object_unref (tmp);

  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare = prepare;
  filter_class->process    = process;

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name",       "gegl:noise-pick",
    "categories", "noise",
    "description", _("Randomly interchange some pixels with neighbors"),
    NULL);
}

#endif
