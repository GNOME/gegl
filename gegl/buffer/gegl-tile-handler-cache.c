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
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2006,2007 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"

#include <glib.h>
#include <glib-object.h>

#include "gegl-buffer-config.h"
#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
#include "gegl-tile.h"
#include "gegl-tile-handler-cache.h"
#include "gegl-tile-storage.h"
#include "gegl-debug.h"

/*
#define GEGL_DEBUG_CACHE_HITS
*/

#define GEGL_CACHE_TRIM_INTERVAL   100000 /* microseconds */
#define GEGL_CACHE_TRIM_RATIO_MIN  0.01
#define GEGL_CACHE_TRIM_RATIO_MAX  0.50
#define GEGL_CACHE_TRIM_RATIO_RATE 2.0

typedef struct CacheItem
{
  GeglTile *tile; /* The tile */
  GList     link; /*  Link in the cache queue, to avoid
                   *  queue lookups involving g_list_find() */

  gint      x;    /* The coordinates this tile was cached for */
  gint      y;
  gint      z;
} CacheItem;

#define LINK_GET_CACHE(l) \
        ((GeglTileHandlerCache *) ((guchar *) l - G_STRUCT_OFFSET (GeglTileHandlerCache, link)))
#define LINK_GET_ITEM(l) \
        ((CacheItem *) ((guchar *) l - G_STRUCT_OFFSET (CacheItem, link)))


static gboolean   gegl_tile_handler_cache_equalfunc  (gconstpointer             a,
                                                      gconstpointer             b);
static guint      gegl_tile_handler_cache_hashfunc   (gconstpointer             key);
static void       gegl_tile_handler_cache_dispose    (GObject                  *object);
static gboolean   gegl_tile_handler_cache_wash       (GeglTileHandlerCache     *cache);
static gpointer   gegl_tile_handler_cache_command    (GeglTileSource           *tile_store,
                                                      GeglTileCommand           command,
                                                      gint                      x,
                                                      gint                      y,
                                                      gint                      z,
                                                      gpointer                  data);
static gboolean   gegl_tile_handler_cache_has_tile   (GeglTileHandlerCache     *cache,
                                                      gint                      x,
                                                      gint                      y,
                                                      gint                      z);
static void       gegl_tile_handler_cache_void       (GeglTileHandlerCache     *cache,
                                                      gint                      x,
                                                      gint                      y,
                                                      gint                      z,
                                                      guint64                   damage);
static void       gegl_tile_handler_cache_invalidate (GeglTileHandlerCache     *cache,
                                                      gint                      x,
                                                      gint                      y,
                                                      gint                      z);
static gboolean   gegl_tile_handler_cache_copy       (GeglTileHandlerCache     *cache,
                                                      gint                      x,
                                                      gint                      y,
                                                      gint                      z,
                                                      const GeglTileCopyParams *params);


static GMutex             mutex                 = { 0, };
static GQueue             cache_queue           = G_QUEUE_INIT;
static gint               cache_wash_percentage = 20;
static          guintptr  cache_total           = 0; /* approximate amount of bytes stored */
static guintptr           cache_total_max       = 0; /* maximal value of cache_total */
static volatile guintptr  cache_total_uncloned  = 0; /* approximate amount of uncloned bytes stored */
static gint               cache_hits            = 0;
static gint               cache_misses          = 0;
static guintptr           cache_time            = 0;


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
  g_queue_init (&cache->queue);

  gegl_tile_handler_cache_connect (cache);
}

static void
drop_hot_tile (GeglTile *tile)
{
  GeglTileStorage *storage = tile->tile_storage;

  if (storage)
    {
      tile = gegl_tile_storage_try_steal_hot_tile (storage, tile);

      if (tile)
        gegl_tile_unref (tile);
    }
}

static void
gegl_tile_handler_cache_reinit (GeglTileHandlerCache *cache)
{
  CacheItem *item;
  GList     *link;

  cache->time = cache->stamp = 0;

  if (cache->tile_storage->hot_tile)
    {
      gegl_tile_unref (cache->tile_storage->hot_tile);
      cache->tile_storage->hot_tile = NULL;
    }

  g_hash_table_remove_all (cache->items);

  while ((link = g_queue_pop_head_link (&cache->queue)))
    {
      item = LINK_GET_ITEM (link);
      if (item->tile)
        {
          if (g_atomic_int_dec_and_test (gegl_tile_n_cached_clones (item->tile)))
            g_atomic_pointer_add (&cache_total, -item->tile->size);
          g_atomic_pointer_add (&cache_total_uncloned, -item->tile->size);
          drop_hot_tile (item->tile);
          gegl_tile_mark_as_stored (item->tile); // to avoid saving
          item->tile->tile_storage = NULL;
          gegl_tile_unref (item->tile);
        }
      g_slice_free (CacheItem, item);
    }
}

