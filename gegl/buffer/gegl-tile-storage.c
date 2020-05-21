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

#include "config.h"

#include <glib-object.h>
#include <glib/gstdio.h>

#include "gegl-buffer.h"
#include "gegl-buffer-types.h"
#include "gegl-tile-storage.h"
#include "gegl-tile-handler-empty.h"
#include "gegl-tile-handler-zoom.h"
#include "gegl-tile-handler-private.h"


G_DEFINE_TYPE (GeglTileStorage, gegl_tile_storage, GEGL_TYPE_TILE_HANDLER_CHAIN)

static GObjectClass * parent_class = NULL;

enum
{
  CHANGED,
  LAST_SIGNAL
};

guint gegl_tile_storage_signals[LAST_SIGNAL] = { 0 };

GeglTileStorage *
gegl_tile_storage_new (GeglTileBackend *backend,
                       gboolean         initialized)
{
  GeglTileStorage *tile_storage = g_object_new (GEGL_TYPE_TILE_STORAGE, NULL);
  GeglTileHandlerChain  *tile_handler_chain;
  GeglTileHandler       *handler;
  GeglTileHandler       *cache = NULL;
  GeglTileHandler       *empty = NULL;
  GeglTileHandler       *zoom = NULL;

  tile_handler_chain = GEGL_TILE_HANDLER_CHAIN (tile_storage);
  handler  = GEGL_TILE_HANDLER (tile_storage);

  tile_storage->tile_width  = backend->priv->tile_width;
  tile_storage->tile_height = backend->priv->tile_height;
  tile_storage->px_size     = backend->priv->px_size;
  tile_storage->format      = gegl_tile_backend_get_format (backend);
  tile_storage->tile_size   = gegl_tile_backend_get_tile_size (backend);

  gegl_tile_handler_set_source (handler, GEGL_TILE_SOURCE (backend));

  cache = gegl_tile_handler_cache_new ();
  empty = gegl_tile_handler_empty_new (backend, initialized);
  zoom = gegl_tile_handler_zoom_new (backend);

  /* set storage and cache on ourself so chain_add() can use it */
  _gegl_tile_handler_set_tile_storage (handler, tile_storage);
  _gegl_tile_handler_set_cache (handler, (GeglTileHandlerCache *) cache);

  gegl_tile_handler_chain_add (tile_handler_chain, cache);
  gegl_tile_handler_chain_add (tile_handler_chain, zoom);
  gegl_tile_handler_chain_add (tile_handler_chain, empty);

  g_object_unref (cache);
  g_object_unref (zoom);
  g_object_unref (empty);

  tile_storage->cache = (GeglTileHandlerCache *) cache;
  ((GeglTileHandlerCache *) cache)->tile_storage = tile_storage;
  gegl_tile_handler_chain_bind (tile_handler_chain);

  backend->priv->storage = tile_storage;

  return tile_storage;
}

void
gegl_tile_storage_add_handler (GeglTileStorage *tile_storage,
                               GeglTileHandler *handler)
{
  GeglTileHandlerChain *chain = GEGL_TILE_HANDLER_CHAIN (tile_storage);

  g_return_if_fail (GEGL_IS_TILE_HANDLER (handler));

  gegl_tile_handler_chain_add (chain, handler);

  /* FIXME: Move the handler to before the cache and other custom handlers */
  chain->chain = g_slist_remove (chain->chain, handler);
  chain->chain = g_slist_insert (chain->chain, handler, 2);

  gegl_tile_handler_chain_bind (chain);

  tile_storage->n_user_handlers++;
}

void
gegl_tile_storage_remove_handler (GeglTileStorage *tile_storage,
                                  GeglTileHandler *handler)
{
  GeglTileHandlerChain *chain = GEGL_TILE_HANDLER_CHAIN (tile_storage);

  g_return_if_fail (GEGL_IS_TILE_HANDLER (handler));
  g_return_if_fail (g_slist_find (chain->chain, handler));

  chain->chain = g_slist_remove (chain->chain, handler);
  gegl_tile_handler_set_source (handler, NULL);
  g_object_unref (handler);

  gegl_tile_handler_chain_bind (chain);

  tile_storage->n_user_handlers--;
}

GeglTile *
gegl_tile_storage_steal_hot_tile (GeglTileStorage *tile_storage)
{
  GeglTile *tile;

  tile = g_atomic_pointer_get (&tile_storage->hot_tile);

  if (tile &&
      ! g_atomic_pointer_compare_and_exchange (&tile_storage->hot_tile,
                                               tile, NULL))
    {
      tile = NULL;
    }

  return tile;
}

GeglTile *
gegl_tile_storage_try_steal_hot_tile (GeglTileStorage *tile_storage,
                                      GeglTile        *tile)
{
  if (tile &&
      ! g_atomic_pointer_compare_and_exchange (&tile_storage->hot_tile,
                                               tile, NULL))
    {
      tile = NULL;
    }

  return tile;
}

void
gegl_tile_storage_take_hot_tile (GeglTileStorage *tile_storage,
                                 GeglTile        *tile)
{
  if (! g_atomic_pointer_compare_and_exchange (&tile_storage->hot_tile,
                                               NULL, tile))
    {
      gegl_tile_unref (tile);
    }
}

static void
gegl_tile_storage_dispose (GObject *object)
{
  GeglTileStorage *self = GEGL_TILE_STORAGE (object);

  /* disconnect the cache before destruction, to avoid a race condition with
   * other threads trimming the global cache through an unrelated cache
   * handler.  see bug #795597.
   */
  gegl_tile_handler_cache_disconnect (self->cache);

  (*G_OBJECT_CLASS (parent_class)->dispose)(object);
}

static void
gegl_tile_storage_finalize (GObject *object)
{
  GeglTileStorage *self = GEGL_TILE_STORAGE (object);

  g_rec_mutex_clear (&self->mutex);

  (*G_OBJECT_CLASS (parent_class)->finalize)(object);
}

static void
gegl_tile_storage_class_init (GeglTileStorageClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  parent_class                = g_type_class_peek_parent (class);
  gobject_class->dispose      = gegl_tile_storage_dispose;
  gobject_class->finalize     = gegl_tile_storage_finalize;

  gegl_tile_storage_signals[CHANGED] =
        g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_RECTANGLE);
}

static void
gegl_tile_storage_init (GeglTileStorage *tile_storage)
{
  tile_storage->seen_zoom = 0;
  g_rec_mutex_init (&tile_storage->mutex);
}
