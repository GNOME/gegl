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
 * Copyright 2006-2009 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"

#include "string.h" /* memcpy */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>

#include "gegl-types-internal.h"

#include "gegl.h"
#include "gegl-buffer.h"
#include "gegl-tile.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-source.h"
#include "gegl-tile-storage.h"

/* the offset at which the tile data begins, when it shares the same buffer as
 * n_clones.  use 16 bytes, which is the alignment we use for gegl_malloc(), so
 * that the tile data is similarly aligned.
 */
#define INLINE_N_ELEMENTS_DATA_OFFSET 16
G_STATIC_ASSERT (INLINE_N_ELEMENTS_DATA_OFFSET >= sizeof (gint));

GeglTile *gegl_tile_ref (GeglTile *tile)
{
  g_atomic_int_inc (&tile->ref_count);
  return tile;
}

static int free_n_clones_directly;
static int free_data_directly;

void gegl_tile_unref (GeglTile *tile)
{
  if (!g_atomic_int_dec_and_test (&tile->ref_count))
    return;

  /* In the case of a file store for example, we must make sure that
   * the in-memory tile is written to disk before we free the memory,
   * otherwise this data will be lost.
   */
  gegl_tile_store (tile);

  if (g_atomic_int_dec_and_test (tile->n_clones))
    { /* no clones */
      if (tile->destroy_notify == (void*)&free_n_clones_directly)
        {
          /* tile->n_clones and tile->data share the same buffer,
           * with tile->n_clones at the front, so free the buffer
           * through it.
           */
          gegl_free (tile->n_clones);
        }
      else
        {
          /* tile->n_clones and tile->data are unrelated, so free them
           * separately.
           */
          if (tile->data)
            {
              if (tile->destroy_notify == (void*)&free_data_directly)
                gegl_free (tile->data);
              else if (tile->destroy_notify)
                tile->destroy_notify (tile->destroy_notify_data);
            }

          g_slice_free (gint, tile->n_clones);
        }
    }

  g_slice_free (GeglTile, tile);
}

static GeglTile *
gegl_tile_new_bare_internal (void)
{
  GeglTile *tile     = g_slice_new0 (GeglTile);
  tile->ref_count    = 1;
  tile->tile_storage = NULL;
  tile->stored_rev   = 1;
  tile->rev          = 1;
  tile->lock         = 0;
  tile->data         = NULL;

  return tile;
}

GeglTile *
gegl_tile_new_bare (void)
{
  GeglTile *tile = gegl_tile_new_bare_internal ();

  tile->n_clones  = g_slice_new (gint);
  *tile->n_clones = 1;

  tile->destroy_notify = (void*)&free_data_directly;
  tile->destroy_notify_data = NULL;

  return tile;
}

GeglTile *
gegl_tile_dup (GeglTile *src)
{
  GeglTile *tile = gegl_tile_new_bare_internal ();

  tile->tile_storage = src->tile_storage;
  tile->data         = src->data;
  tile->size         = src->size;
  tile->is_zero_tile = src->is_zero_tile;
  tile->n_clones     = src->n_clones;

  tile->destroy_notify      = src->destroy_notify;
  tile->destroy_notify_data = src->destroy_notify_data;

  g_atomic_int_inc (tile->n_clones);

  return tile;
}

GeglTile *
gegl_tile_new (gint size)
{
  GeglTile *tile = gegl_tile_new_bare_internal ();

  /* allocate a single buffer for both tile->n_clones and tile->data */
  tile->n_clones  = gegl_malloc (INLINE_N_ELEMENTS_DATA_OFFSET + size);
  *tile->n_clones = 1;

  tile->data      = (guchar *) tile->n_clones + INLINE_N_ELEMENTS_DATA_OFFSET;
  tile->size      = size;

  tile->destroy_notify = (void*)&free_n_clones_directly;
  tile->destroy_notify_data = NULL;

  return tile;
}

