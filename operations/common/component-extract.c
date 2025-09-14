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
 * Copyright 2015 Thomas Manni <thomas.manni@free.fr>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_component_extract)
  enum_value (GEGL_COMPONENT_EXTRACT_RGB_RED, "rgb-r", N_("RGB Red"))
  enum_value (GEGL_COMPONENT_EXTRACT_RGB_GREEN, "rgb-g", N_("RGB Green"))
  enum_value (GEGL_COMPONENT_EXTRACT_RGB_BLUE, "rgb-b", N_("RGB Blue"))
  enum_value (GEGL_COMPONENT_EXTRACT_HUE, "hue", N_("Hue"))
  enum_value (GEGL_COMPONENT_EXTRACT_HSV_SATURATION, "hsv-s", N_("HSV Saturation"))
  enum_value (GEGL_COMPONENT_EXTRACT_HSV_VALUE, "hsv-v", N_("HSV Value"))
  enum_value (GEGL_COMPONENT_EXTRACT_HSL_SATURATION, "hsl-s", N_("HSL Saturation"))
  enum_value (GEGL_COMPONENT_EXTRACT_HSL_LIGHTNESS, "hsl-l", N_("HSL Lightness"))
  enum_value (GEGL_COMPONENT_EXTRACT_CMYK_CYAN, "cmyk-c", N_("CMYK Cyan"))
  enum_value (GEGL_COMPONENT_EXTRACT_CMYK_MAGENTA, "cmyk-m", N_("CMYK Magenta"))
  enum_value (GEGL_COMPONENT_EXTRACT_CMYK_YELLOW, "cmyk-y", N_("CMYK Yellow"))
  enum_value (GEGL_COMPONENT_EXTRACT_CMYK_KEY, "cmyk-k", N_("CMYK Key"))
  enum_value (GEGL_COMPONENT_EXTRACT_YCBCR_Y, "ycbcr-y", N_("Y'CbCr Y'"))
  enum_value (GEGL_COMPONENT_EXTRACT_YCBCR_CB, "ycbcr-cb", N_("Y'CbCr Cb"))
  enum_value (GEGL_COMPONENT_EXTRACT_YCBCR_CR, "ycbcr-cr", N_("Y'CbCr Cr"))
  enum_value (GEGL_COMPONENT_EXTRACT_LAB_L, "lab-l", N_("LAB L"))
  enum_value (GEGL_COMPONENT_EXTRACT_LAB_A, "lab-a", N_("LAB A"))
  enum_value (GEGL_COMPONENT_EXTRACT_LAB_B, "lab-b", N_("LAB B"))
  enum_value (GEGL_COMPONENT_EXTRACT_LCH_C, "lch-c", N_("LCH C(ab)"))
  enum_value (GEGL_COMPONENT_EXTRACT_LCH_H, "lch-h", N_("LCH H(ab)"))
  enum_value (GEGL_COMPONENT_EXTRACT_ALPHA, "alpha", N_("Alpha"))
enum_end (GeglComponentExtract)

property_enum (component, _("Component"),
               GeglComponentExtract, gegl_component_extract,
               GEGL_COMPONENT_EXTRACT_RGB_RED)
  description (_("Component to extract"))

property_boolean (invert, _("Invert component"), FALSE)
     description (_("Invert the extracted component"))

