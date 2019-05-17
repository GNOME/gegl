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
 * Copyright 2018 Øyvind Kolås
 */

#ifndef __GEGL_BUFFER_FORMATS_H__
#define __GEGL_BUFFER_FORMATS_H__


//G_BEGIN_DECLS

#include <babl/babl.h>
#include <glib-object.h>

/* the code in babl for looking up models, formats and types is quick -
   but when formats end up being used as consts for comparisons in the core of
   GEGL, it is good to have even better caching, hence this way of generating
   and accessing per compilation unit caches of formats.
 */

#define GEGL_CACHED_BABL(klass, typ, name)  \
static inline const Babl *gegl_babl_##typ (void) { static const Babl *type = NULL; if (!type) type = babl_##klass (name); return type; }

GEGL_CACHED_BABL(type, half, "half")
GEGL_CACHED_BABL(type, float, "float")
GEGL_CACHED_BABL(type, u8, "u8")
GEGL_CACHED_BABL(type, u16, "u16")
GEGL_CACHED_BABL(type, u32, "u32")
GEGL_CACHED_BABL(type, double, "double")

GEGL_CACHED_BABL(model, rgb_linear, "RGB")
GEGL_CACHED_BABL(model, rgba_linear, "RGBA")
GEGL_CACHED_BABL(model, rgbA_linear, "RaGaBaA")
GEGL_CACHED_BABL(model, y_linear, "Y")
GEGL_CACHED_BABL(model, ya_linear, "YA")
GEGL_CACHED_BABL(model, yA_linear, "YaA")

GEGL_CACHED_BABL(format, rgba_float, "R'G'B'A float")
GEGL_CACHED_BABL(format, rgba_u8, "R'G'B'A u8")
GEGL_CACHED_BABL(format, rgb_u8, "R'G'B' u8")
GEGL_CACHED_BABL(format, rgbA_float, "R'aG'aB'aA float")
GEGL_CACHED_BABL(format, rgba_linear_float, "RGBA float")
GEGL_CACHED_BABL(format, rgba_linear_u16, "RGBA u16")
GEGL_CACHED_BABL(format, rgbA_linear_float, "RaGaBaA float")
GEGL_CACHED_BABL(format, ya_float, "Y'A float")
GEGL_CACHED_BABL(format, yA_float, "Y'aA float")
GEGL_CACHED_BABL(format, ya_linear_float, "Y float")
GEGL_CACHED_BABL(format, yA_linear_float, "YaA float")


#ifdef G_OS_WIN32
  /* one use 16kb of stack before an exception triggered warning on win32 */
  #define GEGL_ALLOCA_THRESHOLD  8192
#else
/* on linux/OSX 0.5mb is reasonable, the stack size of new threads is 2MB */
  #define GEGL_ALLOCA_THRESHOLD  (1024*1024/2)
#endif

static inline int int_floorf (float x)
{
  int i = (int)x;       /* truncate */
  return i - ( i > x ); /* convert trunc to floor */
}

static inline int int_ceilf (float x)
{
  return -int_floorf(-(x));
}

//G_END_DECLS

#endif /* __GEGL_BUFFER_FORMATS_H__ */
