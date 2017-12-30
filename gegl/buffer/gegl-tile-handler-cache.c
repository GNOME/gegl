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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006,2007 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"

#include <glib.h>
#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-config.h"
#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
#include "gegl-tile.h"
#include "gegl-tile-handler-cache.h"
#include "gegl-tile-storage.h"
#include "gegl-debug.h"

#include "gegl-buffer-cl-cache.h"

/*
#define GEGL_DEBUG_CACHE_HITS
*/

typedef struct CacheItem
{
  GeglTileHandlerCache *handler; /* The specific handler that cached this item*/
  GeglTile *tile;                /* The tile */
  GList     link;                /*  Link in the cache_queue, to avoid
                                  *  queue lookups involving g_list_find() */

  gint      x;                   /* The coordinates this tile was cached for */
  gint      y;
  gint      z;
} CacheItem;

#define LINK_GET_ITEM(link) \
        ((CacheItem *) ((guchar *) link - G_STRUCT_OFFSET (CacheItem, link)))


static gboolean   gegl_tile_handler_cache_equalfunc  (gconstpointer         a,
                                                      gconstpointer         b);
static guint      gegl_tile_handler_cache_hashfunc   (gconstpointer         key);
static void       gegl_tile_handler_cache_dispose    (GObject              *object);
static gboolean   gegl_tile_handler_cache_wash       (GeglTileHandlerCache *cache);
static gpointer   gegl_tile_handler_cache_command    (GeglTileSource       *tile_store,
                                                      GeglTileCommand       command,
                                                      gint                  x,
                                                      gint                  y,
                                                      gint                  z,
                                                      gpointer              data);
static GeglTile * gegl_tile_handler_cache_get_tile   (GeglTileHandlerCache *cache,
                                                      gint                  x,
                                                      gint                  y,
                                                      gint                  z);
static gboolean   gegl_tile_handler_cache_has_tile   (GeglTileHandlerCache *cache,
                                                      gint                  x,
                                                      gint                  y,
                                                      gint                  z);
void              gegl_tile_handler_cache_insert     (GeglTileHandlerCache *cache,
                                                      GeglTile             *tile,
                                                      gint                  x,
                                                      gint                  y,
                                                      gint                  z);
static void       gegl_tile_handler_cache_void       (GeglTileHandlerCache *cache,
                                                      gint                  x,
                                                      gint                  y,
                                                      gint                  z);
static void       gegl_tile_handler_cache_invalidate (GeglTileHandlerCache *cache,
                                                      gint                  x,
                                                      gint                  y,
                                                      gint                  z);


static GMutex             mutex                 = { 0, };
static GQueue            *cache_queue           = NULL;
static gint               cache_wash_percentage = 20;
static volatile guintptr  cache_total           = 0; /* approximate amount of bytes stored */
static guintptr           cache_total_max       = 0; /* maximal value of cache_total */
static guintptr           cache_total_uncloned  = 0; /* approximate amount of uncloned bytes stored */
static gint               cache_hits            = 0;
static gint               cache_misses          = 0;


G_DEFINE_TYPE (GeglTileHandlerCache, gegl_tile_handler_cache, GEGL_TYPE_TILE_HANDLER)


static void
gegl_tile_handler_cache_class_init (GeglTileHandlerCacheClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->dispose = gegl_tile_handler_cache_dispose;
}

static void
gegl_tile_handler_cache_init (GeglTileHandlerCache *cache)
{
  ((GeglTileSource*)cache)->command = gegl_tile_handler_cache_command;
  cache->items = g_hash_table_new (gegl_tile_handler_cache_hashfunc, gegl_tile_handler_cache_equalfunc);
  gegl_tile_cache_init ();
}

static void
drop_hot_tile (GeglTile *tile)
{
  GeglTileStorage *storage = tile->tile_storage;

  if (storage)
    {
#if 0 /* the storage's mutex should already be locked at this point */
      if (gegl_config_threads()>1)
        g_rec_mutex_lock (&storage->mutex);
#endif

      if (storage->hot_tile == tile)
        {
          gegl_tile_unref (storage->hot_tile);
          storage->hot_tile = NULL;
        }

#if 0
      if (gegl_config_threads()>1)
        g_rec_mutex_unlock (&storage->mutex);
#endif
    }
}

