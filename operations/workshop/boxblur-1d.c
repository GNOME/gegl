/* This file is an image processing operation for GEGL
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
 * Copyright 2019 Thomas Manni <thomas.manni@free.fr>
 *
 * TODO:
 * - manage mipmap level
 * - add opencl support
 */

 #include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int (radius, _("Radius"), 4)
   description(_("Radius of row pixel region, (size will be radius*2+1)"))
   value_range (0, 1000)
   ui_range    (0, 100)
   ui_gamma   (1.5)

property_enum (orientation, _("Orientation"),
               GeglOrientation, gegl_orientation,
               GEGL_ORIENTATION_HORIZONTAL)
  description (_("The orientation of the blur - hor/ver"))

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     boxblur_1d
#define GEGL_OP_C_SOURCE boxblur-1d.c

#include "gegl-op.h"

#include "opencl/gegl-cl.h"
#include "gegl-buffer-cl-iterator.h"

#include "opencl/boxblur-1d.cl.h"

static GeglClRunData *cl_data = NULL;


static gboolean
cl_boxblur (cl_mem                 in_tex,
            cl_mem                 out_tex,
            const GeglRectangle   *roi,
            gint                   radius,
            GeglOrientation        orientation)
{
  cl_int cl_err = 0;
  size_t global_ws[2];
  gint   kernel_num;

  if (!cl_data)
    {
      const char *kernel_name[] = {"box_blur_hor", "box_blur_ver", NULL};
      cl_data = gegl_cl_compile_and_build (boxblur_1d_cl_source, kernel_name);
    }

  if (!cl_data)
    return TRUE;

  if (orientation == GEGL_ORIENTATION_VERTICAL)
    kernel_num = 1;
  else
    kernel_num = 0;

  global_ws[0] = roi->width;
  global_ws[1] = roi->height;

  cl_err = gegl_cl_set_kernel_args (cl_data->kernel[kernel_num],
                                    sizeof(cl_mem), (void*)&in_tex,
                                    sizeof(cl_mem), (void*)&out_tex,
                                    sizeof(cl_int), (void*)&radius,
                                    NULL);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                        cl_data->kernel[kernel_num], 2,
                                        NULL, global_ws, NULL,
                                        0, NULL, NULL);
  CL_CHECK;

  cl_err = gegl_clFinish (gegl_cl_get_command_queue ());
  CL_CHECK;

  return FALSE;

error:
  return TRUE;
}

static gboolean
cl_process (GeglBuffer            *input,
            GeglBuffer            *output,
            const GeglRectangle   *result,
            const Babl            *format,
            gint                   radius,
            GeglOrientation        orientation)
{
  gboolean              err = FALSE;
  cl_int                cl_err = 0;
  GeglBufferClIterator *i;
  gint                  read;
  gint                  left, right, top, bottom;

  if (orientation == GEGL_ORIENTATION_HORIZONTAL)
    {
      right = left = radius;
      top = bottom = 0;
    }
  else
    {
      right = left = 0;
      top = bottom = radius;
    }

  i = gegl_buffer_cl_iterator_new (output,
                                   result,
                                   format,
                                   GEGL_CL_BUFFER_WRITE);

  read = gegl_buffer_cl_iterator_add_2 (i,
                                        input,
                                        result,
                                        format,
                                        GEGL_CL_BUFFER_READ,
                                        left, right,
                                        top, bottom,
                                        GEGL_ABYSS_CLAMP);
  CL_CHECK;

  while (gegl_buffer_cl_iterator_next (i, &err) && !err)
    {
      err = cl_boxblur(i->tex[read],
                       i->tex[0],
                       &i->roi[0],
                       radius,
                       orientation);

      if (err)
        {
          gegl_buffer_cl_iterator_stop (i);
          break;
        }
    }

  CL_CHECK;

  return !err;

error:
  return FALSE;
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties          *o = GEGL_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  const Babl *space      = gegl_operation_get_source_space (operation, "input");
  const Babl *src_format = gegl_operation_get_source_format (operation, "input"); 
  const char *format     = "RaGaBaA float";

  if (o->orientation == GEGL_ORIENTATION_HORIZONTAL)
    {
      op_area->left = op_area->right = o->radius;
    }
  else
    {
      op_area->top = op_area->bottom = o->radius;    
    }

  if (src_format)
    {
      const Babl *model = babl_format_get_model (src_format);

      if (babl_model_is (model, "RGB") || babl_model_is (model, "R'G'B'"))
        {
          format = "RGB float";
        }
      else if (babl_model_is (model, "Y") || babl_model_is (model, "Y'"))
        {
          format = "Y float";
        }
      else if (babl_model_is (model, "YA") || babl_model_is (model, "Y'A") ||
               babl_model_is (model, "YaA") || babl_model_is (model, "Y'aA"))
        {
          format = "YaA float";
        }
      else if (babl_model_is (model, "cmyk"))
        {
          format = "cmyk float";
        }
      else if (babl_model_is (model, "CMYK"))
        {
          format = "CMYK float";
        }
      else if (babl_model_is (model, "cmykA") || 
               babl_model_is (model, "camayakaA") ||
               babl_model_is (model, "CMYKA") ||
               babl_model_is (model, "CaMaYaKaA"))
        {
          format = "camayakaA float";
        }
    }

  gegl_operation_set_format (operation, "input",
                             babl_format_with_space (format, space));
  gegl_operation_set_format (operation, "output",
                             babl_format_with_space (format, space));
}

