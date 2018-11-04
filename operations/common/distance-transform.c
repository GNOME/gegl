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
 * Copyright (C) 2014 Simon Budig <simon@gimp.org>
 *
 * implemented following this paper:
 *   A. Meijster, J.B.T.M. Roerdink, and W.H. Hesselink
 *   "A General Algorithm for Computing Distance Transforms in Linear Time",
 *   Mathematical Morphology and its Applications to Image and
 *   Signal Processing, Kluwer Acad. Publ., 2000, pp. 331-340.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "gegl-config.h"

#ifdef GEGL_PROPERTIES

property_enum (metric, _("Metric"),
               GeglDistanceMetric, gegl_distance_metric, GEGL_DISTANCE_METRIC_EUCLIDEAN)
    description (_("Metric to use for the distance calculation"))

property_double (threshold_lo, _("Threshold low"), 0.0001)
  value_range (0.0, 1.0)

property_double (threshold_hi, _("Threshold high"), 1.0)
  value_range (0.0, 1.0)

property_int    (averaging, _("Grayscale Averaging"), 0)
    description (_("Number of computations for grayscale averaging"))
    value_range (0, 1000)
    ui_range    (0, 256)
    ui_gamma    (1.5)

property_boolean (normalize, _("Normalize"), TRUE)
  description(_("Normalize output to range 0.0 to 1.0."))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     distance_transform
#define GEGL_OP_C_SOURCE distance-transform.c
#include "gegl-op.h"
#include <math.h>
#include <stdio.h>

#define EPSILON 0.000000000001

gfloat edt_f   (gfloat x, gfloat i, gfloat g_i);
gint   edt_sep (gint i, gint u, gfloat g_i, gfloat g_u);
gfloat mdt_f   (gfloat x, gfloat i, gfloat g_i);
gint   mdt_sep (gint i, gint u, gfloat g_i, gfloat g_u);
gfloat cdt_f   (gfloat x, gfloat i, gfloat g_i);
gint   cdt_sep (gint i, gint u, gfloat g_i, gfloat g_u);

typedef struct ThreadUserData
{
  gint                width;
  gint                height;
  GeglDistanceMetric  metric;
  gfloat             *src;
} ThreadUserData;

typedef struct ThreadData
{
  void     (*func) (GeglOperation      *operation,
                    gint                width,
                    gint                height,
                    gfloat              thres_lo,
                    GeglDistanceMetric  metric,
                    gfloat             *src,
                    gfloat             *dest,
                    gint                start,
                    gint                end);
  gint      start;
  gint      end;
  gfloat    threshold;
  gfloat   *dest;
  gint     *pending;
} ThreadData;

static void
thread_func (gpointer data,
             gpointer user_data)
{
  ThreadUserData *ud = (ThreadUserData *) user_data;
  ThreadData     *d  = (ThreadData *) data;

  d->func (NULL, ud->width, ud->height, d->threshold,
           ud->metric, ud->src, d->dest, d->start, d->end);
  g_atomic_int_add (d->pending, -1);
}

/**
 * Prepare function of gegl filter.
 * @param operation given Gegl operation
 */
static void
prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  const Babl *format = babl_format_with_space ("Y float", space);

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

/**
 * Returns the cached region. This is an area filter, which acts on the whole image.
 * @param operation given Gegl operation
 * @param roi the rectangle of interest
 * @return result the new rectangle
 */
static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");
  return result;
}


/* Meijster helper functions for euclidean distance transform */
gfloat
edt_f (gfloat x, gfloat i, gfloat g_i)
{
  return sqrt ((x - i) * (x - i) + g_i * g_i);
}

gint
edt_sep (gint i, gint u, gfloat g_i, gfloat g_u)
{
  return (u * u - i * i + ((gint) (g_u * g_u - g_i * g_i))) / (2 * (u - i));
}

/* Meijster helper functions for manhattan distance transform */
gfloat
mdt_f (gfloat x, gfloat i, gfloat g_i)
{
  return fabs (x - i) + g_i;
}

