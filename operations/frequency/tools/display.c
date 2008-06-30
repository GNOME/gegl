/* This file is a part of GEGL
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
 * Copyright 2008 Zhang Junbo  <zhangjb@svn.gnome.org>
 */

#ifndef FFT_HALF
#define FFT_HALF(n) (gint)((n)/2+1)
#define ELEM_ID_MATRIX(x, y, c) ((y)*(c)+(x)) 
#define ELEM_ID_HALF_MATRIX(x, y, c) ((y)*(FFT_HALF(c))+(x))
#endif

#include "gegl.h"
#include <fftw3.h>
#include <math.h>

static gint fft_complex_get_half_id(gint x, gint y, gint width, gint height);
gboolean shift_dft(gdouble *buf, gint width, gint height);
void get_min_max(gdouble *buf, gdouble *min, gdouble *max, glong samples);
gboolean zoomshow(gdouble *buf, glong samples);
gboolean fre2img(fftw_complex *src_buf, gdouble *des_buf, gint width, gint height);

static gint
fft_complex_get_half_id(gint x, gint y, gint width, gint height)
{
  if (x >= FFT_HALF(x))
    {
      if (y == 0)
        return ELEM_ID_HALF_MATRIX(width-x, y, width);
      else
        return ELEM_ID_HALF_MATRIX(width-x, height-y, width);
    }
  else
    return 0;
}

gboolean
shift_dft(gdouble *buf, gint width, gint height)
{
  gint cx, cy;
  gint add_x, add_y;
  gint x, y;
  gdouble tmp_buf[width*height];

  cx = width/2;
  cy = height/2;

  for (x=0; x<width; x++)
    {
      for (y=0; y<height; y++)
        {
          add_x = (x<(cx+width%2)) ? cx : (0-cx-width%2);
          add_y = (y<(cy+height%2)) ? cy : (0-cy-height%2);
          tmp_buf[ELEM_ID_MATRIX(x+add_x, y+add_y, width)] = buf[ELEM_ID_MATRIX(x, y, width)];
        }
    }
  for (x=0; x<width*height; x++)
    {
      buf[x] = tmp_buf[x];
    }

  return TRUE;
}

void
get_min_max(gdouble *buf, gdouble *min, gdouble *max, glong samples)
{
  gfloat tmin = 9000000.0;
  gfloat tmax =-9000000.0;

  glong i;
  for (i=0; i<samples; i++)
    {
      gfloat val = buf[i];
      if (val<tmin)
        tmin=val;
      if (val>tmax)
        tmax=val;
    }
  if (min)
    *min = tmin;
  if (max)
    *max = tmax;
}

gboolean
zoomshow(gdouble *buf, glong samples)
{
  glong i;
  gdouble min, max;

  for (i=0; i<samples; i++)
    {
      *(buf+i) = log(*(buf+i)+1);
    }
  get_min_max(buf, &min, &max, samples);
  for (i=0; i<samples; i++)
    {
      *(buf+i) = (*(buf+i))/max;
    }
  return TRUE;
}

gboolean
fre2img(fftw_complex *src_buf, gdouble *dst_buf, gint width, gint height)
{
  gint x, y;
  glong samples = width*height;
  gdouble *dst_real_buf;
  gdouble *dst_imag_buf;

  dst_real_buf = g_new0(gdouble, width*height);
  dst_imag_buf = g_new0(gdouble, width*height);

  for (y=0; y<height; y++)
    {
      for (x=0; x<width; x++)
        {
          if (x<FFT_HALF(width))
            {
              dst_real_buf[ELEM_ID_MATRIX(x, y, width)] =
                src_buf[ELEM_ID_HALF_MATRIX(x, y, width)][0];
              dst_imag_buf[ELEM_ID_MATRIX(x, y, width)] =
                src_buf[ELEM_ID_HALF_MATRIX(x, y, width)][1];
            }
          else
            {
              dst_real_buf[ELEM_ID_MATRIX(x, y, width)] =
                src_buf[fft_complex_get_half_id(x, y, width, height)][0];
              dst_imag_buf[ELEM_ID_MATRIX(x, y, width)] =
                0-src_buf[fft_complex_get_half_id(x, y, width, height)][1];
            }
        }
    }
  for (x=0; x<width*height; x++)
    {
      dst_buf[x] =
        sqrt(dst_real_buf[x]*dst_real_buf[x]+dst_imag_buf[x]*dst_imag_buf[x]);
    }
  
  zoomshow(dst_buf, samples);
  shift_dft(dst_buf, width, height);

  g_free(dst_real_buf);
  g_free(dst_imag_buf);
  return TRUE;

}
