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
 * Copyright 2012 Nicolas Robidoux based on earlier code
 *           2012 Massimo Valentini
 *           2018 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <string.h>
#include <math.h>

#include "gegl-buffer.h"
#include "gegl-buffer-formats.h"
#include "gegl-sampler-cubic.h"

enum
{
  PROP_0,
  PROP_B,
  PROP_C,
  PROP_TYPE,
  PROP_LAST
};

static void            gegl_sampler_cubic_finalize    (      GObject               *gobject);
static inline void     gegl_sampler_cubic_interpolate (      GeglSampler* restrict  self,
                                                        const gdouble               absolute_x,
                                                        const gdouble               absolute_y,
                                                              gfloat*     restrict  output,
                                                              GeglAbyssPolicy       repeat_mode);
static void            gegl_sampler_cubic_get         (      GeglSampler* restrict  self,
                                                       const gdouble                absolute_x,
                                                       const gdouble                absolute_y,
                                                             GeglBufferMatrix2*     scale,
                                                             void*        restrict  output,
                                                             GeglAbyssPolicy        repeat_mode);
static void            get_property                   (      GObject               *gobject,
                                                             guint                  prop_id,
                                                             GValue                *value,
                                                             GParamSpec            *pspec);
static void            set_property                   (      GObject               *gobject,
                                                             guint                  prop_id,
                                                       const GValue                *value,
                                                             GParamSpec            *pspec);
static inline gfloat   cubicKernel                    (const gfloat                 x,
                                                       const gfloat                 b,
                                                       const gfloat                 c);


G_DEFINE_TYPE (GeglSamplerCubic, gegl_sampler_cubic, GEGL_TYPE_SAMPLER)

static void
gegl_sampler_cubic_class_init (GeglSamplerCubicClass *klass)
{
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);
  GObjectClass     *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;
  object_class->finalize     = gegl_sampler_cubic_finalize;

  sampler_class->get         = gegl_sampler_cubic_get;
  sampler_class->interpolate = gegl_sampler_cubic_interpolate;

  g_object_class_install_property ( object_class, PROP_B,
    g_param_spec_double ("b",
                         "B",
                         "B-spline parameter",
                         0.0,
                         1.0,
                         1.0,
                         G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS |
                         G_PARAM_READWRITE));

  g_object_class_install_property ( object_class, PROP_C,
    g_param_spec_double ("c",
                         "C",
                         "C-spline parameter",
                         0.0,
                         1.0,
                         0.0,
                         G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS |
                         G_PARAM_READWRITE));

  g_object_class_install_property ( object_class, PROP_TYPE,
    g_param_spec_string ("type",
                         "type",
                         "B-spline type (cubic | catmullrom | formula) 2c+b=1",
                         "cubic",
                         G_PARAM_CONSTRUCT |
                         G_PARAM_STATIC_STRINGS |
                         G_PARAM_READWRITE));
}

static void
gegl_sampler_cubic_finalize (GObject *object)
{
  g_free (GEGL_SAMPLER_CUBIC (object)->type);
  G_OBJECT_CLASS (gegl_sampler_cubic_parent_class)->finalize (object);
}

static void
gegl_sampler_cubic_init (GeglSamplerCubic *self)
{
  /*
   * In principle, x=y=-1 and width=height=4 are enough. The following
   * values are chosen so as to make the context_rect symmetrical
   * w.r.t. the anchor point. This is so that enough elbow room is
   * added with transformations that reflect the context rect. If the
   * context_rect is not symmetrical, the transformation may turn
   * right into left, and if the context_rect does not stretch far
   * enough on the left, pixel lookups will fail.
   */
  GEGL_SAMPLER (self)->level[0].context_rect.x = -2;
  GEGL_SAMPLER (self)->level[0].context_rect.y = -2;
  GEGL_SAMPLER (self)->level[0].context_rect.width = 5;
  GEGL_SAMPLER (self)->level[0].context_rect.height = 5;

  self->b=0.5;  /* 0.0 = sharp, but with anomaly of issue #167
                   1.0 = fuzzy cubic, without anomaly
                   0.5 is a compromise against issue #145 */
 
  /*
   * This ensures that the spline is a Keys spline. The c of
   * BC-splines is the alpha of Keys.
   */
  self->c = 0.5 * (1.0 - self->b);
}

