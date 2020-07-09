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
 * Copyright (C) 1997 Hirotsuna Mizuno <s1041150@u-aizu.ac.jp>
 * Copyright (C) 2011 Robert Sasu <sasu.robert@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_fractal_trace_type)
  enum_value (GEGL_FRACTAL_TRACE_TYPE_MANDELBROT, "mandelbrot", N_("Mandelbrot"))
  enum_value (GEGL_FRACTAL_TRACE_TYPE_JULIA,      "julia",      N_("Julia"))
enum_end (GeglFractalTraceType)

property_enum (fractal, _("Fractal type"),
               GeglFractalTraceType, gegl_fractal_trace_type,
               GEGL_FRACTAL_TRACE_TYPE_MANDELBROT)

property_double (X1, _("X1"), -1.0)
  description   (_("X1 value, position"))
  value_range   (-50.0, 50.0)

property_double (X2, _("X2"), 0.50)
  description   (_("X2 value, position"))
  value_range   (-50.0, 50.0)

property_double (Y1, _("Y1"), -1.0)
  description   (_("Y1 value, position"))
  value_range   (-50.0, 50.0)

property_double (Y2, _("Y2"), 1.0)
  description   (_("Y2 value, position"))
  value_range   (-50.0, 50.0)

property_double (JX, _("JX"), 0.5)
  description (_("Julia seed X value, position"))
  value_range   (-50.0, 50.0)
  ui_meta       ("visible", "fractal {julia}")

property_double (JY, _("JY"), 0.5)
  description (_("Julia seed Y value, position"))
  value_range   (-50.0, 50.0)
  ui_meta       ("visible", "$JX.visible")

property_int    (depth, _("Depth"), 3)
  value_range   (1, 65536)

property_double (bailout, _("Bailout length"), 10000.0)
  value_range   (0.0, G_MAXDOUBLE)
  ui_range      (0.0, 10000.0)

property_enum   (abyss_policy, _("Abyss policy"),
                   GeglAbyssPolicy, gegl_abyss_policy, GEGL_ABYSS_LOOP)
  description   (_("How to deal with pixels outside of the input buffer"))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     fractal_trace
#define GEGL_OP_C_SOURCE fractal-trace.c

#include "gegl-op.h"

static void
julia (gdouble  x,
       gdouble  y,
       gdouble  jx,
       gdouble  jy,
       gdouble *u,
       gdouble *v,
       gint     depth,
       gdouble  bailout2)
{
  gint    i;
  gdouble xx = x;
  gdouble yy = y;

  for (i = 0; i < depth; i++)
    {
      gdouble x2, y2, tmp;

      x2 = xx * xx;
      y2 = yy * yy;
      tmp = x2 - y2 + jx;
      yy  = 2 * xx * yy + jy;
      xx  = tmp;

      if ((x2 + y2) > bailout2)
        break;
    }

  *u = xx;
  *v = yy;
}

