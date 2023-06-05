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
 * Copyright 2006,2007,2015,2018 Øyvind Kolås <pippin@gimp.org>
 *           2013 Daniel Sabo
 */

#include "config.h"

#include <string.h>
#include <stdint.h>
#include <glib-object.h>

#include <babl/babl.h>

#include "gegl-buffer.h"
#include "gegl-buffer-formats.h"
#include "gegl-algorithms.h"

#include <math.h>

void
GEGL_SIMD_SUFFIX(gegl_downscale_2x2) (const Babl *format,
                                      gint        src_width,
                                      gint        src_height,
                                      guchar     *src_data,
                                      gint        src_rowstride,
                                      guchar     *dst_data,
                                      gint        dst_rowstride)
{
  GEGL_SIMD_SUFFIX(gegl_downscale_2x2_get_fun) (format)(format, src_width, src_height,
                                              src_data, src_rowstride,
                                              dst_data, dst_rowstride);;
}

#include <stdio.h>

#define ALIGN 16
static inline void *align_16 (unsigned char *ret)
{
  int offset = ALIGN - ((uintptr_t) ret) % ALIGN;
  ret = ret + offset;
  return ret;
}


extern uint16_t gegl_lut_u8_to_u16[256];
extern float    gegl_lut_u8_to_u16f[256];
extern uint8_t  gegl_lut_u16_to_u8[65536/GEGL_ALGORITHMS_LUT_DIVISOR];


static void
gegl_boxfilter_u8_nl (guchar              *dest_buf,
                      const guchar        *source_buf,
                      const GeglRectangle *dst_rect,
                      const GeglRectangle *src_rect,
                      const gint           s_rowstride,
                      const gdouble        scale,
                      const Babl          *format,
                      const gint           bpp,
                      const gint           d_rowstride)
{
  const uint8_t *src[9];
  gint  components = bpp / sizeof(uint8_t);

  gfloat left_weight[dst_rect->width];
  gfloat right_weight[dst_rect->width];

  gint   jj[dst_rect->width];

  for (gint x = 0; x < dst_rect->width; x++)
  {
    gfloat sx  = (dst_rect->x + x + .5f) / scale - src_rect->x;
    jj[x]  = int_floorf (sx);

    left_weight[x]   = .5f - scale * (sx - jj[x]);
    left_weight[x]   = MAX (0.0f, left_weight[x]);
    right_weight[x]  = .5f - scale * ((jj[x] + 1) - sx);
    right_weight[x]  = MAX (0.0f, right_weight[x]);

    jj[x] *= components;
  }

  for (gint y = 0; y < dst_rect->height; y++)
    {
      gfloat top_weight, middle_weight, bottom_weight;
      const gfloat sy = (dst_rect->y + y + .5f) / scale - src_rect->y;
      const gint     ii = int_floorf (sy);
      uint8_t             *dst = (uint8_t*)(dest_buf + y * d_rowstride);
      const guchar  *src_base = source_buf + ii * s_rowstride;

      top_weight    = .5f - scale * (sy - ii);
      top_weight    = MAX (0.f, top_weight);
      bottom_weight = .5f - scale * ((ii + 1 ) - sy);
      bottom_weight = MAX (0.f, bottom_weight);
      middle_weight = 1.f - top_weight - bottom_weight;

#define CASE(case_val, ...)\
  case case_val:\
    for (gint x = 0; x < dst_rect->width; x++)\
      {\
      src[4] = (const uint8_t*)src_base + jj[x];\
      src[1] = src[4] - s_rowstride;\
      src[7] = src[4] + s_rowstride;\
      src[2] = src[1] + case_val;\
      src[5] = src[4] + case_val;\
      src[8] = src[7] + case_val;\
      src[0] = src[1] - case_val;\
      src[3] = src[4] - case_val;\
      src[6] = src[7] - case_val;\
      {\
        const gfloat l = left_weight[x];\
        const gfloat r = right_weight[x];\
        const gfloat c = 1.f-l-r;\
        const gfloat t = top_weight;\
        const gfloat m = middle_weight;\
        const gfloat b = bottom_weight;\
        __VA_ARGS__;\
      }\
      dst += components;\
      }\
  break;\

      switch (components)
      {
        case 4:
          for (gint x = 0; x < dst_rect->width; x++)
            {
              src[4] = (const uint8_t*)src_base + jj[x];
              src[1] = src[4] - s_rowstride;
              src[7] = src[4] + s_rowstride;
              src[2] = src[1] + 4;
              src[5] = src[4] + 4;
              src[8] = src[7] + 4;
              src[0] = src[1] - 4;
              src[3] = src[4] - 4;
              src[6] = src[7] - 4;

            {
              const gfloat l = left_weight[x];
              const gfloat r = right_weight[x];
              const gfloat c = 1.f - l - r;

              const gfloat t = top_weight;
              const gfloat m = middle_weight;
              const gfloat b = bottom_weight;

#define C(val)                     gegl_lut_u8_to_u16f[(val)]
#define BOXFILTER_ROUND(val)       gegl_lut_u16_to_u8[((int)((val)+0.5f))]
#define BOXFILTER_ROUND_ALPHA(val) ((int)((val)+0.5f))
              dst[0] = BOXFILTER_ROUND(
                (C(src[0][0]) * t + C(src[3][0]) * m + C(src[6][0]) * b) * l +
                (C(src[1][0]) * t + C(src[4][0]) * m + C(src[7][0]) * b) * c +
                (C(src[2][0]) * t + C(src[5][0]) * m + C(src[8][0]) * b) * r);
              dst[1] = BOXFILTER_ROUND(
                (C(src[0][1]) * t + C(src[3][1]) * m + C(src[6][1]) * b) * l +
                (C(src[1][1]) * t + C(src[4][1]) * m + C(src[7][1]) * b) * c +
                (C(src[2][1]) * t + C(src[5][1]) * m + C(src[8][1]) * b) * r);
              dst[2] = BOXFILTER_ROUND(
                (C(src[0][2]) * t + C(src[3][2]) * m + C(src[6][2]) * b) * l +
                (C(src[1][2]) * t + C(src[4][2]) * m + C(src[7][2]) * b) * c +
                (C(src[2][2]) * t + C(src[5][2]) * m + C(src[8][2]) * b) * r);
              dst[3] = BOXFILTER_ROUND(
                (C(src[0][3]) * t + C(src[3][3]) * m + C(src[6][3]) * b) * l +
                (C(src[1][3]) * t + C(src[4][3]) * m + C(src[7][3]) * b) * c +
                (C(src[2][3]) * t + C(src[5][3]) * m + C(src[8][3]) * b) * r);
              }
            dst += 4;
            }
          break;
        CASE(3,
              dst[0] = BOXFILTER_ROUND(
                (C(src[0][0]) * t + C(src[3][0]) * m + C(src[6][0]) * b) * l +
                (C(src[1][0]) * t + C(src[4][0]) * m + C(src[7][0]) * b) * c +
                (C(src[2][0]) * t + C(src[5][0]) * m + C(src[8][0]) * b) * r);
              dst[1] = BOXFILTER_ROUND(
                (C(src[0][1]) * t + C(src[3][1]) * m + C(src[6][1]) * b) * l +
                (C(src[1][1]) * t + C(src[4][1]) * m + C(src[7][1]) * b) * c +
                (C(src[2][1]) * t + C(src[5][1]) * m + C(src[8][1]) * b) * r);
              dst[2] = BOXFILTER_ROUND(
                (C(src[0][2]) * t + C(src[3][2]) * m + C(src[6][2]) * b) * l +
                (C(src[1][2]) * t + C(src[4][2]) * m + C(src[7][2]) * b) * c +
                (C(src[2][2]) * t + C(src[5][2]) * m + C(src[8][2]) * b) * r);
            );
        CASE(2,
              dst[0] = BOXFILTER_ROUND(
                (C(src[0][0]) * t + C(src[3][0]) * m + C(src[6][0]) * b) * l +
                (C(src[1][0]) * t + C(src[4][0]) * m + C(src[7][0]) * b) * c +
                (C(src[2][0]) * t + C(src[5][0]) * m + C(src[8][0]) * b) * r);
              dst[1] = BOXFILTER_ROUND(
                (C(src[0][1]) * t + C(src[3][1]) * m + C(src[6][1]) * b) * l +
                (C(src[1][1]) * t + C(src[4][1]) * m + C(src[7][1]) * b) * c +
                (C(src[2][1]) * t + C(src[5][1]) * m + C(src[8][1]) * b) * r);
            );
        CASE(1,
              dst[0] = BOXFILTER_ROUND(
                (C(src[0][0]) * t + C(src[3][0]) * m + C(src[6][0]) * b) * l +
                (C(src[1][0]) * t + C(src[4][0]) * m + C(src[7][0]) * b) * c +
                (C(src[2][0]) * t + C(src[5][0]) * m + C(src[8][0]) * b) * r);
            );
        default:
        CASE(0,
              for (gint i = 0; i < components; ++i)
                {
                  dst[i] = BOXFILTER_ROUND(
                  (C(src[0][i]) * t + C(src[3][i]) * m + C(src[6][i]) * b) * l +
                  (C(src[1][i]) * t + C(src[4][i]) * m + C(src[7][i]) * b) * c +
                  (C(src[2][i]) * t + C(src[5][i]) * m + C(src[8][i]) * b) * r);
                }
           );
      }
    }
}

