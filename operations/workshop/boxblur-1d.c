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
  gfloat       *src_buf;
  gfloat       *dst_buf;

  if (o->orientation == GEGL_ORIENTATION_HORIZONTAL)
    {
      src_rect.x      = roi->x - o->radius;
      src_rect.width  = roi->width + 2 * o->radius;
      src_rect.height = 1;
      
      dst_rect.x      = roi->x;
      dst_rect.width  = roi->width;
      dst_rect.height = 1; 
      
      src_buf = g_new (gfloat, src_rect.width * n_components);

      for (src_rect.y = roi->y; src_rect.y < roi->y + roi->height; src_rect.y++)
        {
          dst_rect.y = src_rect.y;
          dst_buf = g_new0 (gfloat, dst_rect.width * n_components);

          gegl_buffer_get (input, &src_rect, 1.0, format, src_buf,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

          box_blur_1d (src_buf, dst_buf, dst_rect.width, o->radius, n_components);

          gegl_buffer_set (output, &dst_rect, 0, format, dst_buf,
                           GEGL_AUTO_ROWSTRIDE);
          g_free (dst_buf);
        }
    }
  else
    {
      src_rect.y      = roi->y - o->radius;
      src_rect.width  = 1;
      src_rect.height = roi->height + 2 * o->radius;
      
      dst_rect.y      = roi->y;
      dst_rect.width  = 1;
      dst_rect.height = roi->height;
      
      src_buf = g_new  (gfloat, src_rect.height * n_components);

      for (src_rect.x = roi->x; src_rect.x < roi->x + roi->width; src_rect.x++)
        {
          dst_rect.x = src_rect.x;
          dst_buf = g_new0 (gfloat, dst_rect.height * n_components);

          gegl_buffer_get (input, &src_rect, 1.0, format, src_buf,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

          box_blur_1d (src_buf, dst_buf, dst_rect.height, o->radius, n_components);

          gegl_buffer_set (output, &dst_rect, 0, format, dst_buf,
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
  operation_class->opencl_support    = FALSE;
  operation_class->prepare           = prepare;
  operation_class->process           = operation_process;

  gegl_operation_class_set_keys (operation_class,
      "name",        "gegl:boxblur-1d",
      "categories",  "hidden:blur",
      "title",       _("1D Box Blur"),
      "description", _("Blur resulting from averaging the colors of a row "
                       "neighbourhood."),
      NULL);
}

#endif