gint
mdt_sep (gint i, gint u, gfloat g_i, gfloat g_u)
{
  if (g_u >= g_i + u - i + EPSILON)
    return INT32_MAX / 4;
  if (g_i > g_u + u - i + EPSILON)
    return INT32_MIN / 4;

  return (((gint) (g_u - g_i)) + u + i) / 2;
}

/* Meijster helper functions for chessboard distance transform */
gfloat
cdt_f (gfloat x, gfloat i, gfloat g_i)
{
  return MAX (fabs (x - i), g_i);
}

gint
cdt_sep (gint i, gint u, gfloat g_i, gfloat g_u)
{
  if (g_i <= g_u)
    return MAX (i + (gint) g_u, (i+u)/2);
  else
    return MIN (u - (gint) g_i, (i+u)/2);
}


static void
binary_dt_2nd_pass_sub (GeglOperation      *operation,
                        gint                width,
                        gint                height,
                        gfloat              thres_lo,
                        GeglDistanceMetric  metric,
                        gfloat             *src,
                        gfloat             *dest,
                        gint                start,
                        gint                end)
{
  gint u, y;
  gint q, w, *t, *s;
  gfloat *g, *row_copy;

  gfloat (*dt_f)   (gfloat, gfloat, gfloat);
  gint   (*dt_sep) (gint, gint, gfloat, gfloat);

  switch (metric)
    {
      case GEGL_DISTANCE_METRIC_CHEBYSHEV:
        dt_f   = cdt_f;
        dt_sep = cdt_sep;
        break;
      case GEGL_DISTANCE_METRIC_MANHATTAN:
        dt_f   = mdt_f;
        dt_sep = mdt_sep;
        break;
      default: /* GEGL_DISTANCE_METRIC_EUCLIDEAN */
        dt_f   = edt_f;
        dt_sep = edt_sep;
        break;
    }

  /* sorry for the variable naming, they are taken from the paper */

  s = gegl_calloc (sizeof (gint), width);
  t = gegl_calloc (sizeof (gint), width);
  row_copy = gegl_calloc (sizeof (gfloat), width);

  for (y = start; y < end; y++)
    {
      q = 0;
      s[0] = 0;
      t[0] = 0;
      g = dest + y * width;

      dest[0 + y * width] = MIN (dest[0 + y * width], 1.0);
      dest[width - 1 + y * width] = MIN (dest[width - 1 + y * width], 1.0);

      for (u = 1; u < width; u++)
        {
          while (q >= 0 &&
                 dt_f (t[q], s[q], g[s[q]]) >= dt_f (t[q], u, g[u]) + EPSILON)
            {
              q --;
            }

          if (q < 0)
            {
              q = 0;
              s[0] = u;
            }
          else
            {
              /* function Sep from paper */
              w = dt_sep (s[q], u, g[s[q]], g[u]);
              w += 1;

              if (w < width)
                {
                  q ++;
                  s[q] = u;
                  t[q] = w;
                }
            }
        }

      memcpy (row_copy, g, width * sizeof (gfloat));

      for (u = width - 1; u >= 0; u--)
        {
          if (u == s[q])
            g[u] = row_copy[u];
          else
            g[u] = dt_f (u, s[q], row_copy[s[q]]);

          if (q > 0 && u == t[q])
            {
              q--;
            }
        }

      if (operation)
        gegl_operation_progress (operation,
                                 (gdouble) y / (gdouble) height / 2.0 + 0.5, "");
    }

  gegl_free (t);
  gegl_free (s);
  gegl_free (row_copy);
}

