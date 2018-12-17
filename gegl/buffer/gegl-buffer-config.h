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
 * Copyright 2006, 2007 Øyvind Kolås <pippin@gimp.org>
 */

#ifndef __GEGL_BUFFER_CONFIG_H__
#define __GEGL_BUFFER_CONFIG_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GEGL_BUFFER_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_BUFFER_CONFIG, GeglBufferConfigClass))
#define GEGL_IS_BUFFER_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_BUFFER_CONFIG))
#define GEGL_BUFFER_CONFIG_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_BUFFER_CONFIG, GeglBufferConfigClass))
GType gegl_buffer_config_get_type (void) G_GNUC_CONST;
#define GEGL_TYPE_BUFFER_CONFIG     (gegl_buffer_config_get_type ())
#define GEGL_BUFFER_CONFIG(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_BUFFER_CONFIG, GeglBufferConfig))
#define GEGL_IS_BUFFER_CONFIG(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_BUFFER_CONFIG))


typedef struct _GeglBufferConfig GeglBufferConfig;



typedef struct _GeglBufferConfigClass GeglBufferConfigClass;

struct _GeglBufferConfig
{
  GObject  parent_instance;

  gchar   *swap;
  gchar   *swap_compression;
  guint64  tile_cache_size;
  gint     tile_width;
  gint     tile_height;
  gint     queue_size;
};

struct _GeglBufferConfigClass
{
  GObjectClass parent_class;
};

GeglBufferConfig *gegl_buffer_config (void);

G_END_DECLS

#endif
