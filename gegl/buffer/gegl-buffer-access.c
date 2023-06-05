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
 * Copyright 2006-2012,2014-2017  Øyvind Kolås <pippin@gimp.org>
 *           2013 Daniel Sabo
 */

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#include <glib-object.h>
#include <glib/gprintf.h>
#include <gio/gio.h>

#include <babl/babl.h>
#include "gegl-algorithms.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-storage.h"
#include "gegl-tile-handler-empty.h"
#include "gegl-sampler.h"
#include "gegl-tile-backend.h"
#include "gegl-buffer-iterator.h"
#include "gegl-rectangle.h"
#include "gegl-buffer-iterator-private.h"
#include "gegl-buffer-formats.h"

static void gegl_buffer_iterate_read_fringed (GeglBuffer          *buffer,
                                              const GeglRectangle *roi,
                                              const GeglRectangle *abyss,
                                              guchar              *buf,
                                              gint                 buf_stride,
                                              const Babl          *format,
                                              gint                 level,
                                              GeglAbyssPolicy      repeat_mode);


static void gegl_buffer_iterate_read_dispatch (GeglBuffer          *buffer,
                                               const GeglRectangle *roi,
                                               guchar              *buf,
                                               gint                 rowstride,
                                               const Babl          *format,
                                               gint                 level,
                                               GeglAbyssPolicy      repeat_mode);

static inline void
gegl_buffer_get_pixel (GeglBuffer     *buffer,
                       gint            x,
                       gint            y,
                       const Babl     *format,
                       gpointer        data,
                       GeglAbyssPolicy repeat_mode)
{
  const GeglRectangle *abyss = &buffer->abyss;
  guchar              *buf   = data;

  if (G_UNLIKELY (y <  abyss->y ||
      x <  abyss->x ||
      y >= abyss->y + abyss->height ||
      x >= abyss->x + abyss->width))
    {
      switch (repeat_mode)
      {
        case GEGL_ABYSS_CLAMP:
          x = CLAMP (x, abyss->x, abyss->x+abyss->width-1);
          y = CLAMP (y, abyss->y, abyss->y+abyss->height-1);
          break;

        case GEGL_ABYSS_LOOP:
          x = abyss->x + GEGL_REMAINDER (x - abyss->x, abyss->width);
          y = abyss->y + GEGL_REMAINDER (y - abyss->y, abyss->height);
          break;

        case GEGL_ABYSS_BLACK:
          {
            gfloat color[4] = {0.0, 0.0, 0.0, 1.0};
            babl_process (babl_fish (gegl_babl_rgba_linear_float (), format),
                          color,
                          buf,
                          1);
            return;
          }

        case GEGL_ABYSS_WHITE:
          {
            gfloat color[4] = {1.0, 1.0, 1.0, 1.0};
            babl_process (babl_fish (gegl_babl_rgba_linear_float (),
                                     format),
                          color,
                          buf,
                          1);
            return;
          }

        default:
        case GEGL_ABYSS_NONE:
          memset (buf, 0x00, babl_format_get_bytes_per_pixel (format));
          return;
      }
    }

  {
    gint tile_width  = buffer->tile_width;
    gint tile_height = buffer->tile_height;
    gint tiledy      = y + buffer->shift_y;
    gint tiledx      = x + buffer->shift_x;
    gint indice_x    = gegl_tile_indice (tiledx, tile_width);
    gint indice_y    = gegl_tile_indice (tiledy, tile_height);

    GeglTile *tile = gegl_tile_storage_steal_hot_tile (buffer->tile_storage);

    if (!(tile &&
          tile->x == indice_x &&
          tile->y == indice_y))
      {
        g_rec_mutex_lock (&buffer->tile_storage->mutex);

        if (tile)
          gegl_tile_unref (tile);

        tile = gegl_tile_source_get_tile ((GeglTileSource *) (buffer),
                                          indice_x, indice_y,
                                          0);

        g_rec_mutex_unlock (&buffer->tile_storage->mutex);
      }

    if (tile)
      {
        gint tile_origin_x = indice_x * tile_width;
        gint tile_origin_y = indice_y * tile_height;
        gint       offsetx = tiledx - tile_origin_x;
        gint       offsety = tiledy - tile_origin_y;
        gint px_size = babl_format_get_bytes_per_pixel (buffer->soft_format);
        guchar    *tp;

        gegl_tile_read_lock (tile);

        tp = gegl_tile_get_data (tile) +
             (offsety * tile_width + offsetx) * px_size;

        if (G_UNLIKELY (format != buffer->soft_format))
          {
            babl_process (babl_fish (buffer->soft_format, format), tp, buf, 1);
          }
        else
          {
            memcpy (buf, tp, px_size);
          }

        gegl_tile_read_unlock (tile);

        gegl_tile_storage_take_hot_tile (buffer->tile_storage, tile);
      }
  }
}

static inline void
__gegl_buffer_set_pixel (GeglBuffer     *buffer,
                       gint            x,
                       gint            y,
                       const Babl     *format,
                       gconstpointer   data)
{
  const GeglRectangle *abyss = &buffer->abyss;
  const guchar        *buf   = data;

  if (G_UNLIKELY (y <  abyss->y ||
      x <  abyss->x ||
      y >= abyss->y + abyss->height ||
      x >= abyss->x + abyss->width))
    return;


  {
    gint tile_width  = buffer->tile_width;
    gint tile_height = buffer->tile_height;
    gint tiledy      = y + buffer->shift_y;
    gint tiledx      = x + buffer->shift_x;
    gint indice_x    = gegl_tile_indice (tiledx, tile_width);
    gint indice_y    = gegl_tile_indice (tiledy, tile_height);

    GeglTile *tile = gegl_tile_storage_steal_hot_tile (buffer->tile_storage);
    const Babl *fish = NULL;
    gint px_size;

    if (G_UNLIKELY (format != buffer->soft_format))
      {
        fish    = babl_fish (format, buffer->soft_format);
        px_size = babl_format_get_bytes_per_pixel (buffer->soft_format);
      }
    else
      {
        px_size = babl_format_get_bytes_per_pixel (buffer->soft_format);
      }

    if (!(tile &&
          tile->x == indice_x &&
          tile->y == indice_y))
      {
        g_rec_mutex_lock (&buffer->tile_storage->mutex);

        if (tile)
          gegl_tile_unref (tile);

        tile = gegl_tile_source_get_tile ((GeglTileSource *) (buffer),
                                          indice_x, indice_y,
                                          0);

        g_rec_mutex_unlock (&buffer->tile_storage->mutex);
      }

    if (tile)
      {
        gint tile_origin_x = indice_x * tile_width;
        gint tile_origin_y = indice_y * tile_height;
        gint       offsetx = tiledx - tile_origin_x;
        gint       offsety = tiledy - tile_origin_y;

        guchar *tp;
        gegl_tile_lock (tile);
        tp = gegl_tile_get_data (tile) + (offsety * tile_width + offsetx) * px_size;

        if (fish)
          babl_process (fish, buf, tp, 1);
        else
          memcpy (tp, buf, px_size);

        gegl_tile_unlock (tile);

        gegl_tile_storage_take_hot_tile (buffer->tile_storage, tile);
      }
  }

}

enum _GeglBufferSetFlag {
  GEGL_BUFFER_SET_FLAG_FAST   = 0,
  GEGL_BUFFER_SET_FLAG_LOCK   = 1<<0,
  GEGL_BUFFER_SET_FLAG_NOTIFY = 1<<1,
  GEGL_BUFFER_SET_FLAG_FULL   = GEGL_BUFFER_SET_FLAG_LOCK |
                                GEGL_BUFFER_SET_FLAG_NOTIFY
};

typedef enum _GeglBufferSetFlag GeglBufferSetFlag;

void
gegl_buffer_set_with_flags (GeglBuffer          *buffer,
                            const GeglRectangle *rect,
                            gint                 level,
                            const Babl          *format,
                            const void          *src,
                            gint                 rowstride,
                            GeglBufferSetFlag    set_flags);


static inline void
_gegl_buffer_set_pixel (GeglBuffer       *buffer,
                        gint              x,
                        gint              y,
                        const Babl       *format,
                        gconstpointer     data,
                        GeglBufferSetFlag flags)
{
  GeglRectangle rect = {x,y,1,1};
  switch (flags)
  {
    case GEGL_BUFFER_SET_FLAG_FAST:
      __gegl_buffer_set_pixel (buffer, x, y, format, data);
    break;
    case GEGL_BUFFER_SET_FLAG_LOCK:
      gegl_buffer_lock (buffer);
      __gegl_buffer_set_pixel (buffer, x, y, format, data);
      gegl_buffer_unlock (buffer);
      break;
    case GEGL_BUFFER_SET_FLAG_NOTIFY:
      __gegl_buffer_set_pixel (buffer, x, y, format, data);
      gegl_buffer_emit_changed_signal (buffer, &rect);
      break;
    case GEGL_BUFFER_SET_FLAG_LOCK|GEGL_BUFFER_SET_FLAG_NOTIFY:
    default:
      gegl_buffer_lock (buffer);
      __gegl_buffer_set_pixel (buffer, x, y, format, data);
      gegl_buffer_unlock (buffer);
      gegl_buffer_emit_changed_signal (buffer, &rect);
      break;
  }
}

/* flush any unwritten data (flushes the hot-cache of a single
 * tile used by gegl_buffer_set for 1x1 pixel sized rectangles
 */
void
gegl_buffer_flush (GeglBuffer *buffer)
{
  GeglTileBackend *backend;

  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  backend = gegl_buffer_backend (buffer);

  g_rec_mutex_lock (&buffer->tile_storage->mutex);

  _gegl_buffer_drop_hot_tile (buffer);

  if (backend)
    gegl_tile_backend_set_extent (backend, &buffer->extent);

  gegl_tile_source_command (GEGL_TILE_SOURCE (buffer),
                            GEGL_TILE_FLUSH, 0,0,0,NULL);

  g_rec_mutex_unlock (&buffer->tile_storage->mutex);
}

void
gegl_buffer_flush_ext (GeglBuffer *buffer, const GeglRectangle *rect)
{
  if (gegl_buffer_ext_flush)
    gegl_buffer_ext_flush (buffer, rect);
}



