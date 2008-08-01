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

#ifndef COMPONENT_H
#define COMPONENT_H

#ifndef FFT_HALF
#define FFT_HALF(n) (gint)((n)/2+1)
#define ELEM_ID_MATRIX(x, y, c) ((y)*(c)+(x)) 
#define ELEM_ID_HALF_MATRIX(x, y, c) ((y)*(FFT_HALF(c))+(x))
#endif

gboolean get_rgba_component(gdouble *, gdouble *, gint, glong);
gboolean set_rgba_component(gdouble *, gdouble *, gint, glong);
gboolean get_freq_component(gdouble *, gdouble *, gint, glong);
gboolean set_freq_component(gdouble *, gdouble *, gint, glong);
gboolean get_complex_component(gdouble *, gdouble *, gint, glong);
gboolean set_complex_component(gdouble *, gdouble *, gint, glong);


gboolean
get_rgba_component(gdouble* src_buf, gdouble *comp_buf, gint place,
                   glong samples)
{
  src_buf += place;
  while (samples--)
    {
      *(comp_buf++) = *src_buf;
      src_buf += 4;
    }
  return TRUE;
}

gboolean
set_rgba_component(gdouble* comp_buf, gdouble* dst_buf, gint place, glong samples)
{
  dst_buf += place;
  while (samples--)
    {
      *dst_buf = *(comp_buf++);
      dst_buf += 4;
    }
  return TRUE;
}

gboolean
get_freq_component(gdouble* src_buf, gdouble *comp_buf, gint place,
                   glong samples)
{
  src_buf += place;
  while (samples--)
    {
      *(comp_buf++) = *src_buf;
      src_buf += 8;
    }
  return TRUE;
}

gboolean
set_freq_component(gdouble* comp_buf, gdouble* dst_buf, gint place,
                   glong samples)
{
  dst_buf += place;
  while (samples--)
    {
      *dst_buf = *(comp_buf++);
      dst_buf += 8;
    }
  return TRUE;
}

gboolean
set_complex_component(gdouble* comp_buf, gdouble* dst_buf, gint place,
                      glong samples)
{
  dst_buf+=place;
  while(samples--)
    {
      dst_buf[0]=comp_buf[0];
      dst_buf[4]=comp_buf[1];
      comp_buf+=2;
      dst_buf+=8;
    }

  return TRUE;
}

gboolean
get_complex_component(gdouble* src_buf, gdouble* comp_buf, gint place,
                      glong samples)
{
  src_buf+=place;
  while(samples--)
    {
      comp_buf[0]=src_buf[0];
      comp_buf[1]=src_buf[4];
      comp_buf+=2;
      src_buf+=8;
    }

  return TRUE;
}

#endif
