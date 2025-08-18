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

#include "gegl.h"
#include "gegl-debug.h"
#include "gegl-operation-point-composer3.h"
#include "gegl-operation-context.h"
#include "gegl-types-internal.h"
#include "gegl-config.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-storage.h"
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "opencl/gegl-cl.h"
#include "gegl-buffer-cl-cache.h"
#include "gegl-buffer-cl-iterator.h"

typedef struct ThreadData
{
  GeglOperationPointComposer3Class *klass;
  GeglOperation                    *operation;
  GeglBuffer                       *input;
  GeglBuffer                       *aux;
  GeglBuffer                       *aux2;
  GeglBuffer                       *output;
  gint                              level;
  gboolean                          success;

  const Babl *input_format;
  const Babl *aux_format;
  const Babl *aux2_format;
  const Babl *output_format;
} ThreadData;

static void
thread_process (const GeglRectangle *area,
                ThreadData          *data)
{
  gint read = 0;
  gint aux  = 0;
  gint aux2 = 0;
  GeglBufferIterator *i = gegl_buffer_iterator_new (data->output,
                                                    area,
                                                    data->level,
                                                    data->output_format,
                                                    GEGL_ACCESS_WRITE,
                                                    GEGL_ABYSS_NONE, 4);

  if (data->input)
    read = gegl_buffer_iterator_add (i, data->input, area, data->level,
                                     data->input_format,
                                     GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  if (data->aux)
    aux = gegl_buffer_iterator_add (i, data->aux, area, data->level,
                                    data->aux_format,
                                    GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  if (data->aux2)
    aux2 = gegl_buffer_iterator_add (i, data->aux2, area, data->level,
                                    data->aux2_format,
                                    GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (i))
  {
     data->success =
     data->klass->process (data->operation,
                           data->input?i->items[read].data:NULL,
                           data->aux?i->items[aux].data:NULL,
                           data->aux2?i->items[aux2].data:NULL,
                           i->items[0].data, i->length, &(i->items[0].roi), data->level);
  }
}

static gboolean
gegl_operation_composer3_process (GeglOperation        *operation,
                                  GeglOperationContext *context,
                                  const gchar          *output_prop,
                                  const GeglRectangle  *result,
                                  gint                  level)
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

  aux   = (GeglBuffer*) gegl_operation_context_dup_object (context, "aux");
  aux2  = (GeglBuffer*) gegl_operation_context_dup_object (context, "aux2");

  /* A composer with a NULL aux, can still be valid, the
   * subclass has to handle it.
   */
  if (input != NULL ||
      aux != NULL ||
      aux2 != NULL)
    {
      success = klass->process (operation, input, aux, aux2, output, result, level);

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

static gboolean gegl_operation_point_composer3_process
                              (GeglOperation       *operation,
                               GeglBuffer          *input,
                               GeglBuffer          *aux,
                               GeglBuffer          *aux2,
                               GeglBuffer          *output,
                               const GeglRectangle *result,
                               gint                 level);

G_DEFINE_TYPE (GeglOperationPointComposer3, gegl_operation_point_composer3, GEGL_TYPE_OPERATION_COMPOSER3)

static gboolean
gegl_operation_point_composer3_cl_process (GeglOperation       *operation,
                                           GeglBuffer          *input,
                                           GeglBuffer          *aux,
                                           GeglBuffer          *aux2,
                                           GeglBuffer          *output,
                                           const GeglRectangle *result,
                                           gint                 level)
{
  GeglOperationClass               *operation_class       = GEGL_OPERATION_GET_CLASS (operation);
  GeglOperationPointComposer3Class *point_composer3_class = GEGL_OPERATION_POINT_COMPOSER3_GET_CLASS (operation);
  const Babl           *in_format  = NULL;
  const Babl           *out_format = NULL;
  GeglBufferClIterator *iter       = NULL;
  gboolean err;

  GEGL_NOTE (GEGL_DEBUG_OPENCL, "GEGL_OPERATION_POINT_COMPOSER3: %s", operation_class->name);

  in_format  = gegl_operation_get_format (operation, "input");
  out_format = gegl_operation_get_format (operation, "output");

  iter = gegl_buffer_cl_iterator_new (output, result, out_format, GEGL_ACCESS_WRITE);
  gegl_buffer_cl_iterator_add (iter, input, result, in_format, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  if (aux)
    {
      const Babl *aux_format = gegl_operation_get_format (operation, "aux");
      gegl_buffer_cl_iterator_add (iter, aux, result, aux_format, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
    }

  if (aux2)
    {
      const Babl *aux2_format = gegl_operation_get_format (operation, "aux2");
      gegl_buffer_cl_iterator_add (iter, aux2, result, aux2_format, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
    }

  while (gegl_buffer_cl_iterator_next (iter, &err))
    {
      if (point_composer3_class->cl_process)
        {
          err = point_composer3_class->cl_process (operation,
                                                   iter->tex[1],
                                                   (aux)  ? iter->tex[2] : NULL,
                                                   (aux2) ? iter->tex[3] : NULL,
                                                   iter->tex[0],
                                                   iter->size[0],
                                                   &iter->roi[0],
                                                   level);
          if (err)
            {
              GEGL_NOTE (GEGL_DEBUG_OPENCL, "Error: %s", operation_class->name);
              gegl_buffer_cl_iterator_stop (iter);
              return FALSE;
            }
        }
      else
        {
          g_warning ("OpenCL support enabled, but no way to execute");
          gegl_buffer_cl_iterator_stop (iter);
          return FALSE;
        }
    }

  return TRUE;
}

static void prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  const Babl *format;
  if (!space)
    space = gegl_operation_get_source_space (operation, "aux");
  if (!space)
    space = gegl_operation_get_source_space (operation, "aux2");

  format  = babl_format_with_space ("RGBA float", space);

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "aux", format);
  gegl_operation_set_format (operation, "aux2", format);
  gegl_operation_set_format (operation, "output", format);
}

static void
gegl_operation_point_composer3_class_init (GeglOperationPointComposer3Class *klass)
{
  GeglOperationClass          *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationComposer3Class *composer_class  = GEGL_OPERATION_COMPOSER3_CLASS (klass);

  composer_class->process = gegl_operation_point_composer3_process;
  operation_class->process = gegl_operation_composer3_process;
  operation_class->prepare = prepare;
  operation_class->want_in_place = TRUE;
  operation_class->threaded = TRUE;
}

static void
gegl_operation_point_composer3_init (GeglOperationPointComposer3 *self)
{

}

static gboolean
gegl_operation_point_composer3_process (GeglOperation       *operation,
                                        GeglBuffer          *input,
                                        GeglBuffer          *aux,
                                        GeglBuffer          *aux2,
                                        GeglBuffer          *output,
                                        const GeglRectangle *result,
                                        gint                 level)
{
  GeglOperationPointComposer3Class *point_composer3_class = GEGL_OPERATION_POINT_COMPOSER3_GET_CLASS (operation);
  const Babl *in_format   = gegl_operation_get_format (operation, "input");
  const Babl *aux_format  = gegl_operation_get_format (operation, "aux");
  const Babl *aux2_format = gegl_operation_get_format (operation, "aux2");
  const Babl *out_format  = gegl_operation_get_format (operation, "output");

  GeglRectangle scaled_result = *result;
  if (level)
  {
    scaled_result.x >>= level;
    scaled_result.y >>= level;
    scaled_result.width >>= level;
    scaled_result.height >>= level;
    result = &scaled_result;
  }

  if ((result->width > 0) && (result->height > 0))
    {
      if (gegl_operation_use_opencl (operation) && point_composer3_class->cl_process)
        {
          if (gegl_operation_point_composer3_cl_process (operation, input, aux, aux2, output, result, level))
              return TRUE;
        }

      if (gegl_operation_use_threading (operation, result))
      {
        ThreadData data;

        data.klass = point_composer3_class;
        data.operation = operation;
        data.input = input;
        data.aux = aux;
        data.aux2 = aux2;
        data.output = output;
        data.level = level;
        data.input_format = in_format;
        data.aux_format = aux_format;
        data.aux2_format = aux2_format;
        data.output_format = out_format;

        if (gegl_cl_is_accelerated ())
        {
          if (input)
            gegl_buffer_flush_ext (input, result);
          if (aux)
            gegl_buffer_flush_ext (aux, result);
          if (aux2)
            gegl_buffer_flush_ext (aux2, result);
        }

        gegl_parallel_distribute_area (
          result,
          gegl_operation_get_pixels_per_thread (operation),
          GEGL_SPLIT_STRATEGY_AUTO,
          (GeglParallelDistributeAreaFunc) thread_process,
          &data);

        return TRUE;
      }
      else
      {
        GeglBufferIterator *i = gegl_buffer_iterator_new (output, result, level, out_format,
                                                          GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 4);
        gint foo = 0, bar = 0, read = 0;

        if (input)
          read = gegl_buffer_iterator_add (i, input, result, level, in_format,
                                           GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
        if (aux)
          foo = gegl_buffer_iterator_add (i, aux, result, level, aux_format,
                                          GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
        if (aux2)
          bar = gegl_buffer_iterator_add (i, aux2, result, level, aux2_format,
                                          GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

        while (gegl_buffer_iterator_next (i))
          {
            point_composer3_class->process (operation, input?i->items[read].data:NULL,
                                                       aux?i->items[foo].data:NULL,
                                                       aux2?i->items[bar].data:NULL,
                                                       i->items[0].data, i->length, &(i->items[0].roi), level);
          }
        return TRUE;
      }
    }
  return TRUE;
}
