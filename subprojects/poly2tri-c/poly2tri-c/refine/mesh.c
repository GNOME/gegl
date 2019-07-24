/*
 * This file is a part of Poly2Tri-C
 * (c) Barak Itkin <lightningismyname@gmail.com>
 * http://code.google.com/p/poly2tri-c/
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of Poly2Tri nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without specific
 *   prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <glib.h>
#include "utils.h"

#include "mesh.h"
#include "point.h"
#include "edge.h"
#include "triangle.h"
#include "mesh-action.h"

P2trMesh*
p2tr_mesh_new (void)
{
  P2trMesh *mesh = g_slice_new (P2trMesh);

  mesh->refcount = 1;
  mesh->edges = p2tr_hash_set_new_default ();
  mesh->points = p2tr_hash_set_new_default ();
  mesh->triangles = p2tr_hash_set_new_default ();

  mesh->record_undo = FALSE;
  g_queue_init (&mesh->undo);

  return mesh;
}

P2trPoint*
p2tr_mesh_add_point (P2trMesh  *self,
                     P2trPoint *point)
{
  g_assert (point->mesh == NULL);
  point->mesh = self;
  p2tr_mesh_ref (self);
  p2tr_hash_set_insert (self->points, point);

  if (self->record_undo)
    g_queue_push_tail (&self->undo, p2tr_mesh_action_new_point (point));

  return p2tr_point_ref (point);
}

P2trPoint*
p2tr_mesh_new_point (P2trMesh          *self,
                     const P2trVector2 *c)
{
  return p2tr_mesh_new_point2 (self, c->x, c->y);
}

P2trPoint*
p2tr_mesh_new_point2 (P2trMesh *self,
                      gdouble   x,
                      gdouble   y)
{
  return p2tr_mesh_add_point (self, p2tr_point_new2 (x, y));
}

P2trEdge*
p2tr_mesh_add_edge (P2trMesh *self,
                    P2trEdge *edge)
{
  p2tr_hash_set_insert (self->edges, p2tr_edge_ref (edge->mirror));
  p2tr_hash_set_insert (self->edges, p2tr_edge_ref (edge));

  if (self->record_undo)
    g_queue_push_tail (&self->undo, p2tr_mesh_action_new_edge (edge));

  return edge;
}

P2trEdge*
p2tr_mesh_new_edge (P2trMesh  *self,
                    P2trPoint *start,
                    P2trPoint *end,
                    gboolean   constrained)
{
  return p2tr_mesh_add_edge (self, p2tr_edge_new (start, end, constrained));
}

P2trEdge*
p2tr_mesh_new_or_existing_edge (P2trMesh  *self,
                                P2trPoint *start,
                                P2trPoint *end,
                                gboolean   constrained)
{
  P2trEdge *result = p2tr_point_has_edge_to (start, end);
  if (result)
    p2tr_edge_ref (result);
  else
    result = p2tr_mesh_new_edge (self, start, end, constrained);
  return result;
}

P2trTriangle*
p2tr_mesh_add_triangle (P2trMesh     *self,
                        P2trTriangle *tri)
{
  p2tr_hash_set_insert (self->triangles, tri);

  if (self->record_undo)
    g_queue_push_tail (&self->undo, p2tr_mesh_action_new_triangle (tri));

  return p2tr_triangle_ref (tri);
}

P2trTriangle*
p2tr_mesh_new_triangle (P2trMesh *self,
                        P2trEdge *AB,
                        P2trEdge *BC,
                        P2trEdge *CA)
{
  return p2tr_mesh_add_triangle (self, p2tr_triangle_new (AB, BC, CA));
}

void
p2tr_mesh_on_point_removed (P2trMesh  *self,
                            P2trPoint *point)
{
  if (self != point->mesh)
    p2tr_exception_programmatic ("Point does not belong to this mesh!");

  point->mesh = NULL;
  p2tr_mesh_unref (self);

  p2tr_hash_set_remove (self->points, point);

  if (self->record_undo)
    g_queue_push_tail (&self->undo, p2tr_mesh_action_del_point (point));

  p2tr_point_unref (point);
}

void
p2tr_mesh_on_edge_removed (P2trMesh *self,
                           P2trEdge *edge)
{
  p2tr_hash_set_remove (self->edges, edge->mirror);
  p2tr_edge_unref (edge->mirror);
  p2tr_hash_set_remove (self->edges, edge);

  if (self->record_undo)
    g_queue_push_tail (&self->undo, p2tr_mesh_action_del_edge (edge));

  p2tr_edge_unref (edge);
}

void
p2tr_mesh_on_triangle_removed (P2trMesh     *self,
                               P2trTriangle *triangle)
{
  p2tr_hash_set_remove (self->triangles, triangle);

  if (self->record_undo)
    g_queue_push_tail (&self->undo, p2tr_mesh_action_del_triangle (triangle));

  p2tr_triangle_unref (triangle);
}

void
p2tr_mesh_action_group_begin (P2trMesh *self)
{
  g_assert (! self->record_undo);
  self->record_undo = TRUE;
}

void
p2tr_mesh_action_group_commit (P2trMesh *self)
{
  GList *iter;

  g_assert (self->record_undo);

  self->record_undo = FALSE;

  for (iter = self->undo.head; iter != NULL; iter = iter->next)
    p2tr_mesh_action_unref ((P2trMeshAction*)iter->data);
  g_queue_clear (&self->undo);
}

void
p2tr_mesh_action_group_undo (P2trMesh *self)
{
  GList *iter;

  g_assert (self->record_undo);

  /* Set the record_undo flag to FALSE before p2tr_mesh_action_undo, so that
   * we don't create zombie objects therein. */
  self->record_undo = FALSE;

  for (iter = self->undo.tail; iter != NULL; iter = iter->prev)
    {
      p2tr_mesh_action_undo ((P2trMeshAction*)iter->data, self);
      p2tr_mesh_action_unref ((P2trMeshAction*)iter->data);
    }
  g_queue_clear (&self->undo);
}

