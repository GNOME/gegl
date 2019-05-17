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
 * 2012, 2017 (c) Nicolas Robidoux
 */

/*
 * ==============
 * LOHALO SAMPLER
 * ==============
 *
 * The Lohalo ("Low Halo") sampler is a Jacobian-adaptive blend of
 * sigmoidized tensor filtering with the Mitchell-Netravali Keys cubic
 * filter used as an upsampler (and consequently unscaled), with
 * non-sigmoidized (plain linear light) EWA (Elliptical Weighted
 * Averaging) filtering with the Robidoux Keys cubic, which is used
 * whenever some downsampling occurs and consequently is appropriately
 * scaled.
 *
 * WARNING: This version of Lohalo only gives top quality results down
 * to about a downsampling of about ratio 2/(LOHALO_OFFSET_0+0.5).
 * Beyond that, the quality degrades gracefully (The "2" in the
 * numerator is because the radius of the Robidoux EWA filter is 2.)
 */

/*
 * Credits and thanks:
 *
 * This code owes a lot to R&D performed for ImageMagick by
 * N. Robidoux and Anthony Thyssen, and student research performed by
 * Adam Turcotte and Chantal Racette.
 *
 * Sigmoidization was invented by N. Robidoux as a method of
 * minimizing the over and undershoots that arise out of filtering
 * with kernel with one more negative lobe. It basically consists of
 * resampling through a colorspace in which gamut extremes are "far"
 * from midtones.
 *
 * Jacobian adaptive resampling was developed by N. Robidoux and
 * A. Turcotte of the Department of Mathematics and Computer Science
 * of Laurentian University in the course of A. Turcotte's Masters in
 * Computational Sciences (even though the eventual thesis did not
 * include this topic). Øyvind Kolås and Michael Natterer contributed
 * much to the GEGL implementation.
 *
 * The Robidoux Keys cubic filter was developed by N. Robidoux and is
 * based on earlier research by A. Thyssen on the use of cubic
 * filters, like Mitchell-Netravali, for Elliptical Weighted
 * Averaging.
 *
 * Clamped EWA was developed by N. Robidoux and A. Thyssen with the
 * assistance of Chantal Racette and Frederick Weinhaus. It is based
 * on methods of Paul Heckbert, Andreas Gustaffson and almost
 * certainly other researchers.
 *
 * Fast computation methods for Keys cubic weights were developed by
 * N. Robidoux. Variants are used by the VIPS and ImageMagick
 * libraries.
 *
 * A. Turcotte's image resampling research on Jacobian adaptive
 * methods funded in part by an OGS (Ontario Graduate Scholarship) and
 * an NSERC Alexander Graham Bell Canada Graduate Scholarship awarded
 * to him, by GSoC (Google Summer of Code) 2010 funding awarded to
 * GIMP (Gnu Image Manipulation Program), and by Laurentian University
 * research funding provided by N. Robidoux and Ralf Meyer.
 *
 * C. Racette's image resampling research and programming funded in
 * part by a NSERC Discovery Grant awarded to Julien Dompierre
 * (20-61098) and by a NSERC Alexander Graham Bell Canada Graduate
 * Scholarship awarded to her.
 *
 * N. Robidoux's development of fast formulas for the computation of
 * Mitchell-Netravali Keys cubic weights was funded in part by Pixel
 * Analytics Ltd.
 *
 * The programming of this and other GEGL samplers by N. Robidoux was
 * funded by a large number of freedomsponsors.org patrons, including
 * A. Prokoudine.
 *
 * In addition to the above, N. Robidoux thanks Craig DeForest,
 * Mathias Rauen, John Cupitt, Henry HO and Massimo Valentini for
 * useful comments, with special thanks to Cristy, the lead developer
 * of ImageMagick, for making it available as a platform for research
 * in image processing.
 */

/*
 * General convention:
 *
 * Looking at the image as a (linear algebra) matrix, the index j has
 * to do with the x-coordinate, that is, horizontal position, and the
 * index i has to do with the y-coordinate (which runs from top to
 * bottom), that is, the vertical position.
 *
 * However, in order to match the GIMP convention that states that
 * pixel indices match the position of the top left corner of the
 * corresponding pixel, so that the center of pixel (i,j) is located
 * at (i+1/2,j+1/2), pixel center positions do NOT match their index.
 */

#include "config.h"
#include <math.h>

#include "gegl-buffer.h"
#include "gegl-buffer-formats.h"
#include "gegl-sampler-lohalo.h"

/*
 * Macros set up so the likely winner in in the first argument
 * (forward branch likely etc):
 */
#define LOHALO_MIN(_x_,_y_) ( (_x_) <= (_y_) ? (_x_) : (_y_) )
#define LOHALO_MAX(_x_,_y_) ( (_x_) >= (_y_) ? (_x_) : (_y_) )

enum
{
  PROP_0,
  PROP_LAST
};

