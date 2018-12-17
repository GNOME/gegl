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
 * Copyright 2006, 2007, 2008 Øyvind Kolås <pippin@gimp.org>
 *           2012, 2013 Ville Sokk <ville.sokk@gmail.com>
 */

#include "config.h"

#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <errno.h>

#include <glib-object.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#include "gegl-buffer.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer-backend.h"
#include "gegl-buffer-private.h"
#include "gegl-buffer-swap.h"
#include "gegl-compression.h"
#include "gegl-tile-backend.h"
#include "gegl-tile-backend-swap.h"
#include "gegl-debug.h"
#include "gegl-buffer-config.h"


#ifndef HAVE_FSYNC

#ifdef G_OS_WIN32
#define fsync _commit
#endif

#endif


/* maximal data size allowed to be pending in the swap queue at any given time,
 * as a factor of the maximal cache size.  when the amount of data in the queue
 * reaches this limit, attempting to push more data to the queue blocks until
 * the queued data size drops below the limit.
 */
#define QUEUED_MAX_RATIO 0.1

/* maximal tile-data compression ratio, above which we use the uncompressed
 * tile, to avoid decompression overhead.
 */
#define COMPRESSION_MAX_RATIO 0.95


G_DEFINE_TYPE (GeglTileBackendSwap, gegl_tile_backend_swap, GEGL_TYPE_TILE_BACKEND)

static GObjectClass * parent_class = NULL;


typedef enum
{
  OP_WRITE,
  OP_DESTROY,
} ThreadOp;

typedef struct
{
  gint                   ref_count;
  gint                   size;
  const GeglCompression *compression;
  GList                 *link;
  gint64                 offset;
} SwapBlock;

typedef struct
{
  gint       x;
  gint       y;
  gint       z;
  SwapBlock *block;
} SwapEntry;

typedef struct
{
  SwapBlock  *block;
  const Babl *format;
  GeglTile   *tile;
  gpointer    compressed;
  gint        size;
  gint        compressed_size;
  ThreadOp    operation;
} ThreadParams;

typedef struct
{
  gint64 start;
  gint64 end;
} SwapGap;


static void        gegl_tile_backend_swap_push_queue             (ThreadParams             *params,
                                                                  gboolean                  head);
static void        gegl_tile_backend_swap_resize                 (gint64                    size);
static SwapGap *   gegl_tile_backend_swap_gap_new                (gint64                    start,
                                                                  gint64                    end);
static gint64      gegl_tile_backend_swap_find_offset            (gint                      block_size);
static void        gegl_tile_backend_swap_free_block             (SwapBlock                *block);
static void        gegl_tile_backend_swap_free_data              (ThreadParams             *params);
static void        gegl_tile_backend_swap_write                  (ThreadParams             *params);
static void        gegl_tile_backend_swap_destroy                (ThreadParams             *params);
static gpointer    gegl_tile_backend_swap_writer_thread          (gpointer ignored);
static GeglTile   *gegl_tile_backend_swap_entry_read             (GeglTileBackendSwap      *self,
                                                                  SwapEntry                *entry);
static void        gegl_tile_backend_swap_entry_write            (GeglTileBackendSwap      *self,
                                                                  SwapEntry                *entry,
                                                                  GeglTile                 *tile);
static SwapBlock * gegl_tile_backend_swap_block_create           (GeglTileBackendSwap      *self);
static void        gegl_tile_backend_swap_block_free             (SwapBlock                *block);
static SwapBlock * gegl_tile_backend_swap_block_ref              (GeglTileBackendSwap      *self,
                                                                  SwapBlock                *block);
static void        gegl_tile_backend_swap_block_unref            (GeglTileBackendSwap      *self,
                                                                  SwapBlock                *block,
                                                                  gboolean                  lock);
static gboolean    gegl_tile_backend_swap_block_is_unique        (GeglTileBackendSwap      *self,
                                                                  SwapBlock                *block);
static SwapEntry * gegl_tile_backend_swap_entry_create           (GeglTileBackendSwap      *self,
                                                                  gint                      x,
                                                                  gint                      y,
                                                                  gint                      z,
                                                                  SwapBlock                *block);
static void        gegl_tile_backend_swap_entry_destroy          (GeglTileBackendSwap      *self,
                                                                  SwapEntry                *entry,
                                                                  gboolean                  lock);
static SwapEntry * gegl_tile_backend_swap_lookup_entry           (GeglTileBackendSwap      *self,
                                                                  gint                      x,
                                                                  gint                      y,
                                                                  gint                      z);
static GeglTile *  gegl_tile_backend_swap_get_tile               (GeglTileSource           *self,
                                                                  gint                      x,
                                                                  gint                      y,
                                                                  gint                      z);
static gpointer    gegl_tile_backend_swap_set_tile               (GeglTileSource           *self,
                                                                  GeglTile                 *tile,
                                                                  gint                      x,
                                                                  gint                      y,
                                                                  gint                      z);
static gpointer    gegl_tile_backend_swap_void_tile              (GeglTileSource           *self,
                                                                  gint                      x,
                                                                  gint                      y,
                                                                  gint                      z);
