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
 * Copyright 2006, 2007, 2008 Øyvind Kolås <pippin@gimp.org>
 *           2012 Ville Sokk <ville.sokk@gmail.com>
 */

#include "config.h"

#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <errno.h>

#ifdef G_OS_WIN32
#include <process.h>
#define getpid() _getpid()
#endif

#include <glib-object.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

#include "gegl.h"
#include "gegl-buffer-backend.h"
#include "gegl-buffer-types.h"
#include "gegl-tile-backend.h"
#include "gegl-tile-backend-file.h"
#include "gegl-aio-file.h"
#include "gegl-debug.h"
#include "gegl-config.h"


#ifndef HAVE_FSYNC

#ifdef G_OS_WIN32
#define fsync _commit
#endif

#endif


G_DEFINE_TYPE (GeglTileBackendFile, gegl_tile_backend_file, GEGL_TYPE_TILE_BACKEND)

static GObjectClass * parent_class = NULL;


typedef struct
{
  guint64 offset;
  gint    rev;
  gint    x;
  gint    y;
  gint    z;
} FileEntry;

typedef struct
{
  guint64 start;
  guint64 end;
} FileGap;

enum
{
  PROP_0,
  PROP_PATH
};


void                     gegl_tile_backend_file_stats         (void);
static void              gegl_tile_backend_file_dbg_alloc     (gint                      size);
static void              gegl_tile_backend_file_dbg_dealloc   (gint                      size);
static inline FileEntry *gegl_tile_backend_file_entry_create  (gint                      x,
                                                               gint                      y,
                                                               gint                      z);
static guint64           gegl_tile_backend_file_find_offset   (GeglTileBackendFile      *self,
                                                               gint                      size);
static inline FileGap *  gegl_tile_backend_file_gap_new       (guint64                   start,
                                                               guint64                   end);
static void              gegl_tile_backend_file_entry_destroy (GeglTileBackendFile      *self,
                                                               FileEntry                *entry);
static inline FileEntry *gegl_tile_backend_file_lookup_entry  (GeglTileBackendFile      *self,
                                                               gint                      x,
                                                               gint                      y,
                                                               gint                      z);
static GeglTile *        gegl_tile_backend_file_get_tile      (GeglTileSource           *self,
                                                               gint                      x,
                                                               gint                      y,
                                                               gint                      z);
static gpointer          gegl_tile_backend_file_set_tile      (GeglTileSource           *self,
                                                               GeglTile                 *tile,
                                                               gint                      x,
                                                               gint                      y,
                                                               gint                      z);
static gpointer          gegl_tile_backend_file_void_tile     (GeglTileSource           *self,
                                                               GeglTile                 *tile,
                                                               gint                      x,
                                                               gint                      y,
                                                               gint                      z);
static GeglBufferHeader  gegl_tile_backend_file_read_header   (GeglTileBackendFile      *self);
static void              gegl_tile_backend_file_write_header  (GeglTileBackendFile      *self);
static GList*            gegl_tile_backend_file_read_index    (GeglTileBackendFile      *self);
static void              gegl_tile_backend_file_load_index    (GeglTileBackendFile      *self);
static gpointer          gegl_tile_backend_file_flush         (GeglTileSource           *source);
static gpointer          gegl_tile_backend_file_exist_tile    (GeglTileSource           *self,
                                                               GeglTile                 *tile,
                                                               gint                      x,
                                                               gint                      y,
                                                               gint                      z);
static gpointer          gegl_tile_backend_file_command       (GeglTileSource           *self,
                                                               GeglTileCommand           command,
                                                               gint                      x,
                                                               gint                      y,
                                                               gint                      z,
                                                               gpointer                  data);
static void              gegl_tile_backend_file_file_changed  (GFileMonitor             *monitor,
                                                               GFile                    *file,
                                                               GFile                    *other_file,
                                                               GFileMonitorEvent         event_type,
                                                               GeglTileBackendFile      *self);
static guint             gegl_tile_backend_file_hashfunc      (gconstpointer             key);
static gboolean          gegl_tile_backend_file_equalfunc     (gconstpointer             a,
                                                               gconstpointer             b);
