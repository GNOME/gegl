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
 * Copyright 2006 Øyvind Kolås
 */

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl.h"
#include "gegl-operation-meta.h"
#include "gegl-operation-context-private.h"

static GeglNode * detect       (GeglOperation *operation,
                                gint           x,
                                gint           y);

G_DEFINE_TYPE (GeglOperationMeta, gegl_operation_meta, GEGL_TYPE_OPERATION)


static void
gegl_operation_meta_class_init (GeglOperationMetaClass *klass)
{
  GEGL_OPERATION_CLASS (klass)->detect = detect;
}
#include <stdio.h>
static void
gegl_operation_meta_property_notify (GeglOperation *self)
{
  GeglOperationMetaClass *klass = GEGL_OPERATION_META_GET_CLASS (self);
  if (_gegl_operation_is_attached (self) &&
      klass->update)
    klass->update (self);
}

static void
gegl_operation_meta_init (GeglOperationMeta *self)
{
  g_signal_connect (self, "notify",
                    G_CALLBACK (gegl_operation_meta_property_notify),
                    self);
}

static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  return NULL; /* hands request over to the internal nodes */
}

void
gegl_operation_meta_redirect (GeglOperation *operation,
                              const gchar   *name,
                              GeglNode      *internal,
                              const gchar   *internal_name)
{
  GeglOperation *internal_operation = gegl_node_get_gegl_operation (internal);
  g_object_bind_property (operation, name, internal_operation, internal_name, G_BINDING_SYNC_CREATE);
}

void
gegl_operation_meta_watch_node (GeglOperation     *operation,
                                GeglNode          *node)
{
  g_warning ("%s does nothing\n", __FUNCTION__);
}

void
gegl_operation_meta_watch_nodes (GeglOperation     *operation,
                                 GeglNode          *node,
                                 ...)
{
  g_warning ("%s does nothing\n", __FUNCTION__);
}