static gpointer    gegl_tile_backend_swap_exist_tile             (GeglTileSource           *self,
                                                                  GeglTile                 *tile,
                                                                  gint                      x,
                                                                  gint                      y,
                                                                  gint                      z);
static gpointer    gegl_tile_backend_swap_copy_tile              (GeglTileSource           *self,
                                                                  gint                      x,
                                                                  gint                      y,
                                                                  gint                      z,
                                                                  const GeglTileCopyParams *params);
static gpointer    gegl_tile_backend_swap_command                (GeglTileSource           *self,
                                                                  GeglTileCommand           command,
                                                                  gint                      x,
                                                                  gint                      y,
                                                                  gint                      z,
                                                                  gpointer                  data);
static guint       gegl_tile_backend_swap_hashfunc               (gconstpointer             key);
static gboolean    gegl_tile_backend_swap_equalfunc              (gconstpointer             a,
                                                                  gconstpointer             b);
static void        gegl_tile_backend_swap_constructed            (GObject                  *object);
static void        gegl_tile_backend_swap_finalize               (GObject                  *object);
static void        gegl_tile_backend_swap_ensure_exist           (void);
static void        gegl_tile_backend_swap_class_init             (GeglTileBackendSwapClass *klass);
static void        gegl_tile_backend_swap_tile_cache_size_notify (GObject                  *config,
                                                                  GParamSpec               *pspec,
                                                                  gpointer                  data);
static void        gegl_tile_backend_swap_init                   (GeglTileBackendSwap      *self);
void               gegl_tile_backend_swap_cleanup                (void);


static gchar                 *path               = NULL;
static const GeglCompression *compression        = NULL;
static gint                   in_fd              = -1;
static gint                   out_fd             = -1;
static gint64                 in_offset          = 0;
static gint64                 out_offset         = 0;
static GList                 *gap_list           = NULL;
static gint64                 file_size          = 0;
static gint64                 total              = 0;
static guintptr               total_uncompressed = 0;
static gboolean               busy               = FALSE;
static gboolean               reading            = FALSE;
static gint64                 read_total         = 0;
static gboolean               writing            = FALSE;
static gint64                 write_total        = 0;
static gint64                 queued_total       = 0;
static gint64                 queued_cost        = 0;
static gint64                 queued_max         = 0;
static gint                   queue_stalls       = 0;

static GThread      *writer_thread           = NULL;
static GQueue       *queue                   = NULL;
static ThreadParams *in_progress             = NULL;
static gboolean      exit_thread             = FALSE;
static gpointer      compression_buffer      = NULL;
static gint          compression_buffer_size = 0;
static GMutex        read_mutex;
static GMutex        queue_mutex;
static GCond         queue_cond;
static GCond         push_cond;


static void
gegl_tile_backend_swap_push_queue (ThreadParams *params,
                                   gboolean      head)
{
  if (params->tile || params->compressed)
    {
      if (params->tile)
        params->block->compression = compression;

      if (queued_cost > queued_max)
        {
          queue_stalls++;

          if (params->tile && compression)
            {
              gint     bpp = babl_format_get_bytes_per_pixel (params->format);
              gpointer compressed;
              gint     max_compressed_size;
              gint     compressed_size;

              g_mutex_unlock (&queue_mutex);

              max_compressed_size = params->size * COMPRESSION_MAX_RATIO;
              compressed          = g_malloc (max_compressed_size);

              if (gegl_compression_compress (compression, params->format,
                                             gegl_tile_get_data (params->tile),
                                             params->size / bpp,
                                             compressed, &compressed_size,
                                             max_compressed_size))
                {
                  gegl_tile_unref (params->tile);
                  params->tile = NULL;

                  params->compressed      = compressed;
                  params->compressed_size = compressed_size;
                }
              else
                {
                  params->block->compression = NULL;

                  g_free (compressed);
                }

              g_mutex_lock (&queue_mutex);
            }

          while (queued_cost > queued_max)
            g_cond_wait (&push_cond, &queue_mutex);
        }

      if (params->tile)
        queued_total += params->size;
      else
        queued_total += params->compressed_size;

      queued_cost  += params->compressed_size;
    }

  busy = TRUE;

  if (head)
    g_queue_push_head (queue, params);
  else
    g_queue_push_tail (queue, params);

  if (params->block)
    params->block->link = g_queue_peek_tail_link (queue);

  /* wake up the writer thread */
  g_cond_signal (&queue_cond);
}

static void
gegl_tile_backend_swap_resize (gint64 size)
{
  file_size = size;

  if (ftruncate (out_fd, file_size) != 0)
    {
      g_warning ("failed to resize swap file: %s", g_strerror (errno));
      return;
    }

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "resized swap to %i", (gint)file_size);
}

static SwapGap *
gegl_tile_backend_swap_gap_new (gint64 start,
                                gint64 end)
{
  SwapGap *gap = g_slice_new (SwapGap);

  gap->start = start;
  gap->end = end;

  return gap;
}

