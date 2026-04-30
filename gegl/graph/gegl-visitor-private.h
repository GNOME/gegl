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
 * Copyright 2003 Calvin Williamson
 *           2017 Ell
 */

#ifndef __GEGL_VISITOR_H__
#define __GEGL_VISITOR_H__

G_BEGIN_DECLS


#define GEGL_TYPE_VISITOR               (gegl_visitor_get_type ())
#define GEGL_VISITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_VISITOR, GeglVisitor))
#define GEGL_VISITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_VISITOR, GeglVisitorClass))
#define GEGL_IS_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_VISITOR))
#define GEGL_IS_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_VISITOR))
#define GEGL_VISITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_VISITOR, GeglVisitorClass))


typedef struct _GeglVisitorClass GeglVisitorClass;

struct _GeglVisitor
{
  GObject  parent_instance;
};

struct _GeglVisitorClass
{
  GObjectClass  parent_class;

  /* these functions return TRUE to stop traversal, FALSE to continue */
  gboolean (* visit_pad)  (GeglVisitor  *self,
                           GeglPad      *pad);
  gboolean (* visit_node) (GeglVisitor  *self,
                           GeglNode     *node);
};


GType      gegl_visitor_get_type                     (void) G_GNUC_CONST;

gboolean   gegl_visitor_visit_pad                    (GeglVisitor   *self,
                                                      GeglPad       *pad);
gboolean   gegl_visitor_visit_node                   (GeglVisitor   *self,
                                                      GeglNode      *node);

gboolean   gegl_visitor_traverse                     (GeglVisitor   *self,
                                                      GeglVisitable *visitable);
gboolean   gegl_visitor_traverse_topological         (GeglVisitor   *self,
                                                      GeglVisitable *visitable);
gboolean   gegl_visitor_traverse_reverse_topological (GeglVisitor   *self,
                                                      GeglVisitable *visitable);


G_END_DECLS

#endif /* __GEGL_VISITOR_H__ */