static void
gegl_tile_handler_cache_reinit (GeglTileHandlerCache *cache)
{
  CacheItem            *item;
  GHashTableIter        iter;
  gpointer              key, value;

  if (cache->tile_storage->hot_tile)
    {
      gegl_tile_unref (cache->tile_storage->hot_tile);
      cache->tile_storage->hot_tile = NULL;
    }

  if (!cache->count)
    return;

  g_mutex_lock (&mutex);
  {
    g_hash_table_iter_init (&iter, cache->items);
    while (g_hash_table_iter_next (&iter, &key, &value))
    {
      item = (CacheItem *) value;
      if (item->tile)
        {
          if (g_atomic_int_dec_and_test (gegl_tile_n_cached_clones (item->tile)))
            g_atomic_pointer_add (&cache_total, -item->tile->size);
          cache_total_uncloned -= item->tile->size;
          drop_hot_tile (item->tile);
          gegl_tile_mark_as_stored (item->tile); // to avoid saving
          item->tile->tile_storage = NULL;
          gegl_tile_unref (item->tile);
          cache->count--;
        }
      g_queue_unlink (cache_queue, &item->link);
      g_hash_table_iter_remove (&iter);
      g_slice_free (CacheItem, item);
    }
  }

  g_mutex_unlock (&mutex);
}

static void
gegl_tile_handler_cache_dispose (GObject *object)
{
  GeglTileHandlerCache *cache = GEGL_TILE_HANDLER_CACHE (object);

  gegl_tile_handler_cache_reinit (cache);

  if (cache->count < 0)
    {
      g_warning ("cache-handler tile balance not zero: %i\n", cache->count);
    }

  g_hash_table_destroy (cache->items);
  G_OBJECT_CLASS (gegl_tile_handler_cache_parent_class)->dispose (object);
}

static GeglTile *
gegl_tile_handler_cache_get_tile_command (GeglTileSource *tile_store,
                                          gint        x,
                                          gint        y,
                                          gint        z)
{
  GeglTileHandlerCache *cache    = (GeglTileHandlerCache*) (tile_store);
  GeglTileSource       *source   = ((GeglTileHandler*) (tile_store))->source;
  GeglTile             *tile     = NULL;

  if (G_UNLIKELY (gegl_cl_is_accelerated ()))
    gegl_buffer_cl_cache_flush2 (cache, NULL);

  tile = gegl_tile_handler_cache_get_tile (cache, x, y, z);
  if (tile)
    {
      /* we don't bother making cache_{hits,misses} atomic, since they're only
       * needed for GeglStats.
       */
      cache_hits++;
      return tile;
    }
  cache_misses++;

  if (source)
    tile = gegl_tile_source_get_tile (source, x, y, z);

  if (tile)
    gegl_tile_handler_cache_insert (cache, tile, x, y, z);

  return tile;
}

static gpointer
gegl_tile_handler_cache_command (GeglTileSource  *tile_store,
                                 GeglTileCommand  command,
                                 gint             x,
                                 gint             y,
                                 gint             z,
                                 gpointer         data)
{
  GeglTileHandler      *handler = (GeglTileHandler*) (tile_store);
  GeglTileHandlerCache *cache   = (GeglTileHandlerCache*) (handler);

  switch (command)
    {
      case GEGL_TILE_FLUSH:
        {
          if (gegl_cl_is_accelerated ())
            gegl_buffer_cl_cache_flush2 (cache, NULL);

          if (cache->count)
            {
              CacheItem      *item;
              GHashTableIter  iter;
              gpointer        key, value;

              g_mutex_lock (&mutex);

              g_hash_table_iter_init (&iter, cache->items);
              while (g_hash_table_iter_next (&iter, &key, &value))
                {
                  item = (CacheItem *) value;
                  if (item->tile)
                    gegl_tile_store (item->tile);
                }

              g_mutex_unlock (&mutex);
            }
        }
        break;
      case GEGL_TILE_GET:
        /* XXX: we should perhaps store a NIL result, and place the empty
         * generator after the cache, this would have to be possible to disable
         * to work in sync operation with backend.
         */
        return gegl_tile_handler_cache_get_tile_command (tile_store, x, y, z);
      case GEGL_TILE_IS_CACHED:
        return GINT_TO_POINTER(gegl_tile_handler_cache_has_tile (cache, x, y, z));
      case GEGL_TILE_EXIST:
        {
          gboolean exist = gegl_tile_handler_cache_has_tile (cache, x, y, z);
          if (exist)
            return (gpointer)TRUE;
        }
        break;
      case GEGL_TILE_IDLE:
        {
          gboolean action = gegl_tile_handler_cache_wash (cache);
          if (action)
            return GINT_TO_POINTER(action);
          /* with no action, we chain up to lower levels */
          break;
        }
      case GEGL_TILE_REFETCH:
        gegl_tile_handler_cache_invalidate (cache, x, y, z);
        break;
      case GEGL_TILE_VOID:
        gegl_tile_handler_cache_void (cache, x, y, z);
        break;
      case GEGL_TILE_REINIT:
        gegl_tile_handler_cache_reinit (cache);
        break;
      default:
        break;
    }

  return gegl_tile_handler_source_command (handler, command, x, y, z, data);
}

