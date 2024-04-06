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
 * Copyright (C) 2018 Ell
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_enum (metric, _("Metric"),
               GeglDistanceMetric, gegl_distance_metric,
               GEGL_DISTANCE_METRIC_EUCLIDEAN)
  description (_("Metric to use for the distance calculation"))

property_color (mask, _("Mask"), "transparent")
  description (_("Unseeded region color"))

property_boolean (invert, _("Invert"), FALSE)
  description (_("Invert mask"))

property_boolean (seed_edges, _("Seed edges"), FALSE)
  description (_("Whether the image edges are also seeded"))

property_enum (abyss_policy, _("Abyss policy"),
               GeglAbyssPolicy, gegl_abyss_policy,
               GEGL_ABYSS_NONE)
  description (_("How image edges are handled"))
  ui_meta     ("sensitive", "seed-edges")

#else

#define GEGL_OP_COMPOSER
#define GEGL_OP_NAME     voronoi_diagram
#define GEGL_OP_C_SOURCE voronoi-diagram.cc

#include "gegl-op.h"


static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return gegl_operation_get_bounding_box (operation);
}

static GeglRectangle
get_invalidated_by_change (GeglOperation       *operation,
                           const gchar         *input_pad,
                           const GeglRectangle *roi)
{
  return gegl_operation_get_bounding_box (operation);
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  return gegl_operation_get_bounding_box (operation);
}

static void
prepare (GeglOperation *operation)
{
  const Babl *format = gegl_operation_get_source_format (operation, "input");

  if (! format)
    format = babl_format ("RGBA float");

  gegl_operation_set_format (operation, "output", format);
}

struct BasicMetric
{
  static gint
  distance (gint x)
  {
    return x;
  }
};

struct EuclideanMetric : BasicMetric
{
  static gint
  distance (gint x)
  {
    return x * x;
  }

  static gint
  distance (gint x2,
            gint y2)
  {
    return x2 + y2;
  }
};

struct ManhattanMetric : BasicMetric
{
  using BasicMetric::distance;

  static gint
  distance (gint x,
            gint y)
  {
    return x + y;
  }
};

struct ChebyshevMetric : BasicMetric
{
  using BasicMetric::distance;

  static gint
  distance (gint x,
            gint y)
  {
    return MAX (x, y);
  }
};

