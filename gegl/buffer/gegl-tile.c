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
 * Copyright 2006-2009 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"

#include "string.h" /* memcpy */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>

#include "gegl-buffer.h"
#include "gegl-tile.h"
#include "gegl-tile-alloc.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-storage.h"

/* the offset of the n_clones array, relative to the tile data, when it shares
 * the same buffer as the data.
 */
#define INLINE_N_CLONES_OFFSET (2 * sizeof (gint))

enum
{
  CLONE_STATE_UNCLONED,
  CLONE_STATE_CLONED,
  CLONE_STATE_UNCLONING
};

GeglTile *gegl_tile_ref (GeglTile *tile)
{
  g_atomic_int_inc (&tile->ref_count);
  return tile;
}

static const gint free_data_directly;

void gegl_tile_unref (GeglTile *tile)
{
  if (!g_atomic_int_dec_and_test (&tile->ref_count))
    return;

  /* In the case of a file store for example, we must make sure that
   * the in-memory tile is written to disk before we free the memory,
   * otherwise this data will be lost.
   */
  gegl_tile_store (tile);

  if (g_atomic_int_dec_and_test (gegl_tile_n_clones (tile)))
    { /* no clones */
      if (tile->destroy_notify == (gpointer) &free_data_directly)
        {
          /* tile->data and tile->n_clones share the same buffer,
           * which is freed through tile->data.
           */
          gegl_tile_free (tile->data);
        }
      else
        {
          /* tile->data and tile->n_clones are unrelated, so free them
           * separately.
           */
          if (tile->data && tile->destroy_notify)
            tile->destroy_notify (tile->destroy_notify_data);

          g_slice_free1 (2 * sizeof (gint), tile->n_clones);
        }
    }

  g_slice_free (GeglTile, tile);
}

static inline GeglTile *
gegl_tile_new_bare_internal (void)
{
  GeglTile *tile        = g_slice_new0 (GeglTile);
  tile->ref_count       = 1;
  tile->tile_storage    = NULL;
  tile->stored_rev      = 1;
  tile->rev             = 1;
  tile->lock_count      = 0;
  tile->read_lock_count = 0;
  tile->clone_state     = CLONE_STATE_UNCLONED;
  tile->data            = NULL;

  return tile;
}

GeglTile *
gegl_tile_new_bare (void)
{
  GeglTile *tile = gegl_tile_new_bare_internal ();

  tile->n_clones                    = g_slice_alloc (2 * sizeof (gint));
  *gegl_tile_n_clones (tile)        = 1;
  *gegl_tile_n_cached_clones (tile) = 0;

  tile->destroy_notify      = NULL;
  tile->destroy_notify_data = NULL;

  return tile;
}

GeglTile *
gegl_tile_dup (GeglTile *src)
{
  GeglTile *tile;

  g_warn_if_fail (src->lock_count == 0);
  g_warn_if_fail (! src->damage);

  if (! src->keep_identity)
    {
      src->clone_state          = CLONE_STATE_CLONED;

      tile                      = gegl_tile_new_bare_internal ();

      tile->data                = src->data;
      tile->size                = src->size;
      tile->is_zero_tile        = src->is_zero_tile;
      tile->is_global_tile      = src->is_global_tile;
      tile->clone_state         = CLONE_STATE_CLONED;
      tile->n_clones            = src->n_clones;

      tile->destroy_notify      = src->destroy_notify;
      tile->destroy_notify_data = src->destroy_notify_data;

      g_atomic_int_inc (gegl_tile_n_clones (tile));
    }
  else
    {
      /* we can't clone the source tile if we need to keep its data-pointer
       * identity, since we have no way of uncloning it without changing its
       * data pointer.
       */

      tile = gegl_tile_new (src->size);

      memcpy (tile->data, src->data, src->size);
    }

  /* mark the tile as dirty, since, even though the in-memory tile data may be
   * shared with the source tile, the stored tile data is separate.
   */
  tile->rev++;

  return tile;
}

GeglTile *
gegl_tile_new (gint size)
{
  GeglTile *tile = gegl_tile_new_bare_internal ();

  tile->data = gegl_tile_alloc (size);
  tile->size = size;

  /* gegl_tile_alloc() guarantees that there's enough room for the n_clones
   * array in front of the data buffer.
   */
  tile->n_clones                    = (gint *) (tile->data -
                                                INLINE_N_CLONES_OFFSET);
  *gegl_tile_n_clones (tile)        = 1;
  *gegl_tile_n_cached_clones (tile) = 0;

  tile->destroy_notify      = (gpointer) &free_data_directly;
  tile->destroy_notify_data = NULL;

  return tile;
}