static gint64
gegl_tile_backend_swap_find_offset (gint block_size)
{
  SwapGap *gap;
  gint64   offset;

  total += block_size;

  if (gap_list)
    {
      GList *link = gap_list;

      while (link)
        {
          gint64 length;

          gap    = link->data;
          length = gap->end - gap->start;

          if (length > block_size)
            {
              offset = gap->start;
              gap->start += block_size;

              return offset;
            }
          else if (length == block_size)
            {
              offset = gap->start;
              g_slice_free (SwapGap, gap);
              gap_list = g_list_delete_link (gap_list, link);

              return offset;
            }

          link = link->next;
        }
    }

  offset = file_size;

  gegl_tile_backend_swap_resize (file_size + 32 * block_size);

  gap = gegl_tile_backend_swap_gap_new (offset + block_size, file_size);
  gap_list = g_list_append (gap_list, gap);

  return offset;
}

static void
gegl_tile_backend_swap_free_block (SwapBlock *block)
{
  gint64  start, end;
  GList  *hlink;

  /* storage for entry not allocated yet.  nothing more to do. */
  if (block->offset < 0)
    return;

  start = block->offset;
  end   = start + block->size;

  block->offset = -1;

  if ((hlink = gap_list))
    while (hlink)
      {
        GList *llink = hlink->prev;
        SwapGap *lgap, *hgap;

        if (llink)
          lgap = llink->data;

        hgap = hlink->data;

        /* attempt to merge lower, upper and this gap */
        if (llink != NULL && lgap->end == start &&
            hgap->start == end)
          {
            llink->next = hlink->next;
            if (hlink->next)
              hlink->next->prev = llink;
            lgap->end = hgap->end;

            g_slice_free (SwapGap, hgap);
            hlink->next = NULL;
            hlink->prev = NULL;
            g_list_free (hlink);
            break;
          }
        /* attempt to merge with upper gap */
        else if (hgap->start == end)
          {
            hgap->start = start;
            break;
          }
        /* attempt to merge with lower gap */
        else if (llink != NULL && lgap->end == start)
          {
            lgap->end = end;
            break;
          }
        /* create new gap */
        else if (hgap->start > end)
          {
            lgap = gegl_tile_backend_swap_gap_new (start, end);
            gap_list = g_list_insert_before (gap_list, hlink, lgap);
            break;
          }

        /* if there's no more elements in the list after this */
        else if (hlink->next == NULL)
          {
            /* attempt to merge with the last gap */
            if (hgap->end == start)
              {
                hgap->end = end;
              }
            /* create a new gap in the end of the list */
            else
              {
                GList *new_link;
                hgap = gegl_tile_backend_swap_gap_new (start, end);
                new_link = g_list_alloc ();
                new_link->next = NULL;
                new_link->prev = hlink;
                new_link->data = hgap;
                hlink->next = new_link;
              }
            break;
          }

        hlink = hlink->next;
      }
  else
    gap_list = g_list_prepend (NULL,
                               gegl_tile_backend_swap_gap_new (start, end));

  total -= end - start;
}

static void
gegl_tile_backend_swap_free_data (ThreadParams *params)
{
  if (params->tile || params->compressed)
    {
      if (params->tile)
        {
          queued_total -= params->size;

          gegl_tile_unref (params->tile);
          params->tile = NULL;
        }
      else
        {
          queued_total -= params->compressed_size;

          g_free (params->compressed);
          params->compressed = NULL;
        }

      queued_cost -= params->compressed_size;

      if (queued_cost <= queued_max &&
          queued_cost + params->compressed_size > queued_max)
        {
          g_cond_broadcast (&push_cond);
        }
    }
}

static void
gegl_tile_backend_swap_write (ThreadParams *params)
{
  const guint8 *data;
  gint64        offset = params->block->offset;
  gint          to_be_written;

  gegl_tile_backend_swap_ensure_exist ();

  if (params->tile)
    {
      data          = gegl_tile_get_data (params->tile);
      to_be_written = params->size;

      if (params->block->compression)
        {
          gint bpp = babl_format_get_bytes_per_pixel (params->format);
          gint compressed_size;
          gint max_compressed_size;

          max_compressed_size = params->size * COMPRESSION_MAX_RATIO;

          if (max_compressed_size > compression_buffer_size)
            {
              compression_buffer      = g_realloc (compression_buffer,
                                                   max_compressed_size);
              compression_buffer_size = max_compressed_size;
            }

          if (gegl_compression_compress (params->block->compression,
                                         params->format,
                                         data, params->size / bpp,
                                         compression_buffer, &compressed_size,
                                         max_compressed_size))
            {
              data          = compression_buffer;
              to_be_written = compressed_size;
            }
          else
            {
              params->block->compression = NULL;
            }
        }
    }
  else
    {
      data          = params->compressed;
      to_be_written = params->compressed_size;
    }

  if (offset >= 0 && params->block->size != to_be_written)
    {
      g_atomic_pointer_add (&total_uncompressed, -params->size);

      gegl_tile_backend_swap_free_block (params->block);

      offset = -1;
    }

  if (offset < 0)
    {
      /* storage for entry not allocated yet.  allocate now. */
      offset = gegl_tile_backend_swap_find_offset (to_be_written);

      params->block->offset = offset;
      params->block->size   = to_be_written;

      g_atomic_pointer_add (&total_uncompressed, +params->size);
    }

  if (out_offset != offset)
    {
      if (lseek (out_fd, offset, SEEK_SET) < 0)
        {
          g_warning ("unable to seek to tile in buffer: %s", g_strerror (errno));
          return;
        }
      out_offset = offset;
    }

  writing = TRUE;

  while (to_be_written > 0)
    {
      gint wrote;
      wrote = write (out_fd, data, to_be_written);
      if (wrote <= 0)
        {
          g_message ("unable to write tile data to self: "
                     "%s (%d/%d bytes written)",
                     g_strerror (errno), wrote, to_be_written);
          break;
        }

      data          += wrote;
      to_be_written -= wrote;
      out_offset    += wrote;

      write_total   += wrote;
    }

  writing = FALSE;

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "writer thread wrote at %i", (gint)offset);
}

