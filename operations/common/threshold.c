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

property_double (value, _("Threshold"), 0.5) // TODO : rename low - to match GIMP op
    value_range (-200, 200)
    ui_range    (-1, 2)
    description(_("Lowest value to be included."))

property_double (high, _("High"), 1.0)
    value_range (-200, 200)
    ui_range    (0, 1)
    description(_("Highest value to be included as white."))

#else

#define GEGL_OP_POINT_COMPOSER
#define GEGL_OP_NAME     threshold
#define GEGL_OP_C_SOURCE threshold.c

#include "gegl-op.h"

static void prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  gegl_operation_set_format (operation, "input",  babl_format_with_space ("Y'A float", space));
  gegl_operation_set_format (operation, "aux",    babl_format_with_space ("Y' float", space));
  gegl_operation_set_format (operation, "output", babl_format_with_space ("Y'A float", space));
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
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  gfloat *aux = aux_buf;
  glong   i;
  gfloat level_low_p  = GEGL_PROPERTIES (op)->value;
  gfloat level_high_p = GEGL_PROPERTIES (op)->high;

  if (aux == NULL)
    {
      for (i=0; i<n_pixels; i++)
        {
          gfloat c;

          c = in[0];
          c = (c>=level_low_p && c <= level_high_p)
              
              ?1.0:0.0;
          out[0] = c;

          out[1] = in[1];
          in  += 2;
          out += 2;
        }
    }
  else
    {
      for (i=0; i<n_pixels; i++)
        {
          gfloat level_gray = *aux;
          gfloat c;

          gfloat level_low = level_gray;
          gfloat level_high = level_gray;

          if (level_low_p <= 0.5)
          {
            gfloat t = 1.0-((level_low_p)/0.5);
            level_low = 0.0 * t + level_low * (1.0-t);
          }
          else
          {
            gfloat t = 1.0-((1.0-level_low_p)/0.5);
            level_low = 1.0* t + level_low * (1.0-t);
          }
          if (level_high_p <= 0.5)
          {
            gfloat t = 1.0-((level_high_p)/0.5);
            level_high = 0.0 * t + level_high * (1.0-t);
          }
          else
          {
            gfloat t = 1.0-((1.0-level_high_p)/0.5);
            level_high = 1.0* t + level_high * (1.0-t);
          }

          c = in[0];
          c = (c>=level_low && c <= level_high)?1.0:0.0;
          out[0] = c;

          out[1] = in[1];
          in  += 2;
          out += 2;
          aux += 1;
        }
    }
  return TRUE;
}

#include "opencl/threshold.cl.h"

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass              *operation_class;
  GeglOperationPointComposerClass *point_composer_class;
  gchar                            *composition =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "  <node operation='gegl:crop' width='200' height='200'/>"
    "  <node operation='gegl:over'>"
    "    <node operation='gegl:threshold'>"
    "      <params>"
    "        <param name='value'>0.5</param>"
    "        <param name='high'>1.0</param>"
    "      </params>"
    "    </node>"
    "    <node operation='gegl:load' path='standard-input.png'/>"
    "  </node>"
    "  <node operation='gegl:checkerboard'>"
    "    <params>"
    "      <param name='color1'>rgb(0.25,0.25,0.25)</param>"
    "      <param name='color2'>rgb(0.75,0.75,0.75)</param>"
    "    </params>"
    "  </node>"    
    "</gegl>";

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_composer_class = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  point_composer_class->process = process;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
    "name" ,       "gegl:threshold",
    "title",       _("Threshold"),
    "categories" , "color",
    "reference-hash", "17f9861344e1105c15f3633f7312a9bd",
    "reference-composition", composition,
    "description",
          _("Thresholds the image to white/black based on either the global values "
            "set in the value (low) and high properties, or per pixel from the aux input."),
    "cl-source",   threshold_cl_source,
    NULL);
}

#endif
