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
 * Copyright 2017 Ell
 */

#ifndef __GEGL_NODE_OUTPUT_VISITABLE_H__
#define __GEGL_NODE_OUTPUT_VISITABLE_H__

G_BEGIN_DECLS


#define GEGL_TYPE_NODE_OUTPUT_VISITABLE            (gegl_node_output_visitable_get_type ())
#define GEGL_NODE_OUTPUT_VISITABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_NODE_OUTPUT_VISITABLE, GeglNodeOutputVisitable))
#define GEGL_NODE_OUTPUT_VISITABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_NODE_OUTPUT_VISITABLE, GeglNodeOutputVisitableClass))
#define GEGL_IS_NODE_OUTPUT_VISITABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_NODE_OUTPUT_VISITABLE))
#define GEGL_IS_NODE_OUTPUT_VISITABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_NODE_OUTPUT_VISITABLE))
#define GEGL_NODE_OUTPUT_VISITABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_NODE_OUTPUT_VISITABLE, GeglNodeOutputVisitableClass))


typedef struct _GeglNodeOutputVisitable      GeglNodeOutputVisitable;
typedef struct _GeglNodeOutputVisitableClass GeglNodeOutputVisitableClass;

struct _GeglNodeOutputVisitable
{
  GObject   parent_instance;

  GeglNode *node;
};

struct _GeglNodeOutputVisitableClass
{
  GObjectClass  parent_class;
};


GType           gegl_node_output_visitable_get_type (void) G_GNUC_CONST;

GeglVisitable * gegl_node_output_visitable_new      (GeglNode *node);


G_END_DECLS

#endif /* __GEGL_NODE_OUTPUT_VISITABLE_H__ */