/* write the least recently used dirty tile to disk if it
 * is in the wash_percentage (20%) least recently used tiles,
 * calling this function in an idle handler distributes the
 * tile flushing overhead over time.
 */
gboolean
gegl_tile_handler_cache_wash (GeglTileHandlerCache *cache)
{
  GeglTile  *last_dirty = NULL;
  guint      count      = 0;
  gint       length     = g_queue_get_length (cache_queue);
  gint       wash_tiles = cache_wash_percentage * length / 100;
  GList     *link;

  g_mutex_lock (&mutex);

  for (link = g_queue_peek_tail_link (cache_queue);
       link && count < wash_tiles;
       link = link->prev, count++)
    {
      CacheItem *item = LINK_GET_ITEM (link);
      GeglTile  *tile = item->tile;

      if (tile->tile_storage && ! gegl_tile_is_stored (tile))
        {
          last_dirty = tile;
          g_object_ref (last_dirty->tile_storage);
          gegl_tile_ref (last_dirty);

          break;
        }
    }

  g_mutex_unlock (&mutex);

  if (last_dirty != NULL)
    {
      gegl_tile_store (last_dirty);
      g_object_unref (last_dirty->tile_storage);
      gegl_tile_unref (last_dirty);
      return TRUE;
    }
  return FALSE;
}

static inline CacheItem *
cache_lookup (GeglTileHandlerCache *cache,
              gint                  x,
              gint                  y,
              gint                  z)
{
  CacheItem key;

  key.x       = x;
  key.y       = y;
  key.z       = z;
  key.handler = cache;

  return g_hash_table_lookup (cache->items, &key);
}

/* returns the requested Tile if it is in the cache, NULL otherwize.
 */
static GeglTile *
gegl_tile_handler_cache_get_tile (GeglTileHandlerCache *cache,
                                  gint                  x,
                                  gint                  y,
                                  gint                  z)
{
  CacheItem *result;

  if (cache->count == 0)
    return NULL;

  g_mutex_lock (&mutex);
  result = cache_lookup (cache, x, y, z);
  if (result)
    {
      GeglTile *volatile tile;

      g_queue_unlink (cache_queue, &result->link);
      g_queue_push_head_link (cache_queue, &result->link);
      while (result->tile == NULL)
      {
        g_printerr ("NULL tile in %s %p %i %i %i %p\n", __FUNCTION__, result, result->x, result->y, result->z,
                result->tile);
        g_mutex_unlock (&mutex);
        return NULL;
      }
      gegl_tile_ref (result->tile);
      tile = result->tile;
      g_mutex_unlock (&mutex);
      return tile;
    }
  g_mutex_unlock (&mutex);
  return NULL;
}

static gboolean
gegl_tile_handler_cache_has_tile (GeglTileHandlerCache *cache,
                                  gint                  x,
                                  gint                  y,
                                  gint                  z)
{
  GeglTile *tile = gegl_tile_handler_cache_get_tile (cache, x, y, z);

  if (tile)
    {
      gegl_tile_unref (tile);
      return TRUE;
    }

  return FALSE;
}

