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
 * Copyright 2003      Calvin Williamson
 * 2005-2009,2011-2014 Øyvind Kolås
 */

#include "config.h"

#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gegl.h"
#include "gegl-config.h"
#include "gegl-types-internal.h"
#include "gegl-operation.h"
#include "gegl-operation-private.h"
#include "gegl-operation-context.h"
#include "gegl-operations-util.h"
#include "graph/gegl-node-private.h"
#include "graph/gegl-connection.h"
#include "graph/gegl-pad.h"
#include "gegl-operations.h"

static void         attach                    (GeglOperation       *self);

static GeglRectangle get_bounding_box          (GeglOperation       *self);
static GeglRectangle get_invalidated_by_change (GeglOperation       *self,
                                                const gchar         *input_pad,
                                                const GeglRectangle *input_region);
static GeglRectangle get_required_for_output   (GeglOperation       *self,
                                                const gchar         *input_pad,
                                                const GeglRectangle *region);

static void gegl_operation_class_init     (GeglOperationClass *klass);
static void gegl_operation_base_init      (GeglOperationClass *klass);
static void gegl_operation_init           (GeglOperation      *self);

GType
gegl_operation_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GeglOperationClass),
        (GBaseInitFunc)      gegl_operation_base_init,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc)     gegl_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GeglOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gegl_operation_init,
      };

      type = g_type_register_static (G_TYPE_OBJECT,
                                     "GeglOperation",
                                     &info, 0);
    }
  return type;
}


static void
gegl_operation_class_init (GeglOperationClass *klass)
{
  klass->name                      = NULL;  /* an operation class with
                                             * name == NULL is not
                                             * included when doing
                                             * operation lookup by
                                             * name
                                             */
  klass->compat_name               = NULL;
  klass->attach                    = attach;
  klass->prepare                   = NULL;
  klass->no_cache                  = FALSE;
  klass->threaded                  = FALSE;
  klass->cache_policy              = GEGL_CACHE_POLICY_AUTO;
  klass->get_bounding_box          = get_bounding_box;
  klass->get_invalidated_by_change = get_invalidated_by_change;
  klass->get_required_for_output   = get_required_for_output;
  klass->cl_data                   = NULL;
}

static void
gegl_operation_base_init (GeglOperationClass *klass)
{
  /* XXX: leaked for now, should replace G_DEFINE_TYPE with the expanded one */
  klass->keys = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
}

static void
gegl_operation_init (GeglOperation *self)
{
}

/**
 * gegl_operation_create_pad:
 * @self: a #GeglOperation.
 * @param_spec:
 *
 * Create a property.
 **/
void
gegl_operation_create_pad (GeglOperation *self,
                           GParamSpec    *param_spec)
{
  GeglPad *pad;

  g_return_if_fail (GEGL_IS_OPERATION (self));
  g_return_if_fail (param_spec != NULL);

  if (!self->node)
    {
      g_warning ("%s: aborting, no associated node. "
                 "This method should only be called after the operation is "
                 "associated with a node.", G_STRFUNC);
      return;
    }

  pad = g_object_new (GEGL_TYPE_PAD, NULL);
  gegl_pad_set_param_spec (pad, param_spec);
  gegl_pad_set_node (pad, self->node);
  gegl_node_add_pad (self->node, pad);
}