static inline void
gegl_buffer_iterate_write (GeglBuffer          *buffer,
                           const GeglRectangle *roi,
                           guchar              *buf,
                           gint                 rowstride,
                           const Babl          *format,
                           gint                 level)
{
  gint tile_width  = buffer->tile_storage->tile_width;
  gint tile_height = buffer->tile_storage->tile_height;
  gint px_size     = babl_format_get_bytes_per_pixel (buffer->soft_format);
  gint bpx_size    = babl_format_get_bytes_per_pixel (format);
  gint tile_stride = px_size * tile_width;
  gint buf_stride;
  gint bufy        = 0;

  gint buffer_shift_x = buffer->shift_x;
  gint buffer_shift_y = buffer->shift_y;

  gint width    = buffer->extent.width;
  gint height   = buffer->extent.height;
  gint buffer_x = buffer->extent.x + buffer_shift_x;
  gint buffer_y = buffer->extent.y + buffer_shift_y;

  gint buffer_abyss_x = buffer->abyss.x + buffer_shift_x;
  gint buffer_abyss_y = buffer->abyss.y + buffer_shift_y;
  gint abyss_x_total  = buffer_abyss_x + buffer->abyss.width;
  gint abyss_y_total  = buffer_abyss_y + buffer->abyss.height;
  gint factor         = 1<<level;
  const Babl *fish;
  GeglRectangle scaled_rect;
  if (G_UNLIKELY (level && roi))
  {
    scaled_rect = *roi;
    scaled_rect.x <<= level;
    scaled_rect.y <<= level;
    scaled_rect.width <<= level;
    scaled_rect.height <<= level;
    roi = &scaled_rect;
  }

  /* roi specified, override buffers extent */
  if (roi)
    {
      width  = roi->width;
      height = roi->height;
      buffer_x = roi->x + buffer_shift_x;
      buffer_y = roi->y + buffer_shift_y;
    }

  buffer_shift_x /= factor;
  buffer_shift_y /= factor;
  buffer_abyss_x /= factor;
  buffer_abyss_y /= factor;
  abyss_x_total  /= factor;
  abyss_y_total  /= factor;
  buffer_x       /= factor;
  buffer_y       /= factor;
  width          /= factor;
  height         /= factor;


  buf_stride = width * bpx_size;
  if (rowstride != GEGL_AUTO_ROWSTRIDE)
    buf_stride = rowstride;

  if (G_LIKELY (format == buffer->soft_format))
    {
      fish = NULL;
    }
  else
      fish = babl_fish ((gpointer) format,
                        (gpointer) buffer->soft_format);

  while (bufy < height)
    {
      gint tiledy  = buffer_y + bufy;
      gint offsety = gegl_tile_offset (tiledy, tile_height);
      gint bufx    = 0;

      while (bufx < width)
        {
          gint      tiledx  = buffer_x + bufx;
          gint      offsetx = gegl_tile_offset (tiledx, tile_width);
          gint      y       = bufy;
          gint      index_x;
          gint      index_y;
          gint      lskip, rskip, pixels, row;
          guchar   *bp, *tile_base, *tp;
          GeglTile *tile;
          gboolean  whole_tile;

          bp = buf + (gsize) bufy * buf_stride + bufx * bpx_size;

          if (G_UNLIKELY (width + offsetx - bufx < tile_width))
            pixels = (width + offsetx - bufx) - offsetx;
          else
            pixels = tile_width - offsetx;

          index_x = gegl_tile_indice (tiledx, tile_width);
          index_y = gegl_tile_indice (tiledy, tile_height);

          lskip = (buffer_abyss_x) - (buffer_x + bufx);
          /* gap between left side of tile, and abyss */
          rskip = (buffer_x + bufx + pixels) - abyss_x_total;
          /* gap between right side of tile, and abyss */

          if (G_UNLIKELY (lskip < 0))
            lskip = 0;
          if (G_UNLIKELY (lskip > pixels))
            lskip = pixels;
          if (G_UNLIKELY (rskip < 0))
            rskip = 0;
          if (G_UNLIKELY (rskip > pixels))
            rskip = pixels;

          pixels -= lskip;
          pixels -= rskip;

          whole_tile = pixels == tile_width && bufy >= buffer_abyss_y &&
                       MIN (MIN (height - bufy, tile_height - offsety),
                            abyss_y_total - bufy) == tile_height;

          g_rec_mutex_lock (&buffer->tile_storage->mutex);

          tile = gegl_tile_handler_get_tile ((GeglTileHandler *) buffer,
                                             index_x, index_y, level,
                                             ! whole_tile);

          g_rec_mutex_unlock (&buffer->tile_storage->mutex);

          if (!tile)
            {
              g_warning ("didn't get tile, trying to continue");
              bufx += (tile_width - offsetx);
              continue;
            }

          gegl_tile_lock (tile);

          tile_base = gegl_tile_get_data (tile);
          tp        = ((guchar *) tile_base) + (offsety * tile_width + offsetx) * px_size;

          if (G_UNLIKELY (fish))
            {
              int bskip, skip = 0;
              int rows = MIN(height - bufy, tile_height - offsety);

              bskip = (buffer_y + bufy + rows) - abyss_y_total;
              bskip = CLAMP (bskip, 0, rows);
              rows -= bskip;

/*
XXX XXX XXX
Making the abyss geometry handling work with babl_process_rows is proving a bit
tricky - the choice that skips the skipping at the start of a batch entering a
tile seem to let all expected code paths work and no reported non-working code
paths yet. This will also be a slight performance loss - but it might be that
we seldom do writes that fall in the abyss anyways.

Not writing into the abyss will permit better control over sliced rendering
with multi-threading - and should be added back later.

*/

#if 0
              skip = buffer_abyss_y - bufy;
              if (skip < 0)
                skip = 0;
              rows-=skip;
#endif
              if (rows==1)
                babl_process (fish,bp + lskip * bpx_size + skip * buf_stride, tp + lskip * px_size + skip * tile_stride, pixels);
              else if (rows>0)
                babl_process_rows (fish,
                                   bp + lskip * bpx_size + skip * buf_stride,
                                   buf_stride,
                                   tp + lskip * px_size + skip * tile_stride,
                                   tile_stride,
                                   pixels,
                                   rows);
            }
          else
            {
              int lskip_offset = lskip * px_size;

              #ifdef __GNUC__
              #define CHECK_ALIGNMENT_ALIGNOF __alignof__
              #else
              #define CHECK_ALIGNMENT_ALIGNOF(type) \
                (G_STRUCT_OFFSET (struct alignof_helper, b))
              #endif
              #define CHECK_ALIGNMENT(type)                                    \
                do                                                             \
                  {                                                            \
                    struct alignof_helper { char a; type b; };                 \
                    /* verify 'alignof (type)' is a power of 2 */              \
                    G_STATIC_ASSERT (! (CHECK_ALIGNMENT_ALIGNOF (type) &       \
                                        (CHECK_ALIGNMENT_ALIGNOF (type) - 1)));\
                    if ((((uintptr_t) tp + lskip_offset) |                     \
                         ((uintptr_t) bp + lskip_offset) |                     \
                         tile_stride                     |                     \
                         buf_stride) &                                         \
                        (CHECK_ALIGNMENT_ALIGNOF (type) - 1))                  \
                      {                                                        \
                        goto unaligned;                                        \
                      }                                                        \
                  }                                                            \
                while (FALSE)

              switch (pixels * px_size)
                {
                  case 1:
                    CHECK_ALIGNMENT (guchar);
                    for (row = offsety;
                         row < tile_height && y < height;
                         row++, y++)
                    {
                      if (buffer_y + y >= buffer_abyss_y &&
                          buffer_y + y < abyss_y_total)
                        {
                          tp[lskip_offset] = bp[lskip_offset];
                        }
                      tp += tile_stride;
                      bp += buf_stride;
                    }
                    break;
                  case 2:
                    CHECK_ALIGNMENT (uint16_t);
                    for (row = offsety;
                         row < tile_height && y < height;
                         row++, y++)
                    {
                      if (buffer_y + y >= buffer_abyss_y &&
                          buffer_y + y < abyss_y_total)
                        {
                          ((uint16_t*)(&tp[lskip_offset]))[0] =
                          ((uint16_t*)(&bp[lskip_offset]))[0];
                        }
                      tp += tile_stride;
                      bp += buf_stride;
                    }
                    break;
                  case 3:
                    CHECK_ALIGNMENT (guchar);
                    for (row = offsety;
                         row < tile_height && y < height;
                         row++, y++)
                    {
                      if (buffer_y + y >= buffer_abyss_y &&
                          buffer_y + y < abyss_y_total)
                        {
                          tp[lskip_offset] = bp[lskip_offset];
                          tp[lskip_offset+1] = bp[lskip_offset+1];
                          tp[lskip_offset+2] = bp[lskip_offset+2];
                        }
                      tp += tile_stride;
                      bp += buf_stride;
                    }
                    break;
                  case 4:
                    CHECK_ALIGNMENT (uint32_t);
                    for (row = offsety;
                         row < tile_height && y < height;
                         row++, y++)
                    {
                      if (buffer_y + y >= buffer_abyss_y &&
                          buffer_y + y < abyss_y_total)
                        {
                          ((uint32_t*)(&tp[lskip_offset]))[0] =
                          ((uint32_t*)(&bp[lskip_offset]))[0];
                        }
                      tp += tile_stride;
                      bp += buf_stride;
                    }
                    break;
                  case 6:
                    CHECK_ALIGNMENT (uint16_t);
                    for (row = offsety;
                         row < tile_height && y < height;
                         row++, y++)
                    {
                      if (buffer_y + y >= buffer_abyss_y &&
                          buffer_y + y < abyss_y_total)
                        {
                          ((uint16_t*)(&tp[lskip_offset]))[0] =
                          ((uint16_t*)(&bp[lskip_offset]))[0];
                          ((uint16_t*)(&tp[lskip_offset]))[1] =
                          ((uint16_t*)(&bp[lskip_offset]))[1];
                          ((uint16_t*)(&tp[lskip_offset]))[2] =
                          ((uint16_t*)(&bp[lskip_offset]))[2];
                        }
                      tp += tile_stride;
                      bp += buf_stride;
                    }
                    break;
                  case 8:
                    CHECK_ALIGNMENT (uint64_t);
                    for (row = offsety;
                         row < tile_height && y < height;
                         row++, y++)
                    {
                      if (buffer_y + y >= buffer_abyss_y &&
                          buffer_y + y < abyss_y_total)
                        {
                          ((uint64_t*)(&tp[lskip_offset]))[0] =
                          ((uint64_t*)(&bp[lskip_offset]))[0];
                        }
                      tp += tile_stride;
                      bp += buf_stride;
                    }
                    break;
                  case 12:
                    CHECK_ALIGNMENT (uint32_t);
                    for (row = offsety;
                         row < tile_height && y < height;
                         row++, y++)
                    {
                      if (buffer_y + y >= buffer_abyss_y &&
                          buffer_y + y < abyss_y_total)
                        {
                          ((uint32_t*)(&tp[lskip_offset]))[0] =
                          ((uint32_t*)(&bp[lskip_offset]))[0];
                          ((uint32_t*)(&tp[lskip_offset]))[1] =
                          ((uint32_t*)(&bp[lskip_offset]))[1];
                          ((uint32_t*)(&tp[lskip_offset]))[2] =
                          ((uint32_t*)(&bp[lskip_offset]))[2];
                        }
                      tp += tile_stride;
                      bp += buf_stride;
                    }
                    break;
                  case 16:
                    CHECK_ALIGNMENT (uint64_t);
                    for (row = offsety;
                         row < tile_height && y < height;
                         row++, y++)
                    {
                      if (buffer_y + y >= buffer_abyss_y &&
                          buffer_y + y < abyss_y_total)
                        {
                          ((uint64_t*)(&tp[lskip_offset]))[0] =
                          ((uint64_t*)(&bp[lskip_offset]))[0];
                          ((uint64_t*)(&tp[lskip_offset]))[1] =
                          ((uint64_t*)(&bp[lskip_offset]))[1];
                        }
                      tp += tile_stride;
                      bp += buf_stride;
                    }
                    break;
                  case 24:
                    CHECK_ALIGNMENT (uint64_t);
                    for (row = offsety;
                         row < tile_height && y < height;
                         row++, y++)
                    {
                      if (buffer_y + y >= buffer_abyss_y &&
                          buffer_y + y < abyss_y_total)
                        {
                          ((uint64_t*)(&tp[lskip_offset]))[0] =
                          ((uint64_t*)(&bp[lskip_offset]))[0];
                          ((uint64_t*)(&tp[lskip_offset]))[1] =
                          ((uint64_t*)(&bp[lskip_offset]))[1];
                          ((uint64_t*)(&tp[lskip_offset]))[2] =
                          ((uint64_t*)(&bp[lskip_offset]))[2];
                        }
                      tp += tile_stride;
                      bp += buf_stride;
                    }
                    break;
                  case 32:
                    CHECK_ALIGNMENT (uint64_t);
                    for (row = offsety;
                         row < tile_height && y < height;
                         row++, y++)
                    {
                      if (buffer_y + y >= buffer_abyss_y &&
                          buffer_y + y < abyss_y_total)
                        {
                          ((uint64_t*)(&tp[lskip_offset]))[0] =
                          ((uint64_t*)(&bp[lskip_offset]))[0];
                          ((uint64_t*)(&tp[lskip_offset]))[1] =
                          ((uint64_t*)(&bp[lskip_offset]))[1];
                          ((uint64_t*)(&tp[lskip_offset]))[2] =
                          ((uint64_t*)(&bp[lskip_offset]))[2];
                          ((uint64_t*)(&tp[lskip_offset]))[3] =
                          ((uint64_t*)(&bp[lskip_offset]))[3];
                        }
                      tp += tile_stride;
                      bp += buf_stride;
                    }
                    break;
                  case 40:
                    CHECK_ALIGNMENT (uint64_t);
                    for (row = offsety;
                         row < tile_height && y < height;
                         row++, y++)
                    {
                      if (buffer_y + y >= buffer_abyss_y &&
                          buffer_y + y < abyss_y_total)
                        {
                          ((uint64_t*)(&tp[lskip_offset]))[0] =
                          ((uint64_t*)(&bp[lskip_offset]))[0];
                          ((uint64_t*)(&tp[lskip_offset]))[1] =
                          ((uint64_t*)(&bp[lskip_offset]))[1];
                          ((uint64_t*)(&tp[lskip_offset]))[2] =
                          ((uint64_t*)(&bp[lskip_offset]))[2];
                          ((uint64_t*)(&tp[lskip_offset]))[3] =
                          ((uint64_t*)(&bp[lskip_offset]))[3];
                          ((uint64_t*)(&tp[lskip_offset]))[4] =
                          ((uint64_t*)(&bp[lskip_offset]))[4];
                        }
                      tp += tile_stride;
                      bp += buf_stride;
                    }
                    break;
                  case 48:
                    CHECK_ALIGNMENT (uint64_t);
                    for (row = offsety;
                         row < tile_height && y < height;
                         row++, y++)
                    {
                      if (buffer_y + y >= buffer_abyss_y &&
                          buffer_y + y < abyss_y_total)
                        {
                          ((uint64_t*)(&tp[lskip_offset]))[0] =
                          ((uint64_t*)(&bp[lskip_offset]))[0];
                          ((uint64_t*)(&tp[lskip_offset]))[1] =
                          ((uint64_t*)(&bp[lskip_offset]))[1];
                          ((uint64_t*)(&tp[lskip_offset]))[2] =
                          ((uint64_t*)(&bp[lskip_offset]))[2];
                          ((uint64_t*)(&tp[lskip_offset]))[3] =
                          ((uint64_t*)(&bp[lskip_offset]))[3];
                          ((uint64_t*)(&tp[lskip_offset]))[4] =
                          ((uint64_t*)(&bp[lskip_offset]))[4];
                          ((uint64_t*)(&tp[lskip_offset]))[5] =
                          ((uint64_t*)(&bp[lskip_offset]))[5];
                        }
                      tp += tile_stride;
                      bp += buf_stride;
                    }
                    break;
                  case 56:
                    CHECK_ALIGNMENT (uint64_t);
                    for (row = offsety;
                         row < tile_height && y < height;
                         row++, y++)
                    {
                      if (buffer_y + y >= buffer_abyss_y &&
                          buffer_y + y < abyss_y_total)
                        {
                          ((uint64_t*)(&tp[lskip_offset]))[0] =
                          ((uint64_t*)(&bp[lskip_offset]))[0];
                          ((uint64_t*)(&tp[lskip_offset]))[1] =
                          ((uint64_t*)(&bp[lskip_offset]))[1];
                          ((uint64_t*)(&tp[lskip_offset]))[2] =
                          ((uint64_t*)(&bp[lskip_offset]))[2];
                          ((uint64_t*)(&tp[lskip_offset]))[3] =
                          ((uint64_t*)(&bp[lskip_offset]))[3];
                          ((uint64_t*)(&tp[lskip_offset]))[4] =
                          ((uint64_t*)(&bp[lskip_offset]))[4];
                          ((uint64_t*)(&tp[lskip_offset]))[5] =
                          ((uint64_t*)(&bp[lskip_offset]))[5];
                          ((uint64_t*)(&tp[lskip_offset]))[6] =
                          ((uint64_t*)(&bp[lskip_offset]))[6];
                        }
                      tp += tile_stride;
                      bp += buf_stride;
                    }
                    break;
                  case 64:
                    CHECK_ALIGNMENT (uint64_t);
                    for (row = offsety;
                         row < tile_height && y < height;
                         row++, y++)
                    {
                      if (buffer_y + y >= buffer_abyss_y &&
                          buffer_y + y < abyss_y_total)
                        {
                          ((uint64_t*)(&tp[lskip_offset]))[0] =
                          ((uint64_t*)(&bp[lskip_offset]))[0];
                          ((uint64_t*)(&tp[lskip_offset]))[1] =
                          ((uint64_t*)(&bp[lskip_offset]))[1];
                          ((uint64_t*)(&tp[lskip_offset]))[2] =
                          ((uint64_t*)(&bp[lskip_offset]))[2];
                          ((uint64_t*)(&tp[lskip_offset]))[3] =
                          ((uint64_t*)(&bp[lskip_offset]))[3];
                          ((uint64_t*)(&tp[lskip_offset]))[4] =
                          ((uint64_t*)(&bp[lskip_offset]))[4];
                          ((uint64_t*)(&tp[lskip_offset]))[5] =
                          ((uint64_t*)(&bp[lskip_offset]))[5];
                          ((uint64_t*)(&tp[lskip_offset]))[6] =
                          ((uint64_t*)(&bp[lskip_offset]))[6];
                          ((uint64_t*)(&tp[lskip_offset]))[7] =
                          ((uint64_t*)(&bp[lskip_offset]))[7];
                        }
                      tp += tile_stride;
                      bp += buf_stride;
                    }
                    break;
                  default:
                  unaligned:
                    for (row = offsety;
                         row < tile_height && y < height;
                         row++, y++)
                    {
                      if (buffer_y + y >= buffer_abyss_y &&
                          buffer_y + y < abyss_y_total)
                        {
                          memcpy (tp + lskip_offset,
                                  bp + lskip_offset,
                                  pixels * px_size);
                        }
                      tp += tile_stride;
                      bp += buf_stride;
                    }
                }

              #undef CHECK_ALIGNMENT
              #undef CHECK_ALIGNMENT_ALIGNOF
            }

          gegl_tile_unlock_no_void (tile);
          gegl_tile_unref (tile);
          bufx += (tile_width - offsetx);
        }
      bufy += (tile_height - offsety);
    }

  if (level == 0)
    {
      gegl_tile_handler_damage_rect (GEGL_TILE_HANDLER (buffer->tile_storage),
                                     GEGL_RECTANGLE (buffer_x, buffer_y,
                                                     width,    height));
    }
}