static void
gegl_tile_backend_swap_destroy (ThreadParams *params)
{
  if (params->block->offset >= 0)
    g_atomic_pointer_add (&total_uncompressed, -params->size);

  gegl_tile_backend_swap_free_block (params->block);

  gegl_tile_backend_swap_block_free (params->block);
}

static gpointer
gegl_tile_backend_swap_writer_thread (gpointer ignored)
{
  g_mutex_lock (&queue_mutex);

  while (TRUE)
    {
      ThreadParams *params;

      while (g_queue_is_empty (queue) && !exit_thread)
        {
          busy = FALSE;

          g_cond_wait (&queue_cond, &queue_mutex);
        }

      if (exit_thread)
        break;

      params = (ThreadParams *)g_queue_pop_head (queue);
      if (params->block)
        {
          in_progress = params;
          params->block->link = NULL;
        }

      g_mutex_unlock (&queue_mutex);

      switch (params->operation)
        {
        case OP_WRITE:
          gegl_tile_backend_swap_write (params);
          break;
        case OP_DESTROY:
          gegl_tile_backend_swap_destroy (params);
          break;
        }

      g_mutex_lock (&queue_mutex);

      in_progress = NULL;

      gegl_tile_backend_swap_free_data (params);

      g_slice_free (ThreadParams, params);
    }

  g_mutex_unlock (&queue_mutex);
  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "exiting writer thread");
  return NULL;
}

static GeglTile *
gegl_tile_backend_swap_entry_read (GeglTileBackendSwap *self,
                                   SwapEntry           *entry)
{
  GeglTileBackend *backend = GEGL_TILE_BACKEND (self);
  const Babl      *format;
  GeglTile        *tile;
  guint8          *data;
  guint8          *dest;
  gint64           offset;
  gint             tile_size;
  gint             bpp;
  gint             to_be_read;

  format    = gegl_tile_backend_get_format (backend);
  tile_size = gegl_tile_backend_get_tile_size (backend);
  bpp       = babl_format_get_bytes_per_pixel (format);

  g_mutex_lock (&queue_mutex);

  if (entry->block->link || in_progress)
    {
      ThreadParams *queued_op = NULL;

      if (entry->block->link)
        queued_op = entry->block->link->data;
      else if (in_progress->block == entry->block)
        queued_op = in_progress;

      if (queued_op)
        {
          if (queued_op->tile)
            {
              tile = gegl_tile_dup (queued_op->tile);
            }
          else
            {
              tile = gegl_tile_new (tile_size);
              dest = gegl_tile_get_data (tile);

              if (! gegl_compression_decompress (
                      entry->block->compression, format,
                      dest, tile_size / bpp,
                      queued_op->compressed, queued_op->compressed_size))
                {
                  g_warning ("failed to decompress tile");
                }
            }

          g_mutex_unlock (&queue_mutex);

          gegl_tile_mark_as_stored (tile);

          GEGL_NOTE(GEGL_DEBUG_TILE_BACKEND, "read entry %i, %i, %i from queue", entry->x, entry->y, entry->z);

          return tile;
        }
    }

  offset = entry->block->offset;

  g_mutex_unlock (&queue_mutex);

  if (offset < 0 || in_fd < 0)
    {
      g_warning ("no swap storage allocated for tile");
      return NULL;
    }

  tile = gegl_tile_new (tile_size);
  dest = gegl_tile_get_data (tile);
  gegl_tile_mark_as_stored (tile);

  if (entry->block->compression)
    data = g_malloc (entry->block->size);
  else
    data = dest;

  g_mutex_lock (&read_mutex);

  if (in_offset != offset)
    {
      if (lseek (in_fd, offset, SEEK_SET) < 0)
        {
          g_mutex_unlock (&read_mutex);

          g_warning ("unable to seek to tile in buffer: %s", g_strerror (errno));
          return tile;
        }
      in_offset = offset;
    }

  reading = TRUE;

  to_be_read = entry->block->size;

  while (to_be_read > 0)
    {
      GError *error = NULL;
      gint    bytes_read;

      bytes_read = read (in_fd,
                         data + entry->block->size - to_be_read, to_be_read);

      if (bytes_read <= 0)
        {
          reading = FALSE;

          g_mutex_unlock (&read_mutex);

          if (entry->block->compression)
            g_free (data);

          g_message ("unable to read tile data from swap: "
                     "%s (%d/%d bytes read) %s",
                     g_strerror (errno), bytes_read, to_be_read, error?error->message:"--");
          return tile;
        }

      to_be_read -= bytes_read;
      in_offset  += bytes_read;

      read_total += bytes_read;
    }

  reading = FALSE;

  g_mutex_unlock (&read_mutex);

  if (entry->block->compression)
    {
      if (! gegl_compression_decompress (
              entry->block->compression, format,
              dest, tile_size / bpp,
              data, entry->block->size))
        {
          g_warning ("failed to decompress tile");
        }

      g_free (data);
    }

  GEGL_NOTE(GEGL_DEBUG_TILE_BACKEND, "read entry %i, %i, %i from %i", entry->x, entry->y, entry->z, (gint)offset);

  return tile;
}

