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
 * Copyright 2019 Ell
 */

#ifndef __GEGL_TILE_ALLOC_H__
#define __GEGL_TILE_ALLOC_H__


void       gegl_tile_alloc_init      (void);
void       gegl_tile_alloc_cleanup   (void);

/* the buffer returned by gegl_tile_alloc() and gegl_tile_alloc0() is
 * guaranteed to have room for two `int`s in front of the buffer.
 */

gpointer   gegl_tile_alloc           (gsize    size) G_GNUC_MALLOC;
gpointer   gegl_tile_alloc0          (gsize    size) G_GNUC_MALLOC;
void       gegl_tile_free            (gpointer ptr);

guint64    gegl_tile_alloc_get_total (void);


#endif /* __GEGL_TILE_ALLOC_H__ */