static inline void
gegl_buffer_set_internal (GeglBuffer          *buffer,
                          const GeglRectangle *rect,
                          gint                 level,
                          const Babl          *format,
                          const void          *src,
                          gint                 rowstride)
{
  if (gegl_buffer_ext_flush)
    {
      gegl_buffer_ext_flush (buffer, rect);
    }

  gegl_buffer_iterate_write (buffer, rect, (void *) src, rowstride, format, level);

  if (G_UNLIKELY (gegl_buffer_is_shared (buffer)))
    {
      gegl_buffer_flush (buffer);
    }
}

static inline void
_gegl_buffer_set_with_flags (GeglBuffer       *buffer,
                            const GeglRectangle *rect,
                            gint                 level,
                            const Babl          *format,
                            const void          *src,
                            gint                 rowstride,
                            GeglBufferSetFlag    flags)
{
  switch (flags)
  {
    case GEGL_BUFFER_SET_FLAG_FAST:
      gegl_buffer_set_internal (buffer, rect, level, format, src, rowstride);
    break;
    case GEGL_BUFFER_SET_FLAG_LOCK:
      gegl_buffer_lock (buffer);
      gegl_buffer_set_internal (buffer, rect, level, format, src, rowstride);
      gegl_buffer_unlock (buffer);
      break;
    case GEGL_BUFFER_SET_FLAG_NOTIFY:
      gegl_buffer_set_internal (buffer, rect, level, format, src, rowstride);
      if (flags & GEGL_BUFFER_SET_FLAG_NOTIFY)
        gegl_buffer_emit_changed_signal(buffer, rect);
      break;
    case GEGL_BUFFER_SET_FLAG_LOCK|GEGL_BUFFER_SET_FLAG_NOTIFY:
    default:
      gegl_buffer_lock (buffer);
      gegl_buffer_set_internal (buffer, rect, level, format, src, rowstride);
      gegl_buffer_unlock (buffer);
      gegl_buffer_emit_changed_signal(buffer, rect);
      break;
  }
}

void
gegl_buffer_set_with_flags (GeglBuffer       *buffer,
                            const GeglRectangle *rect,
                            gint                 level,
                            const Babl          *format,
                            const void          *src,
                            gint                 rowstride,
                            GeglBufferSetFlag    flags)
{
  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  if (format == NULL)
    format = buffer->soft_format;
  _gegl_buffer_set_with_flags (buffer, rect, level, format, src, rowstride, flags);
}

static void
gegl_buffer_iterate_read_simple (GeglBuffer          *buffer,
                                 const GeglRectangle *roi,
                                 guchar              *buf,
                                 gint                 buf_stride,
                                 const Babl          *format,
                                 gint                 level)
{
  gint tile_width  = buffer->tile_storage->tile_width;
  gint tile_height = buffer->tile_storage->tile_height;
  gint px_size     = babl_format_get_bytes_per_pixel (buffer->soft_format);
  gint bpx_size    = babl_format_get_bytes_per_pixel (format);
  gint tile_stride = px_size * tile_width;
  gint bufy        = 0;

  gint width    = roi->width;
  gint height   = roi->height;
  gint buffer_x = roi->x;
  gint buffer_y = roi->y;

  const Babl *fish;

  if (G_LIKELY (format == buffer->soft_format))
    fish = NULL;
  else
    fish = babl_fish ((gpointer) buffer->soft_format,
                      (gpointer) format);

  while (bufy < height)
    {
      gint tiledy  = buffer_y + bufy;
      gint offsety = gegl_tile_offset (tiledy, tile_height);
      gint bufx    = 0;

      while (bufx < width)
        {
          gint      tiledx  = buffer_x + bufx;
          gint      offsetx = gegl_tile_offset (tiledx, tile_width);
          guchar   *bp, *tile_base, *tp;
          gint      pixels, row, y;
          GeglTile *tile;

          bp = buf + (gsize) bufy * buf_stride + bufx * bpx_size;

          if (G_LIKELY (width + offsetx - bufx < tile_width))
            pixels = width - bufx;
          else
            pixels = tile_width - offsetx;

          g_rec_mutex_lock (&buffer->tile_storage->mutex);
          tile = gegl_tile_source_get_tile ((GeglTileSource *) (buffer),
                                          gegl_tile_indice (tiledx, tile_width),
                                          gegl_tile_indice (tiledy, tile_height),
                                          level);
          g_rec_mutex_unlock (&buffer->tile_storage->mutex);

          if (!tile)
            {
              g_warning ("didn't get tile, trying to continue");
              bufx += (tile_width - offsetx);
              continue;
            }

          gegl_tile_read_lock (tile);

          tile_base = gegl_tile_get_data (tile);
          tp        = ((guchar *) tile_base) + (offsety * tile_width + offsetx) * px_size;

          y = bufy;

          if (G_UNLIKELY (fish))
            {
              int rows = MIN(height - bufy, tile_height - offsety);
              if (rows == 1)
              babl_process (fish,
                            tp,
                            bp,
                            pixels);
              else
              babl_process_rows (fish,
                                 tp,
                                 tile_stride,
                                 bp,
                                 buf_stride,
                                 pixels,
                                 rows);

            }
          else
            {
              #ifdef __GNUC__
              #define CHECK_ALIGNMENT_ALIGNOF __alignof__
              #else
              #define CHECK_ALIGNMENT_ALIGNOF(type) \
                (G_STRUCT_OFFSET (struct alignof_helper, b))
              #endif
              #define CHECK_ALIGNMENT(type)                                    \
                do                                                             \
                  {                                                            \
                    struct alignof_helper { char a; type b; };                 \
                    /* verify 'alignof (type)' is a power of 2 */              \
                    G_STATIC_ASSERT (! (CHECK_ALIGNMENT_ALIGNOF (type) &       \
                                        (CHECK_ALIGNMENT_ALIGNOF (type) - 1)));\
                    if (((uintptr_t) tp |                                      \
                         (uintptr_t) bp |                                      \
                         tile_stride    |                                      \
                         buf_stride) &                                         \
                        (CHECK_ALIGNMENT_ALIGNOF (type) - 1))                  \
                      {                                                        \
                        goto unaligned;                                        \
                      }                                                        \
                  }                                                            \
                while (FALSE)

              switch (pixels * px_size)
                {
                  case 1:
                    CHECK_ALIGNMENT (guchar);
                    for (row = offsety; row < tile_height && y < height;
                         row++, y++)
                      {
                         bp[0] = tp[0];
                         tp += tile_stride;
                         bp += buf_stride;
                      }
                    break;
                  case 2:
                    CHECK_ALIGNMENT (uint16_t);
                    for (row = offsety; row < tile_height && y < height;
                         row++, y++)
                      {
                         ((uint16_t*)bp)[0] = ((uint16_t*)tp)[0];
                         tp += tile_stride;
                         bp += buf_stride;
                      }
                    break;
                  case 3:
                    CHECK_ALIGNMENT (guchar);
                    for (row = offsety; row < tile_height && y < height;
                         row++, y++)
                      {
                         bp[0] = tp[0];
                         bp[1] = tp[1];
                         bp[2] = tp[2];
                         tp += tile_stride;
                         bp += buf_stride;
                      }
                    break;
                  case 4:
                    CHECK_ALIGNMENT (uint32_t);
                    for (row = offsety; row < tile_height && y < height;
                         row++, y++)
                      {
                         ((uint32_t*)bp)[0] = ((uint32_t*)tp)[0];
                         tp += tile_stride;
                         bp += buf_stride;
                      }
                    break;
                  case 6:
                    CHECK_ALIGNMENT (uint16_t);
                    for (row = offsety; row < tile_height && y < height;
                         row++, y++)
                      {
                         ((uint16_t*)bp)[0] = ((uint16_t*)tp)[0];
                         ((uint16_t*)bp)[1] = ((uint16_t*)tp)[1];
                         ((uint16_t*)bp)[2] = ((uint16_t*)tp)[2];
                         tp += tile_stride;
                         bp += buf_stride;
                      }
                    break;
                  case 8:
                    CHECK_ALIGNMENT (uint64_t);
                    for (row = offsety; row < tile_height && y < height;
                         row++, y++)
                      {
                         ((uint64_t*)bp)[0] = ((uint64_t*)tp)[0];
                         tp += tile_stride;
                         bp += buf_stride;
                      }
                    break;
                  case 12:
                    CHECK_ALIGNMENT (uint32_t);
                    for (row = offsety; row < tile_height && y < height;
                         row++, y++)
                      {
                         ((uint32_t*)bp)[0] = ((uint32_t*)tp)[0];
                         ((uint32_t*)bp)[1] = ((uint32_t*)tp)[1];
                         ((uint32_t*)bp)[2] = ((uint32_t*)tp)[2];
                         tp += tile_stride;
                         bp += buf_stride;
                      }
                    break;
                  case 16:
                    CHECK_ALIGNMENT (uint64_t);
                    for (row = offsety; row < tile_height && y < height;
                         row++, y++)
                      {
                         ((uint64_t*)bp)[0] = ((uint64_t*)tp)[0];
                         ((uint64_t*)bp)[1] = ((uint64_t*)tp)[1];
                         tp += tile_stride;
                         bp += buf_stride;
                      }
                    break;
                  case 24:
                    CHECK_ALIGNMENT (uint64_t);
                    for (row = offsety; row < tile_height && y < height;
                         row++, y++)
                      {
                         ((uint64_t*)bp)[0] = ((uint64_t*)tp)[0];
                         ((uint64_t*)bp)[1] = ((uint64_t*)tp)[1];
                         ((uint64_t*)bp)[2] = ((uint64_t*)tp)[2];
                         tp += tile_stride;
                         bp += buf_stride;
                      }
                    break;
                  case 32:
                    CHECK_ALIGNMENT (uint64_t);
                    for (row = offsety; row < tile_height && y < height;
                         row++, y++)
                      {
                         ((uint64_t*)bp)[0] = ((uint64_t*)tp)[0];
                         ((uint64_t*)bp)[1] = ((uint64_t*)tp)[1];
                         ((uint64_t*)bp)[2] = ((uint64_t*)tp)[2];
                         ((uint64_t*)bp)[3] = ((uint64_t*)tp)[3];
                         tp += tile_stride;
                         bp += buf_stride;
                      }
                    break;
                  case 40:
                    CHECK_ALIGNMENT (uint64_t);
                    for (row = offsety; row < tile_height && y < height;
                         row++, y++)
                      {
                         ((uint64_t*)bp)[0] = ((uint64_t*)tp)[0];
                         ((uint64_t*)bp)[1] = ((uint64_t*)tp)[1];
                         ((uint64_t*)bp)[2] = ((uint64_t*)tp)[2];
                         ((uint64_t*)bp)[3] = ((uint64_t*)tp)[3];
                         ((uint64_t*)bp)[4] = ((uint64_t*)tp)[4];
                         tp += tile_stride;
                         bp += buf_stride;
                      }
                    break;
                  case 48:
                    CHECK_ALIGNMENT (uint64_t);
                    for (row = offsety; row < tile_height && y < height;
                         row++, y++)
                      {
                         ((uint64_t*)bp)[0] = ((uint64_t*)tp)[0];
                         ((uint64_t*)bp)[1] = ((uint64_t*)tp)[1];
                         ((uint64_t*)bp)[2] = ((uint64_t*)tp)[2];
                         ((uint64_t*)bp)[3] = ((uint64_t*)tp)[3];
                         ((uint64_t*)bp)[4] = ((uint64_t*)tp)[4];
                         ((uint64_t*)bp)[5] = ((uint64_t*)tp)[5];
                         tp += tile_stride;
                         bp += buf_stride;
                      }
                    break;
                  case 56:
                    CHECK_ALIGNMENT (uint64_t);
                    for (row = offsety; row < tile_height && y < height;
                         row++, y++)
                      {
                         ((uint64_t*)bp)[0] = ((uint64_t*)tp)[0];
                         ((uint64_t*)bp)[1] = ((uint64_t*)tp)[1];
                         ((uint64_t*)bp)[2] = ((uint64_t*)tp)[2];
                         ((uint64_t*)bp)[3] = ((uint64_t*)tp)[3];
                         ((uint64_t*)bp)[4] = ((uint64_t*)tp)[4];
                         ((uint64_t*)bp)[5] = ((uint64_t*)tp)[5];
                         ((uint64_t*)bp)[6] = ((uint64_t*)tp)[6];
                         tp += tile_stride;
                         bp += buf_stride;
                      }
                    break;
                  case 64:
                    CHECK_ALIGNMENT (uint64_t);
                    for (row = offsety; row < tile_height && y < height;
                         row++, y++)
                      {
                         ((uint64_t*)bp)[0] = ((uint64_t*)tp)[0];
                         ((uint64_t*)bp)[1] = ((uint64_t*)tp)[1];
                         ((uint64_t*)bp)[2] = ((uint64_t*)tp)[2];
                         ((uint64_t*)bp)[3] = ((uint64_t*)tp)[3];
                         ((uint64_t*)bp)[4] = ((uint64_t*)tp)[4];
                         ((uint64_t*)bp)[5] = ((uint64_t*)tp)[5];
                         ((uint64_t*)bp)[6] = ((uint64_t*)tp)[6];
                         ((uint64_t*)bp)[7] = ((uint64_t*)tp)[7];
                         tp += tile_stride;
                         bp += buf_stride;
                      }
                    break;

                  default:
                  unaligned:
                    for (row = offsety;
                         row < tile_height && y < height;
                         row++, y++)
                      {
                         memcpy (bp, tp, pixels * px_size);
                         tp += tile_stride;
                         bp += buf_stride;
                      }
                }

              #undef CHECK_ALIGNMENT
              #undef CHECK_ALIGNMENT_ALIGNOF
            }

          gegl_tile_read_unlock (tile);
          gegl_tile_unref (tile);
          bufx += (tile_width - offsetx);
        }
      bufy += (tile_height - offsety);
    }

}

