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
 */

#ifndef __GEGL_BUFFER_SWAP_H__
#define __GEGL_BUFFER_SWAP_H__


#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

/**
 * gegl_buffer_swap_create_file:
 * @suffix: (nullable): a string to suffix the filename with, for
 *          identification purposes, or %NULL.
 *
 * Generates a unique filename in the GEGL swap directory, suitable for
 * using as swap space.  When the file is no longer needed, it may be
 * removed with gegl_buffer_swap_remove_file(); otherwise, it will be
 * removed when gegl_exit() is called.
 *
 * Returns: (type filename) (nullable):  a string containing the full
 * file path, or %NULL is the swap is disabled.  The returned string
 * should be freed with g_free() when no longer needed.
 */
gchar    * gegl_buffer_swap_create_file (const gchar *suffix);

/**
 * gegl_buffer_swap_remove_file:
 * @path: (type filename): the swap file to remove, as returned by
 *        gegl_buffer_swap_create_file()
 *
 * Removes a swap file, generated using gegl_buffer_swap_create_file(),
 * unlinking the file, if exists.
 */
void       gegl_buffer_swap_remove_file (const gchar *path);

/**
 * gegl_buffer_swap_has_file:
 * @path: (type filename): a filename
 *
 * Tests if @path is a swap file, that is, if it has been created
 * with gegl_buffer_swap_create_file(), and hasn't been removed
 * yet.
 */
gboolean   gegl_buffer_swap_has_file (const gchar *path);

G_END_DECLS

#endif