static void
gegl_tile_backend_swap_entry_write (GeglTileBackendSwap *self,
                                    SwapEntry           *entry,
                                    GeglTile            *tile)
{
  GeglTileBackend *backend = GEGL_TILE_BACKEND (self);
  ThreadParams    *params;
  gint             n_clones;
  gint             size;
  gint             cost;

  n_clones = *gegl_tile_n_clones (tile);
  size     = gegl_tile_backend_get_tile_size (backend);
  cost     = (size + n_clones / 2) / n_clones;

  g_mutex_lock (&queue_mutex);

  if (entry->block->link)
    {
      params = entry->block->link->data;

      g_assert (params->operation == OP_WRITE);

      gegl_tile_backend_swap_free_data (params);

      if (queued_cost <= queued_max)
        {
          params->block->compression = compression;

          params->tile            = gegl_tile_dup (tile);
          params->compressed_size = cost;

          queued_total += size;
          queued_cost  += cost;

          g_mutex_unlock (&queue_mutex);

          GEGL_NOTE(GEGL_DEBUG_TILE_BACKEND, "tile %i, %i, %i at %i is already enqueued, changed data", entry->x, entry->y, entry->z, (gint)entry->block->offset);

          return;
        }

      g_queue_delete_link (queue, entry->block->link);
      entry->block->link = NULL;
    }
  else
    {
      params = g_slice_new0 (ThreadParams);
    }

  params->operation       = OP_WRITE;
  params->block           = entry->block;
  params->format          = gegl_tile_backend_get_format (backend);
  params->tile            = gegl_tile_dup (tile);
  params->compressed      = NULL;
  params->size            = size;
  params->compressed_size = cost;

  gegl_tile_backend_swap_push_queue (params, /* head = */ FALSE);

  g_mutex_unlock (&queue_mutex);

  GEGL_NOTE(GEGL_DEBUG_TILE_BACKEND, "pushed write of entry %i, %i, %i at %i", entry->x, entry->y, entry->z, (gint)entry->block->offset);
}

static SwapBlock *
gegl_tile_backend_swap_block_create (GeglTileBackendSwap *self)
{
  SwapBlock *block = g_slice_new (SwapBlock);

  block->ref_count = 1;
  block->link      = NULL;
  block->offset    = -1;

  return block;
}

static void
gegl_tile_backend_swap_block_free (SwapBlock *block)
{
  g_return_if_fail (block->ref_count == 0);

  g_slice_free (SwapBlock, block);
}

static SwapBlock *
gegl_tile_backend_swap_block_ref (GeglTileBackendSwap *self,
                                  SwapBlock           *block)
{
  g_atomic_int_inc (&block->ref_count);

  g_atomic_pointer_add (
    &total_uncompressed,
    +gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self)));

  return block;
}

static void
gegl_tile_backend_swap_block_unref (GeglTileBackendSwap *self,
                                    SwapBlock           *block,
                                    gboolean             lock)
{
  if (g_atomic_int_dec_and_test (&block->ref_count))
    {
      if (lock)
        g_mutex_lock (&queue_mutex);

      if (block->link)
        {
          GList        *link      = block->link;
          ThreadParams *queued_op = link->data;

          gegl_tile_backend_swap_free_data (queued_op);

          /* reuse the queued op, changing it to an OP_DESTROY. */
          queued_op->operation = OP_DESTROY;

          /* move to op to the top of the queue, so that it gets served before
           * any write ops, which are then free to reuse the reclaimed space.
           */
          g_queue_unlink (queue, link);
          g_queue_push_head_link (queue, link);
        }
      else
        {
          ThreadParams *params;

          params            = g_slice_new0 (ThreadParams);
          params->operation = OP_DESTROY;
          params->block     = block;
          params->size      = gegl_tile_backend_get_tile_size (
                                GEGL_TILE_BACKEND (self));

          /* push the destroy op at the top of the queue, so that it gets
           * served before any write ops, which are then free to reuse the
           * reclaimed space.
           */
          gegl_tile_backend_swap_push_queue (params, /* head = */ TRUE);
        }

      if (lock)
        g_mutex_unlock (&queue_mutex);
    }
  else
    {
      g_atomic_pointer_add (
        &total_uncompressed,
        -gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self)));
    }
}