static void gegl_sampler_lohalo_get (      GeglSampler* restrict  self,
                                     const gdouble                absolute_x,
                                     const gdouble                absolute_y,
                                           GeglBufferMatrix2     *scale,
                                           void*        restrict  output,
                                           GeglAbyssPolicy        repeat_mode);

G_DEFINE_TYPE (GeglSamplerLohalo, gegl_sampler_lohalo, GEGL_TYPE_SAMPLER)

static void
gegl_sampler_lohalo_class_init (GeglSamplerLohaloClass *klass)
{
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);
  sampler_class->get = gegl_sampler_lohalo_get;
}

/*
 * The Mitchell-Netravali cubic filter uses 4 pixels in each
 * direction. Because we anchor ourselves at the closest pixel, we
 * need 2 pixels on both sides, because we don't know ahead of time
 * which way things will be reflected. So, we need the size to be at
 * least 5x5. 4x4 would be enough if we did not perform reflections in
 * order to keep things as centered as possible between mipmap levels
 * (which have been removed). In any case, LOHALO_OFFSET_0 must be >=
 * 2 the way things are implemented.
 *
 * Speed/quality trade-off:
 *
 * Downsampling quality will decrease around ratio
 * 1/(LOHALO_OFFSET_0+0.5). In addition, the smaller LOHALO_OFFSET_0,
 * the more noticeable the artifacts. To maintain maximum quality for
 * the widest downsampling range possible, a somewhat large
 * LOHALO_OFFSET_0 should be used. However, the larger the "level 0"
 * offset, the slower Lohalo will run when no significant downsampling
 * is done, because the width and height of context_rect is
 * (2*LOHALO_OFFSET_0+1), and consequently there is less data "tile"
 * reuse with large LOHALO_OFFSET_0.
 */
/*
 * IMPORTANT: LOHALO_OFFSET_0 SHOULD BE AN INTEGER >= 2.
 */
#define LOHALO_OFFSET_0 (13)
#define LOHALO_SIZE_0 (1+2*LOHALO_OFFSET_0)

static void
gegl_sampler_lohalo_init (GeglSamplerLohalo *self)
{
  GeglSampler *sampler = GEGL_SAMPLER (self);
  GeglSamplerLevel *level;

  level = &sampler->level[0];
  level->context_rect.x   = -LOHALO_OFFSET_0;
  level->context_rect.y   = -LOHALO_OFFSET_0;
  level->context_rect.width  = LOHALO_SIZE_0;
  level->context_rect.height = LOHALO_SIZE_0;
}

/*
 * This value of the sigmoidal contrast (determined approximately
 * using ImageMagick) was obtained as follows: Enlarge one white pixel
 * on a black background with tensor Mitchell-Netravali. The following
 * value of contrast is such that the result has exactly the right
 * mass. (This also works with a single black pixel on a white
 * background.)
 *
 * As sigmoidization goes, this contrast value is rather mild. I
 * (N. Robidoux, the creator of sigmoidization) am not totally sure
 * how to best to determine sigmoidization values. Actually, I'm not
 * totally sure the tanh sigmoidal is the best possible curve
 * either. But this combination seems to work pretty well.
 *
 * Probably should be recomputed directly using GEGL (making sure
 * abyss issues and the like don't bite). And I'm not totally sure
 * it's the greatest thing with sudden transparency.
 *
 * Note: If you decide to turn is off without changing the code, don't
 * set it to 0: There is a removable singularity which I've not
 * removed. Setting the contrast to .0000001 basically turns it off
 * (but the flops are still done).
 */
#define LOHALO_CONTRAST (3.38589)

static inline double
sigmoidal (const double p)
{
  /*
   * Only used to compute compile-time constants, so efficiency is
   * irrelevant.
   */
  return tanh (0.5*LOHALO_CONTRAST*(p-0.5));
}

static inline float
sigmoidalf (const float p)
{
  /*
   * Cheaper runtime version.
   */
  return tanhf ((gfloat) (0.5*LOHALO_CONTRAST) * p +
                (gfloat) (-0.25*LOHALO_CONTRAST));
}

static inline gfloat
extended_sigmoidal (const gfloat q)
{
  /*
   * This function extends the standard sigmoidal with straight lines
   * at p=0 and p=1, in such a way that there is no value or slope
   * discontinuity.
   */
  const gdouble sig1  = sigmoidal (1.);
  const gdouble slope = ( 1./sig1 - sig1 ) * 0.25 * LOHALO_CONTRAST;

  const gfloat slope_times_q = (gfloat) slope * q;

  if (q <= (gfloat) 0.)
    return slope_times_q;

  if (q >= (gfloat) 1.)
    return slope_times_q + (gfloat) (1. - slope);

  {
    const gfloat p = (float) (0.5/sig1) * sigmoidalf ((float) q) + (float) 0.5;
    return p;
  }
}