static void
gegl_boxfilter_u8_nl_alpha (guchar              *dest_buf,
                            const guchar        *source_buf,
                            const GeglRectangle *dst_rect,
                            const GeglRectangle *src_rect,
                            const gint           s_rowstride,
                            const gdouble        scale,
                            const Babl          *format,
                            const gint           bpp,
                            const gint           d_rowstride)
{
  const uint8_t *src[9];
  gint  components = bpp / sizeof(uint8_t);

  gfloat left_weight[dst_rect->width];
  gfloat right_weight[dst_rect->width];

  gint   jj[dst_rect->width];

  for (gint x = 0; x < dst_rect->width; x++)
  {
    gfloat sx  = (dst_rect->x + x + .5f) / scale - src_rect->x;
    jj[x]  = int_floorf (sx);

    left_weight[x]   = .5f - scale * (sx - jj[x]);
    left_weight[x]   = MAX (0.0f, left_weight[x]);
    right_weight[x]  = .5f - scale * ((jj[x] + 1) - sx);
    right_weight[x]  = MAX (0.0f, right_weight[x]);

    jj[x] *= components;
  }

  for (gint y = 0; y < dst_rect->height; y++)
    {
      gfloat top_weight, middle_weight, bottom_weight;
      const gfloat sy = (dst_rect->y + y + .5f) / scale - src_rect->y;
      const gint     ii = int_floorf (sy);
      uint8_t             *dst = (uint8_t*)(dest_buf + y * d_rowstride);
      const guchar  *src_base = source_buf + ii * s_rowstride;

      top_weight    = .5f - scale * (sy - ii);
      top_weight    = MAX (0.f, top_weight);
      bottom_weight = .5f - scale * ((ii + 1 ) - sy);
      bottom_weight = MAX (0.f, bottom_weight);
      middle_weight = 1.f - top_weight - bottom_weight;

      switch (components)
      {
        case 4:
          for (gint x = 0; x < dst_rect->width; x++)
            {
              src[4] = (const uint8_t*)src_base + jj[x];
              src[1] = src[4] - s_rowstride;
              src[7] = src[4] + s_rowstride;
              src[2] = src[1] + 4;
              src[5] = src[4] + 4;
              src[8] = src[7] + 4;
              src[0] = src[1] - 4;
              src[3] = src[4] - 4;
              src[6] = src[7] - 4;

            {
              const gfloat l = left_weight[x];
              const gfloat r = right_weight[x];
              const gfloat c = 1.f - l - r;

              const gfloat t = top_weight;
              const gfloat m = middle_weight;
              const gfloat b = bottom_weight;

              dst[0] = BOXFILTER_ROUND(
                (C(src[0][0]) * t + C(src[3][0]) * m + C(src[6][0]) * b) * l +
                (C(src[1][0]) * t + C(src[4][0]) * m + C(src[7][0]) * b) * c +
                (C(src[2][0]) * t + C(src[5][0]) * m + C(src[8][0]) * b) * r);
              dst[1] = BOXFILTER_ROUND(
                (C(src[0][1]) * t + C(src[3][1]) * m + C(src[6][1]) * b) * l +
                (C(src[1][1]) * t + C(src[4][1]) * m + C(src[7][1]) * b) * c +
                (C(src[2][1]) * t + C(src[5][1]) * m + C(src[8][1]) * b) * r);
              dst[2] = BOXFILTER_ROUND(
                (C(src[0][2]) * t + C(src[3][2]) * m + C(src[6][2]) * b) * l +
                (C(src[1][2]) * t + C(src[4][2]) * m + C(src[7][2]) * b) * c +
                (C(src[2][2]) * t + C(src[5][2]) * m + C(src[8][2]) * b) * r);
              dst[3] = BOXFILTER_ROUND_ALPHA(
                ((src[0][3]) * t + (src[3][3]) * m + (src[6][3]) * b) * l +
                ((src[1][3]) * t + (src[4][3]) * m + (src[7][3]) * b) * c +
                ((src[2][3]) * t + (src[5][3]) * m + (src[8][3]) * b) * r);
              }
            dst += 4;
            }
          break;
        CASE(2,
              dst[0] = BOXFILTER_ROUND(
                (C(src[0][0]) * t + C(src[3][0]) * m + C(src[6][0]) * b) * l +
                (C(src[1][0]) * t + C(src[4][0]) * m + C(src[7][0]) * b) * c +
                (C(src[2][0]) * t + C(src[5][0]) * m + C(src[8][0]) * b) * r);
              dst[1] = BOXFILTER_ROUND_ALPHA(
                ((src[0][1]) * t + (src[3][1]) * m + (src[6][1]) * b) * l +
                ((src[1][1]) * t + (src[4][1]) * m + (src[7][1]) * b) * c +
                ((src[2][1]) * t + (src[5][1]) * m + (src[8][1]) * b) * r);
            );
        break;
        default:
        CASE(0,
              for (gint i = 0; i < components - 1; ++i)
                {
                  dst[i] = BOXFILTER_ROUND(
                  (C(src[0][i]) * t + C(src[3][i]) * m + C(src[6][i]) * b) * l +
                  (C(src[1][i]) * t + C(src[4][i]) * m + C(src[7][i]) * b) * c +
                  (C(src[2][i]) * t + C(src[5][i]) * m + C(src[8][i]) * b) * r);
                }
              dst[components-1] = BOXFILTER_ROUND_ALPHA(
                  ((src[0][components-1]) * t + (src[3][components-1]) * m + (src[6][components-1]) * b) * l +
                  ((src[1][components-1]) * t + (src[4][components-1]) * m + (src[7][components-1]) * b) * c +
                  ((src[2][components-1]) * t + (src[5][components-1]) * m + (src[8][components-1]) * b) * r);
           );
      }
    }
}
#undef CASE
#undef C
#undef BOXFILTER_ROUND
#undef BOXFILTER_ROUND_ALPHA