static gboolean
gegl_tile_backend_swap_block_is_unique (GeglTileBackendSwap *self,
                                        SwapBlock           *block)
{
  return g_atomic_int_get (&block->ref_count) == 1;
}

static SwapEntry *
gegl_tile_backend_swap_entry_create (GeglTileBackendSwap *self,
                                     gint                 x,
                                     gint                 y,
                                     gint                 z,
                                     SwapBlock           *block)
{
  SwapEntry *entry = g_slice_new (SwapEntry);

  if (block)
    gegl_tile_backend_swap_block_ref (self, block);
  else
    block = gegl_tile_backend_swap_block_create (self);

  entry->x     = x;
  entry->y     = y;
  entry->z     = z;
  entry->block = block;

  return entry;
}

static void
gegl_tile_backend_swap_entry_destroy (GeglTileBackendSwap *self,
                                      SwapEntry           *entry,
                                      gboolean             lock)
{
  gegl_tile_backend_swap_block_unref (self, entry->block, lock);

  g_slice_free (SwapEntry, entry);
}

static SwapEntry *
gegl_tile_backend_swap_lookup_entry (GeglTileBackendSwap *self,
                                     gint                 x,
                                     gint                 y,
                                     gint                 z)
{
  SwapEntry key = {x, y, z};

  return g_hash_table_lookup (self->index, &key);
}

static GeglTile *
gegl_tile_backend_swap_get_tile (GeglTileSource *self,
                                 gint            x,
                                 gint            y,
                                 gint            z)
{
  GeglTileBackendSwap *swap;
  SwapEntry           *entry;

  swap  = GEGL_TILE_BACKEND_SWAP (self);
  entry = gegl_tile_backend_swap_lookup_entry (swap, x, y, z);

  if (! entry)
    return NULL;

  return gegl_tile_backend_swap_entry_read (swap, entry);
}

static gpointer
gegl_tile_backend_swap_set_tile (GeglTileSource *self,
                                 GeglTile       *tile,
                                 gint            x,
                                 gint            y,
                                 gint            z)
{
  GeglTileBackendSwap *swap;
  SwapEntry           *entry;

  if (tile->is_zero_tile)
    {
      /* the tile is empty.  avoid an expensive write to disk, and just drop
       * the existing tile (if there is one); the empty tile handler will serve
       * a new empty tile upon request.
       */
      gegl_tile_backend_swap_void_tile (self, x, y, z);

      gegl_tile_mark_as_stored (tile);

      return GINT_TO_POINTER (TRUE);
    }

  swap  = GEGL_TILE_BACKEND_SWAP (self);
  entry = gegl_tile_backend_swap_lookup_entry (swap, x, y, z);

  if (entry)
    {
      if (! gegl_tile_backend_swap_block_is_unique (swap, entry->block))
        {
          gegl_tile_backend_swap_block_unref (swap, entry->block, TRUE);
          entry->block = gegl_tile_backend_swap_block_create (swap);
        }
    }
  else
    {
      entry = gegl_tile_backend_swap_entry_create (swap, x, y, z, NULL);
      g_hash_table_insert (swap->index, entry, entry);
    }

  gegl_tile_backend_swap_entry_write (swap, entry, tile);

  gegl_tile_mark_as_stored (tile);

  return GINT_TO_POINTER (TRUE);
}

static gpointer
gegl_tile_backend_swap_void_tile (GeglTileSource *self,
                                  gint            x,
                                  gint            y,
                                  gint            z)
{
  GeglTileBackendSwap *swap;
  SwapEntry           *entry;

  swap  = GEGL_TILE_BACKEND_SWAP (self);
  entry = gegl_tile_backend_swap_lookup_entry (swap, x, y, z);

  if (entry != NULL)
    {
      GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "void tile %i, %i, %i", x, y, z);

      g_hash_table_remove (swap->index, entry);
      gegl_tile_backend_swap_entry_destroy (swap, entry, TRUE);
    }

  return NULL;
}

static gpointer
gegl_tile_backend_swap_exist_tile (GeglTileSource *self,
                                   GeglTile       *tile,
                                   gint            x,
                                   gint            y,
                                   gint            z)
{
  GeglTileBackendSwap *swap;
  SwapEntry           *entry;

  swap  = GEGL_TILE_BACKEND_SWAP (self);
  entry = gegl_tile_backend_swap_lookup_entry (swap, x, y, z);

  return GINT_TO_POINTER (entry != NULL);
}