gboolean
gegl_operation_process (GeglOperation        *operation,
                        GeglOperationContext *context,
                        const gchar          *output_pad,
                        const GeglRectangle  *result,
                        gint                  level)
{
  GeglOperationClass *klass;
  gboolean            success = TRUE;

  g_return_val_if_fail (GEGL_IS_OPERATION (operation), FALSE);

  klass = GEGL_OPERATION_GET_CLASS (operation);

  operation->node->success = TRUE;
  g_clear_error (&operation->node->error);

  if (!strcmp (output_pad, "output") &&
      (result->width == 0 || result->height == 0))
    {
      GeglBuffer *output = gegl_buffer_new (NULL, NULL);

      gegl_operation_set_error (operation,
                                g_error_new (g_quark_from_static_string ("gegl"),
                                             0, _("Operation %s failed: %s"),
                                             gegl_operation_get_name (operation),
                                             _("processing 0px rectangle.")));
      operation->node->success = FALSE;

      /* when this case is hit.. we've done something bad.. */
      gegl_operation_context_take_object (context, "output", G_OBJECT (output));

      return FALSE;
    }

  if (operation->node->passthrough)
  {
    GeglBuffer *input = GEGL_BUFFER (gegl_operation_context_get_object (context, "input"));
    gegl_operation_context_take_object (context, output_pad, g_object_ref (G_OBJECT (input)));
    return TRUE;
  }

  g_return_val_if_fail (klass->process, FALSE);

  success = klass->process (operation, context, output_pad, result, level);

  operation->node->success = success;
  if (! success && operation->node->error == NULL)
    gegl_operation_set_error (operation,
                              g_error_new (g_quark_from_static_string ("gegl"),
                                           0, _("Operation %s failed: %s"),
                                           gegl_operation_get_name (operation),
                                           _("unknown error.")));

  return success;
}

void
gegl_operation_set_error (GeglOperation *operation,
                          GError        *error)
{
  g_clear_error (&operation->node->error);
  g_propagate_error (&operation->node->error, error);
}

/* Calls an extending class' get_bound_box method if defined otherwise
 * just returns a zero-initialised bounding box
 */
GeglRectangle
gegl_operation_get_bounding_box (GeglOperation *self)
{
  GeglOperationClass *klass = GEGL_OPERATION_GET_CLASS (self);
  GeglRectangle       rect  = { 0, 0, 0, 0 };

  g_return_val_if_fail (GEGL_IS_OPERATION (self), rect);
  g_return_val_if_fail (GEGL_IS_NODE (self->node), rect);

  if (self->node->passthrough)
  {
    GeglRectangle  result = { 0, 0, 0, 0 };
    GeglRectangle *in_rect;
    in_rect = gegl_operation_source_get_bounding_box (self, "input");
    if (in_rect)
      return *in_rect;
    return result;
  }
  else if (klass->get_bounding_box)
    return klass->get_bounding_box (self);

  return rect;
}

GeglRectangle
gegl_operation_get_invalidated_by_change (GeglOperation       *self,
                                          const gchar         *input_pad,
                                          const GeglRectangle *input_region)
{
  GeglOperationClass *klass;
  GeglRectangle       retval = { 0, };

  g_return_val_if_fail (GEGL_IS_OPERATION (self), retval);
  g_return_val_if_fail (input_pad != NULL, retval);
  g_return_val_if_fail (input_region != NULL, retval);

  if (self->node && self->node->passthrough)
    return *input_region;

  klass = GEGL_OPERATION_GET_CLASS (self);

  if (input_region->width  == 0 ||
      input_region->height == 0)
    return *input_region;

  if (klass->get_invalidated_by_change)
    return klass->get_invalidated_by_change (self, input_pad, input_region);

  return *input_region;
}

static GeglRectangle
get_required_for_output (GeglOperation        *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  if (operation->node->passthrough)
    return *roi;

  if (operation->node->is_graph)
    {
      return gegl_operation_get_required_for_output (operation, input_pad, roi);
    }

  return *roi;
}

GeglRectangle
gegl_operation_get_required_for_output (GeglOperation        *operation,
                                        const gchar         *input_pad,
                                        const GeglRectangle *roi)
{
  GeglOperationClass *klass = GEGL_OPERATION_GET_CLASS (operation);

  if (roi->width == 0 ||
      roi->height == 0)
    return *roi;

  if (operation->node->passthrough)
    return *roi;

  g_assert (klass->get_required_for_output);

  return klass->get_required_for_output (operation, input_pad, roi);
}



GeglRectangle
gegl_operation_get_cached_region (GeglOperation        *operation,
                                  const GeglRectangle *roi)
{
  GeglOperationClass *klass = GEGL_OPERATION_GET_CLASS (operation);

  if (operation->node->passthrough)
    return *roi;

  if (!klass->get_cached_region)
    {
      return *roi;
    }

  return klass->get_cached_region (operation, roi);
}