void
p2tr_mesh_clear (P2trMesh *self)
{
  P2trHashSetIter iter;
  gpointer temp;

  /* While iterating over the sets of points/edges/triangles to remove
   * all the mesh elements, the sets will be modified by the removal
   * operation itself. Therefore we can't use a regular iterator -
   * instead we must look always at the first place */
  p2tr_hash_set_iter_init (&iter, self->triangles);
  while (p2tr_hash_set_iter_next (&iter, &temp))
    {
      p2tr_triangle_remove ((P2trTriangle*)temp);
      p2tr_hash_set_iter_init (&iter, self->triangles);
    }

  p2tr_hash_set_iter_init (&iter, self->edges);
  while (p2tr_hash_set_iter_next (&iter, &temp))
    {
      g_assert (((P2trEdge*)temp)->tri == NULL);
      p2tr_edge_remove ((P2trEdge*)temp);
      p2tr_hash_set_iter_init (&iter, self->edges);
    }

  p2tr_hash_set_iter_init (&iter, self->points);
  while (p2tr_hash_set_iter_next (&iter, &temp))
    {
      g_assert (((P2trPoint*)temp)->outgoing_edges == NULL);
      p2tr_point_remove ((P2trPoint*)temp);
      p2tr_hash_set_iter_init (&iter, self->points);
    }
}

void
p2tr_mesh_free (P2trMesh *self)
{
  if (self->record_undo)
    p2tr_mesh_action_group_commit (self);

  p2tr_mesh_clear (self);

  p2tr_hash_set_free (self->points);
  p2tr_hash_set_free (self->edges);
  p2tr_hash_set_free (self->triangles);

  g_slice_free (P2trMesh, self);
}

void
p2tr_mesh_unref (P2trMesh *self)
{
  g_assert (self->refcount > 0);
  if (--self->refcount == 0)
    p2tr_mesh_free (self);
}

