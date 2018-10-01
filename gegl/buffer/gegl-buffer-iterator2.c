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
 * Copyright 2008,2011,2012,2014,2017 Øyvind Kolås <pippin@gimp.org>
 *           2013 Daniel Sabo
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <glib-object.h>
#include <glib/gprintf.h>

#include "gegl-buffer.h"
#include "gegl-buffer-types.h"
#include "gegl-rectangle.h"
#include "gegl-buffer-iterator.h"
#include "gegl-buffer-iterator2.h"
#include "gegl-buffer-iterator-private.h"
#include "gegl-buffer-private.h"

typedef enum {
  GeglIteratorState_Start,
  GeglIteratorState_InTile,
  GeglIteratorState_InRows,
  GeglIteratorState_Linear,
  GeglIteratorState_Stop,
  GeglIteratorState_Invalid,
} GeglIteratorState;

typedef enum {
  GeglIteratorTileMode_Invalid,
  GeglIteratorTileMode_DirectTile,
  GeglIteratorTileMode_LinearTile,
  GeglIteratorTileMode_GetBuffer,
  GeglIteratorTileMode_Empty,
} GeglIteratorTileMode;

typedef struct _SubIterState {
  GeglRectangle        full_rect; /* The entire area we are iterating over */
  GeglBuffer          *buffer;
  GeglAccessMode       access_mode;
  GeglAbyssPolicy      abyss_policy;
  const Babl          *format;
  gint                 format_bpp;
  GeglIteratorTileMode current_tile_mode;
  gint                 row_stride;
  GeglRectangle        real_roi;
  gint                 level;
  /* Direct data members */
  GeglTile            *current_tile;
  /* Indirect data members */
  gpointer             real_data;
  /* Linear data members */
  GeglTile            *linear_tile;
  gpointer             linear;
} SubIterState;

struct _GeglBufferIterator2Priv
{
  gint              num_buffers;
  GeglIteratorState state;
  GeglRectangle     origin_tile;
  gint              remaining_rows;
  gint              max_slots;
  SubIterState      sub_iter[];
  /* gint           access_order[]; */ /* allocated, but accessed through
                                        * get_access_order().
                                        */
};

static inline gint *
get_access_order (GeglBufferIterator2 *iter)
{
  GeglBufferIterator2Priv *priv = iter->priv;

  return (gint *) &priv->sub_iter[priv->max_slots];
}

static inline GeglBufferIterator2 *
_gegl_buffer_iterator2_empty_new (gint max_slots)
{
  GeglBufferIterator2 *iter = g_malloc0 (sizeof (GeglBufferIterator2) +
                                         max_slots * sizeof (GeglBufferIterator2Item) +
                                         sizeof (GeglBufferIterator2Priv) +
                                         max_slots * sizeof (SubIterState) +
                                         max_slots * sizeof (gint));
  iter->priv      = (void*)(((char*)iter) + sizeof (GeglBufferIterator2) +
                                         max_slots * sizeof (GeglBufferIterator2Item));

  iter->priv->max_slots = max_slots;

  iter->priv->num_buffers = 0;
  iter->priv->state       = GeglIteratorState_Start;

  return iter;
}


GeglBufferIterator2 *
gegl_buffer_iterator2_empty_new (gint max_slots)
{
  return _gegl_buffer_iterator2_empty_new (max_slots);
}


static inline int
_gegl_buffer_iterator2_add (GeglBufferIterator2  *iter,
                          GeglBuffer          *buf,
                          const GeglRectangle *roi,
                          gint                 level,
                          const Babl          *format,
                          GeglAccessMode       access_mode,
                          GeglAbyssPolicy      abyss_policy)
{
  GeglBufferIterator2Priv *priv = iter->priv;
  int                     index;
  SubIterState           *sub;

  g_return_val_if_fail (priv->num_buffers < priv->max_slots, 0);

  index = priv->num_buffers++;
  sub = &priv->sub_iter[index];

  if (!format)
    format = gegl_buffer_get_format (buf);

  if (!roi)
    roi = &buf->extent;

  if (index == 0 && (roi->width <= 0 || roi->height <= 0))
    priv->state = GeglIteratorState_Invalid;

  if (priv->state != GeglIteratorState_Invalid)
    {
      sub->buffer       = buf;
      sub->full_rect    = *roi;

      sub->access_mode  = access_mode;
      sub->abyss_policy = abyss_policy;
      sub->current_tile = NULL;
      sub->real_data    = NULL;
      sub->linear_tile  = NULL;
      sub->format       = format;
      sub->format_bpp   = babl_format_get_bytes_per_pixel (format);
      sub->level        = level;

      if (index > 0)
        {
          priv->sub_iter[index].full_rect.width  = priv->sub_iter[0].full_rect.width;
          priv->sub_iter[index].full_rect.height = priv->sub_iter[0].full_rect.height;
        }
    }

  return index;
}