static void
attach (GeglOperation *self)
{
  g_warning ("kilroy was at What The Hack (%p, %s)\n", (void *) self,
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
  return;
}

void
gegl_operation_attach (GeglOperation *self,
                       GeglNode      *node)
{
  GeglOperationClass *klass;

  g_return_if_fail (GEGL_IS_OPERATION (self));
  g_return_if_fail (GEGL_IS_NODE (node));

  klass = GEGL_OPERATION_GET_CLASS (self);

  g_assert (klass->attach);
  self->node = node;
  klass->attach (self);
}

/* Calls the prepare function on the operation that extends this base class */
void
gegl_operation_prepare (GeglOperation *self)
{
  GeglOperationClass *klass;

  g_return_if_fail (GEGL_IS_OPERATION (self));

  if (self->node->passthrough)
    {
      const Babl *format;

      format = gegl_operation_get_source_format (self, "input");
      gegl_operation_set_format (self, "output", format);
      return;
    }

  klass = GEGL_OPERATION_GET_CLASS (self);

  /* build OpenCL kernel */
  if (!klass->cl_data)
  {
    const gchar *cl_source = gegl_operation_class_get_key (klass, "cl-source");
    if (cl_source)
      {
        char *name = g_strdup (klass->name);
        const char *kernel_name[] = {name, NULL};
        char *k;
        for (k=name; *k; k++)
          switch (*k)
            {
              case ' ': case ':': case '-':
                *k = '_';
                break;
            }
        klass->cl_data = gegl_cl_compile_and_build (cl_source, kernel_name);
        g_free (name);
      }
  }

  if (klass->prepare)
    klass->prepare (self);
}

GeglNode *
gegl_operation_get_source_node (GeglOperation *operation,
                                const gchar   *input_pad_name)
{
  GeglNode *node;
  GeglPad *pad;

  g_return_val_if_fail (GEGL_IS_OPERATION (operation), NULL);
  g_return_val_if_fail (GEGL_IS_NODE (operation->node), NULL);
  g_return_val_if_fail (input_pad_name != NULL, NULL);

  node = operation->node;
  if (node->is_graph)
    {
      node = gegl_node_get_input_proxy (node, input_pad_name);
      input_pad_name = "input";
    }

  pad = gegl_node_get_pad (node, input_pad_name);

  if (!pad)
    return NULL;

  pad = gegl_pad_get_connected_to (pad);

  if (!pad)
    return NULL;

  g_assert (gegl_pad_get_node (pad));

  return gegl_pad_get_node (pad);
}

GeglRectangle *
gegl_operation_source_get_bounding_box (GeglOperation *operation,
                                        const gchar   *input_pad_name)
{
  GeglNode *node = gegl_operation_get_source_node (operation, input_pad_name);

  if (node)
    {
      GeglRectangle *ret;
      /* g_mutex_lock (&node->mutex); */
      /* make sure node->have_rect is valid */
      (void) gegl_node_get_bounding_box (node);
      ret = &node->have_rect;
      /* g_mutex_unlock (&node->mutex); */
      return ret;
    }

  return NULL;
}

static GeglRectangle
get_bounding_box (GeglOperation *self)
{
  GeglRectangle rect = { 0, 0, 0, 0 };

  if (self->node->is_graph)
    {
      GeglOperation *operation;

      operation = gegl_node_get_output_proxy (self->node, "output")->operation;
      rect      = gegl_operation_get_bounding_box (operation);
    }
  else
    {
      g_warning ("Operation '%s' has no get_bounding_box() method",
                 G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
    }

  return rect;
}

static GeglRectangle
get_invalidated_by_change (GeglOperation        *self,
                           const gchar         *input_pad,
                           const GeglRectangle *input_region)
{
  return *input_region;
}

/* returns a freshly allocated list of the properties of the object, does not list
 * the regular gobject properties of GeglNode ('name' and 'operation') */
GParamSpec **
gegl_operation_list_properties (const gchar *operation_type,
                                guint       *n_properties_p)
{
  GParamSpec  **pspecs;
  GType         type;
  GObjectClass *klass;

  type = gegl_operation_gtype_from_name (operation_type);
  if (!type)
    {
      if (n_properties_p)
        *n_properties_p = 0;
      return NULL;
    }
  klass  = g_type_class_ref (type);
  pspecs = g_object_class_list_properties (klass, n_properties_p);
  g_type_class_unref (klass);
  return pspecs;
}

GParamSpec *
gegl_operation_find_property (const gchar *operation_type,
                              const gchar *property_name)
{
  GParamSpec *ret = NULL;
  GType         type;
  GObjectClass *klass;

  type = gegl_operation_gtype_from_name (operation_type);
  if (!type)
    return NULL;

  klass  = g_type_class_ref (type);
  ret = g_object_class_find_property (klass, property_name);
  g_type_class_unref (klass);

  return ret;
}

GeglNode *
gegl_operation_detect (GeglOperation *operation,
                       gint           x,
                       gint           y)
{
  GeglNode           *node = NULL;
  GeglOperationClass *klass;

  if (!operation)
    return NULL;

  g_return_val_if_fail (GEGL_IS_OPERATION (operation), NULL);
  node = operation->node;

  klass = GEGL_OPERATION_GET_CLASS (operation);

  if (klass->detect)
    {
      return klass->detect (operation, x, y);
    }

  if (x >= node->have_rect.x &&
      x < node->have_rect.x + node->have_rect.width &&
      y >= node->have_rect.y &&
      y < node->have_rect.y + node->have_rect.height)
    {
      return node;
    }
  return NULL;
}

void
gegl_operation_set_format (GeglOperation *self,
                           const gchar   *pad_name,
                           const Babl    *format)
{
  GeglPad *pad;

  g_return_if_fail (GEGL_IS_OPERATION (self));
  g_return_if_fail (pad_name != NULL);

  pad = gegl_node_get_pad (self->node, pad_name);

  g_return_if_fail (pad != NULL);

  pad->format = format;
}

const Babl *
gegl_operation_get_format (GeglOperation *self,
                           const gchar   *pad_name)
{
  GeglPad *pad;

  g_return_val_if_fail (GEGL_IS_OPERATION (self), NULL);
  g_return_val_if_fail (pad_name != NULL, NULL);

  pad = gegl_node_get_pad (self->node, pad_name);

  g_return_val_if_fail (pad != NULL, NULL);

  return pad->format;
}

const gchar *
gegl_operation_get_name (GeglOperation *operation)
{
  GeglOperationClass *klass;

  g_return_val_if_fail (GEGL_IS_OPERATION (operation), NULL);

  klass = GEGL_OPERATION_GET_CLASS (operation);

  return klass->name;
}

void
gegl_operation_invalidate (GeglOperation       *operation,
                           const GeglRectangle *roi,
                           gboolean             clear_cache)
{
  g_return_if_fail (GEGL_IS_OPERATION (operation));

  if (operation->node)
    gegl_node_invalidated (operation->node, roi, clear_cache);
}

gboolean
gegl_operation_cl_set_kernel_args (GeglOperation *operation,
                                   cl_kernel      kernel,
                                   gint          *p,
                                   cl_int        *err)
{
  GParamSpec **self;
  GParamSpec **parent;
  guint n_self;
  guint n_parent;
  gint prop_no;

  self = g_object_class_list_properties (
            G_OBJECT_CLASS (g_type_class_ref (G_OBJECT_CLASS_TYPE (GEGL_OPERATION_GET_CLASS(operation)))),
            &n_self);

  parent = g_object_class_list_properties (
            G_OBJECT_CLASS (g_type_class_ref (GEGL_TYPE_OPERATION)),
            &n_parent);

  for (prop_no=0;prop_no<n_self;prop_no++)
    {
      gint parent_no;
      gboolean found=FALSE;
      for (parent_no=0;parent_no<n_parent;parent_no++)
        if (self[prop_no]==parent[parent_no])
          found=TRUE;
      /* only print properties if we are an addition compared to
       * GeglOperation
       */

      /* Removing pads */
      if (!strcmp(g_param_spec_get_name (self[prop_no]), "input") ||
          !strcmp(g_param_spec_get_name (self[prop_no]), "output") ||
          !strcmp(g_param_spec_get_name (self[prop_no]), "aux"))
        continue;

      if (!found)
        {
          if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (self[prop_no]), G_TYPE_DOUBLE))
            {
              gdouble value;
              cl_float v;

              g_object_get (G_OBJECT (operation), g_param_spec_get_name (self[prop_no]), &value, NULL);

              v = value;
              *err = gegl_clSetKernelArg(kernel, (*p)++, sizeof(cl_float), (void*)&v);
            }
          else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (self[prop_no]), G_TYPE_FLOAT))
            {
              gfloat value;
              cl_float v;

              g_object_get (G_OBJECT (operation), g_param_spec_get_name (self[prop_no]), &value, NULL);

              v = value;
              *err = gegl_clSetKernelArg(kernel, (*p)++, sizeof(cl_float), (void*)&v);
            }
          else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (self[prop_no]), G_TYPE_INT))
            {
              gint value;
              cl_int v;

              g_object_get (G_OBJECT (operation), g_param_spec_get_name (self[prop_no]), &value, NULL);

              v = value;
              *err = gegl_clSetKernelArg(kernel, (*p)++, sizeof(cl_int), (void*)&v);
            }
          else if (g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (self[prop_no]), G_TYPE_BOOLEAN))
            {
              gboolean value;
              cl_bool v;

              g_object_get (G_OBJECT (operation), g_param_spec_get_name (self[prop_no]), &value, NULL);

              v = value;
              *err = gegl_clSetKernelArg(kernel, (*p)++, sizeof(cl_bool), (void*)&v);
            }
          else
            {
              g_error ("Unsupported OpenCL kernel argument");
              return FALSE;
            }
        }
    }

  g_free (self);
  g_free (parent);

  return TRUE;
}

