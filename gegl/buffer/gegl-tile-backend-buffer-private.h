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
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2018 Ell
 */

#ifndef __GEGL_TILE_BACKEND_BUFFER_H__
#define __GEGL_TILE_BACKEND_BUFFER_H__

#include <glib.h>
#include "gegl-tile-backend.h"

G_BEGIN_DECLS

#define GEGL_TYPE_TILE_BACKEND_BUFFER            (gegl_tile_backend_buffer_get_type ())
#define GEGL_TILE_BACKEND_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_BACKEND_BUFFER, GeglTileBackendBuffer))
#define GEGL_TILE_BACKEND_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_BACKEND_BUFFER, GeglTileBackendBufferClass))
#define GEGL_IS_TILE_BACKEND_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_BACKEND_BUFFER))
#define GEGL_IS_TILE_BACKEND_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_BACKEND_BUFFER))
#define GEGL_TILE_BACKEND_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_BACKEND_BUFFER, GeglTileBackendBufferClass))


typedef struct _GeglTileBackendBuffer      GeglTileBackendBuffer;
typedef struct _GeglTileBackendBufferClass GeglTileBackendBufferClass;

struct _GeglTileBackendBuffer
{
  GeglTileBackend  parent_instance;

  GeglBuffer      *buffer;
};

struct _GeglTileBackendBufferClass
{
  GeglTileBackendClass parent_class;
};

GType             gegl_tile_backend_buffer_get_type (void) G_GNUC_CONST;

GeglTileBackend * gegl_tile_backend_buffer_new      (GeglBuffer *buffer);

G_END_DECLS

#endif
