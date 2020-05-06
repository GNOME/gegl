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
 *           2006-2008 Øyvind Kolås
 *           2013 Daniel Sabo
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-operation-context.h"
#include "gegl-operation-context-private.h"
#include "gegl-node-private.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-backend-buffer.h"
#include "gegl-config.h"

#include "operation/gegl-operation.h"

static GValue *
gegl_operation_context_add_value (GeglOperationContext *self,
                                  const gchar          *property_name);


void
gegl_operation_context_set_need_rect (GeglOperationContext *self,
                                      const GeglRectangle  *rect)
{
  g_assert (self);
  self->need_rect = *rect;
}

GeglRectangle *
gegl_operation_context_get_result_rect (GeglOperationContext *self)
{
  return &self->result_rect;
}

void
gegl_operation_context_set_result_rect (GeglOperationContext *self,
                                        const GeglRectangle  *rect)
{
  g_assert (self);
  self->result_rect = *rect;
}

GeglRectangle *
gegl_operation_context_get_need_rect (GeglOperationContext *self)
{
  return &self->need_rect;
}

void
gegl_operation_context_set_property (GeglOperationContext *context,
                                     const gchar          *property_name,
                                     const GValue         *value)
{
  GValue     *storage;

  g_return_if_fail (context != NULL);
  g_return_if_fail (G_VALUE_TYPE (value) == GEGL_TYPE_BUFFER);

  /* if the value already exists in the context it will be reused */
  storage = gegl_operation_context_add_value (context, property_name);

  g_value_copy (value, storage);
}

void
gegl_operation_context_get_property (GeglOperationContext *context,
                                     const gchar          *property_name,
                                     GValue               *value)
{
  GValue     *storage;

  storage = gegl_operation_context_get_value (context, property_name);
  if (storage != NULL)
    {
      g_value_copy (storage, value);
    }
}

typedef struct Property
{
  gchar *name;
  GValue value;
} Property;

static Property *
property_new (const gchar *property_name)
{
  Property *property = g_slice_new0 (Property);

  property->name = g_strdup (property_name);
  return property;
}

static void
property_destroy (Property *property)
{
  g_free (property->name);
  g_value_unset (&property->value); /* does an unref */
  g_slice_free (Property, property);
}

static gint
lookup_property (gconstpointer a,
                 gconstpointer property_name)
{
  Property *property = (void *) a;

  return strcmp (property->name, property_name);
}

GValue *
gegl_operation_context_get_value (GeglOperationContext *self,
                                  const gchar          *property_name)
{
  Property *property = NULL;

  {
    GSList *found;
    found = g_slist_find_custom (self->property, property_name, lookup_property);
    if (found)
      property = found->data;
  }
  if (!property)
    {
      return NULL;
    }
  return &property->value;
}

void
gegl_operation_context_remove_property (GeglOperationContext *self,
                                        const gchar          *property_name)
{
  Property *property = NULL;

  GSList *found;
  found = g_slist_find_custom (self->property, property_name, lookup_property);
  if (found)
    property = found->data;

  if (!property)
    {
      g_warning ("didn't find property %s for %s", property_name,
                 GEGL_OPERATION_GET_CLASS (self->operation)->name);
      return;
    }
  self->property = g_slist_remove (self->property, property);
  property_destroy (property);
}

static GValue *
gegl_operation_context_add_value (GeglOperationContext *self,
                                  const gchar          *property_name)
{
  Property *property = NULL;
  GSList   *found;

  found = g_slist_find_custom (self->property, property_name, lookup_property);

  if (found)
    {
      property = found->data;
    }

  if (property)
    {
      g_value_reset (&property->value);
      return &property->value;
    }

  property = property_new (property_name);

  self->property = g_slist_prepend (self->property, property);
  g_value_init (&property->value, GEGL_TYPE_BUFFER);

  return &property->value;
}

GeglOperationContext *
gegl_operation_context_new (GeglOperation *operation,
                            GHashTable    *hashtable)
{
  GeglOperationContext *self = g_slice_new0 (GeglOperationContext);
  self->operation = operation;
  self->contexts = hashtable;
  return self;
}

void
gegl_operation_context_purge (GeglOperationContext *self)
{
  while (self->property)
    {
      Property *property = self->property->data;
      self->property = g_slist_remove (self->property, property);
      property_destroy (property);
    }
}

void
gegl_operation_context_destroy (GeglOperationContext *self)
{
  gegl_operation_context_purge (self);
  g_slice_free (GeglOperationContext, self);
}

void
gegl_operation_context_set_object (GeglOperationContext *context,
                                   const gchar          *padname,
                                   GObject              *data)
{
  g_return_if_fail (!data || GEGL_IS_BUFFER (data));

  /* Make it simple, just add an extra ref and then take the object */
  if (data)
    g_object_ref (data);
  gegl_operation_context_take_object (context, padname, data);
}