gchar **
gegl_operation_list_keys (const gchar *operation_name,
                          guint       *n_keys)
{
  GType         type;
  GObjectClass *klass;
  GList        *list, *l;
  gchar       **ret;
  int count;
  int i;
  type = gegl_operation_gtype_from_name (operation_name);
  if (!type)
    {
      if (n_keys)
        *n_keys = 0;
      return NULL;
    }
  klass  = g_type_class_ref (type);
  count = g_hash_table_size (GEGL_OPERATION_CLASS (klass)->keys);
  ret = g_malloc0 (sizeof (gpointer) * (count + 1));
  list = g_hash_table_get_keys (GEGL_OPERATION_CLASS (klass)->keys);
  for (i = 0, l = list; l; l = l->next, i++)
    {
      ret[i] = l->data;
    }
  g_list_free (list);
  if (n_keys)
    *n_keys = count;
  g_type_class_unref (klass);
  return ret;
}

void
gegl_operation_class_set_key (GeglOperationClass *klass,
                              const gchar        *key_name,
                              const gchar        *key_value)
{
  gchar *key_value_dup;

  g_return_if_fail (key_name != NULL);

  if (!key_value)
    {
      g_hash_table_remove (klass->keys, key_name);
      return;
    }

  key_value_dup = g_strdup (key_value);

  if (!strcmp (key_name, "name"))
    {
      klass->name = key_value_dup;
      gegl_operation_class_register_name (klass, key_value, FALSE);
    }
  else if (!strcmp (key_name, "compat-name"))
    {
      klass->compat_name = key_value_dup;
      gegl_operation_class_register_name (klass, key_value, TRUE);
    }

  g_hash_table_insert (klass->keys, g_strdup (key_name),
                       (void*)key_value_dup);
}