static GObject *         gegl_tile_backend_file_constructor   (GType                     type,
                                                               guint                     n_params,
                                                               GObjectConstructParam    *params);
static void              gegl_tile_backend_file_finalize      (GObject                  *object);
static void              gegl_tile_backend_file_set_property  (GObject                  *object,
                                                               guint                     property_id,
                                                               const GValue             *value,
                                                               GParamSpec               *pspec);
static void              gegl_tile_backend_file_get_property  (GObject                  *object,
                                                               guint                     property_id,
                                                               GValue                   *value,
                                                               GParamSpec               *pspec);
static void              gegl_tile_backend_file_class_init    (GeglTileBackendFileClass *klass);
static void              gegl_tile_backend_file_init          (GeglTileBackendFile      *self);
void                     gegl_tile_backend_file_cleanup       (void);


GeglAIOFile *swap_file     = NULL;
GList       *swap_gap_list = NULL;


/* this debugging is across all buffers */
static gint allocs         = 0;
static gint file_size      = 0;
static gint peak_allocs    = 0;
static gint peak_file_size = 0;

void
gegl_tile_backend_file_stats (void)
{
  g_warning ("leaked: %i chunks (%f mb)  peak: %i (%i bytes %fmb))",
             allocs, file_size / 1024 / 1024.0,
             peak_allocs, peak_file_size, peak_file_size / 1024 / 1024.0);
}

static void
gegl_tile_backend_file_dbg_alloc (gint size)
{
  allocs++;
  file_size += size;
  if (allocs > peak_allocs)
    peak_allocs = allocs;
  if (file_size > peak_file_size)
    peak_file_size = file_size;
}

static void
gegl_tile_backend_file_dbg_dealloc (gint size)
{
  allocs--;
  file_size -= size;
}

static inline FileEntry *
gegl_tile_backend_file_entry_create (gint x,
                                     gint y,
                                     gint z)
{
  FileEntry *entry = g_slice_new0 (FileEntry);

  entry->x = x;
  entry->y = y;
  entry->z = z;

  return entry;
}

static guint64
gegl_tile_backend_file_find_offset (GeglTileBackendFile *self,
                                    gint                 size)
{
  FileGap *gap;
  guint64  offset;

  if (*self->gap_list)
    {
      GList *link = *self->gap_list;

      while (link)
        {
          gap = link->data;

          if ((gap->end - gap->start) >= size)
            {
              guint64 offset = gap->start;

              gap->start += size;

              if (gap->start == gap->end)
                {
                  g_slice_free (FileGap, gap);
                  *self->gap_list = g_list_remove_link (*self->gap_list, link);
                  g_list_free (link);
                }

              return offset;
            }

          link = link->next;
        }
    }

  offset = self->file->total;

  gegl_aio_file_resize (self->file, self->file->total + size + 32 * 4 * 4 * 128 * 64);

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "pushed resize to %i", (gint)self->file->total);

  return offset;
}

static inline FileGap *
gegl_tile_backend_file_gap_new (guint64 start,
                                guint64 end)
{
  FileGap *gap = g_slice_new (FileGap);

  gap->start = start;
  gap->end   = end;

  return gap;
}

