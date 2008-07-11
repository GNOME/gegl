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
#include <math.h>

gboolean freq_multiply(gdouble *, gdouble *, gdouble *, gdouble *, gint, gint);
gboolean getH_lowpass_gaussian(gdouble *, gdouble *, gint, gint, gint);

gboolean
freq_multiply(gdouble *Xr, gdouble *Xi, gdouble *Hr,
              gdouble *Hi, gint width, gint height)
{
  gint x, y;
  gint yc = 0;
  gdouble *Yr;
  gdouble *Yi;

  Yr = g_new0(gdouble, FFT_HALF(width)*height);
  Yi = g_new0(gdouble, FFT_HALF(width)*height);

  for (y=height/2; y<height; y++)
    {
      for (x=0; x<width; x++)
        {
          Yr[ELEM_ID_HALF_MATRIX(x, yc, width)] =
            Xr[ELEM_ID_HALF_MATRIX(x, yc, width)] *
            Hr[ELEM_ID_HALF_MATRIX(x, y, width)] -
            Xi[ELEM_ID_HALF_MATRIX(x, yc, width)] *
            Hi[ELEM_ID_HALF_MATRIX(x, y, width)];
          Yi[ELEM_ID_HALF_MATRIX(x, yc, width)] =
            Xi[ELEM_ID_HALF_MATRIX(x, yc, width)] *
            Hr[ELEM_ID_HALF_MATRIX(x, y, width)] +
            Xr[ELEM_ID_HALF_MATRIX(x, yc, width)] *
            Hi[ELEM_ID_HALF_MATRIX(x, y, width)];
        }
      yc++;
    }
  for (y=0; y<height/2; y++)
    {
      for (x=0; x<width; x++)
        {
          Yr[ELEM_ID_HALF_MATRIX(x, yc, width)] =
            Xr[ELEM_ID_HALF_MATRIX(x, yc, width)] *
            Hr[ELEM_ID_HALF_MATRIX(x, y, width)] -
            Xi[ELEM_ID_HALF_MATRIX(x, yc, width)] *
            Hi[ELEM_ID_HALF_MATRIX(x, y, width)];
          Yi[ELEM_ID_HALF_MATRIX(x, yc, width)] =
            Xi[ELEM_ID_HALF_MATRIX(x, yc, width)] *
            Hr[ELEM_ID_HALF_MATRIX(x, y, width)] +
            Xr[ELEM_ID_HALF_MATRIX(x, yc, width)] *
            Hi[ELEM_ID_HALF_MATRIX(x, y, width)];
        }
      yc++;
    }
  for (y=0; y<height; y++)
    {
      for (x=0; x<width; x++)
        {
          Xr[ELEM_ID_HALF_MATRIX(x, y, width)] =
            Yr[ELEM_ID_HALF_MATRIX(x, y, width)];
          Xr[ELEM_ID_HALF_MATRIX(x, y, width)] =
            Yr[ELEM_ID_HALF_MATRIX(x, y, width)];
        }
    }

  g_free(Yr);
  g_free(Yi);
  return TRUE;
}

gboolean
getH_lowpass_gaussian(gdouble *Hr, gdouble *Hi, gint width, gint height,
                      gint cutoff)
{
  gint x, y;
  for (x=0; x<FFT_HALF(width); x++)
    {
      for (y=0; y<height; y++)
        {
          Hi[ELEM_ID_HALF_MATRIX(x, y, width)] = 0;
          Hr[ELEM_ID_HALF_MATRIX(x, y, width)]
            = exp(0 - (x*x+y*y)/2/(cutoff*cutoff));
        }
    }
  retrun TRUE;
}
