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
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_string (space_name, _("Name"), "sRGB")
   description (_("One of: sRGB, Adobish, Rec2020, ProPhoto, Apple, ACEScg, ACES2065-1"))
property_format (pointer, _("Pointer"), NULL)
   description (_("pointer to a const * Babl space"))
property_file_path (path, _("Path"), "")
  description (_("File system path to ICC matrix profile to load"))

#else

#define GEGL_OP_COMPOSER
#define GEGL_OP_NAME     cast_space
#define GEGL_OP_C_SOURCE cast-space.c

#include "gegl-op.h"
#include <stdio.h>

static void
prepare (GeglOperation *operation)
{
  const Babl *in_format = gegl_operation_get_source_format (operation,
                                                            "input");
  const Babl *aux_format = gegl_operation_get_source_format (operation,
                                                            "aux");
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *space = babl_space (o->space_name);
  if (o->pointer)
    space = o->pointer;
  if (o->path && o->path[0])
  {
    gchar *icc_data = NULL;
    gsize icc_length;
    g_file_get_contents (o->path, &icc_data, &icc_length, NULL);
    if (icc_data)
    {
      const char *error = NULL;
      const Babl *s = babl_space_from_icc (icc_data, (gint)icc_length,
                                 BABL_ICC_INTENT_DEFAULT, &error);
      if (s) space = s;
      g_free (icc_data);
    }
  }
  if (aux_format)
  {
    space = babl_format_get_space (aux_format);
  }

  const char *old_encoding = babl_format_get_encoding (in_format);

  gegl_operation_set_format (operation, "input",
                             babl_format_with_space (old_encoding, in_format));
  gegl_operation_set_format (operation, "output",
                             babl_format_with_space (old_encoding, space));
}

static gboolean
process (GeglOperation        *operation,
         GeglOperationContext *context,
         const gchar          *output_prop,
         const GeglRectangle  *roi,
         gint                  level)
{
  const Babl *in_format = gegl_operation_get_format (operation, "input");
  const Babl *out_format = gegl_operation_get_format (operation, "output");
  GeglBuffer *input;
  GeglBuffer *output;

  if (strcmp (output_prop, "output"))
    {
      g_warning ("cast-format: requested processing of %s pad", output_prop);
      return FALSE;
    }

  input = (GeglBuffer*) gegl_operation_context_dup_object (context, "input");
  if (! input)
    {
      g_warning ("cast: received NULL input");
      return FALSE;
    }

  output = gegl_buffer_new (roi, in_format);

  gegl_buffer_copy (input, roi, GEGL_ABYSS_NONE, output, roi);

  gegl_buffer_set_format (output, out_format);

  g_object_unref (input);

  gegl_operation_context_take_object (context, "output", G_OBJECT (output));

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->prepare = prepare;
  operation_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name",       "gegl:cast-space",
    "title",      _("Cast color space"),
    "categories", "core:color",
    "description", _("Override the specified color space setting a pointer to a format override the string property and setting an aux pad overrides both. "),
    NULL);
}

#endif
