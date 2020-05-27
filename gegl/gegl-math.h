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
 * Copyright 2020 Øyvind Kolås
 */

#ifndef __GEGL_MATH_H__
#define __GEGL_MATH_H__
#ifndef __cplusplus

static inline float gegl_fabsf (float x)
{
  union {
   float f;
   guint32 i;
  } u = {x};
  u.i &= 0x7fffffff;
  return u.f;
}

static inline double gegl_fabs (double x)
{
  union {
   double d;
   guint64 i;
  } u = {x};
  u.i &= 0x7fffffffffffffff;
  return u.d;
}

static inline float gegl_floorf (float x)
{
  int i = (int)x;       /* truncate */
  return i - ( i > x ); /* convert trunc to floor */
}

static inline float gegl_ceilf (float x)
{
  return - gegl_floorf (-x);
}

static inline double gegl_floor (double x)
{
  gint64 i = (gint64)x; /* truncate */
  return i - ( i > x ); /* convert trunc to floor */
}

static inline double  gegl_ceil (double x)
{
  return - gegl_floor (-x);
}

static inline float gegl_fmodf (float x, float y)
{
  return x - y * gegl_floorf (x/y);
}

static inline double gegl_fmod (double x, double y)
{
  return x - y * gegl_floor (x/y);
}

/* in innerloops inlining these is much faster than the overhead of
 * using implementations from elsewhere (that might be stricter than us
 * on their behavior on nans and more) */

#define fabs(x)    gegl_fabs(x)
#define fabsf(x)   gegl_fabsf(x)
#define floorf(x)  gegl_floorf(x)
#define floor(x)   gegl_floor(x)
#define ceilf(x)   gegl_ceilf(x)
#define ceil(x)    gegl_ceil(x)
#define fmodf(x,y) gegl_fmodf(x,y)
#define fmod(x,y)  gegl_fmod(x,y)


#endif
#endif
