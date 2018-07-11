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
 * Copyright 2017 Elle Stone <ellestone@ninedegreesbelow.com>
 *                Michael Natterer <mitch@gimp.org>
 *                Miroslav Talasek <miroslav.talasek@seznam.cz>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (hue_sel_center, _("Hue selection center"),  180.0)
   description  (_("Center of Hue selection interval  "))
   value_range  (0.0, 360.0)

property_double (hue_sel_width, _("Hue selection width"), 50.0)
   description  (_("Width of Hue selection interval  "))
   value_range  (0.0, 360.0)

property_double (hue, _("Hue"),  0.0)
   description  (_("Hue adjustment"))
   value_range  (-180.0, 180.0)

property_double (saturation, _("Saturation"), 0.0)
   description  (_("Saturation adjustment"))
   value_range  (-100.0, 100.0)

property_double (lightness, _("Lightness"), 0.0)
   description  (_("Lightness adjustment"))
   value_range  (-100.0, 100.0)

#else

#define GEGL_OP_POINT_FILTER

#define GEGL_OP_NAME     selective_hue_saturation
#define GEGL_OP_C_SOURCE selective-hue-saturation.c

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",
                             babl_format ("HSLA float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("HSLA float"));
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (op);
  gfloat         *GEGL_ALIGNED in_pixel;
  gfloat         *GEGL_ALIGNED out_pixel;
  gfloat          hue;
  gfloat          saturation;
  gfloat          lightness;
  gfloat fromHue1 = 0, toHue1 = 0, fromHue2 = 0, toHue2 = 0;
  gint splitHue = 0;

  in_pixel   = in_buf;
  out_pixel  = out_buf;

  hue       = o->hue / 180.0;
  saturation = o->saturation / 100.0;
  lightness = o->lightness / 100.0 ;

  if ((o->hue_sel_center - o->hue_sel_width / 2.0) < 0.0)
  {
    fromHue1 = 0.0;
    fromHue2 = ((o->hue_sel_center - o->hue_sel_width / 2.0) + 360.0) / 360.0;
    toHue2 = 1.0;
    splitHue = 1;
  }
  else
  {
    fromHue1 = (o->hue_sel_center - o->hue_sel_width / 2.0) / 360.0;
  }

  if ((o->hue_sel_center + o->hue_sel_width / 2.0) > 360.0)
  {
    toHue1 = 1.0;
    fromHue2 = 0;
    toHue2 = ((o->hue_sel_center + o->hue_sel_width / 2.0) - 360.0) / 360.0;
    splitHue = 1;
  }
  else
  {
    toHue1 = (o->hue_sel_center + o->hue_sel_width / 2.0) / 360.0;
  }
/*
  printf("f1:%f t1:%f\n",fromHue1,toHue1);
  if (splitHue){
    printf("f2:%f t2:%f\n",fromHue2,toHue2);
  }
*/
  while (n_pixels--)
    {

    if ((in_pixel[0] >= fromHue1 && in_pixel[0] <= toHue1)
        || (splitHue == 1 && in_pixel[0] >= fromHue2 && in_pixel[0] <= toHue2) )
      {
        out_pixel[0] = in_pixel[0] + hue;
        out_pixel[1] = in_pixel[1] + saturation;
        out_pixel[2] = in_pixel[2] + lightness;

        out_pixel[1] = CLAMP (out_pixel[1], 0, 1.0);
        out_pixel[2] = CLAMP (out_pixel[2], 0, 1.0);
        if (out_pixel[0] < 0)
          {
            out_pixel[0] = out_pixel[0] + 1.0;
          }
        if (out_pixel[0] > 1.0)
          {
            out_pixel[0] = out_pixel[0] - 1.0;
          }
      }
    else
      {
        out_pixel[0] = in_pixel[0];
        out_pixel[1] = in_pixel[1];
        out_pixel[2] = in_pixel[2];
      }

    out_pixel[3] = in_pixel[3];
    in_pixel  += 4;
    out_pixel += 4;

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

  operation_class->prepare    = prepare;
  point_filter_class->process = process;

  operation_class->opencl_support = FALSE;

  gegl_operation_class_set_keys (operation_class,
      "name",           "gegl:selective-hue-saturation",
      "title",          _("Selective Hue-Saturation"),
      "categories",     "color",
      "reference-hash", "ffb9e86edb25bc92e8d4e68f59bbb04b",
      "description",    _("Selective adjust Hue, Saturation and Lightness"),
      NULL);
}

#endif
