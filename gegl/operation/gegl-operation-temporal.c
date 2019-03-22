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
 * Copyright 2008 Øyvind Kolås
 */

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl.h"
#include "gegl-operation-temporal.h"
#include "gegl-operation-context.h"

struct _GeglOperationTemporalPrivate
{
  gint                count;
  gint                history_length;

  gint                width;
  gint                height;
  gint                next_to_write;
  GeglBuffer         *frame_store;
};

static void gegl_operation_temporal_prepare (GeglOperation *operation);

G_DEFINE_TYPE_WITH_PRIVATE (GeglOperationTemporal, gegl_operation_temporal,
                            GEGL_TYPE_OPERATION_FILTER)

#define GEGL_OPERATION_TEMPORAL_GET_PRIVATE(obj) \
  ((GeglOperationTemporalPrivate *) gegl_operation_temporal_get_instance_private ((GeglOperationTemporal *) (obj)))



GeglBuffer *
gegl_operation_temporal_get_frame (GeglOperation *op,
                                   gint           frame)
{
  GeglOperationTemporal *temporal= GEGL_OPERATION_TEMPORAL (op);
  GeglOperationTemporalPrivate *priv = temporal->priv;
  GeglBuffer   *buffer;

   if (frame > priv->count)
     {
       frame = (priv->count-1);
       if (frame<0)
         frame=0;
       g_print ("%i > priv->count(%i), using frame %i", frame, priv->count,
        frame);
     }
   else
     {
       frame = (priv->next_to_write - 1 + priv->history_length + frame) % priv->history_length;
       g_print ("using frame %i", frame);
     }
  /* build in shift */
  buffer = g_object_new (GEGL_TYPE_BUFFER,
                         "source",  priv->frame_store,
                         "shift-y", frame * priv->height,
                         "width", priv->width,
                         "height", priv->height,
                         "x", 0,
                         "y", 0,
                         NULL);
  return buffer;
}

static gboolean gegl_operation_temporal_process2 (GeglOperation       *self,
                                                  GeglBuffer          *input,
                                                  GeglBuffer          *output,
                                                  const GeglRectangle *result,
                                                  gint                 level,
                                                  GError             **error)
{
  GeglOperationTemporal *temporal = GEGL_OPERATION_TEMPORAL (self);
  GeglOperationTemporalPrivate *priv = temporal->priv;
  GeglOperationTemporalClass *temporal_class;

  temporal_class = GEGL_OPERATION_TEMPORAL_GET_CLASS (self);

  priv->width  = result->width;
  priv->height = result->height;

  {
   GeglRectangle write_rect = *result;
   write_rect.y = priv->next_to_write * priv->height;

   gegl_buffer_copy (input, result, GEGL_ABYSS_NONE, priv->frame_store, &write_rect);
   priv->count++;
   priv->next_to_write++;
   if (priv->next_to_write >= priv->history_length)
     priv->next_to_write=0;
  }

 if (temporal_class->process2)
   return temporal_class->process2 (self, input, output, result, level, error);
 else
   return temporal_class->process (self, input, output, result, level);
 return FALSE;
}

static void gegl_operation_temporal_prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  const Babl *format = babl_format_with_space ("RGB u8", space);
  gegl_operation_set_format (operation, "output", format);
  gegl_operation_set_format (operation, "input", format);
}

static void
gegl_operation_temporal_class_init (GeglOperationTemporalClass *klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationFilterClass *operation_filter_class = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare = gegl_operation_temporal_prepare;
  operation_filter_class->process2 = gegl_operation_temporal_process2;
}

static void
gegl_operation_temporal_init (GeglOperationTemporal *self)
{
  GeglOperationTemporalPrivate *priv;
  GeglRectangle rect = { 0, 0, 4096, 4096*600 };

  self->priv = GEGL_OPERATION_TEMPORAL_GET_PRIVATE(self);
  priv=self->priv;
  priv->count          = 0;
  priv->history_length = 500;
  priv->width          = 1024;
  priv->height         = 1024;
  priv->next_to_write  = 0;

  /* FIXME: the format used for the frame_store should be autodetected from
   * input
   */
  priv->frame_store    =
      gegl_buffer_new (&rect, babl_format ("RGB u8"));
;
}

void gegl_operation_temporal_set_history_length (GeglOperation *op,
                                                 gint           history_length)
{
  GeglOperationTemporal *self = GEGL_OPERATION_TEMPORAL (op);
  GeglOperationTemporalPrivate *priv = self->priv;
  priv->history_length = history_length;
}

guint gegl_operation_temporal_get_history_length (GeglOperation *op)
{
  GeglOperationTemporal *self = GEGL_OPERATION_TEMPORAL (op);
  GeglOperationTemporalPrivate *priv = self->priv;
  return priv->history_length;
}