static inline void
gegl_sampler_cubic_interpolate (      GeglSampler     *self,
                                const gdouble          absolute_x,
                                const gdouble          absolute_y,
                                      gfloat          *output,
                                      GeglAbyssPolicy  repeat_mode)
{
  GeglSamplerCubic *cubic      = (GeglSamplerCubic*)(self);
  gint              components = self->interpolate_components;
  gfloat            cubic_b    = cubic->b;
  gfloat            cubic_c    = cubic->c;
  gfloat           *sampler_bptr;
  gfloat            factor_i[4];
  gint              c;
  gint              i;
  gint              j;

  /*
   * The "-1/2"s are there because we want the index of the pixel
   * center to the left and top of the location, and with GIMP's
   * convention the top left of the top left pixel is located at
   * (0,0), and its center is at (1/2,1/2), so that anything less than
   * 1/2 needs to go negative. Another way to look at this is that we
   * are converting from a coordinate system in which the origin is at
   * the top left corner of the pixel with index (0,0), to a
   * coordinate system in which the origin is at the center of the
   * same pixel.
   */
  const double iabsolute_x = (double) absolute_x - 0.5;
  const double iabsolute_y = (double) absolute_y - 0.5;

  const gint ix = int_floorf (iabsolute_x);
  const gint iy = int_floorf (iabsolute_y);

  /*
   * x is the x-coordinate of the sampling point relative to the
   * position of the center of the top left pixel. Similarly for
   * y. Range of values: [0,1].
   */
  const gfloat x = iabsolute_x - ix;
  const gfloat y = iabsolute_y - iy;

  sampler_bptr = gegl_sampler_get_ptr (self, ix, iy, repeat_mode) -
                 (GEGL_SAMPLER_MAXIMUM_WIDTH + 1) * components;

  for (c = 0; c < components; c++)
    output[c] = 0.0f;

  for (i = 0; i < 4; i++)
    factor_i[i] = cubicKernel (x - (i - 1), cubic_b, cubic_c);

  for (j = 0; j < 4; j++)
    {
      gfloat factor_j = cubicKernel (y - (j - 1), cubic_b, cubic_c);

      for (i = 0; i < 4; i++)
        {
          const gfloat factor = factor_j * factor_i[i];

          for (c = 0; c < components; c++)
            output[c] += factor * sampler_bptr[c];

          sampler_bptr += components;
        }

      sampler_bptr += (GEGL_SAMPLER_MAXIMUM_WIDTH - 4) * components;
    }
}

static void
gegl_sampler_cubic_get (      GeglSampler       *self,
                        const gdouble            absolute_x,
                        const gdouble            absolute_y,
                              GeglBufferMatrix2 *scale,
                              void              *output,
                              GeglAbyssPolicy    repeat_mode)
{
  if (! _gegl_sampler_box_get (self, absolute_x, absolute_y, scale,
                               output, repeat_mode, 5))
  {
    gfloat result[5];

    gegl_sampler_cubic_interpolate (self, absolute_x, absolute_y, result,
                                    repeat_mode);

    babl_process (self->fish, result, output, 1);
  }
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglSamplerCubic *self = GEGL_SAMPLER_CUBIC (object);

  switch (prop_id)
    {
      case PROP_B:
        g_value_set_double (value, self->b);
        break;

      case PROP_TYPE:
        g_value_set_string (value, self->type);
        break;

      default:
        break;
    }
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglSamplerCubic *self = GEGL_SAMPLER_CUBIC (object);

  switch (prop_id)
    {
      case PROP_B:
        self->b = g_value_get_double (value);
        gegl_sampler_cubic_init (self);
        break;

      case PROP_TYPE:
        if (self->type)
          g_free (self->type);
        self->type = g_value_dup_string (value);
        gegl_sampler_cubic_init (self);
        break;

      default:
        break;
    }
}

static inline gfloat int_fabsf (const gfloat x)
{
  union {gfloat f; guint32 i;} u = {x};
  u.i &= 0x7fffffff;
  return u.f;
}

static inline gfloat
cubicKernel (const gfloat x,
             const gfloat b,
             const gfloat c)
{
  const gfloat x2 = x*x;
  const gfloat ax = int_fabsf (x);

  if (x2 <= (gfloat) 1.f) return ( (gfloat) ((12-9*b-6*c)/6) * ax +
                                  (gfloat) ((-18+12*b+6*c)/6) ) * x2 +
                                  (gfloat) ((6-2*b)/6);

  if (x2 < (gfloat) 4.f) return ( (gfloat) ((-b-6*c)/6) * ax +
                                 (gfloat) ((6*b+30*c)/6) ) * x2 +
                                 (gfloat) ((-12*b-48*c)/6) * ax +
                                 (gfloat) ((8*b+24*c)/6);

  return (gfloat) 0.f;
}