int
gegl_buffer_iterator2_add (GeglBufferIterator2  *iter,
                          GeglBuffer          *buf,
                          const GeglRectangle *roi,
                          gint                 level,
                          const Babl          *format,
                          GeglAccessMode       access_mode,
                          GeglAbyssPolicy      abyss_policy)
{
  return _gegl_buffer_iterator2_add (iter, buf, roi, level, format, access_mode,
abyss_policy);
}


GeglBufferIterator2 *
gegl_buffer_iterator2_new (GeglBuffer          *buf,
                          const GeglRectangle *roi,
                          gint                 level,
                          const Babl          *format,
                          GeglAccessMode       access_mode,
                          GeglAbyssPolicy      abyss_policy,
                          gint                 max_slots)
{
  GeglBufferIterator2 *iter = _gegl_buffer_iterator2_empty_new (max_slots);
  _gegl_buffer_iterator2_add (iter, buf, roi, level, format,
                             access_mode, abyss_policy);

  return iter;
}

static inline void
release_tile (GeglBufferIterator2 *iter,
              int index)
{
  GeglBufferIterator2Priv *priv = iter->priv;
  SubIterState           *sub  = &priv->sub_iter[index];

  if (sub->current_tile_mode == GeglIteratorTileMode_DirectTile)
    {
      if (sub->access_mode & GEGL_ACCESS_WRITE)
        gegl_tile_unlock_no_void (sub->current_tile);
      else
        gegl_tile_read_unlock (sub->current_tile);
      gegl_tile_unref (sub->current_tile);

      sub->current_tile = NULL;
      iter->items[index].data = NULL;

      sub->current_tile_mode = GeglIteratorTileMode_Empty;
    }
  else if (sub->current_tile_mode == GeglIteratorTileMode_LinearTile)
    {
      sub->current_tile = NULL;
      iter->items[index].data = NULL;

      sub->current_tile_mode = GeglIteratorTileMode_Empty;
    }
  else if (sub->current_tile_mode == GeglIteratorTileMode_GetBuffer)
    {
      if (sub->access_mode & GEGL_ACCESS_WRITE)
        {
          gegl_buffer_set_unlocked_no_notify (sub->buffer,
                                              &sub->real_roi,
                                              sub->level,
                                              sub->format,
                                              sub->real_data,
                                              GEGL_AUTO_ROWSTRIDE);
        }

      gegl_free (sub->real_data);
      sub->real_data = NULL;
      iter->items[index].data = NULL;

      sub->current_tile_mode = GeglIteratorTileMode_Empty;
    }
  else if (sub->current_tile_mode == GeglIteratorTileMode_Empty)
    {
      return;
    }
  else
    {
      g_warn_if_reached ();
    }
}

