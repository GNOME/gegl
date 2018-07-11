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

#include "config.h"

#include <glib-object.h>

#include "gegl-types-internal.h"

#include "gegl.h"
#include "gegl-connection.h"
#include "gegl-node-output-visitable.h"
#include "gegl-node-private.h"
#include "gegl-pad.h"
#include "gegl-visitable.h"
#include "gegl-visitor.h"


static void       gegl_node_output_visitable_class_init           (GeglNodeOutputVisitableClass *klass);
static void       gegl_node_output_visitable_init                 (GeglNodeOutputVisitable      *self);
static void       gegl_node_output_visitable_visitable_iface_init (gpointer                      ginterface,
                                                                   gpointer                      interface_data);
static gboolean   gegl_node_output_visitable_visitable_accept     (GeglVisitable                *visitable,
                                                                   GeglVisitor                  *visitor);
static GSList   * gegl_node_output_visitable_visitable_depends_on (GeglVisitable                *visitable);


G_DEFINE_TYPE_WITH_CODE (GeglNodeOutputVisitable, gegl_node_output_visitable, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GEGL_TYPE_VISITABLE,
                                                gegl_node_output_visitable_visitable_iface_init))


static void
gegl_node_output_visitable_class_init (GeglNodeOutputVisitableClass *klass)
{
}

static void
gegl_node_output_visitable_init (GeglNodeOutputVisitable *self)
{
  self->node = NULL;
}

static void
gegl_node_output_visitable_visitable_iface_init (gpointer ginterface,
                                                 gpointer interface_data)
{
  GeglVisitableClass *visitable_class = ginterface;

  visitable_class->accept     = gegl_node_output_visitable_visitable_accept;
  visitable_class->depends_on = gegl_node_output_visitable_visitable_depends_on;
}

static gboolean
gegl_node_output_visitable_visitable_accept (GeglVisitable *visitable,
                                             GeglVisitor   *visitor)
{
  GeglNodeOutputVisitable *self = GEGL_NODE_OUTPUT_VISITABLE (visitable);

  return gegl_visitor_visit_node (visitor, self->node);
}

static GSList *
gegl_node_output_visitable_visitable_depends_on (GeglVisitable *visitable)
{
  GeglNodeOutputVisitable *self         = GEGL_NODE_OUTPUT_VISITABLE (visitable);
  GSList                  *dependencies = NULL;
  GSList                  *iter;

  for (iter = gegl_node_get_sinks (self->node); iter; iter = g_slist_next (iter))
    {
      GeglConnection *connection = iter->data;
      GeglNode       *sink_node;
      GeglVisitable  *sink_visitable;

      sink_node      = gegl_connection_get_sink_node (connection);
      sink_visitable = gegl_node_get_output_visitable (sink_node);

      dependencies = g_slist_prepend (dependencies, sink_visitable);
    }

  return dependencies;
}

GeglVisitable *
gegl_node_output_visitable_new (GeglNode *node)
{
  GeglNodeOutputVisitable *self;

  g_return_val_if_fail (GEGL_IS_NODE (node), NULL);

  self = g_object_new (GEGL_TYPE_NODE_OUTPUT_VISITABLE, NULL);

  self->node = node;

  return GEGL_VISITABLE (self);
}