static void
binary_dt_2nd_pass (GeglOperation      *operation,
                    GThreadPool        *pool,
                    gint                width,
                    gint                height,
                    gfloat              thres_lo,
                    GeglDistanceMetric  metric,
                    gfloat             *src,
                    gfloat             *dest,
                    gint                threads)
{
  ThreadData data[GEGL_MAX_THREADS];

  if (pool)
    {
      gint subheight = height / (threads + 1);
      gint i;
      gint y = 0;
      gint pending = threads;

      for (i = 0; i < threads; i++)
        {
          data[i].func      = binary_dt_2nd_pass_sub;
          data[i].start     = y;
          data[i].end       = y + subheight;
          data[i].threshold = thres_lo;
          data[i].dest      = dest;
          data[i].pending   = &pending;

          y = data[i].end;

          g_thread_pool_push (pool, &data[i], NULL);
        }
      binary_dt_2nd_pass_sub (NULL, width, height, thres_lo, metric,
                              src, dest, y, height);
      while (g_atomic_int_get (&pending))
        {
          gegl_operation_progress (operation,
                                   (gdouble) (threads - pending) / (gdouble) threads / 2.0 + 0.5, "");
        };
    }
  else
    {
      binary_dt_2nd_pass_sub (operation, width, height, thres_lo,
                              metric, src, dest, 0, height);
    }
}


static void
binary_dt_1st_pass_sub (GeglOperation      *operation,
                        gint                width,
                        gint                height,
                        gfloat              thres_lo,
                        GeglDistanceMetric  metric G_GNUC_UNUSED,
                        gfloat             *src,
                        gfloat             *dest,
                        gint                start,
                        gint                end)
{
  int x, y;

  for (x = start; x < end; x++)
    {
      /* consider out-of-range as 0, i.e. the outside is "empty" */
      dest[x + 0 * width] = src[x + 0 * width] > thres_lo ? 1.0 : 0.0;

      for (y = 1; y < height; y++)
        {
          if (src[x + y * width] > thres_lo)
            dest[x + y * width] = 1.0 + dest[x + (y - 1) * width];
          else
            dest[x + y * width] = 0.0;
        }

      dest[x + (height - 1) * width] = MIN (dest[x + (height - 1) * width], 1.0);
      for (y = height - 2; y >= 0; y--)
        {
          if (dest[x + (y + 1) * width] + 1.0 < dest[x + y * width])
            dest [x + y * width] = dest[x + (y + 1) * width] + 1.0;
        }
      if (operation)
        gegl_operation_progress (operation,
                                 (gdouble) x / (gdouble) width / 2.0, "");
    }
}

static void
binary_dt_1st_pass (GeglOperation      *operation,
                    GThreadPool        *pool,
                    gint                width,
                    gint                height,
                    gfloat              thres_lo,
                    GeglDistanceMetric  metric,
                    gfloat             *src,
                    gfloat             *dest,
                    gint                threads)
{
  ThreadData data[GEGL_MAX_THREADS];

  if (pool)
    {
      gint subwidth = width / (threads + 1);
      gint i;
      gint x = 0;
      gint pending = threads;

      for (i = 0; i < threads; i++)
        {
          data[i].func      = binary_dt_1st_pass_sub;
          data[i].start     = x;
          data[i].end       = x + subwidth;
          data[i].threshold = thres_lo;
          data[i].dest      = dest;
          data[i].pending   = &pending;

          x = data[i].end;

          g_thread_pool_push (pool, &data[i], NULL);
        }
      binary_dt_1st_pass_sub (NULL, width, height, thres_lo, metric,
                              src, dest, x, width);
      while (g_atomic_int_get (&pending))
        {
          gegl_operation_progress (operation,
                                   (gdouble) (threads - pending) / (gdouble) threads / 2.0, "");
        }
      gegl_operation_progress (operation, 0.5, "");
    }
  else
    {
      binary_dt_1st_pass_sub (operation, width, height, thres_lo,
                              metric, src, dest, 0, width);
    }
}

/**
 * Process the gegl filter
 * @param operation the given Gegl operation
 * @param input the input buffer.
 * @param output the output buffer.
 * @param result the region of interest.
 * @param level the level of detail
 * @return True, if the filter was successfully applied.
 */
