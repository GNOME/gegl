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

#ifndef __GEGL_MEMORY_H__
#define __GEGL_MEMORY_H__

G_BEGIN_DECLS

/***
 * Aligned memory:
 *
 * GEGL provides functions to allocate and free buffers that are guaranteed to
 * be on 16 byte aligned memory addresses.
 */

/**
 * gegl_malloc: (skip) (attributes skip-reason=unknown_since_9e8f617e)
 * @n_bytes: the number of bytes to allocte.
 *
 * Allocates @n_bytes of memory. If @n_bytes is 0, returns NULL.
 *
 * Returns a pointer to the allocated memory.
 */
gpointer   gegl_malloc         (gsize         n_bytes) G_GNUC_MALLOC;

/**
 * gegl_try_malloc: (skip) (attributes skip-reason=unknown_since_2c3f2475)
 * @n_bytes: the number of bytes to allocte.
 *
 * Allocates @n_bytes of memory. If allocation fails, or if @n_bytes is 0,
 * returns %NULL.
 *
 * Returns a pointer to the allocated memory, or NULL.
 */
gpointer   gegl_try_malloc     (gsize         n_bytes) G_GNUC_MALLOC;

/**
 * gegl_free: (skip) (attributes skip-reason=unknown_since_b4913381)
 * @mem: the memory to free.
 *
 * Frees the memory pointed to by @mem. If @mem is NULL, does nothing.
 */
void       gegl_free           (gpointer      mem);

/**
 * gegl_calloc: (skip) (attributes skip-reason=unknown_since_b4913381)
 * @size: size of items to allocate
 * @n_memb: number of members
 *
 * allocated 0'd memory.
 */
gpointer   gegl_calloc         (gsize         size,
                                gint          n_memb) G_GNUC_MALLOC;

/**
 * gegl_memeq_zero: (skip) (attributes skip-reason=unknown_since_b0c68ce4)
 * @ptr: pointer to the memory block
 * @size: block size
 *
 * Checks if all the bytes of the memory block @ptr, of size @size,
 * are equal to zero.
 *
 * Returns: TRUE if all the bytes are equal to zero.
 */
gboolean   gegl_memeq_zero     (gconstpointer ptr,
                                gsize         size);

/**
 * gegl_memset_pattern: (skip) (attributes skip-reason=unknown_since_bf597a18)
 * @dst_ptr: pointer to copy to
 * @src_ptr: pointer to copy from
 * @pattern_size: the length of @src_ptr
 * @count: number of copies
 *
 * Fill @dst_ptr with @count copies of the bytes in @src_ptr.
 */
void       gegl_memset_pattern (gpointer      dst_ptr,
                                gconstpointer src_ptr,
                                gint          pattern_size,
                                gint          count);

G_END_DECLS

#endif  /* __GEGL_MEMORY_H__ */