static void
fill_abyss_none (guchar *buf, gint width, gint height, gint buf_stride, gint pixel_size)
{
  const int byte_width = width * pixel_size;

  if (buf_stride == byte_width)
    {
      memset (buf, 0, byte_width * height);
    }
  else
    {
      while (height--)
        {
          memset (buf, 0, byte_width);
          buf += buf_stride;
        }
    }
}

static void
fill_abyss_color (guchar *buf, gint width, gint height, gint buf_stride, guchar *pixel, gint pixel_size)
{
  if (buf_stride == width * pixel_size)
    {
      gegl_memset_pattern (buf, pixel, pixel_size, width * height);
    }
  else
    {
      while (height--)
        {
          gegl_memset_pattern (buf, pixel, pixel_size, width);
          buf += buf_stride;
        }
    }
}

static void
gegl_buffer_iterate_read_abyss_color (GeglBuffer          *buffer,
                                      const GeglRectangle *roi,
                                      const GeglRectangle *abyss,
                                      guchar              *buf,
                                      gint                 buf_stride,
                                      const Babl          *format,
                                      gint                 level,
                                      guchar              *color,
                                      GeglAbyssPolicy      repeat_mode)
{
  GeglRectangle current_roi = *roi;
  gint bpp = babl_format_get_bytes_per_pixel (format);

  if (current_roi.y < abyss->y)
    {
      /* Abyss above image */
      gint height = abyss->y - current_roi.y;
      if (current_roi.height < height)
        height = current_roi.height;
      if (color)
        fill_abyss_color (buf, current_roi.width, height, buf_stride, color, bpp);
      else
        fill_abyss_none (buf, current_roi.width, height, buf_stride, bpp);
      buf += buf_stride * height;
      current_roi.y += height;
      current_roi.height -= height;
    }

  if (current_roi.height && (current_roi.y < abyss->y + abyss->height))
    {
      GeglRectangle inner_roi = current_roi;
      guchar *inner_buf       = buf;

      if (inner_roi.height + inner_roi.y > abyss->height + abyss->y)
        {
          /* Clamp inner_roi to the in abyss height */
          inner_roi.height -= (inner_roi.height + inner_roi.y) - (abyss->height + abyss->y);
        }

      if (inner_roi.x < abyss->x)
        {
          /* Abyss left of image */
          gint width = abyss->x - inner_roi.x;
          if (width > inner_roi.width)
            width = inner_roi.width;

          if (color)
            fill_abyss_color (inner_buf, width, inner_roi.height, buf_stride, color, bpp);
          else
            fill_abyss_none (inner_buf, width, inner_roi.height, buf_stride, bpp);
          inner_buf += width * bpp;
          inner_roi.x += width;
          inner_roi.width -= width;
        }

      if (inner_roi.width && (inner_roi.x < abyss->x + abyss->width))
        {
          gint full_width = inner_roi.width;

          if (inner_roi.width + inner_roi.x > abyss->width + abyss->x)
            {
              /* Clamp inner_roi to the in abyss width */
              inner_roi.width -= (inner_roi.width + inner_roi.x) - (abyss->width + abyss->x);
            }

          if (level)
            gegl_buffer_iterate_read_fringed (buffer,
                                              &inner_roi,
                                              abyss,
                                              inner_buf,
                                              buf_stride,
                                              format,
                                              level,
                                              repeat_mode);
          else
            gegl_buffer_iterate_read_simple (buffer,
                                             &inner_roi,
                                             inner_buf,
                                             buf_stride,
                                             format,
                                             level);

          inner_buf += inner_roi.width * bpp;
          inner_roi.width = full_width - inner_roi.width;
        }

      if (inner_roi.width)
        {
          /* Abyss right of image */
          if (color)
            fill_abyss_color (inner_buf, inner_roi.width, inner_roi.height, buf_stride, color, bpp);
          else
            fill_abyss_none (inner_buf, inner_roi.width, inner_roi.height, buf_stride, bpp);
        }

      buf += inner_roi.height * buf_stride;
      /* current_roi.y += inner_roi.height; */
      current_roi.height -= inner_roi.height;
    }

  if (current_roi.height)
    {
      /* Abyss below image */
      if (color)
        fill_abyss_color (buf, current_roi.width, current_roi.height, buf_stride, color, bpp);
      else
        fill_abyss_none (buf, current_roi.width, current_roi.height, buf_stride, bpp);
    }
}

static void
gegl_buffer_iterate_read_abyss_clamp (GeglBuffer          *buffer,
                                      const GeglRectangle *roi,
                                      const GeglRectangle *abyss,
                                      guchar              *buf,
                                      gint                 buf_stride,
                                      const Babl          *format,
                                      gint                 level)
{
  GeglRectangle read_output_rect;
  GeglRectangle read_input_rect;

  gint    bpp           = babl_format_get_bytes_per_pixel (format);
  gint    x_read_offset = 0;
  gint    y_read_offset = 0;
  gint    buf_offset_cols;
  gint    buf_offset_rows;
  gint    top_rows, left_cols, right_cols, bottom_rows;
  guchar *read_buf;

  if (roi->x >= abyss->x + abyss->width) /* Right of */
    x_read_offset = roi->x - (abyss->x + abyss->width) + 1;
  else if (roi->x + roi->width <= abyss->x) /* Left of */
    x_read_offset = (roi->x + roi->width) - abyss->x - 1;

  if (roi->y >= abyss->y + abyss->height) /* Above */
    y_read_offset = roi->y - (abyss->y + abyss->height) + 1;
  else if (roi->y + roi->height <= abyss->y) /* Below of */
    y_read_offset = (roi->y + roi->height) - abyss->y - 1;

  /* Intersect our shifted abyss with the roi */
  gegl_rectangle_intersect (&read_output_rect,
                            roi,
                            GEGL_RECTANGLE (abyss->x + x_read_offset,
                                            abyss->y + y_read_offset,
                                            abyss->width,
                                            abyss->height));

  /* Offset into *buf based on the intersected rect's x & y */
  buf_offset_cols = read_output_rect.x - roi->x;
  buf_offset_rows = read_output_rect.y - roi->y;
  read_buf = buf + (buf_offset_cols * bpp + buf_offset_rows * buf_stride);

  /* Convert the read output to a coresponding input */
  read_input_rect.x = read_output_rect.x - x_read_offset;
  read_input_rect.y = read_output_rect.y - y_read_offset;
  read_input_rect.width = read_output_rect.width;
  read_input_rect.height = read_output_rect.height;
#if 1
  if (level)
    gegl_buffer_iterate_read_fringed (buffer,
                                      &read_input_rect,
                                      abyss,
                                      read_buf,
                                      buf_stride,
                                      format,
                                      level,
                                      GEGL_ABYSS_CLAMP);
  else
#endif
    gegl_buffer_iterate_read_simple (buffer,
                                     &read_input_rect,
                                     read_buf,
                                     buf_stride,
                                     format,
                                     level);

  /* All calculations are done relative to read_output_rect because it is guranteed
   * to be inside of the roi rect and none of these calculations can return a value
   * less than 0.
   */
  top_rows = read_output_rect.y - roi->y;
  left_cols = read_output_rect.x - roi->x;
  right_cols = (roi->x + roi->width) - (read_output_rect.x + read_output_rect.width);
  bottom_rows = (roi->y + roi->height) - (read_output_rect.y + read_output_rect.height);

  if (top_rows)
    {
      guchar *fill_buf = buf;
      /* Top left pixel */
      if (left_cols)
        {
          guchar *src_pixel = read_buf;
          fill_abyss_color (fill_buf, left_cols, top_rows, buf_stride, src_pixel, bpp);
          fill_buf += left_cols * bpp;
        }

      /* Top rows */
      {
        guchar *src_pixel = read_buf;
        guchar *row_fill_buf = fill_buf;
        gint byte_width = read_output_rect.width * bpp;
        gint i;
        for (i = 0; i < top_rows; ++i)
          {
            memcpy (row_fill_buf, src_pixel, byte_width);
            row_fill_buf += buf_stride;
          }
      }

      fill_buf += (read_input_rect.width) * bpp;
      /* Top right pixel */
      if (right_cols)
        {
          guchar *src_pixel = read_buf + (read_input_rect.width - 1) * bpp;
          fill_abyss_color (fill_buf, right_cols, top_rows, buf_stride, src_pixel, bpp);
        }
    }

  /* Left */
  if (left_cols)
    {
      guchar *row_fill_buf = buf + (top_rows * buf_stride);
      guchar *src_pixel = read_buf;
      gint i;

      for (i = 0; i < read_output_rect.height; ++i)
        {
          gegl_memset_pattern (row_fill_buf, src_pixel, bpp, left_cols);
          row_fill_buf += buf_stride;
          src_pixel += buf_stride;
        }
    }

  /* Right */
  if (right_cols)
    {
      guchar *row_fill_buf = buf + (read_input_rect.width + left_cols) * bpp
                                 + top_rows * buf_stride;
      guchar *src_pixel = read_buf + (read_input_rect.width - 1) * bpp;
      gint i;

      for (i = 0; i < read_output_rect.height; ++i)
        {
          gegl_memset_pattern (row_fill_buf, src_pixel, bpp, right_cols);
          row_fill_buf += buf_stride;
          src_pixel += buf_stride;
        }
    }

  if (bottom_rows)
    {
      guchar *fill_buf = buf + (read_input_rect.height + top_rows) * buf_stride;
      /* Bottom left */
      if (left_cols)
        {
          guchar *src_pixel = read_buf + (read_input_rect.height - 1) * buf_stride;
          fill_abyss_color (fill_buf, left_cols, bottom_rows, buf_stride, src_pixel, bpp);
          fill_buf += left_cols * bpp;
        }

      /* Bottom rows */
      {
        guchar *src_pixel = read_buf + (read_input_rect.height - 1) * buf_stride;
        guchar *row_fill_buf = fill_buf;
        gint byte_width = read_output_rect.width * bpp;
        gint i;
        for (i = 0; i < bottom_rows; ++i)
          {
            memcpy (row_fill_buf, src_pixel, byte_width);
            row_fill_buf += buf_stride;
          }
      }

      fill_buf += read_input_rect.width * bpp;
      /* Bottom right */
      if (right_cols)
        {
          guchar *src_pixel = read_buf + (read_input_rect.width - 1) * bpp + (read_input_rect.height - 1) * buf_stride;
          fill_abyss_color (fill_buf, right_cols, bottom_rows, buf_stride, src_pixel, bpp);
        }
    }
}

