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

#include "config.h"

#include <glib-object.h>

#include "gegl-types-internal.h"

#include "gegl.h"
#include "graph/gegl-node-private.h"
#include "graph/gegl-pad.h"
#include "gegl-visitor.h"
#include "gegl-visitable.h"


static void       gegl_visitor_class_init                        (GeglVisitorClass  *klass);
static void       gegl_visitor_init                              (GeglVisitor       *self);
static gboolean   gegl_visitor_traverse_step                     (GeglVisitor       *self,
                                                                  GeglVisitable     *visitable,
                                                                  GHashTable        *visited_set);
static gboolean   gegl_visitor_traverse_topological_step         (GeglVisitor       *self,
                                                                  GeglVisitable     *visitable,
                                                                  GHashTable        *visited_set);
static void       gegl_visitor_traverse_reverse_topological_step (GeglVisitor       *self,
                                                                  GeglVisitable     *visitable,
                                                                  GHashTable        *visited_set,
                                                                  GSList           **stack);


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
 * gegl_visitor_traverse:
 * @self: #GeglVisitor
 * @visitable: the start #GeglVisitable
 *
 * Traverse in arbitrary order, starting at @visitable.
 * Use this function when you don't need a specific
 * traversal order, since it can be more efficient.
 *
 * Returns: %TRUE if traversal was terminated early.
 **/
gboolean
gegl_visitor_traverse (GeglVisitor   *self,
                       GeglVisitable *visitable)
{
  GHashTable *visited_set;
  gboolean    result;

  g_return_val_if_fail (GEGL_IS_VISITOR (self), FALSE);
  g_return_val_if_fail (GEGL_IS_VISITABLE (visitable), FALSE);

  visited_set = g_hash_table_new (NULL, NULL);

  result = gegl_visitor_traverse_step (self, visitable,
                                       visited_set);

  g_hash_table_unref (visited_set);

  return result;
}

static gboolean
gegl_visitor_traverse_step (GeglVisitor   *self,
                            GeglVisitable *visitable,
                            GHashTable    *visited_set)
{
  GSList *dependencies;
  GSList *iter;

  if (gegl_visitable_accept (visitable, self))
    return TRUE;

  dependencies = gegl_visitable_depends_on (visitable);

  for (iter = dependencies; iter; iter = g_slist_next (iter))
    {
      GeglVisitable *dependency = iter->data;

      if (! g_hash_table_contains (visited_set, dependency))
        {
          if (gegl_visitor_traverse_step (self, dependency,
                                          visited_set))
            {
              g_slist_free (dependencies);

              return TRUE;
            }
        }
    }

  g_slist_free (dependencies);

  g_hash_table_add (visited_set, visitable);

  return FALSE;
}

/**
 * gegl_visitor_traverse_topological:
 * @self: #GeglVisitor
 * @visitable: the start #GeglVisitable
 *
 * Traverse in topological order (dependencies first),
 * starting at @visitable.
 *
 * Returns: %TRUE if traversal was terminated early.
 **/
gboolean
gegl_visitor_traverse_topological (GeglVisitor   *self,
                                   GeglVisitable *visitable)
{
  GHashTable *visited_set;
  gboolean    result;

  g_return_val_if_fail (GEGL_IS_VISITOR (self), FALSE);
  g_return_val_if_fail (GEGL_IS_VISITABLE (visitable), FALSE);

  visited_set = g_hash_table_new (NULL, NULL);

  result = gegl_visitor_traverse_topological_step (self, visitable,
                                                   visited_set);

  g_hash_table_unref (visited_set);

  return result;
}

static gboolean
gegl_visitor_traverse_topological_step (GeglVisitor   *self,
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
          if (gegl_visitor_traverse_topological_step (self, dependency,
                                                      visited_set))
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
 * gegl_visitor_traverse_reverse_topological:
 * @self: #GeglVisitor
 * @visitable: the start #GeglVisitable
 *
 * Traverse in reverse-topological order (dependencies
 * last), starting at @visitable.
 *
 * Returns: %TRUE if traversal was terminated early.
 **/
gboolean
gegl_visitor_traverse_reverse_topological (GeglVisitor   *self,
                                           GeglVisitable *visitable)
{
  GHashTable *visited_set;
  GSList     *stack = NULL;

  g_return_val_if_fail (GEGL_IS_VISITOR (self), FALSE);
  g_return_val_if_fail (GEGL_IS_VISITABLE (visitable), FALSE);

  visited_set = g_hash_table_new (NULL, NULL);

  gegl_visitor_traverse_reverse_topological_step (self, visitable,
                                                  visited_set, &stack);

  g_hash_table_unref (visited_set);

  while (stack)
    {
      visitable = stack->data;

      stack = g_slist_delete_link (stack, stack);

      if (gegl_visitable_accept (visitable, self))
        {
          g_slist_free (stack);

          return TRUE;
        }
    }

  return FALSE;
}

static void
gegl_visitor_traverse_reverse_topological_step (GeglVisitor    *self,
                                                GeglVisitable  *visitable,
                                                GHashTable     *visited_set,
                                                GSList        **stack)
{
  GSList *dependencies;
  GSList *iter;

  dependencies = gegl_visitable_depends_on (visitable);

  for (iter = dependencies; iter; iter = g_slist_next (iter))
    {
      GeglVisitable *dependency = iter->data;

      if (! g_hash_table_contains (visited_set, dependency))
        {
          gegl_visitor_traverse_reverse_topological_step (self, dependency,
                                                          visited_set, stack);
        }
    }

  g_slist_free (dependencies);

  *stack = g_slist_prepend (*stack, visitable);
  g_hash_table_add (visited_set, visitable);
}