static GeglSplitStrategy
get_split_strategy (GeglOperation        *operation,
                    GeglOperationContext *context,
                    const gchar          *output_prop,
                    const GeglRectangle  *result,
                    gint                  level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  if (o->orientation == GEGL_ORIENTATION_HORIZONTAL)
    return GEGL_SPLIT_STRATEGY_HORIZONTAL;
  else
    return GEGL_SPLIT_STRATEGY_VERTICAL;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (operation,"input");

  if (! in_rect)
    return *GEGL_RECTANGLE (0, 0, 0, 0);
  
  return *in_rect;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *output_roi)
{
  GeglProperties    *o        = GEGL_PROPERTIES (operation);
  const GeglRectangle in_rect = get_bounding_box (operation);
  GeglRectangle      cached_region = *output_roi;

  if (! gegl_rectangle_is_empty (&in_rect) &&
      ! gegl_rectangle_is_infinite_plane (&in_rect))
    {
      if (o->orientation == GEGL_ORIENTATION_HORIZONTAL)
        {
          cached_region.x     = in_rect.x;
          cached_region.width = in_rect.width;
        }
      else
        {
          cached_region.y      = in_rect.y;
          cached_region.height = in_rect.height;
        }
    }

  return cached_region;
}

static inline void
box_blur_1d (gfloat  *src_buf,
             gfloat  *dst_buf,
             gint     dst_size,
             gint     radius,
             gint     n_components)
{
  gint   n, i;
  gint   src_offset;
  gint   dst_offset = 0;
  gint   prev_rad = radius * n_components + n_components;
  gint   next_rad = radius * n_components;
  gfloat rad1 = 1.f / (gfloat)(radius * 2 + 1);

  for (i = 0; i < (2 * radius + 1); i++)
    {
      src_offset = i * n_components;
      for (n = 0; n < n_components; n++)
        {
          dst_buf[dst_offset + n] += src_buf[src_offset + n] * rad1;
        }    
    }
  
  dst_offset += n_components;
  
  for (i = 1; i < dst_size; i++)
    {
      src_offset = (i + radius) * n_components;
      for (n = 0; n < n_components; n++)
        {
          dst_buf[dst_offset] = dst_buf[dst_offset - n_components]
                              - src_buf[src_offset - prev_rad] * rad1
                              + src_buf[src_offset + next_rad] * rad1;
          src_offset++;
          dst_offset++;
        }
    }
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties  *o = GEGL_PROPERTIES (operation);
  const Babl *format = gegl_operation_get_format (operation, "input");
  gint n_components  = babl_format_get_n_components (format);
  GeglRectangle src_rect;
  GeglRectangle dst_rect;
  GeglRectangle scaled_roi;
  gfloat       *src_buf;
  gfloat       *dst_buf;
  gfloat        factor = 1.0f / (1 << level);
  gint          scaled_radius = o->radius * factor;

  scaled_roi = *roi;

  if (level)
    {
      scaled_roi.x      *= factor;
      scaled_roi.y      *= factor;
      scaled_roi.width  *= factor;
      scaled_roi.height *= factor;
    }

  if (gegl_operation_use_opencl (operation) &&
          format == babl_format ("RaGaBaA float"))
    {
      return cl_process (input, output, &scaled_roi, format,
                         o->radius, o->orientation);
    }

  if (o->orientation == GEGL_ORIENTATION_HORIZONTAL)
    {
      src_rect.x      = scaled_roi.x - scaled_radius;
      src_rect.width  = scaled_roi.width + 2 * scaled_radius;
      src_rect.height = 1;
      
      dst_rect.x      = scaled_roi.x;
      dst_rect.width  = scaled_roi.width;
      dst_rect.height = 1; 

      src_buf = g_new (gfloat, src_rect.width * n_components);

      for (src_rect.y = scaled_roi.y;
           src_rect.y < scaled_roi.y + scaled_roi.height;
           src_rect.y++)
        {
          dst_rect.y = src_rect.y;
          dst_buf = g_new0 (gfloat, dst_rect.width * n_components);

          gegl_buffer_get (input, &src_rect, factor, format, src_buf,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

          box_blur_1d (src_buf, dst_buf, dst_rect.width, scaled_radius,
                       n_components);

          gegl_buffer_set (output, &dst_rect, level, format, dst_buf,
                           GEGL_AUTO_ROWSTRIDE);
          g_free (dst_buf);
        }
    }
  else
    {
      src_rect.y      = scaled_roi.y - scaled_radius;
      src_rect.width  = 1;
      src_rect.height = scaled_roi.height + 2 * scaled_radius;
      
      dst_rect.y      = scaled_roi.y;
      dst_rect.width  = 1;
      dst_rect.height = scaled_roi.height;

      src_buf = g_new  (gfloat, src_rect.height * n_components);

      for (src_rect.x = scaled_roi.x;
           src_rect.x < scaled_roi.x + scaled_roi.width;
           src_rect.x++)
        {
          dst_rect.x = src_rect.x;
          dst_buf = g_new0 (gfloat, dst_rect.height * n_components);

          gegl_buffer_get (input, &src_rect, factor, format, src_buf,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

          box_blur_1d (src_buf, dst_buf, dst_rect.height, scaled_radius,
                       n_components);

          gegl_buffer_set (output, &dst_rect, level, format, dst_buf,
                           GEGL_AUTO_ROWSTRIDE);
          g_free (dst_buf);
        }
    }  

  g_free (src_buf);

  return TRUE;
}

/* Pass-through if radius is zero
 */
static gboolean
operation_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglOperationClass  *operation_class;
  GeglProperties      *o = GEGL_PROPERTIES (operation);

  operation_class = GEGL_OPERATION_CLASS (gegl_op_parent_class);
      
  if (! o->radius)
    {
      gpointer in = gegl_operation_context_get_object (context, "input");
      gegl_operation_context_take_object (context, "output",
                                          g_object_ref (G_OBJECT (in)));
      return TRUE;
    }
  
  return operation_class->process (operation, context, output_prop, result,
                                   gegl_operation_context_get_level (context));
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->get_split_strategy   = get_split_strategy;
  filter_class->process              = process;

  operation_class->get_bounding_box  = get_bounding_box;
  operation_class->get_cached_region = get_cached_region;
  operation_class->opencl_support    = TRUE;
  operation_class->prepare           = prepare;
  operation_class->process           = operation_process;

  gegl_operation_class_set_keys (operation_class,
      "name",        "gegl:boxblur-1d",
      "categories",  "hidden:blur",
      "title",       _("1D Box Blur"),
      "description", _("Blur resulting from averaging the colors of a row "
                       "neighborhood."),
      NULL);
}

#endif
