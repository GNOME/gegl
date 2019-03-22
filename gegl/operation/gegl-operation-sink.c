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
#include "gegl-types-internal.h"
#include "gegl-operation-sink.h"
#include "gegl-operation-context.h"

static gboolean      gegl_operation_sink_process2                (GeglOperation        *operation,
                                                                  GeglOperationContext *context,
                                                                  const gchar          *output_prop,
                                                                  const GeglRectangle  *result,
                                                                  gint                  level,
                                                                  GError              **error);
static void          gegl_operation_sink_attach                  (GeglOperation        *operation);
static GeglRectangle gegl_operation_sink_get_bounding_box        (GeglOperation        *self);
static GeglRectangle gegl_operation_sink_get_required_for_output (GeglOperation        *operation,
                                                                  const gchar          *input_pad,
                                                                  const GeglRectangle  *roi);


G_DEFINE_TYPE (GeglOperationSink, gegl_operation_sink, GEGL_TYPE_OPERATION)


static void
gegl_operation_sink_class_init (GeglOperationSinkClass * klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  klass->needs_full = FALSE;

  operation_class->process2                = gegl_operation_sink_process2;
  operation_class->attach                  = gegl_operation_sink_attach;
  operation_class->get_bounding_box        = gegl_operation_sink_get_bounding_box;
  operation_class->get_required_for_output = gegl_operation_sink_get_required_for_output;
}

static void
gegl_operation_sink_init (GeglOperationSink *self)
{
}

static void
gegl_operation_sink_attach (GeglOperation *self)
{
  GeglOperation *operation = GEGL_OPERATION (self);
  GParamSpec    *pspec;

  pspec = g_param_spec_object ("input",
                               "Input",
                               "Input pad, for image buffer input.",
                               GEGL_TYPE_BUFFER,
                               G_PARAM_READWRITE |
                               GEGL_PARAM_PAD_INPUT);
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);
}

static gboolean
gegl_operation_sink_process2 (GeglOperation         *operation,
                              GeglOperationContext  *context,
                              const gchar           *output_prop,
                              const GeglRectangle   *result,
                              gint                   level,
                              GError               **error)
{
  GeglOperationSinkClass *klass;
  GeglBuffer             *input;
  gboolean                success = FALSE;

  klass = GEGL_OPERATION_SINK_GET_CLASS (operation);

  g_assert (klass->process || klass->process2);

  input = (GeglBuffer*) gegl_operation_context_dup_object (context, "input");
  if (input)
    {
      if (klass->process2)
        success = klass->process2 (operation, input, result, level, error);
      else
        success = klass->process (operation, input, result, level);

      g_object_unref (input);
    }
  else if (error)
    {
      *error = g_error_new (g_quark_from_static_string ("gegl"),
                            0, "Sink operation has no input");
    }

  return success;
}

static GeglRectangle
gegl_operation_sink_get_bounding_box (GeglOperation *self)
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
gegl_operation_sink_get_required_for_output (GeglOperation       *operation,
                                             const gchar         *input_pad,
                                             const GeglRectangle *roi)
{
  GeglRectangle rect=*roi;
  return rect;
}

gboolean gegl_operation_sink_needs_full (GeglOperation *operation)
{
  GeglOperationSinkClass *klass;

  klass  = GEGL_OPERATION_SINK_CLASS (G_OBJECT_GET_CLASS (operation));
  return klass->needs_full;
}