static inline void
retile_subs (GeglBufferIterator2 *iter,
             int                 x,
             int                 y)
{
  GeglBufferIterator2Priv *priv = iter->priv;
  GeglRectangle real_roi;
  int index;

  int shift_x = priv->origin_tile.x;
  int shift_y = priv->origin_tile.y;

  int tile_x = gegl_tile_indice (x + shift_x, priv->origin_tile.width);
  int tile_y = gegl_tile_indice (y + shift_y, priv->origin_tile.height);

  /* Reset tile size */
  real_roi.x = (tile_x * priv->origin_tile.width)  - shift_x;
  real_roi.y = (tile_y * priv->origin_tile.height) - shift_y;
  real_roi.width  = priv->origin_tile.width;
  real_roi.height = priv->origin_tile.height;

  /* Trim tile down to the iteration roi */
  gegl_rectangle_intersect (&iter->items[0].roi, &real_roi, &priv->sub_iter[0].full_rect);
  priv->sub_iter[0].real_roi = iter->items[0].roi;

  for (index = 1; index < priv->num_buffers; index++)
    {
      SubIterState *lead_sub = &priv->sub_iter[0];
      SubIterState *sub = &priv->sub_iter[index];

      int roi_offset_x = sub->full_rect.x - lead_sub->full_rect.x;
      int roi_offset_y = sub->full_rect.y - lead_sub->full_rect.y;

      iter->items[index].roi.x = iter->items[0].roi.x + roi_offset_x;
      iter->items[index].roi.y = iter->items[0].roi.y + roi_offset_y;
      iter->items[index].roi.width  = iter->items[0].roi.width;
      iter->items[index].roi.height = iter->items[0].roi.height;
      sub->real_roi = iter->items[index].roi;
    }
}

static inline gboolean
initialize_rects (GeglBufferIterator2 *iter)
{
  GeglBufferIterator2Priv *priv = iter->priv;
  SubIterState           *sub  = &priv->sub_iter[0];

  retile_subs (iter, sub->full_rect.x, sub->full_rect.y);

  return TRUE;
}

static inline gboolean
increment_rects (GeglBufferIterator2 *iter)
{
  GeglBufferIterator2Priv *priv = iter->priv;
  SubIterState           *sub  = &priv->sub_iter[0];

  /* Next tile in row */
  int x = iter->items[0].roi.x + iter->items[0].roi.width;
  int y = iter->items[0].roi.y;

  if (x >= sub->full_rect.x + sub->full_rect.width)
    {
      /* Next row */
      x  = sub->full_rect.x;
      y += iter->items[0].roi.height;

      if (y >= sub->full_rect.y + sub->full_rect.height)
        {
          /* All done */
          return FALSE;
        }
    }

  retile_subs (iter, x, y);

  return TRUE;
}

static inline void
get_tile (GeglBufferIterator2 *iter,
          int                 index)
{
  GeglBufferIterator2Priv *priv = iter->priv;
  SubIterState           *sub  = &priv->sub_iter[index];

  GeglBuffer *buf = priv->sub_iter[index].buffer;

  if (sub->linear_tile)
    {
      sub->current_tile = sub->linear_tile;

      sub->real_roi = buf->extent;

      sub->current_tile_mode = GeglIteratorTileMode_LinearTile;
    }
  else
    {
      int shift_x = buf->shift_x;
      int shift_y = buf->shift_y;

      int tile_width  = buf->tile_width;
      int tile_height = buf->tile_height;

      int tile_x = gegl_tile_indice (iter->items[index].roi.x + shift_x, tile_width);
      int tile_y = gegl_tile_indice (iter->items[index].roi.y + shift_y, tile_height);

      sub->current_tile = gegl_buffer_get_tile (buf, tile_x, tile_y, sub->level);

      if (sub->access_mode & GEGL_ACCESS_WRITE)
        gegl_tile_lock (sub->current_tile);
      else
        gegl_tile_read_lock (sub->current_tile);

      sub->real_roi.x = (tile_x * tile_width)  - shift_x;
      sub->real_roi.y = (tile_y * tile_height) - shift_y;
      sub->real_roi.width  = tile_width;
      sub->real_roi.height = tile_height;

      sub->current_tile_mode = GeglIteratorTileMode_DirectTile;
    }

  sub->row_stride = buf->tile_width * sub->format_bpp;

  iter->items[index].data = gegl_tile_get_data (sub->current_tile);
}

static inline double
level_to_scale (int level)
{
  return level?1.0/(1<<level):1.0;
}

