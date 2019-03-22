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
#include "gegl-operation-source.h"
#include "gegl-operation-context.h"
#include "gegl-config.h"

static gboolean gegl_operation_source_process2
                             (GeglOperation        *operation,
                              GeglOperationContext *context,
                              const gchar          *output_prop,
                              const GeglRectangle  *result,
                              gint                  level,
                              GError              **error);
static void     attach       (GeglOperation *operation);



G_DEFINE_TYPE (GeglOperationSource, gegl_operation_source, GEGL_TYPE_OPERATION)

static GeglRectangle get_bounding_box          (GeglOperation        *self);
static GeglRectangle get_required_for_output   (GeglOperation        *operation,
                                                 const gchar         *input_pad,
                                                 const GeglRectangle *roi);


static void
gegl_operation_source_class_init (GeglOperationSourceClass * klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->process2 = gegl_operation_source_process2;
  operation_class->attach   = attach;

  operation_class->get_bounding_box  = get_bounding_box;
  operation_class->get_required_for_output = get_required_for_output;
}

static void
gegl_operation_source_init (GeglOperationSource *self)
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
}

typedef struct ThreadData
{
  GeglOperationSourceClass *klass;
  GeglOperation            *operation;
  GeglBuffer               *output;
  gint                      level;
  gboolean                  success;
  GError                  **error;
} ThreadData;

static void
thread_process (const GeglRectangle *area,
                ThreadData          *data)
{
  gboolean success = TRUE;

  if (data->klass->process2)
    success = data->klass->process2 (data->operation,
                                     data->output, area, data->level, data->error);
  else
    success = data->klass->process (data->operation,
                                    data->output, area, data->level);
  if (! success)
    data->success = FALSE;
}

static gboolean
gegl_operation_source_process2 (GeglOperation        *operation,
                                GeglOperationContext *context,
                                const gchar          *output_prop,
                                const GeglRectangle  *result,
                                gint                  level,
                                GError              **error)
{
  GeglOperationSourceClass *klass = GEGL_OPERATION_SOURCE_GET_CLASS (operation);
  GeglBuffer               *output;
  gboolean                  success = FALSE;

  if (strcmp (output_prop, "output"))
    {
      g_warning ("requested processing of %s pad on a source operation", output_prop);
      return FALSE;
    }

  g_assert (klass->process || klass->process2);
  output = gegl_operation_context_get_target (context, "output");

  if (gegl_operation_use_threading (operation, result))
  {
    ThreadData data;

    data.klass = klass;
    data.operation = operation;
    data.output = output;
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
      success = klass->process2 (operation, output, result, level, error);
    else
      success = klass->process (operation, output, result, level);
  }

  return success;
}

static GeglRectangle
get_bounding_box (GeglOperation *self)
{
  GeglRectangle result = { 0, 0, 0, 0 };

  g_warning ("Gegl Source '%s' does not override %s()",
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)),
             G_STRFUNC);

  return result;
}

static GeglRectangle
get_required_for_output (GeglOperation        *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return *roi;
}
