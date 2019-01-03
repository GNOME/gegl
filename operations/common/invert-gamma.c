/* This file is an image processing operation for GEGL
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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

   /* no properties */

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     invert_gamma
#define GEGL_OP_C_SOURCE invert-gamma.c

#include "gegl-op.h"

#define INVERT_GAMMA "'"
#include "invert-common.h"

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->prepare     = prepare;
  point_filter_class->process  = process;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:invert-gamma",
    "title",      _("Invert in Perceptual space"),
    "categories" , "color",
    "reference-hash", "db07b9d85f2786db29560bd50ae0e7a1",
    "description",
       _("Invert the components (except alpha) perceptually, "
         "the result is the corresponding \"negative\" image."),
    NULL);
}

#endif