static void
gegl_buffer_iterate_read_abyss_loop (GeglBuffer          *buffer,
                                     const GeglRectangle *roi,
                                     const GeglRectangle *abyss,
                                     guchar              *buf,
                                     gint                 buf_stride,
                                     const Babl          *format,
                                     gint                 level)
{
  GeglRectangle current_roi;
  gint          bpp = babl_format_get_bytes_per_pixel (format);
  gint          origin_x;

  /* Loop abyss works like iterating over a grid of tiles the size of the abyss */
  gint loop_chunk_ix = gegl_tile_indice (roi->x - abyss->x, abyss->width);
  gint loop_chunk_iy = gegl_tile_indice (roi->y - abyss->y, abyss->height);

  current_roi.x = loop_chunk_ix * abyss->width  + abyss->x;
  current_roi.y = loop_chunk_iy * abyss->height + abyss->y;

  current_roi.width  = abyss->width;
  current_roi.height = abyss->height;

  origin_x = current_roi.x;

  while (current_roi.y < roi->y + roi->height)
    {
      guchar *inner_buf  = buf;
      gint    row_height = 0;

      while (current_roi.x < roi->x + roi->width)
        {
          GeglRectangle simple_roi;
          gegl_rectangle_intersect (&simple_roi, &current_roi, roi);

          gegl_buffer_iterate_read_simple (buffer,
                                           GEGL_RECTANGLE (abyss->x + (simple_roi.x - current_roi.x),
                                                           abyss->y + (simple_roi.y - current_roi.y),
                                                           simple_roi.width,
                                                           simple_roi.height),
                                           inner_buf,
                                           buf_stride,
                                           format,
                                           level);

          row_height  = simple_roi.height;
          inner_buf  += simple_roi.width * bpp;

          current_roi.x += abyss->width;
        }

      buf += buf_stride * row_height;

      current_roi.x  = origin_x;
      current_roi.y += abyss->height;
    }
}

static gpointer
gegl_buffer_read_at_level (GeglBuffer          *buffer,
                           const GeglRectangle *roi,
                           guchar              *buf,
                           gint                 rowstride,
                           const Babl          *format,
                           gint                 level,
                           GeglAbyssPolicy      repeat_mode)
{
  gint bpp = babl_format_get_bytes_per_pixel (format);

  if (level == 0)
    {
      if (!buf)
        {
          gpointer scratch = gegl_scratch_alloc (bpp * roi->width * roi->height);

          gegl_buffer_iterate_read_dispatch (buffer, roi, scratch, roi->width * bpp, format, 0, repeat_mode);

          return scratch;
        }
      else
        {
          gegl_buffer_iterate_read_dispatch (buffer, roi, buf, rowstride, format, 0, repeat_mode);

          return NULL;
        }
    }
  else
    {
      gpointer scratch;
      GeglRectangle next_roi;
      next_roi.x = roi->x * 2;
      next_roi.y = roi->y * 2;
      next_roi.width = roi->width * 2;
      next_roi.height = roi->height * 2;

      /* If the next block is too big split it in half */
      if (next_roi.width * next_roi.height > 256 * 256)
        {
          GeglRectangle next_roi_a = next_roi;
          GeglRectangle next_roi_b = next_roi;
          gint scratch_stride = next_roi.width * bpp;
          gpointer scratch_a;
          gpointer scratch_b;
          scratch = gegl_scratch_alloc (bpp * next_roi.width * next_roi.height);

          if (next_roi.width > next_roi.height)
            {
              next_roi_a.width = roi->width;
              next_roi_b.width = roi->width;
              next_roi_b.x += next_roi_a.width;

              scratch_a = scratch;
              scratch_b = (guchar *)scratch + next_roi_a.width * bpp;
            }
          else
            {
              next_roi_a.height = roi->height;
              next_roi_b.height = roi->height;
              next_roi_b.y += next_roi_a.height;

              scratch_a = scratch;
              scratch_b = (guchar *)scratch + next_roi_a.height * scratch_stride;
            }

          gegl_buffer_read_at_level (buffer, &next_roi_a, scratch_a, scratch_stride, format, level - 1, repeat_mode);
          gegl_buffer_read_at_level (buffer, &next_roi_b, scratch_b, scratch_stride, format, level - 1, repeat_mode);

        }
      else
        {
          scratch = gegl_buffer_read_at_level (buffer, &next_roi, NULL, 0, format, level - 1, repeat_mode);
        }

      if (buf)
        {
          gegl_downscale_2x2 (format,
                              next_roi.width,
                              next_roi.height,
                              scratch,
                              next_roi.width * bpp,
                              buf,
                              rowstride);
          gegl_scratch_free (scratch);
          return NULL;
        }
      else
        {
          gegl_downscale_2x2 (format,
                              next_roi.width,
                              next_roi.height,
                              scratch,
                              next_roi.width * bpp,
                              scratch,
                              roi->width * bpp);
          return scratch;
        }
    }
}

static void
gegl_buffer_iterate_read_fringed (GeglBuffer          *buffer,
                                  const GeglRectangle *roi,
                                  const GeglRectangle *abyss,
                                  guchar              *buf,
                                  gint                 buf_stride,
                                  const Babl          *format,
                                  gint                 level,
                                  GeglAbyssPolicy      repeat_mode)
{
  gint x = roi->x;
  gint y = roi->y;
  gint width  = roi->width;
  gint height = roi->height;
  guchar        *inner_buf = buf;

  gint bpp = babl_format_get_bytes_per_pixel (format);

  if (x <= abyss->x)
    {
      GeglRectangle fringe_roi = {x, y, 1, height};
      guchar *fringe_buf = inner_buf;

      gegl_buffer_read_at_level (buffer, &fringe_roi, fringe_buf, buf_stride, format, level, repeat_mode);
      inner_buf += bpp;
      x     += 1;
      width -= 1;

      if (!width)
        return;
    }

  if (y <= abyss->y)
    {
      GeglRectangle fringe_roi = {x, y, width, 1};
      guchar *fringe_buf = inner_buf;

      gegl_buffer_read_at_level (buffer, &fringe_roi, fringe_buf, buf_stride, format, level, repeat_mode);
      inner_buf += buf_stride;
      y      += 1;
      height -= 1;

      if (!height)
        return;
    }

  if (y + height >= abyss->y + abyss->height)
    {
      GeglRectangle fringe_roi = {x, y + height - 1, width, 1};
      guchar *fringe_buf = inner_buf + (height - 1) * buf_stride;

      gegl_buffer_read_at_level (buffer, &fringe_roi, fringe_buf, buf_stride, format, level, repeat_mode);
      height -= 1;

      if (!height)
        return;
    }

  if (x + width >= abyss->x + abyss->width)
    {
      GeglRectangle fringe_roi = {x + width - 1, y, 1, height};
      guchar *fringe_buf = inner_buf + (width - 1) * bpp;

      gegl_buffer_read_at_level (buffer, &fringe_roi, fringe_buf, buf_stride, format, level, repeat_mode);
      width -= 1;

      if (!width)
        return;
    }

  gegl_buffer_iterate_read_simple (buffer,
                                   GEGL_RECTANGLE (x, y, width, height),
                                   inner_buf,
                                   buf_stride,
                                   format,
                                   level);
}

static void
gegl_buffer_iterate_read_dispatch (GeglBuffer          *buffer,
                                   const GeglRectangle *roi,
                                   guchar              *buf,
                                   gint                 rowstride,
                                   const Babl          *format,
                                   gint                 level,
                                   GeglAbyssPolicy      repeat_mode)
{
  GeglRectangle abyss          = buffer->abyss;
  GeglRectangle abyss_factored = abyss;
  GeglRectangle roi_factored   = *roi;

  if (level)
    {
      const gint    factor         = 1 << level;
      const gint    x1 = buffer->shift_x + abyss.x;
      const gint    y1 = buffer->shift_y + abyss.y;
      const gint    x2 = buffer->shift_x + abyss.x + abyss.width;
      const gint    y2 = buffer->shift_y + abyss.y + abyss.height;

      abyss_factored.x      = (x1 + (x1 < 0 ? 1 - factor : 0)) / factor;
      abyss_factored.y      = (y1 + (y1 < 0 ? 1 - factor : 0)) / factor;
      abyss_factored.width  = (x2 + (x2 < 0 ? 0 : factor - 1)) / factor - abyss_factored.x;
      abyss_factored.height = (y2 + (y2 < 0 ? 0 : factor - 1)) / factor - abyss_factored.y;

      roi_factored.x       = (buffer->shift_x + roi_factored.x) / factor;
      roi_factored.y       = (buffer->shift_y + roi_factored.y) / factor;
      roi_factored.width  /= factor;
      roi_factored.height /= factor;
    }
  else
    {
      roi_factored.x += buffer->shift_x;
      roi_factored.y += buffer->shift_y;
      abyss_factored.x += buffer->shift_x;
      abyss_factored.y += buffer->shift_y;
    }

  if (rowstride == GEGL_AUTO_ROWSTRIDE)
    rowstride = roi_factored.width * babl_format_get_bytes_per_pixel (format);

  if (gegl_rectangle_contains (&abyss, roi))
    {
      gegl_buffer_iterate_read_simple (buffer, &roi_factored, buf, rowstride, format, level);
    }
  else if (repeat_mode == GEGL_ABYSS_NONE)
    {
      gegl_buffer_iterate_read_abyss_color (buffer, &roi_factored, &abyss_factored,
                                            buf, rowstride, format, level, NULL,
                                            GEGL_ABYSS_NONE);
    }
  else if (repeat_mode == GEGL_ABYSS_WHITE)
    {
      guchar color[128];
      gfloat in_color[] = {1.0f, 1.0f, 1.0f, 1.0f};

      babl_process (babl_fish (gegl_babl_rgba_linear_float (), format),
                    in_color, color, 1);

      gegl_buffer_iterate_read_abyss_color (buffer, &roi_factored, &abyss_factored,
                                            buf, rowstride, format, level, color,
                                            GEGL_ABYSS_WHITE);
    }
  else if (repeat_mode == GEGL_ABYSS_BLACK)
    {
      guchar color[128];
      gfloat  in_color[] = {0.0f, 0.0f, 0.0f, 1.0f};

      babl_process (babl_fish (gegl_babl_rgba_linear_float (), format),
                    in_color, color, 1);

      gegl_buffer_iterate_read_abyss_color (buffer, &roi_factored, &abyss_factored,
                                            buf, rowstride, format, level, color,
                                            GEGL_ABYSS_BLACK);
    }
  else if (repeat_mode == GEGL_ABYSS_CLAMP)
    {
      if (abyss_factored.width == 0 || abyss_factored.height == 0)
        gegl_buffer_iterate_read_abyss_color (buffer, &roi_factored, &abyss_factored,
                                              buf, rowstride, format, level, NULL,
                                              GEGL_ABYSS_NONE);
      else
        gegl_buffer_iterate_read_abyss_clamp (buffer, &roi_factored, &abyss_factored,
                                              buf, rowstride, format, level);
    }
  else
    {
      if (abyss_factored.width == 0 || abyss_factored.height == 0)
        gegl_buffer_iterate_read_abyss_color (buffer, &roi_factored, &abyss_factored,
                                              buf, rowstride, format, level, NULL,
                                              GEGL_ABYSS_NONE);
      else
        gegl_buffer_iterate_read_abyss_loop (buffer, &roi_factored, &abyss_factored,
                                             buf, rowstride, format, level);
    }
}

void
gegl_buffer_set_unlocked (GeglBuffer          *buffer,
                          const GeglRectangle *rect,
                          gint                 level,
                          const Babl          *format,
                          const void          *src,
                          gint                 rowstride)
{
  _gegl_buffer_set_with_flags (buffer, rect, level, format, src, rowstride,
                               GEGL_BUFFER_SET_FLAG_NOTIFY);
}