static void
gegl_tile_backend_file_entry_destroy (GeglTileBackendFile *self,
                                      FileEntry           *entry)
{
  guint64  start, end;
  gint     tile_size = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self));
  GList   *link, *link2;
  FileGap *gap, *gap2;

  start = entry->offset;
  end = start + tile_size;

  link = *self->gap_list;
  while (link)
    {
      gap = link->data;

      if (end == gap->start)
        {
          gap->start = start;

          if (link->prev)
            {
              gap2 = link->prev->data;

              if (gap->start == gap2->end)
                {
                  gap2->end = gap->end;
                  g_slice_free (FileGap, gap);
                  *self->gap_list = g_list_remove_link (*self->gap_list, link);
                  g_list_free (link);
                }
            }
          break;
        }
      else if (start == gap->end)
        {
          gap->end = end;

          if (link->next)
            {
              gap2 = link->next->data;

              if (gap->end == gap2->start)
                {
                  gap2->start = gap->start;
                  g_slice_free (FileGap, gap);
                  *self->gap_list =
                    g_list_remove_link (*self->gap_list, link);
                  g_list_free (link);
                }
            }
          break;
        }
      else if (end < gap->start)
        {
          gap = gegl_tile_backend_file_gap_new (start, end);

          link2 = g_list_alloc ();
          link2->data = gap;
          link2->next = link;
          link2->prev = link->prev;
          if (link->prev)
            link->prev->next = link2;
          link->prev = link2;

          if (link == *self->gap_list)
            *self->gap_list = link2;
          break;
        }
      else if (!link->next)
        {
          gap = gegl_tile_backend_file_gap_new (start, end);
          link->next = g_list_alloc ();
          link->next->data = gap;
          link->next->prev = link;
          break;
        }

      link = link->next;
    }

  if (!*self->gap_list)
    {
      gap = gegl_tile_backend_file_gap_new (start, end);
      *self->gap_list = g_list_append (*self->gap_list, gap);
    }

  link = g_list_last (*self->gap_list);
  gap = link->data;

  if (gap->end == self->file->total)
    {
      gegl_aio_file_resize (self->file, gap->start);
      g_slice_free (FileGap, gap);
      *self->gap_list = g_list_remove_link (*self->gap_list, link);
      g_list_free (link);
    }

  g_hash_table_remove (self->index, entry);
  g_slice_free (FileEntry, entry);
  gegl_tile_backend_file_dbg_dealloc (gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self)));
}

static inline FileEntry *
gegl_tile_backend_file_lookup_entry (GeglTileBackendFile *self,
                                     gint                 x,
                                     gint                 y,
                                     gint                 z)
{
  FileEntry *ret = NULL;
  FileEntry *key = gegl_tile_backend_file_entry_create (x, y, z);

  ret = g_hash_table_lookup (self->index, key);
  g_slice_free (FileEntry, key);

  return ret;
}

static GeglTile *
gegl_tile_backend_file_get_tile (GeglTileSource *self,
                                 gint            x,
                                 gint            y,
                                 gint            z)
{
  GeglTileBackend     *backend;
  GeglTileBackendFile *tile_backend_file;
  FileEntry           *entry;
  GeglTile            *tile = NULL;
  gint                 tile_size;

  backend           = GEGL_TILE_BACKEND (self);
  tile_backend_file = GEGL_TILE_BACKEND_FILE (backend);
  entry             = gegl_tile_backend_file_lookup_entry (tile_backend_file, x, y, z);

  if (!entry)
    return NULL;

  tile_size = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self));
  tile      = gegl_tile_new (tile_size);
  gegl_tile_set_rev (tile, entry->rev);
  gegl_tile_mark_as_stored (tile);

  gegl_aio_file_read (tile_backend_file->file, entry->offset, tile_size,
                      gegl_tile_get_data (tile));

  GEGL_NOTE(GEGL_DEBUG_TILE_BACKEND, "read entry %i, %i, %i from %i", entry->x, entry->y, entry->z, (gint)entry->offset);

  return tile;
}

static gpointer
gegl_tile_backend_file_set_tile (GeglTileSource *self,
                                 GeglTile       *tile,
                                 gint            x,
                                 gint            y,
                                 gint            z)
{
  GeglTileBackend     *backend;
  GeglTileBackendFile *tile_backend_file;
  FileEntry           *entry;
  gint                 tile_size;

  backend           = GEGL_TILE_BACKEND (self);
  tile_backend_file = GEGL_TILE_BACKEND_FILE (backend);
  tile_size         = gegl_tile_backend_get_tile_size (backend);
  entry             = gegl_tile_backend_file_lookup_entry (tile_backend_file, x, y, z);

  if (entry == NULL)
    {
      entry          = gegl_tile_backend_file_entry_create (x, y, z);
      entry->offset  = gegl_tile_backend_file_find_offset (tile_backend_file, tile_size);
      g_hash_table_insert (tile_backend_file->index, entry, entry);
      gegl_tile_backend_file_dbg_alloc (tile_size);
    }

  gegl_aio_file_write (tile_backend_file->file, entry->offset,
                       tile_size,
                       gegl_tile_get_data (tile));

  gegl_tile_mark_as_stored (tile);

  GEGL_NOTE(GEGL_DEBUG_TILE_BACKEND, "pushed write of entry %i, %i, %i at %i", entry->x, entry->y, entry->z, (gint)entry->offset);

  return NULL;
}