static void
gegl_tile_handler_cache_dispose (GObject *object)
{
  GeglTileHandlerCache *cache = GEGL_TILE_HANDLER_CACHE (object);

  gegl_tile_handler_cache_disconnect (cache);

  gegl_tile_handler_cache_reinit (cache);

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

  if (gegl_tile_handler_cache_ext_flush)
    gegl_tile_handler_cache_ext_flush (cache, NULL);

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
          GList *link;

          if (gegl_tile_handler_cache_ext_flush)
            gegl_tile_handler_cache_ext_flush (cache, NULL);

          for (link = g_queue_peek_head_link (&cache->queue);
               link;
               link = g_list_next (link))
            {
              CacheItem *item = LINK_GET_ITEM (link);

              if (item->tile)
                gegl_tile_store (item->tile);
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
        gegl_tile_handler_cache_void (cache, x, y, z,
                                      data ? *(const guint64 *) data :
                                             ~(guint64) 0);
        break;
      case GEGL_TILE_REINIT:
        gegl_tile_handler_cache_reinit (cache);
        break;
      case GEGL_TILE_COPY:
        /* we potentially chain up in gegl_tile_handler_cache_copy(), because
         * its logic is interleaved with that of the backend.
         */
        return GINT_TO_POINTER (gegl_tile_handler_cache_copy (cache,
                                                              x, y, z, data));
      default:
        break;
    }

  return gegl_tile_handler_source_command (handler, command, x, y, z, data);
}

/* find the oldest (least-recently used) nonempty cache, after prev_cache (if
 * not NULL).  passing the previous result of this function as prev_cache
 * allows iterating over the caches in chronological order.
 *
 * if most caches haven't been accessed since the last call to this function,
 * it should be rather cheap (approaching O(1)).
 *
 * the global cache mutex must be held while calling this function, however,
 * individual caches may be accessed concurrently.  as a result, there is a
 * race between modifying the caches' last-access time during access, and
 * inspecting the time by this function.  this isn't critical, but it does mean
 * that the result might not always be accurate.
 */