static inline void
get_indirect (GeglBufferIterator2 *iter,
              int        index)
{
  GeglBufferIterator2Priv *priv = iter->priv;
  SubIterState           *sub  = &priv->sub_iter[index];

  sub->real_data = gegl_malloc (sub->format_bpp * sub->real_roi.width * sub->real_roi.height);

  if (sub->access_mode & GEGL_ACCESS_READ)
    {
      gegl_buffer_get_unlocked (sub->buffer, level_to_scale (sub->level), &sub->real_roi, sub->format, sub->real_data,
                                GEGL_AUTO_ROWSTRIDE, sub->abyss_policy);
    }

  sub->row_stride = sub->real_roi.width * sub->format_bpp;

  iter->items[index].data = sub->real_data;
  sub->current_tile_mode = GeglIteratorTileMode_GetBuffer;
}

static inline gboolean
needs_indirect_read (GeglBufferIterator2 *iter,
                     int        index)
{
  GeglBufferIterator2Priv *priv = iter->priv;
  SubIterState           *sub  = &priv->sub_iter[index];

  if (sub->access_mode & GEGL_ITERATOR_INCOMPATIBLE)
    return TRUE;

  /* Needs abyss generation */
  if (!gegl_rectangle_contains (&sub->buffer->abyss, &iter->items[index].roi))
    return TRUE;

  return FALSE;
}

static inline gboolean
needs_rows (GeglBufferIterator2 *iter,
            int        index)
{
  GeglBufferIterator2Priv *priv = iter->priv;
  SubIterState           *sub  = &priv->sub_iter[index];

  if (sub->current_tile_mode == GeglIteratorTileMode_GetBuffer)
   return FALSE;

  if (iter->items[index].roi.width  != sub->buffer->tile_width ||
      iter->items[index].roi.height != sub->buffer->tile_height)
    return TRUE;

  return FALSE;
}

/* Do the final setup of the iter struct */
static inline void
prepare_iteration (GeglBufferIterator2 *iter)
{
  GeglBufferIterator2Priv *priv = iter->priv;
  gint *access_order = get_access_order (iter);
  gint origin_offset_x;
  gint origin_offset_y;
  gint i;

  /* Set up the origin tile */
  /* FIXME: Pick the most compatable buffer, not just the first */
  {
    GeglBuffer *buf = priv->sub_iter[0].buffer;

    priv->origin_tile.x      = buf->shift_x;
    priv->origin_tile.y      = buf->shift_y;
    priv->origin_tile.width  = buf->tile_width;
    priv->origin_tile.height = buf->tile_height;

    origin_offset_x = buf->shift_x + priv->sub_iter[0].full_rect.x;
    origin_offset_y = buf->shift_y + priv->sub_iter[0].full_rect.y;
  }

  /* Set up access order */
  {
    gint i_write = 0;
    gint i_read  = priv->num_buffers - 1;
    gint index;

    /* Sort the write-access sub-iterators before the read-access ones */

    for (index = 0; index < priv->num_buffers; index++)
      {
        SubIterState *sub = &priv->sub_iter[index];

        if (sub->access_mode & GEGL_ACCESS_WRITE)
          access_order[i_write++] = index;
        else
          access_order[i_read--]  = index;
      }
  }

  for (i = 0; i < priv->num_buffers; i++)
    {
      gint          index = access_order[i];
      SubIterState *sub   = &priv->sub_iter[index];
      GeglBuffer   *buf   = sub->buffer;

      gint current_offset_x = buf->shift_x + priv->sub_iter[index].full_rect.x;
      gint current_offset_y = buf->shift_y + priv->sub_iter[index].full_rect.y;

      /* Format converison needed */
      if (gegl_buffer_get_format (sub->buffer) != sub->format)
        sub->access_mode |= GEGL_ITERATOR_INCOMPATIBLE;
      /* Incompatiable tiles */
      else if ((priv->origin_tile.width  != buf->tile_width) ||
               (priv->origin_tile.height != buf->tile_height) ||
               (abs(origin_offset_x - current_offset_x) % priv->origin_tile.width != 0) ||
               (abs(origin_offset_y - current_offset_y) % priv->origin_tile.height != 0))
        {
          /* Check if the buffer is a linear buffer */
          if ((buf->extent.x      == -buf->shift_x) &&
              (buf->extent.y      == -buf->shift_y) &&
              (buf->extent.width  == buf->tile_width) &&
              (buf->extent.height == buf->tile_height))
            {
              sub->linear_tile = gegl_buffer_get_tile (sub->buffer, 0, 0, 0);

              if (sub->access_mode & GEGL_ACCESS_WRITE)
                gegl_tile_lock (sub->linear_tile);
              else
                gegl_tile_read_lock (sub->linear_tile);
            }
          else
            sub->access_mode |= GEGL_ITERATOR_INCOMPATIBLE;
        }

      gegl_buffer_lock (sub->buffer);
    }
}

