/* This file is part of GEGL
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
 * Copyright 2003-2007 Calvin Williamson, Øyvind Kolås.
 */

#include "config.h"

#include <string.h>
#include <glib-object.h>
#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
#include "gegl-rectangle.h"

GeglRectangle *
gegl_rectangle_new (gint           x,
                    gint           y,
                    guint          w,
                    guint          h)
{
  GeglRectangle *r = g_new (GeglRectangle, 1);

  r->x      = x;
  r->y      = y;
  r->width  = w;
  r->height = h;

  return r;
}

void
gegl_rectangle_set (GeglRectangle *r,
                    gint           x,
                    gint           y,
                    guint          w,
                    guint          h)
{
  r->x      = x;
  r->y      = y;
  r->width  = w;
  r->height = h;
}

gboolean
gegl_rectangle_align (GeglRectangle          *dest,
                      const GeglRectangle    *rectangle,
                      const GeglRectangle    *tile,
                      GeglRectangleAlignment  alignment)
{
  gint x1, x2;
  gint y1, y2;

  x1 = rectangle->x - tile->x;
  x2 = x1 + rectangle->width;

  y1 = rectangle->y - tile->y;
  y2 = y1 + rectangle->height;

  switch (alignment)
    {
    case GEGL_RECTANGLE_ALIGNMENT_SUBSET:
      if (x1 > 0) x1 += tile->width  - 1;
      if (x2 < 0) x2 -= tile->width  - 1;

      if (y1 > 0) y1 += tile->height - 1;
      if (y2 < 0) y2 -= tile->height - 1;

      break;

    case GEGL_RECTANGLE_ALIGNMENT_SUPERSET:
      if (x1 < 0) x1 -= tile->width  - 1;
      if (x2 > 0) x2 += tile->width  - 1;

      if (y1 < 0) y1 -= tile->height - 1;
      if (y2 > 0) y2 += tile->height - 1;

      break;

    case GEGL_RECTANGLE_ALIGNMENT_NEAREST:
      if (x1 > 0) x1 += tile->width  / 2;
      else        x1 -= tile->width  / 2 - 1;
      if (x2 > 0) x2 += tile->width  / 2;
      else        x2 -= tile->width  / 2 - 1;

      if (y1 > 0) y1 += tile->height / 2;
      else        y1 -= tile->height / 2 - 1;
      if (y2 > 0) y2 += tile->height / 2;
      else        y2 -= tile->height / 2 - 1;

      break;
    }

  if (tile->width)
    {
      x1 = x1 / tile->width  * tile->width;
      x2 = x2 / tile->width  * tile->width;
    }
  if (tile->height)
    {
      y1 = y1 / tile->height * tile->height;
      y2 = y2 / tile->height * tile->height;
    }

  if (x1 < x2 && y1 < y2)
    {
      if (dest)
        {
          gegl_rectangle_set (dest,
                              tile->x + x1,
                              tile->y + y1,
                              x2 - x1,
                              y2 - y1);
        }

      return TRUE;
    }
  else
    {
      if (dest)
        gegl_rectangle_set (dest, 0, 0, 0, 0);

      return FALSE;
    }
}

gboolean
gegl_rectangle_align_to_buffer (GeglRectangle          *dest,
                                const GeglRectangle    *rectangle,
                                GeglBuffer             *buffer,
                                GeglRectangleAlignment  alignment)
{
  return gegl_rectangle_align (dest, rectangle,
                               GEGL_RECTANGLE (buffer->shift_x,
                                               buffer->shift_y,
                                               buffer->tile_width,
                                               buffer->tile_height),
                               alignment);
}

