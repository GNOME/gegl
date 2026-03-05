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
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2013 Daniel Sabo
 */

#ifndef __GEGL_GRAPH_TRAVERSAL_PRIVATE_H__
#define __GEGL_GRAPH_TRAVERSAL_PRIVATE_H__

G_BEGIN_DECLS

struct _GeglGraphTraversal
{
  GHashTable *contexts;
  GQueue      path;
  gboolean    rects_dirty;
  GeglBuffer *shared_empty;
};

G_END_DECLS

#endif /* __GEGL_GRAPH_TRAVERSAL_PRIVATE_H__ */
