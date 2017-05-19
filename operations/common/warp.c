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
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2011 Michael Muré <batolettre@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

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
#define GEGL_OP_C_SOURCE warp.c
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

  if (priv->lookup)
    {
      g_free (priv->lookup);

      priv->lookup = NULL;
    }

  if (priv->buffer)
    {
      g_object_unref (priv->buffer);

      priv->buffer = NULL;
    }

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

  rect.x      = floor (roi->x - o->size/2);
  rect.y      = floor (roi->y - o->size/2);
  rect.width  = ceil (roi->x + roi->width  + o->size/2) - rect.x;
  rect.height = ceil (roi->y + roi->height + o->size/2) - rect.y;

  /* avoid clearing the cache.  it will be cleared, if necessary, when
   * validating the stroke.
   */
  g_signal_handlers_block_by_func (operation->node,
                                   node_invalidated, operation);

  gegl_operation_invalidate (operation, &rect, FALSE);

  g_signal_handlers_unblock_by_func (operation->node,
                                     node_invalidated, operation);
}

static void
attach (GeglOperation *operation)
{
  GEGL_OPERATION_CLASS (gegl_op_parent_class)->attach (operation);

  g_signal_connect_object (operation->node, "invalidated",
                           G_CALLBACK (node_invalidated), operation, 0);
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

  radius = o->size / 2.0;

  length = floor (radius) + 2;

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

static gfloat
get_stamp_force (GeglProperties *o,
                 gfloat          x,
                 gfloat          y)
{
  WarpPrivate  *priv = (WarpPrivate*) o->user_data;
  gfloat        radius;

  if (!priv->lookup)
    {
      calc_lut (o);
    }

  radius = sqrtf (x * x + y * y);

  if (radius < 0.5f * o->size)
    {
      /* linear interpolation */
      gint   a;
      gfloat f;
      gfloat ratio;
      gfloat before, after;

      f = floorf (radius);
      ratio = (radius - f);

      a = f;

      before = priv->lookup[a];
      after = priv->lookup[a + 1];

      return before + ratio * (after - before);
    }

  return 0.0f;
}

static void
stamp (GeglProperties      *o,
       gfloat              *srcbuf,
       gint                 srcbuf_stride,
       const GeglRectangle *srcbuf_extent,
       const GeglRectangle *srcbuf_clip,
       gfloat               x,
       gfloat               y)
{
  WarpPrivate   *priv = (WarpPrivate*) o->user_data;
  gfloat         stamp_force, influence;
  gfloat         x_mean = 0.0;
  gfloat         y_mean = 0.0;
  gint           x_iter, y_iter;
  gfloat         xi, yi;
  GeglRectangle  area;
  gfloat        *stampbuf;
  gfloat        *vals;
  gfloat        *srcvals;
  gfloat         strength = 0.01 * o->strength;
  gfloat         s = 0, c = 0;
  gfloat         motion_x, motion_y;

  motion_x = priv->last_x - x;
  motion_y = priv->last_y - y;

  /* Memorize the stamp location for movement dependant behavior like move */
  priv->last_x = x;
  priv->last_y = y;

  if (strength == 0.0f)
    return; /* nop */

  /* Shift the coordiantes so that we work relative to the top-left corner of
   * the source buffer.
   */
  x -= srcbuf_extent->x;
  y -= srcbuf_extent->y;

  area.x = floor (x - o->size / 2.0);
  area.y = floor (y - o->size / 2.0);
  area.width   = ceil (x + o->size / 2.0);
  area.height  = ceil (y + o->size / 2.0);
  area.width  -= area.x;
  area.height -= area.y;

  if (! gegl_rectangle_intersect (&area, &area, srcbuf_clip))
    return;

  /* Align the source buffer with the stamped area */
  srcbuf += srcbuf_stride * area.y + 2 * area.x;

  /* If needed, compute the mean deformation */
  if (o->behavior == GEGL_WARP_BEHAVIOR_SMOOTH)
    {
      for (y_iter = 0; y_iter < area.height; y_iter++)
        {
          srcvals = srcbuf + srcbuf_stride * y_iter;

          for (x_iter = 0; x_iter < area.width; x_iter++, srcvals += 2)
            {
              x_mean += srcvals[0];
              y_mean += srcvals[1];
            }
        }

      x_mean /= area.width * area.height;
      y_mean /= area.width * area.height;
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

  vals = stampbuf;

  yi = area.y - y + 0.5f;

  for (y_iter = 0; y_iter < area.height; y_iter++, yi++)
    {
      srcvals = srcbuf + srcbuf_stride * y_iter;

      xi = area.x - x + 0.5f;

      for (x_iter = 0;
           x_iter < area.width;
           x_iter++, xi++, vals += 2, srcvals += 2)
        {
          gfloat nvx, nvy;
          gfloat fx, fy;
          gint   dx, dy;
          gfloat weight_x, weight_y;
          gfloat a0, b0;
          gfloat a1, b1;
          gfloat *srcptr;

          stamp_force = get_stamp_force (o, xi, yi);

          if (stamp_force == 0.0f)
            {
              vals[0] = srcvals[0];
              vals[1] = srcvals[1];

              continue;
            }

          influence = strength * stamp_force;

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
              vals[0] = srcvals[0] * (1.0f - MIN (influence, 1.0f));
              vals[1] = srcvals[1] * (1.0f - MIN (influence, 1.0f));
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

              /* yep, that's a "- 2", since we need to access two neighboring
               * rows/columns.  note that the source buffer is padded with a
               * 2-pixel-wide border of (0, 0) vectors, so that out-of-bounds
               * pixels behave as if they had a (0, 0) vector stored.
               */
              dx = CLAMP (dx, 0, srcbuf_extent->width  - 2);
              dy = CLAMP (dy, 0, srcbuf_extent->height - 2);

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

  /* Paste the stamp into the source buffer. */
  vals    = stampbuf;
  srcvals = srcbuf;

  for (y_iter = 0; y_iter < area.height; y_iter++)
    {
      memcpy (srcvals, vals, 2 * sizeof (gfloat) * area.width);

      vals    += 2 * area.width;
      srcvals += srcbuf_stride;
    }

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
  GeglRectangle   srcbuf_extent;
  GeglRectangle   srcbuf_clip;

  gfloat         *srcbuf_ptr;

  GeglPathPoint   prev, next, lerp;
  GeglPathList   *event;
  WarpPointList  *processed_event;

  if (!o->stroke || strcmp (output_prop, "output"))
    return FALSE;

  event = priv->remaining_stroke;

  /* if there is no stroke data left to process, pass the cached buffer right
   * away, or, if we don't have a cacehd buffer, pass the input buffer
   * directly.
   */
  if (! event)
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
      /* we pad the source buffer with a 2-pixel-wide border of (0, 0) vectors,
       * to simplify abyss sampling in stamp().
       */
      srcbuf_clip.x      = 2;
      srcbuf_clip.y      = 2;
      srcbuf_clip.width  = srcbuf_extent.width;
      srcbuf_clip.height = srcbuf_extent.height;

      srcbuf_extent.x      -= srcbuf_clip.x;
      srcbuf_extent.y      -= srcbuf_clip.y;
      srcbuf_extent.width  += 2 * srcbuf_clip.x;
      srcbuf_extent.height += 2 * srcbuf_clip.y;

      srcbuf_stride = 2 * srcbuf_extent.width;

      /* that's our source buffer.  we both read input data from it, and write
       * the result to it.
       */
      srcbuf = g_new (gfloat, srcbuf_stride * srcbuf_extent.height);

      /* fill the border with (0, 0) vectors. */
      srcbuf_ptr = srcbuf;

      memset (srcbuf_ptr, 0, sizeof (gfloat) * srcbuf_stride * srcbuf_clip.y);
      srcbuf_ptr += srcbuf_stride * srcbuf_clip.y;

      for (i = 0; i < srcbuf_clip.height; i++)
        {
          memset (srcbuf_ptr, 0, sizeof (gfloat) * 2 * srcbuf_clip.x);
          srcbuf_ptr += 2 * srcbuf_clip.x;

          srcbuf_ptr += 2 * srcbuf_clip.width;

          memset (srcbuf_ptr, 0, sizeof (gfloat) * 2 * srcbuf_clip.x);
          srcbuf_ptr += 2 * srcbuf_clip.x;
        }

      memset (srcbuf_ptr, 0, sizeof (gfloat) * srcbuf_stride * srcbuf_clip.y);

      /* read the input data from the cached buffer */
      gegl_buffer_get (priv->buffer,
                       GEGL_RECTANGLE (srcbuf_extent.x + srcbuf_clip.x,
                                       srcbuf_extent.y + srcbuf_clip.y,
                                       srcbuf_clip.width,
                                       srcbuf_clip.height),
                       1.0, NULL,
                       srcbuf + srcbuf_stride * srcbuf_clip.y +
                                            2 * srcbuf_clip.x,
                       sizeof (gfloat) * srcbuf_stride,
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
              stamp (o,
                     srcbuf, srcbuf_stride, &srcbuf_extent, &srcbuf_clip,
                     next.x, next.y);
            }
          else
            {
              for (i = 0; i < stamps; i++)
                {
                  t = 1.0 - ((stamps - i - 1) * spacing) / dist;

                  gegl_path_point_lerp (&lerp, &prev, &next, t);
                  stamp (o,
                         srcbuf, srcbuf_stride, &srcbuf_extent, &srcbuf_clip,
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
      gegl_buffer_set (priv->buffer,
                       GEGL_RECTANGLE (srcbuf_extent.x + srcbuf_clip.x,
                                       srcbuf_extent.y + srcbuf_clip.y,
                                       srcbuf_clip.width,
                                       srcbuf_clip.height),
                       0, NULL,
                       srcbuf + srcbuf_stride * srcbuf_clip.y +
                                            2 * srcbuf_clip.x,
                       sizeof (gfloat) * srcbuf_stride);
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
