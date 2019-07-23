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

  property_double (inklimit, "Inklevel warn", 250.0)
  value_range (0.0, 400.0)

  property_double (amount, "amount", 100.0)
  value_range (0.0, 100.0)


#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     gcr
#define GEGL_OP_C_SOURCE gcr.c

#include "gegl-op.h"
#include <stdio.h>

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (op);

  glong   i;
  float *in  = in_buf;
  float *out = out_buf;

  for (i=0; i<samples; i++)
    {
      float C = in[0];
      float M = in[1];
      float Y = in[2];
      float K = in[3];
#if 1
      float mincol = C;
      float pullout;

      if (M < mincol) mincol = M;
      if (Y < mincol) mincol = Y;

      pullout = mincol * o->amount / 100.0;

      C = (C - pullout) / (1.0 - pullout);
      M = (M - pullout) / (1.0 - pullout);
      Y = (Y - pullout) / (1.0 - pullout);

      K = 1.0- ((1.0 - K) * (1.0- pullout));

      if (C + M + Y + K > o->inklimit / 100.0)
      {
        C = 0; Y= 0; M = 1.0; K=0.0;
      }
#endif

      out[0] = C;
      out[1] = M;
      out[2] = Y;
      out[3] = K;
      out[4] = in[4];

      in  += 5;
      out += 5;
    }
  return TRUE;
}

static void prepare (GeglOperation *operation)
{
  const Babl *format = gegl_operation_get_source_format (operation, "input");
  format = babl_format_with_space ("CMYKA float", format);
  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->prepare = prepare;
  point_filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name"        , "gegl:gray-component-replacement",
    "categories"  , "color",
    "title"       , "Gray Component Replacement",
    "description" , "Reduces total ink-coverage by transferring color from CMY to K component",
    NULL);
}

#endif