static gpointer
gegl_tile_backend_swap_copy_tile (GeglTileSource           *self,
                                  gint                      x,
                                  gint                      y,
                                  gint                      z,
                                  const GeglTileCopyParams *params)
{
  GeglTileBackendSwap *swap;
  GeglTileBackendSwap *dst_swap;
  SwapEntry           *entry;
  SwapEntry           *dst_entry;

  if (params->dst_buffer &&
      ! GEGL_IS_TILE_BACKEND_SWAP (params->dst_buffer->backend))
    {
      return GINT_TO_POINTER (FALSE);
    }

  swap  = GEGL_TILE_BACKEND_SWAP (self);
  entry = gegl_tile_backend_swap_lookup_entry (swap, x, y, z);

  if (! entry)
    return GINT_TO_POINTER (FALSE);

  if (params->dst_buffer)
    dst_swap = GEGL_TILE_BACKEND_SWAP (params->dst_buffer->backend);
  else
    dst_swap = swap;
  dst_entry  = gegl_tile_backend_swap_lookup_entry (dst_swap,
                                                    params->dst_x,
                                                    params->dst_y,
                                                    params->dst_z);

  if (dst_entry)
    {
      if (dst_entry->block == entry->block)
        {
          return GINT_TO_POINTER (TRUE);
        }
      else
        {
          gegl_tile_backend_swap_block_unref (dst_swap, dst_entry->block, TRUE);
          dst_entry->block = gegl_tile_backend_swap_block_ref (dst_swap,
                                                               entry->block);
        }
    }
  else
    {
      dst_entry = gegl_tile_backend_swap_entry_create (dst_swap,
                                                       params->dst_x,
                                                       params->dst_y,
                                                       params->dst_z,
                                                       entry->block);
      g_hash_table_insert (dst_swap->index, dst_entry, dst_entry);
    }

  return GINT_TO_POINTER (TRUE);
}

static gpointer
gegl_tile_backend_swap_command (GeglTileSource  *self,
                                GeglTileCommand  command,
                                gint             x,
                                gint             y,
                                gint             z,
                                gpointer         data)
{
  switch (command)
    {
      case GEGL_TILE_GET:
        return gegl_tile_backend_swap_get_tile (self, x, y, z);
      case GEGL_TILE_SET:
        return gegl_tile_backend_swap_set_tile (self, data, x, y, z);
      case GEGL_TILE_IDLE:
        return NULL;
      case GEGL_TILE_VOID:
        return gegl_tile_backend_swap_void_tile (self, x, y, z);
      case GEGL_TILE_EXIST:
        return gegl_tile_backend_swap_exist_tile (self, data, x, y, z);
      case GEGL_TILE_FLUSH:
        return NULL;
      case GEGL_TILE_COPY:
        return gegl_tile_backend_swap_copy_tile (self, x, y, z, data);

      default:
        break;
    }

  return gegl_tile_backend_command (GEGL_TILE_BACKEND (self),
                                    command, x, y, z, data);
}

static guint
gegl_tile_backend_swap_hashfunc (gconstpointer key)
{
  const SwapEntry *entry = key;
  guint            hash;
  gint             i;
  gint             srcA  = entry->x;
  gint             srcB  = entry->y;
  gint             srcC  = entry->z;

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
gegl_tile_backend_swap_equalfunc (gconstpointer a,
                                  gconstpointer b)
{
  const SwapEntry *ea = a;
  const SwapEntry *eb = b;

  if (ea->x == eb->x &&
      ea->y == eb->y &&
      ea->z == eb->z)
    return TRUE;

  return FALSE;
}

static void
gegl_tile_backend_swap_constructed (GObject *object)
{
  GeglTileBackend *backend = GEGL_TILE_BACKEND (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gegl_tile_backend_set_flush_on_destroy (backend, FALSE);

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "constructing swap backend");
}

static void
gegl_tile_backend_swap_finalize (GObject *object)
{
  GeglTileBackendSwap *self = GEGL_TILE_BACKEND_SWAP (object);

  if (self->index)
    {
      if (g_hash_table_size (self->index) > 0)
        {
          GHashTableIter iter;
          gpointer       key;
          gpointer       value;

          g_hash_table_iter_init (&iter, self->index);

          g_mutex_lock (&queue_mutex);

          while (g_hash_table_iter_next (&iter, &key, &value))
            gegl_tile_backend_swap_entry_destroy (self, value, FALSE);

          g_mutex_unlock (&queue_mutex);
        }

      g_hash_table_unref (self->index);

      self->index = NULL;
    }

  (*G_OBJECT_CLASS (parent_class)->finalize)(object);
}

static void
gegl_tile_backend_swap_ensure_exist (void)
{
  if (in_fd == -1 || out_fd == -1)
    {
      path = gegl_buffer_swap_create_file ("shared");

      if (! path)
        {
          g_warning ("using swap backend, but swap is disabled");

          return;
        }

      GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "creating swapfile %s", path);

      out_fd = g_open (path, O_RDWR|O_CREAT, 0770);
      in_fd = g_open (path, O_RDONLY, 0);

      if (out_fd == -1 || in_fd == -1)
        {
          g_warning ("could not open swap file '%s': %s",
                     path, g_strerror (errno));

          gegl_buffer_swap_remove_file (path);
          g_clear_pointer (&path, g_free);
        }
    }
}

static void
gegl_tile_backend_swap_compression_notify (GObject    *config,
                                           GParamSpec *pspec,
                                           gpointer    data)
{
  gchar *swap_compression;

  g_mutex_lock (&queue_mutex);

  g_object_get (config,
                "swap-compression", &swap_compression,
                NULL);

  compression = gegl_compression (swap_compression);

  g_free (swap_compression);

  g_mutex_unlock (&queue_mutex);
}

