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
#include <math.h>
#include <fftw3.h>

gboolean dft(gdouble *src_buf, fftw_complex *dst_buf_pointer, gint width, gint height);
gboolean idft(fftw_complex *src_buf, gdouble *dst_buf, gint width, gint height);
gboolean homo_dft(gdouble *src_buf, fftw_complex *dst_buf_pointer, gint width, gint height);
gboolean homo_idft(fftw_complex *src_buf, gdouble *dst_buf, gint width, gint height);
gboolean encode(gdouble *, gint);
gint decode(gdouble *);

gboolean
dft(gdouble *src_buf, fftw_complex *dst_buf, gint width, gint height)
{
  fftw_plan fftplan;

  fftplan = fftw_plan_dft_r2c_2d(height,
                                 width,
                                 src_buf,
                                 dst_buf,
                                 FFTW_ESTIMATE);
  fftw_execute(fftplan);
  fftw_destroy_plan(fftplan);
  return TRUE;
}

gboolean 
idft(fftw_complex *src_buf, gdouble *dst_buf, gint width, gint height)
{
  glong i;
  fftw_plan fftplan;
  glong samples;

  samples= height*width;
  fftplan = fftw_plan_dft_c2r_2d(height,
                                 width,
                                 src_buf,
                                 dst_buf,
                                  FFTW_ESTIMATE);
  fftw_execute(fftplan);
  for (i=0; i<samples; i++)
    {
      dst_buf[i] /= samples;
    }

  fftw_destroy_plan(fftplan);
  return TRUE;
}

gboolean encode(gdouble *pixelhandle, gint is_even)
{
  glong int_pixel = (glong)(*pixelhandle);

  if (is_even != 1 && is_even != 0)
    return FALSE;
  if (*pixelhandle<0 && *pixelhandle>-1)
    is_even = 5;
  *pixelhandle -= int_pixel;  
  int_pixel = 10 * int_pixel + is_even;
  *pixelhandle += int_pixel;

  return TRUE;
}

  
gint decode(gdouble *pixelhandle)
{
  glong int_pixel = (glong)(*pixelhandle);
  *pixelhandle -= int_pixel;
  if (int_pixel == 5)
    {
      return 1;
    }
  else if (int_pixel%10 == 1)
    {
      *pixelhandle += int_pixel/10;
      return 1;
    }
  else if (int_pixel%10 == -9)
    {
      *pixelhandle += int_pixel/10 - 1;
      return 1;
    }
  else
    {
      *pixelhandle += int_pixel/10;
      return 0;
    }
}

gboolean
homo_dft(gdouble *src_buf, fftw_complex *dst_buf, gint width, gint height)
{
  glong i;
  
  for (i=0; i<width*height; i++)
    {
      src_buf[i] = log(src_buf[i]);
    }
  dft(src_buf, dst_buf, width, height);
  return TRUE;
}

gboolean 
homo_idft(fftw_complex *src_buf, gdouble *dst_buf, gint width, gint height)
{
  glong i;
  
  idft(src_buf, dst_buf, width, height);
  for (i=0; i<width*height; i++)
    {
      dst_buf[i] = exp(dst_buf[i]);
    }
  return TRUE;
}