static gpointer
gegl_tile_backend_file_void_tile (GeglTileSource *self,
                                  GeglTile       *tile,
                                  gint            x,
                                  gint            y,
                                  gint            z)
{
  GeglTileBackend     *backend;
  GeglTileBackendFile *tile_backend_file;
  FileEntry           *entry;

  backend           = GEGL_TILE_BACKEND (self);
  tile_backend_file = GEGL_TILE_BACKEND_FILE (backend);
  entry             = gegl_tile_backend_file_lookup_entry (tile_backend_file, x, y, z);

  if (entry != NULL)
    {
      GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "void tile %i, %i, %i", x, y, z);

      gegl_tile_backend_file_entry_destroy (tile_backend_file, entry);
    }

  return NULL;
}

static GeglBufferHeader
gegl_tile_backend_file_read_header (GeglTileBackendFile *self)
{
  GeglBufferHeader ret;

  gegl_aio_file_read(self->file, 0, sizeof (GeglBufferHeader), (guchar*)&ret);

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "read header: tile-width: %i tile-height: %i next:%i  %ix%i\n",
             ret.tile_width,
             ret.tile_height,
             (guint)ret.next,
             ret.width,
             ret.height);

  if (!(ret.magic[0]=='G' &&
       ret.magic[1]=='E' &&
       ret.magic[2]=='G' &&
       ret.magic[3]=='L'))
    {
      g_warning ("Magic is wrong! %s", ret.magic);
    }

  return ret;
}

static void
gegl_tile_backend_file_write_header (GeglTileBackendFile *self)
{
  gegl_aio_file_write (self->file, 0, sizeof (GeglBufferHeader), (guchar*)&self->header);
}

static GList *
gegl_tile_backend_file_read_index (GeglTileBackendFile *self)
{
  GList          *ret    = NULL;
  guint64         offset = self->header.next;
  GeglBufferTile *item;

  while (offset) /* ->next of the last block is 0 */
    {
      item = g_malloc (sizeof (GeglBufferTile));
      gegl_aio_file_read (self->file, offset, sizeof (GeglBufferTile), (guchar*)item);
      offset = item->block.next;
      ret = g_list_prepend (ret, item);
    }

  ret = g_list_reverse (ret);

  return ret;
}

static void
gegl_tile_backend_file_load_index (GeglTileBackendFile *self)
{
  GeglBufferHeader  new_header;
  GList            *tiles, *iter;
  GeglTileBackend  *backend = GEGL_TILE_BACKEND (self);
  guint64           max     = 0;
  gint              tile_size;

  new_header = gegl_tile_backend_file_read_header (self);

  while (new_header.flags & GEGL_FLAG_LOCKED)
    {
      g_usleep (50000);
      new_header = gegl_tile_backend_file_read_header (self);
    }

  if (new_header.rev == self->header.rev)
    {
      GEGL_NOTE(GEGL_DEBUG_TILE_BACKEND, "header not changed: %s", self->path);
      return;
    }
  else
    {
      self->header = new_header;
      GEGL_NOTE(GEGL_DEBUG_TILE_BACKEND, "loading index: %s", self->path);
    }

  tile_size = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (self));
  tiles = gegl_tile_backend_file_read_index (self);

  for (iter = tiles; iter; iter = iter->next)
    {
      GeglBufferTile *item           = iter->data;
      FileEntry      *new, *existing =
        gegl_tile_backend_file_lookup_entry (self, item->x, item->y, item->z);

      if (item->offset > max)
        max = item->offset + tile_size;

      if (existing)
        {
          if (existing->rev == item->rev)
            {
              g_assert (existing->offset == item->offset);
              g_free (item);
              continue;
            }
          else
            {
              GeglTileStorage *storage =
                (void*) gegl_tile_backend_peek_storage (backend);
              GeglRectangle rect;

              g_hash_table_remove (self->index, existing);
              gegl_tile_source_refetch (GEGL_TILE_SOURCE (storage),
                                        existing->x,
                                        existing->y,
                                        existing->z);

              if (existing->z == 0)
                {
                  rect.width = self->header.tile_width;
                  rect.height = self->header.tile_height;
                  rect.x = existing->x * self->header.tile_width;
                  rect.y = existing->y * self->header.tile_height;
                }
              g_free (existing);
              g_signal_emit_by_name (storage, "changed", &rect, NULL);
            }
        }

      /* we've found a new tile */
      new = gegl_tile_backend_file_entry_create (item->x, item->y, item->z);
      new->rev = item->rev;
      new->offset = item->offset;
      g_hash_table_insert (self->index, new, new);
      g_free (item);
    }

  g_list_free (tiles);
  g_list_free (*self->gap_list);
  *self->gap_list   = NULL;
  self->file->total = max;
}