static gboolean
gegl_tile_handler_cache_trim (GeglTileHandlerCache *cache)
{
  GList *link;

  link = g_queue_peek_tail_link (cache_queue);

  while ((guintptr) g_atomic_pointer_get (&cache_total) >
         gegl_config ()->tile_cache_size)
    {
      CacheItem       *last_writable;
      GeglTile        *tile;
      GeglTileStorage *storage;
      gboolean         dirty;
      GList           *prev_link;

#ifdef GEGL_DEBUG_CACHE_HITS
      GEGL_NOTE(GEGL_DEBUG_CACHE, "cache_total:"G_GUINT64_FORMAT" > cache_size:"G_GUINT64_FORMAT, cache_total, gegl_config()->tile_cache_size);
      GEGL_NOTE(GEGL_DEBUG_CACHE, "%f%% hit:%i miss:%i  %i]", cache_hits*100.0/(cache_hits+cache_misses), cache_hits, cache_misses, g_queue_get_length (cache_queue));
#endif

      for (; link != NULL; link = g_list_previous (link))
        {
          last_writable = LINK_GET_ITEM (link);
          tile          = last_writable->tile;

          /* if the tile's ref-count is greater than one, then someone is still
           * using the tile, and we must keep it in the cache, so that we can
           * return the same tile object upon request; otherwise, we would end
           * up with two different tile objects referring to the same tile.
           */
          if (tile->ref_count > 1)
            continue;

          storage = tile->tile_storage;

          dirty = storage && ! gegl_tile_is_stored (tile);
          if (dirty && gegl_config_threads()>1)
            {
              /* XXX:  if the tile is dirty, then gegl_tile_unref() will try to
               * store it, acquiring the storage mutex in the process.  this
               * can lead to a deadlock if another thread is already holding
               * that mutex, and is waiting on the global cache mutex, or on a
               * tile storage mutex held by the current thread.  try locking
               * the storage mutex here, and skip the tile if it fails.
               */
              if (! g_rec_mutex_trylock (&storage->mutex))
                continue;
            }

          break;
        }

      if (link == NULL)
        return FALSE;

      prev_link = g_list_previous (link);
      g_queue_unlink (cache_queue, link);
      g_hash_table_remove (last_writable->handler->items, last_writable);
      if (g_atomic_int_dec_and_test (gegl_tile_n_cached_clones (tile)))
        g_atomic_pointer_add (&cache_total, -tile->size);
      cache_total_uncloned -= tile->size;
      last_writable->handler->count--;
      /* drop_hot_tile (tile); */ /* XXX:  no use in trying to drop the hot
                                   * tile, since this tile can't be it --
                                   * the hot tile will have a ref-count of
                                   * at least two.
                                   */
      gegl_tile_store (tile);
      tile->tile_storage = NULL;
      gegl_tile_unref (tile);

      if (dirty && gegl_config_threads()>1)
        g_rec_mutex_unlock (&storage->mutex);

      g_slice_free (CacheItem, last_writable);
      link = prev_link;
    }

  return TRUE;
}

static void
gegl_tile_handler_cache_invalidate (GeglTileHandlerCache *cache,
                                    gint                  x,
                                    gint                  y,
                                    gint                  z)
{
  CacheItem *item;

  g_mutex_lock (&mutex);
  item = cache_lookup (cache, x, y, z);
  if (item)
    {
      if (g_atomic_int_dec_and_test (gegl_tile_n_cached_clones (item->tile)))
        g_atomic_pointer_add (&cache_total, -item->tile->size);
      cache_total_uncloned -= item->tile->size;
      cache->count--;

      g_queue_unlink (cache_queue, &item->link);
      g_hash_table_remove (cache->items, item);

      g_mutex_unlock (&mutex);

      drop_hot_tile (item->tile);
      gegl_tile_mark_as_stored (item->tile); /* to cheat it out of being stored */
      item->tile->tile_storage = NULL;
      gegl_tile_unref (item->tile);

      g_slice_free (CacheItem, item);
    }
  else
    g_mutex_unlock (&mutex);
}


static void
gegl_tile_handler_cache_void (GeglTileHandlerCache *cache,
                              gint                  x,
                              gint                  y,
                              gint                  z)
{
  CacheItem *item;

  g_mutex_lock (&mutex);
  item = cache_lookup (cache, x, y, z);
  if (item)
    {
      if (g_atomic_int_dec_and_test (gegl_tile_n_cached_clones (item->tile)))
        g_atomic_pointer_add (&cache_total, -item->tile->size);
      cache_total_uncloned -= item->tile->size;
      g_queue_unlink (cache_queue, &item->link);
      g_hash_table_remove (cache->items, item);
      cache->count--;
    }
  g_mutex_unlock (&mutex);

  if (item)
    {
      drop_hot_tile (item->tile);
      gegl_tile_void (item->tile);
      item->tile->tile_storage = NULL;
      gegl_tile_unref (item->tile);
    }

  g_slice_free (CacheItem, item);
}

