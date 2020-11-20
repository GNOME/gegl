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
 * Copyright 2020 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
  description (_("Path of file to load"))

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_NAME icc_load
#define GEGL_OP_C_SOURCE icc-load.c

#include "gegl-op.h"

static void
gegl_icc_load_prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gchar *icc_data = NULL;
  gsize  icc_length;
  g_file_get_contents (o->path, &icc_data, &icc_length, NULL);
  if (icc_data)
  {
    const char *error = NULL;
    const Babl *space = babl_space_from_icc ((char*)icc_data, (int)icc_length,
                             BABL_ICC_INTENT_RELATIVE_COLORIMETRIC,
                             &error);
    if (space)
    {
      const Babl *format = NULL;
      if (babl_space_is_gray (space))
        format = babl_format_with_space ("Y float", space);
      else if (babl_space_is_cmyk (space))
        format = babl_format_with_space ("CMYK float", space);
      else {
        format = babl_format_with_space ("RGB float", space);
      }
      if (format)
        gegl_operation_set_format (operation, "output", format);
    }
  }
}

static GeglRectangle
gegl_icc_load_get_bounding_box (GeglOperation *operation)
{
  return (GeglRectangle) {0, 0, 1, 1};
}

static gboolean
gegl_icc_load_process (GeglOperation       *operation,
                       GeglBuffer          *output,
                       const GeglRectangle *result,
                       gint                 level)
{
  /* no real processing */
  return 0;
}

static GeglRectangle
gegl_icc_load_get_cached_region (GeglOperation       *operation,
                                 const GeglRectangle *roi)
{
  return gegl_icc_load_get_bounding_box (operation);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = gegl_icc_load_process;
  operation_class->prepare  = gegl_icc_load_prepare;
  operation_class->get_bounding_box = gegl_icc_load_get_bounding_box;
  operation_class->get_cached_region = gegl_icc_load_get_cached_region;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:icc-load",
    "title",       _("ICC File Loader"),
    "categories",  "hidden",
    "description", _("ICC profile loader."),
    NULL);

  gegl_operation_handlers_register_loader (
    "application/vnd.iccprofile", "gegl:icc-load");
  gegl_operation_handlers_register_loader (
    ".icc", "gegl:icc-load");
}
#endif