static inline gfloat
inverse_sigmoidal (const gfloat p)
{
  /*
   * This function is the inverse of extended_sigmoidal above.
   */
  const gdouble sig1  = sigmoidal (1.);
  const gdouble sig0  = -sig1;
  const gdouble slope = ( 1./sig1 + sig0 ) * 0.25 * LOHALO_CONTRAST;
  const gdouble one_over_slope = 1./slope;

  const gfloat p_over_slope = p * (gfloat) one_over_slope;

  if (p <= (gfloat) 0.)
    return p_over_slope;

  if (p >= (gfloat) 1.)
    return p_over_slope + (gfloat) (1.-one_over_slope);

  {
    const gfloat ssq = (gfloat) (2.*sig1) * p + (gfloat) sig0;
    const gfloat q =
      (float) (2./LOHALO_CONTRAST) * atanhf (ssq) + (float) 0.5;
    return q;
  }
}

static inline gfloat
robidoux (const gfloat c_major_x,
          const gfloat c_major_y,
          const gfloat c_minor_x,
          const gfloat c_minor_y,
          const gfloat s,
          const gfloat t)
{
  /*
   * This function computes -398/(7+72sqrt(2)) times the Robidoux
   * cubic. The factor of -398/(7+72sqrt(2)) is to remove one
   * flop. This scaling is harmless because the final results is
   * normalized by the sum of the weights, which means nonzero overall
   * multiplicative factors have no impact.
   *
   * The Robidoux cubic is the Keys cubic defined, as a BC-spline, by
   *
   * B = 1656 / ( 1592 + 597 * sqrt(2) )
   *
   * and
   *
   * C = 15407 / ( 35422 + 42984 * sqrt(2) ).
   *
   * Keys cubics are the BC-splines with B+2C=1.
   *
   * The Robidoux cubic is the unique Keys cubic with the property
   * that it preserves images which pixel values constant along
   * columns (or rows) under no-op EWA resampling. Informally, it is
   * the unique Keys cubic for which a vertical or horizontal line or
   * boundary does not "bleed" into neighbouring original pixel
   * locations when used, as an EWA filter kernel, to resample without
   * downsampling.
   */
  const gfloat q1 = s * c_major_x + t * c_major_y;
  const gfloat q2 = s * c_minor_x + t * c_minor_y;

  const gfloat r2 = q1 * q1 + q2 * q2;

  if (r2 >= (gfloat) 4.)
    return (gfloat) 0.;

  {
    const gfloat r = sqrtf ((float) r2);

    const gfloat minus_inner_root =
      ( -103. - 36. * sqrt(2.) ) / ( 7. + 72. * sqrt(2.) );
    const gfloat minus_outer_root = -2.;

    const gfloat a3 = -3.;
    const gfloat a2 = ( 45739. +   7164. * sqrt(2.) ) / 10319.;
    const gfloat a0 = ( -8926. + -14328. * sqrt(2.) ) / 10319.;

    const gfloat weight =
      (r2 >= (float) 1.)
      ?
      (r + minus_inner_root) * (r + minus_outer_root) * (r + minus_outer_root)
      :
      r2 * (a3 * r + a2) + a0;

    return weight;
  }
}

static inline void
ewa_update (const gint              j,
            const gint              i,
            const gfloat            c_major_x,
            const gfloat            c_major_y,
            const gfloat            c_minor_x,
            const gfloat            c_minor_y,
            const gfloat            x_0,
            const gfloat            y_0,
            const gint              channels,
            const gint              row_skip,
            const gfloat*  restrict input_ptr,
                  gdouble* restrict total_weight,
                  gfloat*  restrict ewa_newval)
{
  const gint skip = j * channels + i * row_skip;
  gint c;
  const gfloat weight = robidoux (c_major_x,
                                  c_major_y,
                                  c_minor_x,
                                  c_minor_y,
                                  x_0 - (gfloat) j,
                                  y_0 - (gfloat) i);

  *total_weight += weight;
  for (c = 0; c < channels; c++)
    ewa_newval[c] += weight * input_ptr[ skip + c ];
}