static void
gegl_tile_unclone (GeglTile *tile)
{
  if (*tile->n_clones > 1)
    {
      /* the tile data is shared with other tiles,
       * create a local copy
       */
      if (tile->is_zero_tile)
        {
          if (g_atomic_int_dec_and_test (tile->n_clones))
            {
              /* someone else uncloned the tile in the meantime, and we're now
               * the last copy; bail.
               */
              *tile->n_clones = 1;
              return;
            }

          tile->n_clones     = gegl_calloc (INLINE_N_ELEMENTS_DATA_OFFSET +
                                            tile->size, 1);
          tile->is_zero_tile = 0;
        }
      else
        {
          guchar *buf;

          buf = gegl_malloc (INLINE_N_ELEMENTS_DATA_OFFSET + tile->size);
          memcpy (buf + INLINE_N_ELEMENTS_DATA_OFFSET, tile->data, tile->size);

          if (g_atomic_int_dec_and_test (tile->n_clones))
            {
              /* someone else uncloned the tile in the meantime, and we're now
               * the last copy; bail.
               */
              gegl_free (buf);
              *tile->n_clones = 1;
              return;
            }

          tile->n_clones = (gint *) buf;
        }

      *tile->n_clones = 1;
      tile->data      = (guchar *) tile->n_clones +
                        INLINE_N_ELEMENTS_DATA_OFFSET;

      tile->destroy_notify      = (void*)&free_n_clones_directly;
      tile->destroy_notify_data = NULL;
    }
}

void
gegl_tile_lock (GeglTile *tile)
{
  int slept = 0;
  while (! (g_atomic_int_compare_and_exchange (&tile->lock, 0, 1)))
  {
    if (slept++ == 1000)
    {
      g_warning ("blocking when trying to lock tile");
    }
    g_usleep (5);
  }

  gegl_tile_unclone (tile);
}

static void
_gegl_tile_void_pyramid (GeglTileSource *source,
                         gint            x,
                         gint            y,
                         gint            z)
{
  if (z > ((GeglTileStorage*)source)->seen_zoom)
    return;
  gegl_tile_source_void (source, x, y, z);
  _gegl_tile_void_pyramid (source, x/2, y/2, z+1);
}

static void
gegl_tile_void_pyramid (GeglTile *tile)
{
  if (tile->tile_storage &&
      tile->tile_storage->seen_zoom &&
      tile->z == 0) /* we only accepting voiding the base level */
    {
      _gegl_tile_void_pyramid (GEGL_TILE_SOURCE (tile->tile_storage),
                               tile->x/2,
                               tile->y/2,
                               tile->z+1);
      return;
    }
}

void
gegl_tile_unlock (GeglTile *tile)
{
  if (tile->unlock_notify != NULL)
    {
      tile->unlock_notify (tile, tile->unlock_notify_data);
    }

  if (tile->lock == 0)
    {
      g_warning ("unlocked a tile with lock count == 0");
    }
  else if (tile->lock==1)
  {
    if (tile->z == 0)
      {
        gegl_tile_void_pyramid (tile);
      }
      tile->rev++;
  }

  g_atomic_int_add (&tile->lock, -1);
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

void
gegl_tile_void (GeglTile *tile)
{
  g_rec_mutex_lock (&tile->tile_storage->mutex);
  gegl_tile_mark_as_stored (tile);

  if (tile->z == 0)
    gegl_tile_void_pyramid (tile);
  g_rec_mutex_unlock (&tile->tile_storage->mutex);
}

gboolean gegl_tile_store (GeglTile *tile)
{
  gboolean ret;
  if (gegl_tile_is_stored (tile))
    return TRUE;
  if (tile->tile_storage == NULL)
    return FALSE;
  g_rec_mutex_lock (&tile->tile_storage->mutex);
  if (gegl_tile_is_stored (tile))
  {
    g_rec_mutex_unlock (&tile->tile_storage->mutex);
    return FALSE;
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