static GeglTileHandlerCache *
gegl_tile_handler_cache_find_oldest_cache (GeglTileHandlerCache *prev_cache)
{
  GList                *link;
  GeglTileHandlerCache *oldest_cache = NULL;
  guintptr              oldest_time  = 0;

  /* find the oldest cache, after prev_cache */
  for (link = prev_cache ? g_list_next (&prev_cache->link) :
                           g_queue_peek_head_link (&cache_queue);
       link;
       link = g_list_next (link))
    {
      GeglTileHandlerCache *cache = LINK_GET_CACHE (link);
      guintptr              time  = cache->time;
      guintptr              stamp = cache->stamp;

      /* the cache is empty */
      if (! time)
        continue;

      /* the cache is being disconnected */
      if (! cache->link.data)
        continue;

      if (time == stamp)
        {
          oldest_cache = cache;
          oldest_time  = time;

          /* the cache is still stamped, so it must be the oldest of the
           * remaining caches, and we can break early
           */
          break;
        }
      else if (! oldest_time || time < oldest_time)
        {
          oldest_cache = cache;
          oldest_time  = time;
        }
    }

  if (oldest_cache)
    {
      /* stamp the cache */
      oldest_cache->stamp = oldest_time;

      /* ... and move it after prev_cache */
      g_queue_unlink (&cache_queue, &oldest_cache->link);

      if (prev_cache)
        {
          if (prev_cache->link.next)
            {
              oldest_cache->link.prev = &prev_cache->link;
              oldest_cache->link.next = prev_cache->link.next;

              oldest_cache->link.prev->next = &oldest_cache->link;
              oldest_cache->link.next->prev = &oldest_cache->link;

              cache_queue.length++;
            }
          else
            {
              g_queue_push_tail_link (&cache_queue, &oldest_cache->link);
            }
        }
      else
        {
          g_queue_push_head_link (&cache_queue, &oldest_cache->link);
        }
    }

  return oldest_cache;
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
  guintptr   size       = 0;
  guintptr   wash_size;

  g_mutex_lock (&mutex);

  wash_size = (gdouble) cache_total_uncloned *
              cache_wash_percentage / 100.0 + 0.5;

  cache = NULL;

  while (size < wash_size)
    {
      GList *link;

      cache = gegl_tile_handler_cache_find_oldest_cache (cache);

      if (cache == NULL)
        break;

      if (! g_rec_mutex_trylock (&cache->tile_storage->mutex))
        {
          continue;
        }

      for (link = g_queue_peek_tail_link (&cache->queue);
           link && size < wash_size;
           link = g_list_previous (link))
        {
          CacheItem *item = LINK_GET_ITEM (link);
          GeglTile  *tile = item->tile;

          if (tile->tile_storage && ! gegl_tile_is_stored (tile))
            {
              last_dirty = tile;
              g_object_ref (last_dirty->tile_storage);
              gegl_tile_ref (last_dirty);

              size = wash_size;
              break;
            }

          size += tile->size;
        }

      g_rec_mutex_unlock (&cache->tile_storage->mutex);
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

  key.x = x;
  key.y = y;
  key.z = z;

  return g_hash_table_lookup (cache->items, &key);
}

/* returns the requested Tile if it is in the cache, NULL otherwize.
 */
GeglTile *
gegl_tile_handler_cache_get_tile (GeglTileHandlerCache *cache,
                                  gint                  x,
                                  gint                  y,
                                  gint                  z)
{
  CacheItem *result;

  if (g_queue_is_empty (&cache->queue))
    return NULL;

  result = cache_lookup (cache, x, y, z);
  if (result)
    {
      g_queue_unlink (&cache->queue, &result->link);
      g_queue_push_head_link (&cache->queue, &result->link);
      cache->time = ++cache_time;
      if (result->tile == NULL)
      {
        g_printerr ("NULL tile in %s %p %i %i %i %p\n", __FUNCTION__, result, result->x, result->y, result->z,
                result->tile);
        return NULL;
      }
      gegl_tile_ref (result->tile);
      return result->tile;
    }
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
  GList          *link;
  gint64          time;
  static gint64   last_time;
  static gdouble  ratio  = GEGL_CACHE_TRIM_RATIO_MIN;
  guint64         target_size;
  static guint    counter;

  cache = NULL;
  link  = NULL;

  g_mutex_lock (&mutex);

  target_size = gegl_buffer_config ()->tile_cache_size;

  if ((guintptr) g_atomic_pointer_get (&cache_total) <= target_size)
    {
      g_mutex_unlock (&mutex);

      return TRUE;
    }

  time = g_get_monotonic_time ();

  if (time - last_time < GEGL_CACHE_TRIM_INTERVAL)
    {
      ratio = MIN (ratio * GEGL_CACHE_TRIM_RATIO_RATE,
                   GEGL_CACHE_TRIM_RATIO_MAX);
    }
  else if (time - last_time >= 2 * GEGL_CACHE_TRIM_INTERVAL)
    {
      ratio = GEGL_CACHE_TRIM_RATIO_MIN;
    }

  target_size -= target_size * ratio;

  g_mutex_unlock (&mutex);

  while ((guintptr) g_atomic_pointer_get (&cache_total) > target_size)
    {
      CacheItem *last_writable;
      GeglTile  *tile;
      GList     *prev_link;

#ifdef GEGL_DEBUG_CACHE_HITS
      GEGL_NOTE(GEGL_DEBUG_CACHE, "cache_total:"G_GUINT64_FORMAT" > cache_size:"G_GUINT64_FORMAT, cache_total, gegl_buffer_config()->tile_cache_size);
      GEGL_NOTE(GEGL_DEBUG_CACHE, "%f%% hit:%i miss:%i  %i]", cache_hits*100.0/(cache_hits+cache_misses), cache_hits, cache_misses, g_queue_get_length (&cache_queue));
#endif

      if (! link)
        {
          if (cache)
            g_rec_mutex_unlock (&cache->tile_storage->mutex);

          g_mutex_lock (&mutex);

          do
            {
              cache = gegl_tile_handler_cache_find_oldest_cache (cache);
            }
          while (cache &&
                 /* XXX:  when trimming a dirty tile, gegl_tile_unref() will
                  * try to store it, acquiring the cache's storage mutex in the
                  * process.  this can lead to a deadlock if another thread is
                  * already holding that mutex, and is waiting on the global
                  * cache mutex, or on a tile-storage mutex held by the current
                  * thread.  try locking the cache's storage mutex here, and
                  * skip the cache if it fails.
                  */
                 ! g_rec_mutex_trylock (&cache->tile_storage->mutex));

          g_mutex_unlock (&mutex);

          if (! cache)
            break;

          link = g_queue_peek_tail_link (&cache->queue);
        }

      for (; link; link = g_list_previous (link))
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

          /* if we need to maintain the tile's data-pointer identity we can't
           * remove it from the cache, since the storage might copy the data
           * and throw the tile away.
           */
          if (tile->keep_identity)
            continue;

          /* a set of cloned tiles is only counted once toward the total cache
           * size, so the entire set has to be removed from the cache in order
           * to reclaim the memory of a single tile.  in other words, in a set
           * of n cloned tiles, we can assume that each individual tile
           * contributes only 1/n of its size to the total cache size.  on the
           * other hand, storing a cloned tile is as expensive as storing an
           * uncloned tile.  therefore, if the tile needs to be stored, we only
           * remove it with a probability of 1/n.
           */
          if (gegl_tile_needs_store (tile) &&
              counter++ % *gegl_tile_n_cached_clones (tile))
            {
              continue;
            }

          break;
        }

      /* the cache is being disconnected */
      if (! cache->link.data)
        link = NULL;

      if (! link)
        continue;

      prev_link = g_list_previous (link);
      g_queue_unlink (&cache->queue, link);
      g_hash_table_remove (cache->items, last_writable);
      if (g_queue_is_empty (&cache->queue))
        cache->time = cache->stamp = 0;
      if (g_atomic_int_dec_and_test (gegl_tile_n_cached_clones (tile)))
        g_atomic_pointer_add (&cache_total, -tile->size);
      g_atomic_pointer_add (&cache_total_uncloned, -tile->size);
      /* drop_hot_tile (tile); */ /* XXX:  no use in trying to drop the hot
                                   * tile, since this tile can't be it --
                                   * the hot tile will have a ref-count of
                                   * at least two.
                                   */
      gegl_tile_store (tile);
      tile->tile_storage = NULL;
      gegl_tile_unref (tile);

      g_slice_free (CacheItem, last_writable);
      link = prev_link;
    }

  if (cache)
    g_rec_mutex_unlock (&cache->tile_storage->mutex);

  g_mutex_lock (&mutex);

  last_time = g_get_monotonic_time ();

  g_mutex_unlock (&mutex);

  return cache != NULL;
}