void
gegl_operation_context_take_object (GeglOperationContext *context,
                                    const gchar          *padname,
                                    GObject              *data)
{
  GValue *storage;

  g_return_if_fail (context != NULL);
  g_return_if_fail (!data || GEGL_IS_BUFFER (data));

  storage = gegl_operation_context_add_value (context, padname);
  g_value_take_object (storage, data);
}

GObject *
gegl_operation_context_dup_object (GeglOperationContext *context,
                                   const gchar          *padname)
{
  GObject *ret;

  ret = gegl_operation_context_get_object (context, padname);
  if (ret != NULL)
    g_object_ref (ret);

  return ret;
}

GObject *
gegl_operation_context_get_object (GeglOperationContext *context,
                                   const gchar          *padname)
{
  GObject     *ret;
  GValue      *value;

  value = gegl_operation_context_get_value (context, padname);

  if (value != NULL)
    {
      ret = g_value_get_object (value);
      if (ret != NULL)
        {
          return ret;
        }
    }

  return NULL;
}

GeglBuffer *
gegl_operation_context_get_source (GeglOperationContext *context,
                                   const gchar          *padname)
{
  GeglBuffer *input;

  input = GEGL_BUFFER (gegl_operation_context_dup_object (context, padname));
  return input;
}

GeglBuffer *
gegl_operation_context_get_target (GeglOperationContext *context,
                                   const gchar          *padname)
{
  GeglBuffer          *output         = NULL;
  const GeglRectangle *result;
  const Babl          *format;
  GeglNode            *node;
  GeglOperation       *operation;
  static gint          linear_buffers = -1;

#if 0
  g_return_val_if_fail (GEGL_IS_OPERATION_CONTEXT (context), NULL);
#endif

  g_return_val_if_fail (g_strcmp0 (padname, "output") == 0, NULL);

  if (linear_buffers == -1)
    linear_buffers = g_getenv ("GEGL_LINEAR_BUFFERS")?1:0;

  operation = context->operation;
  node = operation->node; /* <ick */
  format = gegl_operation_get_format (operation, padname);

  if (format == NULL)
    {
      g_warning ("no format for %s presuming RGBA float\n",
                 gegl_node_get_debug_name (node));
      format = gegl_babl_rgba_linear_float ();
    }
  g_assert (format != NULL);

  result = &context->result_rect;

  if (result->width == 0 ||
      result->height == 0)
    {
      if (linear_buffers)
        output = gegl_buffer_linear_new (GEGL_RECTANGLE(0, 0, 0, 0), format);
      else
        output = gegl_buffer_new (GEGL_RECTANGLE (0, 0, 0, 0), format);
    }
  else if (gegl_node_use_cache (node))
    {
      GeglBuffer    *cache;
      cache = GEGL_BUFFER (gegl_node_get_cache (node));

      /* Only use the cache if the result is within the cache
       * extent. This is certainly not optimal. My gut feeling is that
       * the current caching mechanism needs to be redesigned
       */
      if (gegl_rectangle_contains (gegl_buffer_get_extent (cache), result))
        output = g_object_ref (cache);
    }

  if (! output)
    {
      if (linear_buffers)
        {
          output = gegl_buffer_linear_new (result, format);
        }
      else
        {
          output = g_object_new (
            GEGL_TYPE_BUFFER,
            "x",           result->x,
            "y",           result->y,
            "width",       result->width,
            "height",      result->height,
            "format",      format,
            "initialized", gegl_operation_context_get_init_output (),
            NULL);
        }
    }

  gegl_operation_context_take_object (context, padname, G_OBJECT (output));

  return output;
}

gint
gegl_operation_context_get_level (GeglOperationContext *ctxt)
{
  return ctxt->level;
}


GeglBuffer *
gegl_operation_context_get_output_maybe_in_place (GeglOperation *operation,
                                                  GeglOperationContext *context,
                                                  GeglBuffer    *input,
                                                  const GeglRectangle *roi)
{
  GeglOperationClass *klass = GEGL_OPERATION_GET_CLASS (operation);
  GeglBuffer *output;

  if (klass->want_in_place                    &&
      ! gegl_node_use_cache (operation->node) &&
      gegl_can_do_inplace_processing (operation, input, roi))
    {
      output = g_object_ref (input);
      gegl_operation_context_take_object (context, "output", G_OBJECT (output));
    }
  else
    {
      output = gegl_operation_context_get_target (context, "output");
    }
  return output;
}

GeglOperationContext *
gegl_operation_context_node_get_context (GeglOperationContext *context,
                                         GeglNode             *node)
{
  if (context->contexts)
    return g_hash_table_lookup (context->contexts, node);
  return NULL;
}


