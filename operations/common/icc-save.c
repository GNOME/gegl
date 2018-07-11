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
 * Copyright 2018 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
  description (_("Target path and filename"))

#else

#define GEGL_OP_SINK
#define GEGL_OP_NAME     icc_save
#define GEGL_OP_C_SOURCE icc-save.c

#include <gegl-op.h>
#include <gegl-gio-private.h>


static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         const GeglRectangle *result,
         int                  level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *space = babl_format_get_space (gegl_buffer_get_format (input));
  int icc_len;
  const char *name = babl_get_name (space);
  const char *icc_profile;
  if (strlen (name) > 10) name = "babl/GEGL";
  icc_profile = babl_space_get_icc (space, &icc_len);
  if (icc_profile)
  {
    g_file_set_contents (o->path, icc_profile, icc_len, NULL);
  }

  return  TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass     *operation_class;
  GeglOperationSinkClass *sink_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  sink_class->process    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",          "gegl:icc-save",
    "title",       _("ICC profile saver"),
    "categories",    "output",
    "description", _("Stores the ICC profile that would be embedded if stored as an image."),
    NULL);

  gegl_operation_handlers_register_saver (
    ".icc", "gegl:icc-save");
}

#endif
