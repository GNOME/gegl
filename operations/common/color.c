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

property_color (value, _("Color"), "black")
    /* TRANSLATORS: the string 'black' should not be translated */
    description (_("The color to render (defaults to 'black')"))
    ui_meta     ("role", "color-primary")

property_format (format, _("Babl Format"), 666)
    description (_("The babl format of the output"))

#else

#define GEGL_OP_POINT_RENDER
#define GEGL_OP_NAME     color
#define GEGL_OP_C_SOURCE color.c

#include "gegl-op.h"

static void
gegl_color_op_prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  if (o->format)
    gegl_operation_set_format (operation, "output", o->format);
  else
    gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static GeglRectangle
gegl_color_op_get_bounding_box (GeglOperation *operation)
{
  return gegl_rectangle_infinite_plane ();
}

static gboolean
gegl_color_op_process (GeglOperation       *operation,
                       GeglBuffer          *output,
                       const GeglRectangle *roi,
                       gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  gegl_buffer_set_color (output, roi, o->value);

  return TRUE;
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process             = gegl_color_op_process;
  operation_class->get_bounding_box = gegl_color_op_get_bounding_box;
  operation_class->prepare          = gegl_color_op_prepare;
  operation_class->threaded         = FALSE;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:color",
    "title",      _("Color"),
    "categories" , "render",
    "reference-hash", "fd519ccc1b0badb3ff41501112ca3463",
    "description",
      _("Generates a buffer entirely filled with the specified color, "
        "use gegl:crop to get smaller dimensions."),
    NULL);
}

#endif
