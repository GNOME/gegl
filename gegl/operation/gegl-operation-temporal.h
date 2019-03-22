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

/* GeglOperationTemporal
 * Base class for operations that want access to previous frames in a video sequence,
 * it contains API to configure the amounts of frames to store as well as getting a
 * GeglBuffer pointing to any of the previously stored frames.
 */

#ifndef __GEGL_OPERATION_TEMPORAL_H__
#define __GEGL_OPERATION_TEMPORAL_H__

#include "gegl-operation-filter.h"

G_BEGIN_DECLS

#define GEGL_TYPE_OPERATION_TEMPORAL            (gegl_operation_temporal_get_type ())
#define GEGL_OPERATION_TEMPORAL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_TEMPORAL, GeglOperationTemporal))
#define GEGL_OPERATION_TEMPORAL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_TEMPORAL, GeglOperationTemporalClass))
#define GEGL_IS_OPERATION_TEMPORAL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_TEMPORAL))
#define GEGL_IS_OPERATION_TEMPORAL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_TEMPORAL))
#define GEGL_OPERATION_TEMPORAL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_TEMPORAL, GeglOperationTemporalClass))

typedef struct _GeglOperationTemporal         GeglOperationTemporal;
typedef struct _GeglOperationTemporalPrivate  GeglOperationTemporalPrivate;
struct _GeglOperationTemporal
{
  GeglOperationFilter parent_instance;
  GeglOperationTemporalPrivate *priv;
};

typedef struct _GeglOperationTemporalClass GeglOperationTemporalClass;
struct _GeglOperationTemporalClass
{
  GeglOperationFilterClass parent_class;
  gboolean (* process)  (GeglOperation       *self,
                         GeglBuffer          *input,
                         GeglBuffer          *output,
                         const GeglRectangle *roi,
                         gint                 level);
  gboolean (* process2) (GeglOperation       *self,
                         GeglBuffer          *input,
                         GeglBuffer          *output,
                         const GeglRectangle *roi,
                         gint                 level,
                         GError             **error);
  gpointer               pad[4];
};

GType gegl_operation_temporal_get_type (void) G_GNUC_CONST;

void gegl_operation_temporal_set_history_length (GeglOperation *op,
                                                 gint           history_length);

guint gegl_operation_temporal_get_history_length (GeglOperation *op);

/* you need to unref the buffer when you're done with it */
GeglBuffer *gegl_operation_temporal_get_frame (GeglOperation *op,
                                               gint           frame);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GeglOperationTemporal, g_object_unref)

G_END_DECLS

#endif
