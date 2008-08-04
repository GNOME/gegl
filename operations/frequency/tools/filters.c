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
gboolean getH_highpass_gaussian(gdouble *, gdouble *, gint, gint, gint);
gboolean getH_bandpass_gaussian(gdouble *, gdouble *, gint, gint, gint, gdouble);

gboolean
freq_multiply(gdouble *Xr, gdouble *Xi, gdouble *Hr,
              gdouble *Hi, gint width, gint height)
{
  gint x, y;
  gdouble Yr,Yi;
  gint index,h_index;
  gint max_x = FFT_HALF(width);

  for(x=0;x<max_x;x++)
    {
      for(y=0;y<height/2;y++)
        {
          index = y*max_x+x;
          h_index = (height/2-y-1)*max_x+width/2-x-1;

          Yr= Xr[index]*Hr[h_index] - Xi[index]*Hi[h_index];
          Yi= Xi[index]*Hr[h_index] + Xr[index]*Hi[h_index];
          Xr[index] = Yr;
          Xi[index] = Yi;
        }

      for(y=height/2;y<height;y++)
        {
          index = (y*max_x)+x;
          h_index = (3*height/2-y-1)*max_x+width/2-x-1;

          Yr= Xr[index]*Hr[h_index] - Xi[index]*Hi[h_index];
          Yi= Xi[index]*Hr[h_index] + Xr[index]*Hi[h_index];
          Xr[index] = Yr;
          Xi[index] = Yi;
        }
    }
  return TRUE;
}

gboolean
getH_lowpass_gaussian(gdouble *Hr, gdouble *Hi, gint width, gint height,
                      gint cutoff)
{
  gint x, y;
  gint max_x = FFT_HALF(width);
  gint index;
  gint cutoff_cutoff_double = cutoff*cutoff*2;

  for (y=0; y<height; y++){
    for (x=0; x<max_x; x++)
      {
        index = ELEM_ID_HALF_MATRIX(x, y, width);
        Hi[index] = 0;
        Hr[index] =  exp( -((gdouble)(x+1-width/2)*(x+1-width/2)
                                   +(y+1-height/2)*(y+1-height/2))/(cutoff_cutoff_double) );
      }
  }

  return TRUE;
}

gboolean
getH_highpass_gaussian(gdouble *Hr, gdouble *Hi, gint width, gint height,
                      gint cutoff)
{
  gint x, y;
  gint max_x = FFT_HALF(width);
  gint index;
  gint cutoff_cutoff_double = cutoff*cutoff*2;

  for (y=0; y<height; y++){
    for (x=0; x<max_x; x++)
      {
        index = ELEM_ID_HALF_MATRIX(x, y, width);
        Hi[index] = 0;
        Hr[index] = 1 - exp( -((gdouble)(x+1-width/2)*(x+1-width/2)
                                   +(y+1-height/2)*(y+1-height/2))/cutoff_cutoff_double);
      }
  }

  return TRUE;
}

gboolean
getH_bandpass_gaussian(gdouble *Hr, gdouble *Hi, gint width, gint height,
                       gint cutoff, gdouble bandwidth )
{
  gint x, y;
  gint max_x = FFT_HALF(width);
  gint index;
  gdouble dist;
  gint cutoff_cutoff = cutoff*cutoff;

  for (y=0; y<height; y++){
    for (x=0; x<max_x; x++)
      {
        index = ELEM_ID_HALF_MATRIX(x, y, width);
        dist = sqrt((x+1-width/2)*(x+1-width/2)+(y+1-height/2)*(y+1-height/2));
        Hi[index] = 0;
        Hr[index] = exp(-pow((dist*dist-cutoff_cutoff)/dist/bandwidth,2)/2);
      }
  }

  return TRUE;
}
