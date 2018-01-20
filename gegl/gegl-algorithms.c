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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006,2007,2015,2018 Øyvind Kolås <pippin@gimp.org>
 *           2013 Daniel Sabo
 */

#include "config.h"

#include <string.h>
#include <stdint.h>
#include <glib-object.h>

#include <babl/babl.h>

#include "gegl-types.h"
#include "gegl-types-internal.h"
#include "gegl-utils.h"
#include "gegl-algorithms.h"

#include <math.h>

void gegl_downscale_2x2 (const Babl *format,
                         gint        src_width,
                         gint        src_height,
                         guchar     *src_data,
                         gint        src_rowstride,
                         guchar     *dst_data,
                         gint        dst_rowstride)
{
  gegl_downscale_2x2_get_fun (format)(format, src_width, src_height,
                                              src_data, src_rowstride,
                                              dst_data, dst_rowstride);;
}

#include <stdio.h>

static void
gegl_downscale_2x2_generic (const Babl *format,
                            gint        src_width,
                            gint        src_height,
                            guchar     *src_data,
                            gint        src_rowstride,
                            guchar     *dst_data,
                            gint        dst_rowstride)
{
  const Babl *tmp_format = gegl_babl_rgba_linear_float ();
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
    in_tmp = alloca (src_height * in_tmp_rowstride);
    out_tmp = alloca (dst_height * out_tmp_rowstride);
  }
  else
  {
    in_tmp = gegl_malloc (src_height * in_tmp_rowstride);
    out_tmp = gegl_malloc (dst_height * out_tmp_rowstride);
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
     gegl_free (in_tmp);
     gegl_free (out_tmp);
   }
}

static uint16_t lut_u8_to_u16[256];
static uint8_t lut_u16_to_u8[65537];

void _gegl_init_u8_lut (void);
void _gegl_init_u8_lut (void)
{
  static int lut_inited = 0;
  uint8_t u8_ramp[256];
  uint16_t u16_ramp[65537];
  int i;

  if (lut_inited)
    return;
  for (i = 0; i < 256; i++) u8_ramp[i]=i;
  for (i = 0; i < 65536; i++) u16_ramp[i]=i;
  babl_process (babl_fish (babl_format ("Y' u8"), babl_format("Y u16")),
                &u8_ramp[0], &lut_u8_to_u16[0],
                256);

  /* workaround for bug, doing this conversion sample by sample */
  for (i = 0; i < 65536; i++)
    babl_process (babl_fish (babl_format ("Y u16"), babl_format("Y' u8")),
                  &u16_ramp[i], &lut_u16_to_u8[i],
                  1);

  lut_inited = 1;
}

static inline void
u8_to_u16_rows (int            components,
                const uint8_t *source_buf,
                int            source_stride,
                uint16_t      *dest_buf,
                int            dest_stride,
                int            n,
                int            rows)
{
  n *= components;

  while (rows--)
   {
      const uint8_t *src = source_buf;
      uint16_t      *dest = dest_buf;
      int i = n;
      while (i--)
        *(dest++) = lut_u8_to_u16[*(src++)];
      source_buf += source_stride;
      dest_buf += (dest_stride / 2);
   }
}

static inline void
u16_to_u8_rows (int             components,
                const uint16_t *source_buf,
                int             source_stride,
                uint8_t        *dest_buf,
                int             dest_stride,
                int             n,
                int             rows)
{
  n *= components;
  while (rows--)
   {
      int i = n;
      const uint16_t *src = source_buf;
      uint8_t      *dest = dest_buf;
      while (i--)
        *(dest++) = lut_u16_to_u8[*(src++)];

      source_buf += (source_stride / 2);
      dest_buf += dest_stride;
   }
}

static inline int int_floorf (float x)
{
  int i = (int)x; /* truncate */
  return i - ( i > x ); /* convert trunc to floor */
}