static gboolean
process (GeglOperation        *operation,
         GeglOperationContext *context,
         const gchar          *output_prop,
         const GeglRectangle  *result,
         gint                  level)
{
  GeglProperties     *o               = GEGL_PROPERTIES (operation);
  GeglBuffer         *input           = NULL;
  const Babl         *input_format    = gegl_operation_get_format (operation, "input");
  const int           bytes_per_pixel = babl_format_get_bytes_per_pixel (input_format);
  GeglBuffer         *output;
  GThreadPool        *pool            = NULL;
  gint                threads         = gegl_config_threads () - 1;
  ThreadUserData      user_data;
  GeglDistanceMetric  metric;
  gint                width, height, averaging, i;
  gfloat              threshold_lo, threshold_hi, maxval, *src_buf, *dst_buf;
  gboolean            normalize;

  input  = (GeglBuffer*) gegl_operation_context_get_object (context, "input");
  output = gegl_operation_context_get_target (context, "output");

  width  = result->width;
  height = result->height;

  threshold_lo = o->threshold_lo;
  threshold_hi = o->threshold_hi;
  normalize    = o->normalize;
  metric       = o->metric;
  averaging    = o->averaging;

  src_buf = gegl_malloc (width * height * bytes_per_pixel);
  dst_buf = gegl_calloc (width * height, bytes_per_pixel);

  gegl_operation_progress (operation, 0.0, "");

  gegl_buffer_get (input, result, 1.0, input_format, src_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  if (gegl_operation_use_threading (operation, result) &&
      width / (threads + 1) > 0 && height / (threads + 1) > 0)
    {
      user_data.width  = width;
      user_data.height = height;
      user_data.metric = metric;
      user_data.src    = src_buf;

      pool = g_thread_pool_new (thread_func, &user_data, threads, FALSE, NULL);
    }

  if (!averaging)
    {
      binary_dt_1st_pass (operation, pool, width, height, threshold_lo, metric,
                          src_buf, dst_buf, threads);
      binary_dt_2nd_pass (operation, pool, width, height, threshold_lo, metric,
                          src_buf, dst_buf, threads);
    }
  else
    {
      gfloat *tmp_buf;
      gint i, j;

      tmp_buf = gegl_malloc (width * height * bytes_per_pixel);

      for (i = 0; i < averaging; i++)
        {
          gfloat thres;

          thres = (i+1) * (threshold_hi - threshold_lo) / (averaging + 1);
          thres += threshold_lo;

          binary_dt_1st_pass (operation, pool, width, height, thres, metric,
                              src_buf, tmp_buf, threads);
          binary_dt_2nd_pass (operation, pool, width, height, thres, metric,
                              src_buf, tmp_buf, threads);

          for (j = 0; j < width * height; j++)
            dst_buf[j] += tmp_buf[j];
        }

      gegl_free (tmp_buf);
    }

  if (normalize)
    {
      maxval = EPSILON;

      for (i = 0; i < width * height; i++)
        maxval = MAX (dst_buf[i], maxval);
    }
  else
    {
      maxval = averaging;
    }

  if (averaging > 0 || normalize)
    {
      for (i = 0; i < width * height; i++)
        dst_buf[i] = dst_buf[i] * threshold_hi / maxval;
    }

  gegl_buffer_set (output, result, 0, input_format, dst_buf,
                   GEGL_AUTO_ROWSTRIDE);

  gegl_operation_progress (operation, 1.0, "");

  gegl_free (dst_buf);
  gegl_free (src_buf);
  if (pool)
    g_thread_pool_free (pool, TRUE, FALSE);

  return TRUE;
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  gchar                    *composition =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "<node operation='gegl:distance-transform'>"
    "  <params>"
    "    <param name='metric'>euclidean</param>"
    "    <param name='threshold_lo'>0.0001</param>"
    "    <param name='threshold_hi'>1.0</param>"
    "    <param name='averaging'>0</param>"
    "    <param name='normalize'>true</param>"
    "  </params>"
    "</node>"
    "<node operation='gegl:load'>"
    "  <params>"
    "    <param name='path'>standard-input.png</param>"
    "  </params>"
    "</node>"
    "</gegl>";

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->threaded          = TRUE;
  operation_class->prepare           = prepare;
  operation_class->get_cached_region = get_cached_region;
  operation_class->process           = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:distance-transform",
    "title",       _("Distance Transform"),
    "reference-hash", "31dd3c9b78a79583db929b0f77a56191",
    "categories",  "map",
    "description", _("Calculate a distance transform"),
    "reference-composition", composition,
    NULL);
}

#endif
