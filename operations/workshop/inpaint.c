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
 *
 */

#include <stdio.h>
#include "config.h"
#include <glib/gi18n-lib.h>

//retire propes after given set of completed re-runs


#ifdef GEGL_PROPERTIES


property_int (seek_distance, "seek radius", 30)
  value_range (4, 512)

property_int (min_neigh, "min neigh", 2)
  value_range (0, 4)

property_int (min_iter, "min iter", 100)
  value_range (1, 512)

property_int (max_iter, "max iter", 2000)
  value_range (1, 40000)

property_int (improvement_iters, "improvement iters", 2)

property_int (k, "k", 3)
  value_range (1, 8)

property_double (chance_try, "try chance", 0.33)
  value_range (0.0, 1.0)
  ui_steps    (0.01, 0.1)
property_double (chance_retry, "retry chance", 1.0)
  value_range (0.0, 1.0)
  ui_steps    (0.01, 0.1)

property_double (ring_gap,    "ring gap", 1.3)
  value_range (0.0, 8.0)
  ui_steps    (0.1, 0.2)

property_double (ring_gamma, "ring gamma", 1.4)
  value_range (0.0, 4.0)
  ui_steps    (0.1, 0.2)
property_double (ring_twist, "ring twist", 0.0)
  value_range (0.0, 1.0)
  ui_steps    (0.01, 0.2)

property_double (ring_gap1,    "ring gap1", 1.2)
  value_range (0.0, 8.0)
  ui_steps    (0.25, 0.25)

property_double (ring_gap2,    "ring gap2", 2.5)
  value_range (0.0, 8.0)
  ui_steps    (0.25, 0.25)

property_double (ring_gap3,    "ring gap3", 3.5)
  value_range (0.0, 8.0)
  ui_steps    (0.25, 0.25)

property_double (ring_gap4,    "ring gap4", 4.5)
  value_range (0.0, 8.0)
  ui_steps    (0.25, 0.25)

property_double (metric_dist_powk, "metric dist powk", 2.0)
  value_range (0.0, 10.0)
  ui_steps    (0.1, 1.0)

property_double (metric_empty_hay_score, "metric empty hay score", 0.5)
  value_range (0.01, 100.0)
  ui_steps    (0.05, 0.1)

property_double (metric_empty_needle_score, "metric empty needle score", 0.033)
  value_range (0.01, 100.0)
  ui_steps    (0.05, 0.1)

property_double (metric_cohesion, "metric cohesion", 0.004)
  value_range (0.0, 10.0)
  ui_steps    (0.2, 0.2)

property_double (scale, "enlarge as well as inpaint 1.0 does nothing", 1.0)
  value_range (0.0, 10.0)
  ui_steps    (0.5, 0.5)


#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME      inpaint
#define GEGL_OP_C_SOURCE  inpaint.c

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

  rect = *gegl_buffer_get_extent (in);
  for (y = 0; y < rect.height; y++)
    for (x = 0; x < rect.width; x++)
      {
        float rgba[4];
        gegl_sampler_get (duster->in_sampler_f, x, y, NULL,
                          &rgba[0], 0);
        {
          GeglRectangle r = {x * scale, y * scale, 1, 1};
          gegl_buffer_set (out, &r, 0, format, &rgba[0], 0);
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
  PixelDuster    *duster = pixel_duster_new (input, input, output, &in_rect, &out_rect,
                                             o->seek_distance,
                                             o->k, // max_k
                                             o->min_neigh,
                                             o->min_iter,
                                             o->max_iter,
                                             o->chance_try,
                                             o->chance_retry,
                                             o->scale, // scale_x
                                             o->scale, // scale_y
                                             o->improvement_iters,
                                             o->ring_gap,
                                             o->ring_gap1,
                                             o->ring_gap2,
                                             o->ring_gap3,
                                             o->ring_gap4,
                                             o->ring_gamma,
                                             o->ring_twist,
                                             o->metric_dist_powk,
                                             o->metric_empty_hay_score,
                                             o->metric_empty_needle_score,
                                             o->metric_cohesion/1000.0,
                                             operation);

  if (o->scale > 0.9999 && o->scale < 1.0001)
    gegl_buffer_copy (input, NULL, GEGL_ABYSS_NONE, output, NULL);
  else
    scaled_copy (duster, input, output, o->scale);

  pixel_duster_add_probes_for_transparent (duster);

  seed_db (duster);
  pixel_duster_fill (duster);
  pixel_duster_destroy (duster);

  return TRUE;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;

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

  if ((in_rect && gegl_rectangle_is_infinite_plane (in_rect)))
    {
      gpointer in = gegl_operation_context_get_object (context, "input");
      gegl_operation_context_take_object (context, "output",
                                          g_object_ref (G_OBJECT (in)));
      return TRUE;
    }

  return operation_class->process (operation, context, output_prop, result,
                                   gegl_operation_context_get_level (context));
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  GeglRectangle *res = gegl_operation_source_get_bounding_box (operation, "input");
  GeglRectangle result = {0,0,100,100};
  if (res)
    result = *res;
  result.x = 0;
  result.y = 0;
  result.width  *= o->scale;
  result.height *= o->scale;

  return result;
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
      "name",        "gegl:alpha-inpaint",
      "title",       "Heal transparent",
      "categories",  "heal",
      "description", "Replaces fully transparent pixels with good candidate pixels found in the whole image",
      NULL);
}

#endif 