P2trMesh*
p2tr_mesh_ref (P2trMesh *self)
{
  ++self->refcount;
  return self;
}

P2trTriangle*
p2tr_mesh_find_point (P2trMesh *self,
                      const P2trVector2 *pt)
{
  gdouble u, v;
  return p2tr_mesh_find_point2 (self, pt, &u, &v);
}

P2trTriangle*
p2tr_mesh_find_point2 (P2trMesh          *self,
                       const P2trVector2 *pt,
                       gdouble           *u,
                       gdouble           *v)
{
  P2trHashSetIter iter;
  P2trTriangle *result;
  
  p2tr_hash_set_iter_init (&iter, self->triangles);
  while (p2tr_hash_set_iter_next (&iter, (gpointer*)&result))
    if (p2tr_triangle_contains_point2 (result, pt, u, v) != P2TR_INTRIANGLE_OUT)
      return p2tr_triangle_ref (result);

  return NULL;
}

P2trTriangle*
p2tr_mesh_find_point_local (P2trMesh          *self,
                            const P2trVector2 *pt,
                            P2trTriangle      *initial_guess)
{
  gdouble u, v;
  return p2tr_mesh_find_point_local2 (self, pt, initial_guess, &u, &v);
}

P2trTriangle*
p2tr_mesh_find_point_local2 (P2trMesh          *self,
                             const P2trVector2 *pt,
                             P2trTriangle      *initial_guess,
                             gdouble           *u,
                             gdouble           *v)
{
  P2trHashSet *checked_tris;
  GQueue to_check;
  P2trTriangle *result = NULL;

  if (initial_guess == NULL)
    return p2tr_mesh_find_point2(self, pt, u, v);

  checked_tris = p2tr_hash_set_new_default ();
  g_queue_init (&to_check);
  g_queue_push_head (&to_check, initial_guess);

  while (! g_queue_is_empty (&to_check))
    {
      P2trTriangle *tri = (P2trTriangle*) g_queue_pop_head (&to_check);
      
      p2tr_hash_set_insert (checked_tris, tri);
      if (p2tr_triangle_contains_point2 (tri, pt, u, v) != P2TR_INTRIANGLE_OUT)
        {
          result = tri;
          break;
        }
      else
        {
          gint i;
          for (i = 0; i < 3; i++)
            {
              P2trTriangle *new_to_check = tri->edges[i]->mirror->tri;
              if (new_to_check != NULL
                  && ! p2tr_hash_set_contains (checked_tris, new_to_check))
                {
                  p2tr_hash_set_insert (checked_tris, new_to_check);
                  g_queue_push_tail (&to_check, new_to_check);
                }
            }
        }
    }

  p2tr_hash_set_free (checked_tris);
  g_queue_clear (&to_check);

  if (result != NULL)
    p2tr_triangle_ref (result);

  return result;
}

void
p2tr_mesh_get_bounds (P2trMesh    *self,
                      gdouble     *min_x,
                      gdouble     *min_y,
                      gdouble     *max_x,
                      gdouble     *max_y)
{
  gdouble lmin_x = + G_MAXDOUBLE, lmin_y = + G_MAXDOUBLE;
  gdouble lmax_x = - G_MAXDOUBLE, lmax_y = - G_MAXDOUBLE;

  P2trHashSetIter iter;
  P2trPoint *pt;

  p2tr_hash_set_iter_init (&iter, self->points);
  while (p2tr_hash_set_iter_next (&iter, (gpointer*) &pt))
    {
      gdouble x = pt->c.x;
      gdouble y = pt->c.y;

      lmin_x = MIN (lmin_x, x);
      lmin_y = MIN (lmin_y, y);
      lmax_x = MAX (lmax_x, x);
      lmax_y = MAX (lmax_y, y);
    }
  *min_x = lmin_x;
  *min_y = lmin_y;
  *max_x = lmax_x;
  *max_y = lmax_y;
}
