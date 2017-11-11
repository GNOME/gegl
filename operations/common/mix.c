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
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2017 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_double (ratio, _("Ratio"), 0.5)
    description (_("mixing ratio, read as amount of aux, 0=input 0.5=half 1.0=aux"))

#else

#define GEGL_OP_POINT_COMPOSER
#define GEGL_OP_NAME     mix
#define GEGL_OP_C_SOURCE mix.c

#include "gegl-op.h"

static void prepare (GeglOperation *operation)
{
  const Babl *format;
  format = babl_format ("RGBA float");
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

  if (aux==NULL)
    return TRUE;

  while (n_pixels--)
    {
      out[0] = aux[0] * r + in[0] * rr;
      out[1] = aux[1] * r + in[1] * rr;
      out[2] = aux[2] * r + in[2] * rr;
      out[3] = aux[3] * r + in[3] * rr;
      in  += 4;
      aux += 4;
      out += 4;
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
    "reference-hash", "20c678baa5b1f5c72692ab9dce6a5951",
    "description",
          _("do a lerp, linear interpolation (lerp) between input and aux"),
    NULL);
}

#endif