void
gegl_operation_class_set_keys (GeglOperationClass *klass,
                               const gchar        *key_name,
                                ...)
{
  va_list var_args;

  va_start (var_args, key_name);
  while (key_name)
    {
      const char *value = va_arg (var_args, char *);

      gegl_operation_class_set_key (klass, key_name, value);

      key_name = va_arg (var_args, char *);
    }
  va_end (var_args);
}

void
gegl_operation_set_key (const gchar *operation_name,
                        const gchar *key_name,
                        const gchar *key_value)
{
  GType         type;
  GObjectClass *klass;
  type = gegl_operation_gtype_from_name (operation_name);
  if (!type)
    return;
  klass  = g_type_class_ref (type);
  gegl_operation_class_set_key (GEGL_OPERATION_CLASS (klass), key_name, key_value);
  g_type_class_unref (klass);
}

const gchar *
gegl_operation_class_get_key (GeglOperationClass *operation_class,
                              const gchar        *key_name)
{
  const gchar  *ret = NULL;
  ret = g_hash_table_lookup (GEGL_OPERATION_CLASS (operation_class)->keys, key_name);
  return ret;
}

const gchar *
gegl_operation_get_key (const gchar *operation_name,
                        const gchar *key_name)
{
  GType         type;
  GObjectClass *klass;
  const gchar  *ret = NULL;
  type = gegl_operation_gtype_from_name (operation_name);
  if (!type)
    {
      return NULL;
    }
  klass  = g_type_class_ref (type);
  ret = gegl_operation_class_get_key (GEGL_OPERATION_CLASS (klass), key_name);
  g_type_class_unref (klass);
  return ret;
}

