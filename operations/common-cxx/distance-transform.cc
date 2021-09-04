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

#ifdef GEGL_PROPERTIES

enum_start (gegl_distance_transform_policy)
    enum_value (GEGL_DT_ABYSS_ABOVE, "above", N_("Above threshold"))
    enum_value (GEGL_DT_ABYSS_BELOW, "below", N_("Below threshold"))
enum_end (GeglDistanceTransformPolicy)

property_enum (metric, _("Metric"),
               GeglDistanceMetric, gegl_distance_metric, GEGL_DISTANCE_METRIC_EUCLIDEAN)
    description (_("Metric to use for the distance calculation"))

property_enum (edge_handling, _("Edge handling"), GeglDistanceTransformPolicy,
               gegl_distance_transform_policy, GEGL_DT_ABYSS_BELOW)
  description (_("How areas outside the input are considered when calculating distance"))

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
#define GEGL_OP_C_SOURCE distance-transform.cc
#include "gegl-op.h"
#include <stdio.h>

#define EPSILON 0.000000000001

static gfloat edt_f   (gfloat x, gfloat i, gfloat g_i);
static gint   edt_sep (gint i, gint u, gfloat g_i, gfloat g_u);
static gfloat mdt_f   (gfloat x, gfloat i, gfloat g_i);
static gint   mdt_sep (gint i, gint u, gfloat g_i, gfloat g_u);
static gfloat cdt_f   (gfloat x, gfloat i, gfloat g_i);
static gint   cdt_sep (gint i, gint u, gfloat g_i, gfloat g_u);

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
  const GeglRectangle *in_rect =
      gegl_operation_source_get_bounding_box (operation, "input");

  if (! in_rect || gegl_rectangle_is_infinite_plane (in_rect))
    return *roi;

  return *in_rect;
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return get_cached_region (operation, roi);
}

/* Meijster helper functions for euclidean distance transform */
static gfloat
edt_f (gfloat x, gfloat i, gfloat g_i)
{
  return sqrtf ((x - i) * (x - i) + g_i * g_i);
}

static gint
edt_sep (gint i, gint u, gfloat g_i, gfloat g_u)
{
  return (u * u - i * i + ((gint) (g_u * g_u - g_i * g_i))) / (2 * (u - i));
}

/* Meijster helper functions for manhattan distance transform */
static gfloat
mdt_f (gfloat x, gfloat i, gfloat g_i)
{
  return fabsf (x - i) + g_i;
}

static gint
mdt_sep (gint i, gint u, gfloat g_i, gfloat g_u)
{
  if (g_u >= g_i + u - i + EPSILON)
    return INT32_MAX / 4;
  if (g_i > g_u + u - i + EPSILON)
    return INT32_MIN / 4;

  return (((gint) (g_u - g_i)) + u + i) / 2;
}

/* Meijster helper functions for chessboard distance transform */
static gfloat
cdt_f (gfloat x, gfloat i, gfloat g_i)
{
  gfloat absd = fabsf (x - i);
  return MAX (absd, g_i);
}

static gint
cdt_sep (gint i, gint u, gfloat g_i, gfloat g_u)
{
  if (g_i <= g_u)
    return MAX (i + (gint) g_u, (i+u)/2);
  else
    return MIN (u - (gint) g_i, (i+u)/2);
}

/* Second pass finds distance for each row by determining contiguous segments with one "minimizer" pixel each,
 * using the vertical distance information from the first pass as an input.
 * For each pixel in a segment, the minimizer gives the least distance out of any other pixel in the row when
 * using its distance from the pixel as width and the minimizer's value (from 1st pass) as height. */
