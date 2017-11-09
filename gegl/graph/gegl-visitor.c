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
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2003 Calvin Williamson
 *           2017 Ell
 */

#include "config.h"

#include <glib-object.h>

#include "gegl-types-internal.h"

#include "gegl.h"
#include "graph/gegl-node-private.h"
#include "graph/gegl-pad.h"
#include "gegl-visitor.h"
#include "gegl-visitable.h"


static void       gegl_visitor_class_init        (GeglVisitorClass *klass);
static void       gegl_visitor_init              (GeglVisitor      *self);
static gboolean   gegl_visitor_dfs_traverse_step (GeglVisitor      *self,
                                                  GeglVisitable    *visitable,
                                                  GHashTable       *visited_set);
static void       gegl_visitor_bfs_init_step     (GeglVisitor      *self,
                                                  GeglVisitable    *visitable,
                                                  GHashTable       *edge_counts);


G_DEFINE_TYPE (GeglVisitor, gegl_visitor, G_TYPE_OBJECT)


static void
gegl_visitor_class_init (GeglVisitorClass *klass)
{
  klass->visit_pad  = NULL;
  klass->visit_node = NULL;
}

static void
gegl_visitor_init (GeglVisitor *self)
{
}

/* should be called by visitables, to visit a pad */
gboolean
gegl_visitor_visit_pad (GeglVisitor *self,
                        GeglPad     *pad)
{
  GeglVisitorClass *klass;

  g_return_val_if_fail (GEGL_IS_VISITOR (self), FALSE);
  g_return_val_if_fail (GEGL_IS_PAD (pad), FALSE);

  klass = GEGL_VISITOR_GET_CLASS (self);

  if (klass->visit_pad)
    return klass->visit_pad (self, pad);
  else
    return FALSE;
}

/* should be called by visitables, to visit a node */
gboolean
gegl_visitor_visit_node (GeglVisitor *self,
                         GeglNode    *node)
{
  GeglVisitorClass *klass;

  g_return_val_if_fail (GEGL_IS_VISITOR (self), FALSE);
  g_return_val_if_fail (GEGL_IS_NODE (node), FALSE);

  klass = GEGL_VISITOR_GET_CLASS (self);

  if (klass->visit_node)
    return klass->visit_node (self, node);
  else
    return FALSE;
}

/**
 * gegl_visitor_dfs_traverse:
 * @self: #GeglVisitor
 * @visitable: the start #GeglVisitable
 *
 * Traverse depth first starting at @visitable.
 *
 * Returns: %TRUE if traversal was terminated early.
 **/
gboolean
gegl_visitor_dfs_traverse (GeglVisitor   *self,
                           GeglVisitable *visitable)
{
  GHashTable *visited_set;
  gboolean    result;

  g_return_val_if_fail (GEGL_IS_VISITOR (self), FALSE);
  g_return_val_if_fail (GEGL_IS_VISITABLE (visitable), FALSE);

  visited_set = g_hash_table_new (NULL, NULL);

  result = gegl_visitor_dfs_traverse_step (self, visitable, visited_set);

  g_hash_table_unref (visited_set);

  return result;
}

static gboolean
gegl_visitor_dfs_traverse_step (GeglVisitor   *self,
                                GeglVisitable *visitable,
                                GHashTable    *visited_set)
{
  GSList *dependencies;
  GSList *iter;

  dependencies = gegl_visitable_depends_on (visitable);

  for (iter = dependencies; iter; iter = g_slist_next (iter))
    {
      GeglVisitable *dependency = iter->data;

      if (! g_hash_table_contains (visited_set, dependency))
        {
          if (gegl_visitor_dfs_traverse_step (self, dependency, visited_set))
            {
              g_slist_free (dependencies);

              return TRUE;
            }
        }
    }

  g_slist_free (dependencies);

  if (gegl_visitable_accept (visitable, self))
    return TRUE;

  g_hash_table_add (visited_set, visitable);

  return FALSE;
}

/**
 * gegl_visitor_bfs_traverse:
 * @self: a #GeglVisitor
 * @visitable: the root #GeglVisitable.
 *
 * Traverse breadth-first starting at @visitable.
 *
 * Returns: %TRUE if traversal was terminated early.
 **/
gboolean
gegl_visitor_bfs_traverse (GeglVisitor   *self,
                           GeglVisitable *visitable)
{
  GHashTable *edge_counts;
  GQueue      queue = G_QUEUE_INIT;

  g_return_val_if_fail (GEGL_IS_VISITOR (self), FALSE);
  g_return_val_if_fail (GEGL_IS_VISITABLE (visitable), FALSE);

  edge_counts = g_hash_table_new (NULL, NULL);

  gegl_visitor_bfs_init_step (self, visitable, edge_counts);

  g_queue_push_tail (&queue, visitable);

  while ((visitable = g_queue_pop_head (&queue)))
    {
      GSList *dependencies;
      GSList *iter;

      if (gegl_visitable_accept (visitable, self))
        {
          g_queue_clear (&queue);
          g_hash_table_unref (edge_counts);

          return TRUE;
        }

      dependencies = gegl_visitable_depends_on (visitable);

      for (iter = dependencies; iter; iter = g_slist_next (iter))
        {
          GeglVisitable *dependency = iter->data;
          gint           edges;

          edges = GPOINTER_TO_INT (g_hash_table_lookup (edge_counts, dependency));

          if (edges == 1)
            {
              g_queue_push_tail (&queue, dependency);
            }
          else
            {
              g_hash_table_insert (edge_counts,
                                   dependency, GINT_TO_POINTER (edges - 1));
            }
        }

      g_slist_free (dependencies);
    }

  g_hash_table_unref (edge_counts);

  return FALSE;
}

static void
gegl_visitor_bfs_init_step (GeglVisitor   *self,
                            GeglVisitable *visitable,
                            GHashTable    *edge_counts)
{
  GSList *dependencies;
  GSList *iter;

  dependencies = gegl_visitable_depends_on (visitable);

  for (iter = dependencies; iter; iter = g_slist_next (iter))
    {
      GeglVisitable *dependency = iter->data;
      gint           edges;

      edges = GPOINTER_TO_INT (g_hash_table_lookup (edge_counts, dependency));
      g_hash_table_insert (edge_counts,
                           dependency, GINT_TO_POINTER (edges + 1));

      if (edges == 0)
        gegl_visitor_bfs_init_step (self, dependency, edge_counts);
    }

  g_slist_free (dependencies);
}
