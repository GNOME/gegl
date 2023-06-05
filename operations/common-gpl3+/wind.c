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
 * Copyright Nigel Wetten
 * Copyright 2000 Tim Copperfield <timecop@japan.co.jp>
 * Copyright 2011 Hans Lo <hansshulo@gmail.com>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_wind_style)
  enum_value (GEGL_WIND_STYLE_WIND, "wind", N_("Wind"))
  enum_value (GEGL_WIND_STYLE_BLAST, "blast", N_("Blast"))
enum_end (GeglWindStyle)

enum_start (gegl_wind_direction)
  enum_value (GEGL_WIND_DIRECTION_LEFT, "left", N_("Left"))
  enum_value (GEGL_WIND_DIRECTION_RIGHT, "right", N_("Right"))
  enum_value (GEGL_WIND_DIRECTION_TOP, "top", N_("Top"))
  enum_value (GEGL_WIND_DIRECTION_BOTTOM, "bottom", N_("Bottom"))
enum_end (GeglWindDirection)

enum_start (gegl_wind_edge)
  enum_value (GEGL_WIND_EDGE_BOTH, "both", N_("Both"))
  enum_value (GEGL_WIND_EDGE_LEADING, "leading", N_("Leading"))
  enum_value (GEGL_WIND_EDGE_TRAILING, "trailing", N_("Trailing"))
enum_end (GeglWindEdge)

property_enum (style, _("Style"),
               GeglWindStyle, gegl_wind_style,
               GEGL_WIND_STYLE_WIND)
  description (_("Style of effect"))

property_enum (direction, _("Direction"),
               GeglWindDirection, gegl_wind_direction,
               GEGL_WIND_DIRECTION_LEFT)
  description (_("Direction of the effect"))

property_enum (edge, _("Edge Affected"),
               GeglWindEdge, gegl_wind_edge,
               GEGL_WIND_EDGE_LEADING)
  description (_("Edge behavior"))

property_int (threshold, _("Threshold"), 10)
 description (_("Higher values restrict the effect to fewer areas of the image"))
 value_range (0, 50)

property_int (strength, _("Strength"), 10)
 description (_("Higher values increase the magnitude of the effect"))
 value_range (1, 100)

property_seed (seed, _("Random seed"), rand)

#else

#define GEGL_OP_FILTER
#define GEGL_OP_C_SOURCE wind.c
#define GEGL_OP_NAME     wind

#include "gegl-op.h"

#define COMPARE_WIDTH    3

static void
get_derivative (gfloat       *pixel1,
                gfloat       *pixel2,
                gboolean      has_alpha,
                GeglWindEdge  edge,
                gfloat       *derivative)
{
  gint i;

  for (i = 0; i < 3; i++)
    derivative[i] = pixel2[i] - pixel1[i];

  if (has_alpha)
    derivative[3] = pixel2[3] - pixel1[3];
  else
    derivative[3] = 0.0;

  if (edge == GEGL_WIND_EDGE_BOTH)
    {
      for (i = 0; i < 4; i++)
        derivative[i] = fabs (derivative[i]);
    }
  else if (edge == GEGL_WIND_EDGE_LEADING)
    {
      for (i = 0; i < 4; i++)
        derivative[i] = - derivative[i];
    }
}

static gboolean
threshold_exceeded (gfloat         *pixel1,
                    gfloat         *pixel2,
                    gboolean        has_alpha,
                    GeglWindEdge    edge,
                    gint            threshold)
{
  gfloat derivative[4];
  gint i;
  gfloat sum = 0.0;

  get_derivative (pixel1, pixel2, has_alpha, edge, derivative);

  for (i = 0; i < 4; i++)
    sum += derivative[i];

  return ((sum / 4.0f) > (threshold / 200.0));
}

static void
reverse_buffer (gfloat *buffer,
                gint    length,
                gint    bytes)
{
  gint b, i, si;
  gfloat temp;
  gint midpoint;

  midpoint = length / 2;
  for (i = 0; i < midpoint; i += bytes)
    {
      si = length - bytes - i;

      for (b = 0; b < bytes; b++)
        {
          temp = buffer[i + b];
          buffer[i + b] = buffer[si + b];
          buffer[si + b] = temp;
        }
    }
}