static void
binary_dt_2nd_pass (GeglOperation      *operation,
                    gint                width,
                    gint                height,
                    gfloat              thres_lo,
                    GeglDistanceMetric  metric,
                    gfloat             *src,
                    gfloat             *dest)
{
  gfloat (*dt_f)   (gfloat, gfloat, gfloat);
  gint   (*dt_sep) (gint, gint, gfloat, gfloat);
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gfloat inf_dist;

  /* An impossibly large value for infinite distance, set to width + height as suggested from paper */
  inf_dist = width + height;

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

  /* Parallelize the loop. We don't even need a mutex as we edit data per
   * lines (i.e. each thread will work on a given range of lines without
   * needing to read data updated by other threads).
   */
  gegl_parallel_distribute_range (
    height, gegl_operation_get_pixels_per_thread (operation) / width,
    [&] (gint y0, gint size)
    {
      gfloat *dest_row; /* Current destination row, where final distance is output */
      gfloat *g; /* Current row of 1st pass's output, with an added abyss value on either side */
      gint q;  /* Index of the foremost segment in t and s */
      gint w;  /* Used to set the boundary of newly defined segments */
      gint *t; /* Locations for each segment's lower boundary */
      gint *s; /* Locations for each segment's minimizer */
      gint u;  /* Used for x-coordinate, named from paper */
      gint y;

      /* sorry for the variable naming, they are taken from the paper */

      s = (gint *) gegl_calloc (sizeof (gint), width + 1);
      t = (gint *) gegl_calloc (sizeof (gint), width + 1);
      g = (gfloat *) gegl_calloc (sizeof (gfloat), width + 2);

      for (y = y0; y < y0 + size; y++)
        {
          dest_row = &dest[0 + y * width];

          /* Copy over dest_row to g, and line with a zero or inf_dist on either side.
           * Mind the offset and difference in width when working between g and the dest row */
          memcpy (&g[1], dest_row, width * sizeof (gfloat));
          g[0] = g[width + 1] = (o->edge_handling == GEGL_DT_ABYSS_ABOVE) ? inf_dist : 0.0f;

          q = 0;
          s[0] = 0;
          t[0] = 0;

          /* Determine regions and their minimizers, scanning pixels left to right */
          for (u = 1; u < width + 2; u++)
            {
              /* If the scanned pixel produces a distance less than or equal to the current
               * minimizer, clear out the current segment and any others similarly affected */
              while (q >= 0 &&
                     dt_f (t[q], s[q], g[s[q]]) >= dt_f (t[q], u, g[u]) + EPSILON)
                {
                  q --;
                }

              /* Define new segment, with the currently scanned pixel as a minimizer */
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

                  if (w < width + 1)
                    {
                      q ++;
                      s[q] = u;
                      t[q] = w;
                    }
                }
            }

          /* Calculate final distances within each region from its respective minimizer */
          for (u = width; u >= 1; u--)
            {
              if (u == s[q])
                dest_row[u - 1] = g[u];
              else
                dest_row[u - 1] = dt_f (u, s[q], g[s[q]]);

              if (q > 0 && u == t[q])
                {
                  q--;
                }
            }
        }

      gegl_free (t);
      gegl_free (s);
      gegl_free (g);
    });
}