static void
gegl_bilinear_u8_nl (guchar              *dest_buf,
                     const guchar        *source_buf,
                     const GeglRectangle *dst_rect,
                     const GeglRectangle *src_rect,
                     const gint           s_rowstride,
                     const gdouble        scale,
                     const gint           components,
                     const gint           d_rowstride)
{
  const gint ver  = s_rowstride;
  const gint diag = ver + components;
  const gint dst_y = dst_rect->y;
  const gint src_y = src_rect->y;
  const gint dst_width = dst_rect->width;
  const gint dst_height = dst_rect->height;
  gfloat dx[dst_rect->width];
  gint jj[dst_rect->width];

  for (gint x = 0; x < dst_rect->width; x++)
  {
    gfloat sx  = (dst_rect->x + x + 0.5f) / scale - src_rect->x - 0.5f;
    jj[x]  = int_floorf (sx);
    dx[x]  = (sx - jj[x]);
    jj[x] *= components;
  }
#define IMPL(components, ...) do{ \
  for (gint y = 0; y < dst_height; y++)\
    {\
      const gfloat sy = (dst_y + y + 0.5f) / scale - src_y - 0.5f;\
      const gint   ii = int_floorf (sy);\
      const gfloat dy = (sy - ii);\
      const gfloat rdy = 1.0f - dy;\
      guchar *dst = (guchar*)(dest_buf + y * d_rowstride);\
      const guchar  *src_base = source_buf + ii * s_rowstride;\
      gfloat *idx=&dx[0];\
      gint *ijj=&jj[0];\
\
      for (gint x = 0; x < dst_width; x++)\
      {\
        const gfloat ldx = *(idx++);\
        const gfloat rdx = 1.0f-ldx;\
        const guchar *src[4];\
        src[0] = (const guchar*)(src_base) + *(ijj++);\
        src[1] = src[0] + components;\
        src[2] = src[0] + ver;\
        src[3] = src[0] + diag;\
        __VA_ARGS__;\
        dst += components;\
      }\
    }\
}while(0)

#define C(val)                    gegl_lut_u8_to_u16f[(val)]
#define BILINEAR_ROUND(val)       gegl_lut_u16_to_u8[((int)((val)+0.5f))]
#define BILINEAR_ROUND_ALPHA(val) ((int)((val)+0.5f))

   switch (components)
   {
     default:
       IMPL(components,
         for (gint i = 0; i < components; ++i)
            dst[i] =
               BILINEAR_ROUND(
               (C(src[0][i]) * rdx + C(src[1][i]) * ldx) * rdy +
               (C(src[2][i]) * rdx + C(src[3][i]) * ldx) * dy);
         );
       break;
#if 1
     case 1:
       IMPL(1,
         dst[0] = BILINEAR_ROUND(
               (C(src[0][0]) * rdx + C(src[1][0]) * ldx) * rdy +
               (C(src[2][0]) * rdx + C(src[3][0]) * ldx) * dy);
           );
       break;
     case 2:
       IMPL(2,
         dst[0] = BILINEAR_ROUND(
               (C(src[0][0]) * rdx + C(src[1][0]) * ldx) * rdy +
               (C(src[2][0]) * rdx + C(src[3][0]) * ldx) * dy);
         dst[1] = BILINEAR_ROUND(
               (C(src[0][1]) * rdx + C(src[1][1]) * ldx) * rdy +
               (C(src[2][1]) * rdx + C(src[3][1]) * ldx) * dy);
           );
       break;
     case 3:
       IMPL(3,
         dst[0] = BILINEAR_ROUND(
               (C(src[0][0]) * rdx + C(src[1][0]) * ldx) * rdy +
               (C(src[2][0]) * rdx + C(src[3][0]) * ldx) * dy);
         dst[1] = BILINEAR_ROUND(
               (C(src[0][1]) * rdx + C(src[1][1]) * ldx) * rdy +
               (C(src[2][1]) * rdx + C(src[3][1]) * ldx) * dy);
         dst[2] = BILINEAR_ROUND(
               (C(src[0][2]) * rdx + C(src[1][2]) * ldx) * rdy +
               (C(src[2][2]) * rdx + C(src[3][2]) * ldx) * dy);
           );
       break;
     case 4:
       IMPL(4,
           dst[0] = BILINEAR_ROUND(
               (C(src[0][0]) * rdx + C(src[1][0]) * ldx) * rdy +
               (C(src[2][0]) * rdx + C(src[3][0]) * ldx) * dy);
           dst[1] = BILINEAR_ROUND(
               (C(src[0][1]) * rdx + C(src[1][1]) * ldx) * rdy +
               (C(src[2][1]) * rdx + C(src[3][1]) * ldx) * dy);
           dst[2] = BILINEAR_ROUND(
               (C(src[0][2]) * rdx + C(src[1][2]) * ldx) * rdy +
               (C(src[2][2]) * rdx + C(src[3][2]) * ldx) * dy);
           dst[3] = BILINEAR_ROUND(
               (C(src[0][3]) * rdx + C(src[1][3]) * ldx) * rdy +
               (C(src[2][3]) * rdx + C(src[3][3]) * ldx) * dy);
           );
       break;
     case 5:
       IMPL(5,
           dst[0] = BILINEAR_ROUND(
               (C(src[0][0]) * rdx + C(src[1][0]) * ldx) * rdy +
               (C(src[2][0]) * rdx + C(src[3][0]) * ldx) * dy);
           dst[1] = BILINEAR_ROUND(
               (C(src[0][1]) * rdx + C(src[1][1]) * ldx) * rdy +
               (C(src[2][1]) * rdx + C(src[3][1]) * ldx) * dy);
           dst[2] = BILINEAR_ROUND(
               (C(src[0][2]) * rdx + C(src[1][2]) * ldx) * rdy +
               (C(src[2][2]) * rdx + C(src[3][2]) * ldx) * dy);
           dst[3] = BILINEAR_ROUND(
               (C(src[0][3]) * rdx + C(src[1][3]) * ldx) * rdy +
               (C(src[2][3]) * rdx + C(src[3][3]) * ldx) * dy);
           dst[4] = BILINEAR_ROUND(
               (C(src[0][4]) * rdx + C(src[1][4]) * ldx) * rdy +
               (C(src[2][4]) * rdx + C(src[3][4]) * ldx) * dy);
           );
       break;
#endif
   }
}

