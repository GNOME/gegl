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
 * Copyright 2017 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_double (ratio, _("Ratio"), 0.5)
    description (_("Mixing ratio, read as amount of aux, 0=input 0.5=half 1.0=aux"))

#else

#define GEGL_OP_POINT_COMPOSER
#define GEGL_OP_NAME     mix
#define GEGL_OP_C_SOURCE mix.c

#include "gegl-op.h"

static void prepare (GeglOperation *operation)
{
  const Babl *format = gegl_operation_get_source_format (operation, "input");
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  BablModelFlag flag = babl_get_model_flags (format);

  if (flag & BABL_MODEL_FLAG_CMYK)
   format = babl_format_with_space ("cmykA float", space);
  else if (flag & BABL_MODEL_FLAG_GRAY)
   format = babl_format_with_space ("YA float", space);
  else
   format = babl_format_with_space ("RGBA float", space);

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "aux", format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *aux_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (op);
  gfloat * GEGL_ALIGNED in = in_buf;
  gfloat * GEGL_ALIGNED aux = aux_buf;
  gfloat * GEGL_ALIGNED out = out_buf;
  gfloat r = o->ratio;
  gfloat rr = 1.0 - o->ratio;
  const Babl *format = gegl_operation_get_format (op, "output");
  int components = babl_format_get_n_components (format);

  if (aux==NULL)
  {
     while (n_pixels--)
      {
        for (int c = 0; c < components; c++)
          out[c] = in[c];
        in  += components;
        aux += components;
        out += components;
      }

    return TRUE;
  }

  while (n_pixels--)
    {
      for (int c = 0; c < components; c++)
        out[c] = aux[c] * r + in[c] * rr;

      in  += components;
      aux += components;
      out += components;
    }
  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass              *operation_class;
  GeglOperationPointComposerClass *point_composer_class;

  operation_class      = GEGL_OPERATION_CLASS (klass);
  point_composer_class = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);
  operation_class->prepare = prepare;

  point_composer_class->process    = process;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:mix",
    "title",       _("Mix"),
    "categories", "compositors:blend",
    "reference-hash", "20c678baa5b1f5c72692ab9dce6a5951",
    "description",
          _("Do a lerp, linear interpolation (lerp) between input and aux"),
    NULL);
}

#endif
