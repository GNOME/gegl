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

#include "config.h"

#include <glib-object.h>

#include "gegl-buffer.h"
#include "gegl-buffer-types.h"

#include "gegl-buffer-backend.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-backend.h"
#include "gegl-tile-backend-buffer.h"
#include "gegl-tile-handler-cache.h"
#include "gegl-tile-storage.h"


enum
{
  PROP_0,
  PROP_BUFFER
};


/*  local function prototypes  */

static void       gegl_tile_backend_buffer_dispose             (GObject               *object);
static void       gegl_tile_backend_buffer_set_property        (GObject               *object,
                                                                guint                  property_id,
                                                                const GValue          *value,
                                                                GParamSpec            *pspec);
static void       gegl_tile_backend_buffer_get_property        (GObject               *object,
                                                                guint                  property_id,
                                                                GValue                *value,
                                                                GParamSpec            *pspec);

static gpointer   gegl_tile_backend_buffer_command             (GeglTileSource        *tile_source,
                                                                GeglTileCommand        command,
                                                                gint                   x,
                                                                gint                   y,
                                                                gint                   z,
                                                                gpointer               data);

static GeglTile * gegl_tile_backend_buffer_get_tile            (GeglTileBackendBuffer *tile_backend_buffer,
                                                                gint                   x,
                                                                gint                   y,
                                                                gint                   z);
static void       gegl_tile_backend_buffer_set_tile            (GeglTileBackendBuffer *tile_backend_buffer,
                                                                GeglTile              *tile,
                                                                gint                   x,
                                                                gint                   y,
                                                                gint                   z);
static gpointer   gegl_tile_backend_buffer_forward_command     (GeglTileBackendBuffer *tile_backend_buffer,
                                                                GeglTileCommand        command,
                                                                gint                   x,
                                                                gint                   y,
                                                                gint                   z,
                                                                gpointer               data,
                                                                gboolean               emit_changed_signal);

static void       gegl_tile_backend_buffer_emit_changed_signal (GeglTileBackendBuffer *tile_backend_buffer,
                                                                gint                   x,
                                                                gint                   y,
                                                                gint                   z);


G_DEFINE_TYPE (GeglTileBackendBuffer, gegl_tile_backend_buffer, GEGL_TYPE_TILE_BACKEND)

#define parent_class gegl_tile_backend_buffer_parent_class


/*  private functions  */