static void
gegl_bilinear_u8_nl_alpha (guchar              *dest_buf,
                           const guchar        *source_buf,
                           const GeglRectangle *dst_rect,
                           const GeglRectangle *src_rect,
                           const gint           s_rowstride,
                           const gdouble        scale,
                           const gint           components,
                           const gint           d_rowstride)
{
  const gint ver  = s_rowstride;
  const gint diag = ver + components;
  const gint dst_y = dst_rect->y;
  const gint src_y = src_rect->y;
  const gint dst_width = dst_rect->width;
  const gint dst_height = dst_rect->height;
  gfloat dx[dst_rect->width];
  gint jj[dst_rect->width];

  for (gint x = 0; x < dst_rect->width; x++)
  {
    gfloat sx  = (dst_rect->x + x + 0.5f) / scale - src_rect->x - 0.5f;
    jj[x]  = int_floorf (sx);
    dx[x]  = (sx - jj[x]);
    jj[x] *= components;
  }

   switch (components)
   {
     default:
       IMPL(components,
         for (gint i = 0; i < components - 1; ++i)
            dst[i] =
               BILINEAR_ROUND(
               (C(src[0][i]) * rdx + C(src[1][i]) * ldx) * rdy +
               (C(src[2][i]) * rdx + C(src[3][i]) * ldx) * dy);
         dst[components - 1] =
           BILINEAR_ROUND_ALPHA(
             ((src[0][components-1]) * rdx + (src[1][components-1]) * ldx) * rdy +
             ((src[2][components-1]) * rdx + (src[3][components-1]) * ldx) * dy);
         );
       break;
     case 2:
       IMPL(2,
         dst[0] = BILINEAR_ROUND(
               (C(src[0][0]) * rdx + C(src[1][0]) * ldx) * rdy +
               (C(src[2][0]) * rdx + C(src[3][0]) * ldx) * dy);
         dst[1] = BILINEAR_ROUND_ALPHA(
               ((src[0][1]) * rdx + (src[1][1]) * ldx) * rdy +
               ((src[2][1]) * rdx + (src[3][1]) * ldx) * dy);
           );
       break;
     case 4:
       IMPL(4,
           dst[0] = BILINEAR_ROUND(
               (C(src[0][0]) * rdx + C(src[1][0]) * ldx) * rdy +
               (C(src[2][0]) * rdx + C(src[3][0]) * ldx) * dy);
           dst[1] = BILINEAR_ROUND(
               (C(src[0][1]) * rdx + C(src[1][1]) * ldx) * rdy +
               (C(src[2][1]) * rdx + C(src[3][1]) * ldx) * dy);
           dst[2] = BILINEAR_ROUND(
               (C(src[0][2]) * rdx + C(src[1][2]) * ldx) * rdy +
               (C(src[2][2]) * rdx + C(src[3][2]) * ldx) * dy);
           dst[3] = BILINEAR_ROUND_ALPHA(
               ((src[0][3]) * rdx + (src[1][3]) * ldx) * rdy +
               ((src[2][3]) * rdx + (src[3][3]) * ldx) * dy);
           );
       break;
     case 5:
       IMPL(5,
           dst[0] = BILINEAR_ROUND(
               (C(src[0][0]) * rdx + C(src[1][0]) * ldx) * rdy +
               (C(src[2][0]) * rdx + C(src[3][0]) * ldx) * dy);
           dst[1] = BILINEAR_ROUND(
               (C(src[0][1]) * rdx + C(src[1][1]) * ldx) * rdy +
               (C(src[2][1]) * rdx + C(src[3][1]) * ldx) * dy);
           dst[2] = BILINEAR_ROUND(
               (C(src[0][2]) * rdx + C(src[1][2]) * ldx) * rdy +
               (C(src[2][2]) * rdx + C(src[3][2]) * ldx) * dy);
           dst[3] = BILINEAR_ROUND(
               ((src[0][3]) * rdx + (src[1][3]) * ldx) * rdy +
               ((src[2][3]) * rdx + (src[3][3]) * ldx) * dy);
           dst[4] = BILINEAR_ROUND_ALPHA(
               ((src[0][4]) * rdx + (src[1][4]) * ldx) * rdy +
               ((src[2][4]) * rdx + (src[3][4]) * ldx) * dy);
           );
       break;
   }
#undef IMPL
}
#undef C
#undef BILINEAR_ROUND
#undef BILINEAR_ROUND_ALPHA

static void
gegl_downscale_2x2_u8_nl (const Babl *format,
                          gint        src_width,
                          gint        src_height,
                          guchar     *src_data,
                          gint        src_rowstride,
                          guchar     *dst_data,
                          gint        dst_rowstride)
{
  gint y;
  gint bpp = babl_format_get_bytes_per_pixel (format);
  gint diag = src_rowstride + bpp;
  const gint components = bpp / sizeof(uint8_t);

  if (!src_data || !dst_data)
    return;

#define CASE(case_val, ...) \
case case_val: \
  for (y = 0; y < src_height / 2; y++) \
    { \
      gint    x; \
      guchar *src = src_data + src_rowstride * y * 2; \
      guchar *dst = dst_data + dst_rowstride * y; \
  for (x = 0; x < src_width / 2; x++)\
    {\
      uint8_t * aa = ((uint8_t *)(src));\
      uint8_t * ab = ((uint8_t *)(src + bpp));\
      uint8_t * ba = ((uint8_t *)(src + src_rowstride));\
      uint8_t * bb = ((uint8_t *)(src + diag));\
      __VA_ARGS__;\
      dst += bpp;\
      src += bpp * 2;\
    }\
   }\
break;\

      switch (components)
      {
        CASE(1,
            ((uint8_t *)dst)[0] = gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[0]] +
                                                  gegl_lut_u8_to_u16[ab[0]] +
                                                  gegl_lut_u8_to_u16[ba[0]] +
                                                  gegl_lut_u8_to_u16[bb[0]])>>2 ];);
        CASE(2,
            ((uint8_t *)dst)[0] = gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[0]] +
                                                  gegl_lut_u8_to_u16[ab[0]] +
                                                  gegl_lut_u8_to_u16[ba[0]] +
                                                  gegl_lut_u8_to_u16[bb[0]])>>2 ];
            ((uint8_t *)dst)[1] = gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[1]] +
                                                  gegl_lut_u8_to_u16[ab[1]] +
                                                  gegl_lut_u8_to_u16[ba[1]] +
                                                  gegl_lut_u8_to_u16[bb[1]])>>2 ];);
        CASE(3,
            ((uint8_t *)dst)[0] = gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[0]] +
                                                  gegl_lut_u8_to_u16[ab[0]] +
                                                  gegl_lut_u8_to_u16[ba[0]] +
                                                  gegl_lut_u8_to_u16[bb[0]])>>2 ];
            ((uint8_t *)dst)[1] = gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[1]] +
                                                  gegl_lut_u8_to_u16[ab[1]] +
                                                  gegl_lut_u8_to_u16[ba[1]] +
                                                  gegl_lut_u8_to_u16[bb[1]])>>2 ];
            ((uint8_t *)dst)[2] = gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[2]] +
                                                  gegl_lut_u8_to_u16[ab[2]] +
                                                  gegl_lut_u8_to_u16[ba[2]] +
                                                  gegl_lut_u8_to_u16[bb[2]])>>2 ];);
        CASE(4,
            ((uint8_t *)dst)[0] = gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[0]] +
                                                  gegl_lut_u8_to_u16[ab[0]] +
                                                  gegl_lut_u8_to_u16[ba[0]] +
                                                  gegl_lut_u8_to_u16[bb[0]])>>2 ];
            ((uint8_t *)dst)[1] = gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[1]] +
                                                  gegl_lut_u8_to_u16[ab[1]] +
                                                  gegl_lut_u8_to_u16[ba[1]] +
                                                  gegl_lut_u8_to_u16[bb[1]])>>2 ];
            ((uint8_t *)dst)[2] = gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[2]] +
                                                  gegl_lut_u8_to_u16[ab[2]] +
                                                  gegl_lut_u8_to_u16[ba[2]] +
                                                  gegl_lut_u8_to_u16[bb[2]])>>2 ];
            ((uint8_t *)dst)[3] = gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[3]] +
                                                  gegl_lut_u8_to_u16[ab[3]] +
                                                  gegl_lut_u8_to_u16[ba[3]] +
                                                  gegl_lut_u8_to_u16[bb[3]])>>2 ];);
        default:
         CASE(0,
            for (gint i = 0; i < components; i++)
              ((uint8_t *)dst)[i] =
                gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[i]] +
                                gegl_lut_u8_to_u16[ab[i]] +
                                gegl_lut_u8_to_u16[ba[i]] +
                                gegl_lut_u8_to_u16[bb[i]])>>2 ];);
      }
}

