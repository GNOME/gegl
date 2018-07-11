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

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-callback-visitor.h"


static void       gegl_callback_visitor_class_init (GeglCallbackVisitorClass *klass);
static void       gegl_callback_visitor_init       (GeglCallbackVisitor      *self);
static gboolean   gegl_callback_visitor_visit_node (GeglVisitor              *visitor,
                                                    GeglNode                 *node);


G_DEFINE_TYPE (GeglCallbackVisitor, gegl_callback_visitor, GEGL_TYPE_VISITOR)


static void
gegl_callback_visitor_class_init (GeglCallbackVisitorClass *klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);

  visitor_class->visit_node = gegl_callback_visitor_visit_node;
}

static void
gegl_callback_visitor_init (GeglCallbackVisitor *self)
{
}

static gboolean
gegl_callback_visitor_visit_node (GeglVisitor *visitor,
                                  GeglNode    *node)
{
  GeglCallbackVisitor *self = GEGL_CALLBACK_VISITOR (visitor);

  return self->callback (node, self->data);
}

GeglVisitor *
gegl_callback_visitor_new (GeglCallbackVisitorCallback callback,
                           gpointer                    data)
{
  GeglCallbackVisitor *self;

  g_return_val_if_fail (callback != NULL, NULL);

  self = g_object_new (GEGL_TYPE_CALLBACK_VISITOR, NULL);

  self->callback = callback;
  self->data     = data;

  return GEGL_VISITOR (self);
}
