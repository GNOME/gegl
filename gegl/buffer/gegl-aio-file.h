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

#ifndef __GEGL_AIO_FILE_H__
#define __GEGL_AIO_FILE_H__

#include <glib.h>

G_BEGIN_DECLS

#define GEGL_TYPE_AIO_FILE            (gegl_aio_file_get_type ())
#define GEGL_AIO_FILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_AIO_FILE, GeglAIOFile))
#define GEGL_AIO_FILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_AIO_FILE, GeglAIOFileClass))
#define GEGL_IS_AIO_FILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_AIO_FILE))
#define GEGL_IS_AIO_FILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_AIO_FILE))
#define GEGL_AIO_FILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_AIO_FILE, GeglAIOFileClass))


typedef struct _GeglAIOFile      GeglAIOFile;
typedef struct _GeglAIOFileClass GeglAIOFileClass;

struct _GeglAIOFileClass
{
  GObjectClass parent_class;
};

struct _GeglAIOFile
{
  GObject     parent_instance;
  GHashTable *index;
  gchar      *path;
  gint        in_fd;
  gint        out_fd;
  guint64     in_offset;
  guint64     out_offset;
  guint64     total;
};

GType gegl_aio_file_get_type (void) G_GNUC_CONST;

void gegl_aio_file_resize (GeglAIOFile *self,
                           guint64      size);

void gegl_aio_file_read (GeglAIOFile *self,
                         guint64      offset,
                         guint        length,
                         guchar      *dest);

void gegl_aio_file_write (GeglAIOFile *self,
                          guint64      offset,
                          guint        length,
                          guchar      *source);

void gegl_aio_file_sync (GeglAIOFile *self);

G_END_DECLS

#endif