void
gegl_buffer_set_unlocked_no_notify (GeglBuffer          *buffer,
                                    const GeglRectangle *rect,
                                    gint                 level,
                                    const Babl          *format,
                                    const void          *src,
                                    gint                 rowstride)
{
  _gegl_buffer_set_with_flags (buffer, rect, level, format, src, rowstride,
                               GEGL_BUFFER_SET_FLAG_FAST);
}


void
gegl_buffer_set (GeglBuffer          *buffer,
                 const GeglRectangle *rect,
                 gint                 level,
                 const Babl          *format,
                 const void          *src,
                 gint                 rowstride)
{
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  if (G_UNLIKELY (gegl_rectangle_is_empty (rect ? rect : &buffer->extent)))
    return;

  g_return_if_fail (src != NULL);

  if (G_LIKELY (format == NULL))
    format = buffer->soft_format;

  if (G_UNLIKELY (rect && rect->width == 1))
    {
      if (level == 0 && rect->height == 1)
        {
          _gegl_buffer_set_pixel (buffer, rect->x, rect->y,
                                  format, src,
                                  GEGL_BUFFER_SET_FLAG_LOCK|
                                  GEGL_BUFFER_SET_FLAG_NOTIFY);
          return;
        }
      else if (buffer->soft_format != format &&
               rowstride == babl_format_get_bytes_per_pixel (format))
        {
          int     bpp = babl_format_get_bytes_per_pixel (buffer->soft_format);
          uint8_t tmp[rect->height * bpp];
          babl_process (babl_fish (format, buffer->soft_format),
                        src, &tmp[0], rect->height);
          _gegl_buffer_set_with_flags (buffer, rect, level, buffer->soft_format, tmp, bpp,
                                       GEGL_BUFFER_SET_FLAG_LOCK|
                                       GEGL_BUFFER_SET_FLAG_NOTIFY);
          return;
        }
    }

    _gegl_buffer_set_with_flags (buffer, rect, level, format, src, rowstride,
                                 GEGL_BUFFER_SET_FLAG_LOCK|
                                 GEGL_BUFFER_SET_FLAG_NOTIFY);
}

/* Expand roi by scale so it uncludes all pixels needed
 * to satisfy a gegl_buffer_get() call at level 0.
 */
GeglRectangle
_gegl_get_required_for_scale (const GeglRectangle *roi,
                              gdouble              scale)
{
  if (GEGL_FLOAT_EQUAL (scale, 1.0))
    return *roi;
  else
    {
      gint x1 = int_floorf (roi->x / scale + GEGL_SCALE_EPSILON);
      gint x2 = int_ceilf ((roi->x + roi->width) / scale - GEGL_SCALE_EPSILON);
      gint y1 = int_floorf (roi->y / scale + GEGL_SCALE_EPSILON);
      gint y2 = int_ceilf ((roi->y + roi->height) / scale - GEGL_SCALE_EPSILON);

      gint pad = (1.0 / scale > 1.0) ? int_ceilf (1.0 / scale) : 1;

      if (scale < 1.0)
        {
          return *GEGL_RECTANGLE (x1 - pad,
                                  y1 - pad,
                                  x2 - x1 + 2 * pad,
                                  y2 - y1 + 2 * pad);
        }
      else
        {
          return *GEGL_RECTANGLE (x1,
                                  y1,
                                  x2 - x1,
                                  y2 - y1);
        }
      }
}

static inline void
_gegl_buffer_get_unlocked (GeglBuffer          *buffer,
                           gdouble              scale,
                           const GeglRectangle *rect,
                           const Babl          *format,
                           gpointer             dest_buf,
                           gint                 rowstride,
                           GeglAbyssPolicy      flags)
{
  GeglAbyssPolicy repeat_mode = flags & 0x7; /* mask off interpolation from repeat mode part of flags */

  g_return_if_fail (scale > 0.0f);

  if (! rect && GEGL_FLOAT_EQUAL (scale, 1.0))
    rect = &buffer->extent;

  g_return_if_fail (rect != NULL);

  if (G_UNLIKELY (gegl_rectangle_is_empty (rect)))
    return;

  g_return_if_fail (dest_buf != NULL);

  if (! format)
    format = buffer->soft_format;

  if (gegl_buffer_ext_flush)
    gegl_buffer_ext_flush (buffer, rect);

  if (G_UNLIKELY (scale == 1.0 &&
      rect->width == 1))
  {
    if (rect->height == 1)
      {
        gegl_buffer_get_pixel (buffer, rect->x, rect->y, format, dest_buf,
                               repeat_mode);
      }
    else
      {
        if (buffer->soft_format == format ||
            rowstride != babl_format_get_bytes_per_pixel (format))
        {
          gegl_buffer_iterate_read_dispatch (buffer, rect, dest_buf,
                                             rowstride, format, 0, repeat_mode);
        }
        else
        {
          /* first fetch all pixels to a temporary buffer */
          gint    bpp = babl_format_get_bytes_per_pixel (buffer->soft_format);
          uint8_t tmp[rect->height * bpp];
          gegl_buffer_iterate_read_dispatch (buffer, rect, &tmp[0],
                                             bpp, buffer->soft_format, 0, repeat_mode);
          /* then convert in a single shot */
          babl_process (babl_fish (buffer->soft_format, format),
                        &tmp[0], dest_buf, rect->height);
        }
      }
    return;
  }

  if (GEGL_FLOAT_EQUAL (scale, 1.0))
    {
      gegl_buffer_iterate_read_dispatch (buffer, rect, dest_buf, rowstride,
                                         format, 0, repeat_mode);
      return;
    }
  else
  {
    gint chunk_height;
    GeglRectangle rect2       = *rect;
    gint    bpp               = babl_format_get_bytes_per_pixel (format);
    gint    ystart            = rect->y;
    float   scale_orig        = scale;
    gint    level             = 0;
    void   *sample_buf;
    gint    x1 = int_floorf (rect->x / scale_orig + GEGL_SCALE_EPSILON);
    gint    x2 = int_ceilf ((rect->x + rect->width) / scale_orig - GEGL_SCALE_EPSILON);
    int     max_bytes_per_row = ((rect->width+1) * bpp * 2);
    int     allocated         = 0;
    gint interpolation = (flags & GEGL_BUFFER_FILTER_ALL);
    gint    factor = 1;

    while (scale <= 0.5)
      {
        x1 = 0 < x1 ? x1 / 2 : (x1 - 1) / 2;
        x2 = 0 < x2 ? (x2 + 1) / 2 : x2 / 2;
        scale  *= 2;
        factor *= 2;
        level++;
      }

    if (GEGL_FLOAT_EQUAL (scale, 1.0))
      {
        GeglRectangle rect0;

        rect0.x      = int_floorf (rect->x / scale_orig + GEGL_SCALE_EPSILON);
        rect0.y      = int_floorf (rect->y / scale_orig + GEGL_SCALE_EPSILON);
        rect0.width  = int_ceilf ((rect->x + rect->width) / scale_orig -
                                  GEGL_SCALE_EPSILON) -
                       rect0.x;
        rect0.height = int_ceilf ((rect->y + rect->height) / scale_orig -
                                  GEGL_SCALE_EPSILON) -
                       rect0.y;

        gegl_buffer_iterate_read_dispatch (buffer, &rect0,
                                           dest_buf, rowstride,
                                           format, level, repeat_mode);
        return;
      }

    chunk_height = (1024 * 128) / max_bytes_per_row;

    if (chunk_height < 4)
      chunk_height = 4;

    rect2.y = ystart;
    rect2.height = chunk_height;
    if (rect2.y + rect2.height > rect->y + rect->height)
    {
      rect2.height = (rect->y + rect->height) - rect2.y;
      chunk_height = rect2.height;
    }

    allocated = max_bytes_per_row * ((chunk_height+1) * 2);

    if (interpolation == GEGL_BUFFER_FILTER_AUTO)
    {
      /* with no specified interpolation we aim for a trade-off where
         100-200% ends up using box-filter - which is a better transition
         to nearest neighbor which happens beyond 200% further below.
       */
      if (scale >= 2.0)
        interpolation = GEGL_BUFFER_FILTER_NEAREST;
      else if (scale > 1.0)
        {
          interpolation = GEGL_BUFFER_FILTER_BOX;
        }
      else
        interpolation = GEGL_BUFFER_FILTER_BILINEAR;
    }

    sample_buf = gegl_scratch_alloc (allocated);

    while (rect2.width > 0 && rect2.height > 0)
    {
      GeglRectangle sample_rect;
      gint    buf_width, buf_height;
      gint    y1 = int_floorf (rect2.y / scale_orig + GEGL_SCALE_EPSILON);
      gint    y2 = int_ceilf ((rect2.y + rect2.height) / scale_orig - GEGL_SCALE_EPSILON);
      scale = scale_orig;

      while (scale <= 0.5)
        {
          y1 = 0 < y1 ? y1 / 2 : (y1 - 1) / 2;
          y2 = 0 < y2 ? (y2 + 1) / 2 : y2 / 2;
          scale  *= 2;
        }

      if (rowstride == GEGL_AUTO_ROWSTRIDE)
        rowstride = rect2.width * bpp;

      /* this is the level where we split and chew through a small temp-buf worth of data
       * possibly managing to keep things in L2 cache
       */

      sample_rect.x      = factor * x1;
      sample_rect.y      = factor * y1;
      sample_rect.width  = factor * (x2 - x1);
      sample_rect.height = factor * (y2 - y1);
      buf_width  = x2 - x1;
      buf_height = y2 - y1;


      if (buf_height && buf_width)
        switch(interpolation)
        {
          case GEGL_BUFFER_FILTER_NEAREST:

            gegl_buffer_iterate_read_dispatch (buffer, &sample_rect,
                                               (guchar*)sample_buf,
                                               buf_width * bpp,
                                               format, level, repeat_mode);
            sample_rect.x      = x1;
            sample_rect.y      = y1;
            sample_rect.width  = x2 - x1;
            sample_rect.height = y2 - y1;

            gegl_resample_nearest (dest_buf,
                                   sample_buf,
                                   &rect2,
                                   &sample_rect,
                                   buf_width * bpp,
                                   scale,
                                   bpp,
                                   rowstride);
            break;
          case GEGL_BUFFER_FILTER_BILINEAR:
            buf_width  += 1;
            buf_height += 1;

            /* fill the regions of the buffer outside the sampled area with
             * zeros, since they may be involved in the arithmetic.  even
             * though their actual value should have no, or negligible, effect,
             * they must at least be finite, when dealing with float formats.
             */
            {
              guchar *p = sample_buf;
              gint    y;

              for (y = 0; y < buf_height - 1; y++)
                {
                  memset (p + (buf_width - 1) * bpp, 0, bpp);

                  p += buf_width * bpp;
                }

              memset (p, 0, buf_width * bpp);
            }

            gegl_buffer_iterate_read_dispatch (buffer, &sample_rect,
                                       (guchar*)sample_buf,
                                        buf_width * bpp,
                                        format, level, repeat_mode);

            sample_rect.x      = x1;
            sample_rect.y      = y1;
            sample_rect.width  = x2 - x1 + 1;
            sample_rect.height = y2 - y1 + 1;

            gegl_resample_bilinear (dest_buf,
                                    sample_buf,
                                    &rect2,
                                    &sample_rect,
                                    buf_width * bpp,
                                    scale,
                                    format,
                                    rowstride);
            break;
          case GEGL_BUFFER_FILTER_BOX:
          default:
            {
              gint offset;
              buf_width  += 2;
              buf_height += 2;
              offset = (buf_width + 1) * bpp;

              /* fill the regions of the buffer outside the sampled area with
               * zeros, since they may be involved in the arithmetic.  even
               * though their actual value should have no, or negligible,
               * effect, they must at least be finite, when dealing with float
               * formats.
               */
              {
                guchar *p = sample_buf;
                gint    y;

                memset (p, 0, (buf_width - 1) * bpp);

                for (y = 0; y < buf_height - 1; y++)
                  {
                    memset (p + (buf_width - 1) * bpp, 0, 2 * bpp);

                    p += buf_width * bpp;
                  }

                memset (p + bpp, 0, (buf_width - 1) * bpp);
              }

              gegl_buffer_iterate_read_dispatch (buffer, &sample_rect,
                                         (guchar*)sample_buf + offset,
                                          buf_width * bpp,
                                          format, level, repeat_mode);

              sample_rect.x      = x1 - 1;
              sample_rect.y      = y1 - 1;
              sample_rect.width  = x2 - x1 + 2;
              sample_rect.height = y2 - y1 + 2;

              gegl_resample_boxfilter (dest_buf,
                                       sample_buf,
                                       &rect2,
                                       &sample_rect,
                                       buf_width * bpp,
                                       scale,
                                       format,
                                       rowstride);
            }
            break;
      }

    dest_buf = ((guchar*)dest_buf) + rowstride * rect2.height;
    ystart+=rect2.height;
    rect2.y = ystart;
    rect2.height = chunk_height;
    if (rect2.y + rect2.height > rect->y + rect->height)
      rect2.height = (rect->y + rect->height) - rect2.y;

    }

    gegl_scratch_free (sample_buf);
  }
}