static void
gegl_tile_handler_cache_invalidate (GeglTileHandlerCache *cache,
                                    gint                  x,
                                    gint                  y,
                                    gint                  z)
{
  CacheItem *item;

  item = cache_lookup (cache, x, y, z);
  if (item)
    {
      if (g_atomic_int_dec_and_test (gegl_tile_n_cached_clones (item->tile)))
        g_atomic_pointer_add (&cache_total, -item->tile->size);
      g_atomic_pointer_add (&cache_total_uncloned, -item->tile->size);

      g_queue_unlink (&cache->queue, &item->link);
      g_hash_table_remove (cache->items, item);

      if (g_queue_is_empty (&cache->queue))
        cache->time = cache->stamp = 0;

      drop_hot_tile (item->tile);
      gegl_tile_mark_as_stored (item->tile); /* to cheat it out of being stored */
      item->tile->tile_storage = NULL;
      gegl_tile_unref (item->tile);

      g_slice_free (CacheItem, item);
    }
}

static gboolean
gegl_tile_handler_cache_copy (GeglTileHandlerCache     *cache,
                              gint                      x,
                              gint                      y,
                              gint                      z,
                              const GeglTileCopyParams *params)
{
  GeglTile *tile;
  GeglTile *dst_tile = NULL;
  gboolean  success  = FALSE;

  if (gegl_tile_handler_cache_ext_flush)
    gegl_tile_handler_cache_ext_flush (cache, NULL);

  tile = gegl_tile_handler_cache_get_tile (cache, x, y, z);

  /* if the tile is not fully valid, bail, so that the copy happens using a
   * TILE_GET commands, validating the tile in the process.
   */
  if (tile && tile->damage)
    {
      gegl_tile_unref (tile);

      return FALSE;
    }

  /* otherwise, if we have the tile and it's valid, copy it directly to the
   * destination first, so that the copied tile will already be present in the
   * destination cache.
   */
  if (tile)
    {
      GeglTileHandler *dst_handler;

      if (params->dst_buffer)
        dst_handler = GEGL_TILE_HANDLER (params->dst_buffer->tile_storage);
      else
        dst_handler = GEGL_TILE_HANDLER (cache);

      dst_tile = gegl_tile_handler_dup_tile (dst_handler,
                                             tile,
                                             params->dst_x,
                                             params->dst_y,
                                             params->dst_z);

      success = TRUE;
    }
  /* otherwise, if we don't have the tile, remove the corresponding tile from
   * the destination cache, so that, if the backend copies the tile to the
   * destination backend below, the destination cache doesn't hold an outdated
   * tile.  we do this *before* letting the backend copy the tile, since the
   * backend might insert the copied tile to the destination cache as well, as
   * is the case for GeglTileBackendBuffer.
   */
  else
    {
      GeglTileHandlerCache *dst_cache;

      if (params->dst_buffer)
        dst_cache = params->dst_buffer->tile_storage->cache;
      else
        dst_cache = cache;

      if (dst_cache)
        {
          gegl_tile_handler_cache_remove (dst_cache,
                                          params->dst_x,
                                          params->dst_y,
                                          params->dst_z);
        }
    }

  /* then, if we don't have the tile, or if the tile is stored, iow, if the
   * tile might exist in the backend in an up-to-date state ...
   */
  if (! tile || gegl_tile_is_stored (tile))
    {
      /* ... try letting the backend copy the tile, so that the copied tile
       * will already be stored in the destination backend.
       */
      if (gegl_tile_handler_source_command (GEGL_TILE_HANDLER (cache),
                                            GEGL_TILE_COPY, x, y, z,
                                            (gpointer) params))
        {
          /* if the backend copied the tile, we can mark the copied tile, if
           * exists, as stored.
           */
          if (dst_tile)
            gegl_tile_mark_as_stored (dst_tile);

          success = TRUE;
        }
    }

  if (dst_tile)
    gegl_tile_unref (dst_tile);

  if (tile)
    gegl_tile_unref (tile);

  /* the copy is considered successful if either we or the backend (or both)
   * copied the tile.
   */
  return success;
}


