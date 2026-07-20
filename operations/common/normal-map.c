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

property_enum (x_component, _("X component"),
               GeglNormalMapComponent, gegl_normal_map_component,
               GEGL_NORMAL_MAP_COMPONENT_RED)
  description (_("The component used for the X coordinates"))

property_enum (y_component, _("Y component"),
               GeglNormalMapComponent, gegl_normal_map_component,
               GEGL_NORMAL_MAP_COMPONENT_GREEN)
  description (_("The component used for the Y coordinates"))

property_boolean (flip_x, _("Flip X"), FALSE)
  description (_("Flip the X coordinates"))

property_boolean (flip_y, _("Flip Y"), FALSE)
  description (_("Flip the Y coordinates"))

property_boolean (full_z, _("Full Z range"), FALSE)
  description (_("Use the full [0,1] range to encode the Z coordinates"))

property_boolean (tileable, _("Tileable"), FALSE)
  description (_("Generate a tileable map"))

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     normal_map
#define GEGL_OP_C_SOURCE normal-map.c

#define KERNEL_SIZE 1

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
  area->bottom = KERNEL_SIZE;

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
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties       *o              = GEGL_PROPERTIES (operation);
  const Babl           *in_format      = gegl_operation_get_format (operation, "input");
  const Babl           *out_format     = gegl_operation_get_format (operation, "output");
  const gint            in_components  = babl_format_get_n_components (in_format);
  const gint            out_components = babl_format_get_n_components (out_format);
  const GeglAbyssPolicy abyss_policy   = get_abyss_policy (operation, NULL);
  const GeglRectangle   in_roi         = {
    roi->x - KERNEL_SIZE,
    roi->y - KERNEL_SIZE,
    roi->width  + KERNEL_SIZE * 2,
    roi->height + KERNEL_SIZE * 2,
  };
  gfloat scale       = o->scale / 2.0;
  gfloat x_scale     = (o->flip_x ? -0.5 : +0.5);
  gfloat y_scale     = (o->flip_y ? -0.5 : +0.5);
  gfloat z_scale     = (o->full_z ? +1.0 : +0.5);
  gfloat z_base      = (o->full_z ? 0.0 : 0.5);
  gint   x_component = o->x_component;
  gint   y_component = o->y_component;
  gint   z_component = 2;

  while (y_component == x_component)
    y_component = (y_component + 1) % 3;

  while (z_component == x_component || z_component == y_component)
    z_component = (z_component + 1) % 3;

  GeglBufferIterator *iter = gegl_buffer_iterator_new (
      output, roi, 0, out_format, GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 2);
  gegl_buffer_iterator_add (iter, input, &in_roi, 0, in_format,
                            GEGL_ACCESS_READ, abyss_policy);

  while (gegl_buffer_iterator_next (iter))
    {
      const GeglRectangle *out_roi = &iter->items[0].roi;
      const GeglRectangle *in_roi  = &iter->items[1].roi;
      const gfloat        *in      = iter->items[1].data;
      gfloat              *out     = iter->items[0].data;

      for (gint y = 0; y < out_roi->height; y++)
        {
          for (gint x = 0; x < out_roi->width; x++)
            {
#define KERNEL(dx, dy, c) in[(((KERNEL_SIZE + y + dy) * in_roi->width) + (KERNEL_SIZE + x + dx)) * in_components + c]
              gfloat l = KERNEL(-1, 0, 0);
              gfloat r = KERNEL(1, 0, 0);
              gfloat t = KERNEL(0, -1, 0);
              gfloat b = KERNEL(0, 1, 0);
              gfloat a = KERNEL(0, 0, 1);
#undef KERNEL

              gfloat nx = scale * (l - r);
              gfloat ny = scale * (t - b);
              gfloat nz = 1.0f / sqrtf (nx * nx + ny * ny + 1.0f);

              nx *= nz;
              ny *= nz;

              out[x_component] = 0.5f   + x_scale * nx;
              out[y_component] = 0.5f   + y_scale * ny;
              out[z_component] = z_base + z_scale * nz;
              out[3]           = a;

              out += out_components;
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
    "reference-hash", "59c0af6158719538c0a77bc459b20779",
    "description",    _("Generate a normal map from a height map"),
    NULL);
}

#endif
