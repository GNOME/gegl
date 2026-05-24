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
 * Copyright 2008-2018 Øyvind Kolås <pippin@gimp.org>
 *           2013 Daniel Sabo
 */

#ifndef __GEGL_BUFFER_ITERATOR_H__
#define __GEGL_BUFFER_ITERATOR_H__

#include "gegl-buffer.h"

G_BEGIN_DECLS

#define GEGL_BUFFER_READ      GEGL_ACCESS_READ
#define GEGL_BUFFER_WRITE     GEGL_ACCESS_WRITE
#define GEGL_BUFFER_READWRITE GEGL_ACCESS_READWRITE

typedef struct _GeglBufferIteratorPriv GeglBufferIteratorPriv;

/***
 * GeglBufferIteratorItem:
 * @data: a linear chunk of memory.
 * @roi: a rectangle whose size is the number of pixels fitting into @data and
 *   whose position is the offset wrt the original ROI of the iterator.
 *
 * A handle of a buffer sub-iterator managed by [struct@Gegl.BufferIterator].
 */

typedef struct GeglBufferIteratorItem
{
  gpointer      data;
  GeglRectangle roi;
} GeglBufferIteratorItem;

/***
 * GeglBufferIterator:
 * @length: the number of pixels in the linear chunk of memory fitting in the
 *   first [struct@Gegl.BufferIteratorItem].
 * @items: the list of all [struct@Gegl.BufferIteratorItem].
 *
 * [struct@Gegl.BufferIterator] allows to iterate over one or more
 * [class@Gegl.Buffer]. In each iteration the new data is made available as a
 * linear chunk of memory. For 2 and more buffers the linear chunks of memory
 * are matched exactly in size and are aligned based on the offset between the
 * ROI of the first buffer and the other buffer.
 *
 * For example, if [struct@Gegl.BufferIterator] is constructed with two
 * sub-iterators where the second one has a larger ROI than the first, the
 * content of the second buffer, in the area outside of the ROI of the first
 * iterator, will not be iterated over.
 *
 * See [ctor@Gegl.BufferIterator.new] and [method@Gegl.BufferIterator.next].
 */

typedef struct GeglBufferIterator
{
  gint                    length;
  GeglBufferIteratorPriv *priv;
  GeglBufferIteratorItem  items[];
} GeglBufferIterator;


/**
 * gegl_buffer_iterator_empty_new: (skip)
 * Create a new buffer iterator without adding any sub-iterators.
 *
 * Returns: a new [struct@Gegl.BufferIterator] without any sub-iterators.
 */
GeglBufferIterator *gegl_buffer_iterator_empty_new (int max_slots);

/**
 * gegl_buffer_iterator_new: (skip)
 * @buffer: a [class@Gegl.Buffer] to iterate over
 * @roi: the region in @buffer to iterate over
 * @level: the level at which we are iterating. The @roi will indicate the
 *   extent at 1:1. `x`, `y`, `width` and `height` are `/ (2^level)`
 * @format: the format we want to process this buffers data in, pass 0 to use
 *   the buffers format.
 * @access_mode: whether we need reading or writing to this buffer one of
 *   [flags@Gegl.AccessMode.READ], [flags@Gegl.AccessMode.WRITE] and
 *   [flags@Gegl.AccessMode.READWRITE].
 * @abyss_policy: how requests outside the buffer extent are handled.
 * @max_slots: the maximum number of buffer iterators.
 *
 * Creates a new [struct@Gegl.BufferIterator]. The iterator can use one or more
 * sub-iterators to iterate over one or more [class@Gegl.Buffer].
 * [struct@Gegl.BufferIterator] takes care of accessing the given buffers in
 * the most optimal ways.
 *
 * See the documentation of [method.Gegl.BufferIterator.add] and
 * [method.Gegl.BufferIterator.next].
 *
 * Returns: a new [struct@Gegl.BufferIterator] with one sub-iterator.
 */
GeglBufferIterator * gegl_buffer_iterator_new  (GeglBuffer          *buffer,
                                                const GeglRectangle *roi,
                                                gint                 level,
                                                const Babl          *format,
                                                GeglAccessMode       access_mode,
                                                GeglAbyssPolicy      abyss_policy,
                                                gint                 max_slots);

/**
 * gegl_buffer_iterator_add: (skip)
 * @iterator: a [struct@Gegl.BufferIterator]
 * @buffer: a [class@Gegl.Buffer] to iterate over
 * @roi: the region in @buffer to iterate over
 * @level: the level at which we are iterating. The @roi will indicate the
 *   extent at 1:1. `x`, `y`, `width` and `height` are `/ (2^level)`
 * @format: the format we want to process this buffers data in, pass 0 to use
 *   the buffers format.
 * @access_mode: whether we need reading or writing to this buffer.
 * @abyss_policy: how requests outside the buffer extent are handled.
 *
 * Adds an additional buffer sub-iterator into @iterator that will be processed
 * in sync with the first one (unless this is the first sub-iterator).
 *
 * The iterator prioritizes accessing tile data of @buffer directly but if the
 * @buffer doesn't align with the first one for tile access the corresponding
 * scans and regions will be serialized automatically using
 * [method@Gegl.Buffer.get]. The @roi of all buffer sub-iterators added after
 * the first one will have their dimensions set to the first one.
 *
 * If @buffer shares its tiles with a previously-added [class@Gegl.Buffer] (in
 * particular, if the same [class@Gegl.Buffer] is added more than once), and at
 * least one of the buffers is accessed for writing, the corresponding
 * iterated-over areas should either completely overlap, or not overlap at all,
 * in the coordinate system of the underlying tile storage (that is, after
 * shifting each area by the corresponding buffer's
 * [property@Gegl.Buffer:shift-x] and [property@Gegl.Buffer:shift-y]
 * properties). If the areas overlap, at most one of the buffers may be
 * accessed for writing, and the data pointers of the corresponding iterator
 * items may refer to the same data.
 *
 * Returns: an integer handle refering to the indice in the iterator structure
 * of the added sub-iterator.
 */
gint                 gegl_buffer_iterator_add  (GeglBufferIterator  *iterator,
                                                GeglBuffer          *buffer,
                                                const GeglRectangle *roi,
                                                gint                 level,
                                                const Babl          *format,
                                                GeglAccessMode       access_mode,
                                                GeglAbyssPolicy      abyss_policy);

/**
 * gegl_buffer_iterator_stop: (skip)
 * @iterator: a #GeglBufferIterator
 *
 * Cancels the current iteration and invalidates the [struct@Gegl.BufferIterator].
 */
void                 gegl_buffer_iterator_stop  (GeglBufferIterator *iterator);

/**
 * gegl_buffer_iterator_next: (skip)
 * @iterator: a #GeglBufferIterator
 *
 * Do an iteration. If there is more data to process, a new set of linear
 * chunks of memory becomes available in all @items via
 * [struct@Gegl.BufferIteratorItem].
 *
 * Changed data from a previous iteration step will be saved now. When there
 * is no more data to be processed FALSE will be returned (and the
 * [struct@Gegl.BufferIterator] becomes invalid).
 *
 * Returns: TRUE if there is more work FALSE if iteration is complete.
 */
gboolean             gegl_buffer_iterator_next (GeglBufferIterator *iterator);

G_END_DECLS

#endif