void
gegl_rectangle_bounding_box (GeglRectangle       *dest,
                             const GeglRectangle *src1,
                             const GeglRectangle *src2)
{
  gboolean s1_has_area = src1->width && src1->height;
  gboolean s2_has_area = src2->width && src2->height;

  if (!s1_has_area && !s2_has_area)
    gegl_rectangle_set (dest, 0, 0, 0, 0);
  else if (!s1_has_area)
    gegl_rectangle_copy (dest, src2);
  else if (!s2_has_area)
    gegl_rectangle_copy (dest, src1);
  else
    {
      gint x1 = MIN (src1->x, src2->x);
      gint x2 = MAX (src1->x + src1->width, src2->x + src2->width);
      gint y1 = MIN (src1->y, src2->y);
      gint y2 = MAX (src1->y + src1->height, src2->y + src2->height);

      dest->x      = x1;
      dest->y      = y1;
      dest->width  = x2 - x1;
      dest->height = y2 - y1;
    }
}

gboolean
gegl_rectangle_intersect (GeglRectangle       *dest,
                          const GeglRectangle *src1,
                          const GeglRectangle *src2)
{
  gint x1, x2, y1, y2;

  x1 = MAX (src1->x, src2->x);
  x2 = MIN (src1->x + src1->width, src2->x + src2->width);

  if (x2 <= x1)
    {
      if (dest)
        gegl_rectangle_set (dest, 0, 0, 0, 0);
      return FALSE;
    }

  y1 = MAX (src1->y, src2->y);
  y2 = MIN (src1->y + src1->height, src2->y + src2->height);

  if (y2 <= y1)
    {
      if (dest)
        gegl_rectangle_set (dest, 0, 0, 0, 0);
      return FALSE;
    }

  if (dest)
    gegl_rectangle_set (dest, x1, y1, x2 - x1, y2 - y1);
  return TRUE;
}

gint
gegl_rectangle_subtract (GeglRectangle        dest[4],
                         const GeglRectangle *minuend,
                         const GeglRectangle *subtrahend)
{
  gint mx1, mx2;
  gint my1, my2;

  gint sx1, sx2;
  gint sy1, sy2;

  gint n = 0;

  mx1 = minuend->x;
  mx2 = minuend->x + minuend->width;
  my1 = minuend->y;
  my2 = minuend->y + minuend->height;

  sx1 = subtrahend->x;
  sx2 = subtrahend->x + subtrahend->width;
  sy1 = subtrahend->y;
  sy2 = subtrahend->y + subtrahend->height;

  if (sx2 <= mx1 || sx1 >= mx2 || sy2 <= my1 || sy1 >= my2)
    {
      dest[0] = *minuend;

      return 1;
    }

  if (sy1 > my1)
    {
      gegl_rectangle_set (&dest[n++], mx1, my1, mx2 - mx1, sy1 - my1);

      my1 = sy1;
    }

  if (sy2 < my2)
    {
      gegl_rectangle_set (&dest[n++], mx1, sy2, mx2 - mx1, my2 - sy2);

      my2 = sy2;
    }

  if (sx1 > mx1)
    {
      gegl_rectangle_set (&dest[n++], mx1, my1, sx1 - mx1, my2 - my1);

      mx1 = sx1;
    }

  if (sx2 < mx2)
    {
      gegl_rectangle_set (&dest[n++], sx2, my1, mx2 - sx2, my2 - my1);

      mx2 = sx2;
    }

  return n;
}

gboolean
gegl_rectangle_subtract_bounding_box (GeglRectangle       *dest,
                                      const GeglRectangle *minuend,
                                      const GeglRectangle *subtrahend)
{
  gint mx1, mx2;
  gint my1, my2;

  gint sx1, sx2;
  gint sy1, sy2;

  mx1 = minuend->x;
  mx2 = minuend->x + minuend->width;
  my1 = minuend->y;
  my2 = minuend->y + minuend->height;

  sx1 = subtrahend->x;
  sx2 = subtrahend->x + subtrahend->width;
  sy1 = subtrahend->y;
  sy2 = subtrahend->y + subtrahend->height;

  if (sx1 <= mx1 && sx2 >= mx2)
    {
      if (sy1 <= my1) my1 = MAX (my1, sy2);
      if (sy2 >= my2) my2 = MIN (my2, sy1);
    }
  else if (sy1 <= my1 && sy2 >= my2)
    {
      if (sx1 <= mx1) mx1 = MAX (mx1, sx2);
      if (sx2 >= mx2) mx2 = MIN (mx2, sx1);
    }

  if (mx1 < mx2 && my1 < my2)
    {
      if (dest)
        gegl_rectangle_set (dest, mx1, my1, mx2 - mx1, my2 - my1);
      return TRUE;
    }
  else
    {
      if (dest)
        gegl_rectangle_set (dest, 0, 0, 0, 0);
      return FALSE;
    }
}

