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
#define GEGL_OP_NAME     grey
#define GEGL_OP_C_SOURCE grey.c

#include "gegl-op.h"
#include <string.h>

static void prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  const Babl *format;
  const Babl *input_format;

  input_format = gegl_operation_get_source_format (operation, "input");

  if (input_format && babl_format_has_alpha (input_format))
    format = babl_format_with_space ("YA float", space);
  else
    format = babl_format_with_space ("Y float", space);

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  const Babl *output_format;
  gint n_components;

  output_format = gegl_operation_get_format (op, "output");
  g_return_val_if_fail (output_format != NULL, FALSE);

  n_components = babl_format_get_n_components (output_format);
  memmove (out_buf, in_buf, sizeof (gfloat) * n_components * samples);

  return TRUE;
}

#include "opencl/gegl-cl.h"

static gboolean
cl_process (GeglOperation       *op,
            cl_mem               in_tex,
            cl_mem               out_tex,
            size_t               global_worksize,
            const GeglRectangle *roi,
            gint                 level)
{
  const Babl *output_format;
  cl_int cl_err = 0;
  gint n_components;
  gsize bytes_per_pixel;

  output_format = gegl_operation_get_format (op, "output");
  g_return_val_if_fail (output_format != NULL, TRUE);

  n_components = babl_format_get_n_components (output_format);
  switch (n_components)
    {
    case 1:
      bytes_per_pixel = sizeof (cl_float);
      break;

    case 2:
      bytes_per_pixel = sizeof (cl_float2);
      break;

    default:
      g_return_val_if_reached (TRUE);
      break;
    }

  cl_err = gegl_clEnqueueCopyBuffer(gegl_cl_get_command_queue(),
                                    in_tex , out_tex , 0 , 0 ,
                                    global_worksize * bytes_per_pixel,
                                    0, NULL, NULL);
  CL_CHECK;

  return FALSE;

error:
  return TRUE;
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  point_filter_class->process = process;
  point_filter_class->cl_process = cl_process;
  operation_class->prepare = prepare;

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
      "name",        "gegl:gray",
      "compat-name", "gegl:grey",
      "title",       _("Make Gray"),
      "categories" , "grayscale:color",
      "reference-hash", "43ddd80572ab34095298ac7c36368b0c",
      "description", _("Turns the image grayscale"),
      NULL);
}

#endif