static void
gegl_tile_backend_buffer_class_init (GeglTileBackendBufferClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose      = gegl_tile_backend_buffer_dispose;
  gobject_class->set_property = gegl_tile_backend_buffer_set_property;
  gobject_class->get_property = gegl_tile_backend_buffer_get_property;

  g_object_class_install_property (gobject_class, PROP_BUFFER,
                                   g_param_spec_object ("buffer", NULL, NULL,
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
}

static void
gegl_tile_backend_buffer_init (GeglTileBackendBuffer *tile_backend_buffer)
{
  GeglTileSource *tile_source = GEGL_TILE_SOURCE (tile_backend_buffer);

  tile_source->command = gegl_tile_backend_buffer_command;
}

static void
gegl_tile_backend_buffer_dispose (GObject *object)
{
  GeglTileBackendBuffer *tile_backend_buffer = GEGL_TILE_BACKEND_BUFFER (object);

  g_clear_object (&tile_backend_buffer->buffer);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gegl_tile_backend_buffer_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GeglTileBackendBuffer *tile_backend_buffer = GEGL_TILE_BACKEND_BUFFER (object);

  switch (property_id)
    {
    case PROP_BUFFER:
      if (tile_backend_buffer->buffer)
        g_object_unref (tile_backend_buffer->buffer);

      tile_backend_buffer->buffer = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gegl_tile_backend_buffer_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  GeglTileBackendBuffer *tile_backend_buffer = GEGL_TILE_BACKEND_BUFFER (object);

  switch (property_id)
    {
    case PROP_BUFFER:
      g_value_set_object (value, tile_backend_buffer->buffer);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gpointer
gegl_tile_backend_buffer_command (GeglTileSource  *tile_source,
                                  GeglTileCommand  command,
                                  gint             x,
                                  gint             y,
                                  gint             z,
                                  gpointer         data)
{
  GeglTileBackendBuffer *tile_backend_buffer = GEGL_TILE_BACKEND_BUFFER (tile_source);

  if (G_UNLIKELY(! tile_backend_buffer->buffer))
    return NULL;

  switch (command)
    {
    case GEGL_TILE_GET:
      return gegl_tile_backend_buffer_get_tile (tile_backend_buffer,
                                                x, y, z);

    case GEGL_TILE_SET:
      gegl_tile_backend_buffer_set_tile (tile_backend_buffer,
                                         data, x, y, z);

      return NULL;

    case GEGL_TILE_VOID:
      return gegl_tile_backend_buffer_forward_command (tile_backend_buffer,
                                                       command, x, y, z, data,
                                                       TRUE);

    case GEGL_TILE_EXIST:
      return gegl_tile_backend_buffer_forward_command (tile_backend_buffer,
                                                       command, x, y, z, data,
                                                       FALSE);

    case GEGL_TILE_COPY:
      /* we avoid forwarding the TILE_COPY command to the underlying buffer if
       * it has user-provided tile handlers, for the same reason we avoid using
       * this command in gegl_buffer_copy() under the same conditions.  see the
       * comment regarding 'fast_copy' in gegl_buffer_copy().
       */
      if (tile_backend_buffer->buffer->tile_storage->n_user_handlers == 0)
        {
          return gegl_tile_backend_buffer_forward_command (tile_backend_buffer,
                                                           command,
                                                           x, y, z, data,
                                                           FALSE);
        }
      return GINT_TO_POINTER (FALSE);

    default:
      return gegl_tile_backend_command (GEGL_TILE_BACKEND (tile_source),
                                        command, x, y, z, data);
    }
}

static GeglTile *
gegl_tile_backend_buffer_get_tile (GeglTileBackendBuffer *tile_backend_buffer,
                                   gint                   x,
                                   gint                   y,
                                   gint                   z)
{
  GeglBuffer *buffer = tile_backend_buffer->buffer;
  GeglTile   *src_tile;
  GeglTile   *tile   = NULL;

  src_tile = gegl_buffer_get_tile (buffer, x, y, z);

  if (G_LIKELY(src_tile))
    {
      tile = gegl_tile_dup (src_tile);

      gegl_tile_unref (src_tile);

      gegl_tile_mark_as_stored (tile);
    }

  return tile;
}

static void
gegl_tile_backend_buffer_set_tile (GeglTileBackendBuffer *tile_backend_buffer,
                                   GeglTile              *tile,
                                   gint                   x,
                                   gint                   y,
                                   gint                   z)
{
  GeglBuffer           *buffer = tile_backend_buffer->buffer;
  GeglTileHandlerCache *cache  = buffer->tile_storage->cache;
  GeglTile             *dest_tile;

  dest_tile = gegl_tile_dup (tile);

  g_rec_mutex_lock (&buffer->tile_storage->mutex);

  gegl_tile_handler_cache_insert (cache, dest_tile, x, y, z);

  g_rec_mutex_unlock (&buffer->tile_storage->mutex);

  gegl_tile_unref (dest_tile);

  gegl_tile_backend_buffer_emit_changed_signal (tile_backend_buffer, x, y, z);
}

static gpointer
gegl_tile_backend_buffer_forward_command (GeglTileBackendBuffer *tile_backend_buffer,
                                          GeglTileCommand        command,
                                          gint                   x,
                                          gint                   y,
                                          gint                   z,
                                          gpointer               data,
                                          gboolean               emit_changed_signal)
{
  GeglBuffer     *buffer = tile_backend_buffer->buffer;
  GeglTileSource *source = GEGL_TILE_SOURCE (buffer);
  gpointer        result;

  g_rec_mutex_lock (&buffer->tile_storage->mutex);

  result = gegl_tile_source_command (source, command, x, y, z, data);

  g_rec_mutex_unlock (&buffer->tile_storage->mutex);

  if (G_UNLIKELY (emit_changed_signal))
    gegl_tile_backend_buffer_emit_changed_signal (tile_backend_buffer, x, y, z);

  return result;
}

static void
gegl_tile_backend_buffer_emit_changed_signal (GeglTileBackendBuffer *tile_backend_buffer,
                                              gint                   x,
                                              gint                   y,
                                              gint                   z)
{
  GeglBuffer *buffer = tile_backend_buffer->buffer;

  if (buffer->changed_signal_connections)
    {
      GeglRectangle rect;

      rect.width  = buffer->tile_width  >> z;
      rect.height = buffer->tile_height >> z;
      rect.x      = x * rect.width  - buffer->shift_x;
      rect.y      = y * rect.height - buffer->shift_y;

      gegl_buffer_emit_changed_signal (buffer, &rect);
    }
}


/*  public functions  */


GeglTileBackend *
gegl_tile_backend_buffer_new (GeglBuffer *buffer)
{
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  return g_object_new (GEGL_TYPE_TILE_BACKEND_BUFFER,
                       "tile-width",  buffer->tile_width,
                       "tile-height", buffer->tile_height,
                       "format",      buffer->soft_format,
                       "buffer",      buffer,
                       NULL);
}
