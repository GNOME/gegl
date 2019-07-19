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
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "gegl-memory.h"
#include "gegl-memory-private.h"


G_STATIC_ASSERT (GEGL_ALIGNMENT <= G_MAXUINT8);


/*  public functions  */

/* utility call that makes sure allocations are 16 byte aligned.
 * making RGBA float buffers have aligned access for pixels.
 */
static inline gpointer
gegl_malloc_align (gchar *mem)
{
  gchar *ret;
  gint   offset;

  offset = GEGL_ALIGNMENT - GPOINTER_TO_UINT (mem) % GEGL_ALIGNMENT;
  ret    = (gpointer) (mem + offset);

  /* store the offset to the real malloc one byte in front of this malloc */
  *(guint8 *) (ret - 1) = offset;

  return (gpointer) ret;
}

gpointer
gegl_malloc (gsize size)
{
  gchar *mem;

  mem = g_malloc (size + GEGL_ALIGNMENT);

  return gegl_malloc_align (mem);
}

gpointer
gegl_try_malloc (gsize size)
{
  gchar *mem;

  mem = g_try_malloc (size + GEGL_ALIGNMENT);

  if (! mem)
    return NULL;

  return gegl_malloc_align (mem);
}

gpointer
gegl_calloc (gsize size,
             gint  n_memb)
{
  gchar *ret = gegl_malloc (size * n_memb);
  memset (ret, 0, size * n_memb);
  return ret;
}

void
gegl_free (gpointer buf)
{
  if (! buf)
    return;

  g_free ((gchar*)buf - *((guint8*)buf -1));
}

gboolean
gegl_memeq_zero (gconstpointer ptr,
                 gsize         size)
{
  const guint8 *p = ptr;

  if (size >= 1 && (guintptr) p & 0x1)
    {
      if (*(const guint8 *) p)
        return FALSE;

      p    += 1;
      size -= 1;
    }

  if (size >= 2 && (guintptr) p & 0x2)
    {
      if (*(const guint16 *) p)
        return FALSE;

      p    += 2;
      size -= 2;
    }

  if (size >= 4 && (guintptr) p & 0x4)
    {
      if (*(const guint32 *) p)
        return FALSE;

      p    += 4;
      size -= 4;
    }

  while (size >= 8)
    {
      if (*(const guint64 *) p)
        return FALSE;

      p    += 8;
      size -= 8;
    }

  if (size >= 4)
    {
      if (*(const guint32 *) p)
        return FALSE;

      p    += 4;
      size -= 4;
    }

  if (size >= 2)
    {
      if (*(const guint16 *) p)
        return FALSE;

      p    += 2;
      size -= 2;
    }

  if (size >= 1)
    {
      if (*(const guint8 *) p)
        return FALSE;

      p    += 1;
      size -= 1;
    }

  return TRUE;
}

void
gegl_memset_pattern (gpointer      restrict dst_ptr,
                     gconstpointer restrict src_ptr,
                     gint                   pattern_size,
                     gint                   count)
{
  guchar       *dst = dst_ptr;
  const guchar *src = src_ptr;

  /* g_assert (pattern_size > 0 && count >= 0); */

  if (pattern_size == 1 || count == 0)
    {
      memset (dst, *src, count);
    }
  else
    {
      gsize block_size;
      gsize remaining_size;

      block_size = pattern_size,

      memcpy (dst, src, block_size);
      src  = dst;
      dst += block_size;

      remaining_size = (count - 1) * block_size;

      while (block_size < remaining_size)
        {
          memcpy (dst, src, block_size);
          dst += block_size;

          remaining_size -= block_size;

          /* limit the block size, so that we don't saturate the cache.
           *
           * FIXME: optimal limit could use more benchmarking.
           */
          if (block_size <= 2048)
            block_size *= 2;
        }

      memcpy (dst, src, remaining_size);
    }
}
