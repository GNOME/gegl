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
 * Copyright 2011 Michael Muré <batolettre@gmail.com>
 *
 */

/* This file holds public enums from GEGL
 *
 * !!!!!!!!!!!! NOTE !!!!!!!!!!!!!!
 *
 * Normally, gegl-enums.c file would be be generated my glib-mkenums,
 * but we use the enum values' registered names for translatable,
 * human readable labels for the GUI, so gegl-enums.c is maintained
 * manually.
 *
 * DON'T FORGET TO UPDATE gegl-enums.c AFTER CHANGING THIS HEADER
 *
 * !!!!!!!!!!!! NOTE !!!!!!!!!!!!!!
 */

#ifndef __GEGL_ENUMS_H__
#define __GEGL_ENUMS_H__

G_BEGIN_DECLS

typedef enum {
  GEGL_DITHER_NONE,
  GEGL_DITHER_FLOYD_STEINBERG,
  GEGL_DITHER_BAYER,
  GEGL_DITHER_RANDOM,
  GEGL_DITHER_RANDOM_COVARIANT,
  GEGL_DITHER_ARITHMETIC_ADD,
  GEGL_DITHER_ARITHMETIC_ADD_COVARIANT,
  GEGL_DITHER_ARITHMETIC_XOR,
  GEGL_DITHER_ARITHMETIC_XOR_COVARIANT,
  GEGL_DITHER_BLUE_NOISE,
  GEGL_DITHER_BLUE_NOISE_COVARIANT,
} GeglDitherMethod;

GType gegl_dither_method_get_type (void) G_GNUC_CONST;

#define GEGL_TYPE_DITHER_METHOD (gegl_dither_method_get_type ())

typedef enum {
  GEGL_DISTANCE_METRIC_EUCLIDEAN,
  GEGL_DISTANCE_METRIC_MANHATTAN,
  GEGL_DISTANCE_METRIC_CHEBYSHEV
} GeglDistanceMetric;

GType gegl_distance_metric_get_type (void) G_GNUC_CONST;

#define GEGL_TYPE_DISTANCE_METRIC (gegl_distance_metric_get_type ())


typedef enum {
  GEGL_ORIENTATION_HORIZONTAL,
  GEGL_ORIENTATION_VERTICAL
} GeglOrientation;

GType gegl_orientation_get_type (void) G_GNUC_CONST;

#define GEGL_TYPE_ORIENTATION (gegl_orientation_get_type ())


enum _GeglBablVariant
{
  GEGL_BABL_VARIANT_FLOAT=0,
     /* pass this one to just ensure a format is float */
  GEGL_BABL_VARIANT_LINEAR,
     /* Y YA RGB RGBA         */
  GEGL_BABL_VARIANT_NONLINEAR,
     /* Y' Y'A R'G'B' R'G'B'A */
  GEGL_BABL_VARIANT_PERCEPTUAL,
     /* Y~ Y~A R~G~B~ R~G~B~A */
  GEGL_BABL_VARIANT_LINEAR_PREMULTIPLIED,
     /* YaA RaGaBaA           */
  GEGL_BABL_VARIANT_PERCEPTUAL_PREMULTIPLIED,
     /* Y~aA R~aG~aB~aA       */
  GEGL_BABL_VARIANT_LINEAR_PREMULTIPLIED_IF_ALPHA,
     /* Y YaA RGB RaGaBaA           */
  GEGL_BABL_VARIANT_PERCEPTUAL_PREMULTIPLIED_IF_ALPHA,
     /* Y~ Y~aA R~G~B~A R~aG~aB~aA       */
  GEGL_BABL_VARIANT_ALPHA
     /* add alpha if missing keep as premultiplied if already so  */
};
typedef enum _GeglBablVariant GeglBablVariant;

GType gegl_babl_variant_get_type (void) G_GNUC_CONST;

#define GEGL_TYPE_BABL_VARIANT (gegl_babl_variant_get_type ())


typedef enum {
  GEGL_CACHE_POLICY_AUTO,
  GEGL_CACHE_POLICY_NEVER,
  GEGL_CACHE_POLICY_ALWAYS
} GeglCachePolicy;

GType gegl_cache_policy_get_type (void) G_GNUC_CONST;

#define GEGL_TYPE_CACHE_POLICY (gegl_cache_policy_get_type ())

G_END_DECLS

#endif /* __GEGL_ENUMS_H__ */
