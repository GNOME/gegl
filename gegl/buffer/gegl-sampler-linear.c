/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General
 * Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see
 * <https://www.gnu.org/licenses/>.
 *
 * Copyright 2011-2012 Nicolas Robidoux based on earlier code
 *           2012      Massimo Valentini
 */

#include "config.h"
#include <math.h>

#include "gegl-buffer.h"
#include "gegl-buffer-formats.h"
#include "gegl-sampler-linear.h"

enum
{
  PROP_0,
  PROP_LAST
};


static inline void   gegl_sampler_linear_interpolate (      GeglSampler* restrict  self,
                                                      const gdouble                absolute_x,
                                                      const gdouble                absolute_y,
                                                            gfloat*      restrict  output,
                                                            GeglAbyssPolicy        repeat_mode);
static void          gegl_sampler_linear_get         (      GeglSampler* restrict  self,
                                                      const gdouble                absolute_x,
                                                      const gdouble                absolute_y,
                                                            GeglBufferMatrix2     *scale,
                                                            void*        restrict  output,
                                                            GeglAbyssPolicy        repeat_mode);

G_DEFINE_TYPE (GeglSamplerLinear, gegl_sampler_linear, GEGL_TYPE_SAMPLER)

static void
gegl_sampler_linear_class_init (GeglSamplerLinearClass *klass)
{
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);

  sampler_class->get         = gegl_sampler_linear_get;
  sampler_class->interpolate = gegl_sampler_linear_interpolate;
}

/*
 * In principle, x=y=0 and width=height=2 are enough. The following
 * values are chosen so as to make the context_rect symmetrical
 * w.r.t. the anchor point. This is so that enough elbow room is added
 * with transformations that reflect the context rect. If the
 * context_rect is not symmetrical, the transformation may turn right
 * into left, and if the context_rect does not stretch far enough on
 * the left, pixel lookups will fail.
 *
 * Additional extra elbow room is added all around. It could be set to
 * 0 if it is found that round off error never sends things "too far
 * away". Nicolas would be very surprised if more than 1 is necessary.
 */
#define LINEAR_EXTRA_ELBOW_ROOM 0

static void
gegl_sampler_linear_init (GeglSamplerLinear *self)
{
  GEGL_SAMPLER (self)->level[0].context_rect.x      = -1 -   LINEAR_EXTRA_ELBOW_ROOM;
  GEGL_SAMPLER (self)->level[0].context_rect.y      = -1 -   LINEAR_EXTRA_ELBOW_ROOM;
  GEGL_SAMPLER (self)->level[0].context_rect.width  =  3 + 2*LINEAR_EXTRA_ELBOW_ROOM;
  GEGL_SAMPLER (self)->level[0].context_rect.height =  3 + 2*LINEAR_EXTRA_ELBOW_ROOM;
}

static inline void
gegl_sampler_linear_interpolate (      GeglSampler       *self,
                                 const gdouble            absolute_x,
                                 const gdouble            absolute_y,
                                       gfloat            *output,
                                       GeglAbyssPolicy    repeat_mode)
{
  gint       nc                    = self->interpolate_components;
  const gint pixels_per_buffer_row = GEGL_SAMPLER_MAXIMUM_WIDTH;

  /*
   * The "-1/2"s are there because we want the index of the pixel to
   * the left and top of the location, and with GIMP's convention the
   * top left of the top left pixel is located at
   * (1/2,1/2). Basically, we are converting from a coordinate system
   * in which the origin is at the top left pixel of the pixel with
   * index (0,0), to a coordinate system in which the origin is at the
   * center of the same pixel.
   */
  const float iabsolute_x = (float) absolute_x - 0.5;
  const float iabsolute_y = (float) absolute_y - 0.5;

  const gint ix = int_floorf (iabsolute_x);
  const gint iy = int_floorf (iabsolute_y);

  /*
   * Point the data tile pointer to the first channel of the top_left
   * pixel value:
   */
  const gfloat* restrict in_bptr =
    gegl_sampler_get_ptr (self, ix, iy, repeat_mode);

  /*
   * x is the x-coordinate of the sampling point relative to the
   * position of the center of the top left pixel. Similarly for
   * y. Range of values: [0,1].
   */
  const gfloat x = iabsolute_x - ix;
  const gfloat y = iabsolute_y - iy;

  /*
   * First bilinear weight:
   */
  const gfloat x_times_y = x * y;

  /*
   * Load top row:
   */
  gfloat top_left[nc];
  gfloat top_rite[nc];

  for (gint c = 0; c < nc; c++)
    top_left[c] = *in_bptr++;

  for (gint c = 0; c < nc; c++)
    top_rite[c] = *in_bptr++;

  in_bptr += ( pixels_per_buffer_row - 2 ) * nc;

  {
  /*
   * More bilinear weights:
   *
   * (Note: w = 1-x and z = 1-y.)
   */
  const gfloat w_times_y = y - x_times_y;
  const gfloat x_times_z = x - x_times_y;

  /*
   * Load bottom row:
   */
  gfloat bot_left[5];
  gfloat bot_rite[5];
  for (gint c = 0; c < nc; c++)
    bot_left[c] = *in_bptr++;
  for (gint c = 0; c < nc; c++)
    bot_rite[c] = *in_bptr++;

  /*
   * Last bilinear weight:
   */
  {
  const gfloat w_times_z = (gfloat) 1. - ( x + w_times_y );

  for (gint c = 0; c < nc; c++)
    {
      output[c] =
        x_times_y * bot_rite[c]
        +
        w_times_y * bot_left[c]
        +
        x_times_z * top_rite[c]
        +
        w_times_z * top_left[c];
    }
  }
  }
}

static void
gegl_sampler_linear_get (      GeglSampler       *self,
                         const gdouble            absolute_x,
                         const gdouble            absolute_y,
                               GeglBufferMatrix2 *scale,
                               void              *output,
                               GeglAbyssPolicy    repeat_mode)
{
  if (G_LIKELY(! _gegl_sampler_box_get (self, absolute_x, absolute_y, scale,
                               output, repeat_mode, 4)))
  {
    gfloat result[5];

    gegl_sampler_linear_interpolate (self, absolute_x, absolute_y, result,
                                     repeat_mode);

#if BABL_MINOR_VERSION > 1 || (BABL_MINOR_VERSION ==1 && BABL_MICRO_VERSION >= 90)
    self->fish_process (self->fish, (void*)result, (void*)output, 1, NULL);
#else
    babl_process (self->fish, (void*)result, (void*)output, 1);
#endif
  }
}