void
gegl_buffer_get_unlocked (GeglBuffer          *buffer,
                          gdouble              scale,
                          const GeglRectangle *rect,
                          const Babl          *format,
                          gpointer             dest_buf,
                          gint                 rowstride,
                          GeglAbyssPolicy      repeat_mode)
{
  return _gegl_buffer_get_unlocked (buffer, scale, rect, format, dest_buf, rowstride, repeat_mode);
}

void
gegl_buffer_get (GeglBuffer          *buffer,
                 const GeglRectangle *rect,
                 gdouble              scale,
                 const Babl          *format,
                 gpointer             dest_buf,
                 gint                 rowstride,
                 GeglAbyssPolicy      repeat_mode)
{
  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  gegl_buffer_lock (buffer);
  _gegl_buffer_get_unlocked (buffer, scale, rect, format, dest_buf, rowstride, repeat_mode);
  gegl_buffer_unlock (buffer);
}

static void
gegl_buffer_copy2 (GeglBuffer          *src,
                   const GeglRectangle *src_rect,
                   GeglAbyssPolicy      repeat_mode,
                   GeglBuffer          *dst,
                   const GeglRectangle *dst_rect)
{
  GeglBufferIterator *i;
  gint offset_x = src_rect->x - dst_rect->x;
  gint offset_y = src_rect->y - dst_rect->y;

  i = gegl_buffer_iterator_new (dst, dst_rect, 0, dst->soft_format,
                                GEGL_ACCESS_WRITE | GEGL_ITERATOR_NO_NOTIFY,
                                repeat_mode, 1);
  while (gegl_buffer_iterator_next (i))
    {
      GeglRectangle src_rect = i->items[0].roi;
      src_rect.x += offset_x;
      src_rect.y += offset_y;
      gegl_buffer_iterate_read_dispatch (src, &src_rect, i->items[0].data, 0,
                                         dst->soft_format, 0, repeat_mode);
    }
}

void
gegl_buffer_copy (GeglBuffer          *src,
                  const GeglRectangle *src_rect,
                  GeglAbyssPolicy      repeat_mode,
                  GeglBuffer          *dst,
                  const GeglRectangle *dst_rect)
{
  GeglRectangle real_src_rect;
  GeglRectangle real_dst_rect;

  g_return_if_fail (GEGL_IS_BUFFER (src));
  g_return_if_fail (GEGL_IS_BUFFER (dst));

  if (!src_rect)
    {
      src_rect = gegl_buffer_get_extent (src);
    }
  if (G_UNLIKELY (src_rect->width <= 0 ||
      src_rect->height <= 0))
    return;

  if (!dst_rect)
    {
      dst_rect = src_rect;
    }

  real_dst_rect        = *dst_rect;
  real_dst_rect.width  = src_rect->width;
  real_dst_rect.height = src_rect->height;

  if (G_UNLIKELY (! gegl_rectangle_intersect (&real_dst_rect, &real_dst_rect, &dst->abyss)))
    return;

  real_src_rect    = real_dst_rect;
  real_src_rect.x += src_rect->x - dst_rect->x;
  real_src_rect.y += src_rect->y - dst_rect->y;

  src_rect = &real_src_rect;
  dst_rect = &real_dst_rect;

  if (! gegl_rectangle_intersect (NULL, src_rect, &src->abyss))
    {
      switch (repeat_mode)
        {
        case GEGL_ABYSS_CLAMP:
        case GEGL_ABYSS_LOOP:
          if (! gegl_rectangle_is_empty (&src->abyss))
            break;

          /* fall through */

        case GEGL_ABYSS_NONE:
          {
            const gfloat color[4] = {0.0f, 0.0f, 0.0f, 0.0f};

            gegl_buffer_set_color_from_pixel (dst, dst_rect, color,
                                              gegl_babl_rgba_linear_float ());

            return;
          }

        case GEGL_ABYSS_BLACK:
          {
            const gfloat color[4] = {0.0f, 0.0f, 0.0f, 1.0f};

            gegl_buffer_set_color_from_pixel (dst, dst_rect, color,
                                              gegl_babl_rgba_linear_float ());

            return;
          }

        case GEGL_ABYSS_WHITE:
          {
            const gfloat color[4] = {1.0f, 1.0f, 1.0f, 1.0f};

            gegl_buffer_set_color_from_pixel (dst, dst_rect, color,
                                              gegl_babl_rgba_linear_float ());

            return;
          }

        default:
          break;
        }
    }

  if (src->soft_format == dst->soft_format &&
      src_rect->width >= src->tile_width &&
      src_rect->height >= src->tile_height &&
      src->tile_width == dst->tile_width  &&
      src->tile_height == dst->tile_height &&
      !g_object_get_data (G_OBJECT (dst), "is-linear") &&
      gegl_buffer_scan_compatible (src, src_rect->x, src_rect->y,
                                   dst, dst_rect->x, dst_rect->y))
    {
      gint tile_width  = dst->tile_width;
      gint tile_height = dst->tile_height;

      GeglRectangle cow_rect;
      gint          rem;

      gegl_rectangle_intersect (&cow_rect, src_rect, &src->abyss);

      cow_rect.x += dst_rect->x - src_rect->x;
      cow_rect.y += dst_rect->y - src_rect->y;

      /* adjust origin to match the start of tile alignment */
      rem = (cow_rect.x + dst->shift_x) % tile_width;
      if (rem > 0)
        rem -= tile_width;
      cow_rect.x      -= rem;
      cow_rect.width  += rem;

      rem = (cow_rect.y + dst->shift_y) % tile_height;
      if (rem > 0)
        rem -= tile_height;
      cow_rect.y      -= rem;
      cow_rect.height += rem;

      /* adjust size of rect to match multiple of tiles */
      cow_rect.width  -= cow_rect.width  % tile_width;
      cow_rect.height -= cow_rect.height % tile_height;

      if (cow_rect.width > 0 && cow_rect.height > 0)
        {
          GeglRectangle top, bottom, left, right;

          /* iterate over rectangle that can be cow copied, duplicating
           * one and one tile
           */
          {
            /* first we do a dumb copy,. but with fetched tiles */
            GeglTileSource       *source = GEGL_TILE_SOURCE (src->tile_storage);
            GeglTileHandlerCache *cache  = dst->tile_storage->cache;
            gboolean fast_copy;
            gint dst_x, dst_y;

            /* we only attempt performing a fast copy, using the TILE_COPY
             * command, if the source buffer doesn't have any user-provided
             * tile handlers.  the problem with user-provided tile handlers is
             * that they might intenrally track the validity of tile contents,
             * in a way that's opaque to gegl, and the only way to know for
             * sure a given tile is valid, is to fetch it using TILE_GET.
             *
             * in the future, it might make sense to relax this condition, by
             * adding some api, or changing the requirements of tile handlers.
             */
            fast_copy = (src->tile_storage->n_user_handlers == 0);

            if (src->tile_storage < dst->tile_storage)
              {
                g_rec_mutex_lock (&src->tile_storage->mutex);
                g_rec_mutex_lock (&dst->tile_storage->mutex);
              }
            else
              {
                g_rec_mutex_lock (&dst->tile_storage->mutex);
                g_rec_mutex_lock (&src->tile_storage->mutex);
              }

            for (dst_y = cow_rect.y + dst->shift_y; dst_y < cow_rect.y + dst->shift_y + cow_rect.height; dst_y += tile_height)
            for (dst_x = cow_rect.x + dst->shift_x; dst_x < cow_rect.x + dst->shift_x + cow_rect.width; dst_x += tile_width)
              {
                gint src_x, src_y;
                gint stx, sty, dtx, dty;

                src_x = dst_x + (src_rect->x - dst_rect->x) + (src->shift_x - dst->shift_x);
                src_y = dst_y + (src_rect->y - dst_rect->y) + (src->shift_y - dst->shift_y);

                stx = gegl_tile_indice (src_x, tile_width);
                sty = gegl_tile_indice (src_y, tile_height);
                dtx = gegl_tile_indice (dst_x, tile_width);
                dty = gegl_tile_indice (dst_y, tile_height);

                if (! fast_copy ||
                    ! gegl_tile_source_copy (source, stx, sty, 0,
                                             dst,    dtx, dty, 0))
                  {
                    GeglTile *src_tile;
                    GeglTile *dst_tile;

                    src_tile = gegl_tile_source_get_tile (source, stx, sty, 0);

                    dst_tile = gegl_tile_dup (src_tile);
                    dst_tile->tile_storage = dst->tile_storage;
                    dst_tile->x = dtx;
                    dst_tile->y = dty;
                    dst_tile->z = 0;

                    gegl_tile_handler_cache_insert (cache, dst_tile, dtx, dty, 0);

                    gegl_tile_unref (dst_tile);
                    gegl_tile_unref (src_tile);
                  }
              }

            g_rec_mutex_unlock (&src->tile_storage->mutex);

            gegl_tile_handler_damage_rect (
              GEGL_TILE_HANDLER (dst->tile_storage),
              GEGL_RECTANGLE (cow_rect.x + dst->shift_x,
                              cow_rect.y + dst->shift_y,
                              cow_rect.width,
                              cow_rect.height));

            g_rec_mutex_unlock (&dst->tile_storage->mutex);
          }

          top = *dst_rect;
          top.height = (cow_rect.y - dst_rect->y);


          left = *dst_rect;
          left.y = cow_rect.y;
          left.height = cow_rect.height;
          left.width = (cow_rect.x - dst_rect->x);

          bottom = *dst_rect;
          bottom.y = (cow_rect.y + cow_rect.height);
          bottom.height = (dst_rect->y + dst_rect->height) -
                          (cow_rect.y  + cow_rect.height);

          if (bottom.height < 0)
            bottom.height = 0;

          right  =  *dst_rect;
          right.x = (cow_rect.x + cow_rect.width);
          right.width = (dst_rect->x + dst_rect->width) -
                          (cow_rect.x  + cow_rect.width);
          right.y = cow_rect.y;
          right.height = cow_rect.height;

          if (right.width < 0)
            right.width = 0;

          if (top.height)
          gegl_buffer_copy2 (src,
                             GEGL_RECTANGLE (src_rect->x + (top.x-dst_rect->x),
                                             src_rect->y + (top.y-dst_rect->y),
                                 top.width, top.height),
                             repeat_mode, dst, &top);
          if (bottom.height)
          gegl_buffer_copy2 (src,
                             GEGL_RECTANGLE (src_rect->x + (bottom.x-dst_rect->x),
                                             src_rect->y + (bottom.y-dst_rect->y),
                                 bottom.width, bottom.height),
                             repeat_mode, dst, &bottom);
          if (left.width && left.height)
          gegl_buffer_copy2 (src,
                             GEGL_RECTANGLE (src_rect->x + (left.x-dst_rect->x),
                                             src_rect->y + (left.y-dst_rect->y),
                                 left.width, left.height),
                             repeat_mode, dst, &left);
          if (right.width && right.height)
          gegl_buffer_copy2 (src,
                             GEGL_RECTANGLE (src_rect->x + (right.x-dst_rect->x),
                                             src_rect->y + (right.y-dst_rect->y),
                                 right.width, right.height),
                             repeat_mode, dst, &right);
        }
      else
        {
          gegl_buffer_copy2 (src, src_rect, repeat_mode, dst, dst_rect);
        }
    }
  else
    {
      gegl_buffer_copy2 (src, src_rect, repeat_mode, dst, dst_rect);
    }

  gegl_buffer_emit_changed_signal (dst, dst_rect);
}

typedef void (* GeglBufferTileFunc) (GeglBuffer          *buffer,
                                     gint                 tile_x,
                                     gint                 tile_y,
                                     gpointer             data);
typedef void (* GeglBufferRectFunc) (GeglBuffer          *buffer,
                                     const GeglRectangle *rect,
                                     gpointer             data);