template <class Metric>
static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *aux,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level,
         Metric               metric)
{
  GeglProperties *o           = GEGL_PROPERTIES (operation);
  gboolean        invert      = o->invert;
  const Babl     *format      = gegl_buffer_get_format (output);
  const Babl     *dist_format = babl_format ("Y u32");
  const Babl     *aux_format  = aux ? gegl_buffer_get_format (aux) : format;
  gint            bpp         = babl_format_get_bytes_per_pixel (format);
  gint            aux_bpp     = babl_format_get_bytes_per_pixel (aux_format);
  GeglBuffer     *dist;
  guint8          mask[64];

  if (aux_bpp > sizeof (mask))
    return FALSE;

  gegl_color_get_pixel (o->mask, aux_format, mask);

  dist = gegl_buffer_new (roi, dist_format);

  gegl_parallel_distribute_range (
    roi->width, gegl_operation_get_pixels_per_thread (operation) / roi->height,
    [=] (gint x0, gint width)
    {
      guint8  *in_col;
      guint8  *out_col;
      guint32 *dist_col;
      guint8  *aux_col;
      gint     x;

      in_col   = (guint8 *) g_malloc (bpp * (roi->height + 2));
      out_col  = (guint8 *) g_malloc (bpp * roi->height);
      dist_col = g_new (guint32, roi->height);

      in_col += bpp;

      if (aux)
        aux_col = (guint8 *) g_malloc (aux_bpp * roi->height);
      else
        aux_col = in_col;

      for (x = x0; x < x0 + width; x++)
        {
          const guint8 *aux_ptr = aux_col;
          gint          state   = -1;
          gint          y0      = 0;
          gint          y;

          gegl_buffer_get (input,
                           GEGL_RECTANGLE (roi->x + x, roi->y - 1,
                                           1,          roi->height + 2),
                           1.0, format, in_col - bpp,
                           GEGL_AUTO_ROWSTRIDE, o->abyss_policy);

          if (aux)
            {
              gegl_buffer_get (aux,
                               GEGL_RECTANGLE (roi->x + x, roi->y,
                                               1,          roi->height),
                               1.0, aux_format, aux_col,
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
            }

          auto segment = [&] (gint first, gint last)
          {
            gint i;

            if (state == 0)
              {
                if (! o->seed_edges)
                  {
                    if (first == 0)
                      {
                        if (last == roi->height)
                          {
                            guint32 d = metric.distance (roi->width  +
                                                         roi->height +
                                                         1);

                            gegl_memset_pattern (dist_col, &d,
                                                 sizeof (d), roi->height);
                          }
                        else
                          {
                            gegl_memset_pattern (out_col,
                                                 in_col + last * bpp,
                                                 bpp, last);

                            for (i = 0; i < last; i++)
                              dist_col[i] = metric.distance (last - i);
                          }

                        return;
                      }
                    else if (last == roi->height)
                      {
                        last -= first;

                        gegl_memset_pattern (out_col + first       * bpp,
                                             in_col  + (first - 1) * bpp,
                                             bpp, last);

                        for (i = 0; i < last; i++)
                          dist_col[first + i] = metric.distance (i + 1);

                        return;
                      }
                  }

                gegl_memset_pattern (out_col + first       * bpp,
                                     in_col  + (first - 1) * bpp,
                                     bpp, (last - first + 1) / 2);
                gegl_memset_pattern (out_col + (first + last + 1) / 2 * bpp,
                                     in_col  + last                   * bpp,
                                     bpp, (last - first) / 2);

                for (i = 0; i < (last - first + 1) / 2; i++)
                  {
                    gint d = metric.distance (i + 1);

                    dist_col[first      + i] = d;
                    dist_col[(last - 1) - i] = d;
                  }
              }
            else if (state == 1)
              {
                memcpy (out_col + first * bpp,
                        in_col  + first * bpp,
                        (last - first) * bpp);

                memset (dist_col + first, 0,
                        (last - first) * sizeof (guint32));
              }
          };

          for (y = 0; y < roi->height; y++)
            {
              gint new_state = ((! memcmp (aux_ptr, mask, aux_bpp)) == invert);

              if (new_state != state)
                {
                  segment (y0, y);

                  state = new_state;
                  y0    = y;
                }

              aux_ptr += aux_bpp;
            }

          segment (y0, y);

          gegl_buffer_set (output,
                           GEGL_RECTANGLE (roi->x + x, roi->y, 1, roi->height),
                           0, format, out_col, GEGL_AUTO_ROWSTRIDE);
          gegl_buffer_set (dist,
                           GEGL_RECTANGLE (roi->x + x, roi->y, 1, roi->height),
                           0, dist_format, dist_col, GEGL_AUTO_ROWSTRIDE);
        }

      in_col -= bpp;

      g_free (in_col);
      g_free (out_col);
      g_free (dist_col);

      if (aux)
        g_free (aux_col);
    });

  gegl_parallel_distribute_range (
    roi->height, gegl_operation_get_pixels_per_thread (operation) / roi->width,
    [=] (gint y0, gint height)
    {
      guint8  *in_row;
      guint8  *out_row;
      guint32 *dist_row;
      gint     y;

      in_row   = (guint8 *) g_malloc (bpp * (roi->width + 2));
      out_row  = (guint8 *) g_malloc (bpp * roi->width);
      dist_row = g_new (guint32, roi->width + 2);

      in_row += bpp;

      dist_row++;
      dist_row[-1]         = 0;
      dist_row[roi->width] = 0;

      for (y = y0; y < y0 + height; y++)
        {
          gegl_buffer_get (output,
                           GEGL_RECTANGLE (roi->x,     roi->y + y,
                                           roi->width, 1),
                           1.0, format, in_row,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          if (o->seed_edges)
            {
              gegl_buffer_get (input,
                               GEGL_RECTANGLE (roi->x - 1, roi->y + y,
                                               1,          1),
                               1.0, format, in_row - bpp,
                               GEGL_AUTO_ROWSTRIDE, o->abyss_policy);
              gegl_buffer_get (input,
                               GEGL_RECTANGLE (roi->x + roi->width, roi->y + y,
                                               1,                   1),
                               1.0, format, in_row + roi->width * bpp,
                               GEGL_AUTO_ROWSTRIDE, o->abyss_policy);
            }

          gegl_buffer_get (dist,
                           GEGL_RECTANGLE (roi->x,     roi->y + y,
                                           roi->width, 1),
                           1.0, dist_format, dist_row,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          auto bisect = [&] (gint in_first,  gint in_last,
                             gint out_first, gint out_last,
                             auto bisect) -> void
          {
            if (in_last - in_first == 1)
              {
                gegl_memset_pattern (out_row + out_first * bpp,
                                     in_row  + in_first  * bpp,
                                     bpp, out_last - out_first);
              }
            else
              {
                gint cx  = (out_first + out_last) / 2;
                gint mx  = cx;
                gint md  = dist_row[mx];
                gint any = md;
                gint x;

                for (x = cx - 1; x >= in_first; x--)
                  {
                    gint dx = metric.distance (cx - x);
                    gint dy = dist_row[x];

                    if (any && md <= dx)
                      break;

                    any |= dy;

                    if (dy < md)
                      {
                        gint d = metric.distance (dx, dy);

                        if (d < md)
                          {
                            mx = x;
                            md = d;
                          }
                      }
                  }

                for (x = cx + 1; x < in_last; x++)
                  {
                    gint dx = metric.distance (x - cx);
                    gint dy = dist_row[x];

                    if (any && md <= dx)
                      break;

                    any |= dy;

                    if (dy < md)
                      {
                        gint d = metric.distance (dx, dy);

                        if (d < md)
                          {
                            mx = x;
                            md = d;
                          }
                      }
                  }

                if (! any)
                  {
                    gint first = MAX (in_first, out_first);
                    gint last  = MIN (in_last,  out_last);

                    gegl_memset_pattern (out_row + out_first * bpp,
                                         in_row  + in_first  * bpp,
                                         bpp, first - out_first);

                    memcpy (out_row + first * bpp,
                            in_row  + first * bpp,
                            (last - first) * bpp);

                    gegl_memset_pattern (out_row + last          * bpp,
                                         in_row  + (in_last - 1) * bpp,
                                         bpp, out_last - last);
                  }
                else
                  {
                    memcpy (out_row + cx * bpp, in_row + mx * bpp, bpp);

                    if (out_first < cx)
                      bisect (in_first, mx + 1, out_first, cx, bisect);
                    if (cx + 1 < out_last)
                      bisect (mx, in_last, cx + 1, out_last, bisect);
                  }
              }
          };

          if (o->seed_edges)
            bisect (-1, roi->width + 1, 0, roi->width, bisect);
          else
            bisect (0,  roi->width,     0, roi->width, bisect);

          gegl_buffer_set (output,
                           GEGL_RECTANGLE (roi->x, roi->y + y, roi->width, 1),
                           0, format, out_row, GEGL_AUTO_ROWSTRIDE);
        }

      in_row -= bpp;

      dist_row--;

      g_free (in_row);
      g_free (out_row);
      g_free (dist_row);
    });

  g_object_unref (dist);

  return TRUE;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *aux,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  switch (o->metric)
    {
    case GEGL_DISTANCE_METRIC_EUCLIDEAN:
      return process (operation, input, aux, output, roi, level,
                      EuclideanMetric ());

    case GEGL_DISTANCE_METRIC_MANHATTAN:
      return process (operation, input, aux, output, roi, level,
                      ManhattanMetric ());

    case GEGL_DISTANCE_METRIC_CHEBYSHEV:
      return process (operation, input, aux, output, roi, level,
                      ChebyshevMetric ());
    }

  g_return_val_if_reached (FALSE);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass         *operation_class;
  GeglOperationComposerClass *composer_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  composer_class  = GEGL_OPERATION_COMPOSER_CLASS (klass);

  operation_class->get_required_for_output   = get_required_for_output;
  operation_class->get_invalidated_by_change = get_invalidated_by_change;
  operation_class->get_cached_region         = get_cached_region;
  operation_class->prepare                   = prepare;

  operation_class->threaded                  = FALSE;
  operation_class->want_in_place             = TRUE;

  composer_class->process                    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",            "gegl:voronoi-diagram",
    "title",           _("Voronoi Diagram"),
    "categories",      "map",
#if 0
    "reference-chain", "color                     "
                       "noise-hurl pct-random=0.1 "
                       "crop width=256 height=256 "
                       "voronoi-diagram mask=black",
#endif
    "reference-hash",  "983f0fd7b29e1ac36721038817f4de74",
    "description",     _("Paints each non-seed pixel with the color of "
                         "the nearest seed pixel."),
    NULL);
}

#endif
