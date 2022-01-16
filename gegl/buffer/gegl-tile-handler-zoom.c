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

#include <string.h>

#include <babl/babl.h>
#include <glib-object.h>

#include "gegl-buffer.h"
#include "gegl-buffer-types.h"
#include "gegl-tile-handler.h"
#include "gegl-tile-handler-cache.h"
#include "gegl-tile-handler-private.h"
#include "gegl-tile-handler-zoom.h"
#include "gegl-tile-storage.h"
#include "gegl-buffer-private.h"
#include "gegl-algorithms.h"
#include "gegl-cpuaccel.h"


G_DEFINE_TYPE (GeglTileHandlerZoom, gegl_tile_handler_zoom,
               GEGL_TYPE_TILE_HANDLER)

static guint64 total_size = 0;

static void
downscale (GeglTileHandlerZoom *zoom,
           const Babl          *format,
           gint                 bpp,
           guchar              *src,
           guchar              *dest,
           gint                 stride,
           gint                 x,
           gint                 y,
           gint                 width,
           gint                 height,
           guint                damage,
           gint                 i)
{
  gint  n    = 1 << i;
  guint mask = (1 << n) - 1;

  if ((damage & mask) == mask)
    {
      if (src)
        {
          if (!zoom->downscale_2x2)
          {
#ifdef ARCH_X86_64
             GeglCpuAccelFlags cpu_accel = gegl_cpu_accel_get_support ();
             if (cpu_accel & GEGL_CPU_ACCEL_X86_64_V3)
               zoom->downscale_2x2 = gegl_downscale_2x2_get_fun_x86_64_v3 (format);
             else if (cpu_accel & GEGL_CPU_ACCEL_X86_64_V2)
               zoom->downscale_2x2 = gegl_downscale_2x2_get_fun_x86_64_v2 (format);
             else
#endif
             zoom->downscale_2x2 = gegl_downscale_2x2_get_fun_generic (format);
          }

          zoom->downscale_2x2 (format,
                               width, height,
                               src +   y      * stride +  x      * bpp, stride,
                               dest + (y / 2) * stride + (x / 2) * bpp, stride);
        }
      else
        {
          gint h = height / 2;
          gint n = (width / 2) * bpp;
          gint i;

          dest += (y / 2) * stride + (x / 2) * bpp;

          for (i = 0; i < h; i++)
            {
              memset (dest, 0, n);
              dest += stride;
            }
        }

      total_size += (width / 2) * (height / 2) * bpp;
    }
  else
    {
      i--;
      n    /=  2;
      mask >>= n;

      if (damage & mask)
        {
          if (i & 1)
            {
              downscale (zoom,
                         format, bpp, src, dest, stride,
                         x, y,
                         width, height / 2,
                         damage, i);
            }
          else
            {
              downscale (zoom,
                         format, bpp, src, dest, stride,
                         x, y,
                         width / 2, height,
                         damage, i);

            }
        }

      damage >>= n;

      if (damage & mask)
        {
          if (i & 1)
            {
              downscale (zoom,
                         format, bpp, src, dest, stride,
                         x, y + height / 2,
                         width, height / 2,
                         damage, i);
            }
          else
            {
              downscale (zoom,
                         format, bpp, src, dest, stride,
                         x + width / 2, y,
                         width / 2, height,
                         damage, i);
            }
        }
    }
}