static void
gegl_downscale_2x2_u8_nl_alpha (const Babl *format,
                                gint        src_width,
                                gint        src_height,
                                guchar     *src_data,
                                gint        src_rowstride,
                                guchar     *dst_data,
                                gint        dst_rowstride)
{
  gint y;
  gint bpp = babl_format_get_bytes_per_pixel (format);
  gint diag = src_rowstride + bpp;
  const gint components = bpp / sizeof(uint8_t);

  if (!src_data || !dst_data)
    return;

      switch (components)
      {
        CASE(2,
            ((uint8_t *)dst)[0] = gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[0]] +
                                                  gegl_lut_u8_to_u16[ab[0]] +
                                                  gegl_lut_u8_to_u16[ba[0]] +
                                                  gegl_lut_u8_to_u16[bb[0]])>>2 ];
            ((uint8_t *)dst)[1] = (aa[1] + ab[1] + ba[1] + bb[1])>>2;);
        CASE(4,
            ((uint8_t *)dst)[0] = gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[0]] +
                                                  gegl_lut_u8_to_u16[ab[0]] +
                                                  gegl_lut_u8_to_u16[ba[0]] +
                                                  gegl_lut_u8_to_u16[bb[0]])>>2 ];
            ((uint8_t *)dst)[1] = gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[1]] +
                                                  gegl_lut_u8_to_u16[ab[1]] +
                                                  gegl_lut_u8_to_u16[ba[1]] +
                                                  gegl_lut_u8_to_u16[bb[1]])>>2 ];
            ((uint8_t *)dst)[2] = gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[2]] +
                                                  gegl_lut_u8_to_u16[ab[2]] +
                                                  gegl_lut_u8_to_u16[ba[2]] +
                                                  gegl_lut_u8_to_u16[bb[2]])>>2 ];
            ((uint8_t *)dst)[3] = (aa[3] + ab[3] + ba[3] + bb[3])>>2;);
        default:
         CASE(0,
            for (gint i = 0; i < components - 1; i++)
              ((uint8_t *)dst)[i] =
                gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[i]] +
                                gegl_lut_u8_to_u16[ab[i]] +
                                gegl_lut_u8_to_u16[ba[i]] +
                                gegl_lut_u8_to_u16[bb[i]])>>2 ];
            ((uint8_t *)dst)[components-1] = (aa[components-1] + ab[components-1] + ba[components-1] + bb[components-1])>>2;);
      }
#undef CASE
}

static void
gegl_downscale_2x2_u8_rgba (const Babl *format,
                            gint        src_width,
                            gint        src_height,
                            guchar     *src_data,
                            gint        src_rowstride,
                            guchar     *dst_data,
                            gint        dst_rowstride)
{
  gint y;
  const gint bpp = 4;
  gint diag = src_rowstride + bpp;

  if (!src_data || !dst_data)
    return;

  for (y = 0; y < src_height / 2; y++)
    {
      gint    x;
      guchar *src = src_data + src_rowstride * y * 2;
      guchar *dst = dst_data + dst_rowstride * y;

      uint8_t * aa = ((uint8_t *)(src));
      uint8_t * ab = ((uint8_t *)(src + bpp));
      uint8_t * ba = ((uint8_t *)(src + src_rowstride));
      uint8_t * bb = ((uint8_t *)(src + diag));

      for (x = 0; x < src_width / 2; x++)
        {

          ((uint8_t *)dst)[0] = gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[0]] +
                                                gegl_lut_u8_to_u16[ab[0]] +
                                                gegl_lut_u8_to_u16[ba[0]] +
                                                gegl_lut_u8_to_u16[bb[0]])>>2 ];
          ((uint8_t *)dst)[1] = gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[1]] +
                                                gegl_lut_u8_to_u16[ab[1]] +
                                                gegl_lut_u8_to_u16[ba[1]] +
                                                gegl_lut_u8_to_u16[bb[1]])>>2 ];
          ((uint8_t *)dst)[2] = gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[2]] +
                                                gegl_lut_u8_to_u16[ab[2]] +
                                                gegl_lut_u8_to_u16[ba[2]] +
                                                gegl_lut_u8_to_u16[bb[2]])>>2 ];
          ((uint8_t *)dst)[3] = (aa[3] + ab[3] + ba[3] + bb[3])>>2;

          dst += bpp;
          aa += bpp * 2;
          ab += bpp * 2;
          ba += bpp * 2;
          bb += bpp * 2;
        }
  }
}

static void
gegl_downscale_2x2_u8_rgb (const Babl *format,
                           gint        src_width,
                           gint        src_height,
                           guchar     *src_data,
                           gint        src_rowstride,
                           guchar     *dst_data,
                           gint        dst_rowstride)
{
  gint y;
  const gint bpp = 3;
  gint diag = src_rowstride + bpp;

  if (!src_data || !dst_data)
    return;

  for (y = 0; y < src_height / 2; y++)
    {
      gint    x;
      guchar *src = src_data + src_rowstride * y * 2;
      guchar *dst = dst_data + dst_rowstride * y;

      uint8_t * aa = ((uint8_t *)(src));
      uint8_t * ab = ((uint8_t *)(src + bpp));
      uint8_t * ba = ((uint8_t *)(src + src_rowstride));
      uint8_t * bb = ((uint8_t *)(src + diag));

      for (x = 0; x < src_width / 2; x++)
        {

          ((uint8_t *)dst)[0] = gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[0]] +
                                                gegl_lut_u8_to_u16[ab[0]] +
                                                gegl_lut_u8_to_u16[ba[0]] +
                                                gegl_lut_u8_to_u16[bb[0]])>>2 ];
          ((uint8_t *)dst)[1] = gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[1]] +
                                                gegl_lut_u8_to_u16[ab[1]] +
                                                gegl_lut_u8_to_u16[ba[1]] +
                                                gegl_lut_u8_to_u16[bb[1]])>>2 ];
          ((uint8_t *)dst)[2] = gegl_lut_u16_to_u8[ (gegl_lut_u8_to_u16[aa[2]] +
                                                gegl_lut_u8_to_u16[ab[2]] +
                                                gegl_lut_u8_to_u16[ba[2]] +
                                                gegl_lut_u8_to_u16[bb[2]])>>2 ];
          dst += bpp;
          aa += bpp * 2;
          ab += bpp * 2;
          ba += bpp * 2;
          bb += bpp * 2;
        }
  }
}


/* FIXME:  disable the _alpha() variants for now, so that we use the same gamma
 * curve for the alpha component as we do for the color components.  this is
 * necessary to avoid producing over-saturated pixels when using a format with
 * premultiplied alpha (note that, either way, using such a format with these
 * functions produces wrong results in general, when the data is not fully
 * opaque.)  most notably, when using cairo-ARGB32 (which we do in GIMP's
 * display code), this avoids potential overflow during compositing in cairo.
 *
 * see issue #104.
 *
 * we use the comma operator to avoid "defined but not used" warnings.
 */
#define gegl_boxfilter_u8_nl_alpha     ((void) gegl_boxfilter_u8_nl_alpha,     \
                                        gegl_boxfilter_u8_nl)
#define gegl_bilinear_u8_nl_alpha      ((void) gegl_bilinear_u8_nl_alpha,      \
                                        gegl_bilinear_u8_nl)
#define gegl_downscale_2x2_u8_nl_alpha ((void) gegl_downscale_2x2_u8_nl_alpha, \
                                        gegl_downscale_2x2_u8_nl)