GeglBuffer *
gegl_operation_context_dup_input_maybe_copy (GeglOperationContext *context,
                                             const gchar          *padname,
                                             const GeglRectangle  *roi)
{
  GeglBuffer *input;
  GeglBuffer *output;
  GeglBuffer *result;

  input = GEGL_BUFFER (gegl_operation_context_get_object (context, padname));

  if (! input)
    return NULL;

  output = GEGL_BUFFER (gegl_operation_context_get_object (context, "output"));

  /* return input directly when processing in-place, otherwise, the copied
   * input buffer will occupy space in the cache after the original is modified
   */
  if (input == output)
    return g_object_ref (input);

#if 1
  {
    GeglTileBackend *backend;

    backend = gegl_tile_backend_buffer_new (input);
    gegl_tile_backend_set_flush_on_destroy (backend, FALSE);

    /* create new buffer with similar characteristics to the input buffer */
    result = g_object_new (GEGL_TYPE_BUFFER,
                           "format",       input->soft_format,
                           "x",            input->extent.x,
                           "y",            input->extent.y,
                           "width",        input->extent.width,
                           "height",       input->extent.height,
                           "abyss-x",      input->abyss.x,
                           "abyss-y",      input->abyss.y,
                           "abyss-width",  input->abyss.width,
                           "abyss-height", input->abyss.height,
                           "shift-x",      input->shift_x,
                           "shift-y",      input->shift_y,
                           "tile-width",   input->tile_width,
                           "tile-height",  input->tile_height,
                           "backend",      backend,
                           NULL);

    g_object_unref (backend);
  }
#else
  {
    GeglRectangle required;
    GeglRectangle temp;
    gint          shift_x;
    gint          shift_y;
    gint          tile_width;
    gint          tile_height;

    /* return input directly when processing a level greater than 0, since
     * gegl_buffer_copy() only copies level-0 tiles
     */
    if (context->level > 0)
      return g_object_ref (input);

    /* get required region to copy */
    required = gegl_operation_get_required_for_output (context->operation,
                                                       padname, roi);

    /* return input directly if the required rectangle is infinite, so that we
     * don't attempt to copy an infinite region
     */
    if (gegl_rectangle_is_infinite_plane (&required))
      return g_object_ref (input);

    /* align required region to the tile grid */
    shift_x     = input->shift_x;
    shift_y     = input->shift_y;
    tile_width  = input->tile_width;
    tile_height = input->tile_height;

    temp.x      = (gint) floor ((gdouble) (required.x                   + shift_x) / tile_width)  * tile_width;
    temp.y      = (gint) floor ((gdouble) (required.y                   + shift_y) / tile_height) * tile_height;
    temp.width  = (gint) ceil  ((gdouble) (required.x + required.width  + shift_x) / tile_width)  * tile_width  - temp.x;
    temp.height = (gint) ceil  ((gdouble) (required.y + required.height + shift_y) / tile_height) * tile_height - temp.y;

    temp.x -= shift_x;
    temp.y -= shift_y;

    required = temp;

    /* intersect required region with input abyss */
    gegl_rectangle_intersect (&required, &required, &input->abyss);

    /* create new buffer with similar characteristics to the input buffer */
    result = g_object_new (GEGL_TYPE_BUFFER,
                           "format",       input->soft_format,
                           "x",            input->extent.x,
                           "y",            input->extent.y,
                           "width",        input->extent.width,
                           "height",       input->extent.height,
                           "abyss-x",      input->abyss.x,
                           "abyss-y",      input->abyss.y,
                           "abyss-width",  input->abyss.width,
                           "abyss-height", input->abyss.height,
                           "shift-x",      shift_x,
                           "shift-y",      shift_y,
                           "tile-width",   tile_width,
                           "tile-height",  tile_height,
                           NULL);

    /* if the tile size doesn't match, bail */
    if (result->tile_width != tile_width || result->tile_height != tile_height)
      {
        g_object_unref (result);

        return g_object_ref (input);
      }

    /* copy required region from input to result -- tiles will generally be COWed */
    gegl_buffer_copy (input,  &required, GEGL_ABYSS_NONE,
                      result, &required);
  }
#endif

  return result;
}

gboolean
gegl_operation_context_get_init_output (void)
{
  static gint init_output = -1;

  if (init_output < 0)
    {
      if (g_getenv ("GEGL_OPERATION_INIT_OUTPUT"))
        {
          init_output = atoi (g_getenv ("GEGL_OPERATION_INIT_OUTPUT")) ?
            TRUE : FALSE;
        }
      else
        {
          init_output = FALSE;
        }
    }

  return init_output;
}
