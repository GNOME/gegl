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

gboolean get_rgba_component(gdouble* src_buf, gdouble* comp_buf, gint place,
                            glong samples);
gboolean set_rgba_component(gdouble* comp_buf, gdouble* dst_buf, gint place,
                            glong samples);
gboolean get_freq_component(gdouble* src_buf, gdouble* comp_buf, gint place,
                            glong samples);
gboolean set_freq_component(gdouble* comp_buf, gdouble* dst_buf, gint place,
                            glong samples);

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
  src_buf += place*2;
  while (samples--)
    {
      *(comp_buf++) = *src_buf;
      *(comp_buf++) = *(src_buf+1);
      src_buf += 8;
    }
  return TRUE;
}
  
gboolean
set_freq_component(gdouble* comp_buf, gdouble* dst_buf, gint place,
                   glong samples)
{
  dst_buf += place*2;
  while (samples--)
    {
      *dst_buf = *(comp_buf++);
      *(dst_buf+1) = *(comp_buf++);
      dst_buf += 8;
    }
  return TRUE;
}