static inline void
load_rects (GeglBufferIterator2 *iter)
{
  GeglBufferIterator2Priv *priv = iter->priv;
  const gint *access_order = get_access_order (iter);
  GeglIteratorState next_state = GeglIteratorState_InTile;
  gint i;

  for (i = 0; i < priv->num_buffers; i++)
    {
      gint index = access_order[i];

      if (needs_indirect_read (iter, index))
        get_indirect (iter, index);
      else
        get_tile (iter, index);

      if ((next_state != GeglIteratorState_InRows) && needs_rows (iter, index))
        {
          next_state = GeglIteratorState_InRows;
        }
    }

  if (next_state == GeglIteratorState_InRows)
    {
      gint index;

      if (iter->items[0].roi.height == 1)
        next_state = GeglIteratorState_InTile;

      priv->remaining_rows = iter->items[0].roi.height - 1;

      for (index = 0; index < priv->num_buffers; index++)
        {
          SubIterState *sub = &priv->sub_iter[index];

          int offset_x = iter->items[index].roi.x - sub->real_roi.x;
          int offset_y = iter->items[index].roi.y - sub->real_roi.y;

          iter->items[index].data = ((char *)iter->items[index].data) + (offset_y * sub->row_stride + offset_x * sub->format_bpp);
          iter->items[index].roi.height = 1;
        }
    }

  iter->length = iter->items[0].roi.width * iter->items[0].roi.height;
  priv->state  = next_state;
}

static inline void
_gegl_buffer_iterator2_stop (GeglBufferIterator2 *iter)
{
  GeglBufferIterator2Priv *priv = iter->priv;
  const gint *access_order = get_access_order (iter);
  gint i;

  if (priv->state != GeglIteratorState_Invalid)
    {
      priv->state = GeglIteratorState_Invalid;

      for (i = priv->num_buffers - 1; i >= 0; i--)
        {
          gint          index = access_order[i];
          SubIterState *sub   = &priv->sub_iter[index];

          if (sub->current_tile_mode != GeglIteratorTileMode_Empty)
            release_tile (iter, index);

          if (sub->linear_tile)
            {
              if (sub->access_mode & GEGL_ACCESS_WRITE)
                gegl_tile_unlock_no_void (sub->linear_tile);
              else
                gegl_tile_read_unlock (sub->linear_tile);
              gegl_tile_unref (sub->linear_tile);
            }

          if (sub->level == 0                      &&
              sub->access_mode & GEGL_ACCESS_WRITE &&
              ! (sub->access_mode & GEGL_ITERATOR_INCOMPATIBLE))
            {
              GeglRectangle damage_rect;

              damage_rect.x      = sub->full_rect.x + sub->buffer->shift_x;
              damage_rect.y      = sub->full_rect.y + sub->buffer->shift_y;
              damage_rect.width  = sub->full_rect.width;
              damage_rect.height = sub->full_rect.height;

              gegl_tile_handler_damage_rect (
                GEGL_TILE_HANDLER (sub->buffer->tile_storage),
                &damage_rect);
            }

          gegl_buffer_unlock (sub->buffer);

          if ((sub->access_mode & GEGL_ACCESS_WRITE) &&
              ! (sub->access_mode & GEGL_ITERATOR_NO_NOTIFY))
            {
              gegl_buffer_emit_changed_signal (sub->buffer, &sub->full_rect);
            }
        }
    }

  g_free (iter);
}

void
gegl_buffer_iterator2_stop (GeglBufferIterator2 *iter)
{
  _gegl_buffer_iterator2_stop (iter);
}


