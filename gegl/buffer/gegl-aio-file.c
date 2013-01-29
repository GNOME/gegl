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

#include "gegl.h"
#include "gegl-buffer-backend.h"
#include "gegl-aio-file.h"
#include "gegl-debug.h"
#include "gegl-config.h"

#ifndef HAVE_FSYNC

#ifdef G_OS_WIN32
#define fsync _commit
#endif

#endif


G_DEFINE_TYPE (GeglAIOFile, gegl_aio_file, G_TYPE_OBJECT);

static GObjectClass * parent_class = NULL;


typedef enum
{
  OP_WRITE,
  OP_TRUNCATE,
  OP_SYNC
} ThreadOp;

typedef struct
{
  guint64  offset;
  guint    length;
  GList   *link;
} FileEntry;

typedef struct
{
  GeglAIOFile *file;
  FileEntry   *entry;
  guchar      *source;
  ThreadOp     operation;
} ThreadParams;

enum
{
  PROP_0,
  PROP_PATH
};


static void       gegl_aio_file_push_queue   (ThreadParams          *params);
static void       gegl_aio_file_thread_write (ThreadParams          *params);
static gpointer   gegl_aio_file_thread       (gpointer               ignored);
static FileEntry *gegl_aio_file_entry_create (guint64                offset,
                                              guint                  length);
static FileEntry *gegl_aio_file_lookup_entry (GeglAIOFile           *self,
                                              guint64                offset);
static guint      gegl_aio_file_hashfunc     (gconstpointer          key);
static gboolean   gegl_aio_file_equalfunc    (gconstpointer          a,
                                              gconstpointer          b);
static GObject   *gegl_aio_file_constructor  (GType                  type,
                                              guint                  n_params,
                                              GObjectConstructParam *params);
static void       gegl_aio_file_init         (GeglAIOFile           *self);
static void       gegl_aio_file_ensure_exist (GeglAIOFile           *self);
static void       gegl_aio_file_class_init   (GeglAIOFileClass      *klass);
static void       gegl_aio_file_finalize     (GObject               *object);
static void       gegl_aio_file_get_property (GObject               *gobject,
                                              guint                  property_id,
                                              GValue                *value,
                                              GParamSpec            *pspec);
static void       gegl_aio_file_set_property (GObject               *object,
                                              guint                  property_id,
                                              const GValue          *value,
                                              GParamSpec            *pspec);
void              gegl_aio_file_cleanup      (void);


static GThread      *writer_thread = NULL;
static GQueue       *queue         = NULL;
static ThreadParams *in_progress   = NULL;
static gboolean      exit_thread   = FALSE;
static guint64       queue_size    = 0;
static GCond         empty_cond;
static GCond         max_cond;
static GMutex        mutex;


static gpointer
gegl_aio_file_thread (gpointer ignored)
{
  while (TRUE)
    {
      ThreadParams *params;

      g_mutex_lock (&mutex);

      while (g_queue_is_empty (queue) && !exit_thread)
        g_cond_wait (&empty_cond, &mutex);

      if (exit_thread)
        {
          g_mutex_unlock (&mutex);
          /*GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "exiting writer thread");*/
          return NULL;
        }

      params = (ThreadParams *)g_queue_pop_head (queue);
      if (params->operation == OP_WRITE)
        {
          g_hash_table_remove (params->file->index, params->entry);
          in_progress = params;
          params->entry->link = NULL;
        }

      g_mutex_unlock (&mutex);

      switch (params->operation)
        {
        case OP_WRITE:
          gegl_aio_file_thread_write (params);
          break;
        case OP_TRUNCATE:
          ftruncate (params->file->out_fd, params->file->total);
          break;
        case OP_SYNC:
          fsync (params->file->out_fd);
          break;
        }

      g_mutex_lock (&mutex);

      in_progress = NULL;

      if (params->operation == OP_WRITE)
        {
          queue_size -= params->entry->length + sizeof (GList) +
            sizeof (ThreadParams);
          g_free (params->source);
          g_slice_free (FileEntry, params->entry);

          /* unblock the main thread if the queue had gotten too big */
          if (queue_size <= gegl_config ()->queue_size)
            g_cond_signal (&max_cond);
        }

      g_slice_free (ThreadParams, params);

      g_mutex_unlock (&mutex);
    }

  return NULL;
}

static void
gegl_aio_file_push_queue (ThreadParams *params)
{
  g_mutex_lock (&mutex);

  /* block if the queue has gotten too big */
  while (queue_size > gegl_config ()->queue_size)
    g_cond_wait (&max_cond, &mutex);

  g_queue_push_tail (queue, params);

  if (params->operation == OP_WRITE)
    {
      params->entry->link = g_queue_peek_tail_link (queue);
      queue_size += params->entry->length + sizeof (GList) + sizeof (ThreadParams);
    }

  /* wake up the writer thread */
  g_cond_signal (&empty_cond);

  g_mutex_unlock (&mutex);
}

