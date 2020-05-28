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
 * Copyright 2011 Michael Muré <batolettre@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_warp_behavior)
  enum_value (GEGL_WARP_BEHAVIOR_MOVE,      "move",      N_("Move pixels"))
  enum_value (GEGL_WARP_BEHAVIOR_GROW,      "grow",      N_("Grow area"))
  enum_value (GEGL_WARP_BEHAVIOR_SHRINK,    "shrink",    N_("Shrink area"))
  enum_value (GEGL_WARP_BEHAVIOR_SWIRL_CW,  "swirl-cw",  N_("Swirl clockwise"))
  enum_value (GEGL_WARP_BEHAVIOR_SWIRL_CCW, "swirl-ccw", N_("Swirl counter-clockwise"))
  enum_value (GEGL_WARP_BEHAVIOR_ERASE,     "erase",     N_("Erase warping"))
  enum_value (GEGL_WARP_BEHAVIOR_SMOOTH,    "smooth",    N_("Smooth warping"))
enum_end (GeglWarpBehavior)

property_double (strength, _("Strength"), 50)
  value_range (0, 100)

property_double (size, _("Size"), 40.0)
  value_range (1.0, 10000.0)

property_double (hardness, _("Hardness"), 0.5)
  value_range (0.0, 1.0)

property_double (spacing, _("Spacing"), 0.01)
  value_range (0.0, 100.0)

property_path   (stroke, _("Stroke"), NULL)

property_enum   (behavior, _("Behavior"),
                   GeglWarpBehavior, gegl_warp_behavior,
                   GEGL_WARP_BEHAVIOR_MOVE)
  description   (_("Behavior of the op"))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_C_SOURCE warp.cc
#define GEGL_OP_NAME     warp

#include "gegl-plugin.h"
#include "gegl-path.h"

static void node_invalidated (GeglNode            *node,
                              const GeglRectangle *roi,
                              GeglOperation       *operation);

static void path_changed     (GeglPath            *path,
                              const GeglRectangle *roi,
                              GeglOperation       *operation);

#include "gegl-op.h"

#define HARDNESS_EPSILON 0.0000004

typedef struct WarpPointList
{
  GeglPathPoint         point;
  struct WarpPointList *next;
} WarpPointList;

typedef struct
{
  gfloat         *lookup;
  GeglBuffer     *buffer;
  WarpPointList  *processed_stroke;
  WarpPointList **processed_stroke_tail;
  gboolean        processed_stroke_valid;
  GeglPathList   *remaining_stroke;
  gfloat          last_x;
  gfloat          last_y;
} WarpPrivate;

static void
clear_cache (GeglProperties *o)
{
  WarpPrivate *priv = (WarpPrivate *) o->user_data;

  if (! priv)
    return;

  g_clear_pointer (&priv->lookup, g_free);
  g_clear_object (&priv->buffer);

  while (priv->processed_stroke)
    {
      WarpPointList *next = priv->processed_stroke->next;

      g_slice_free (WarpPointList, priv->processed_stroke);

      priv->processed_stroke = next;
    }

  priv->processed_stroke_tail = &priv->processed_stroke;

  priv->processed_stroke_valid = TRUE;

  priv->remaining_stroke = o->stroke ? gegl_path_get_path (o->stroke) : NULL;
}

static void
validate_processed_stroke (GeglProperties *o)
{
  WarpPrivate   *priv = (WarpPrivate *) o->user_data;
  GeglPathList  *event;
  WarpPointList *processed_event;

  if (priv->processed_stroke_valid)
    return;

  /* check if the previously processed stroke is an initial segment of the
   * current stroke.
   */
  for (event           = o->stroke ? gegl_path_get_path (o->stroke) : NULL,
       processed_event = priv->processed_stroke;

       event && processed_event;

       event           = event->next,
       processed_event = processed_event->next)
    {
      if (event->d.point[0].x != processed_event->point.x ||
          event->d.point[0].y != processed_event->point.y)
        {
          break;
        }
    }

  if (! processed_event)
    {
      /* it is.  prepare for processing the remaining portion of the stroke on
       * the next call to process().
       */
      priv->remaining_stroke = event;

      priv->processed_stroke_valid = TRUE;
    }
  else
    {
      /* it isn't.  clear the cache so that we start from scratch. */
      clear_cache (o);
    }
}