static void
gegl_buffer_foreach_tile (GeglBuffer          *buffer,
                          const GeglRectangle *rect,
                          GeglBufferTileFunc   tile_func,
                          GeglBufferRectFunc   rect_func,
                          gpointer             data)
{
  if (! rect)
    rect = gegl_buffer_get_extent (buffer);

  if (G_UNLIKELY (rect->width <= 0 || rect->height <= 0))
    return;

#if 1
  /* cow for clearing is currently broken */
  if (rect->width  >= buffer->tile_width  &&
      rect->height >= buffer->tile_height &&
      ! g_object_get_data (G_OBJECT (buffer), "is-linear"))
    {
      gint tile_width  = buffer->tile_width;
      gint tile_height = buffer->tile_height;

      GeglRectangle tile_rect = *rect;
      gint          rem;

      /* shift rect to tile coordinate system */
      tile_rect.x += buffer->shift_x;
      tile_rect.y += buffer->shift_y;

      /* adjust origin to match the start of tile alignment */
      rem = tile_rect.x % tile_width;
      if (rem > 0)
        rem -= tile_width;
      tile_rect.x      -= rem;
      tile_rect.width  += rem;

      rem = tile_rect.y % tile_height;
      if (rem > 0)
        rem -= tile_height;
      tile_rect.y      -= rem;
      tile_rect.height += rem;

      /* adjust size of rect to match multiple of tiles */
      tile_rect.width  -= tile_rect.width  % tile_width;
      tile_rect.height -= tile_rect.height % tile_height;

      if (tile_rect.width > 0 && tile_rect.height > 0)
        {
          GeglRectangle top, bottom, left, right;

          /* iterate over the tile rectangle */
          {
            gint x, y;

            g_rec_mutex_lock (&buffer->tile_storage->mutex);

            for (y = tile_rect.y;
                 y < tile_rect.y + tile_rect.height;
                 y += tile_height)
              {
                for (x = tile_rect.x;
                     x < tile_rect.x + tile_rect.width;
                     x += tile_width)
                  {
                    gint tile_x, tile_y;

                    tile_x = gegl_tile_indice (x, tile_width);
                    tile_y = gegl_tile_indice (y, tile_height);

                    tile_func (buffer, tile_x, tile_y, data);
                  }
              }

            gegl_tile_handler_damage_rect (
              GEGL_TILE_HANDLER (buffer->tile_storage),
              &tile_rect);

            g_rec_mutex_unlock (&buffer->tile_storage->mutex);
          }

          /* shift rect to buffer coordinate system */
          tile_rect.x -= buffer->shift_x;
          tile_rect.y -= buffer->shift_y;

          top = *rect;
          top.height = (tile_rect.y - rect->y);


          left = *rect;
          left.y = tile_rect.y;
          left.height = tile_rect.height;
          left.width = (tile_rect.x - rect->x);

          bottom = *rect;
          bottom.y = (tile_rect.y + tile_rect.height);
          bottom.height = (rect->y + rect->height) -
                          (tile_rect.y  + tile_rect.height);

          if (bottom.height < 0)
            bottom.height = 0;

          right  =  *rect;
          right.x = (tile_rect.x + tile_rect.width);
          right.width = (rect->x + rect->width) -
                          (tile_rect.x  + tile_rect.width);
          right.y = tile_rect.y;
          right.height = tile_rect.height;

          if (right.width < 0)
            right.width = 0;

          if (top.height)                  rect_func (buffer, &top,    data);
          if (bottom.height)               rect_func (buffer, &bottom, data);
          if (left.width && left.height)   rect_func (buffer, &left,   data);
          if (right.width && right.height) rect_func (buffer, &right,  data);
        }
      else
        {
          rect_func (buffer, rect, data);
        }
    }
  else
#endif
    {
      rect_func (buffer, rect, data);
    }

  gegl_buffer_emit_changed_signal (buffer, rect);
}

static void
gegl_buffer_clear_tile (GeglBuffer *dst,
                        gint        tile_x,
                        gint        tile_y,
                        gpointer    data)
{
  if (dst->initialized)
    {
      gegl_tile_handler_cache_remove (dst->tile_storage->cache,
                                      tile_x, tile_y, 0);

      gegl_tile_handler_source_command (dst->tile_storage->cache,
                                        GEGL_TILE_VOID,
                                        tile_x, tile_y, 0, NULL);
    }
  else
    {
      GeglTile *tile;

      tile = gegl_tile_handler_empty_new_tile (dst->tile_storage->tile_size);

      gegl_tile_handler_cache_insert (dst->tile_storage->cache, tile,
                                      tile_x, tile_y, 0);

      gegl_tile_unref (tile);
    }
}

static void
gegl_buffer_clear_rect (GeglBuffer          *dst,
                        const GeglRectangle *dst_rect,
                        gpointer             data)
{
  GeglBufferIterator *i;
  gint                pxsize;

  pxsize = babl_format_get_bytes_per_pixel (dst->soft_format);

  if (gegl_buffer_ext_invalidate)
    {
      gegl_buffer_ext_invalidate (dst, dst_rect);
    }

  i = gegl_buffer_iterator_new (dst, dst_rect, 0, dst->soft_format,
                                GEGL_ACCESS_WRITE | GEGL_ITERATOR_NO_NOTIFY,
                                GEGL_ABYSS_NONE, 1);
  while (gegl_buffer_iterator_next (i))
    {
      memset (((guchar*)(i->items[0].data)), 0, i->length * pxsize);
    }
}

void
gegl_buffer_clear (GeglBuffer          *dst,
                   const GeglRectangle *dst_rect)
{
  g_return_if_fail (GEGL_IS_BUFFER (dst));

  gegl_buffer_foreach_tile (dst, dst_rect,
                            gegl_buffer_clear_tile,
                            gegl_buffer_clear_rect,
                            NULL);
}

void
gegl_buffer_set_pattern (GeglBuffer          *buffer,
                         const GeglRectangle *rect,
                         GeglBuffer          *pattern,
                         gint                 x_offset,
                         gint                 y_offset)
{
  const GeglRectangle *pattern_extent;
  const Babl          *buffer_format;
  GeglRectangle        roi;                  /* the rect if not NULL, else the whole buffer */
  GeglRectangle        pattern_data_extent;  /* pattern_extent clamped to rect */
  GeglRectangle        extended_data_extent; /* many patterns to avoid copying too small chunks of data */
  gint                 bpp;
  gint                 x, y;
  gint                 rowstride;
  gpointer             pattern_data;

  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  g_return_if_fail (GEGL_IS_BUFFER (pattern));

  if (rect != NULL)
    roi = *rect;
  else
    roi = *gegl_buffer_get_extent (buffer);

  pattern_extent = gegl_buffer_get_extent (pattern);
  buffer_format  = gegl_buffer_get_format (buffer);

  pattern_data_extent.x      = - x_offset + roi.x;
  pattern_data_extent.y      = - y_offset + roi.y;
  pattern_data_extent.width  = MIN (pattern_extent->width,  roi.width);
  pattern_data_extent.height = MIN (pattern_extent->height, roi.height);

  /* Sanity */
  if (pattern_data_extent.width < 1 || pattern_data_extent.height < 1)
    return;

  bpp = babl_format_get_bytes_per_pixel (buffer_format);

  extended_data_extent = pattern_data_extent;

  /* Avoid gegl_buffer_set on too small chunks */
  extended_data_extent.width  *= (buffer->tile_width  * 2 +
                                  (extended_data_extent.width  - 1)) /
                                 extended_data_extent.width;
  extended_data_extent.width   = MIN (extended_data_extent.width,  roi.width);

  extended_data_extent.height *= (buffer->tile_height * 2 +
                                  (extended_data_extent.height - 1)) /
                                 extended_data_extent.height;
  extended_data_extent.height  = MIN (extended_data_extent.height, roi.height);

  /* XXX: Bad taste, the pattern needs to be small enough.
   * See Bug 712814 for an alternative malloc-free implementation */
  pattern_data = gegl_scratch_alloc (extended_data_extent.width *
                                     extended_data_extent.height *
                                     bpp);

  rowstride = extended_data_extent.width * bpp;

  /* only do babl conversions once on the whole pattern */
  gegl_buffer_get (pattern, &pattern_data_extent, 1.0,
                   buffer_format, pattern_data,
                   rowstride, GEGL_ABYSS_LOOP);

  /* fill the remaining space by duplicating the small pattern */
  for (y = 0; y < pattern_data_extent.height; y++)
    for (x = pattern_extent->width;
         x < extended_data_extent.width;
         x *= 2)
      {
        guchar *src  = ((guchar*) pattern_data) + y * rowstride;
        guchar *dst  = src + x * bpp;
        gint    size = bpp * MIN (extended_data_extent.width - x, x);
        memcpy (dst, src, size);
      }

  for (y = pattern_extent->height;
       y < extended_data_extent.height;
       y *= 2)
    {
      guchar *src  = ((guchar*) pattern_data);
      guchar *dst  = src + y * rowstride;
      gint    size = rowstride * MIN (extended_data_extent.height - y, y);
      memcpy (dst, src, size);
    }

  /* Now fill the acutal buffer */
  for (y = roi.y; y < roi.y + roi.height; y += extended_data_extent.height)
    for (x = roi.x; x < roi.x + roi.width; x += extended_data_extent.width)
      {
        GeglRectangle dest_rect = {x, y,
                                   extended_data_extent.width,
                                   extended_data_extent.height};

        gegl_rectangle_intersect (&dest_rect, &dest_rect, &roi);

        gegl_buffer_set (buffer, &dest_rect, 0, buffer_format,
                         pattern_data, rowstride);
      }

  gegl_scratch_free (pattern_data);
}

typedef struct
{
  gconstpointer  pixel;
  gint           bpp;

  GeglTile      *tile;
} SetColorFromPixelData;

static void
gegl_buffer_set_color_from_pixel_tile (GeglBuffer            *dst,
                                       gint                   tile_x,
                                       gint                   tile_y,
                                       SetColorFromPixelData *data)
{
  GeglTile *tile;

  if (data->tile)
    {
      tile = gegl_tile_dup (data->tile);
    }
  else
    {
      gint tile_size = dst->tile_storage->tile_size;

      if (gegl_memeq_zero (data->pixel, data->bpp))
        {
          tile = gegl_tile_handler_empty_new_tile (tile_size);
        }
      else
        {
          tile = gegl_tile_new (tile_size);

          gegl_tile_lock (tile);

          gegl_memset_pattern (gegl_tile_get_data (tile),
                               data->pixel,
                               data->bpp,
                               tile_size / data->bpp);

          gegl_tile_unlock (tile);
        }
    }

  gegl_tile_handler_cache_insert (dst->tile_storage->cache, tile,
                                  tile_x, tile_y, 0);

  if (data->tile)
    gegl_tile_unref (tile);
  else
    data->tile = tile;
}

static void
gegl_buffer_set_color_from_pixel_rect (GeglBuffer            *dst,
                                       const GeglRectangle   *dst_rect,
                                       SetColorFromPixelData *data)
{
  GeglBufferIterator *i;

  i = gegl_buffer_iterator_new (dst, dst_rect, 0, dst->soft_format,
                                GEGL_ACCESS_WRITE | GEGL_ITERATOR_NO_NOTIFY,
                                GEGL_ABYSS_NONE, 1);
  while (gegl_buffer_iterator_next (i))
    {
      gegl_memset_pattern (i->items[0].data,
                           data->pixel,
                           data->bpp,
                           i->length);
    }
}

void
gegl_buffer_set_color_from_pixel (GeglBuffer          *dst,
                                  const GeglRectangle *dst_rect,
                                  gconstpointer        pixel,
                                  const Babl          *pixel_format)
{
  SetColorFromPixelData data = {};

  g_return_if_fail (GEGL_IS_BUFFER (dst));
  g_return_if_fail (pixel);
  if (pixel_format == NULL)
    pixel_format = dst->soft_format;

  if (!dst_rect)
    {
      dst_rect = gegl_buffer_get_extent (dst);
    }
  if (dst_rect->width <= 0 ||
      dst_rect->height <= 0)
    return;

  data.bpp = babl_format_get_bytes_per_pixel (dst->soft_format);

  /* convert the pixel data to the buffer format */
  if (pixel_format == dst->soft_format)
    {
      data.pixel = pixel;
    }
  else
    {
      data.pixel = g_alloca (data.bpp);

      babl_process (babl_fish (pixel_format, dst->soft_format),
                    pixel, (gpointer) data.pixel, 1);
    }

  gegl_buffer_foreach_tile (
    dst, dst_rect,
    (GeglBufferTileFunc) gegl_buffer_set_color_from_pixel_tile,
    (GeglBufferRectFunc) gegl_buffer_set_color_from_pixel_rect,
    &data);

  if (data.tile)
    gegl_tile_unref (data.tile);
}

GeglBuffer *
gegl_buffer_dup (GeglBuffer *buffer)
{
  GeglBuffer *new_buffer;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  new_buffer = g_object_new (GEGL_TYPE_BUFFER,
                             "format",       buffer->soft_format,
                             "x",            buffer->extent.x,
                             "y",            buffer->extent.y,
                             "width",        buffer->extent.width,
                             "height",       buffer->extent.height,
                             "abyss-x",      buffer->abyss.x,
                             "abyss-y",      buffer->abyss.y,
                             "abyss-width",  buffer->abyss.width,
                             "abyss-height", buffer->abyss.height,
                             "shift-x",      buffer->shift_x,
                             "shift-y",      buffer->shift_y,
                             "tile-width",   buffer->tile_width,
                             "tile-height",  buffer->tile_height,
                             NULL);

  gegl_buffer_copy (buffer, gegl_buffer_get_extent (buffer), GEGL_ABYSS_NONE,
                    new_buffer, gegl_buffer_get_extent (buffer));

  return new_buffer;
}

/*
 *  check whether iterations on two buffers starting from the given coordinates with
 *  the same width and height would be able to run parallell.
 */
gboolean gegl_buffer_scan_compatible (GeglBuffer *bufferA,
                                      gint        xA,
                                      gint        yA,
                                      GeglBuffer *bufferB,
                                      gint        xB,
                                      gint        yB)
{
  if (bufferA->tile_storage->tile_width !=
      bufferB->tile_storage->tile_width)
    return FALSE;
  if (bufferA->tile_storage->tile_height !=
      bufferB->tile_storage->tile_height)
    return FALSE;
  if ( (abs((bufferA->shift_x+xA) - (bufferB->shift_x+xB))
        % bufferA->tile_storage->tile_width) != 0)
    return FALSE;
  if ( (abs((bufferA->shift_y+yA) - (bufferB->shift_y+yB))
        % bufferA->tile_storage->tile_height) != 0)
    return FALSE;
  return TRUE;
}

