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
 * Copyright 2006, 2007 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"

#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <glib-object.h>
#include <glib/gstdio.h>

#include "gegl-buffer.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer-private.h"
#include "gegl-debug.h"
#include "gegl-tile-storage.h"
#include "gegl-tile.h"
#include "gegl-buffer-index.h"

#ifdef G_OS_WIN32
#define BINARY_FLAG O_BINARY
#else
#define BINARY_FLAG 0
#endif

typedef struct
{
  GeglBufferHeader header;
  GList           *tiles;
  gchar           *path;
  gint             o;

  gint             tile_size;
  gint             offset;
  gint             entry_count;
  GeglBufferBlock *in_holding; /* we need to write one block added behind
                                * to be able to recompute the forward pointing
                                * link from one entry to the next.
                                */
} SaveInfo;


GeglBufferTile *
gegl_tile_entry_new (gint x,
                     gint y,
                     gint z)
{
  GeglBufferTile *entry = g_malloc0 (sizeof(GeglBufferTile));
  entry->block.flags = GEGL_FLAG_TILE;
  entry->block.length = sizeof (GeglBufferTile);

  entry->x = x;
  entry->y = y;
  entry->z = z;
  return entry;
}

void
gegl_tile_entry_destroy (GeglBufferTile *entry)
{
  g_free (entry);
}

static gsize write_block (SaveInfo        *info,
                          GeglBufferBlock *block)
{
   gssize ret = 0;

   if (info->in_holding)
     {
       glong allocated_pos = info->offset + info->in_holding->length;
       info->in_holding->next = allocated_pos;

       if (block == NULL)
         info->in_holding->next = 0;

       ret = write (info->o, info->in_holding, info->in_holding->length);
       if (ret == -1)
         ret = 0;
       info->offset += ret;
       g_assert (allocated_pos == info->offset);
     }
  /* write block should also allocate the block and update the
   * previously added blocks next pointer
   */
   info->in_holding = block;
   return ret;
}

static void
save_info_destroy (SaveInfo *info)
{
  if (!info)
    return;
  if (info->path)
    g_free (info->path);
  if (info->o != -1)
    close (info->o);
  if (info->tiles != NULL)
    {
      GList *iter;
      for (iter = info->tiles; iter; iter = iter->next)
        gegl_tile_entry_destroy (iter->data);
      g_list_free (info->tiles);
      info->tiles = NULL;
    }
  g_slice_free (SaveInfo, info);
}



static glong z_order (const GeglBufferTile *entry)
{
  glong value;

  gint  i;
  gint  srcA = entry->x;
  gint  srcB = entry->y;
  gint  srcC = entry->z;

  /* interleave the 10 least significant bits of all coordinates,
   * this gives us Z-order / morton order of the space and should
   * work well as a hash
   */
  value = 0;
  for (i = 20; i >= 0; i--)
    {
#define ADD_BIT(bit)    do { value |= (((bit) != 0) ? 1 : 0); value <<= 1; \
    } \
  while (0)
      ADD_BIT (srcA & (1 << i));
      ADD_BIT (srcB & (1 << i));
      ADD_BIT (srcC & (1 << i));
#undef ADD_BIT
    }
  return value;
}

static gint z_order_compare (gconstpointer a,
                             gconstpointer b)
{
  const GeglBufferTile *entryA = a;
  const GeglBufferTile *entryB = b;

  return z_order (entryB) - z_order (entryA);
}


void
gegl_buffer_header_init (GeglBufferHeader *header,
                         gint              tile_width,
                         gint              tile_height,
                         gint              bpp,
                         const Babl*       format)
{
  memcpy (header->magic, "GEGL", 4);

  header->flags = GEGL_FLAG_HEADER;
  header->tile_width  = tile_width;
  header->tile_height = tile_height;
  header->bytes_per_pixel = bpp;
  {
    gchar buf[64] = { 0, };

    g_snprintf (buf, 64, "%s%c\n%i×%i %ibpp\n%ix%i\n\n\n\n\n\n\n\n\n",
          babl_format_get_encoding (format), 0,
          header->tile_width,
          header->tile_height,
          header->bytes_per_pixel,
          (gint)header->width,
          (gint)header->height);
    memcpy ((header->description), buf, 64);


    if (strcmp (babl_get_name (format), babl_format_get_encoding (format)))
    {
      g_warning ("storing a geglbuffer with non sRGB space, we should store the space in a separate ICC block.");
    }
  }
}