static void
gegl_tile_handler_cache_remove_item (GeglTileHandlerCache *cache,
                                     CacheItem            *item)
{
  if (g_atomic_int_dec_and_test (gegl_tile_n_cached_clones (item->tile)))
    g_atomic_pointer_add (&cache_total, -item->tile->size);
  g_atomic_pointer_add (&cache_total_uncloned, -item->tile->size);

  g_queue_unlink (&cache->queue, &item->link);
  g_hash_table_remove (cache->items, item);

  if (g_queue_is_empty (&cache->queue))
    cache->time = cache->stamp = 0;

  item->tile->tile_storage = NULL;
  gegl_tile_unref (item->tile);

  g_slice_free (CacheItem, item);
}

void
gegl_tile_handler_cache_remove (GeglTileHandlerCache *cache,
                                gint                  x,
                                gint                  y,
                                gint                  z)
{
  CacheItem *item;

  item = cache_lookup (cache, x, y, z);

  if (item)
    {
      drop_hot_tile (item->tile);

      gegl_tile_handler_cache_remove_item (cache, item);
    }
}

static void
gegl_tile_handler_cache_void (GeglTileHandlerCache *cache,
                              gint                  x,
                              gint                  y,
                              gint                  z,
                              guint64               damage)
{
  CacheItem *item;

  item = cache_lookup (cache, x, y, z);
  if (item)
    {
      drop_hot_tile (item->tile);

      if (gegl_tile_damage (item->tile, damage))
        gegl_tile_handler_cache_remove_item (cache, item);
    }
  else if (z == 0 && damage)
    {
      gegl_tile_handler_damage_tile (GEGL_TILE_HANDLER (cache),
                                     x, y, z, damage);
    }
}

