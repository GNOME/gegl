/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 *
 * 2013 Daniel Sabo
 */

#ifndef __GEGL_ALGORITHMS_H__
#define __GEGL_ALGORITHMS_H__


#include "gegl-buffer.h"
G_BEGIN_DECLS

#define GEGL_SCALE_EPSILON 1.e-6

#ifdef SIMD_X86_64_V2
#define GEGL_SIMD_SUFFIX(symbol)  symbol##_x86_64_v2
#endif
#ifdef SIMD_X86_64_V3
#define GEGL_SIMD_SUFFIX(symbol)  symbol##_x86_64_v3
#endif
#ifdef SIMD_ARM_NEON
#define GEGL_SIMD_SUFFIX(symbol)  symbol##_arm_neon
#endif
#ifndef GEGL_SIMD_SUFFIX
#define GEGL_SIMD_SUFFIX(symbol)  symbol##_generic
#ifndef SIMD_GENERIC
#define SIMD_GENERIC
#endif
#endif


void GEGL_SIMD_SUFFIX(gegl_downscale_2x2) (const Babl *format,
                         gint        src_width,
                         gint        src_height,
                         guchar     *src_data,
                         gint        src_rowstride,
                         guchar     *dst_data,
                         gint        dst_rowstride);

typedef void (*GeglDownscale2x2Fun) (const Babl *format,
                                     gint    src_width,
                                     gint    src_height,
                                     guchar *src_data,
                                     gint    src_rowstride,
                                     guchar *dst_data,
                                     gint    dst_rowstride);

void GEGL_SIMD_SUFFIX(gegl_downscale_2x2_nearest) (const Babl *format,
                                 gint        src_width,
                                 gint        src_height,
                                 guchar     *src_data,
                                 gint        src_rowstride,
                                 guchar      *dst_data,
                                 gint        dst_rowstride);

/* Attempt to resample with a 3x3 boxfilter, if no boxfilter is
 * available for #format fall back to nearest neighbor.
 * #scale is assumed to be between 0.5 and +inf.
 */
void GEGL_SIMD_SUFFIX(gegl_resample_boxfilter) (guchar              *dest_buf,
                              const guchar        *source_buf,
                              const GeglRectangle *dst_rect,
                              const GeglRectangle *src_rect,
                              gint                 s_rowstride,
                              gdouble              scale,
                              const Babl          *format,
                              gint                 d_rowstride);

/* Attempt to resample with a 2x2 bilinear filter, if no implementation is
 * available for #format fall back to nearest neighbor.
 */
void GEGL_SIMD_SUFFIX(gegl_resample_bilinear) (guchar              *dest_buf,
                             const guchar        *source_buf,
                             const GeglRectangle *dst_rect,
                             const GeglRectangle *src_rect,
                             gint                 s_rowstride,
                             gdouble              scale,
                             const Babl          *format,
                             gint                 d_rowstride);

void GEGL_SIMD_SUFFIX(gegl_resample_nearest) (guchar              *dst,
                            const guchar        *src,
                            const GeglRectangle *dst_rect,
                            const GeglRectangle *src_rect,
                            gint                 src_stride,
                            gdouble              scale,
                            gint                 bpp,
                            gint                 dst_stride);

GeglDownscale2x2Fun GEGL_SIMD_SUFFIX(gegl_downscale_2x2_get_fun) (const Babl *format);

#ifdef ARCH_X86_64
GeglDownscale2x2Fun gegl_downscale_2x2_get_fun_x86_64_v2 (const Babl *format);
GeglDownscale2x2Fun gegl_downscale_2x2_get_fun_x86_64_v3 (const Babl *format);
#endif

#define GEGL_ALGORITHMS_LUT_DIVISOR 16

G_END_DECLS

#endif /* __GEGL_ALGORITHMS_H__ */
