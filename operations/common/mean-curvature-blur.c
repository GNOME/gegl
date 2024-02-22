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
 * Copyright 2018 Thomas Manni <thomas.manni@free.fr>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int  (iterations, _("Iterations"), 20)
  description (_("Controls the number of iterations"))
  value_range (0, 500)
  ui_range    (0, 60)

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME         mean_curvature_blur
#define GEGL_OP_C_SOURCE     mean-curvature-blur.c

#define POW2(x) ((x)*(x))

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  GeglOperationAreaFilter *area   = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties          *o      = GEGL_PROPERTIES (operation);
  const Babl              *format = babl_format_with_space ("R'G'B'A float", space);

  area->left = area->right = area->top = area->bottom = o->iterations;

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = { 0, 0, 0, 0 };
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (operation, "input");
  if (in_rect)
    {
      result = *in_rect;
    }

  return result;
}

static void
mean_curvature_flow (gfloat *src_buf,
                     gint    src_stride,
                     gfloat *dst_buf,
                     gint    dst_width,
                     gint    dst_height,
                     gint    dst_stride)
{
  gint c;
  gint x, y;
  gfloat *center_pix;

#define O(u,v) (((u)+((v) * src_stride)) * 4)
  gint   offsets[8] = { O( -1, -1), O(0, -1), O(1, -1),
                        O( -1,  0),           O(1,  0),
                        O( -1,  1), O(0, 1),  O(1,  1)};
#undef O

#define LEFT(c) ((center_pix + offsets[3])[c])
#define RIGHT(c) ((center_pix + offsets[4])[c])
#define TOP(c) ((center_pix + offsets[1])[c])
#define BOTTOM(c) ((center_pix + offsets[6])[c])
#define TOPLEFT(c) ((center_pix + offsets[0])[c])
#define TOPRIGHT(c) ((center_pix + offsets[2])[c])
#define BOTTOMLEFT(c) ((center_pix + offsets[5])[c])
#define BOTTOMRIGHT(c) ((center_pix + offsets[7])[c])

  for (y = 0; y < dst_height; y++)
    {
      gint dst_offset = dst_stride * y;
      center_pix = src_buf + ((y+1) * src_stride + 1) * 4;

      for (x = 0; x < dst_width; x++)
        {
          for (c = 0; c < 3; c++) /* do each color component individually */
            {
              gdouble dx  = RIGHT(c) - LEFT(c);
              gdouble dy  = BOTTOM(c) - TOP(c);
              gdouble magnitude = sqrt (POW2(dx) + POW2(dy));

              dst_buf[dst_offset * 4 + c] = center_pix[c];

              if (magnitude)
                {
                  gdouble dx2 = POW2(dx);
                  gdouble dy2 = POW2(dy);

                  gdouble dxx = RIGHT(c) + LEFT(c) - 2. * center_pix[c];
                  gdouble dyy = BOTTOM(c) + TOP(c) - 2. * center_pix[c];
                  gdouble dxy = 0.25 * (BOTTOMRIGHT(c) - TOPRIGHT(c) - BOTTOMLEFT(c) + TOPLEFT(c));

                  gdouble n = dx2 * dyy + dy2 * dxx - 2. * dx * dy * dxy;
                  gdouble d = sqrt (pow (dx2 + dy2, 3.));
                  gdouble mean_curvature = n / d;

                  dst_buf[dst_offset * 4 + c] += (0.25 * magnitude * mean_curvature);
                }
            }

          dst_buf[dst_offset * 4 + 3] = center_pix[3];

          dst_offset++;
          center_pix += 4;
        }
    }

#undef LEFT
#undef RIGHT
#undef TOP
#undef BOTTOM
#undef TOPLEFT
#undef TOPRIGHT
#undef BOTTOMLEFT
#undef BOTTOMRIGHT

}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties  *o = GEGL_PROPERTIES (operation);
  const Babl      *format = gegl_operation_get_format (operation, "output");

  gint iteration;
  gint stride;
  gfloat *src_buf;
  gfloat *dst_buf;
  GeglRectangle rect;

  rect = *roi;

  rect.x      -= o->iterations;
  rect.y      -= o->iterations;
  rect.width  += o->iterations*2;
  rect.height += o->iterations*2;

  stride = roi->width + o->iterations * 2;

  src_buf = g_new (gfloat,
         (stride) * (roi->height + o->iterations * 2) * 4);

  dst_buf = g_new0 (gfloat,
         (stride) * (roi->height + o->iterations * 2) * 4);

  gegl_buffer_get (input, &rect, 1.0, format, src_buf, stride * 4 * 4,
                   GEGL_ABYSS_CLAMP);

  for (iteration = 0; iteration < o->iterations; iteration++)
    {
      mean_curvature_flow (src_buf, stride,
                           dst_buf,
                           roi->width  + (o->iterations - 1 - iteration) * 2,
                           roi->height + (o->iterations - 1 - iteration) * 2,
                           stride);

      { /* swap buffers */
        gfloat *tmp = src_buf;
        src_buf = dst_buf;
        dst_buf = tmp;
      }
    }

  gegl_buffer_set (output, roi, 0, format, src_buf, stride * 4 * 4);

  g_free (src_buf);
  g_free (dst_buf);

  return TRUE;
}

/* Pass-through when iterations parameter is set to zero */

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

  if (! o->iterations)
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

  filter_class->process             = process;
  operation_class->process          = operation_process;
  operation_class->prepare          = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->opencl_support   = FALSE;

  gegl_operation_class_set_keys (operation_class,
    "name",           "gegl:mean-curvature-blur",
    "title",          _("Mean Curvature Blur"),
    "categories",     "blur",
    "reference-hash", "8856d371c39a439e501dc2f2a74d6417",
    "description",    _("Regularize geometry at a speed proportional to the local mean curvature value"),
    NULL);
}

#endif