void
GEGL_SIMD_SUFFIX(gegl_downscale_2x2_nearest) (const Babl *format,
                                              gint        src_width,
                                              gint        src_height,
                                              guchar     *src_data,
                                              gint        src_rowstride,
                                              guchar     *dst_data,
                                              gint        dst_rowstride)
{
  gint bpp = babl_format_get_bytes_per_pixel (format);
  gint y;

  for (y = 0; y < src_height / 2; y++)
    {
      gint x;
      guchar *src = src_data;
      guchar *dst = dst_data;

      for (x = 0; x < src_width / 2; x++)
        {
          memcpy (dst, src, bpp);
          dst += bpp;
          src += bpp * 2;
        }

      dst_data += dst_rowstride;
      src_data += src_rowstride * 2;
    }
}


void
GEGL_SIMD_SUFFIX(gegl_resample_nearest) (guchar              *dst,
                                         const guchar        *src,
                                         const GeglRectangle *dst_rect,
                                         const GeglRectangle *src_rect,
                                         const gint           src_stride,
                                         const gdouble        scale,
                                         const gint           bpp,
                                         const gint           dst_stride)
{
  gint jj[dst_rect->width];
  gint x, y;
  for (x = 0; x < dst_rect->width; x++)
  {
    const gfloat sx = (dst_rect->x + .5 + x) / scale - src_rect->x;
    jj[x] = int_floorf (sx ) * bpp;
  }

#define IMPL(...) do{ \
  for (y = 0; y < dst_rect->height; y++)\
    {\
      const gfloat sy = (dst_rect->y + .5 + y) / scale - src_rect->y;\
      const gint   ii = int_floorf (sy);\
      gint *ijj = &jj[0];\
      guchar *d = &dst[y*dst_stride];\
      const guchar *s = &src[ii * src_stride];\
      for (x = 0; x < dst_rect->width; x++)\
        {\
          __VA_ARGS__;\
          d += bpp; \
        }\
    }\
  }while(0)

  switch(bpp)
  {
    case 1:IMPL(
             d[0] = s[*(ijj++)];
           );
    break;
    case 2:IMPL(
             uint16_t* d16 = (void*) d;
             const uint16_t* s16 = (void*) &s[*(ijj++)];
             d16[0] = s16[0];
           );
    break;
    case 3:IMPL(
             d[0] = s[*ijj];
             d[1] = s[*ijj + 1];
             d[2] = s[*(ijj++) + 2];
           );
    break;
    case 5:IMPL(
             uint32_t* d32 = (void*) d;
             const uint32_t* s32 = (void*) &s[*(ijj++)];
             d32[0] = s32[0];
             d[4] = s[4];
           );
    break;
    case 4:IMPL(
             uint32_t* d32 = (void*) d;
             const uint32_t* s32 = (void*) &s[*(ijj++)];
             d32[0] = s32[0];
           );
    break;
    case 6:IMPL(
             uint32_t* d32 = (void*) d;
             const uint32_t* s32 = (void*) &s[*(ijj++)];
             d32[0] = s32[0];
             d[4] = s[4];
             d[5] = s[5];
           );
    break;
    case 8:IMPL(
             uint64_t* d64 = (void*) d;
             const uint64_t* s64 = (void*) &s[*(ijj++)];
             d64[0] = s64[0];
           );
    break;
    case 12:IMPL(
             uint32_t* d32 = (void*) d;
             const uint32_t* s32 = (void*) &s[*(ijj++)];
             d32[0] = s32[0];
             d32[1] = s32[1];
             d32[2] = s32[2];
           );
    break;
    case 16:IMPL(
             uint64_t* d64 = (void*) d;
             const uint64_t* s64 = (void*) &s[*(ijj++)];
             d64[0] = s64[0];
             d64[1] = s64[1];
           );
    break;
    default:
         IMPL(
          memcpy (&d[0], &s[*(ijj++)], bpp);
           );
    break;
  }
#undef IMPL
}

#define BOXFILTER_FUNCNAME   gegl_resample_boxfilter_double
#define BOXFILTER_TYPE       gdouble
#define BOXFILTER_TEMP_TYPE  gdouble
#define BOXFILTER_ROUND(val) (val)
#include "gegl-algorithms-boxfilter.inc"
#undef BOXFILTER_FUNCNAME
#undef BOXFILTER_TYPE
#undef BOXFILTER_TEMP_TYPE
#undef BOXFILTER_ROUND

#define BOXFILTER_FUNCNAME   gegl_resample_boxfilter_float
#define BOXFILTER_TYPE       gfloat
#define BOXFILTER_TEMP_TYPE  gfloat
#define BOXFILTER_ROUND(val) (val)
#include "gegl-algorithms-boxfilter.inc"
#undef BOXFILTER_FUNCNAME
#undef BOXFILTER_TYPE
#undef BOXFILTER_TEMP_TYPE
#undef BOXFILTER_ROUND

#define BOXFILTER_FUNCNAME   gegl_resample_boxfilter_u8
#define BOXFILTER_TYPE       guchar
#define BOXFILTER_TEMP_TYPE  guchar
#define BOXFILTER_ROUND(val) ((int)((val)+0.5f))
#include "gegl-algorithms-boxfilter.inc"
#undef BOXFILTER_FUNCNAME
#undef BOXFILTER_TYPE
#undef BOXFILTER_TEMP_TYPE
#undef BOXFILTER_ROUND

#define BOXFILTER_FUNCNAME   gegl_resample_boxfilter_u16
#define BOXFILTER_TYPE       guint16
#define BOXFILTER_TEMP_TYPE  guint16
#define BOXFILTER_ROUND(val) ((int)((val)+0.5f))
#include "gegl-algorithms-boxfilter.inc"
#undef BOXFILTER_FUNCNAME
#undef BOXFILTER_TYPE
#undef BOXFILTER_TEMP_TYPE
#undef BOXFILTER_ROUND

static inline guint32 _gegl_trunc_u32(guint64 value)
{
  if ((guint64) value > G_MAXUINT32)
    return G_MAXUINT32;
  return value;
}

#define BOXFILTER_FUNCNAME   gegl_resample_boxfilter_u32
#define BOXFILTER_TYPE       guint32
#define BOXFILTER_TEMP_TYPE  guint64
#define BOXFILTER_ROUND(val) _gegl_trunc_u32((val)+0.5f)
#include "gegl-algorithms-boxfilter.inc"
#undef BOXFILTER_FUNCNAME
#undef BOXFILTER_TEMP_TYPE
#undef BOXFILTER_TYPE
#undef BOXFILTER_ROUND

#define DOWNSCALE_FUNCNAME gegl_downscale_2x2_double
#define DOWNSCALE_TYPE     gdouble
#define DOWNSCALE_SUM      gdouble
#define DOWNSCALE_DIVISOR  4.0
#include "gegl-algorithms-2x2-downscale.inc"
#undef DOWNSCALE_FUNCNAME
#undef DOWNSCALE_TYPE
#undef DOWNSCALE_SUM
#undef DOWNSCALE_DIVISOR

#define DOWNSCALE_FUNCNAME gegl_downscale_2x2_float
#define DOWNSCALE_TYPE     gfloat
#define DOWNSCALE_SUM      gfloat
#define DOWNSCALE_DIVISOR  4.0f
#include "gegl-algorithms-2x2-downscale.inc"
#undef DOWNSCALE_FUNCNAME
#undef DOWNSCALE_TYPE
#undef DOWNSCALE_SUM
#undef DOWNSCALE_DIVISOR

#define DOWNSCALE_FUNCNAME gegl_downscale_2x2_u32
#define DOWNSCALE_TYPE     guint32
#define DOWNSCALE_SUM      guint64
#define DOWNSCALE_DIVISOR  4
#include "gegl-algorithms-2x2-downscale.inc"
#undef DOWNSCALE_FUNCNAME
#undef DOWNSCALE_TYPE
#undef DOWNSCALE_SUM
#undef DOWNSCALE_DIVISOR