static void linear_shortcut (GeglBufferIterator2 *iter)
{
  GeglBufferIterator2Priv *priv = iter->priv;
  const gint *access_order = get_access_order (iter);
  gint index0 = access_order[0];
  SubIterState *sub0 = &priv->sub_iter[index0];
  gint i;

  for (i = 0; i < priv->num_buffers; i++)
  {
    gint          index        = access_order[i];
    SubIterState *sub          = &priv->sub_iter[index];

    sub->real_roi    = sub0->full_rect;
    sub->real_roi.x += sub->full_rect.x - sub0->full_rect.x;
    sub->real_roi.y += sub->full_rect.y - sub0->full_rect.y;

    iter->items[index].roi = sub->real_roi;

    gegl_buffer_lock (sub->buffer);

    if (index == index0)
    {
      get_tile (iter, index);
    }
    else if (sub->buffer == sub0->buffer && sub->format == sub0->format)
    {
      g_print ("!\n");
      iter->items[index].data = iter->items[index0].data;
    }
    else if (sub->buffer->tile_width == sub->buffer->extent.width
             && sub->buffer->tile_height == sub->buffer->extent.height
             && sub->buffer->extent.x == iter->items[index].roi.x
             && sub->buffer->extent.y == iter->items[index].roi.y)
    {
      get_tile (iter, index);
    }
    else
    {
      get_indirect (iter, index);
    }
  }

  iter->length = iter->items[0].roi.width * iter->items[0].roi.height;
  priv->state  = GeglIteratorState_Stop; /* quit on next iterator_next */
}

gboolean
gegl_buffer_iterator2_next (GeglBufferIterator2 *iter)
{
  GeglBufferIterator2Priv *priv = iter->priv;
  const gint *access_order = get_access_order (iter);

  if (priv->state == GeglIteratorState_Start)
    {
      gint          index0  = access_order[0];
      SubIterState *sub0    = &priv->sub_iter[index0];
      GeglBuffer   *primary = sub0->buffer;
      gint          index;

      if (primary->tile_width == primary->extent.width
          && primary->tile_height == primary->extent.height
          && sub0->full_rect.width == primary->tile_width
          && sub0->full_rect.height == primary->tile_height
          && sub0->full_rect.x == primary->extent.x
          && sub0->full_rect.y == primary->extent.y
          && primary->shift_x == 0
          && primary->shift_y == 0
          && FALSE) /* XXX: conditions are not strict enough, GIMPs TIFF
                       plug-in fails; but GEGLs buffer test suite passes

                       XXX: still? */
      {
        if (gegl_buffer_ext_flush)
          for (index = 0; index < priv->num_buffers; index++)
            {
              SubIterState *sub = &priv->sub_iter[index];
              gegl_buffer_ext_flush (sub->buffer, &sub->full_rect);
            }
        linear_shortcut (iter);
        return TRUE;
      }

      prepare_iteration (iter);

      if (gegl_buffer_ext_flush)
        for (index = 0; index < priv->num_buffers; index++)
          {
            SubIterState *sub = &priv->sub_iter[index];
            gegl_buffer_ext_flush (sub->buffer, &sub->full_rect);
          }

      initialize_rects (iter);

      load_rects (iter);

      return TRUE;
    }
  else if (priv->state == GeglIteratorState_InRows)
    {
      gint index;

      for (index = 0; index < priv->num_buffers; index++)
        {
          iter->items[index].data   = ((char *)iter->items[index].data) + priv->sub_iter[index].row_stride;
          iter->items[index].roi.y += 1;
        }

      priv->remaining_rows -= 1;

      if (priv->remaining_rows == 0)
        priv->state = GeglIteratorState_InTile;

      return TRUE;
    }
  else if (priv->state == GeglIteratorState_InTile)
    {
      gint i;

      for (i = priv->num_buffers - 1; i >= 0; i--)
        {
          gint index = access_order[i];

          release_tile (iter, index);
        }

      if (increment_rects (iter) == FALSE)
        {
          _gegl_buffer_iterator2_stop (iter);
          return FALSE;
        }

      load_rects (iter);

      return TRUE;
    }
  else
    {
      _gegl_buffer_iterator2_stop (iter);
      return FALSE;
    }
}
