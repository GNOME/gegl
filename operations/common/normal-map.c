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
 * Copyright (C) 2019 Ell
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_normal_map_component)
  enum_value (GEGL_NORMAL_MAP_COMPONENT_RED,   "red",   N_("Red"))
  enum_value (GEGL_NORMAL_MAP_COMPONENT_GREEN, "green", N_("Green"))
  enum_value (GEGL_NORMAL_MAP_COMPONENT_BLUE,  "blue",  N_("Blue"))
enum_end (GeglNormalMapComponent)

property_double (scale, _("Scale"), 10.0)
  description (_("The amount by which to scale the height values"))
  value_range (0.0, G_MAXDOUBLE)
  ui_range    (0.0, 255.0)

property_enum (x_component, _("X Component"),
               GeglNormalMapComponent, gegl_normal_map_component,
               GEGL_NORMAL_MAP_COMPONENT_RED)
  description (_("The component used for the X coordinates"))

property_enum (y_component, _("Y Component"),
               GeglNormalMapComponent, gegl_normal_map_component,
               GEGL_NORMAL_MAP_COMPONENT_GREEN)
  description (_("The component used for the Y coordinates"))

property_boolean (flip_x, _("Flip X"), FALSE)
  description (_("Flip the X coordinates"))

property_boolean (flip_y, _("Flip Y"), FALSE)
  description (_("Flip the Y coordinates"))

property_boolean (full_z, _("Full Z Range"), FALSE)
  description (_("Use the full [0,1] range to encode the Z coordinates"))

property_boolean (tileable, _("Tileable"), FALSE)
  description (_("Generate a tileable map"))

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     normal_map
#define GEGL_OP_C_SOURCE normal-map.c

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);

  const Babl *format = gegl_operation_get_source_format (operation, "input");
  const Babl *in_format;
  const Babl *out_format;

  area->left   =
  area->right  =
  area->top    =
  area->bottom = 1;

  in_format  = babl_format_with_space ("Y'A float", format);
  out_format = babl_format_with_space ("R'G'B'A float", format);

  gegl_operation_set_format (operation, "input", in_format);
  gegl_operation_set_format (operation, "output", out_format);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle *in_rect;
  GeglRectangle  result = {};

  in_rect = gegl_operation_source_get_bounding_box (operation, "input");

  if (in_rect)
    result = *in_rect;

  return result;
}

static GeglAbyssPolicy
get_abyss_policy (GeglOperation *operation,
                  const gchar   *input_pad)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  return o->tileable ? GEGL_ABYSS_LOOP : GEGL_ABYSS_CLAMP;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties     *o            = GEGL_PROPERTIES (operation);
  const Babl         *in_format    = gegl_operation_get_format (operation, "input");
  const Babl         *out_format   = gegl_operation_get_format (operation, "output");
  GeglAbyssPolicy     abyss_policy = get_abyss_policy (operation, NULL);
  gfloat              scale        = o->scale / 2.0;
  gfloat              x_scale      = (o->flip_x ? -0.5 : +0.5);
  gfloat              y_scale      = (o->flip_y ? -0.5 : +0.5);
  gfloat              z_scale      = (o->full_z ? +1.0 : +0.5);
  gfloat              z_base       = (o->full_z ?  0.0 :  0.5);
  gint                x_component  = o->x_component;
  gint                y_component  = o->y_component;
  gint                z_component  = 2;
  GeglBufferIterator *iter;

  while (y_component == x_component)
    y_component = (y_component + 1) % 3;

  while (z_component == x_component || z_component == y_component)
    z_component = (z_component + 1) % 3;

  iter = gegl_buffer_iterator_new (output, result, 0, out_format,
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 2);

  gegl_buffer_iterator_add (iter, input, result, 0, in_format,
                            GEGL_ACCESS_READ, abyss_policy);

  while (gegl_buffer_iterator_next (iter))
    {
      const gfloat        *in     = iter->items[1].data;
      gfloat              *out    = iter->items[0].data;
      const GeglRectangle *roi    = &iter->items[0].roi;
      gint                 stride = 2 * roi->width;
      gfloat               top[2 * roi->width];
      gfloat               bottom[2 * roi->width];
      gfloat               left[2 * roi->height];
      gfloat               right[2 * roi->height];
      gint                 x;
      gint                 y;

      gegl_buffer_get (input,
                       GEGL_RECTANGLE (roi->x, roi->y - 1,
                                       roi->width, 1),
                       1.0, in_format, top,
                       GEGL_AUTO_ROWSTRIDE, abyss_policy);
      gegl_buffer_get (input,
                       GEGL_RECTANGLE (roi->x, roi->y + roi->height,
                                       roi->width, 1),
                       1.0, in_format, bottom,
                       GEGL_AUTO_ROWSTRIDE, abyss_policy);
      gegl_buffer_get (input,
                       GEGL_RECTANGLE (roi->x - 1, roi->y,
                                       1, roi->height),
                       1.0, in_format, left,
                       GEGL_AUTO_ROWSTRIDE, abyss_policy);
      gegl_buffer_get (input,
                       GEGL_RECTANGLE (roi->x + roi->width, roi->y,
                                       1, roi->height),
                       1.0, in_format, right,
                       GEGL_AUTO_ROWSTRIDE, abyss_policy);

      for (y = 0; y < roi->height; y++)
        {
          for (x = 0; x < roi->width; x++)
            {
              gfloat l;
              gfloat r;
              gfloat t;
              gfloat b;
              gfloat nx;
              gfloat ny;
              gfloat nz;

              if (x > 0)
                l = in[-2];
              else
                l = left[2 * y];

              if (x < roi->width - 1)
                r = in[2];
              else
                r = right[2 * y];

              if (y > 0)
                t = in[-stride];
              else
                t = top[2 * x];

              if (y < roi->height - 1)
                b = in[stride];
              else
                b = bottom[2 * x];

              nx = scale * (l - r);
              ny = scale * (t - b);

              nz = 1.0f / sqrtf (nx * nx + ny * ny + 1.0f);

              nx *= nz;
              ny *= nz;

              out[x_component] = 0.5f   + x_scale * nx;
              out[y_component] = 0.5f   + y_scale * ny;
              out[z_component] = z_base + z_scale * nz;
              out[3]           = in[1];

              in  += 2;
              out += 4;
            }
        }
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass           *operation_class;
  GeglOperationFilterClass     *filter_class;
  GeglOperationAreaFilterClass *area_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);
  area_class      = GEGL_OPERATION_AREA_FILTER_CLASS (klass);

  area_class->get_abyss_policy      = get_abyss_policy;
  filter_class->process             = process;
  operation_class->prepare          = prepare;
  operation_class->get_bounding_box = get_bounding_box;

  gegl_operation_class_set_keys (operation_class,
    "name",           "gegl:normal-map",
    "title",          _("Normal Map"),
    "categories",     "map",
    "reference-hash", "5f6052195f03b52185942a2c1fecd98d",
    "reference-hashB", "adc8bbb4ce3f6c67b4c4cd6ac3c72942",
    "description",    _("Generate a normal map from a height map"),
    NULL);
}

#endif