gboolean
gegl_operation_use_opencl (const GeglOperation *operation)
{
  g_return_val_if_fail (operation->node, FALSE);
  return operation->node->use_opencl && gegl_cl_is_accelerated ();
}

const Babl *
gegl_operation_get_source_format (GeglOperation *operation,
                                  const gchar   *padname)
{
  GeglNode *src_node = gegl_operation_get_source_node (operation, padname);

  if (src_node)
    {
      GeglOperation *op = src_node->operation;
      if (op)
        return gegl_operation_get_format (op, "output");
            /* XXX: could be a different pad than "output" */
    }
  return NULL;
}

gboolean
gegl_operation_use_threading (GeglOperation *operation,
                              const GeglRectangle *roi)
{
  gint threads = gegl_config_threads ();
  if (threads == 1)
    return FALSE;

  {
    GeglOperationClass       *op_class;
    op_class = GEGL_OPERATION_GET_CLASS (operation);

    if (op_class->opencl_support && gegl_cl_is_accelerated ())
      return FALSE;

    if (op_class->threaded &&
        (gdouble) roi->width * (gdouble) roi->height >=
        2 * gegl_operation_get_pixels_per_thread (operation))
      return TRUE;
  }
  return FALSE;
}

gdouble
gegl_operation_get_pixels_per_thread (GeglOperation *operation)
{
  /* FIXME: too arbitrary? */
  return 64 * 64;
}

static guchar *gegl_temp_alloc[GEGL_MAX_THREADS * 4]={NULL,};
static gint    gegl_temp_size[GEGL_MAX_THREADS * 4]={0,};

guchar *gegl_temp_buffer (int no, int size)
{
  if (!gegl_temp_alloc[no] || gegl_temp_size[no] < size)
  {
    if (gegl_temp_alloc[no])
      gegl_free (gegl_temp_alloc[no]);
    gegl_temp_alloc[no] = gegl_malloc (size);
    gegl_temp_size[no] = size;
  }
  return gegl_temp_alloc[no];
}

void gegl_temp_buffer_free (void);
void gegl_temp_buffer_free (void)
{
  int no;
  for (no = 0; no < GEGL_MAX_THREADS * 4; no++)
    if (gegl_temp_alloc[no])
    {
      gegl_free (gegl_temp_alloc[no]);
      gegl_temp_alloc[no] = NULL;
      gegl_temp_size[no] = 0;
    }
}

void gegl_operation_progress (GeglOperation *operation,
                              gdouble        progress,
                              gchar         *message)
{
  if (operation->node)
    gegl_node_progress (operation->node, progress, message);
}

const Babl *
gegl_operation_get_source_space (GeglOperation *operation, const char *in_pad)
{
 const Babl *source_format = gegl_operation_get_source_format (operation, "input");
  if (source_format)
    return babl_format_get_space (source_format);
  return NULL;
}

gboolean
gegl_operation_use_cache (GeglOperation *operation)
{
  GeglOperationClass *klass = GEGL_OPERATION_GET_CLASS (operation);

  switch (klass->cache_policy)
    {
    case GEGL_CACHE_POLICY_AUTO:
      return ! klass->no_cache && klass->get_cached_region != NULL;

    case GEGL_CACHE_POLICY_NEVER:
      return FALSE;

    case GEGL_CACHE_POLICY_ALWAYS:
      return TRUE;
    }

  g_return_val_if_reached (FALSE);
}
