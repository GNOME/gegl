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
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2012 Ville Sokk <ville.sokk@gmail.com>
 */

#ifndef __GEGL_TILE_BACKEND_FILE_H__
#define __GEGL_TILE_BACKEND_FILE_H__

#include <glib.h>
#include <gio/gio.h>
#include "gegl-tile-backend.h"
#include "gegl-buffer-index.h"
#include "gegl-aio-file.h"

G_BEGIN_DECLS

#define GEGL_TYPE_TILE_BACKEND_FILE            (gegl_tile_backend_file_get_type ())
#define GEGL_TILE_BACKEND_FILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_BACKEND_FILE, GeglTileBackendFile))
#define GEGL_TILE_BACKEND_FILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_BACKEND_FILE, GeglTileBackendFileClass))
#define GEGL_IS_TILE_BACKEND_FILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_BACKEND_FILE))
#define GEGL_IS_TILE_BACKEND_FILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_BACKEND_FILE))
#define GEGL_TILE_BACKEND_FILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_BACKEND_FILE, GeglTileBackendFileClass))


typedef struct _GeglTileBackendFile      GeglTileBackendFile;
typedef struct _GeglTileBackendFileClass GeglTileBackendFileClass;

struct _GeglTileBackendFileClass
{
  GeglTileBackendClass parent_class;
};

struct _GeglTileBackendFile
{
  GeglTileBackend    parent_instance;
  gchar             *path;
  GeglAIOFile       *file;
  GHashTable        *index;
  GList            **gap_list;
  GeglBufferHeader   header;
  GFile             *gfile;
  GFileMonitor      *monitor;
};

GType gegl_tile_backend_file_get_type (void) G_GNUC_CONST;

gboolean gegl_tile_backend_file_try_lock (GeglTileBackendFile *self);

gboolean gegl_tile_backend_file_unlock (GeglTileBackendFile *self);

G_END_DECLS

#endif
