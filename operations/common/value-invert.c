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
 * Copyright 2007 Mukund Sivaraman <muks@mukund.org>
 */

/* XXX
 * This plug-in isn't really useful apart for compatibility with GIMP
 * scripts, operating with HSV as a color model for filters isn't really
 * useful.
 */

/*
 * The plug-in only does v = 1.0 - v; for each pixel in the image, or
 * each entry in the colormap depending upon the type of image, where 'v'
 * is the value in HSV color model.
 *
 * The plug-in code is optimized towards this, in that it is not a full
 * RGB->HSV->RGB transform, but shortcuts many of the calculations to
 * effectively only do v = 1.0 - v. In fact, hue is never calculated. The
 * shortcuts can be derived from running a set of r, g, b values through the
 * RGB->HSV transform and then from HSV->RGB and solving out the redundant
 * portions.
 *
 */


#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

   /* no properties */

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     value_invert
#define GEGL_OP_C_SOURCE value-invert.c

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  gegl_operation_set_format (operation, "input", babl_format_with_space ("R'G'B'A float", space));
  gegl_operation_set_format (operation, "output", babl_format_with_space ("R'G'B'A float", space));
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  glong   j;
  gfloat *src  = in_buf;
  gfloat *dest = out_buf;
  gfloat  r, g, b;
  gfloat  value, min;
  gfloat  delta;

  for (j = 0; j < samples; j++)
    {
      r = *src++;
      g = *src++;
      b = *src++;

      if (r > g)
        {
          value = MAX (r, b);
          min = MIN (g, b);
        }
      else
        {
          value = MAX (g, b);
          min = MIN (r, b);
        }

      delta = value - min;
      if ((value == 0) || (delta == 0))
        {
          r = 1.0 - value;
          g = 1.0 - value;
          b = 1.0 - value;
        }
      else
        {
          if (r == value)
            {
              r = 1.0 - r;
              b = r * b / value;
              g = r * g / value;
            }
          else if (g == value)
            {
              g = 1.0 - g;
              r = g * r / value;
              b = g * b / value;
            }
          else
            {
              b = 1.0 - b;
              g = b * g / value;
              r = b * r / value;
            }
        }

      *dest++ = r;
      *dest++ = g;
      *dest++ = b;

      *dest++ = *src++;
    }
  return TRUE;
}

#include "opencl/value-invert.cl.h"

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  point_filter_class->process = process;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:value-invert",
    "title",       _("Value Invert"),
    "categories" , "color",
    "reference-hash", "1457b5c30de7a730a54c80028097e046",
    "reference-hashB", "98a6a7c2b289209dc7ce9309063a6796",
    "description",
        _("Invert the value component, the result has the brightness "
          "inverted, keeping the color."),
    "cl-source"  , value_invert_cl_source,
    NULL);
}

#endif

