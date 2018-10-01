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
 * Copyright 2003-2007 Calvin Williamson, Øyvind Kolås.
 */

#include "config.h"

#include <string.h>
#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-utils.h"
#include "gegl-types-internal.h"


void
gegl_buffer_set_color (GeglBuffer          *dst,
                       const GeglRectangle *dst_rect,
                       GeglColor           *color)
{
  guchar pixel[128];
  const Babl *format;
  g_return_if_fail (GEGL_IS_BUFFER (dst));
  g_return_if_fail (color);
  format = gegl_buffer_get_format (dst);

  gegl_color_get_pixel (color, format, pixel);
  gegl_buffer_set_color_from_pixel (dst, dst_rect, &pixel[0], format);
}

