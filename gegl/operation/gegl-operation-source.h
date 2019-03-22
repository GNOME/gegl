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

/* GeglOperationSource
 * Operations used as render sources or file loaders, the process method receives a
 * GeglBuffer to write it’s output into
 */

#ifndef __GEGL_OPERATION_SOURCE_H__
#define __GEGL_OPERATION_SOURCE_H__

#include "gegl-operation.h"

G_BEGIN_DECLS

#define GEGL_TYPE_OPERATION_SOURCE            (gegl_operation_source_get_type ())
#define GEGL_OPERATION_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_SOURCE, GeglOperationSource))
#define GEGL_OPERATION_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_SOURCE, GeglOperationSourceClass))
#define GEGL_IS_OPERATION_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_SOURCE))
#define GEGL_IS_OPERATION_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_SOURCE))
#define GEGL_OPERATION_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_SOURCE, GeglOperationSourceClass))

typedef struct _GeglOperationSource  GeglOperationSource;
struct _GeglOperationSource
{
  GeglOperation parent_instance;
};

typedef struct _GeglOperationSourceClass GeglOperationSourceClass;
struct _GeglOperationSourceClass
{
  GeglOperationClass     parent_class;

  gboolean (* process)  (GeglOperation       *self,
                         GeglBuffer          *output,
                         const GeglRectangle *roi,
                         gint                 level);
  gboolean (* process2) (GeglOperation       *self,
                         GeglBuffer          *output,
                         const GeglRectangle *roi,
                         gint                 level,
                         GError             **error);
  gpointer               pad[4];
};

GType gegl_operation_source_get_type (void) G_GNUC_CONST;

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GeglOperationSource, g_object_unref)

G_END_DECLS

#endif