static GeglTile *
get_tile (GeglTileSource *gegl_tile_source,
          gint            x,
          gint            y,
          gint            z)
{
  GeglTileSource      *source = ((GeglTileHandler *) gegl_tile_source)->source;
  GeglTileHandlerZoom *zoom   = (GeglTileHandlerZoom *) gegl_tile_source;
  GeglTile            *tile   = NULL;
  GeglTileStorage     *tile_storage;
  gint                 tile_width;
  gint                 tile_height;

  if (source)
    tile = gegl_tile_source_get_tile (source, x, y, z);

  if (z == 0 || (tile && ! tile->damage))
    return tile;

  tile_storage = _gegl_tile_handler_get_tile_storage ((GeglTileHandler *) zoom);

  if (z > tile_storage->seen_zoom)
    tile_storage->seen_zoom = z;

  tile_width = tile_storage->tile_width;
  tile_height = tile_storage->tile_height;

  {
    gint        i, j;
    const Babl *format;
    gint        bpp;
    gint        stride;
    guint64     damage;
    GeglTile   *source_tile[2][2] = { { NULL, NULL }, { NULL, NULL } };
    gboolean    empty             = TRUE;

    if (tile)
      damage = tile->damage;
    else
      damage = ~(guint64) 0;

    for (i = 0; i < 2; i++)
      for (j = 0; j < 2; j++)
        {
          if ((damage >> (32 * j + 16 * i)) & 0xffff)
            {
              /* clear the tile damage region before fetching each lower-level
               * tile, so that if this results in the corresponding portion of
               * the pyramid being voided, our damage region never covers the
               * entire tile, and we're not getting dropped from the cache.
               *
               * note that our damage region is cleared at the end of the
               * process by gegl_tile_unlock() anyway, so clearing it here is
               * harmless.
               */
              if (tile)
                tile->damage = 0;

              /* we get the tile from ourselves, to make successive rescales
               * work correctly */
              source_tile[i][j] = gegl_tile_source_get_tile (
                gegl_tile_source, x * 2 + i, y * 2 + j, z - 1);

              if (source_tile[i][j])
                {
                  if (source_tile[i][j]->is_zero_tile)
                    {
                      gegl_tile_unref (source_tile[i][j]);

                      source_tile[i][j] = NULL;
                    }
                  else
                    {
                      empty = FALSE;
                    }
                }
            }
          else
            {
              empty = FALSE;
            }
        }

    if (empty)
      {
        if (tile)
          gegl_tile_unref (tile);

        return NULL;   /* no data from level below, return NULL and let GeglTileHandlerEmpty
                          fill in the shared empty tile */
      }

    format = gegl_tile_backend_get_format (zoom->backend);
    bpp    = babl_format_get_bytes_per_pixel (format);
    stride = tile_width * bpp;

    if (! tile)
      tile = gegl_tile_handler_create_tile (GEGL_TILE_HANDLER (zoom), x, y, z);

    /* restore the original damage mask, so that fully-damaged tiles aren't
     * copied during uncloning.
     */
    tile->damage = damage;

    gegl_tile_lock (tile);

    for (i = 0; i < 2; i++)
      for (j = 0; j < 2; j++)
        {
          guint dmg = (damage >> (32 * j + 16 * i)) & 0xffff;

          if (dmg)
            {
              gint x = i * tile_width / 2;
              gint y = j * tile_height / 2;
              guchar *src;
              guchar *dest;

              if (source_tile[i][j])
                {
                  gegl_tile_read_lock (source_tile[i][j]);

                  src = gegl_tile_get_data (source_tile[i][j]);
                }
              else
                {
                  src = NULL;
                }

              dest = gegl_tile_get_data (tile) + y * stride + x * bpp;

              downscale (zoom,
                         format, bpp, src, dest, stride,
                         0, 0,
                         tile_width, tile_height,
                         dmg, 4);

              if (source_tile[i][j])
                {
                  gegl_tile_read_unlock (source_tile[i][j]);

                  gegl_tile_unref (source_tile[i][j]);
                }
            }
        }

    gegl_tile_unlock (tile);
  }

  return tile;
}

static gpointer
gegl_tile_handler_zoom_command (GeglTileSource  *tile_store,
                                GeglTileCommand  command,
                                gint             x,
                                gint             y,
                                gint             z,
                                gpointer         data)
{
  GeglTileHandler *handler  = (void*)tile_store;

  if (command == GEGL_TILE_GET)
    return get_tile (tile_store, x, y, z);
  else
    return gegl_tile_handler_source_command (handler, command, x, y, z, data);
}

static void
gegl_tile_handler_zoom_class_init (GeglTileHandlerZoomClass *klass)
{
}

static void
gegl_tile_handler_zoom_init (GeglTileHandlerZoom *self)
{
  ((GeglTileSource *) self)->command = gegl_tile_handler_zoom_command;
}

GeglTileHandler *
gegl_tile_handler_zoom_new (GeglTileBackend *backend)
{
  GeglTileHandlerZoom *ret = g_object_new (GEGL_TYPE_TILE_HANDLER_ZOOM, NULL);

  ret->backend = backend;

  return (void*)ret;
}

guint64
gegl_tile_handler_zoom_get_total (void)
{
  return total_size;
}

void
gegl_tile_handler_zoom_reset_stats (void)
{
  total_size = 0;
}