gint
gegl_rectangle_xor (GeglRectangle        dest[4],
                    const GeglRectangle *src1,
                    const GeglRectangle *src2)
{
  GeglRectangle rect1 = *src1;
  GeglRectangle rect2 = *src2;
  gint          n;

  n  = gegl_rectangle_subtract (dest,     &rect1, &rect2);
  n += gegl_rectangle_subtract (dest + n, &rect2, &rect1);

  return n;
}

void
gegl_rectangle_copy (GeglRectangle       *to,
                     const GeglRectangle *from)
{
  to->x      = from->x;
  to->y      = from->y;
  to->width  = from->width;
  to->height = from->height;
}

gboolean
gegl_rectangle_contains (const GeglRectangle *r,
                         const GeglRectangle *s)
{
  g_return_val_if_fail (r && s, FALSE);

  if (s->x >= r->x &&
      s->y >= r->y &&
      (s->x + s->width) <= (r->x + r->width) &&
      (s->y + s->height) <= (r->y + r->height))
    return TRUE;
  else
    return FALSE;
}

gboolean
gegl_rectangle_equal (const GeglRectangle *r,
                      const GeglRectangle *s)
{
  g_return_val_if_fail (r && s, FALSE);

  if (r->x == s->x &&
      r->y == s->y &&
      r->width == s->width &&
      r->height == s->height)
    return TRUE;
  else
    return FALSE;
}

gboolean
gegl_rectangle_equal_coords (const GeglRectangle *r,
                             gint                 x,
                             gint                 y,
                             gint                 w,
                             gint                 h)
{
  g_return_val_if_fail (r, FALSE);

  if (r->x == x &&
      r->y == y &&
      r->width == w &&
      r->height == h)
    return TRUE;
  else
    return FALSE;
}

gboolean
gegl_rectangle_is_empty (const GeglRectangle *r)
{
  g_return_val_if_fail (r != NULL, FALSE);
  return r->width == 0 || r->height == 0;
}

GeglRectangle *
gegl_rectangle_dup (const GeglRectangle *rectangle)
{
  GeglRectangle *result = g_new (GeglRectangle, 1);

  *result = *rectangle;

  return result;
}

GeglRectangle
gegl_rectangle_infinite_plane (void)
{
  GeglRectangle infinite_plane_rect = {G_MININT / 2, G_MININT / 2, G_MAXINT, G_MAXINT};
  return infinite_plane_rect;
}

gboolean
gegl_rectangle_is_infinite_plane (const GeglRectangle *rectangle)
{
  return (rectangle->x      == G_MININT / 2 &&
          rectangle->y      == G_MININT / 2 &&
          rectangle->width  == G_MAXINT     &&
          rectangle->height == G_MAXINT);
}

void
gegl_rectangle_dump (const GeglRectangle *rectangle)
{
  g_print ("%d, %d, %d×%d\n",
           rectangle->x,
           rectangle->y,
           rectangle->width,
           rectangle->height);
}

GType
gegl_rectangle_get_type (void)
{
  static GType our_type = 0;

  if (our_type == 0)
    our_type = g_boxed_type_register_static (g_intern_static_string ("GeglRectangle"),
                                             (GBoxedCopyFunc) gegl_rectangle_dup,
                                             (GBoxedFreeFunc) g_free);
  return our_type;
}

gint
_gegl_float_epsilon_zero (float value)
{
  return value > -GEGL_FLOAT_EPSILON && value < GEGL_FLOAT_EPSILON;
}

gint
_gegl_float_epsilon_equal (float v1, float v2)
{
  register float diff = v1 - v2;

  return diff > -GEGL_FLOAT_EPSILON && diff < GEGL_FLOAT_EPSILON;
}

