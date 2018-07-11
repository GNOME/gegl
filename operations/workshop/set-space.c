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

property_string (space, _("Space"), "sRGB")
   description (_("space to assign, using babls names"))
property_format (babl_space, _("Babl space"), NULL)
   description (_("pointer to a babl space"))
property_file_path (icc_path, _("ICC path"), "")
  description (_("Path to ICC matrix profile to load"))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME            set_space
#define GEGL_OP_C_SOURCE        set-space.c

#include "gegl-op.h"
#include <stdio.h>

static void
gegl_set_space_prepare (GeglOperation *operation)
{
  const Babl *aux_format = gegl_operation_get_source_format (operation,
                                                            "aux");
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *space = babl_space (o->space);
  if (o->babl_space)
    space = o->babl_space;
  if (o->icc_path)
  {
    gchar *icc_data = NULL;
    gsize icc_length;
    g_file_get_contents (o->icc_path, &icc_data, &icc_length, NULL);
    if (icc_data)
    {
      const char *error = NULL;
      const Babl *s = babl_icc_make_space ((void*)icc_data, icc_length,
                                 BABL_ICC_INTENT_RELATIVE_COLORIMETRIC, &error);
      if (s) space = s;
      g_free (icc_data);
    }
  }
  if (aux_format)
  {
    space = babl_format_get_space (aux_format);
  }
  if (!space)
  {
    fprintf (stderr, "unknown space %s\n", o->space);
  }

  gegl_operation_set_format (operation, "output",
                             babl_format_with_space ("RGBA float", space));
}

static gboolean
gegl_set_space_process (GeglOperation        *operation,
                        GeglOperationContext *context,
                        const gchar          *output_prop,
                        const GeglRectangle  *result,
                        gint                  level)
{
  GeglBuffer *input;

  input = GEGL_BUFFER (gegl_operation_context_get_object (context, "input"));
  if (!input)
    {
      return FALSE;
    }

  gegl_operation_context_take_object (context, "output",
                                      g_object_ref ((GObject *) input));
  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  operation_class->process = gegl_set_space_process;
  operation_class->prepare = gegl_set_space_prepare;

  gegl_operation_class_set_keys (operation_class,
              "name",        "gegl:set-space",
              "title",       _("Set space"),
              "categories",  "core",
              "description", _("set color space, does not do a conversion but changes the space which subsequent formats in the pipeline are created with."),
              NULL);
}

#endif