void
gegl_tile_handler_cache_insert (GeglTileHandlerCache *cache,
                                GeglTile             *tile,
                                gint                  x,
                                gint                  y,
                                gint                  z)
{
  CacheItem *item = g_slice_new (CacheItem);
  guintptr   total;

  item->tile      = gegl_tile_ref (tile);
  item->link.data = item;
  item->link.next = NULL;
  item->link.prev = NULL;
  item->x         = x;
  item->y         = y;
  item->z         = z;

  // XXX : remove entry if it already exists
  gegl_tile_handler_cache_remove (cache, x, y, z);

  tile->x = x;
  tile->y = y;
  tile->z = z;
  tile->tile_storage = cache->tile_storage;

  /* XXX: this is a window when the tile is a zero tile during update */

  cache->time = ++cache_time;

  if (g_atomic_int_add (gegl_tile_n_cached_clones (tile), 1) == 0)
    total = g_atomic_pointer_add (&cache_total, tile->size) + tile->size;
  else
    total = (guintptr) g_atomic_pointer_get (&cache_total);
  g_atomic_pointer_add (&cache_total_uncloned, tile->size);
  g_hash_table_add (cache->items, item);
  g_queue_push_head_link (&cache->queue, &item->link);

  if (total > gegl_buffer_config ()->tile_cache_size)
    gegl_tile_handler_cache_trim (cache);

  /* there's a race between this assignment, and the one at the bottom of
   * gegl_tile_handler_cache_tile_uncloned().  this is acceptable, though,
   * since we only need cache_total_max for GeglStats, so its accuracy is not
   * ciritical.
   */
  cache_total_max = MAX (cache_total_max, cache_total);
}

void
gegl_tile_handler_cache_tile_uncloned (GeglTileHandlerCache *cache,
                                       GeglTile             *tile)
{
  guintptr total;

  total = (guintptr) g_atomic_pointer_add (&cache_total, tile->size) +
          tile->size;

  if (total > gegl_buffer_config ()->tile_cache_size)
    gegl_tile_handler_cache_trim (cache);

  cache_total_max = MAX (cache_total_max, total);
}

GeglTileHandler *
gegl_tile_handler_cache_new (void)
{
  return g_object_new (GEGL_TYPE_TILE_HANDLER_CACHE, NULL);
}

void
gegl_tile_handler_cache_connect (GeglTileHandlerCache *cache)
{
  /* join the global cache queue */
  if (! cache->link.data)
    {
      cache->link.data = cache;

      g_mutex_lock (&mutex);
      g_queue_push_tail_link (&cache_queue, &cache->link);
      g_mutex_unlock (&mutex);
    }
}

void
gegl_tile_handler_cache_disconnect (GeglTileHandlerCache *cache)
{
  /* leave the global cache queue */
  if (cache->link.data)
    {
      cache->link.data = NULL;

      g_rec_mutex_lock (&cache->tile_storage->mutex);

      g_mutex_lock (&mutex);
      g_queue_unlink (&cache_queue, &cache->link);
      g_mutex_unlock (&mutex);

      g_rec_mutex_unlock (&cache->tile_storage->mutex);
    }
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
gegl_tile_handler_cache_get_total_uncompressed (void)
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
  return hash;
}

static gboolean
gegl_tile_handler_cache_equalfunc (gconstpointer a,
                                   gconstpointer b)
{
  const CacheItem *ea = a;
  const CacheItem *eb = b;

  if (ea->x == eb->x &&
      ea->y == eb->y &&
      ea->z == eb->z)
    return TRUE;
  return FALSE;
}

static void
gegl_buffer_config_tile_cache_size_notify (GObject    *gobject,
                                           GParamSpec *pspec,
                                           gpointer    user_data)
{
  if ((guintptr) g_atomic_pointer_get (&cache_total) >
      gegl_buffer_config () ->tile_cache_size)
    {
      gegl_tile_handler_cache_trim (NULL);
    }
}

void
gegl_tile_cache_init (void)
{
  g_signal_connect (gegl_buffer_config (), "notify::tile-cache-size",
                    G_CALLBACK (gegl_buffer_config_tile_cache_size_notify), NULL);
}

void
gegl_tile_cache_destroy (void)
{
  g_signal_handlers_disconnect_by_func (gegl_buffer_config(),
                                        gegl_buffer_config_tile_cache_size_notify,
                                        NULL);
  g_warn_if_fail (g_queue_is_empty (&cache_queue));


  if (g_queue_is_empty (&cache_queue))
    {
      g_queue_clear (&cache_queue);
    }
  else
    {
     /* we leak portions of the GQueue data structure when it is not empty,
        permitting leaked tiles to still be unreffed correctly */
    }
}
