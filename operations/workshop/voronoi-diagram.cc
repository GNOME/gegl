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
  prepare_y (gint y)
  {
    return y;
  }

  static gint
  no_intersection ()
  {
    return G_MAXINT / 2;
  }
};

struct EuclideanMetric : BasicMetric
{
  static gint
  prepare_y (gint y)
  {
    return y * y;
  }

  static gint
  distance (gint x,
            gint y2)
  {
    return x * x + y2;
  }

  static gint
  intersection (gint x_1,
                gint y2_1,
                gint x_2,
                gint y2_2)
  {
    return (distance (x_2, y2_2) - distance (x_1, y2_1) + (x_1 - x_2)) /
           (2 * (x_1 - x_2));
  }
};

struct ManhattanMetric : BasicMetric
{
  static gint
  distance (gint x,
            gint y)
  {
    return x + y;
  }

  static gint
  intersection (gint x_1,
                gint y_1,
                gint x_2,
                gint y_2)
  {
    if (y_2 - y_1 <= x_1 - x_2)
      return 0;
    else
      return no_intersection ();
  }
};

struct ChebyshevMetric : BasicMetric
{
  static gint
  distance (gint x,
            gint y)
  {
    return MAX (x, y);
  }

  static gint
  intersection (gint x_1,
                gint y_1,
                gint x_2,
                gint y_2)
  {
    if (y_2 <= y_1)
      return 0;
    else
      return y_2 - x_1;
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
      guint8  *in_row;
      guint8  *out_row;
      guint32 *dist_row;
      guint8  *aux_row;
      gint     x;

      in_row   = (guint8 *) g_malloc (bpp * (roi->height + 2));
      out_row  = (guint8 *) g_malloc (bpp * roi->height);
      dist_row = g_new (guint32, roi->height);

      in_row += bpp;

      if (aux)
        aux_row = (guint8 *) g_malloc (bpp * roi->height);
      else
        aux_row = in_row;

      for (x = x0; x < x0 + width; x++)
        {
          gegl_buffer_get (input,
                           GEGL_RECTANGLE (roi->x + x, roi->y - 1,
                                           1,          roi->height + 2),
                           1.0, format, in_row - bpp,
                           GEGL_AUTO_ROWSTRIDE, o->abyss_policy);

          if (aux)
            {
              gegl_buffer_get (aux,
                               GEGL_RECTANGLE (roi->x + x, roi->y,
                                               1,          roi->height),
                               1.0, aux_format, aux_row,
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
            }

          auto pass = [&] (gint first, gint last, gint step)
          {
            gint          d = o->seed_edges ? 0 : roi->width + roi->height + 1;
            const guint8 *p = in_row + (first - step) * bpp;
            gint          y;

            for (y = first; y != last; y += step)
              {
                gint dy;

                if ((! memcmp (aux_row + y * bpp, mask, aux_bpp)) == invert)
                  {
                    d = 0;
                    p = in_row + y * bpp;
                  }
                else
                  {
                    d++;
                  }

                dy = metric.prepare_y (d);

                if (step > 0 || dy < dist_row[y])
                  {
                    memcpy (out_row + y * bpp, p, bpp);

                    dist_row[y] = dy;
                  }
              }
          };

          pass (0,               roi->height, +1);
          pass (roi->height - 1, -1,          -1);

          gegl_buffer_set (output,
                           GEGL_RECTANGLE (roi->x + x, roi->y, 1, roi->height),
                           0, format, out_row, GEGL_AUTO_ROWSTRIDE);
          gegl_buffer_set (dist,
                           GEGL_RECTANGLE (roi->x + x, roi->y, 1, roi->height),
                           0, dist_format, dist_row, GEGL_AUTO_ROWSTRIDE);
        }

      in_row -= bpp;

      g_free (in_row);
      g_free (out_row);
      g_free (dist_row);

      if (aux)
        g_free (aux_row);
    });

  gegl_parallel_distribute_range (
    roi->height, gegl_operation_get_pixels_per_thread (operation) / roi->width,
    [=] (gint y0, gint height)
    {
      guint8  *in_row;
      guint8  *out_row;
      guint32 *dist_row;
      gint    *queue;
      gint    *hdist;
      gint     y;

      in_row   = (guint8 *) g_malloc (bpp * (roi->width + 2));
      out_row  = (guint8 *) g_malloc (bpp * roi->width);
      dist_row = g_new (guint32, roi->width);

      in_row += bpp;

      queue = g_new (gint, roi->width);
      hdist = g_new (gint, roi->width);

      for (y = y0; y < y0 + height; y++)
        {
          gegl_buffer_get (output,
                           GEGL_RECTANGLE (roi->x - 1,     roi->y + y,
                                           roi->width + 2, 1),
                           1.0, format, in_row - bpp,
                           GEGL_AUTO_ROWSTRIDE, o->abyss_policy);
          gegl_buffer_get (dist,
                           GEGL_RECTANGLE (roi->x,     roi->y + y,
                                           roi->width, 1),
                           1.0, dist_format, dist_row,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          auto pass = [&] (gint first, gint last, gint step)
          {
            gint          dx;
            gint          dy;
            const guint8 *p;
            gint          x;

            dx = o->seed_edges ? 0 : roi->width + 1;
            dy = metric.prepare_y (o->seed_edges ? 0 : roi->height + 1);
            p  = in_row + (first - step) * bpp;

            memset (queue, 0, roi->width * sizeof (gint));

            for (x = first; x != last; x += step)
              {
                gint d;

                if (dist_row[x] == 0)
                  {
                    dx = 0;
                    dy = metric.prepare_y (0);
                    p  = in_row + x * bpp;
                  }
                else
                  {
                    gint qx = queue[x] - 1;
                    gint dv;
                    gint n;

                    dx++;

                    if (qx >= 0)
                      {
                        gint dh = (x - qx) * step;

                        if (dh < dx)
                          {
                            dv = dist_row[qx];

                            n = metric.intersection (dx, dy, dh, dv);

                            if (n <= 0)
                              {
                                dx = dh;
                                dy = dv;
                                p  = in_row + qx * bpp;
                              }
                            else if (n < (last - x) * step)
                              {
                                queue[x + n * step] = qx + 1;
                              }
                          }
                      }

                    dv = dist_row[x];

                    n = metric.intersection (dx, dy, 0, dv);

                    if (n <= 0)
                      {
                        dx = 0;
                        dy = dv;
                        p  = in_row + x * bpp;
                      }
                    else if (n < (last - x) * step)
                      {
                        queue[x + n * step] = x + 1;
                      }
                  }

                d = metric.distance (dx, dy);

                if (step > 0 || d < hdist[x])
                  {
                    memcpy (out_row + x * bpp, p, bpp);

                    hdist[x] = d;
                  }
              }
          };

          pass (0,              roi->width, +1);
          pass (roi->width - 1, -1,         -1);

          gegl_buffer_set (output,
                           GEGL_RECTANGLE (roi->x, roi->y + y, roi->width, 1),
                           0, format, out_row, GEGL_AUTO_ROWSTRIDE);
        }

      in_row -= bpp;

      g_free (in_row);
      g_free (out_row);
      g_free (dist_row);

      g_free (queue);
      g_free (hdist);
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
    "reference-chain", "color                     "
                       "noise-hurl pct-random=0.1 "
                       "crop width=256 height=256 "
                       "voronoi-diagram mask=black",
    "reference-hash",  "0731590098ed020b5a9e7a71b71735dc",
    "description",     _("Paints each non-seed pixel with the color of "
                         "the nearest seed pixel."),
    NULL);
}

#endif