static gpointer
gegl_tile_backend_file_flush (GeglTileSource *source)
{
  GeglTileBackend     *backend;
  GeglTileBackendFile *self;
  GList               *tiles;

  backend  = GEGL_TILE_BACKEND (source);
  self     = GEGL_TILE_BACKEND_FILE (backend);

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "flushing %s", self->path);

  self->header.rev++;
  tiles = g_hash_table_get_keys (self->index);

  if (tiles == NULL)
    self->header.next = 0;
  else
    {
      GList           *iter;
      guchar          *index;
      guint64          index_offset;
      gint             index_len;
      GeglBufferBlock *previous = NULL;

      index_len    = g_list_length (tiles) * sizeof (GeglBufferTile);
      index        = g_malloc (index_len);
      index_offset = gegl_tile_backend_file_find_offset (self, index_len);
      index_len    = 0;

      for (iter = tiles; iter; iter = iter->next)
        {
          FileEntry      *item = iter->data;
          GeglBufferTile  entry;

          entry.x            = item->x;
          entry.y            = item->y;
          entry.z            = item->z;
          entry.rev          = item->rev;
          entry.offset       = item->offset;
          entry.block.flags  = GEGL_FLAG_TILE;
          entry.block.length = sizeof (GeglBufferTile);

          memcpy (index + index_len, &entry, sizeof (GeglBufferTile));

          if (previous)
            previous->next = index_offset + index_len;

          previous = (GeglBufferBlock *)(index + index_len);

          index_len += sizeof (GeglBufferTile);
        }

      previous->next = 0; /* last block points to offset 0 */
      self->header.next = index_offset;
      gegl_aio_file_write (self->file, index_offset, index_len, index);
      g_free (index);
      g_list_free (tiles);
    }

  gegl_tile_backend_file_write_header (self);
  gegl_aio_file_sync (self->file);

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "flushed %s", self->path);

  return (gpointer)0xf0f;
}

static gpointer
gegl_tile_backend_file_exist_tile (GeglTileSource *self,
                                   GeglTile       *tile,
                                   gint            x,
                                   gint            y,
                                   gint            z)
{
  GeglTileBackend     *backend;
  GeglTileBackendFile *tile_backend_file;
  FileEntry           *entry;

  backend           = GEGL_TILE_BACKEND (self);
  tile_backend_file = GEGL_TILE_BACKEND_FILE (backend);
  entry             = gegl_tile_backend_file_lookup_entry (tile_backend_file, x, y, z);

  return entry!=NULL?((gpointer)0x1):NULL;
}

gboolean
gegl_tile_backend_file_try_lock (GeglTileBackendFile *self)
{
  GeglBufferHeader new_header = gegl_tile_backend_file_read_header (self);

  if (new_header.flags & GEGL_FLAG_LOCKED)
    return FALSE;

  self->header.flags += GEGL_FLAG_LOCKED;
  gegl_tile_backend_file_write_header (self);
  gegl_aio_file_sync (self->file);

  return TRUE;
}