#define DOWNSCALE_FUNCNAME gegl_downscale_2x2_u16
#define DOWNSCALE_TYPE     guint16
#define DOWNSCALE_SUM      guint
#define DOWNSCALE_DIVISOR  4
#include "gegl-algorithms-2x2-downscale.inc"
#undef DOWNSCALE_FUNCNAME
#undef DOWNSCALE_TYPE
#undef DOWNSCALE_SUM
#undef DOWNSCALE_DIVISOR

#define DOWNSCALE_FUNCNAME gegl_downscale_2x2_u8
#define DOWNSCALE_TYPE     guint8
#define DOWNSCALE_SUM      guint
#define DOWNSCALE_DIVISOR  4
#include "gegl-algorithms-2x2-downscale.inc"
#undef DOWNSCALE_FUNCNAME
#undef DOWNSCALE_TYPE
#undef DOWNSCALE_SUM
#undef DOWNSCALE_DIVISOR


#define BILINEAR_FUNCNAME   gegl_resample_bilinear_double
#define BILINEAR_TYPE       gdouble
#define BILINEAR_ROUND(val) (val)
#include "gegl-algorithms-bilinear.inc"
#undef BILINEAR_FUNCNAME
#undef BILINEAR_TYPE
#undef BILINEAR_ROUND

#define BILINEAR_FUNCNAME   gegl_resample_bilinear_float
#define BILINEAR_TYPE       gfloat
#define BILINEAR_ROUND(val) (val)
#include "gegl-algorithms-bilinear.inc"
#undef BILINEAR_FUNCNAME
#undef BILINEAR_TYPE
#undef BILINEAR_ROUND

#define BILINEAR_FUNCNAME   gegl_resample_bilinear_u8
#define BILINEAR_TYPE       guchar
#define BILINEAR_ROUND(val) ((int)((val)+0.5f))
#include "gegl-algorithms-bilinear.inc"
#undef BILINEAR_FUNCNAME
#undef BILINEAR_TYPE
#undef BILINEAR_ROUND

#define BILINEAR_FUNCNAME   gegl_resample_bilinear_u16
#define BILINEAR_TYPE       guint16
#define BILINEAR_ROUND(val) ((int)((val)+0.5f))
#include "gegl-algorithms-bilinear.inc"
#undef BILINEAR_FUNCNAME
#undef BILINEAR_TYPE
#undef BILINEAR_ROUND

#define BILINEAR_FUNCNAME   gegl_resample_bilinear_u32
#define BILINEAR_TYPE       guint32
#define BILINEAR_ROUND(val) _gegl_trunc_u32((val)+0.5f)
#include "gegl-algorithms-bilinear.inc"
#undef BILINEAR_FUNCNAME
#undef BILINEAR_TYPE
#undef BILINEAR_ROUND

static void
gegl_downscale_2x2_generic2 (const Babl *format,
                             gint        src_width,
                             gint        src_height,
                             guchar     *src_data,
                             gint        src_rowstride,
                             guchar     *dst_data,
                             gint        dst_rowstride)
{
  const Babl *tmp_format = babl_format_with_space ("RGBA float", format);
  const Babl *from_fish  = babl_fish (format, tmp_format);
  const Babl *to_fish    = babl_fish (tmp_format, format);
  const gint tmp_bpp     = 4 * 4;
  gint dst_width         = src_width / 2;
  gint dst_height        = src_height / 2;
  gint in_tmp_rowstride  = src_width * tmp_bpp;
  gint out_tmp_rowstride = dst_width * tmp_bpp;
  gint do_free = 0;

  void *in_tmp;
  void *out_tmp;

  if (src_height * in_tmp_rowstride + dst_height * out_tmp_rowstride < GEGL_ALLOCA_THRESHOLD)
  {
    in_tmp = align_16 (alloca (src_height * in_tmp_rowstride + 16));
    out_tmp = align_16 (alloca (dst_height * out_tmp_rowstride + 16));
  }
  else
  {
    in_tmp = gegl_scratch_alloc (src_height * in_tmp_rowstride);
    out_tmp = gegl_scratch_alloc (dst_height * out_tmp_rowstride);
    do_free = 1;
  }

  babl_process_rows (from_fish,
                     src_data, src_rowstride,
                     in_tmp,   in_tmp_rowstride,
                     src_width, src_height);
  gegl_downscale_2x2_float (tmp_format, src_width, src_height,
                            in_tmp,  in_tmp_rowstride,
                            out_tmp, out_tmp_rowstride);
  babl_process_rows (to_fish,
                     out_tmp,   out_tmp_rowstride,
                     dst_data,  dst_rowstride,
                     dst_width, dst_height);

  if (do_free)
   {
     gegl_scratch_free (out_tmp);
     gegl_scratch_free (in_tmp);
   }
}

GeglDownscale2x2Fun GEGL_SIMD_SUFFIX(gegl_downscale_2x2_get_fun) (const Babl *format)
{
  const Babl *comp_type = babl_format_get_type (format, 0);
  const Babl *model     = babl_format_get_model (format);
  BablModelFlag model_flags = babl_get_model_flags (model);
  
  if ((model_flags & BABL_MODEL_FLAG_LINEAR)||
      (model_flags & BABL_MODEL_FLAG_CMYK))
  {
    if (comp_type == gegl_babl_float())
    {
      return gegl_downscale_2x2_float;
    }
    else if (comp_type == gegl_babl_u8())
    {
      return gegl_downscale_2x2_u8;
    }
    else if (comp_type == gegl_babl_u16())
    {
      return gegl_downscale_2x2_u16;
    }
    else if (comp_type == gegl_babl_u32())
    {
      return gegl_downscale_2x2_u32;
    }
    else if (comp_type == gegl_babl_double())
    {
      return gegl_downscale_2x2_double;
    }
  }
  if (comp_type == gegl_babl_u8())
  {
    if (format == gegl_babl_rgba_u8())
      return gegl_downscale_2x2_u8_rgba;
    if (format == gegl_babl_rgb_u8())
      return gegl_downscale_2x2_u8_rgb;

    if (babl_format_has_alpha (format))
      return gegl_downscale_2x2_u8_nl_alpha;
    else
      return gegl_downscale_2x2_u8_nl;
  }
  return gegl_downscale_2x2_generic2;
}


static void
gegl_resample_boxfilter_generic2 (guchar       *dest_buf,
                                  const guchar *source_buf,
                                  const GeglRectangle *dst_rect,
                                  const GeglRectangle *src_rect,
                                  gint  s_rowstride,
                                  gdouble scale,
                                  const Babl *format,
                                  gint bpp,
                                  gint d_rowstride)
{
  const Babl *tmp_format = babl_format_with_space ("RGBA float", format);
  const Babl *from_fish  = babl_fish (format, tmp_format);
  const Babl *to_fish    = babl_fish (tmp_format, format);

  const gint tmp_bpp     = 4 * 4;
  gint in_tmp_rowstride  = src_rect->width * tmp_bpp;
  gint out_tmp_rowstride = dst_rect->width * tmp_bpp;
  gint do_free = 0;

  guchar *in_tmp, *out_tmp;

  if (src_rect->height * in_tmp_rowstride + dst_rect->height * out_tmp_rowstride < GEGL_ALLOCA_THRESHOLD)
  {
    in_tmp = align_16 (alloca (src_rect->height * in_tmp_rowstride + 16));
    out_tmp = align_16 (alloca (dst_rect->height * out_tmp_rowstride + 16));
  }
  else
  {
    in_tmp  = gegl_scratch_alloc (src_rect->height * in_tmp_rowstride);
    out_tmp = gegl_scratch_alloc (dst_rect->height * out_tmp_rowstride);
    do_free = 1;
  }

  babl_process_rows (from_fish,
                     source_buf, s_rowstride,
                     in_tmp, in_tmp_rowstride,
                     src_rect->width, src_rect->height);

  gegl_resample_boxfilter_float (out_tmp, in_tmp, dst_rect, src_rect,
                                 in_tmp_rowstride, scale, tmp_format, tmp_bpp, out_tmp_rowstride);

  babl_process_rows (to_fish,
                     out_tmp,  out_tmp_rowstride,
                     dest_buf, d_rowstride,
                     dst_rect->width, dst_rect->height);

  if (do_free)
    {
      gegl_scratch_free (out_tmp);
      gegl_scratch_free (in_tmp);
    }
}