static void
node_invalidated (GeglNode            *node,
                  const GeglRectangle *rect,
                  GeglOperation       *operation)
{
  /* if the node is invalidated, clear all cached data.  in particular, redraw
   * the entire stroke upon the next call to process().
   */
  clear_cache (GEGL_PROPERTIES (operation));
}

/* return the smallest range of pixels [min_pixel, max_pixel], whose centers
 * are inside the range [min_coord, max_coord].
 */
static inline void
pixel_range (gfloat  min_coord,
             gfloat  max_coord,
             gint   *min_pixel,
             gint   *max_pixel)
{
  *min_pixel = ceilf  (min_coord - 0.5f);
  *max_pixel = floorf (max_coord - 0.5f);
}

/* return the smallest rectangle of pixels, whose centers are inside the
 * horizontal range [min_x, max_x] and the vertical range [min_y, max_y].
 */
static inline GeglRectangle
pixel_extent (gfloat min_x,
              gfloat max_x,
              gfloat min_y,
              gfloat max_y)
{
  GeglRectangle result;

  pixel_range (min_x,     max_x,
               &result.x, &result.width);
  result.width -= result.x - 1;

  pixel_range (min_y,     max_y,
               &result.y, &result.height);
  result.height -= result.y - 1;

  return result;
}

static void
path_changed (GeglPath            *path,
              const GeglRectangle *roi,
              GeglOperation       *operation)
{
  GeglRectangle   rect;
  GeglProperties *o    = GEGL_PROPERTIES (operation);
  WarpPrivate    *priv = (WarpPrivate *) o->user_data;

  /* mark the previously processed stroke as invalid, so that we check it
   * against the new stroke before processing.
   */
  if (priv)
    priv->processed_stroke_valid = FALSE;

  /* invalidate the incoming rectangle */

  rect = pixel_extent (roi->x               - o->size / 2.0,
                       roi->x + roi->width  + o->size / 2.0,
                       roi->y               - o->size / 2.0,
                       roi->y + roi->height + o->size / 2.0);

  /* avoid clearing the cache.  it will be cleared, if necessary, when
   * validating the stroke.
   */
  g_signal_handlers_block_by_func (operation->node,
                                   (gpointer) node_invalidated, operation);

  gegl_operation_invalidate (operation, &rect, FALSE);

  g_signal_handlers_unblock_by_func (operation->node,
                                     (gpointer) node_invalidated, operation);
}

static gdouble
gauss (gdouble f)
{
  /* This is not a real gauss function. */
  /* Approximation is valid if -1 < f < 1 */
  if (f < -1.0)
    return 0.0;

  if (f < -0.5)
    {
      f = -1.0 - f;
      return (2.0 * f*f);
    }

  if (f < 0.5)
    return (1.0 - 2.0 * f*f);

  if (f < 1.0)
    {
      f = 1.0 - f;
      return (2.0 * f*f);
    }

  return 0.0;
}

/* set up lookup table */
static void
calc_lut (GeglProperties  *o)
{
  WarpPrivate  *priv = (WarpPrivate*) o->user_data;
  gdouble       radius;
  gint          length;
  gint          x;
  gdouble       exponent;

  if (priv->lookup)
    return;

  radius = o->size / 2.0;

  length = floor (radius) + 3;

  priv->lookup = g_new (gfloat, length);

  if (1.0 - o->hardness > HARDNESS_EPSILON)
    {
      exponent = 0.4 / (1.0 - o->hardness);

      for (x = 0; x < length; x++)
        {
          priv->lookup[x] = gauss (pow (x / radius, exponent));
        }
    }
  else
    {
      for (x = 0; x < length; x++)
        {
          priv->lookup[x] = 1.0f;
        }
    }
}