property_boolean (linear, _("Linear output"), FALSE)
     description (_("Use linear output instead of gamma corrected"))

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME component_extract
#define GEGL_OP_C_SOURCE  component-extract.c

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  GeglProperties *o             = GEGL_PROPERTIES (operation);
  const Babl     *input_format  = NULL;
  const Babl     *output_format = (o->linear ?
                                   babl_format_with_space ("Y float", space) :
                                   babl_format_with_space ("Y' float", space));

  switch (o->component)
    {
    case GEGL_COMPONENT_EXTRACT_ALPHA:
      input_format = babl_format_with_space ("YA float", space);
      break;

    case GEGL_COMPONENT_EXTRACT_RGB_RED:
    case GEGL_COMPONENT_EXTRACT_RGB_GREEN:
    case GEGL_COMPONENT_EXTRACT_RGB_BLUE:
      input_format = babl_format_with_space ("R'G'B' float", space);
      break;

    case GEGL_COMPONENT_EXTRACT_HUE:
    case GEGL_COMPONENT_EXTRACT_HSV_SATURATION:
    case GEGL_COMPONENT_EXTRACT_HSV_VALUE:
      input_format = babl_format_with_space ("HSV float", space);
      break;

    case GEGL_COMPONENT_EXTRACT_HSL_LIGHTNESS:
    case GEGL_COMPONENT_EXTRACT_HSL_SATURATION:
      input_format = babl_format_with_space ("HSL float", space);
      break;

    case GEGL_COMPONENT_EXTRACT_CMYK_CYAN:
    case GEGL_COMPONENT_EXTRACT_CMYK_MAGENTA:
    case GEGL_COMPONENT_EXTRACT_CMYK_YELLOW:
    case GEGL_COMPONENT_EXTRACT_CMYK_KEY:
      input_format = babl_format_with_space ("CMYK float", space);
      break;

    case GEGL_COMPONENT_EXTRACT_YCBCR_Y:
    case GEGL_COMPONENT_EXTRACT_YCBCR_CB:
    case GEGL_COMPONENT_EXTRACT_YCBCR_CR:
      input_format = babl_format_with_space ("Y'CbCr float", space);
      break;

    case GEGL_COMPONENT_EXTRACT_LAB_L:
    case GEGL_COMPONENT_EXTRACT_LAB_A:
    case GEGL_COMPONENT_EXTRACT_LAB_B:
      input_format = babl_format_with_space ("CIE Lab float", space);
      break;

    case GEGL_COMPONENT_EXTRACT_LCH_C:
    case GEGL_COMPONENT_EXTRACT_LCH_H:
      input_format = babl_format_with_space ("CIE LCH(ab) float", space);
      break;
    }

  gegl_operation_set_format (operation, "input", input_format);
  gegl_operation_set_format (operation, "output", output_format);
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  const Babl     *format = gegl_operation_get_format (operation, "input");
  gfloat         *in     = in_buf;
  gfloat         *out    = out_buf;
  gint            component_index = 0;
  gint            n_components;
  gdouble         min = 0.0;
  gdouble         max = 1.0;

  n_components = babl_format_get_n_components (format);

  switch (o->component)
    {
    case GEGL_COMPONENT_EXTRACT_RGB_RED:
    case GEGL_COMPONENT_EXTRACT_HUE:
    case GEGL_COMPONENT_EXTRACT_CMYK_CYAN:
    case GEGL_COMPONENT_EXTRACT_YCBCR_Y:
    case GEGL_COMPONENT_EXTRACT_LAB_L:
      component_index = 0;

      if (o->component == GEGL_COMPONENT_EXTRACT_LAB_L)
        {
          max = 100.0;
        }
      break;

    case GEGL_COMPONENT_EXTRACT_RGB_GREEN:
    case GEGL_COMPONENT_EXTRACT_HSV_SATURATION:
    case GEGL_COMPONENT_EXTRACT_HSL_SATURATION:
    case GEGL_COMPONENT_EXTRACT_CMYK_MAGENTA:
    case GEGL_COMPONENT_EXTRACT_YCBCR_CB:
    case GEGL_COMPONENT_EXTRACT_LAB_A:
    case GEGL_COMPONENT_EXTRACT_LCH_C:
    case GEGL_COMPONENT_EXTRACT_ALPHA:
      component_index = 1;

      if (o->component == GEGL_COMPONENT_EXTRACT_YCBCR_CB)
        {
          min = -0.5;
          max =  0.5;
        }
      else if (o->component == GEGL_COMPONENT_EXTRACT_LAB_A)
        {
          min = -127.5;
          max =  127.5;
        }
      else if (o->component == GEGL_COMPONENT_EXTRACT_LCH_C)
        {
          max = 200.0;
        }
      break;

    case GEGL_COMPONENT_EXTRACT_RGB_BLUE:
    case GEGL_COMPONENT_EXTRACT_HSV_VALUE:
    case GEGL_COMPONENT_EXTRACT_HSL_LIGHTNESS:
    case GEGL_COMPONENT_EXTRACT_CMYK_YELLOW:
    case GEGL_COMPONENT_EXTRACT_YCBCR_CR:
    case GEGL_COMPONENT_EXTRACT_LAB_B:
    case GEGL_COMPONENT_EXTRACT_LCH_H:
      component_index = 2;

      if (o->component == GEGL_COMPONENT_EXTRACT_YCBCR_CR)
        {
          min = -0.5;
          max =  0.5;
        }
      else if (o->component == GEGL_COMPONENT_EXTRACT_LAB_B)
        {
          min = -127.5;
          max =  127.5;
        }
      else if (o->component == GEGL_COMPONENT_EXTRACT_LCH_H)
        {
          min = 0.0;
          max = 360.0;
        }
      break;

    case GEGL_COMPONENT_EXTRACT_CMYK_KEY:
      component_index = 3;
      break;
    }

    while (samples--)
      {
        gdouble value = in[component_index];

        if (min != 0.0 || max != 1.0)
          {
            gdouble scale  = 1.0 / (max - min);
            gdouble offset = -min;

            value = CLAMP ((value + offset) * scale, 0.0, 1.0);
          }

        if (o->invert)
          out[0] = 1.0 - value;
        else
          out[0] = value;

        in  += n_components;
        out += 1;
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

  operation_class->prepare        = prepare;
  operation_class->opencl_support = FALSE;
  point_filter_class->process     = process;

  gegl_operation_class_set_keys (operation_class,
    "name",            "gegl:component-extract",
    "title",           _("Extract Component"),
    "reference-hash",  "9e9128c635e84fd177d733ba300d6ef5",
    "categories",  "color",
    "description", _("Extract a color model component"),
    NULL);
}

#endif