static void
render_wind_row (gfloat         *buffer,
                 gint            n_components,
                 gint            lpi,
                 GeglProperties *o,
                 gint            x,
                 gint            y)
{
  gfloat *blend_color;
  gfloat *target_color;
  gfloat *blend_amt;
  gboolean has_alpha;
  gint i, j, b;
  gint bleed_length;
  gint n;
  gint sbi;  /* starting bleed index */
  gint lbi;      /* last bleed index */
  gdouble denominator;
  gint comp_stride = n_components * COMPARE_WIDTH;

  target_color = g_new0 (gfloat, n_components);
  blend_color  = g_new0 (gfloat, n_components);
  blend_amt    = g_new0 (gfloat, n_components);

  has_alpha = n_components > 3 ? TRUE : FALSE;

  for (j = 0; j < lpi; j += n_components)
    {
      gint pxi = j;

      if (threshold_exceeded (buffer + pxi,
                              buffer + pxi + comp_stride,
                              has_alpha,
                              o->edge,
                              o->threshold))
        {
          gdouble bleed_length_max;
          sbi = pxi + comp_stride;

          for (b = 0; b < n_components; b++)
            {
              blend_color[b]  = buffer[pxi + b];
              target_color[b] = buffer[sbi + b];
            }

          if (gegl_random_int_range (o->rand, x, y, 0, 0, 0, 3))
            {
              bleed_length_max = o->strength;
            }
          else
            {
              bleed_length_max = 4 * o->strength;
            }

          bleed_length = 1 + (gint) (bleed_length_max *
                                     gegl_random_float (o->rand, x, y, 0, 1));

          lbi = sbi + bleed_length * n_components;
          if (lbi > lpi)
            {
              lbi = lpi;
            }

          for (b = 0; b < n_components; b++)
            blend_amt[b] = target_color[b] - blend_color[b];

          denominator = 2.0 / (bleed_length * bleed_length + bleed_length);
          n = bleed_length;

          for (i = sbi; i < lbi; i += n_components)
            {
              if (!threshold_exceeded (buffer + pxi,
                                       buffer + i,
                                       has_alpha,
                                       o->edge,
                                       o->threshold)
                  && gegl_random_int_range (o->rand, x, y, 0, 2, 0, 1))
                {
                  break;
                }

              for (b = 0; b < n_components; b++)
                {
                  blend_color[b] += blend_amt[b] * n * denominator;
                  blend_color[b] = CLAMP (blend_color[b], 0.0, 1.0);
                  buffer[i + b] = (blend_color[b] * 2 + buffer[i + b]) / 3;
                }

              if (threshold_exceeded (buffer + i,
                                      buffer + i + comp_stride,
                                      has_alpha,
                                      GEGL_WIND_EDGE_BOTH,
                                      o->threshold))
                {
                  for (b = 0; b < n_components; b++)
                    {
                      target_color[b] = buffer[i + comp_stride + b];
                      blend_amt[b] = target_color[b] - blend_color[b];
                    }

                  denominator = 2.0 / (n * n + n);
                }
              n--;
            }
        }

      x++;
    }

  g_free (target_color);
  g_free (blend_color);
  g_free (blend_amt);
}

static gboolean
render_blast_row (gfloat         *buffer,
                  gint            n_components,
                  gint            lpi,
                  GeglProperties *o,
                  gint            x,
                  gint            y)
{
  gint sbi, lbi;
  gint bleed_length;
  gint i, j, b;
  gint weight, random_factor;
  gboolean skip = FALSE;

  for (j = 0; j < lpi; j += n_components)
    {
      gfloat *pbuf = buffer + j;

      if (threshold_exceeded (pbuf,
                              pbuf + n_components,
                              n_components > 3,
                              o->edge,
                              o->threshold))
        {
          sbi = j;
          weight = gegl_random_int_range (o->rand, x, y, 0, 0, 0, 10);

          if (weight > 5)
            {
              random_factor = 2;
            }
          else if (weight > 3)
            {
              random_factor = 3;
            }
          else
            {
              random_factor = 4;
            }

          bleed_length = 0;

          switch (gegl_random_int_range (o->rand, x, y, 0, 1, 0, random_factor))
            {
              case 3: bleed_length += o->strength; /* FALLTHROUGH */
              case 2: bleed_length += o->strength; /* FALLTHROUGH */
              case 1: bleed_length += o->strength; /* FALLTHROUGH */
              case 0: bleed_length += o->strength; /* FALLTHROUGH */
            }

          lbi = sbi + n_components * bleed_length;
          if (lbi > lpi)
            {
              lbi = lpi;
            }

          for (i = sbi; i < lbi; i += n_components)
            for (b = 0; b < n_components; b++)
                buffer[i+b] = *(pbuf + b);

          j = lbi - n_components;

          if (gegl_random_int_range (o->rand, x, y, 0, 2, 0, 10) > 7)
            {
              skip = TRUE;
            }
        }

      x++;
    }
  return skip;
}