static void
gegl_aio_file_thread_write (ThreadParams *params)
{
  GeglAIOFile *file          = params->file;
  guint        to_be_written = params->entry->length;
  guint64      offset        = params->entry->offset;

  if (file->out_offset != offset)
    {
      if (lseek (file->out_fd, offset, SEEK_SET) < 0)
        {
          g_warning ("unable to seek to tile in buffer: %s", g_strerror (errno));
          return;
        }
      file->out_offset = offset;
    }

  while (to_be_written > 0)
    {
      gint wrote;
      wrote = write (file->out_fd,
                     params->source + params->entry->length - to_be_written,
                     to_be_written);
      if (wrote <= 0)
        {
          g_message ("unable to write tile data to self: "
                     "%s (%d/%d bytes written)",
                     g_strerror (errno), wrote, to_be_written);
          break;
        }

      to_be_written    -= wrote;
      file->out_offset += wrote;
    }

  /*GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "writer thread wrote at %i", (gint)offset);*/
}

static FileEntry *
gegl_aio_file_entry_create (guint64 offset,
                            guint   length)
{
  FileEntry *entry = g_slice_new (FileEntry);

  entry->offset = offset;
  entry->length = length;
  entry->link   = NULL;

  return entry;
}

static FileEntry *
gegl_aio_file_lookup_entry (GeglAIOFile *self,
                            guint64      offset)
{
  FileEntry *ret = NULL;
  FileEntry *key = gegl_aio_file_entry_create (offset, 0);

  ret = g_hash_table_lookup (self->index, key);
  g_slice_free (FileEntry, key);

  return ret;
}

void
gegl_aio_file_resize (GeglAIOFile *self,
                      guint64      size)
{
  ThreadParams *params;

  self->total       = size;
  params            = g_slice_new0 (ThreadParams);
  params->file      = self;
  params->operation = OP_TRUNCATE;

  gegl_aio_file_push_queue (params);
}

void
gegl_aio_file_read (GeglAIOFile *self,
                    guint64      offset,
                    guint        length,
                    guchar      *dest)
{
  FileEntry    *entry;
  guint         to_be_read = length;
  ThreadParams *params     = NULL;

  gegl_aio_file_ensure_exist (self);

  g_mutex_lock (&mutex);

  entry = gegl_aio_file_lookup_entry (self, offset);

  if (entry && entry->link)
    params = entry->link->data;
  else if (in_progress && in_progress->entry->offset == offset &&
           in_progress->operation == OP_WRITE)
    params = in_progress;

  if (params)
    {
      if (params->entry->length >= length)
        {
          memcpy (dest, params->source, length);
          g_mutex_unlock (&mutex);
          return;
        }
      else
        {
          guint len = params->entry->length;

          memcpy (dest, params->source, len);
          offset += len;
          to_be_read -= len;
        }
    }

  g_mutex_unlock (&mutex);

  if (self->in_offset != offset)
    {
      if (lseek (self->in_fd, offset, SEEK_SET) < 0)
        {
          g_warning ("unable to seek to tile in buffer: %s", g_strerror (errno));
          return;
        }
      self->in_offset = offset;
    }

  while (to_be_read > 0)
    {
      GError *error = NULL;
      gint    bytes_read;

      bytes_read = read (self->in_fd, dest + length - to_be_read, to_be_read);
      if (bytes_read <= 0)
        {
          g_message ("unable to read tile data from file %s: "
                     "%s (%d/%d bytes read) %s",
                     self->path, g_strerror (errno), bytes_read, to_be_read,
                     error?error->message:"--");
          return;
        }
      to_be_read      -= bytes_read;
      self->in_offset += bytes_read;
    }
}

void
gegl_aio_file_write (GeglAIOFile *self,
                     guint64      offset,
                     guint        length,
                     guchar      *source)
{
  FileEntry    *entry = NULL;
  ThreadParams *params;
  guchar       *new_source;

  gegl_aio_file_ensure_exist (self);

  g_mutex_lock (&mutex);

  entry = gegl_aio_file_lookup_entry (self, offset);

  if (entry && entry->link)
    {
      params = entry->link->data;

      if (entry->length != length)
        {
          if (entry->length < length)
            params->source = g_realloc (params->source, length);

          entry->length = length;
        }

      memcpy (params->source, source, length);
      g_mutex_unlock (&mutex);
      return;
    }

  entry = gegl_aio_file_entry_create (offset, length);
  g_hash_table_insert (self->index, entry, entry);

  g_mutex_unlock (&mutex);

  new_source = g_malloc (length);
  memcpy (new_source, source, length);

  params            = g_slice_new0 (ThreadParams);
  params->operation = OP_WRITE;
  params->source    = new_source;
  params->entry     = entry;
  params->file      = self;

  gegl_aio_file_push_queue (params);
}

