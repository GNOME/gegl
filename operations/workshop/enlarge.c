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

/* most of these should go away - here for ease of algorithm experimentation */

property_int (seek_distance, "seek radius", 128)
  value_range (4, 512)

property_int (max_k, "max k", 4)
  value_range (1, 4)

property_double (scale, "scale", 2.0)
  value_range (0.01, 16.0)

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME      enlarge
#define GEGL_OP_C_SOURCE  enlarge.c

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

static void scaled_copy (GeglBuffer *in,
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
        float rgba[4];
        GeglRectangle r = {x, y, 1, 1};
        gegl_buffer_sample (in, x / scale, y / scale, NULL,
                            &rgba[0], format,
                            GEGL_SAMPLER_NEAREST, 0);
        gegl_buffer_set (out, &r, 0, format, &rgba[0], 0);
      }
}

static void improve (PixelDuster *duster,
                     GeglBuffer *in,
                     GeglBuffer *out,
                     gfloat      scale)
{
  GeglRectangle rect;
  const Babl *format = babl_format ("R'G'B'A float");
  gint x, y;

  rect = *gegl_buffer_get_extent (out);
  for (y = 0; y < rect.height; y++)
  {
    int xstart = 0;
    int xinc = 1;
    int xend = rect.width;
#if 0 /* consistent scanline direction produces more consistent results */
    if (y%2)
     {
       xstart = rect.width - 1;
       xinc = -1;
       xend = -1;
     }
#endif
    for (x = xstart; x != xend; x+=xinc)
      {
        Probe *probe;
        probe = add_probe (duster, x, y);
        if (probe_improve (duster, probe) == 0)
        {
#if PIXDUST_REL_DIGEST==0
          gfloat rgba[4*MAX_K];

          for (int j = 0; j < MAX_K; j++)
            gegl_buffer_sample (duster->input, probe->source_x[j], probe->source_y[j],  NULL, &rgba[j*4], format, GEGL_SAMPLER_NEAREST, 0);


          for (int j = 1; j < probe->k; j++)
            for (int c = 0; c < 4; c++)
            {
              rgba[0+c] += rgba[j*4+c];
            }
          for (int c = 0; c < 4; c++)
            {
              rgba[c] /= probe->k;
            }

          if (rgba[3] <= 0.01)
            fprintf (stderr, "eek %i,%i %f %f %f %f\n", probe->source_x[MAX_K/2], probe->source_y[MAX_K/2], rgba[0], rgba[1], rgba[2], rgba[3]);

          gegl_buffer_set (duster->output, GEGL_RECTANGLE(probe->target_x, probe->target_y, 1, 1), 0, format, &rgba[0], 0);
#else
          gfloat rgba[4];
          gfloat delta[4]={0,0,0,0};
          {
            int dx = 0, dy = 0;
            duster_idx_to_x_y (duster, 1, probe->hay[0][0], &dx, &dy);
            gegl_buffer_sample (duster->output, probe->target_x + dx, probe->target_y + dy,  NULL, &rgba[0], format, GEGL_SAMPLER_NEAREST, 0);
          }

          for (int k = 0; k < probe->k; k++)
          {
            for (int c = 0; c < 3; c ++)
              delta[c] += ((probe->hay[k][4+c]-127.0) / 128);
          }

          for (int c = 0; c < 3; c ++)
            rgba[c] = rgba[c] - delta[c] / probe->k;
          rgba[3]=1.0;

          gegl_buffer_set (duster->output, GEGL_RECTANGLE(probe->target_x, probe->target_y, 1, 1), 0, format, &rgba[0], 0);

#endif
        }
        g_hash_table_remove (duster->probes_ht, xy2offset(probe->target_x, probe->target_y));
      }
   fprintf (stderr, "\r%2.2f   ", y * 100.0 / rect.height);
  }
   fprintf (stderr, "\n");
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
  scaled_copy (input, output, o->scale);
  duster  = pixel_duster_new (input, output,
                              &in_rect, &out_rect,
                              o->seek_distance,
                              o->max_k,
                              1, 1, 1.0, 0.3,
                              o->scale,
                              o->scale,
                              NULL);
  seed_db (duster);
  improve (duster, input, output, o->scale);
  pixel_duster_destroy (duster);

  return TRUE;
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

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

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
      "name",        "gegl:enlarge",
      "title",       "Smart enlarge",
      "categories",  "heal",
      "description", "Enlarges an images based on pixel contents",
      NULL);
}

#endif