static inline void
gegl_tile_unclone (GeglTile *tile)
{
  if (*gegl_tile_n_clones (tile) > 1)
    {
      GeglTileHandlerCache *notify_cache = NULL;
      gboolean              cached;
      gboolean              global;

      global               = tile->is_global_tile;
      tile->is_global_tile = FALSE;

      if (! global)
        {
          while (! g_atomic_int_compare_and_exchange (&tile->read_lock_count,
                                                      0,
                                                      -1));
        }

      cached = tile->tile_storage && tile->tile_storage->cache;

      if (cached)
        {
          if (! g_atomic_int_dec_and_test (gegl_tile_n_cached_clones (tile)))
            notify_cache = tile->tile_storage->cache;
        }

      /* the tile data is shared with other tiles,
       * create a local copy
       */

      if (! ~tile->damage)
        {
          /* if the tile is fully damaged, we only need to allocate a new
           * buffer, but we don't have to copy the old one.
           */

          tile->is_zero_tile = FALSE;

          if (g_atomic_int_dec_and_test (gegl_tile_n_clones (tile)))
            {
              /* someone else uncloned the tile in the meantime, and we're now
               * the last copy; bail.
               */
              *gegl_tile_n_clones (tile)        = 1;
              *gegl_tile_n_cached_clones (tile) = cached;

              goto end;
            }

          tile->data = gegl_tile_alloc (tile->size);
        }
      else if (tile->is_zero_tile)
        {
          tile->is_zero_tile = FALSE;

          if (g_atomic_int_dec_and_test (gegl_tile_n_clones (tile)))
            {
              /* someone else uncloned the tile in the meantime, and we're now
               * the last copy; bail.
               */
              *gegl_tile_n_clones (tile)        = 1;
              *gegl_tile_n_cached_clones (tile) = cached;

              goto end;
            }

          tile->data = gegl_tile_alloc0 (tile->size);
        }
      else
        {
          guchar *buf;

          buf = gegl_tile_alloc (tile->size);
          memcpy (buf, tile->data, tile->size);

          if (g_atomic_int_dec_and_test (gegl_tile_n_clones (tile)))
            {
              /* someone else uncloned the tile in the meantime, and we're now
               * the last copy; bail.
               */
              gegl_tile_free (buf);
              *gegl_tile_n_clones (tile)        = 1;
              *gegl_tile_n_cached_clones (tile) = cached;

              goto end;
            }

          tile->data = buf;
        }

      tile->n_clones = (gint *) (tile->data - INLINE_N_CLONES_OFFSET);
      *gegl_tile_n_clones (tile)        = 1;
      *gegl_tile_n_cached_clones (tile) = cached;

      tile->destroy_notify      = (gpointer) &free_data_directly;
      tile->destroy_notify_data = NULL;

end:
      if (notify_cache)
        gegl_tile_handler_cache_tile_uncloned (notify_cache, tile);

      if (! global)
        g_atomic_int_set (&tile->read_lock_count, 0);
    }
}

void
gegl_tile_lock (GeglTile *tile)
{
  unsigned int count = 0;
  g_atomic_int_inc (&tile->lock_count);

  while (TRUE)
    {
      switch (g_atomic_int_get (&tile->clone_state))
        {
        case CLONE_STATE_UNCLONED:
          return;

        case CLONE_STATE_CLONED:
          if (g_atomic_int_compare_and_exchange (&tile->clone_state,
                                                 CLONE_STATE_CLONED,
                                                 CLONE_STATE_UNCLONING))
            {
              gegl_tile_unclone (tile);

              g_atomic_int_set (&tile->clone_state, CLONE_STATE_UNCLONED);

              return;
            }
          break;

        case CLONE_STATE_UNCLONING:
          break;
        }

      ++count;
      if (count > 32)
          g_usleep (1000); /* the smallest sleepable time on win32 */
    }
}

static inline void
gegl_tile_void_pyramid (GeglTile *tile,
                        guint64   damage)
{
  if (tile->tile_storage &&
      tile->tile_storage->seen_zoom &&
      tile->z == 0) /* we only accepting voiding the base level */
    {
      gegl_tile_handler_damage_tile (GEGL_TILE_HANDLER (tile->tile_storage),
                                     tile->x, tile->y, tile->z,
                                     damage);
    }
}