void
gegl_aio_file_sync (GeglAIOFile *self)
{
  ThreadParams *params = g_slice_new0 (ThreadParams);
  params->operation = OP_SYNC;
  params->file = self;

  gegl_aio_file_push_queue (params);
}

static guint
gegl_aio_file_hashfunc (gconstpointer key)
{
  return ((const FileEntry *) key)->offset;
}

static gboolean
gegl_aio_file_equalfunc (gconstpointer a,
                         gconstpointer b)
{
  const FileEntry *ea = a;
  const FileEntry *eb = b;

  if (ea->offset == eb->offset)
    return TRUE;

  return FALSE;
}

static void
gegl_aio_file_init (GeglAIOFile *self)
{
  self->index     = NULL;
  self->path      = NULL;
  self->in_fd     = self->out_fd = -1;
  self->in_offset = self->out_offset = 0;
  self->total     = 0;
}

static GObject *
gegl_aio_file_constructor (GType                  type,
                           guint                  n_params,
                           GObjectConstructParam *params)
{
  GObject     *object;
  GeglAIOFile *self;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  self   = GEGL_AIO_FILE (object);

  self->index = g_hash_table_new (gegl_aio_file_hashfunc,
                                  gegl_aio_file_equalfunc);

  /*GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "constructing swap backend");*/

  return object;
}

static void
gegl_aio_file_ensure_exist (GeglAIOFile *self)
{
  if (self->in_fd == -1 || self->out_fd == -1)
    {
      /*GEGL_NOTE (GEGL_DEBUG_TILE_BACKEND, "creating swapfile %s", self->path);*/

      self->out_fd = g_open (self->path, O_RDWR|O_CREAT, 0770);
      self->in_fd = g_open (self->path, O_RDONLY);

      if (self->out_fd == -1 || self->in_fd == -1)
        g_warning ("Could not open file '%s': %s", self->path, g_strerror (errno));
    }
}

static void
gegl_aio_file_class_init (GeglAIOFileClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->constructor  = gegl_aio_file_constructor;
  gobject_class->finalize     = gegl_aio_file_finalize;
  gobject_class->get_property = gegl_aio_file_get_property;
  gobject_class->set_property = gegl_aio_file_set_property;

  queue = g_queue_new ();

  writer_thread = g_thread_create_full (gegl_aio_file_thread,
                                        NULL, 0, TRUE, TRUE,
                                        G_THREAD_PRIORITY_NORMAL, NULL);

  g_object_class_install_property (gobject_class, PROP_PATH,
                                   g_param_spec_string ("path",
                                                        "path",
                                                        "The base path for this backing file for a buffer",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE));
}

static void
gegl_aio_file_finalize (GObject *object)
{
  GeglAIOFile *self = GEGL_AIO_FILE (object);

  if (self->index)
    {
      if (g_hash_table_size (self->index) != 0)
        {
          GList *entries, *iter;

          g_mutex_lock (&mutex);

          entries = g_hash_table_get_keys (self->index);

          for (iter = entries; iter; iter = iter->next)
            {
              FileEntry *entry = iter->data;

              if (entry->link)
                {
                  g_slice_free (ThreadParams, entry->link->data);
                  g_queue_delete_link (queue, entry->link);
                }

              g_slice_free (FileEntry, entry);
            }

          g_list_free (entries);

          g_mutex_unlock (&mutex);
        }

      g_hash_table_unref (self->index);
      self->index = NULL;

      g_free (self->path);
      self->path = NULL;

      close (self->in_fd);
      self->in_fd = -1;

      close (self->out_fd);
      self->out_fd = -1;
    }

  (*G_OBJECT_CLASS (parent_class)->finalize)(object);
}

static void
gegl_aio_file_get_property (GObject    *gobject,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GeglAIOFile *self = GEGL_AIO_FILE (gobject);

  switch (property_id)
    {
    case PROP_PATH:
      g_value_set_string (value, self->path);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
    }
}

static void
gegl_aio_file_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  GeglAIOFile *self = GEGL_AIO_FILE (object);

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

void
gegl_aio_file_cleanup (void)
{
  /* check if this class has been used at all before cleaning */
  if (queue)
    {
      exit_thread = TRUE;
      g_cond_signal (&empty_cond);
      g_thread_join (writer_thread);

      if (g_queue_get_length (queue) != 0)
        g_warning ("writer thread queue wasn't empty before freeing\n");

      g_queue_free (queue);
    }
}