void
GEGL_SIMD_SUFFIX(gegl_resample_boxfilter) (guchar              *dest_buf,
                                           const guchar        *source_buf,
                                           const GeglRectangle *dst_rect,
                                           const GeglRectangle *src_rect,
                                           gint                 s_rowstride,
                                           gdouble              scale,
                                           const Babl          *format,
                                           gint                 d_rowstride)
{
  void (*func) (guchar *dest_buf,
                const guchar        *source_buf,
                const GeglRectangle *dst_rect,
                const GeglRectangle *src_rect,
                gint                 s_rowstride,
                gdouble              scale,
                const Babl          *format,
                gint                 bpp,
                gint                 d_rowstride) = gegl_resample_boxfilter_generic2;


  const Babl *model     = babl_format_get_model (format);
  const Babl *comp_type  = babl_format_get_type (format, 0);
  const gint bpp = babl_format_get_bytes_per_pixel (format);
  BablModelFlag model_flags = babl_get_model_flags (model);

  if (func)
  {};

  if ((model_flags & BABL_MODEL_FLAG_LINEAR)||
      (model_flags & BABL_MODEL_FLAG_CMYK))
  {

    if (comp_type == gegl_babl_float())
      func = gegl_resample_boxfilter_float;
    else if (comp_type == gegl_babl_u8())
      func = gegl_resample_boxfilter_u8;
    else if (comp_type == gegl_babl_u16())
      func = gegl_resample_boxfilter_u16;
    else if (comp_type == gegl_babl_u32())
      func = gegl_resample_boxfilter_u32;
    else if (comp_type == gegl_babl_double())
      func = gegl_resample_boxfilter_double;
    }
  else
    {
      if (comp_type == gegl_babl_u8())
        {
          if (babl_format_has_alpha (format))
            func = gegl_boxfilter_u8_nl_alpha;
          else
            func = gegl_boxfilter_u8_nl;
        }
    }

  func (dest_buf, source_buf, dst_rect, src_rect,
        s_rowstride, scale, format, bpp, d_rowstride);

}


static void
gegl_resample_bilinear_generic2 (guchar              *dest_buf,
                                 const guchar        *source_buf,
                                 const GeglRectangle *dst_rect,
                                 const GeglRectangle *src_rect,
                                 gint                 s_rowstride,
                                 gdouble              scale,
                                 const Babl          *format,
                                 gint                 d_rowstride)
{
  const Babl *tmp_format = babl_format_with_space ("RGBA float", format);
  const Babl *from_fish  = babl_fish (format, tmp_format);
  const Babl *to_fish    = babl_fish (tmp_format, format);

  const gint tmp_bpp     = 4 * 4;
  gint in_tmp_rowstride  = src_rect->width * tmp_bpp;
  gint out_tmp_rowstride = dst_rect->width * tmp_bpp;

  gint do_free = 0;

  guchar *in_tmp, *out_tmp;

  if (src_rect->height * in_tmp_rowstride + dst_rect->height * out_tmp_rowstride < GEGL_ALLOCA_THRESHOLD)
  {
    in_tmp = align_16 (alloca (src_rect->height * in_tmp_rowstride + 16));
    out_tmp = align_16 (alloca (dst_rect->height * out_tmp_rowstride + 16));
  }
  else
  {
    in_tmp  = gegl_scratch_alloc (src_rect->height * in_tmp_rowstride);
    out_tmp = gegl_scratch_alloc (dst_rect->height * out_tmp_rowstride);
    do_free = 1;
  }

  babl_process_rows (from_fish,
                     source_buf, s_rowstride,
                     in_tmp, in_tmp_rowstride,
                     src_rect->width, src_rect->height);

  gegl_resample_bilinear_float (out_tmp, in_tmp, dst_rect, src_rect,
                                in_tmp_rowstride, scale, tmp_bpp, out_tmp_rowstride);

  babl_process_rows (to_fish,
                     out_tmp,  out_tmp_rowstride,
                     dest_buf, d_rowstride,
                     dst_rect->width, dst_rect->height);

  if (do_free)
    {
      gegl_scratch_free (out_tmp);
      gegl_scratch_free (in_tmp);
    }
}

void
GEGL_SIMD_SUFFIX(gegl_resample_bilinear) (guchar              *dest_buf,
                                          const guchar        *source_buf,
                                          const GeglRectangle *dst_rect,
                                          const GeglRectangle *src_rect,
                                          gint                 s_rowstride,
                                          gdouble              scale,
                                          const Babl          *format,
                                          gint                 d_rowstride)
{
  const Babl *model     = babl_format_get_model (format);
  const Babl *comp_type  = babl_format_get_type (format, 0);
  BablModelFlag model_flags = babl_get_model_flags (model);

  if ((model_flags & BABL_MODEL_FLAG_LINEAR)||
      (model_flags & BABL_MODEL_FLAG_CMYK))
  {
    const gint bpp = babl_format_get_bytes_per_pixel (format);

    if (comp_type == gegl_babl_float ())
      gegl_resample_bilinear_float (dest_buf, source_buf, dst_rect, src_rect,
                                    s_rowstride, scale, bpp, d_rowstride);
    else if (comp_type == gegl_babl_u8 ())
      gegl_resample_bilinear_u8 (dest_buf, source_buf, dst_rect, src_rect,
                                 s_rowstride, scale, bpp, d_rowstride);
    else if (comp_type == gegl_babl_u16 ())
      gegl_resample_bilinear_u16 (dest_buf, source_buf, dst_rect, src_rect,
                                  s_rowstride, scale, bpp, d_rowstride);
    else if (comp_type == gegl_babl_u32 ())
      gegl_resample_bilinear_u32 (dest_buf, source_buf, dst_rect, src_rect,
                                  s_rowstride, scale, bpp, d_rowstride);
    else if (comp_type == gegl_babl_double ())
      gegl_resample_bilinear_double (dest_buf, source_buf, dst_rect, src_rect,
                                     s_rowstride, scale, bpp, d_rowstride);
    else
      gegl_resample_bilinear_generic2 (dest_buf, source_buf, dst_rect, src_rect,
                                       s_rowstride, scale, format, d_rowstride);
    }
  else
    {
      if (comp_type == gegl_babl_u8 ())
        {
          const gint bpp = babl_format_get_bytes_per_pixel (format);
          if (babl_format_has_alpha (format))
            gegl_bilinear_u8_nl_alpha (dest_buf, source_buf, dst_rect, src_rect,
                                       s_rowstride, scale, bpp, d_rowstride);
          else
            gegl_bilinear_u8_nl (dest_buf, source_buf, dst_rect, src_rect,
                                 s_rowstride, scale, bpp, d_rowstride);
        }
      else
        {
          gegl_resample_bilinear_generic2 (dest_buf, source_buf,
                                           dst_rect, src_rect,
                                           s_rowstride, scale, format,
                                           d_rowstride);
        }
    }
}