void
gegl_tile_unlock (GeglTile *tile)
{
  if (g_atomic_int_dec_and_test (&tile->lock_count))
    {
      g_atomic_int_inc (&tile->rev);
      tile->damage = 0;

      if (tile->unlock_notify != NULL)
        {
          tile->unlock_notify (tile, tile->unlock_notify_data);
        }

      if (tile->z == 0)
        {
          gegl_tile_void_pyramid (tile, ~(guint64) 0);
        }
    }
}

void
gegl_tile_unlock_no_void (GeglTile *tile)
{
  if (g_atomic_int_dec_and_test (&tile->lock_count))
    {
      g_atomic_int_inc (&tile->rev);
      tile->damage = 0;

      if (tile->unlock_notify != NULL)
        {
          tile->unlock_notify (tile, tile->unlock_notify_data);
        }
    }
}

void
gegl_tile_read_lock (GeglTile *tile)
{
  while (TRUE)
    {
      gint count = g_atomic_int_get (&tile->read_lock_count);

      if (count < 0)
        {
          continue;
        }

      if (g_atomic_int_compare_and_exchange (&tile->read_lock_count,
                                             count,
                                             count + 1))
        {
          break;
        }
    }
}

void
gegl_tile_read_unlock (GeglTile *tile)
{
  g_atomic_int_dec_and_test (&tile->read_lock_count);
}

void
gegl_tile_mark_as_stored (GeglTile *tile)
{
  tile->stored_rev = tile->rev;
}

gboolean
gegl_tile_is_stored (GeglTile *tile)
{
  return tile->stored_rev == tile->rev;
}

gboolean
gegl_tile_needs_store (GeglTile *tile)
{
  return tile->tile_storage           &&
         ! gegl_tile_is_stored (tile) &&
         ! tile->damage;
}

void
gegl_tile_void (GeglTile *tile)
{
  gegl_tile_mark_as_stored (tile);

  if (tile->z == 0)
    gegl_tile_void_pyramid (tile, ~(guint64) 0);
}

gboolean
gegl_tile_damage (GeglTile *tile,
                  guint64   damage)
{
  tile->damage |= damage;

  if (! ~tile->damage)
    {
      gegl_tile_void (tile);

      return TRUE;
    }
  else
    {
      if (tile->z == 0)
        gegl_tile_void_pyramid (tile, damage);

      return FALSE;
    }
}

gboolean gegl_tile_store (GeglTile *tile)
{
  gboolean ret;
  if (gegl_tile_is_stored (tile))
    return TRUE;
  else if (! gegl_tile_needs_store (tile))
    return FALSE;

  g_rec_mutex_lock (&tile->tile_storage->mutex);

  if (gegl_tile_is_stored (tile))
    {
      g_rec_mutex_unlock (&tile->tile_storage->mutex);
      return TRUE;
    }

  ret = gegl_tile_source_set_tile (GEGL_TILE_SOURCE (tile->tile_storage),
                                    tile->x,
                                    tile->y,
                                    tile->z,
                                    tile);

  g_rec_mutex_unlock (&tile->tile_storage->mutex);

  return ret;
}

guchar *gegl_tile_get_data (GeglTile *tile)
{
  return tile->data;
}

void gegl_tile_set_data (GeglTile *tile,
                         gpointer  pixel_data,
                         gint      pixel_data_size)
{
  tile->data = pixel_data;
  tile->size = pixel_data_size;
}

void gegl_tile_set_data_full (GeglTile      *tile,
                              gpointer       pixel_data,
                              gint           pixel_data_size,
                              GDestroyNotify destroy_notify,
                              gpointer       destroy_notify_data)
{
  tile->data                = pixel_data;
  tile->size                = pixel_data_size;
  tile->destroy_notify      = destroy_notify;
  tile->destroy_notify_data = destroy_notify_data;
}

void         gegl_tile_set_rev        (GeglTile *tile,
                                       guint     rev)
{
  tile->rev = rev;
}

guint        gegl_tile_get_rev        (GeglTile *tile)
{
  return tile->rev;
}

void gegl_tile_set_unlock_notify (GeglTile         *tile,
                                  GeglTileCallback  unlock_notify,
                                  gpointer          unlock_notify_data)
{
  tile->unlock_notify      = unlock_notify;
  tile->unlock_notify_data = unlock_notify_data;
}
