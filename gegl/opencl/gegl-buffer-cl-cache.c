/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2012 Victor Oliveira (victormatheus@gmail.com)
 */

#include "config.h"
#include <string.h>
#include <glib.h>

#include "gegl.h"
#include "gegl-debug.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-handler-cache.h"
#include "gegl-tile-storage.h"

#include "gegl-buffer-cl-cache.h"
#include "opencl/gegl-cl.h"

typedef struct
{
  GeglBuffer           *buffer;
  GeglRectangle         roi;
  cl_mem                tex;
  gboolean              valid;
  gatomicrefcount       refcount;
} CacheEntry;

static GList *cache_entries = NULL;

static GRecMutex cache_mutex = { 0, };

static gboolean
cache_entry_find_invalid (gpointer *data)
{
  gboolean found = FALSE;

  *data = NULL;

  g_rec_mutex_lock (&cache_mutex);

  for (GList *elem = cache_entries; elem != NULL; elem = elem->next)
    {
      CacheEntry *e = elem->data;
      /* refcount == 1 means nothing else has a reference to the CacheEntry
         but the global cache store itself. */
      if (!e->valid && g_atomic_ref_count_compare (&e->refcount, 1))
        {
          *data = e;
          found = TRUE;
          break;
        }
    }

  g_rec_mutex_unlock (&cache_mutex);

  return found;
}

static void
free_cache_entry (gpointer data)
{
  CacheEntry *entry = data;

  GEGL_NOTE (GEGL_DEBUG_OPENCL, "Removing entry from OpenCL global cache: %p %s {%d %d %d %d}",
             entry->buffer,
             babl_get_name (gegl_buffer_get_format (entry->buffer)),
             entry->roi.x,
             entry->roi.y,
             entry->roi.width,
             entry->roi.height);

  gegl_clReleaseMemObject (entry->tex);
  g_object_unref (entry->buffer);

  g_slice_free (CacheEntry, data);
  cache_entries = g_list_remove (cache_entries, data);
}

cl_mem
gegl_buffer_cl_cache_get (GeglBuffer          *buffer,
                          const GeglRectangle *roi)
{
  cl_mem cache_tex = NULL;

  g_rec_mutex_lock (&cache_mutex);

  for (GList *elem = cache_entries; elem != NULL; elem = elem->next)
    {
      CacheEntry *e = elem->data;

      if (e->valid && e->buffer == buffer
          && gegl_rectangle_equal (&e->roi, roi))
        {
          g_atomic_ref_count_inc (&e->refcount);
          cache_tex = e->tex;
          break;
        }
    }

  g_rec_mutex_unlock (&cache_mutex);

  return cache_tex;
}

gboolean
gegl_buffer_cl_cache_release (cl_mem tex)
{
  gboolean found = FALSE;

  g_rec_mutex_lock (&cache_mutex);

  for (GList *elem = cache_entries; elem != NULL; elem = elem->next)
    {
      CacheEntry *e = elem->data;

      if (e->tex == tex)
        {
          e->valid = !g_atomic_ref_count_dec (&e->refcount);
          found = TRUE;
          break;
        }
    }

  g_rec_mutex_unlock (&cache_mutex);

  return found;
}

void
gegl_buffer_cl_cache_new (GeglBuffer            *buffer,
                          const GeglRectangle   *roi,
                          cl_mem                 tex)
{
  CacheEntry *e = g_slice_new (CacheEntry);

  e->buffer        = buffer;
  e->roi           = *roi;
  e->tex           = tex;
  e->valid         = TRUE;
  g_atomic_ref_count_init (&e->refcount);
  g_object_ref (buffer);

  g_rec_mutex_lock (&cache_mutex);

  cache_entries = g_list_prepend (cache_entries, e);

  g_rec_mutex_unlock (&cache_mutex);
}

static inline gboolean
_gegl_buffer_cl_cache_flush2 (GeglTileHandlerCache *cache,
                              const GeglRectangle  *roi)
{
  gpointer data    = NULL;
  cl_int   cl_err  = 0;
  gboolean need_cl = FALSE;

  g_rec_mutex_lock (&cache_mutex);

  for (GList *elem = cache_entries; elem != NULL; elem = elem->next)
    {
      CacheEntry    *entry = elem->data;
      GeglRectangle  tmp;

      if (entry->valid && entry->buffer->tile_storage->cache == cache
          && (!roi || gegl_rectangle_intersect (&tmp, roi, &entry->roi)))
        {
          const Babl *format = gegl_buffer_get_format (entry->buffer);
          size_t      size;

          need_cl = TRUE;

          /* Flush moves (not just copies) the content of a buffer from an
             OpenCL device to the host and marks that entry as invalid. */
          g_atomic_ref_count_inc (&entry->refcount);
          entry->valid = FALSE;

          gegl_cl_color_babl (format, &size);

          data = gegl_clEnqueueMapBuffer (gegl_cl_get_command_queue (),
                                          entry->tex, CL_TRUE, CL_MAP_READ,
                                          0, entry->roi.width * entry->roi.height * size,
                                          0, NULL, NULL, &cl_err);
          CL_CHECK;

          gegl_buffer_set (entry->buffer, &entry->roi, 0, format, data, GEGL_AUTO_ROWSTRIDE);

          cl_err = gegl_clEnqueueUnmapMemObject (gegl_cl_get_command_queue (),
                                                 entry->tex, data,
                                                 0, NULL, NULL);
          CL_CHECK;

          g_atomic_ref_count_dec (&entry->refcount);
        }
    }

  if (need_cl)
    {
      cl_err = gegl_clFinish (gegl_cl_get_command_queue ());
      CL_CHECK;

      while (cache_entry_find_invalid (&data))
        free_cache_entry (data);
    }

  g_rec_mutex_unlock (&cache_mutex);

  return TRUE;

error:
  while (cache_entry_find_invalid (&data))
    free_cache_entry (data);

  g_rec_mutex_unlock (&cache_mutex);

  /* XXX : result is corrupted */
  return FALSE;
}

gboolean
gegl_buffer_cl_cache_flush2 (GeglTileHandlerCache *cache,
                             const GeglRectangle  *roi)
{
  return _gegl_buffer_cl_cache_flush2 (cache, roi);
}

gboolean
gegl_buffer_cl_cache_flush (GeglBuffer          *buffer,
                            const GeglRectangle *roi)
{
  return _gegl_buffer_cl_cache_flush2 (buffer->tile_storage->cache, roi);
}

void
gegl_buffer_cl_cache_invalidate (GeglBuffer          *buffer,
                                 const GeglRectangle *roi)
{
  gpointer data;

  g_rec_mutex_lock (&cache_mutex);

  for (GList *elem = cache_entries; elem != NULL; elem = elem->next)
    {
      CacheEntry *e = elem->data;
      if (e->valid && e->buffer == buffer &&
          (!roi || gegl_rectangle_intersect (NULL, roi, &e->roi)))
        {
          e->valid = FALSE;
        }
    }

  while (cache_entry_find_invalid (&data))
    free_cache_entry (data);

  g_rec_mutex_unlock (&cache_mutex);
}