/* First pass calculates vertical distance with a simple pass down and up each column */
static void
binary_dt_1st_pass (GeglOperation *operation,
                    gint           width,
                    gint           height,
                    gfloat         thres_lo,
                    gfloat        *src,
                    gfloat        *dest)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gfloat inf_dist, edge_mult;

  /* An impossibly large value for infinite distance, set to width + height as suggested from paper */
  inf_dist = width + height;
  edge_mult = (o->edge_handling == GEGL_DT_ABYSS_ABOVE) ? inf_dist : 1.0f;

  /* Parallelize the loop. We don't even need a mutex as we edit data per
   * columns (i.e. each thread will work on a given range of columns without
   * needing to read data updated by other threads).
   */
  gegl_parallel_distribute_range (
    width, gegl_operation_get_pixels_per_thread (operation) / height,
    [&] (gint x0, gint size)
    {
      gint x;
      gint y;

      for (x = x0; x < x0 + size; x++)
        {
          y = 1;

          /* Set an initial distance for the top pixel, accounting for abyss */
          dest[x + 0 * width] = src[x + 0 * width] > thres_lo ? 1.0f * edge_mult : 0.0f;

          /* For columns starting with inf_dist, don't increment distance until first region transition */
          if (dest[x + 0 * width] > 1.0f)
            while (y < height && src[x + y * width] > thres_lo)
              {
                dest[x + y * width] = inf_dist;
                y++;
              }
          if (y == height)
            continue;

          /* Scan downwards from the top, or where the previous loop left off */
          for (; y < height; y++)
            {
              if (src[x + y * width] > thres_lo)
                dest[x + y * width] = 1.0 + dest[x + (y - 1) * width];
              else
                dest[x + y * width] = 0.0;
            }

          /* If abyss is below threshold, limit the bottom pixel's distance before we scan back up */
          if (o->edge_handling == GEGL_DT_ABYSS_BELOW)
            dest[x + (height - 1) * width] = MIN (dest[x + (height - 1) * width], 1.0f);

          for (y = height - 2; y >= 0; y--)
            {
              if (dest[x + (y + 1) * width] + 1.0f < dest[x + y * width])
                dest [x + y * width] = dest[x + (y + 1) * width] + 1.0f;
            }
        }
    });
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
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties         *o = GEGL_PROPERTIES (operation);
  const Babl  *input_format = gegl_operation_get_format (operation, "output");
  const int bytes_per_pixel = babl_format_get_bytes_per_pixel (input_format);

  GeglDistanceMetric metric;
  gint               width, height, averaging, i;
  gfloat             threshold_lo, threshold_hi, maxval, *src_buf, *dst_buf;
  gboolean           normalize;

  width  = result->width;
  height = result->height;

  threshold_lo = o->threshold_lo;
  threshold_hi = o->threshold_hi;
  normalize    = o->normalize;
  metric       = o->metric;
  averaging    = o->averaging;

  src_buf = (gfloat *) gegl_malloc (width * height * bytes_per_pixel);
  dst_buf = (gfloat *) gegl_calloc (width * height, bytes_per_pixel);

  gegl_operation_progress (operation, 0.0, (gchar *) "");

  gegl_buffer_get (input, result, 1.0, input_format, src_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  if (!averaging)
    {
      binary_dt_1st_pass (operation, width, height, threshold_lo,
                          src_buf, dst_buf);
      gegl_operation_progress (operation, 0.5, (gchar *) "");
      binary_dt_2nd_pass (operation, width, height, threshold_lo, metric,
                          src_buf, dst_buf);
    }
  else
    {
      gfloat *tmp_buf;
      gint i, j;

      tmp_buf = (gfloat *) gegl_malloc (width * height * bytes_per_pixel);

      for (i = 0; i < averaging; i++)
        {
          gfloat thres;

          thres = (i+1) * (threshold_hi - threshold_lo) / (averaging + 1);
          thres += threshold_lo;

          binary_dt_1st_pass (operation, width, height, thres,
                              src_buf, tmp_buf);
          gegl_operation_progress (operation, (i + 1) / averaging - 1 / (2 * averaging),
                                   (gchar *) "");
          binary_dt_2nd_pass (operation, width, height, thres, metric,
                              src_buf, tmp_buf);
          gegl_operation_progress (operation, (i + 1) / averaging, (gchar *) "");

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

  gegl_operation_progress (operation, 1.0, (gchar *) "");

  gegl_free (dst_buf);
  gegl_free (src_buf);

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
                                          G_OBJECT(g_object_ref (in)));
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
  const gchar              *composition =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "  <node operation='gegl:crop' width='200' height='200'/>"
    "  <node operation='gegl:over'>"
    "    <node operation='gegl:distance-transform'>"
    "      <params>"
    "        <param name='metric'>euclidean</param>"
    "        <param name='threshold_lo'>0.0001</param>"
    "        <param name='threshold_hi'>1.0</param>"
    "        <param name='averaging'>0</param>"
    "        <param name='normalize'>true</param>"
    "      </params>"
    "    </node>"
    "    <node operation='gegl:load' path='standard-input.png'/>"
    "  </node>"
    "  <node operation='gegl:checkerboard'>"
    "    <params>"
    "      <param name='color1'>rgb(0.25,0.25,0.25)</param>"
    "      <param name='color2'>rgb(0.75,0.75,0.75)</param>"
    "    </params>"
    "  </node>"    
    "</gegl>";

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->threaded                = FALSE;
  operation_class->prepare                 = prepare;
  operation_class->process                 = operation_process;
  operation_class->get_cached_region       = get_cached_region;
  operation_class->get_required_for_output = get_required_for_output;
  filter_class->process                    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:distance-transform",
    "title",       _("Distance Transform"),
    "categories",  "map",
    "reference-hash", "620bf37294bca66e4190da60c5be5622",
    "reference-composition", composition,
    "description", _("Calculate a distance transform"),
    NULL);
}

#endif
