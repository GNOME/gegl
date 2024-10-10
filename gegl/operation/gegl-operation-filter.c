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
 * Copyright 2006, 2014 Øyvind Kolås
 */

#include "config.h"

#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gegl.h"
#include "gegl-operation-filter.h"
#include "gegl-operation-context.h"
#include "gegl-config.h"

static gboolean gegl_operation_filter_process
                                      (GeglOperation        *operation,
                                       GeglOperationContext *context,
                                       const gchar          *output_prop,
                                       const GeglRectangle  *result,
                                       gint                  level);

static void     attach                 (GeglOperation *operation);
static GeglNode *detect                (GeglOperation *operation,
                                        gint           x,
                                        gint           y);

static GeglRectangle get_bounding_box          (GeglOperation       *self);
static GeglRectangle get_required_for_output   (GeglOperation       *operation,
                                                 const gchar         *input_pad,
                                                 const GeglRectangle *roi);

static void prepare (GeglOperation *operation)
{
  const Babl *space  = NULL;//gegl_operation_get_source_space (operation, "input");
  const Babl *format = babl_format_with_space ("RGBA float", space);

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}


G_DEFINE_TYPE (GeglOperationFilter, gegl_operation_filter, GEGL_TYPE_OPERATION)


static void
gegl_operation_filter_class_init (GeglOperationFilterClass * klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->process                 = gegl_operation_filter_process;
  operation_class->threaded                = TRUE;
  operation_class->attach                  = attach;
  operation_class->detect                  = detect;
  operation_class->prepare                 = prepare;
  operation_class->get_bounding_box        = get_bounding_box;
  operation_class->get_required_for_output = get_required_for_output;
}

static void
gegl_operation_filter_init (GeglOperationFilter *self)
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
                               _("Input"),
                               _("Input pad, for image buffer input."),
                               GEGL_TYPE_BUFFER,
                               G_PARAM_READWRITE |
                               GEGL_PARAM_PAD_INPUT);
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);
}


static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  GeglNode *input_node;

  input_node = gegl_operation_get_source_node (operation, "input");

  if (input_node)
    return gegl_node_detect (input_node, x, y);
  return operation->node;
}

typedef struct ThreadData
{
  GeglOperationFilterClass *klass;
  GeglOperation            *operation;
  GeglOperationContext     *context;
  GeglBuffer               *input;
  GeglBuffer               *output;
  const GeglRectangle      *roi;
  gint                      level;
  gboolean                  success;
} ThreadData;

static void
thread_process (const GeglRectangle *area,
                ThreadData          *data)
{
  GeglBuffer *input;

  if (gegl_rectangle_equal (area, data->roi))
    {
      input = g_object_ref (data->input);
    }
  else
    {
      input = gegl_operation_context_dup_input_maybe_copy (data->context,
                                                           "input", area);
    }

  if (!data->klass->process (data->operation,
                             input, data->output, area, data->level))
    data->success = FALSE;

  g_object_unref (input);
}

static gboolean
gegl_operation_filter_process (GeglOperation        *operation,
                               GeglOperationContext *context,
                               const gchar          *output_prop,
                               const GeglRectangle  *result,
                               gint                  level)
{
  GeglOperationFilterClass *klass;
  GeglBuffer               *input;
  GeglBuffer               *output;
  gboolean                  success = FALSE;

  klass = GEGL_OPERATION_FILTER_GET_CLASS (operation);

  g_assert (klass->process);

  if (strcmp (output_prop, "output"))
    {
      g_warning ("requested processing of %s pad on a filter", output_prop);
      return FALSE;
    }

  input  = (GeglBuffer*)gegl_operation_context_dup_object (context, "input");
  output = gegl_operation_context_get_output_maybe_in_place (operation,
                                                             context,
                                                             input,
                                                             result);

  if (gegl_operation_use_threading (operation, result))
  {
    ThreadData        data;
    GeglSplitStrategy split_strategy = GEGL_SPLIT_STRATEGY_AUTO;

    if (klass->get_split_strategy)
    {
      split_strategy = klass->get_split_strategy (operation, context,
                                                  output_prop, result, level);
    }

    data.klass = klass;
    data.operation = operation;
    data.context = context;
    data.input = input;
    data.output = output;
    data.roi = result;
    data.level = level;
    data.success = TRUE;

    gegl_parallel_distribute_area (
      result,
      gegl_operation_get_pixels_per_thread (operation),
      split_strategy,
      (GeglParallelDistributeAreaFunc) thread_process,
      &data);

    success = data.success;
  }
  else
  {
    success = klass->process (operation, input, output, result, level);
  }

  g_clear_object (&input);
  return success;
}

static GeglRectangle
get_bounding_box (GeglOperation *self)
{
  GeglRectangle  result = { 0, 0, 0, 0 };
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (self, "input");
  if (in_rect)
    {
      result = *in_rect;
    }

  return result;
}

static GeglRectangle
get_required_for_output (GeglOperation        *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return *roi;
}