static void
attach (GeglOperation *operation)
{
  GEGL_OPERATION_CLASS (gegl_op_parent_class)->attach (operation);

  g_signal_connect_object (operation->node, "invalidated",
                           G_CALLBACK (node_invalidated), operation,
                           (GConnectFlags) 0);
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  const Babl *format = babl_format_n (babl_type ("float"), 2);
  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);

  if (!o->user_data)
    {
      o->user_data = g_slice_new0 (WarpPrivate);

      clear_cache (o);
    }

  validate_processed_stroke (o);
  calc_lut (o);
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *output_roi)
{
  GeglProperties *o    = GEGL_PROPERTIES (operation);
  WarpPrivate    *priv = (WarpPrivate*) o->user_data;
  GeglRectangle   rect = {0, 0, 0, 0};

  /* we only need the input if we don't have a cached buffer already. */
  if (! priv->buffer)
    rect = *gegl_operation_source_get_bounding_box (operation, input_pad);

  return rect;
}

static void
finalize (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);

  if (o->user_data)
    {
      clear_cache (o);

      g_slice_free (WarpPrivate, o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static inline gfloat
get_stamp_force (gfloat        x,
                 gfloat        y,
                 const gfloat *lookup)
{
  gfloat radius;
  gint   a;
  gfloat ratio;
  gfloat before, after;

  radius = sqrtf (x * x + y * y);

  /* linear interpolation */
  a = (gint) radius;
  ratio = (radius - a);

  before = lookup[a];
  after = lookup[a + 1];

  return before + ratio * (after - before);
}

static void
stamp (GeglOperation       *operation,
       GeglProperties      *o,
       gfloat              *srcbuf,
       gint                 srcbuf_stride,
       const GeglRectangle *srcbuf_extent,
       gfloat               x,
       gfloat               y)
{
  WarpPrivate   *priv = (WarpPrivate*) o->user_data;
  gfloat         x_mean = 0.0;
  gfloat         y_mean = 0.0;
  GeglRectangle  area;
  gint           sample_min_x, sample_max_x;
  gint           sample_min_y, sample_max_y;
  gfloat        *stampbuf;
  gfloat         stamp_radius_sq = 0.25 * o->size * o->size;
  gfloat         strength = 0.01 * o->strength;
  const gfloat  *lookup = priv->lookup;
  gfloat         s = 0, c = 0;
  gfloat         motion_x, motion_y;

  motion_x = priv->last_x - x;
  motion_y = priv->last_y - y;

  /* Memorize the stamp location for movement dependant behavior like move */
  priv->last_x = x;
  priv->last_y = y;

  if (o->behavior == GEGL_WARP_BEHAVIOR_MOVE &&
      motion_x == 0.0f && motion_y == 0.0f)
    {
      return;
    }

  area = pixel_extent (x - o->size / 2.0, x + o->size / 2.0,
                       y - o->size / 2.0, y + o->size / 2.0);

  if (! gegl_rectangle_intersect (&area, &area, srcbuf_extent))
    return;

  /* Shift the coordiantes so that they're relative to the top-left corner of
   * the stamped area.
   */
  x -= area.x;
  y -= area.y;

  /* Shift the stamped area so that it's relative to the top-left corner of the
   * source buffer.
   */
  area.x -= srcbuf_extent->x;
  area.y -= srcbuf_extent->y;

  /* Align the source buffer with the stamped area */
  srcbuf += srcbuf_stride * area.y + 2 * area.x;

  /* Calculate the sample bounds.  We clamp the coordinates of pixels sampled
   * from the source buffer to these limits.
   */
  sample_min_x = -area.x;
  sample_max_x = -area.x + srcbuf_extent->width - 1;
  sample_min_y = -area.y;
  sample_max_y = -area.y + srcbuf_extent->height - 1;

  /* If needed, compute the mean deformation */
  if (o->behavior == GEGL_WARP_BEHAVIOR_SMOOTH)
    {
      gfloat total_weight = 0.0f;

      gegl_parallel_distribute_range (
        area.height, gegl_operation_get_pixels_per_thread (operation) /
                     area.width,
        [&] (gint y0, gint height)
        {
          static GMutex mutex;
          gfloat        local_x_mean       = 0.0f;
          gfloat        local_y_mean       = 0.0f;
          gfloat        local_total_weight = 0.0f;
          gfloat        yi;
          gint          y_iter;

          yi = -y + y0 + 0.5f;

          for (y_iter = y0; y_iter < y0 + height; y_iter++, yi++)
            {
              gfloat  lim;
              gint    min_x, max_x;
              gfloat *srcvals;
              gfloat  xi;
              gint    x_iter;

              lim = stamp_radius_sq - yi * yi;

              if (lim < 0.0f)
                continue;

              lim = sqrtf (lim);

              pixel_range (x - lim, x + lim,
                           &min_x,  &max_x);

              if (max_x < 0 || min_x >= area.width)
                continue;

              min_x = CLAMP (min_x, 0, area.width - 1);
              max_x = CLAMP (max_x, 0, area.width - 1);

              srcvals = srcbuf + srcbuf_stride * y_iter + 2 * min_x;

              xi = -x + min_x + 0.5f;

              for (x_iter  = min_x;
                   x_iter <= max_x;
                   x_iter++, xi++, srcvals += 2)
                {
                  gfloat stamp_force = get_stamp_force (xi, yi, lookup);

                  local_x_mean += stamp_force * srcvals[0];
                  local_y_mean += stamp_force * srcvals[1];

                  local_total_weight += stamp_force;
                }
            }

          g_mutex_lock (&mutex);

          x_mean       += local_x_mean;
          y_mean       += local_y_mean;
          total_weight += local_total_weight;

          g_mutex_unlock (&mutex);
        });

      x_mean /= total_weight;
      y_mean /= total_weight;
    }
  else if (o->behavior == GEGL_WARP_BEHAVIOR_GROW ||
           o->behavior == GEGL_WARP_BEHAVIOR_SHRINK)
    {
      strength *= 0.1f;

      if (o->behavior == GEGL_WARP_BEHAVIOR_GROW)
        strength = -strength;
    }
  else if (o->behavior == GEGL_WARP_BEHAVIOR_SWIRL_CW ||
           o->behavior == GEGL_WARP_BEHAVIOR_SWIRL_CCW)
    {
      /* swirl by 5 degrees per stamp (for strength 100).
       * not exactly sin/cos factors,
       * since we calculate an off-center offset-vector */

      /* note that this is fudged for stamp_force < 1.0 and
       * results in a slight upscaling there. It is a compromise
       * between exactness and calculation speed. */
      s = sin (0.01 * o->strength * 5.0 / 180 * G_PI);
      c = cos (0.01 * o->strength * 5.0 / 180 * G_PI) - 1.0;

      if (o->behavior == GEGL_WARP_BEHAVIOR_SWIRL_CW)
        s = -s;
    }

  /* We render the stamp into a temporary buffer, to avoid overwriting data
   * that is still needed.
   */
  stampbuf = g_new (gfloat, 2 * area.height * area.width);

  gegl_parallel_distribute_range (
    area.height, gegl_operation_get_pixels_per_thread (operation) / area.width,
    [=] (gint y0, gint height)
    {
      gfloat yi;
      gint   y_iter;

      yi = -y + y0 + 0.5f;

      for (y_iter = y0; y_iter < y0 + height; y_iter++, yi++)
        {
          gfloat  lim;
          gint    min_x, max_x;
          gfloat *vals;
          gfloat *srcvals;
          gfloat  xi;
          gint    x_iter;

          lim = stamp_radius_sq - yi * yi;

          if (lim < 0.0f)
            continue;

          lim = sqrtf (lim);

          pixel_range (x - lim, x + lim,
                       &min_x,  &max_x);

          if (max_x < 0 || min_x >= area.width)
            continue;

          min_x = CLAMP (min_x, 0, area.width - 1);
          max_x = CLAMP (max_x, 0, area.width - 1);

          vals    = stampbuf + 2 * area.width * y_iter + 2 * min_x;
          srcvals = srcbuf   + srcbuf_stride  * y_iter + 2 * min_x;

          xi = -x + min_x + 0.5f;

          for (x_iter  = min_x;
               x_iter <= max_x;
               x_iter++, xi++, vals += 2, srcvals += 2)
            {
              gfloat nvx, nvy;
              gfloat fx, fy;
              gint   dx, dy;
              gfloat weight_x, weight_y;
              gfloat a0, b0;
              gfloat a1, b1;
              gfloat *srcptr;

              gfloat stamp_force = get_stamp_force (xi, yi, lookup);
              gfloat influence   = strength * stamp_force;

              switch (o->behavior)
                {
                  case GEGL_WARP_BEHAVIOR_MOVE:
                    nvx = influence * motion_x;
                    nvy = influence * motion_y;
                    break;
                  case GEGL_WARP_BEHAVIOR_GROW:
                  case GEGL_WARP_BEHAVIOR_SHRINK:
                    nvx = influence * xi;
                    nvy = influence * yi;
                    break;
                  case GEGL_WARP_BEHAVIOR_SWIRL_CW:
                  case GEGL_WARP_BEHAVIOR_SWIRL_CCW:
                    nvx = stamp_force * ( c * xi - s * yi);
                    nvy = stamp_force * ( s * xi + c * yi);
                    break;
                  case GEGL_WARP_BEHAVIOR_ERASE:
                  case GEGL_WARP_BEHAVIOR_SMOOTH:
                  default:
                    /* shut up, gcc */
                    nvx = 0.0f;
                    nvy = 0.0f;
                    break;
                }

              if (o->behavior == GEGL_WARP_BEHAVIOR_ERASE)
                {
                  vals[0] = srcvals[0] * (1.0f - influence);
                  vals[1] = srcvals[1] * (1.0f - influence);
                }
              else if (o->behavior == GEGL_WARP_BEHAVIOR_SMOOTH)
                {
                  vals[0] = srcvals[0] + influence * (x_mean - srcvals[0]);
                  vals[1] = srcvals[1] + influence * (y_mean - srcvals[1]);
                }
              else
                {
                  fx = floorf (nvx);
                  fy = floorf (nvy);

                  weight_x = nvx - fx;
                  weight_y = nvy - fy;

                  dx = fx;
                  dy = fy;

                  dx += x_iter;
                  dy += y_iter;

                  /* clamp the sampled coordinates to the sample bounds */
                  if (dx < sample_min_x || dx >= sample_max_x ||
                      dy < sample_min_y || dy >= sample_max_y)
                    {
                      if (dx < sample_min_x)
                        {
                          dx = sample_min_x;
                          weight_x = 0.0f;
                        }
                      else if (dx >= sample_max_x)
                        {
                          dx = sample_max_x;
                          weight_x = 0.0f;
                        }

                      if (dy < sample_min_y)
                        {
                          dy = sample_min_y;
                          weight_y = 0.0f;
                        }
                      else if (dy >= sample_max_y)
                        {
                          dy = sample_max_y;
                          weight_y = 0.0f;
                        }
                    }

                  srcptr = srcbuf + srcbuf_stride * dy + 2 * dx;

                  /* bilinear interpolation of the vectors */

                  a0 = srcptr[0] + (srcptr[2] - srcptr[0]) * weight_x;
                  b0 = srcptr[srcbuf_stride + 0] +
                       (srcptr[srcbuf_stride + 2] - srcptr[srcbuf_stride + 0]) *
                       weight_x;

                  a1 = srcptr[1] + (srcptr[3] - srcptr[1]) * weight_x;
                  b1 = srcptr[srcbuf_stride + 1] +
                       (srcptr[srcbuf_stride + 3] - srcptr[srcbuf_stride + 1]) *
                       weight_x;

                  vals[0] = a0 + (b0 - a0) * weight_y;
                  vals[1] = a1 + (b1 - a1) * weight_y;

                  vals[0] += nvx;
                  vals[1] += nvy;
                }
            }
        }
    });

  /* Paste the stamp into the source buffer. */
  gegl_parallel_distribute_range (
    area.height, gegl_operation_get_pixels_per_thread (operation) / area.width,
    [=] (gint y0, gint height)
    {
      gfloat yi;
      gint   y_iter;

      yi = -y + y0 + 0.5f;

      for (y_iter = y0; y_iter < y0 + height; y_iter++, yi++)
        {
          gfloat  lim;
          gint    min_x, max_x;
          gfloat *vals;
          gfloat *srcvals;

          lim = stamp_radius_sq - yi * yi;

          if (lim < 0.0f)
            continue;

          lim = sqrtf (lim);

          pixel_range (x - lim, x + lim,
                       &min_x,  &max_x);

          if (max_x < 0 || min_x >= area.width)
            continue;

          min_x = CLAMP (min_x, 0, area.width - 1);
          max_x = CLAMP (max_x, 0, area.width - 1);

          vals    = stampbuf + 2 * area.width * y_iter + 2 * min_x;
          srcvals = srcbuf   + srcbuf_stride  * y_iter + 2 * min_x;

          memcpy (srcvals, vals, 2 * sizeof (gfloat) * (max_x - min_x + 1));
        }
    });

  g_free (stampbuf);
}

static gboolean
process (GeglOperation        *operation,
         GeglOperationContext *context,
         const gchar          *output_prop,
         const GeglRectangle  *result,
         gint                  level)
{
  GeglProperties *o    = GEGL_PROPERTIES (operation);
  WarpPrivate    *priv = (WarpPrivate*) o->user_data;

  GObject        *input;
  GObject        *output;

  gdouble         spacing = MAX (o->size * o->spacing, 0.5);
  gdouble         dist;
  gint            stamps;
  gint            i;
  gdouble         t;

  gdouble         min_x, max_x;
  gdouble         min_y, max_y;

  gfloat         *srcbuf;
  gint            srcbuf_stride;
  gint            srcbuf_padding;
  GeglRectangle   srcbuf_extent;

  GeglPathPoint   prev, next, lerp;
  GeglPathList   *event;
  WarpPointList  *processed_event;

  if (!o->stroke || strcmp (output_prop, "output"))
    return FALSE;

  event = priv->remaining_stroke;

  /* if there is no stroke data left to process, pass the cached buffer right
   * away, or, if we don't have a cacehd buffer, pass the input buffer
   * directly.
   *
   * altenatively, if the stroke's strength is 0, the stroke has no effect.  do
   * the same.
   */
  if (! event || o->strength == 0.0)
    {
      if (priv->buffer)
        output = G_OBJECT (priv->buffer);
      else
        output = gegl_operation_context_get_object (context, "input");

      gegl_operation_context_set_object (context, "output", output);

      return TRUE;
    }
  /* otherwise, we process the remaining stroke on top of the previously-
   * processed buffer.
   */

  /* intialize the cached buffer if we don't already have one. */
  if (! priv->buffer)
    {
      input = gegl_operation_context_get_object (context, "input");

      priv->buffer = gegl_buffer_dup (GEGL_BUFFER (input));

      /* we pass the buffer as output directly while keeping it cached, so mark
       * it as forked.
       */
      gegl_object_set_has_forked (G_OBJECT (priv->buffer));
    }

  /* is this the first event of the stroke? */
  if (! priv->processed_stroke)
    {
      prev = *(event->d.point);

      priv->last_x = prev.x;
      priv->last_y = prev.y;
    }
  else
    {
      prev.x = priv->last_x;
      prev.y = priv->last_y;
    }

  /* find the bounding box of the portion of the stroke we're about to
   * process.
   */
  min_x = max_x = prev.x;
  min_y = max_y = prev.y;

  for (event = priv->remaining_stroke; event; event = event->next)
    {
      min_x = MIN (min_x, event->d.point[0].x);
      max_x = MAX (max_x, event->d.point[0].x);

      min_y = MIN (min_y, event->d.point[0].y);
      max_y = MAX (max_y, event->d.point[0].y);
    }

  srcbuf_extent.x      = floor (min_x - o->size / 2.0) - 1;
  srcbuf_extent.y      = floor (min_y - o->size / 2.0) - 1;
  srcbuf_extent.width  = ceil (max_x + o->size / 2.0) + 1 - srcbuf_extent.x;
  srcbuf_extent.height = ceil (max_y + o->size / 2.0) + 1 - srcbuf_extent.y;

  if (gegl_rectangle_intersect (&srcbuf_extent,
                                &srcbuf_extent,
                                gegl_buffer_get_extent (priv->buffer)))
    {
      /* we allocate a buffer, referred to as the source buffer, into which we
       * read the necessary portion of the input buffer, and consecutively
       * write the stroke results.
       */

      srcbuf_stride = 2 * srcbuf_extent.width;

      /* the source buffer is padded at the back by enough elements to make
       * pointers to out-of-bounds pixels, adjacent to the right and bottom
       * edges of the buffer, valid; such pointers may be formed as part of
       * sampling.  the value of these elements is irrelevant, as long as
       * they're finite.
       */
      srcbuf_padding = srcbuf_stride + 2;

      srcbuf = g_new (gfloat, srcbuf_stride * srcbuf_extent.height +
                              srcbuf_padding);

      /* fill the padding with (0, 0) vectors */
      memset (srcbuf + srcbuf_stride * srcbuf_extent.height,
              0, sizeof (gfloat) * srcbuf_padding);

      /* read the input data from the cached buffer */
      gegl_buffer_get (priv->buffer, &srcbuf_extent, 1.0, NULL,
                       srcbuf, sizeof (gfloat) * srcbuf_stride,
                       GEGL_ABYSS_NONE);

      /* process the remaining stroke */
      for (event = priv->remaining_stroke; event; event = event->next)
        {
          next = *(event->d.point);
          dist = gegl_path_point_dist (&next, &prev);
          stamps = floor (dist / spacing) + 1;

          /* stroke the current segment, such that there's always a stamp at
           * its final endpoint, and at positive integer multiples of
           * `spacing` away from it.
           */

          if (stamps == 1)
            {
              stamp (operation, o,
                     srcbuf, srcbuf_stride, &srcbuf_extent,
                     next.x, next.y);
            }
          else
            {
              for (i = 0; i < stamps; i++)
                {
                  t = 1.0 - ((stamps - i - 1) * spacing) / dist;

                  gegl_path_point_lerp (&lerp, &prev, &next, t);
                  stamp (operation, o,
                         srcbuf, srcbuf_stride, &srcbuf_extent,
                         lerp.x, lerp.y);
                }
            }

          prev = next;

          /* append the current event to the processed stroke. */
          processed_event        = g_slice_new (WarpPointList);
          processed_event->point = next;

          *priv->processed_stroke_tail = processed_event;
          priv->processed_stroke_tail  = &processed_event->next;
        }

      /* write the result back to the cached buffer */
      gegl_buffer_set (priv->buffer, &srcbuf_extent, 0, NULL,
                       srcbuf, sizeof (gfloat) * srcbuf_stride);

      g_free (srcbuf);
    }
  else
    {
      /* if the remaining stroke is completely out of bounds, just append it to
       * the processed stroke.
       */
      for (event = priv->remaining_stroke; event; event = event->next)
        {
          next = *(event->d.point);

          priv->last_x = next.x;
          priv->last_y = next.y;

          /* append the current event to the processed stroke. */
          processed_event        = g_slice_new (WarpPointList);
          processed_event->point = next;

          *priv->processed_stroke_tail = processed_event;
          priv->processed_stroke_tail  = &processed_event->next;
        }
    }

  *priv->processed_stroke_tail = NULL;
  priv->remaining_stroke       = NULL;

  /* pass the processed buffer as output */
  gegl_operation_context_set_object (context,
                                     "output", G_OBJECT (priv->buffer));

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->finalize                   = finalize;
  operation_class->attach                  = attach;
  operation_class->prepare                 = prepare;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->process                 = process;
  operation_class->no_cache = TRUE; /* we're effectively doing the caching
                                     * ourselves.
                                     */
  operation_class->threaded = FALSE;

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:warp",
    "categories",         "transform",
    "title",              _("Warp"),
    "position-dependent", "true",
    "description", _("Compute a relative displacement mapping from a stroke"),
    NULL);
}
#endif
