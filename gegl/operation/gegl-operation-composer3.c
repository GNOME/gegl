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
#include "gegl-operation-composer3.h"
#include "gegl-operation-context.h"
#include "gegl-config.h"

static gboolean gegl_operation_composer3_process2
(GeglOperation        *operation,
 GeglOperationContext *context,
 const gchar          *output_prop,
 const GeglRectangle  *result,
 gint                  level,
 GError             **error);
static void     attach       (GeglOperation        *operation);
static GeglNode*detect       (GeglOperation        *operation,
    gint                  x,
    gint                  y);

static GeglRectangle get_bounding_box        (GeglOperation        *self);
static GeglRectangle get_required_for_output (GeglOperation        *self,
    const gchar         *input_pad,
    const GeglRectangle *roi);

G_DEFINE_TYPE (GeglOperationComposer3, gegl_operation_composer3,
    GEGL_TYPE_OPERATION)


  static void
gegl_operation_composer3_class_init (GeglOperationComposer3Class * klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->process2 = gegl_operation_composer3_process2;
  operation_class->attach = attach;
  operation_class->detect = detect;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_required_for_output = get_required_for_output;
}

  static void
gegl_operation_composer3_init (GeglOperationComposer3 *self)
{
}

  static void
attach (GeglOperation *self)
{
  GeglOperation *operation = GEGL_OPERATION (self);
  GParamSpec    *pspec;

  pspec = g_param_spec_object ("output",
      "Output",
      "Output pad for generated image buffer.",
      GEGL_TYPE_BUFFER,
      G_PARAM_READABLE |
      GEGL_PARAM_PAD_OUTPUT);
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);

  pspec = g_param_spec_object ("input",
      "Input",
      "Input pad, for image buffer input.",
      GEGL_TYPE_BUFFER,
      G_PARAM_READWRITE |
      GEGL_PARAM_PAD_INPUT);
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);

  pspec = g_param_spec_object ("aux",
      "Aux",
      "Auxiliary image buffer input pad.",
      GEGL_TYPE_BUFFER,
      G_PARAM_READWRITE |
      GEGL_PARAM_PAD_INPUT);
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);

  pspec = g_param_spec_object ("aux2",
      "Aux2",
      "Second auxiliary image buffer input pad.",
      GEGL_TYPE_BUFFER,
      G_PARAM_READWRITE |
      GEGL_PARAM_PAD_INPUT);
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);
}

typedef struct ThreadData
{
  GeglOperationComposer3Class *klass;
  GeglOperation               *operation;
  GeglOperationContext        *context;
  GeglBuffer                  *input;
  GeglBuffer                  *aux;
  GeglBuffer                  *aux2;
  GeglBuffer                  *output;
  const GeglRectangle         *roi;
  gint                         level;
  gboolean                     success;
  GError                     **error;
} ThreadData;

static void
thread_process (const GeglRectangle *area,
                ThreadData          *data)
{
  GeglBuffer *input;
  gboolean    success = TRUE;

  if (area->x == data->roi->x && area->y == data->roi->y)
    {
      input = g_object_ref (data->input);
    }
  else
    {
      input = gegl_operation_context_dup_input_maybe_copy (data->context,
                                                           "input", area);
    }

  if (data->klass->process2)
    success = data->klass->process2 (data->operation,
                                     input, data->aux, data->aux2,
                                     data->output, area, data->level,
                                     data->error);
  else
    success = data->klass->process (data->operation,
                                    input, data->aux, data->aux2,
                                    data->output, area, data->level);
  if (! success)
    data->success = FALSE;

  g_object_unref (input);
}


static gboolean
gegl_operation_composer3_process2 (GeglOperation        *operation,
                                   GeglOperationContext *context,
                                   const gchar          *output_prop,
                                   const GeglRectangle  *result,
                                   gint                  level,
                                   GError              **error)
{
  GeglOperationComposer3Class *klass   = GEGL_OPERATION_COMPOSER3_GET_CLASS (operation);
  GeglBuffer                  *input;
  GeglBuffer                  *aux;
  GeglBuffer                  *aux2;
  GeglBuffer                  *output;
  gboolean                     success = FALSE;

  if (strcmp (output_prop, "output"))
  {
    g_warning ("requested processing of %s pad on a composer", output_prop);
    return FALSE;
  }

  if (result->width == 0 || result->height == 0)
  {
    output = gegl_operation_context_get_target (context, "output");
    return TRUE;
  }

  input  = (GeglBuffer*) gegl_operation_context_dup_object (context, "input");
  output = gegl_operation_context_get_output_maybe_in_place (operation,
                                                             context,
                                                             input,
                                                             result);

  aux   = (GeglBuffer*)gegl_operation_context_dup_object (context, "aux");
  aux2  = (GeglBuffer*)gegl_operation_context_dup_object (context, "aux2");

  /* A composer with a NULL aux, can still be valid, the
   * subclass has to handle it.
   */
  if (input != NULL ||
      aux != NULL ||
      aux2 != NULL)
    {
      if (gegl_operation_use_threading (operation, result))
      {
        ThreadData data;

        data.klass = klass;
        data.operation = operation;
        data.context = context;
        data.input = input;
        data.aux = aux;
        data.aux2 = aux2;
        data.output = output;
        data.roi = result;
        data.level = level;
        data.success = TRUE;
        data.error = error;

        gegl_parallel_distribute_area (
          result,
          gegl_operation_get_pixels_per_thread (operation),
          GEGL_SPLIT_STRATEGY_AUTO,
          (GeglParallelDistributeAreaFunc) thread_process,
          &data);

        success = data.success;
      }
      else
      {
        if (klass->process2)
          success = klass->process2 (operation, input, aux, aux2, output, result, level, error);
        else
          success = klass->process (operation, input, aux, aux2, output, result, level);
      }

      g_clear_object (&input);
      g_clear_object (&aux);
      g_clear_object (&aux2);
    }
  else
    {
      g_warning ("%s received NULL input, aux, and aux2",
                 gegl_node_get_operation (operation->node));
    }

  return success;
}

static GeglRectangle
get_bounding_box (GeglOperation *self)
{
  GeglRectangle  result    = { 0, 0, 0, 0 };
  GeglRectangle *in_rect   = gegl_operation_source_get_bounding_box (self, "input");
  GeglRectangle *aux_rect  = gegl_operation_source_get_bounding_box (self, "aux");
  GeglRectangle *aux2_rect = gegl_operation_source_get_bounding_box (self, "aux2");

  if (in_rect)
    result = *in_rect;

  if (aux_rect)
    gegl_rectangle_bounding_box (&result, &result, aux_rect);

  if (aux2_rect)
    gegl_rectangle_bounding_box (&result, &result, aux2_rect);

  return result;
}

static GeglRectangle
get_required_for_output (GeglOperation       *self,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return *roi;
}

static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  GeglNode *input_node = gegl_operation_get_source_node (operation, "input");
  GeglNode *aux_node   = gegl_operation_get_source_node (operation, "aux");
  GeglNode *aux2_node  = gegl_operation_get_source_node (operation, "aux2");

  if (input_node)
    input_node = gegl_node_detect (input_node, x, y);
  if (aux_node)
    aux_node = gegl_node_detect (aux_node, x, y);
  if (aux2_node)
    aux2_node = gegl_node_detect (aux2_node, x, y);

  if (aux2_node)
    return aux2_node;
  if (aux_node)
    return aux_node;
  if (input_node)
    return input_node;
  return NULL;
}