static void
gegl_sampler_lohalo_get (      GeglSampler*    restrict  self,
                         const gdouble                   absolute_x,
                         const gdouble                   absolute_y,
                               GeglBufferMatrix2        *scale,
                               void*           restrict  output,
                               GeglAbyssPolicy           repeat_mode)
{
  /*
   * Needed constants related to the input pixel value pointer
   * provided by gegl_sampler_get_ptr (self, ix, iy). pixels_per_row
   * corresponds to fetch_rectangle.width in gegl_sampler_get_ptr.
   */
  const gint channels       = self->interpolate_components;
  const gint pixels_per_row = GEGL_SAMPLER_MAXIMUM_WIDTH;
  const gint row_skip       = channels * pixels_per_row;

  /*
   * The consequence of the following choice of anchor pixel location
   * is that the sampling location is at most at a box distance of .5
   * from the anchor pixel location. That is: This computes the index
   * of the closest pixel center (one of the closest when there are
   * ties) within the GIMP convention.
   *
   * The reason why floor gives the index of the closest pixel center
   * (with ties resolved toward -infinity) is that absolute positions
   * are corner-based, meaning that the absolute position of the
   * center of the pixel indexed (0,0) is (.5,.5) instead of (0,0), as
   * it would be if absolute positions were center-based.
   */
  const gint ix_0 = floor ((double) absolute_x);
  const gint iy_0 = floor ((double) absolute_y);

  /*
   * Using the neareast anchor pixel position is not the most
   * efficient choice for a tensor bicubic for which anchoring an
   * asymetrical 4 point stencil at the second pixel location in both
   * directions is best. For one thing, it requires having at least a
   * 5x5 stencil when dealing with possible reflexions.
   */

  /*
   * This is the pointer we use to pull pixel from "base" mipmap level
   * (level "0"), the one with scale=1.0.
   */
  const gfloat* restrict input_ptr =
    (gfloat*) gegl_sampler_get_ptr (self, ix_0, iy_0, repeat_mode);

  /*
   * First, we convert from the absolute position in the coordinate
   * system with origin at the top left corner of the pixel with index
   * (0,0) (the "GIMP convention" a.k.a. "corner-based"), to the
   * position in the coordinate system with origin at the center of
   * the pixel with index (0,0) (the "index" convention
   * a.k.a. "center-based").
   */
  const gdouble iabsolute_x = absolute_x - (gdouble) 0.5;
  const gdouble iabsolute_y = absolute_y - (gdouble) 0.5;

  /*
   * (x_0,y_0) is the relative position of the sampling location
   * w.r.t. the anchor pixel.
   */
  const gfloat x_0 = iabsolute_x - ix_0;
  const gfloat y_0 = iabsolute_y - iy_0;

  const gint sign_of_x_0 = 2 * ( x_0 >= (gfloat) 0. ) - 1;
  const gint sign_of_y_0 = 2 * ( y_0 >= (gfloat) 0. ) - 1;

  const gint shift_forw_1_pix = sign_of_x_0 * channels;
  const gint shift_forw_1_row = sign_of_y_0 * row_skip;

  const gint shift_back_1_pix = -shift_forw_1_pix;
  const gint shift_back_1_row = -shift_forw_1_row;

  const gint shift_forw_2_pix = 2 * shift_forw_1_pix;
  const gint shift_forw_2_row = 2 * shift_forw_1_row;

  const gint uno_one_shift = shift_back_1_pix + shift_back_1_row;
  const gint uno_two_shift =                    shift_back_1_row;
  const gint uno_thr_shift = shift_forw_1_pix + shift_back_1_row;
  const gint uno_fou_shift = shift_forw_2_pix + shift_back_1_row;

  const gint dos_one_shift = shift_back_1_pix;
  const gint dos_two_shift = 0;
  const gint dos_thr_shift = shift_forw_1_pix;
  const gint dos_fou_shift = shift_forw_2_pix;

  const gint tre_one_shift = shift_back_1_pix + shift_forw_1_row;
  const gint tre_two_shift =                    shift_forw_1_row;
  const gint tre_thr_shift = shift_forw_1_pix + shift_forw_1_row;
  const gint tre_fou_shift = shift_forw_2_pix + shift_forw_1_row;

  const gint qua_one_shift = shift_back_1_pix + shift_forw_2_row;
  const gint qua_two_shift =                    shift_forw_2_row;
  const gint qua_thr_shift = shift_forw_1_pix + shift_forw_2_row;
  const gint qua_fou_shift = shift_forw_2_pix + shift_forw_2_row;

  /*
   * Flip coordinates so we can assume we are in the interval [0,1].
   */
  const gfloat ax = x_0 >= (gfloat) 0. ? x_0 : -x_0;
  const gfloat ay = y_0 >= (gfloat) 0. ? y_0 : -y_0;
  /*
   * Computation of the Mitchell-Netravali weights using an original
   * method of N. Robidoux that only requires 13 flops to compute each
   * group of four weights.
   */
  const gfloat xt1 = (gfloat) (7./18.) * ax;
  const gfloat yt1 = (gfloat) (7./18.) * ay;
  const gfloat xt2 = (gfloat) 1. - ax;
  const gfloat yt2 = (gfloat) 1. - ay;
  const gfloat fou = ( xt1 + (gfloat) (-1./3.) ) * ax * ax;
  const gfloat qua = ( yt1 + (gfloat) (-1./3.) ) * ay * ay;
  const gfloat one = ( (gfloat) (1./18.) - xt1 ) * xt2 * xt2;
  const gfloat uno = ( (gfloat) (1./18.) - yt1 ) * yt2 * yt2;
  const gfloat xt3 = fou - one;
  const gfloat yt3 = qua - uno;
  const gfloat thr = ax - fou - xt3;
  const gfloat tre = ay - qua - yt3;
  const gfloat two = xt2 - one + xt3;
  const gfloat dos = yt2 - uno + yt3;
  gint c;
  /*
   * The newval array will contain one computed resampled value per
   * channel:
   */
  gfloat newval[channels];
  for (c = 0; c < channels-1; c++)
   newval[c] =
    extended_sigmoidal (
      uno * ( one * inverse_sigmoidal (input_ptr[ uno_one_shift + c ]) +
              two * inverse_sigmoidal (input_ptr[ uno_two_shift + c ]) +
              thr * inverse_sigmoidal (input_ptr[ uno_thr_shift + c ]) +
              fou * inverse_sigmoidal (input_ptr[ uno_fou_shift + c ]) ) +
      dos * ( one * inverse_sigmoidal (input_ptr[ dos_one_shift + c ]) +
              two * inverse_sigmoidal (input_ptr[ dos_two_shift + c ]) +
              thr * inverse_sigmoidal (input_ptr[ dos_thr_shift + c ]) +
              fou * inverse_sigmoidal (input_ptr[ dos_fou_shift + c ]) ) +
      tre * ( one * inverse_sigmoidal (input_ptr[ tre_one_shift + c ]) +
              two * inverse_sigmoidal (input_ptr[ tre_two_shift + c ]) +
              thr * inverse_sigmoidal (input_ptr[ tre_thr_shift + c ]) +
              fou * inverse_sigmoidal (input_ptr[ tre_fou_shift + c ]) ) +
      qua * ( one * inverse_sigmoidal (input_ptr[ qua_one_shift + c ]) +
              two * inverse_sigmoidal (input_ptr[ qua_two_shift + c ]) +
              thr * inverse_sigmoidal (input_ptr[ qua_thr_shift + c ]) +
              fou * inverse_sigmoidal (input_ptr[ qua_fou_shift + c ]) ) );
  /*
   * It appears that it is a bad idea to sigmoidize the transparency
   * channel (in RaGaBaA, at least). So don't.
   */
  newval[channels-1] =
              uno * ( one * input_ptr[ uno_one_shift + channels - 1 ] +
                      two * input_ptr[ uno_two_shift + channels - 1 ] +
                      thr * input_ptr[ uno_thr_shift + channels - 1 ] +
                      fou * input_ptr[ uno_fou_shift + channels - 1 ] ) +
              dos * ( one * input_ptr[ dos_one_shift + channels - 1 ] +
                      two * input_ptr[ dos_two_shift + channels - 1 ] +
                      thr * input_ptr[ dos_thr_shift + channels - 1 ] +
                      fou * input_ptr[ dos_fou_shift + channels - 1 ] ) +
              tre * ( one * input_ptr[ tre_one_shift + channels - 1 ] +
                      two * input_ptr[ tre_two_shift + channels - 1 ] +
                      thr * input_ptr[ tre_thr_shift + channels - 1 ] +
                      fou * input_ptr[ tre_fou_shift + channels - 1 ] ) +
              qua * ( one * input_ptr[ qua_one_shift + channels - 1 ] +
                      two * input_ptr[ qua_two_shift + channels - 1 ] +
                      thr * input_ptr[ qua_thr_shift + channels - 1 ] +
                      fou * input_ptr[ qua_fou_shift + channels - 1 ] );

  {
    /*
     * Determine whether Mitchell-Netravali needs to be blended with
     * the downsampling method (Clamped EWA with the Robidoux
     * filter).
     *
     * This is done by taking the 2x2 matrix Jinv (the exact or
     * approximate inverse Jacobian of the transformation at the
     * location under consideration:
     *
     * Jinv = [ a b ] = [ dx/dX dx/dY ]
     *        [ c d ] = [ dy/dX dy/dY ]
     *
     * and computes from it the major and minor axis vectors
     * [major_x, major_y] and [minor_x,minor_y] of the smallest
     * ellipse containing both the unit disk and the ellipse which
     * is the image of the unit disk by the linear transformation
     *
     * [ a b ] [S] = [s]
     * [ c d ] [T] = [t]
     *
     * The vector [S,T] is the difference between a position in
     * output space and [X,Y], the output location under
     * consideration; the vector [s,t] is the difference between a
     * position in input space and [x,y], the corresponding input
     * location.
     *
     */
    /*
     * GOAL:
     * Fix things so that the pullback, in input space, of a disk of
     * radius r in output space is an ellipse which contains, at
     * least, a disc of radius r. (Make this hold for any r>0.)
     *
     * ESSENCE OF THE METHOD:
     * Compute the product of the first two factors of an SVD of the
     * linear transformation defining the ellipse and make sure that
     * both its columns have norm at least 1.  Because rotations and
     * reflexions map disks to themselves, it is not necessary to
     * compute the third (rightmost) factor of the SVD.
     *
     * DETAILS:
     * Find the singular values and (unit) left singular vectors of
     * Jinv, clamping up the singular values to 1, and multiply the
     * unit left singular vectors by the new singular values in
     * order to get the minor and major ellipse axis vectors.
     *
     * Image resampling context:
     *
     * The Jacobian matrix of the transformation at the output point
     * under consideration is defined as follows:
     *
     * Consider the transformation (x,y) -> (X,Y) from input
     * locations to output locations.
     *
     * The Jacobian matrix of the transformation at (x,y) is equal
     * to
     *
     *   J = [ A, B ] = [ dX/dx, dX/dy ]
     *       [ C, D ]   [ dY/dx, dY/dy ]
     *
     * that is, the vector [A,C] is the tangent vector corresponding
     * to input changes in the horizontal direction, and the vector
     * [B,D] is the tangent vector corresponding to input changes in
     * the vertical direction.
     *
     * In the context of resampling, it is natural to use the
     * inverse Jacobian matrix Jinv because resampling is generally
     * performed by pulling pixel locations in the output image back
     * to locations in the input image. Jinv is
     *
     *   Jinv = [ a, b ] = [ dx/dX, dx/dY ]
     *          [ c, d ]   [ dy/dX, dy/dY ]
     *
     * Note: Jinv can be computed from J with the following matrix
     * formula:
     *
     *   Jinv = 1/(A*D-B*C) [  D, -B ]
     *                      [ -C,  A ]
     *
     * What we do is modify Jinv so that it generates an ellipse
     * which is as close as possible to the original but which
     * contains the unit disk. This can be accomplished as follows:
     *
     * Let
     *
     *   Jinv = U Sigma V^T
     *
     * be an SVD decomposition of Jinv. (The SVD is not unique, but
     * the final ellipse does not depend on the particular SVD.)
     *
     * We could clamp up the entries of the diagonal matrix Sigma so
     * that they are at least 1, and then set
     *
     *   Jinv = U newSigma V^T.
     *
     * However, we do not need to compute V for the following
     * reason: V^T is an orthogonal matrix (that is, it represents a
     * combination of rotations and reflexions) so that it maps the
     * unit circle to itself. For this reason, the exact value of V
     * does not affect the final ellipse, and we can choose V to be
     * the identity matrix. This gives
     *
     *   Jinv = U newSigma.
     *
     * In the end, we return the two diagonal entries of newSigma
     * together with the two columns of U.
     *
     */
    /*
     * We compute:
     *
     * major_mag: half-length of the major axis of the "new"
     *            (post-clamping) ellipse.
     *
     * minor_mag: half-length of the minor axis of the "new"
     *            ellipse.
     *
     * major_unit_x: x-coordinate of the major axis direction vector
     *               of both the "old" and "new" ellipses.
     *
     * major_unit_y: y-coordinate of the major axis direction
     *               vector.
     *
     * minor_unit_x: x-coordinate of the minor axis direction
     *               vector.
     *
     * minor_unit_y: y-coordinate of the minor axis direction
     *               vector.
     *
     * Unit vectors are useful for computing projections, in
     * particular, to compute the distance between a point in output
     * space and the center of a unit disk in output space, using
     * the position of the corresponding point [s,t] in input space.
     * Following the clamping, the square of this distance is
     *
     * ( ( s * major_unit_x + t * major_unit_y ) / major_mag )^2
     * +
     * ( ( s * minor_unit_x + t * minor_unit_y ) / minor_mag )^2
     *
     * If such distances will be computed for many [s,t]'s, it makes
     * sense to actually compute the reciprocal of major_mag and
     * minor_mag and multiply them by the above unit lengths.
     */
    /*
     * History:
     *
     * ClampUpAxes, the ImageMagick function (found in resample.c)
     * on which this is based, was written by Nicolas Robidoux and
     * Chantal Racette of Laurentian University with insightful
     * suggestions from Anthony Thyssen and funding from the
     * National Science and Engineering Research Council of
     * Canada. It is distinguished from its predecessors by its
     * efficient handling of degenerate cases.
     *
     * The idea of clamping up the EWA ellipse's major and minor
     * axes so that the result contains the reconstruction kernel
     * filter support is taken from Andreas Gustaffson's Masters
     * thesis "Interactive Image Warping", Helsinki University of
     * Technology, Faculty of Information Technology, 59 pages, 1993
     * (see Section 3.6).
     *
     * The use of the SVD to clamp up the singular values of the
     * Jacobian matrix of the pullback transformation for EWA
     * resampling is taken from the astrophysicist Craig DeForest.
     * It is implemented in his PDL::Transform code (PDL = Perl Data
     * Language).
     *
     * SVD reference:
     * "We Recommend Singular Value Decomposition" by David Austin
     * http://www.ams.org/samplings/feature-column/fcarc-svd
     *
     * Ellipse reference:
     * http://en.wikipedia.org/wiki/Ellipse#Canonical_form
     */
    /*
     * Use the scale object if defined. Otherwise, assume that the
     * inverse Jacobian matrix is the identity.
     */
    const gdouble a = scale ? scale->coeff[0][0] : 1;
    const gdouble b = scale ? scale->coeff[0][1] : 0;
    const gdouble c = scale ? scale->coeff[1][0] : 0;
    const gdouble d = scale ? scale->coeff[1][1] : 1;

    /*
     * Computations are done in double precision because "direct"
     * SVD computations are prone to round off error. (Computing in
     * single precision most likely would be fine.)
     */
    /*
     * n is the matrix Jinv * transpose(Jinv). Eigenvalues of n are
     * the squares of the singular values of Jinv.
     */
    const gdouble aa = a * a;
    const gdouble bb = b * b;
    const gdouble cc = c * c;
    const gdouble dd = d * d;
    /*
     * Eigenvectors of n are left singular vectors of Jinv.
     */
    const gdouble n11 = aa + bb;
    const gdouble n12 = a * c + b * d;
    const gdouble n21 = n12;
    const gdouble n22 = cc + dd;
    const gdouble det = a * d - b * c;
    const gdouble twice_det = det + det;
    const gdouble frobenius_squared = n11 + n22;
    const gdouble discriminant =
      ( frobenius_squared + twice_det ) * ( frobenius_squared - twice_det );
    /*
     * In exact arithmetic, the discriminant cannot be negative. In
     * floating point, it can, leading a non-deterministic bug in
     * ImageMagick (now fixed, thanks to Cristy, the lead dev).
     */
    const gdouble sqrt_discriminant =
      sqrt (discriminant > 0. ? discriminant : 0.);

    /*
     * Initially, we only compute the squares of the singular
     * values.
     */
    /*
     * s1 is the largest singular value of the inverse Jacobian
     * matrix. In other words, its reciprocal is the smallest
     * singular value of the Jacobian matrix itself.  If s1 = 0,
     * both singular values are 0, and any orthogonal pair of left
     * and right factors produces a singular decomposition of Jinv.
     */
    const gdouble twice_s1s1 = frobenius_squared + sqrt_discriminant;
    /*
     * If s1 <= 1, the forward transformation is not downsampling in
     * any direction, and consequently we do not need the
     * downsampling scheme at all.
     */

    /*
     * Following now done by arithmetic branching.
     */
    // if (twice_s1s1 > (gdouble) 2.) 
      {
        /*
         * The result (most likely) has a nonzero EWA component.
         */
        const gdouble s1s1 = (gdouble) 0.5 * twice_s1s1;
        /*
         * s2 the smallest singular value of the inverse Jacobian
         * matrix. Its reciprocal is the largest singular value of
         * the Jacobian matrix itself.
         */
        const gdouble s2s2 = 0.5 * ( frobenius_squared - sqrt_discriminant );

        const gdouble s1s1minusn11 = s1s1 - n11;
        const gdouble s1s1minusn22 = s1s1 - n22;
        /*
         * u1, the first column of the U factor of a singular
         * decomposition of Jinv, is a (non-normalized) left
         * singular vector corresponding to s1. It has entries u11
         * and u21. We compute u1 from the fact that it is an
         * eigenvector of n corresponding to the eigenvalue s1^2.
         */
        const gdouble s1s1minusn11_squared = s1s1minusn11 * s1s1minusn11;
        const gdouble s1s1minusn22_squared = s1s1minusn22 * s1s1minusn22;
        /*
         * The following selects the largest row of n-s1^2 I ("I"
         * being the 2x2 identity matrix) as the one which is used
         * to find the eigenvector. If both s1^2-n11 and s1^2-n22
         * are zero, n-s1^2 I is the zero matrix.  In that case, any
         * vector is an eigenvector; in addition, norm below is
         * equal to zero, and, in exact arithmetic, this is the only
         * case in which norm = 0. So, setting u1 to the simple but
         * arbitrary vector [1,0] if norm = 0 safely takes care of
         * all cases.
         */
        const gdouble temp_u11 =
          s1s1minusn11_squared >= s1s1minusn22_squared
          ?
          n12
          :
          s1s1minusn22;
        const gdouble temp_u21 =
          s1s1minusn11_squared >= s1s1minusn22_squared
          ?
          s1s1minusn11
          :
          n21;
        const gdouble norm =
          sqrt( temp_u11 * temp_u11 + temp_u21 * temp_u21 );
        /*
         * Finalize the entries of first left singular vector
         * (associated with the largest singular value).
         */
        const gdouble u11 =
          norm > (gdouble) 0.0 ? temp_u11 / norm : (gdouble) 1.0;
        const gdouble u21 =
          norm > (gdouble) 0.0 ? temp_u21 / norm : (gdouble) 0.0;
        /*
         * Clamp the singular values up to 1:
         */
        const gdouble major_mag =
          s1s1 <= (gdouble) 1.0 ? (gdouble) 1.0 : sqrt( s1s1 );
        const gdouble minor_mag =
          s2s2 <= (gdouble) 1.0 ? (gdouble) 1.0 : sqrt( s2s2 );
        /*
         * Unit major and minor axis direction vectors:
         */
        const gdouble major_unit_x =  u11;
        const gdouble major_unit_y =  u21;
        const gdouble minor_unit_x = -u21;
        const gdouble minor_unit_y =  u11;

        /*
         * The square of the distance to the key location in output
         * place of a point [s,t] in input space is the square root of
         *
         * ( s * c_major_x + t * c_major_y )^2
         * +
         * ( s * c_minor_x + t * c_minor_y )^2.
         */
        const gfloat c_major_x = major_unit_x / major_mag;
        const gfloat c_major_y = major_unit_y / major_mag;
        const gfloat c_minor_x = minor_unit_x / minor_mag;
        const gfloat c_minor_y = minor_unit_y / minor_mag;

        /*
         * Remainder of the ellipse geometry computation:
         */
        /*
         * Major and minor axis direction vectors:
         */
        const gdouble major_x = major_mag * major_unit_x;
        const gdouble major_y = major_mag * major_unit_y;
        const gdouble minor_x = minor_mag * minor_unit_x;
        const gdouble minor_y = minor_mag * minor_unit_y;

        /*
         * Ellipse coefficients:
         */
        const gdouble ellipse_a =
          major_y * major_y + minor_y * minor_y;
        const gdouble folded_ellipse_b =
          major_x * major_y + minor_x * minor_y;
        const gdouble ellipse_c =
          major_x * major_x + minor_x * minor_x;
        const gdouble ellipse_f = major_mag * minor_mag;

        /*
         * ewa_radius is the unscaled radius, which here is 2
         * because we use EWA Robidoux, which is based on a Keys
         * cubic.
         */
        const gfloat ewa_radius = 2.;
        /*
         * Bounding box of the ellipse:
         */
        const gdouble bounding_box_factor =
          ellipse_f * ellipse_f /
          ( ellipse_c * ellipse_a - folded_ellipse_b * folded_ellipse_b );
        const gfloat bounding_box_half_width =
          ewa_radius * sqrtf( (gfloat) (ellipse_c * bounding_box_factor) );
        const gfloat bounding_box_half_height =
          ewa_radius * sqrtf( (gfloat) (ellipse_a * bounding_box_factor) );

        /*
         * Relative weight of the contribution of
         * Mitchell-Netravali:
         */
        const gfloat theta = (gfloat) ( (gdouble) 1. / ellipse_f );

        /*
         * Grab the pixel values located within the level 0
         * context_rect.
         */
        const gint out_left_0 =
          LOHALO_MAX
          (
            (gint) int_ceilf  ( (float) ( x_0 - bounding_box_half_width  ) )
            ,
            -LOHALO_OFFSET_0
          );
        const gint out_rite_0 =
          LOHALO_MIN
          (
            (gint) int_floorf ( (float) ( x_0 + bounding_box_half_width  ) )
            ,
            LOHALO_OFFSET_0
          );
        const gint out_top_0 =
          LOHALO_MAX
          (
            (gint) int_ceilf  ( (float) ( y_0 - bounding_box_half_height ) )
            ,
            -LOHALO_OFFSET_0
          );
        const gint out_bot_0 =
          LOHALO_MIN
          (
            (gint) int_floorf ( (float) ( y_0 + bounding_box_half_height ) )
            ,
            LOHALO_OFFSET_0
          );

        /*
         * Accumulator for the EWA weights:
         */
        gdouble total_weight = (gdouble) 0.0;
        /*
         * Storage for the EWA contribution:
         */
        gfloat ewa_newval[channels];
        ewa_newval[0] = (gfloat) 0;
        ewa_newval[1] = (gfloat) 0;
        ewa_newval[2] = (gfloat) 0;
        ewa_newval[3] = (gfloat) 0;

        {
          gint i = out_top_0;
          do {
            gint j = out_left_0;
            do {
              ewa_update (j,
                          i,
                          c_major_x,
                          c_major_y,
                          c_minor_x,
                          c_minor_y,
                          x_0,
                          y_0,
                          channels,
                          row_skip,
                          input_ptr,
                          &total_weight,
                          ewa_newval);
            } while ( ++j <= out_rite_0 );
          } while ( ++i <= out_bot_0 );
        }

        {
          /*
           * Blend the sigmoidized Mitchell-Netravali and EWA Robidoux
           * results:
           */
          const gfloat beta = twice_s1s1 > (gdouble) 2. ? (gfloat) ( ( (gdouble) 1.0 - theta ) / total_weight ) : (gfloat) 0.;
          const gfloat newtheta = twice_s1s1 > (gdouble) 2. ? theta : (gfloat) 1.;
          gint c;
          for (c = 0; c < channels; c++)
            newval[c] = newtheta * newval[c] + beta * ewa_newval[c];
        }
      }

    /*
     * Ship out the result:
     */
    babl_process (self->fish, newval, output, 1);
    return;
  }
}