void
gegl_buffer_save (GeglBuffer          *buffer,
                  const gchar         *path,
                  const GeglRectangle *roi)
{
  SaveInfo *info = g_slice_new0 (SaveInfo);

  glong prediction = 0;
  gint bpp;
  gint tile_width;
  gint tile_height;

  GEGL_BUFFER_SANITY;

  if (! roi)
    roi = &buffer->extent;

  GEGL_NOTE (GEGL_DEBUG_BUFFER_SAVE,
             "starting to save buffer %s, roi: %d,%d %dx%d",
             path, roi->x, roi->y, roi->width, roi->height);

  /* a header should follow the same structure as a blockdef with
   * respect to the flags and next offsets, thus this is a valid
   * cast shortcut.
   */

  info->path = g_strdup (path);

#ifndef G_OS_WIN32
  info->o    = g_open (info->path, O_RDWR|O_CREAT|O_TRUNC|BINARY_FLAG, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
#else
  info->o    = g_open (info->path, O_RDWR|O_CREAT|O_TRUNC|BINARY_FLAG, S_IRUSR|S_IWUSR);
#endif


  if (info->o == -1)
    g_warning ("%s: Could not open '%s': %s", G_STRFUNC, info->path, g_strerror(errno));
  tile_width  = buffer->tile_storage->tile_width;
  tile_height = buffer->tile_storage->tile_height;
  g_object_get (buffer, "px-size", &bpp, NULL);

  info->header.x           = roi->x;
  info->header.y           = roi->y;
  info->header.width       = roi->width;
  info->header.height      = roi->height;
  gegl_buffer_header_init (&info->header,
                           tile_width,
                           tile_height,
                           bpp,
                           buffer->tile_storage->format
                           );
  info->header.next = (prediction += sizeof (GeglBufferHeader));
  info->tile_size = tile_width * tile_height * bpp;

  g_assert (info->tile_size % 16 == 0);

  GEGL_NOTE (GEGL_DEBUG_BUFFER_SAVE,
             "collecting list of tiles to be written");
  {
    gint z;
    gint factor = 1;

    for (z = 0; z < 1; z++)
      {
        gint bufy = roi->y;
        while (bufy < roi->y + roi->height)
          {
            gint tiledy  = roi->y + bufy;
            gint offsety = gegl_tile_offset (tiledy, tile_height);
            gint bufx    = roi->x;

            while (bufx < roi->x + roi->width)
              {
                gint tiledx  = roi->x + bufx;
                gint offsetx = gegl_tile_offset (tiledx, tile_width);

                gint tx = gegl_tile_indice (tiledx / factor, tile_width);
                gint ty = gegl_tile_indice (tiledy / factor, tile_height);

                if (gegl_tile_source_exist (GEGL_TILE_SOURCE (buffer), tx, ty, z))
                  {
                    GeglBufferTile *entry;

                    GEGL_NOTE (GEGL_DEBUG_BUFFER_SAVE,
                               "Found tile to save, tx, ty, z = %d, %d, %d",
                               tx, ty, z);

                    entry = gegl_tile_entry_new (tx, ty, z);
                    info->tiles = g_list_prepend (info->tiles, entry);
                    info->entry_count++;
                  }
                bufx += (tile_width - offsetx) * factor;
              }
            bufy += (tile_height - offsety) * factor;
          }
        factor *= 2;
      }
  GEGL_NOTE (GEGL_DEBUG_BUFFER_SAVE,
             "size of list of tiles to be written: %d",
             g_list_length (info->tiles));
  }

  /* sort the list of tiles into zorder */
  info->tiles = g_list_sort (info->tiles, z_order_compare);

  /* set the offset in the file each tile will be stored on */
  {
    GList *iter;
    gint   predicted_offset = sizeof (GeglBufferHeader) +
                              sizeof (GeglBufferTile) * (info->entry_count);
    for (iter = info->tiles; iter; iter = iter->next)
      {
        GeglBufferTile *entry = iter->data;
        entry->block.next = iter->next?
                            (prediction += sizeof (GeglBufferTile)):0;
        entry->offset = predicted_offset;
        predicted_offset += info->tile_size;
      }
  }

  /* save the header */
  {
    ssize_t ret = write (info->o, &info->header, sizeof (GeglBufferHeader));
    if (ret != -1)
      info->offset += ret;
  }
  g_assert (info->offset == info->header.next);

  /* save the index */
  {
    GList *iter;
    for (iter = info->tiles; iter; iter = iter->next)
      {
        GeglBufferItem *item = iter->data;

        write_block (info, &item->block);

      }
  }
  write_block (info, NULL); /* terminate the index */

  /* update header to point to start of new index (already done for
   * this serial saver, and the header is already written.
   */

  /* save each tile */
  {
    GList *iter;
    gint   i = 0;
    for (iter = info->tiles; iter; iter = iter->next)
      {
        GeglBufferTile *entry = iter->data;
        guchar          *data;
        GeglTile        *tile;

        tile = gegl_tile_source_get_tile (GEGL_TILE_SOURCE (buffer),
                                          entry->x,
                                          entry->y,
                                          entry->z,
					  GEGL_TILE_GET_READ);
        g_assert (tile);
        data = gegl_tile_get_data (tile);
        g_assert (data);

        g_assert (info->offset == entry->offset);
        {
          ssize_t ret = write (info->o, data, info->tile_size);
          if (ret != -1)
            info->offset += ret;
        }
        gegl_tile_unref (tile);
        i++;
      }
  }
  save_info_destroy (info);
}