void
gegl_tile_handler_cache_insert (GeglTileHandlerCache *cache,
                                GeglTile             *tile,
                                gint                  x,
                                gint                  y,
                                gint                  z)
{
  CacheItem *item = g_slice_new (CacheItem);

  item->handler   = cache;
  item->tile      = gegl_tile_ref (tile);
  item->link.data = item;
  item->link.next = NULL;
  item->link.prev = NULL;
  item->x         = x;
  item->y         = y;
  item->z         = z;

  // XXX : remove entry if it already exists
  gegl_tile_handler_cache_void (cache, x, y, z);

  tile->x = x;
  tile->y = y;
  tile->z = z;
  tile->tile_storage = cache->tile_storage;

  /* XXX: this is a window when the tile is a zero tile during update */

  g_mutex_lock (&mutex);
  if (g_atomic_int_add (gegl_tile_n_cached_clones (tile), 1) == 0)
    g_atomic_pointer_add (&cache_total, tile->size);
  cache_total_uncloned += item->tile->size;
  g_queue_push_head_link (cache_queue, &item->link);

  cache->count ++;

  g_hash_table_insert (cache->items, item, item);

  gegl_tile_handler_cache_trim (cache);

  /* there's a race between this assignment, and the one at the bottom of
   * gegl_tile_handler_cache_tile_uncloned().  this is acceptable, though,
   * since we only need cache_total_max for GeglStats, so its accuracy is not
   * ciritical.
   */
  cache_total_max = MAX (cache_total_max, cache_total);

  g_mutex_unlock (&mutex);
}

void
gegl_tile_handler_cache_tile_uncloned (GeglTileHandlerCache *cache,
                                       GeglTile             *tile)
{
  guintptr total;

  total = (guintptr) g_atomic_pointer_add (&cache_total, tile->size) +
          tile->size;

  if (total > gegl_config ()->tile_cache_size)
    {
      g_mutex_lock (&mutex);

      gegl_tile_handler_cache_trim (cache);

      g_mutex_unlock (&mutex);
    }

  cache_total_max = MAX (cache_total_max, total);
}

GeglTileHandler *
gegl_tile_handler_cache_new (void)
{
  return g_object_new (GEGL_TYPE_TILE_HANDLER_CACHE, NULL);
}

gsize
gegl_tile_handler_cache_get_total (void)
{
  return cache_total;
}

gsize
gegl_tile_handler_cache_get_total_max (void)
{
  return cache_total_max;
}

gsize
gegl_tile_handler_cache_get_total_uncloned (void)
{
  return cache_total_uncloned;
}

gint
gegl_tile_handler_cache_get_hits (void)
{
  return cache_hits;
}

gint
gegl_tile_handler_cache_get_misses (void)
{
  return cache_misses;
}

void
gegl_tile_handler_cache_reset_stats (void)
{
  cache_total_max = cache_total;
  cache_hits      = 0;
  cache_misses    = 0;
}


static guint
gegl_tile_handler_cache_hashfunc (gconstpointer key)
{
  const CacheItem *e = key;
  guint           hash;
  gint            i;
  gint            srcA = e->x;
  gint            srcB = e->y;
  gint            srcC = e->z;

  /* interleave the 10 least significant bits of all coordinates,
   * this gives us Z-order / morton order of the space and should
   * work well as a hash
   */
  hash = 0;
  for (i = 9; i >= 0; i--)
    {
#define ADD_BIT(bit)    do { hash |= (((bit) != 0) ? 1 : 0); hash <<= 1; } while (0)
      ADD_BIT (srcA & (1 << i));
      ADD_BIT (srcB & (1 << i));
      ADD_BIT (srcC & (1 << i));
#undef ADD_BIT
    }
  return hash ^ GPOINTER_TO_INT (e->handler);
}

static gboolean
gegl_tile_handler_cache_equalfunc (gconstpointer a,
                                   gconstpointer b)
{
  const CacheItem *ea = a;
  const CacheItem *eb = b;

  if (ea->x == eb->x &&
      ea->y == eb->y &&
      ea->z == eb->z &&
      ea->handler == eb->handler)
    return TRUE;
  return FALSE;
}

void
gegl_tile_cache_init (void)
{
  if (cache_queue == NULL)
    cache_queue = g_queue_new ();
}

void
gegl_tile_cache_destroy (void)
{
  if (cache_queue)
    {
      while (g_queue_pop_head_link (cache_queue));
      g_queue_free (cache_queue);
    }
  cache_queue = NULL;
}
