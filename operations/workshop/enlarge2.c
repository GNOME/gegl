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
 * Copyright 2018 Øyvind Kolås <pippin@gimp.org>
 *
 */

#include <stdio.h>
#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int (seek_distance, "seek radius", 256)
  value_range (4, 512)

property_int (max_k, "max k", 4)
  value_range (1, 4)

property_double (scale, "scale", 2.0)
  value_range (0.1, 20.0)

property_int (min_neigh, "min neigh", 1)
  value_range (1, 10)

property_int (min_iter, "min iter", 512)
  value_range (1, 512)

property_double (chance_try, "try chance", 0.1)
  value_range (0.0, 1.0)

property_double (chance_retry, "retry chance", 0.0)
  value_range (0.0, 1.0)

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME      enlarge2
#define GEGL_OP_C_SOURCE  enlarge2.c

#include "gegl-op.h"
#include <stdio.h>
#include "pixel-duster.h"

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");
  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;
  return result;
}

static void
prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("RGBA float");

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
}

static void scaled_copy (PixelDuster *duster,
                         GeglBuffer *in,
                         GeglBuffer *out,
                         gfloat      scale)
{
  GeglRectangle rect;
  const Babl *format = babl_format ("RGBA float");
  gint x, y;

  rect = *gegl_buffer_get_extent (out);
  for (y = 0; y < rect.height; y++)
    for (x = 0; x < rect.width; x++)
      {
        GeglRectangle r = {x, y, 1, 1};
        gfloat rgba[4] = {0,0,0,0};
        gegl_buffer_set (out, &r, 0, format, &rgba[0], 0);
      }

  rect = *gegl_buffer_get_extent (in);
  for (y = 0; y < rect.height; y++)
    for (x = 0; x < rect.width; x++)
      {
        GeglRectangle r = {x * scale , y * scale, 1, 1};
        gfloat rgba[4];
        gegl_sampler_get (duster->in_sampler_f, x, y, NULL, &rgba[0], 0);
        gegl_buffer_set (out, &r, 0, format, &rgba[0], 0);
      }
}

static void remove_grid (GeglBuffer *in,
                         GeglBuffer *out,
                         gfloat      scale)
{
  GeglRectangle rect;
  const Babl *format = babl_format ("RGBA float");
  gint x, y;
  rect = *gegl_buffer_get_extent (in);
  for (y = 0; y < rect.height; y++)
    for (x = 0; x < rect.width; x++)
      {
        GeglRectangle r = {x * scale , y * scale, 1, 1};
        gfloat rgba[4] = {0,0,0,0};
        gegl_buffer_set (out, &r, 0, format, &rgba[0], 0);
      }
}

static void remove_deviant (PixelDuster *duster,
                            GeglBuffer *in,
                            GeglBuffer *out,
                            gfloat      scale)
{
  GeglRectangle rect;
  const Babl *format = babl_format ("RGBA float");
  gint x, y;
  rect = *gegl_buffer_get_extent (out);
  for (y = 0; y < rect.height; y++)
    for (x = 0; x < rect.width; x++)
      {
        GeglRectangle r = {x * scale , y * scale, 1, 1};
        gfloat rgba_in[4] = {0,0,0,0};
        gfloat rgba_empty[4] = {0,0,0,0};
        gfloat rgba_out[4] = {0,0,0,0};
        gegl_sampler_get (duster->in_sampler_f, x / scale, y / scale, NULL, &rgba_in[0], 0);
        gegl_buffer_sample (out, x, y, NULL, &rgba_out[0], format, GEGL_SAMPLER_NEAREST, 0);

        if (POW2(rgba_in[0]-rgba_out[0])+
            POW2(rgba_in[1]-rgba_out[1])+
            POW2(rgba_in[2]-rgba_out[2]) > 14 + 14 + 14)
        {
          gegl_buffer_set (out, &r, 0, format, &rgba_empty[0], 0);
        }
      }
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  GeglRectangle in_rect = *gegl_buffer_get_extent (input);
  GeglRectangle out_rect = *gegl_buffer_get_extent (output);
  PixelDuster    *duster;

  duster  = pixel_duster_new (input, output,
                              &in_rect, &out_rect,
                              o->seek_distance,
                              o->max_k,
                              o->min_neigh,
                              o->min_iter,
                              o->chance_try,
                              o->chance_retry,
                              o->scale,
                              o->scale,
                              NULL);
  scaled_copy (duster, input, output, o->scale);
  seed_db (duster);
  pixel_duster_add_probes_for_transparent (duster);
  pixel_duster_fill (duster);
  remove_deviant (duster, input, output, o->scale);
  remove_grid (input, output, o->scale);
  pixel_duster_remove_probes (duster);
  pixel_duster_add_probes_for_transparent (duster);
  pixel_duster_fill (duster);

  remove_deviant (duster, input, output, o->scale);
  pixel_duster_remove_probes (duster);
  pixel_duster_add_probes_for_transparent (duster);
  pixel_duster_fill (duster);

  pixel_duster_remove_probes (duster);
  pixel_duster_destroy (duster);

  return TRUE;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  result.x = 0;
  result.y = 0;
  result.width  *= o->scale;
  result.height *= o->scale;

  return result;
}


static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");
  GeglProperties *o      = GEGL_PROPERTIES (operation);

  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;
  result.x = 0;
  result.y = 0;
  result.width  *= o->scale;
  result.height *= o->scale;

  return result;
}

static gboolean
operation_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglOperationClass  *operation_class;

  const GeglRectangle *in_rect =
    gegl_operation_source_get_bounding_box (operation, "input");

  operation_class = GEGL_OPERATION_CLASS (gegl_op_parent_class);

  if (in_rect && gegl_rectangle_is_infinite_plane (in_rect))
    {
      gpointer in = gegl_operation_context_get_object (context, "input");
      gegl_operation_context_take_object (context, "output",
                                          g_object_ref (G_OBJECT (in)));
      return TRUE;
    }

  return operation_class->process (operation, context, output_prop, result,
                                   gegl_operation_context_get_level (context));
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process                    = process;
  operation_class->prepare                 = prepare;
  operation_class->process                 = operation_process;
  operation_class->get_bounding_box        = get_bounding_box;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region       = get_cached_region;
  operation_class->opencl_support          = FALSE;
  operation_class->threaded                = FALSE;

  gegl_operation_class_set_keys (operation_class,
      "name",        "gegl:enlarge2",
      "title",       "Smart enlarge",
      "categories",  "heal",
      "description", "Enlarges an images based on pixel contents",
      NULL);
}

#endif
