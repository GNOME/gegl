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
 * Copyright 2016 Miroslav Talasek <miroslav.talasek@seznam.cz>
 *           2017 Thomas Manni <thomas.manni@free.fr>
 *
 * one dimensional wavelet blur used by wavelet-blur operation
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES


property_double (radius, _("Radius"), 1.0)
    description (_("Radius of the wavelet blur"))
    value_range (0.0, 1500.0)
    ui_range    (0.0, 256.0)
    ui_gamma    (3.0)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("radius", "blur")

property_enum (orientation, _("Orientation"),
              GeglOrientation, gegl_orientation,
              GEGL_ORIENTATION_HORIZONTAL)
description (_("The orientation of the blur - hor/ver"))


#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     wavelet_blur_1d
#define GEGL_OP_C_SOURCE wavelet-blur-1d.c

#include "gegl-op.h"

static inline void
wav_get_mean_pixel_1D (gfloat  *src,
                       gfloat  *dst,
                       gint     radius)
{
  gint     i, offset;
  gdouble  weights[3] = {0.25, 0.5, 0.25};
  gdouble  acc[3]     = {0.0, };

  for (i = 0; i < 3; i++)
    {
      offset  = i * radius * 3;
      acc[0] += src[offset]     * weights[i];
      acc[1] += src[offset + 1] * weights[i];
      acc[2] += src[offset + 2] * weights[i];
    }

  dst[0] = acc[0];
  dst[1] = acc[1];
  dst[2] = acc[2];
}

static void
wav_hor_blur (GeglBuffer          *src,
              GeglBuffer          *dst,
              const GeglRectangle *dst_rect,
              gint                 radius,
              const Babl          *format)
{
  gint x, y;

  GeglRectangle write_rect = {dst_rect->x, dst_rect->y, dst_rect->width, 1};

  GeglRectangle read_rect = {dst_rect->x - radius, dst_rect->y,
                             dst_rect->width + 2 * radius, 1};

  gfloat *src_buf = gegl_malloc (read_rect.width * sizeof (gfloat) * 3);
  gfloat *dst_buf = gegl_malloc (write_rect.width * sizeof (gfloat) * 3);

  for (y = 0; y < dst_rect->height; y++)
    {
      gint offset     = 0;
      read_rect.y     = dst_rect->y + y;
      write_rect.y    = dst_rect->y + y;

      gegl_buffer_get (src, &read_rect, 1.0, format, src_buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

      for (x = 0; x < dst_rect->width; x++)
        {
          wav_get_mean_pixel_1D (src_buf + offset,
                                 dst_buf + offset,
                                 radius);
          offset += 3;
        }

      gegl_buffer_set (dst, &write_rect, 0, format, dst_buf,
                       GEGL_AUTO_ROWSTRIDE);
    }

  gegl_free (src_buf);
  gegl_free (dst_buf);
}

static void
wav_ver_blur (GeglBuffer          *src,
              GeglBuffer          *dst,
              const GeglRectangle *dst_rect,
              gint                 radius,
              const Babl          *format)
{
  gint  x, y;

  GeglRectangle write_rect = {dst_rect->x, dst_rect->y, 1, dst_rect->height};

  GeglRectangle read_rect  = {dst_rect->x, dst_rect->y - radius,
                              1, dst_rect->height + 2 * radius};

  gfloat *src_buf    = gegl_malloc (read_rect.height * sizeof(gfloat) * 3);
  gfloat *dst_buf    = gegl_malloc (write_rect.height * sizeof(gfloat) * 3);

  for (x = 0; x < dst_rect->width; x++)
    {
      gint offset     = 0;
      read_rect.x     = dst_rect->x + x;
      write_rect.x    = dst_rect->x + x;

      gegl_buffer_get (src, &read_rect, 1.0, format, src_buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

      for (y = 0; y < dst_rect->height; y++)
        {
          wav_get_mean_pixel_1D (src_buf + offset,
                                 dst_buf + offset,
                                 radius);
          offset += 3;
        }

      gegl_buffer_set (dst, &write_rect, 0, format, dst_buf,
                       GEGL_AUTO_ROWSTRIDE);
    }

  gegl_free (src_buf);
  gegl_free (dst_buf);
}

static void
prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);

  GeglProperties *o   = GEGL_PROPERTIES (operation);
  const Babl *format  = babl_format_with_space ("R'G'B' float", space);

  if (o->orientation == GEGL_ORIENTATION_HORIZONTAL)
    {
      area->left = area->right  = ceil (o->radius);
      area->top  = area->bottom = 0;
    }
  else
    {
      area->left = area->right  = 0;
      area->top  = area->bottom = ceil (o->radius);
    }

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle   result = { 0, };
  GeglRectangle  *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (operation,"input");

  if (!in_rect)
    return result;

  return *in_rect;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  const Babl     *format = gegl_operation_get_format (operation, "output");

  gint radius = ceil (o->radius);

  if (o->orientation == GEGL_ORIENTATION_HORIZONTAL)
    wav_hor_blur (input, output, result, radius, format);
  else
    wav_ver_blur (input, output, result, radius, format);

  return  TRUE;
}

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

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare          = prepare;
  operation_class->process          = operation_process;
  operation_class->opencl_support   = FALSE;
  operation_class->threaded         = TRUE;

  filter_class->get_split_strategy  = get_split_strategy;
  filter_class->process             = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:wavelet-blur-1d",
    "categories",  "hidden:blur",
    "title",       _("1D Wavelet-blur"),
    "reference-hash", "f7879e0dcf29fa78df7b2c400842ddce",
    "description", _("This blur is used for the wavelet decomposition filter, "
                     "each pixel is computed from another by the HAT transform"),
    NULL);
}

#endif