static void
gegl_tile_backend_swap_tile_cache_size_notify (GObject    *config,
                                               GParamSpec *pspec,
                                               gpointer    data)
{
  g_mutex_lock (&queue_mutex);

  g_object_get (config,
                "tile-cache-size", &queued_max,
                NULL);

  queued_max *= QUEUED_MAX_RATIO;

  g_cond_broadcast (&push_cond);

  g_mutex_unlock (&queue_mutex);
}

static void
gegl_tile_backend_swap_class_init (GeglTileBackendSwapClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->constructed  = gegl_tile_backend_swap_constructed;
  gobject_class->finalize     = gegl_tile_backend_swap_finalize;

  queue         = g_queue_new ();
  writer_thread = g_thread_new ("swap writer",
                                gegl_tile_backend_swap_writer_thread,
                                NULL);

  g_signal_connect (gegl_buffer_config (), "notify::swap-compression",
                    G_CALLBACK (gegl_tile_backend_swap_compression_notify),
                    NULL);

  gegl_tile_backend_swap_compression_notify (G_OBJECT (gegl_buffer_config ()),
                                             NULL, NULL);

  g_signal_connect (gegl_buffer_config (), "notify::tile-cache-size",
                    G_CALLBACK (gegl_tile_backend_swap_tile_cache_size_notify),
                    NULL);

  gegl_tile_backend_swap_tile_cache_size_notify (G_OBJECT (gegl_buffer_config ()),
                                                 NULL, NULL);
}

void
gegl_tile_backend_swap_cleanup (void)
{
  if (! writer_thread)
    return;

  g_signal_handlers_disconnect_by_func (
    gegl_buffer_config (),
    gegl_tile_backend_swap_tile_cache_size_notify,
    NULL);

  g_signal_handlers_disconnect_by_func (
    gegl_buffer_config (),
    gegl_tile_backend_swap_compression_notify,
    NULL);

  g_mutex_lock (&queue_mutex);
  exit_thread = TRUE;
  g_cond_signal (&queue_cond);
  g_mutex_unlock (&queue_mutex);
  g_thread_join (writer_thread);
  writer_thread = NULL;

  if (g_queue_get_length (queue) != 0)
    g_warning ("tile-backend-swap writer queue wasn't empty before freeing\n");

  g_queue_free (queue);
  queue = NULL;

  g_clear_pointer (&compression_buffer, g_free);
  compression_buffer_size = 0;

  if (gap_list)
    {
      SwapGap *gap = gap_list->data;

      if (gap_list->next)
        g_warning ("tile-backend-swap gap list had more than one element\n");

      g_warn_if_fail (gap->start == 0 && gap->end == file_size);

      g_slice_free (SwapGap, gap_list->data);
      g_list_free (gap_list);

      gap_list = NULL;
    }
  else
    {
      g_warn_if_fail (file_size == 0);
    }

  if (in_fd != -1)
    {
      close (in_fd);
      in_fd = -1;
    }

  if (out_fd != -1)
    {
      close (out_fd);
      out_fd = -1;
    }

  if (path)
    {
      gegl_buffer_swap_remove_file (path);
      g_clear_pointer (&path, g_free);
    }
}

static void
gegl_tile_backend_swap_init (GeglTileBackendSwap *self)
{
  ((GeglTileSource*)self)->command = gegl_tile_backend_swap_command;

  self->index = g_hash_table_new (gegl_tile_backend_swap_hashfunc,
                                  gegl_tile_backend_swap_equalfunc);
}

/* the following functions could theoretically return slightly wrong values on
 * 32-bit platforms, due to non-atomic 64-bit access, but since it's not too
 * important for these functions to be completely accurate at all times, it
 * should be ok.
 */

guint64
gegl_tile_backend_swap_get_total (void)
{
  return total;
}

guint64
gegl_tile_backend_swap_get_total_uncompressed (void)
{
  return total_uncompressed;
}

guint64
gegl_tile_backend_swap_get_file_size (void)
{
  return file_size;
}

gboolean
gegl_tile_backend_swap_get_busy (void)
{
  return busy;
}

guint64
gegl_tile_backend_swap_get_queued_total (void)
{
  return queued_total;
}

gboolean
gegl_tile_backend_swap_get_queue_full (void)
{
  return queued_cost > queued_max;
}

gint
gegl_tile_backend_swap_get_queue_stalls (void)
{
  return queue_stalls;
}

gboolean
gegl_tile_backend_swap_get_reading (void)
{
  return reading;
}

guint64
gegl_tile_backend_swap_get_read_total (void)
{
  return read_total;
}

gboolean
gegl_tile_backend_swap_get_writing (void)
{
  return writing;
}

guint64
gegl_tile_backend_swap_get_write_total (void)
{
  return write_total;
}

void
gegl_tile_backend_swap_reset_stats (void)
{
  read_total  = 0;
  write_total = 0;

  queue_stalls = 0;
}
