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
 * Copyright 2012 Victor Oliveira (victormatheus@gmail.com)
 */

#ifndef __GEGL_CL_TYPES_H__
#define __GEGL_CL_TYPES_H__

#include <glib-object.h>

#include "gegl-cl-version.h"
#include "CL/opencl.h"

G_BEGIN_DECLS

struct _GeglClTexture
{
  cl_mem           data;
  cl_image_format  format;
  gint             width;
  gint             height;
};

typedef struct _GeglClTexture GeglClTexture;

G_END_DECLS

#endif /* __GEGL_CL_TYPES_H__ */