static void
fractaltrace (GeglBuffer            *input,
              GeglSampler           *sampler,
              const GeglRectangle   *picture,
              gfloat                *dst_buf,
              const GeglRectangle   *roi,
              GeglProperties        *o,
              gint                   y,
              GeglFractalTraceType   fractal_type,
              const Babl            *format,
              gint                   level)
{
  GeglBufferMatrix2  scale;   /* a matrix indicating scaling factors around the
                                current center pixel.
                              */
  gint         x, i, offset;
  gdouble      scale_x, scale_y;
  gdouble      bailout2;
  gfloat       dest[4];

  scale_x = (o->X2 - o->X1) / picture->width;
  scale_y = (o->Y2 - o->Y1) / picture->height;

  bailout2 = o->bailout * o->bailout;

  offset = (y - roi->y) * roi->width * 4;

  for (x = roi->x; x < roi->x + roi->width; x++)
    {
      gdouble cx, cy;
      gdouble px, py;
      dest[1] = dest[2] = dest[3] = dest[0] = 0.0;

      switch (fractal_type)
        {
        case GEGL_FRACTAL_TRACE_TYPE_JULIA:
#define gegl_unmap(u,v,ud,vd) {                                         \
            gdouble rx, ry;                                             \
            cx = o->X1 + ((u) - picture->x) * scale_x;                  \
            cy = o->Y1 + ((v) - picture->y) * scale_y;                  \
            julia (cx, cy, o->JX, o->JY, &rx, &ry, o->depth, bailout2); \
            ud = (rx - o->X1) / scale_x + picture->x;                   \
            vd = (ry - o->Y1) / scale_y + picture->y;                   \
          }
        gegl_sampler_compute_scale (scale, x, y);
        gegl_unmap(x,y,px,py);
#undef gegl_unmap
        break;

        case GEGL_FRACTAL_TRACE_TYPE_MANDELBROT:
#define gegl_unmap(u,v,ud,vd) {                                     \
            gdouble rx, ry;                                         \
            cx = o->X1 + ((u) - picture->x) * scale_x;              \
            cy = o->Y1 + ((v) - picture->y) * scale_y;              \
            julia (cx, cy, cx, cy, &rx, &ry, o->depth, bailout2);   \
            ud = (rx - o->X1) / scale_x + picture->x;               \
            vd = (ry - o->Y1) / scale_y + picture->y;               \
          }
        gegl_sampler_compute_scale (scale, x, y);
        gegl_unmap(x,y,px,py);
#undef gegl_unmap
        break;

        default:
          g_error (_("Unsupported fractal type"));
        }

      gegl_sampler_get (sampler, px, py, &scale, dest, o->abyss_policy);

      for (i = 0; i < 4; i++)
        dst_buf[offset++] = dest[i];
    }
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties    *o = GEGL_PROPERTIES (operation);
  GeglRectangle  boundary;
  const Babl    *format = babl_format_with_space ("RGBA float",
                              gegl_operation_get_format (operation, "output"));
  GeglSampler   *sampler;
  gfloat        *dst_buf;
  gint           y;

  boundary = gegl_operation_get_bounding_box (operation);

  dst_buf = g_new0 (gfloat, result->width * result->height * 4);
  sampler = gegl_buffer_sampler_new_at_level (input, format, GEGL_SAMPLER_CUBIC, level);

  for (y = result->y; y < result->y + result->height; y++)
    fractaltrace (input, sampler, &boundary, dst_buf, result, o, y, o->fractal, format, level);

  gegl_buffer_set (output, result, 0, format, dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_object_unref (sampler);

  g_free (dst_buf);

  return TRUE;
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  const GeglRectangle *in_rect =
      gegl_operation_source_get_bounding_box (operation, "input");

  if (in_rect && ! gegl_rectangle_is_infinite_plane (in_rect))
    return *in_rect;

  return *roi;
}

static gboolean
operation_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglOperationClass  *operation_class;

  const GeglRectangle *in_rect =
    gegl_operation_source_get_bounding_box (operation, "input");

  if (in_rect && gegl_rectangle_is_infinite_plane (in_rect))
    {
      gpointer in = gegl_operation_context_get_object (context, "input");
      gegl_operation_context_take_object (context, "output",
                                          g_object_ref (G_OBJECT (in)));
      return TRUE;
    }

  operation_class = GEGL_OPERATION_CLASS (gegl_op_parent_class);

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

  operation_class->get_required_for_output = get_required_for_output;
  operation_class->process                 = operation_process;

  filter_class->process                    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:fractal-trace",
    "title",              _("Fractal Trace"),
    "position-dependent", "true",
    "categories",         "map",
    "license",            "GPL3+",
    "reference-hash",     "7636e00bd6be1d6079abf71ab0db00c7",
    "reference-hashB",    "30146f085fd9a7bd30776e817486d3d7",
    "description", _("Transform the image with the fractals"),
    NULL);
}

#endif
