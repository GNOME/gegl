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
 * Copyright 2006 Øyvind Kolås
 */

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl.h"
#include "gegl-operation-meta.h"

static GeglNode * detect       (GeglOperation *operation,
                                gint           x,
                                gint           y);

G_DEFINE_TYPE (GeglOperationMeta, gegl_operation_meta, GEGL_TYPE_OPERATION)


static void
gegl_operation_meta_class_init (GeglOperationMetaClass *klass)
{
  GEGL_OPERATION_CLASS (klass)->detect = detect;
}

static void
gegl_operation_meta_init (GeglOperationMeta *self)
{
}

static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  return NULL; /* hands it over request to the internal nodes */
}

void
gegl_operation_meta_redirect (GeglOperation *operation,
                              const gchar   *name,
                              GeglNode      *internal,
                              const gchar   *internal_name)
{
  GeglOperation *internal_operation;

  internal_operation = gegl_node_get_gegl_operation (internal);
  g_object_bind_property (operation, name, internal_operation, internal_name, G_BINDING_SYNC_CREATE);
}

typedef struct
{
  GeglNode *parent;
  GeglNode *child;
} GeglOperationMetaNodeCleanupContext;

static void
_gegl_operation_meta_node_cleanup (gpointer data,
                                   GObject *where_the_object_was)
{
  GeglOperationMetaNodeCleanupContext *ctx = data;

  if (ctx->child)
    {
      g_object_remove_weak_pointer (G_OBJECT (ctx->child), (void**)&ctx->child);
      gegl_node_remove_child (ctx->parent, ctx->child);
    }

  g_free (data);
}

void
gegl_operation_meta_watch_node (GeglOperation     *operation,
                                GeglNode          *node)
{
  GeglOperationMetaNodeCleanupContext *ctx;
  ctx = g_new0 (GeglOperationMetaNodeCleanupContext, 1);

  ctx->parent = operation->node;
  ctx->child  = node;

  g_object_add_weak_pointer (G_OBJECT (ctx->child), (void**)&ctx->child);
  g_object_weak_ref (G_OBJECT (operation), _gegl_operation_meta_node_cleanup, ctx);
}

void
gegl_operation_meta_watch_nodes (GeglOperation     *operation,
                                 GeglNode          *node,
                                 ...)
{
  va_list var_args;

  va_start (var_args, node);
  while (node)
    {
      gegl_operation_meta_watch_node (operation, node);
      node = va_arg (var_args, GeglNode *);
    }
  va_end (var_args);
}

void
gegl_operation_meta_property_changed (GeglOperationMeta *self,
                                      GParamSpec        *pspec,
                                      gpointer           user_data)
{
  gchar *detailed_signal = NULL;

  g_return_if_fail (GEGL_IS_OPERATION_META (self));
  g_return_if_fail (pspec);

  detailed_signal = g_strconcat ("notify::", pspec->name, NULL);
  g_signal_emit_by_name (self, detailed_signal, pspec);

  g_free (detailed_signal);
}