static void
gegl_boxfilter_u8_nl (guchar              *dest_buf,
                      const guchar        *source_buf,
                      const GeglRectangle *dst_rect,
                      const GeglRectangle *src_rect,
                      const gint           s_rowstride,
                      const gdouble        scale,
                      const gint           bpp,
                      const gint           d_rowstride)
{
  const uint8_t *src[9];
  gint  components = bpp / sizeof(uint8_t);

  gfloat left_weight[dst_rect->width];
  gfloat center_weight[dst_rect->width];
  gfloat right_weight[dst_rect->width];

  gint   jj[dst_rect->width];

  for (gint x = 0; x < dst_rect->width; x++)
  {
    gfloat sx  = (dst_rect->x + x + .5) / scale - src_rect->x;
    jj[x]  = int_floorf (sx);

    left_weight[x]   = .5 - scale * (sx - jj[x]);
    left_weight[x]   = MAX (0.0, left_weight[x]);
    right_weight[x]  = .5 - scale * ((jj[x] + 1) - sx);
    right_weight[x]  = MAX (0.0, right_weight[x]);
    center_weight[x] = 1. - left_weight[x] - right_weight[x];

    jj[x] *= components;
  }

  for (gint y = 0; y < dst_rect->height; y++)
    {
      gfloat top_weight, middle_weight, bottom_weight;
      const gfloat sy = (dst_rect->y + y + .5) / scale - src_rect->y;
      const gint     ii = int_floorf (sy);
      uint8_t             *dst = (uint8_t*)(dest_buf + y * d_rowstride);
      const guchar  *src_base = source_buf + ii * s_rowstride;

      top_weight    = .5 - scale * (sy - ii);
      top_weight    = MAX (0., top_weight);
      bottom_weight = .5 - scale * ((ii + 1 ) - sy);
      bottom_weight = MAX (0., bottom_weight);
      middle_weight = 1. - top_weight - bottom_weight;

      switch (components)
      {
        case 4:
          for (gint x = 0; x < dst_rect->width; x++)
            {
            src[4] = (const uint8_t*)src_base + jj[x];
            src[1] = (const uint8_t*)(src_base - s_rowstride) + jj[x];
            src[7] = (const uint8_t*)(src_base + s_rowstride) + jj[x];
            src[2] = src[1] + 4;
            src[5] = src[4] + 4;
            src[8] = src[7] + 4;
            src[0] = src[1] - 4;
            src[3] = src[4] - 4;
            src[6] = src[7] - 4;

            if (src[0][3] == 0 &&  /* XXX: it would be even better to not call this at all for the abyss...  */
                src[1][3] == 0 &&
                src[2][3] == 0 &&
                src[3][3] == 0 &&
                src[4][3] == 0 &&
                src[5][3] == 0 &&
                src[6][3] == 0 &&
                src[7][3] == 0)
            {
              (*(uint32_t*)(dst)) = 0;
            }
            else
            {
              const gfloat l = left_weight[x];
              const gfloat c = center_weight[x];
              const gfloat r = right_weight[x];

              const gfloat t = top_weight;
              const gfloat m = middle_weight;
              const gfloat b = bottom_weight;

#define BOXFILTER_ROUND(val) lut_u16_to_u8[((int)((val)+0.5))]
#define C(val)               lut_u8_to_u16[(val)]
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
        case 3:
          for (gint x = 0; x < dst_rect->width; x++)
            {
            src[4] = (const uint8_t*)src_base + jj[x];
            src[1] = (const uint8_t*)(src_base - s_rowstride) + jj[x];
            src[7] = (const uint8_t*)(src_base + s_rowstride) + jj[x];
            src[2] = src[1] + 3;
            src[5] = src[4] + 3;
            src[8] = src[7] + 3;
            src[0] = src[1] - 3;
            src[3] = src[4] - 3;
            src[6] = src[7] - 3;
            {
              const gfloat l = left_weight[x];
              const gfloat c = center_weight[x];
              const gfloat r = right_weight[x];

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
            }
            dst += 3;
            }
          break;
        case 2:
          for (gint x = 0; x < dst_rect->width; x++)
            {
            src[4] = (const uint8_t*)src_base + jj[x];
            src[1] = (const uint8_t*)(src_base - s_rowstride) + jj[x];
            src[7] = (const uint8_t*)(src_base + s_rowstride) + jj[x];
            src[2] = src[1] + 2;
            src[5] = src[4] + 2;
            src[8] = src[7] + 2;
            src[0] = src[1] - 2;
            src[3] = src[4] - 2;
            src[6] = src[7] - 2;
            {
              const gfloat l = left_weight[x];
              const gfloat c = center_weight[x];
              const gfloat r = right_weight[x];

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
            }
            dst += 2;
            }
          break;
        case 1:
          for (gint x = 0; x < dst_rect->width; x++)
            {
            src[4] = (const uint8_t*)src_base + jj[x];
            src[1] = (const uint8_t*)(src_base - s_rowstride) + jj[x];
            src[7] = (const uint8_t*)(src_base + s_rowstride) + jj[x];
            src[2] = src[1] + 1;
            src[5] = src[4] + 1;
            src[8] = src[7] + 1;
            src[0] = src[1] - 1;
            src[3] = src[4] - 1;
            src[6] = src[7] - 1;
            {
              const gfloat l = left_weight[x];
              const gfloat c = center_weight[x];
              const gfloat r = right_weight[x];

              const gfloat t = top_weight;
              const gfloat m = middle_weight;
              const gfloat b = bottom_weight;

              dst[0] = BOXFILTER_ROUND(
                (C(src[0][0]) * t + C(src[3][0]) * m + C(src[6][0]) * b) * l +
                (C(src[1][0]) * t + C(src[4][0]) * m + C(src[7][0]) * b) * c +
                (C(src[2][0]) * t + C(src[5][0]) * m + C(src[8][0]) * b) * r);
            }
            dst += 1;
            }
          break;
        default:
          for (gint x = 0; x < dst_rect->width; x++)
          {
            src[4] = (const uint8_t*)src_base + jj[x];
            src[1] = (const uint8_t*)(src_base - s_rowstride) + jj[x];
            src[7] = (const uint8_t*)(src_base + s_rowstride) + jj[x];
            src[2] = src[1] + components;
            src[5] = src[4] + components;
            src[8] = src[7] + components;
            src[0] = src[1] - components;
            src[3] = src[4] - components;
            src[6] = src[7] - components;
            {
              const gfloat l = left_weight[x];
              const gfloat c = center_weight[x];
              const gfloat r = right_weight[x];

              const gfloat t = top_weight;
              const gfloat m = middle_weight;
              const gfloat b = bottom_weight;

              for (gint i = 0; i < components; ++i)
                {
                  dst[i] = BOXFILTER_ROUND(
                  (C(src[0][i]) * t + C(src[3][i]) * m + C(src[6][i]) * b) * l +
                  (C(src[1][i]) * t + C(src[4][i]) * m + C(src[7][i]) * b) * c +
                  (C(src[2][i]) * t + C(src[5][i]) * m + C(src[8][i]) * b) * r);
                }
              }
            dst += components;
        }
        break;
      }
    }
}
#undef BOXFILTER_ROUND
#undef C

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

  for (y = 0; y < src_height / 2; y++)
    {
      gint    x;
      guchar *src = src_data + src_rowstride * y * 2;
      guchar *dst = dst_data + dst_rowstride * y;

      switch (components)
      {
          case 1:
          for (x = 0; x < src_width / 2; x++)
          {
            uint8_t * aa = ((uint8_t *)(src));
            uint8_t * ab = ((uint8_t *)(src + bpp));
            uint8_t * ba = ((uint8_t *)(src + src_rowstride));
            uint8_t * bb = ((uint8_t *)(src + diag));

            ((uint8_t *)dst)[0] = lut_u16_to_u8[ (lut_u8_to_u16[aa[0]] +
                                                  lut_u8_to_u16[ab[0]] +
                                                  lut_u8_to_u16[ba[0]] +
                                                  lut_u8_to_u16[bb[0]])>>2 ];

            dst += bpp;
            src += bpp * 2;
          }
          break;
        case 2:
        for (x = 0; x < src_width / 2; x++)
          {
            uint8_t * aa = ((uint8_t *)(src));
            uint8_t * ab = ((uint8_t *)(src + bpp));
            uint8_t * ba = ((uint8_t *)(src + src_rowstride));
            uint8_t * bb = ((uint8_t *)(src + diag));

            ((uint8_t *)dst)[0] = lut_u16_to_u8[ (lut_u8_to_u16[aa[0]] +
                                                  lut_u8_to_u16[ab[0]] +
                                                  lut_u8_to_u16[ba[0]] +
                                                  lut_u8_to_u16[bb[0]])>>2 ];
            ((uint8_t *)dst)[1] = lut_u16_to_u8[ (lut_u8_to_u16[aa[1]] +
                                                  lut_u8_to_u16[ab[1]] +
                                                  lut_u8_to_u16[ba[1]] +
                                                  lut_u8_to_u16[bb[1]])>>2 ];

            dst += bpp;
            src += bpp * 2;
          }
          break;
        case 3:
        for (x = 0; x < src_width / 2; x++)
          {
            uint8_t * aa = ((uint8_t *)(src));
            uint8_t * ab = ((uint8_t *)(src + bpp));
            uint8_t * ba = ((uint8_t *)(src + src_rowstride));
            uint8_t * bb = ((uint8_t *)(src + diag));

            ((uint8_t *)dst)[0] = lut_u16_to_u8[ (lut_u8_to_u16[aa[0]] +
                                                  lut_u8_to_u16[ab[0]] +
                                                  lut_u8_to_u16[ba[0]] +
                                                  lut_u8_to_u16[bb[0]])>>2 ];
            ((uint8_t *)dst)[1] = lut_u16_to_u8[ (lut_u8_to_u16[aa[1]] +
                                                  lut_u8_to_u16[ab[1]] +
                                                  lut_u8_to_u16[ba[1]] +
                                                  lut_u8_to_u16[bb[1]])>>2 ];
            ((uint8_t *)dst)[2] = lut_u16_to_u8[ (lut_u8_to_u16[aa[2]] +
                                                  lut_u8_to_u16[ab[2]] +
                                                  lut_u8_to_u16[ba[2]] +
                                                  lut_u8_to_u16[bb[2]])>>2 ];

            dst += bpp;
            src += bpp * 2;
          }
          break;
        case 4:
        for (x = 0; x < src_width / 2; x++)
          {
            uint8_t * aa = ((uint8_t *)(src));
            uint8_t * ab = ((uint8_t *)(src + bpp));
            uint8_t * ba = ((uint8_t *)(src + src_rowstride));
            uint8_t * bb = ((uint8_t *)(src + diag));

            ((uint8_t *)dst)[0] = lut_u16_to_u8[ (lut_u8_to_u16[aa[0]] +
                                                  lut_u8_to_u16[ab[0]] +
                                                  lut_u8_to_u16[ba[0]] +
                                                  lut_u8_to_u16[bb[0]])>>2 ];
            ((uint8_t *)dst)[1] = lut_u16_to_u8[ (lut_u8_to_u16[aa[1]] +
                                                  lut_u8_to_u16[ab[1]] +
                                                  lut_u8_to_u16[ba[1]] +
                                                  lut_u8_to_u16[bb[1]])>>2 ];
            ((uint8_t *)dst)[2] = lut_u16_to_u8[ (lut_u8_to_u16[aa[2]] +
                                                  lut_u8_to_u16[ab[2]] +
                                                  lut_u8_to_u16[ba[2]] +
                                                  lut_u8_to_u16[bb[2]])>>2 ];
            ((uint8_t *)dst)[3] = lut_u16_to_u8[ (lut_u8_to_u16[aa[3]] +
                                                  lut_u8_to_u16[ab[3]] +
                                                  lut_u8_to_u16[ba[3]] +
                                                  lut_u8_to_u16[bb[3]])>>2 ];

            dst += bpp;
            src += bpp * 2;
          }
          break;
        default:
        for (x = 0; x < src_width / 2; x++)
          {
            gint i;
            uint8_t * aa = ((uint8_t *)(src));
            uint8_t * ab = ((uint8_t *)(src + bpp));
            uint8_t * ba = ((uint8_t *)(src + src_rowstride));
            uint8_t * bb = ((uint8_t *)(src + diag));

            for (i = 0; i < components; i++)
              ((uint8_t *)dst)[i] =
                lut_u16_to_u8[ (lut_u8_to_u16[aa[i]] +
                                lut_u8_to_u16[ab[i]] +
                                lut_u8_to_u16[ba[i]] +
                                lut_u8_to_u16[bb[i]])>>2 ];
            dst += bpp;
            src += bpp * 2;
          }
      }
  }
}



GeglDownscale2x2Fun gegl_downscale_2x2_get_fun (const Babl *format)
{
  const Babl *comp_type = babl_format_get_type (format, 0);
  const Babl *model     = babl_format_get_model (format);

  if (gegl_babl_model_is_linear (model))
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
    return gegl_downscale_2x2_u8_nl;
  return gegl_downscale_2x2_generic;
}

void
gegl_downscale_2x2_nearest (const Babl *format,
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

static void
gegl_resample_boxfilter_generic_u16 (guchar       *dest_buf,
                                 const guchar *source_buf,
                                 const GeglRectangle *dst_rect,
                                 const GeglRectangle *src_rect,
                                 gint  s_rowstride,
                                 gdouble scale,
                                 const Babl *format,
                                 gint d_rowstride)
{
  gint components = babl_format_get_n_components (format);
  const gint tmp_bpp     = 4 * 2;
  gint in_tmp_rowstride  = src_rect->width * tmp_bpp;
  gint out_tmp_rowstride = dst_rect->width * tmp_bpp;
  gint do_free = 0;

  guchar *in_tmp, *out_tmp;

  if (src_rect->height * in_tmp_rowstride + dst_rect->height * out_tmp_rowstride < GEGL_ALLOCA_THRESHOLD)
  {
    in_tmp = alloca (src_rect->height * in_tmp_rowstride);
    out_tmp = alloca (dst_rect->height * out_tmp_rowstride);
  }
  else
  {
    in_tmp  = gegl_malloc (src_rect->height * in_tmp_rowstride);
    out_tmp = gegl_malloc (dst_rect->height * out_tmp_rowstride);
    do_free = 1;
  }

  u8_to_u16_rows (components,
                  source_buf, s_rowstride,
                  (void*)in_tmp, in_tmp_rowstride,
                  src_rect->width, src_rect->height);

  gegl_resample_boxfilter_u16 (out_tmp, in_tmp, dst_rect, src_rect,
                               in_tmp_rowstride, scale, tmp_bpp, out_tmp_rowstride);

  u16_to_u8_rows (components,
                  (void*)out_tmp,  out_tmp_rowstride,
                  dest_buf, d_rowstride,
                  dst_rect->width, dst_rect->height);

  if (do_free)
    {
      gegl_free (in_tmp);
      gegl_free (out_tmp);
    }
}


static void
gegl_resample_boxfilter_generic (guchar       *dest_buf,
                                 const guchar *source_buf,
                                 const GeglRectangle *dst_rect,
                                 const GeglRectangle *src_rect,
                                 gint  s_rowstride,
                                 gdouble scale,
                                 const Babl *format,
                                 gint d_rowstride)
{
  const Babl *tmp_format = gegl_babl_rgba_linear_float ();
  const Babl *from_fish  = babl_fish (format, tmp_format);
  const Babl *to_fish    = babl_fish (tmp_format, format);

  const gint tmp_bpp     = 4 * 4;
  gint in_tmp_rowstride  = src_rect->width * tmp_bpp;
  gint out_tmp_rowstride = dst_rect->width * tmp_bpp;
  gint do_free = 0;

  guchar *in_tmp, *out_tmp;

  if (src_rect->height * in_tmp_rowstride + dst_rect->height * out_tmp_rowstride < GEGL_ALLOCA_THRESHOLD)
  {
    in_tmp = alloca (src_rect->height * in_tmp_rowstride);
    out_tmp = alloca (dst_rect->height * out_tmp_rowstride);
  }
  else
  {
    in_tmp  = gegl_malloc (src_rect->height * in_tmp_rowstride);
    out_tmp = gegl_malloc (dst_rect->height * out_tmp_rowstride);
    do_free = 1;
  }

  babl_process_rows (from_fish,
                     source_buf, s_rowstride,
                     in_tmp, in_tmp_rowstride,
                     src_rect->width, src_rect->height);

  gegl_resample_boxfilter_float (out_tmp, in_tmp, dst_rect, src_rect,
                                 in_tmp_rowstride, scale, tmp_bpp, out_tmp_rowstride);

  babl_process_rows (to_fish,
                     out_tmp,  out_tmp_rowstride,
                     dest_buf, d_rowstride,
                     dst_rect->width, dst_rect->height);

  if (do_free)
    {
      gegl_free (in_tmp);
      gegl_free (out_tmp);
    }
}

void gegl_resample_boxfilter (guchar              *dest_buf,
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
  const gint bpp = babl_format_get_bytes_per_pixel (format);

  if (gegl_babl_model_is_linear (model))
  {

    if (comp_type == gegl_babl_float())
      gegl_resample_boxfilter_float (dest_buf, source_buf, dst_rect, src_rect,
                                     s_rowstride, scale, bpp, d_rowstride);
    else if (comp_type == gegl_babl_u8())
      gegl_resample_boxfilter_u8 (dest_buf, source_buf, dst_rect, src_rect,
                                  s_rowstride, scale, bpp, d_rowstride);
    else if (comp_type == gegl_babl_u16())
      gegl_resample_boxfilter_u16 (dest_buf, source_buf, dst_rect, src_rect,
                                   s_rowstride, scale, bpp, d_rowstride);
    else if (comp_type == gegl_babl_u32())
      gegl_resample_boxfilter_u32 (dest_buf, source_buf, dst_rect, src_rect,
                                   s_rowstride, scale, bpp, d_rowstride);
    else if (comp_type == gegl_babl_double())
      gegl_resample_boxfilter_double (dest_buf, source_buf, dst_rect, src_rect,
                                      s_rowstride, scale, bpp, d_rowstride);
    else
      gegl_resample_nearest (dest_buf, source_buf, dst_rect, src_rect,
                             s_rowstride, scale, bpp, d_rowstride);
    }
  else
    {
      if (comp_type == gegl_babl_u8())
        gegl_boxfilter_u8_nl (dest_buf, source_buf, dst_rect, src_rect,
                              s_rowstride, scale, bpp, d_rowstride);
#if 0
        gegl_resample_boxfilter_generic_u16 (dest_buf, source_buf, dst_rect, src_rect,
                                         s_rowstride, scale, format, d_rowstride);
#endif
      else
        gegl_resample_boxfilter_generic (dest_buf, source_buf, dst_rect, src_rect,
                                         s_rowstride, scale, format, d_rowstride);
    }
}

static void
gegl_resample_bilinear_generic (guchar              *dest_buf,
                                const guchar        *source_buf,
                                const GeglRectangle *dst_rect,
                                const GeglRectangle *src_rect,
                                gint                 s_rowstride,
                                gdouble              scale,
                                const Babl          *format,
                                gint                 d_rowstride)
{
  const Babl *tmp_format = gegl_babl_rgba_linear_float ();
  const Babl *from_fish  = babl_fish (format, tmp_format);
  const Babl *to_fish    = babl_fish (tmp_format, format);

  const gint tmp_bpp     = 4 * 4;
  gint in_tmp_rowstride  = src_rect->width * tmp_bpp;
  gint out_tmp_rowstride = dst_rect->width * tmp_bpp;

  gint do_free = 0;

  guchar *in_tmp, *out_tmp;

  if (src_rect->height * in_tmp_rowstride + dst_rect->height * out_tmp_rowstride < GEGL_ALLOCA_THRESHOLD)
  {
    in_tmp = alloca (src_rect->height * in_tmp_rowstride);
    out_tmp = alloca (dst_rect->height * out_tmp_rowstride);
  }
  else
  {
    in_tmp  = gegl_malloc (src_rect->height * in_tmp_rowstride);
    out_tmp = gegl_malloc (dst_rect->height * out_tmp_rowstride);
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
      gegl_free (in_tmp);
      gegl_free (out_tmp);
    }
}

void gegl_resample_bilinear (guchar              *dest_buf,
                             const guchar        *source_buf,
                             const GeglRectangle *dst_rect,
                             const GeglRectangle *src_rect,
                             gint                 s_rowstride,
                             gdouble              scale,
                             const Babl          *format,
                             gint                 d_rowstride)
{
  const Babl *model     = babl_format_get_model (format);

  if (gegl_babl_model_is_linear (model))
  {
    const Babl *comp_type  = babl_format_get_type (format, 0);
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
      gegl_resample_nearest (dest_buf, source_buf, dst_rect, src_rect,
                             s_rowstride, scale, bpp, d_rowstride);
    }
  else
    {
      gegl_resample_bilinear_generic (dest_buf, source_buf, dst_rect, src_rect,
                                      s_rowstride, scale, format, d_rowstride);
    }
}

void
gegl_resample_nearest (guchar              *dst,
                       const guchar        *src,
                       const GeglRectangle *dst_rect,
                       const GeglRectangle *src_rect,
                       const gint           src_stride,
                       const gdouble        scale,
                       const gint           bpp,
                       const gint           dst_stride)
{
  int i, j;

  for (i = 0; i < dst_rect->height; i++)
    {
      const gfloat sy = (dst_rect->y + .5 + i) / scale - src_rect->y;
      const gint   ii = int_floorf (sy + GEGL_SCALE_EPSILON);

      for (j = 0; j < dst_rect->width; j++)
        {
          const gfloat sx = (dst_rect->x + .5 + j) / scale - src_rect->x;
          const gint   jj = int_floorf (sx + GEGL_SCALE_EPSILON);

          memcpy (&dst[i * dst_stride + j * bpp],
                  &src[ii * src_stride + jj * bpp],
                  bpp);
        }
    }
}

#define BOXFILTER_FUNCNAME   gegl_resample_boxfilter_double
#define BOXFILTER_TYPE       gdouble
#define BOXFILTER_ROUND(val) (val)
#include "gegl-algorithms-boxfilter.inc"
#undef BOXFILTER_FUNCNAME
#undef BOXFILTER_TYPE
#undef BOXFILTER_ROUND

#define BOXFILTER_FUNCNAME   gegl_resample_boxfilter_float
#define BOXFILTER_TYPE       gfloat
#define BOXFILTER_ROUND(val) (val)
#include "gegl-algorithms-boxfilter.inc"
#undef BOXFILTER_FUNCNAME
#undef BOXFILTER_TYPE
#undef BOXFILTER_ROUND

#define BOXFILTER_FUNCNAME   gegl_resample_boxfilter_u8
#define BOXFILTER_TYPE       guchar
#define BOXFILTER_ROUND(val) ((int)((val)+0.5))
#include "gegl-algorithms-boxfilter.inc"
#undef BOXFILTER_FUNCNAME
#undef BOXFILTER_TYPE
#undef BOXFILTER_ROUND

#define BOXFILTER_FUNCNAME   gegl_resample_boxfilter_u16
#define BOXFILTER_TYPE       guint16
#define BOXFILTER_ROUND(val) ((int)((val)+0.5))
#include "gegl-algorithms-boxfilter.inc"
#undef BOXFILTER_FUNCNAME
#undef BOXFILTER_TYPE
#undef BOXFILTER_ROUND

#define BOXFILTER_FUNCNAME   gegl_resample_boxfilter_u32
#define BOXFILTER_TYPE       guint32
#define BOXFILTER_ROUND(val) ((int)((val)+0.5))
#include "gegl-algorithms-boxfilter.inc"
#undef BOXFILTER_FUNCNAME
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
#define BILINEAR_ROUND(val) ((int)((val)+0.5))
#include "gegl-algorithms-bilinear.inc"
#undef BILINEAR_FUNCNAME
#undef BILINEAR_TYPE
#undef BILINEAR_ROUND

#define BILINEAR_FUNCNAME   gegl_resample_bilinear_u16
#define BILINEAR_TYPE       guint16
#define BILINEAR_ROUND(val) ((int)((val)+0.5))
#include "gegl-algorithms-bilinear.inc"
#undef BILINEAR_FUNCNAME
#undef BILINEAR_TYPE
#undef BILINEAR_ROUND

#define BILINEAR_FUNCNAME   gegl_resample_bilinear_u32
#define BILINEAR_TYPE       guint32
#define BILINEAR_ROUND(val) ((int)((val)+0.5))
#include "gegl-algorithms-bilinear.inc"
#undef BILINEAR_FUNCNAME
#undef BILINEAR_TYPE
#undef BILINEAR_ROUND