static void
prepare (GeglOperation *operation)
{
  const Babl *in_format = gegl_operation_get_source_format (operation, "input");
  const Babl *format = babl_format_with_space ("RGB float", in_format);

  if (in_format)
  {
    if (babl_format_has_alpha (in_format))
      format = babl_format_with_space ("RGBA float", in_format);
  }

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle   result = *roi;

  const GeglRectangle  *in_rect =
      gegl_operation_source_get_bounding_box (operation, "input");

  if (in_rect && ! gegl_rectangle_is_infinite_plane (in_rect))
    {
      if (o->direction == GEGL_WIND_DIRECTION_LEFT ||
          o->direction == GEGL_WIND_DIRECTION_RIGHT)
        {
          result.x     = in_rect->x;
          result.width = in_rect->width;
        }
      else
        {
          result.y      = in_rect->y;
          result.height = in_rect->height;
        }
    }

  return result;
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle   result = *roi;

  const GeglRectangle  *in_rect =
      gegl_operation_source_get_bounding_box (operation, "input");

  if (in_rect && ! gegl_rectangle_is_infinite_plane (in_rect))
    {
      if (o->direction == GEGL_WIND_DIRECTION_TOP)
        {
          result.height = in_rect->height - roi->y;
        }
      else if (o->direction == GEGL_WIND_DIRECTION_BOTTOM)
        {
          result.y      = in_rect->y;
          result.height = in_rect->height - roi->y + roi->height;
        }
      else if (o->direction == GEGL_WIND_DIRECTION_RIGHT)
        {
          result.x     = in_rect->x;
          result.width = in_rect->width - roi->x + roi->width;
        }
      else
        {
          result.width = in_rect->width - roi->x;
        }
    }

  return result;
}

static GeglSplitStrategy
get_split_strategy (GeglOperation        *operation,
                    GeglOperationContext *context,
                    const gchar          *output_prop,
                    const GeglRectangle  *result,
                    gint                  level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

    if (o->direction == GEGL_WIND_DIRECTION_LEFT ||
        o->direction == GEGL_WIND_DIRECTION_RIGHT)
    return GEGL_SPLIT_STRATEGY_HORIZONTAL;
  else
    return GEGL_SPLIT_STRATEGY_VERTICAL;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *format = gegl_operation_get_format (operation, "output");
  gint n_components = babl_format_get_n_components (format);

  gint           y, row_size;
  gint           row_start, row_end;
  GeglRectangle  row_rect;
  gfloat        *row_buf;
  gboolean       skip_rows;
  gboolean       need_reverse;
  gboolean       horizontal_effect;
  gint           last_pix;

  horizontal_effect = (o->direction == GEGL_WIND_DIRECTION_LEFT ||
                       o->direction == GEGL_WIND_DIRECTION_RIGHT);

  need_reverse = (o->direction == GEGL_WIND_DIRECTION_LEFT ||
                  o->direction == GEGL_WIND_DIRECTION_TOP);

  if (horizontal_effect)
    {
      row_size   = result->width * n_components;
      row_start  = result->y;
      row_end    = result->y + result->height;
      row_rect.x = result->x;
      row_rect.width  = result->width;
      row_rect.height = 1;
    }
  else
    {
      row_size  = result->height * n_components;
      row_start = result->x;
      row_end   = result->x + result->width;
      row_rect.y = result->y;
      row_rect.width  = 1;
      row_rect.height = result->height;
    }

  row_buf = g_new (gfloat, row_size);

  for (y = row_start; y < row_end; y++)
    {
      if (horizontal_effect)
        row_rect.y = y;
      else
        row_rect.x = y;

      gegl_buffer_get (input, &row_rect, 1.0, format, row_buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      if (need_reverse)
        reverse_buffer (row_buf, row_size, n_components);

      if (o->style == GEGL_WIND_STYLE_WIND)
        {
          last_pix  = row_size - (n_components * COMPARE_WIDTH);
          skip_rows = FALSE;
          render_wind_row (row_buf, n_components, last_pix, o, row_rect.x, y);
        }
      else
        {
          last_pix = row_size - n_components;
          skip_rows = render_blast_row (row_buf, n_components, last_pix, o,
                                        row_rect.x, y);
        }

      if (need_reverse)
        reverse_buffer (row_buf, row_size, n_components);

      gegl_buffer_set (output, &row_rect, level, format, row_buf,
                       GEGL_AUTO_ROWSTRIDE);

      if (skip_rows)
        {
          GeglRectangle rect   = row_rect;
          gint          n_rows = gegl_random_int_range (o->rand,
                                                        row_rect.x, y, 0, 4,
                                                        1, 3);

          if (horizontal_effect)
            {
              rect.y      = y + 1;
              rect.height = n_rows;
            }
          else
            {
              rect.x     = y + 1;
              rect.width = n_rows;
            }

          gegl_buffer_copy (input, &rect, GEGL_ABYSS_CLAMP, output, &rect);
          y += n_rows;
        }
    }

  g_free (row_buf);

  return TRUE;
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

  filter_class->process                    = process;
  filter_class->get_split_strategy         = get_split_strategy;
  operation_class->prepare                 = prepare;
  operation_class->process                 = operation_process;
  operation_class->get_cached_region       = get_cached_region;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->opencl_support          = FALSE;

  gegl_operation_class_set_keys (operation_class,
     "name",           "gegl:wind",
     "title",          _("Wind"),
     "categories",     "distort",
     "license",        "GPL3+",
     "reference-hash", "0991d44188947d2c355062ce1d522f6e",
     "description",    _("Wind-like bleed effect"),
     NULL);
}

#endif