gboolean
gegl_tile_backend_file_unlock (GeglTileBackendFile *self)
{
  if (!(self->header.flags & GEGL_FLAG_LOCKED))
    {
      g_warning ("tried to unlock unlocked buffer");
      return FALSE;
    }

  self->header.flags -= GEGL_FLAG_LOCKED;
  gegl_tile_backend_file_write_header (self);
  gegl_aio_file_sync (self->file);

  return TRUE;
}

static gpointer
gegl_tile_backend_file_command (GeglTileSource  *self,
                                GeglTileCommand  command,
                                gint             x,
                                gint             y,
                                gint             z,
                                gpointer         data)
{
  switch (command)
    {
      case GEGL_TILE_GET:
        return gegl_tile_backend_file_get_tile (self, x, y, z);
      case GEGL_TILE_SET:
        return gegl_tile_backend_file_set_tile (self, data, x, y, z);
      case GEGL_TILE_IDLE:
        return NULL;
      case GEGL_TILE_VOID:
        return gegl_tile_backend_file_void_tile (self, data, x, y, z);
      case GEGL_TILE_EXIST:
        return gegl_tile_backend_file_exist_tile (self, data, x, y, z);
      case GEGL_TILE_FLUSH:
        return gegl_tile_backend_file_flush (self);

      default:
        g_assert (command < GEGL_TILE_LAST_COMMAND &&
                  command >= 0);
    }
  return FALSE;
}

static void
gegl_tile_backend_file_file_changed (GFileMonitor        *monitor,
                                     GFile               *file,
                                     GFile               *other_file,
                                     GFileMonitorEvent    event_type,
                                     GeglTileBackendFile *self)
{
  if (event_type == G_FILE_MONITOR_EVENT_CHANGED)
    gegl_tile_backend_file_load_index (self);
}

static guint
gegl_tile_backend_file_hashfunc (gconstpointer key)
{
  const FileEntry *entry = key;
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
gegl_tile_backend_file_equalfunc (gconstpointer a,
                                  gconstpointer b)
{
  const FileEntry *ea = a;
  const FileEntry *eb = b;

  if (ea->x == eb->x &&
      ea->y == eb->y &&
      ea->z == eb->z)
    return TRUE;

  return FALSE;
}

static GObject *
gegl_tile_backend_file_constructor (GType                  type,
                                    guint                  n_params,
                                    GObjectConstructParam *params)
{
  GObject             *object;
  GeglTileBackendFile *self;
  GeglTileBackend     *backend;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  self   = GEGL_TILE_BACKEND_FILE (object);
  backend = GEGL_TILE_BACKEND (self);

  self->index = g_hash_table_new (gegl_tile_backend_file_hashfunc,
                                  gegl_tile_backend_file_equalfunc);

  if (self->path)
    {
      /* if the path is specified, use a separate file for this buffer */

      self->file = g_object_new (GEGL_TYPE_AIO_FILE, "path", self->path, NULL);
      self->gap_list = g_malloc (sizeof (void*));
      *self->gap_list = NULL;

      if (g_access (self->path, F_OK) != -1)
        {
          /* if we can access the file, assume that it was created by another
            process and read it */
          self->gfile = g_file_new_for_commandline_arg (self->path);
          self->monitor = g_file_monitor_file (self->gfile,
                                               G_FILE_MONITOR_NONE,
                                               NULL, NULL);
          g_signal_connect (self->monitor, "changed",
                            G_CALLBACK (gegl_tile_backend_file_file_changed),
                            self);
          self->header = gegl_tile_backend_file_read_header (self);
          self->header.rev -= 1;

          /* we are overriding all of the work of the actual constructor here,
           * a really evil hack :d
           */
          backend->priv->tile_width = self->header.tile_width;
          backend->priv->tile_height = self->header.tile_height;
          backend->priv->format = babl_format (self->header.description);
          backend->priv->px_size = babl_format_get_bytes_per_pixel (backend->priv->format);
          backend->priv->tile_size = backend->priv->tile_width *
            backend->priv->tile_height *
            backend->priv->px_size;
          backend->priv->shared = TRUE;

          gegl_tile_backend_file_load_index (self);
        }
      else
        {
          gegl_buffer_header_init (&self->header,
                                   backend->priv->tile_width,
                                   backend->priv->tile_height,
                                   backend->priv->px_size,
                                   backend->priv->format);
        }
    }
  else
    {
      /* use a file that's shared between multiple buffers */
      if (swap_file == NULL)
        {
          gchar *filename, *path;

          filename  = g_strdup_printf ("%i-common-swap-file.swap", getpid ());
          path      = g_build_filename (gegl_config ()->swap, filename, NULL);
          swap_file = g_object_new (GEGL_TYPE_AIO_FILE, "path", path, NULL);
          g_free (filename);
          g_free (path);
        }

      self->gap_list = &swap_gap_list;
      self->file = swap_file;

      gegl_buffer_header_init (&self->header,
                               backend->priv->tile_width,
                               backend->priv->tile_height,
                               backend->priv->px_size,
                               backend->priv->format);
    }

  backend->priv->header = &self->header;

  GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "constructing file backend");

  return object;
}

