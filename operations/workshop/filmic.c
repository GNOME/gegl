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

   /* no properties */

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     filmic
#define GEGL_OP_C_SOURCE filmic.c

#include "gegl-op.h"

static inline float aces_filmic (float x)
{
 /* source of approximation:

    https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
    https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
  */
 float a = x * (x + 0.0245786f) - 0.000090537f;
 float b = x * (0.983729f * x + 0.4329510f) + 0.238081f;
 return a / b;
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  gfloat *in  = in_buf;
  gfloat *out = out_buf;

  while (samples--)
    {
      out[0] = aces_filmic (in[0]);
      out[1] = aces_filmic (in[1]);
      out[2] = aces_filmic (in[2]);
      out[3] = in[3];

      in += 4;
      out+= 4;
    }
  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  point_filter_class->process  = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:filmic",
    "title",       _("ACES Filmic"),
    "categories" , "color:tonemapping",
    "description",
       _("HDR to SDR proofing filter/mapping curve that is an approximation of the ACES filmic curve, useful for consistent previewing of content in near HDR range.").
    NULL);
}

#endif