static void
gegl_tile_backend_file_finalize (GObject *object)
{
  GeglTileBackendFile *self = GEGL_TILE_BACKEND_FILE (object);

  if (self->index)
    {
      GList *tiles = g_hash_table_get_keys (self->index);

      if (tiles != NULL)
        {
          GList *iter;

          for (iter = tiles; iter; iter = iter->next)
            gegl_tile_backend_file_entry_destroy (self, iter->data);
        }

      g_list_free (tiles);

      g_hash_table_unref (self->index);

      self->index = NULL;
    }

  if (self->file && self->file != swap_file)
    {
      g_object_unref (self->file);
      self->file = NULL;
    }

  if (self->gfile)
    {
      g_object_unref (self->gfile);
      self->gfile = NULL;
    }

  if (self->monitor)
    {
      g_object_unref (self->monitor);
      self->monitor = NULL;
    }

  if (self->path)
    {
      g_free (self->path);
      self->path = NULL;
    }

  if (*self->gap_list && *self->gap_list != swap_gap_list)
    {
      GList *iter;

      for (iter = *self->gap_list; iter; iter = iter->next)
        g_slice_free (FileGap, iter->data);

      g_list_free (*self->gap_list);
      *self->gap_list = NULL;
    }

  (*G_OBJECT_CLASS (parent_class)->finalize)(object);
}

static void
gegl_tile_backend_file_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GeglTileBackendFile *self = GEGL_TILE_BACKEND_FILE (object);

  switch (property_id)
    {
    case PROP_PATH:
      self->path = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gegl_tile_backend_file_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GeglTileBackendFile *self = GEGL_TILE_BACKEND_FILE (object);

  switch (property_id)
    {
    case PROP_PATH:
      g_value_set_string (value, self->path);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


static void
gegl_tile_backend_file_class_init (GeglTileBackendFileClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->constructor  = gegl_tile_backend_file_constructor;
  gobject_class->finalize     = gegl_tile_backend_file_finalize;
  gobject_class->set_property = gegl_tile_backend_file_set_property;
  gobject_class->get_property = gegl_tile_backend_file_get_property;

  g_object_class_install_property (gobject_class, PROP_PATH,
                                   g_param_spec_string ("path",
                                                        "path",
                                                        "Path of the backing file",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE));
}

static void
gegl_tile_backend_file_init (GeglTileBackendFile *self)
{
  ((GeglTileSource*)self)->command = gegl_tile_backend_file_command;

  self->path     = NULL;
  self->file     = NULL;
  self->index    = NULL;
  self->gap_list = NULL;
  self->gfile    = NULL;
  self->monitor  = NULL;
}

void
gegl_tile_backend_file_cleanup (void)
{
  if (swap_file)
    g_object_unref (swap_file);

  if (swap_gap_list)
    {
      GList *iter;

      for (iter = swap_gap_list; iter; iter = iter->next)
        g_slice_free (FileGap, iter->data);

      g_list_free (swap_gap_list);
    }
}
