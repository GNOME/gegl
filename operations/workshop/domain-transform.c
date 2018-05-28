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
 * Copyright 2018 Felipe Einsfeld Kersting <fekersting@inf.ufrgs.br>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int (num_iterations, _("Quality"), 3)
  description(_("Number of filtering iterations. A value between 2 and 4 is usually enough."))
  value_range (1, 5)

property_double (spatial_factor, _("Blur radius"),  30.0)
  description  (_("Spatial standard deviation of the blur kernel, measured in pixels."))
  value_range (0, 1000.0)

property_double (edge_preservation, _("Edge preservation"),  0.8)
  description  (_("Amount of edge preservation. This quantity is inversely proportional to the range standard deviation of the blur kernel."))
  value_range (0, 1.0)

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     domain_transform
#define GEGL_OP_C_SOURCE domain-transform.c

#include "gegl-op.h"
#include <math.h>

// This should be 768, since we have 2^8 possible options for each channel.
// Since domain transform is given by:
// 1 + (s_s / s_r) * (diff_channel_R + diff_channel_G + diff_channel_B)
// We will have 3 * 2^8 different possibilities for the transform of each pixel.
// 3 * 2^8 = 768
#define RF_TABLE_SIZE 768
#define SQRT3 1.7320508075f
#define SQRT2 1.4142135623f
#define BLOCK_STRIDE 1
#define REPORT_PROGRESS_TIME 0.5  // time to report gegl_operation_progress

static gint16
absolute (gint16 x)
{
  return (x < 0) ? -x : x;
}

static void
report_progress (GeglOperation* operation, gdouble progress, GTimer* timer)
{
  static gboolean reported = FALSE;

  if (progress == 0.0)
    reported = FALSE;
    
  if (g_timer_elapsed(timer, NULL) >= REPORT_PROGRESS_TIME && !reported)
    {
      reported = TRUE;
      gegl_operation_progress(operation, 0.0, "");
    }

  if (reported)
    gegl_operation_progress (operation, progress, "");
}

static gint
domain_transform (GeglOperation *operation,
                  gint image_width,
                  gint image_height,
                  gint image_channels,
                  gfloat spatial_factor,
                  gfloat range_factor,
                  gint num_iterations,
                  GeglBuffer *input,
                  GeglBuffer *output)
{
  gfloat** rf_table;
  guint16* transforms_buffer;
  gfloat* image_buffer;

  gint16 last_pixel_r, last_pixel_g, last_pixel_b;
  gint16 current_pixel_r, current_pixel_g, current_pixel_b;
  gfloat last_pixel_r_f, last_pixel_g_f, last_pixel_b_f, last_pixel_a_f;
  gfloat current_pixel_r_f, current_pixel_g_f, current_pixel_b_f, current_pixel_a_f;
  gfloat sum_channels_difference, a, w, current_standard_deviation;
  gint i, j, k, n, real_stride, d_x_position, d_y_position, biggest_dimension;
  guint16 d;
  GeglRectangle current_rectangle;
  GTimer* timer;

  timer = g_timer_new();

  /* PRE-ALLOC MEMORY */
  biggest_dimension = (image_width > image_height) ? image_width : image_height;
  image_buffer = g_new(gfloat, BLOCK_STRIDE * biggest_dimension * image_channels);
  transforms_buffer = g_new(guint16, BLOCK_STRIDE * biggest_dimension);

  rf_table = g_new(gfloat*, num_iterations);
  for (i = 0; i < num_iterations; ++i)
    rf_table[i] = g_new(gfloat, RF_TABLE_SIZE);

  /* ************************ */

  report_progress(operation, 0.0, timer);

  // Pre-calculate RF table
  for (i = 0; i < num_iterations; ++i)
    {
      // calculating RF feedback coefficient 'a' from desired variance
      // 'a' will change each iteration while the domain transform will remain constant
      current_standard_deviation = spatial_factor * SQRT3 * (powf(2.0f, (gfloat)(num_iterations - (i + 1))) / sqrtf(powf(4.0f, (gfloat)num_iterations) - 1));
      a = expf(-SQRT2 / current_standard_deviation);
  
      for (j = 0; j < RF_TABLE_SIZE; ++j)
        rf_table[i][j] = powf(a, 1.0f + (spatial_factor / range_factor) * ((gfloat)j / 255.0f));
    }

  // Filter Iterations
  for (n = 0; n < num_iterations; ++n)
    {
      // Horizontal Pass
      for (i = 0; i < image_height; i += BLOCK_STRIDE)
        {
          real_stride = (i + BLOCK_STRIDE > image_height) ? image_height - i : BLOCK_STRIDE;
    
          current_rectangle.x = 0;
          current_rectangle.y = i;
          current_rectangle.width = image_width;
          current_rectangle.height = real_stride;
    
          gegl_buffer_get (input, &current_rectangle, 1.0, babl_format ("R'G'B' u8"), image_buffer, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
    
          // Domain Transform
          for (j = 0; j < current_rectangle.height; ++j)
            {
              last_pixel_r = ((guint8*)image_buffer)[j * current_rectangle.width * 3];
              last_pixel_g = ((guint8*)image_buffer)[j * current_rectangle.width * 3 + 1];
              last_pixel_b = ((guint8*)image_buffer)[j * current_rectangle.width * 3 + 2];
      
              for (k = 0; k < current_rectangle.width; ++k)
                {
                  current_pixel_r = ((guint8*)image_buffer)[j * current_rectangle.width * 3 + k * 3];
                  current_pixel_g = ((guint8*)image_buffer)[j * current_rectangle.width * 3 + k * 3 + 1];
                  current_pixel_b = ((guint8*)image_buffer)[j * current_rectangle.width * 3 + k * 3 + 2];
        
                  sum_channels_difference = absolute(current_pixel_r - last_pixel_r) + absolute(current_pixel_g - last_pixel_g) + absolute(current_pixel_b - last_pixel_b);
        
                  // @NOTE: 'd' should be 1.0f + s_s / s_r * sum_diff
                  // However, we will store just sum_diff.
                  // 1.0f + s_s / s_r will be calculated later when calculating the RF table.
                  // This is done this way because the sum_diff is perfect to be used as the index of the RF table.
                  // d = 1.0f + (vdt_information->spatial_factor / vdt_information->range_factor) * sum_channels_difference;
                  transforms_buffer[j * current_rectangle.width + k] = sum_channels_difference;
        
                  last_pixel_r = current_pixel_r;
                  last_pixel_g = current_pixel_g;
                  last_pixel_b = current_pixel_b;
                }
            }
    
          if (n == 0)
            gegl_buffer_get (input, &current_rectangle, 1.0, babl_format ("R'G'B'A float"), image_buffer, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
          else
            gegl_buffer_get (output, &current_rectangle, 1.0, babl_format ("R'G'B'A float"), image_buffer, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
    
          // Horizontal Filter (Left-Right)
          for (j = 0; j < current_rectangle.height; ++j)
            {
              last_pixel_r_f = image_buffer[j * current_rectangle.width * image_channels];
              last_pixel_g_f = image_buffer[j * current_rectangle.width * image_channels + 1];
              last_pixel_b_f = image_buffer[j * current_rectangle.width * image_channels + 2];
              last_pixel_a_f = image_buffer[j * current_rectangle.width * image_channels + 3];
      
              for (k = 0; k < current_rectangle.width; ++k)
                {
                  d = transforms_buffer[j * current_rectangle.width + k];
                  w = rf_table[n][d];
        
                  current_pixel_r_f = image_buffer[j * current_rectangle.width * image_channels + k * image_channels];
                  current_pixel_g_f = image_buffer[j * current_rectangle.width * image_channels + k * image_channels + 1];
                  current_pixel_b_f = image_buffer[j * current_rectangle.width * image_channels + k * image_channels + 2];
                  current_pixel_a_f = image_buffer[j * current_rectangle.width * image_channels + k * image_channels + 3];
        
                  last_pixel_r_f = ((1 - w) * current_pixel_r_f + w * last_pixel_r_f);
                  last_pixel_g_f = ((1 - w) * current_pixel_g_f + w * last_pixel_g_f);
                  last_pixel_b_f = ((1 - w) * current_pixel_b_f + w * last_pixel_b_f);
                  last_pixel_a_f = ((1 - w) * current_pixel_a_f + w * last_pixel_a_f);
        
                  image_buffer[j * current_rectangle.width * image_channels + k * image_channels] = last_pixel_r_f;
                  image_buffer[j * current_rectangle.width * image_channels + k * image_channels + 1] = last_pixel_g_f;
                  image_buffer[j * current_rectangle.width * image_channels + k * image_channels + 2] = last_pixel_b_f;
                  image_buffer[j * current_rectangle.width * image_channels + k * image_channels + 3] = last_pixel_a_f;
                }
            }
    
          // Horizontal Filter (Right-Left)
          for (j = 0; j < current_rectangle.height; ++j)
            {
              last_pixel_r_f = image_buffer[j * current_rectangle.width * image_channels + (current_rectangle.width - 1) * image_channels];
              last_pixel_g_f = image_buffer[j * current_rectangle.width * image_channels + (current_rectangle.width - 1) * image_channels + 1];
              last_pixel_b_f = image_buffer[j * current_rectangle.width * image_channels + (current_rectangle.width - 1) * image_channels + 2];
              last_pixel_a_f = image_buffer[j * current_rectangle.width * image_channels + (current_rectangle.width - 1) * image_channels + 3];
              
              for (k = current_rectangle.width - 1; k >= 0; --k)
                {
                  d_x_position = (k < current_rectangle.width - 1) ? k + 1 : k;
                  d = transforms_buffer[j * current_rectangle.width + d_x_position];
                  w = rf_table[n][d];
        
                  current_pixel_r_f = image_buffer[j * current_rectangle.width * image_channels + k * image_channels];
                  current_pixel_g_f = image_buffer[j * current_rectangle.width * image_channels + k * image_channels + 1];
                  current_pixel_b_f = image_buffer[j * current_rectangle.width * image_channels + k * image_channels + 2];
                  current_pixel_a_f = image_buffer[j * current_rectangle.width * image_channels + k * image_channels + 3];
        
                  last_pixel_r_f = ((1 - w) * current_pixel_r_f + w * last_pixel_r_f);
                  last_pixel_g_f = ((1 - w) * current_pixel_g_f + w * last_pixel_g_f);
                  last_pixel_b_f = ((1 - w) * current_pixel_b_f + w * last_pixel_b_f);
                  last_pixel_a_f = ((1 - w) * current_pixel_a_f + w * last_pixel_a_f);
        
                  image_buffer[j * current_rectangle.width * image_channels + k * image_channels] = last_pixel_r_f;
                  image_buffer[j * current_rectangle.width * image_channels + k * image_channels + 1] = last_pixel_g_f;
                  image_buffer[j * current_rectangle.width * image_channels + k * image_channels + 2] = last_pixel_b_f;
                  image_buffer[j * current_rectangle.width * image_channels + k * image_channels + 3] = last_pixel_a_f;
                }
            }
    
          gegl_buffer_set (output, &current_rectangle, 0, babl_format ("R'G'B'A float"), image_buffer, GEGL_AUTO_ROWSTRIDE);
        }
  
      report_progress(operation, (2.0 * n + 1.0) / (2.0 * num_iterations), timer);
  
      // Vertical Pass
      for (i = 0; i < image_width; i += BLOCK_STRIDE)
        {
          real_stride = (i + BLOCK_STRIDE > image_width) ? image_width - i : BLOCK_STRIDE;
    
          current_rectangle.x = i;
          current_rectangle.y = 0;
          current_rectangle.width = real_stride;
          current_rectangle.height = image_height;
    
          gegl_buffer_get (input, &current_rectangle, 1.0, babl_format ("R'G'B' u8"), image_buffer, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
    
          // Domain Transform
          for (j = 0; j < current_rectangle.width; ++j)
            {
              last_pixel_r = ((guint8*)image_buffer)[j * 3];
              last_pixel_g = ((guint8*)image_buffer)[j * 3 + 1];
              last_pixel_b = ((guint8*)image_buffer)[j * 3 + 2];
      
              for (k = 0; k < current_rectangle.height; ++k)
                {
                  current_pixel_r = ((guint8*)image_buffer)[k * current_rectangle.width * 3 + j * 3];
                  current_pixel_g = ((guint8*)image_buffer)[k * current_rectangle.width * 3 + j * 3 + 1];
                  current_pixel_b = ((guint8*)image_buffer)[k * current_rectangle.width * 3 + j * 3 + 2];
        
                  sum_channels_difference = absolute(current_pixel_r - last_pixel_r) + absolute(current_pixel_g - last_pixel_g) + absolute(current_pixel_b - last_pixel_b);
        
                  // @NOTE: 'd' should be 1.0f + s_s / s_r * sum_diff
                  // However, we will store just sum_diff.
                  // 1.0f + s_s / s_r will be calculated later when calculating the RF table.
                  // This is done this way because the sum_diff is perfect to be used as the index of the RF table.
                  // d = 1.0f + (vdt_information->spatial_factor / vdt_information->range_factor) * sum_channels_difference;
                  transforms_buffer[k * current_rectangle.width + j] = sum_channels_difference;
        
                  last_pixel_r = current_pixel_r;
                  last_pixel_g = current_pixel_g;
                  last_pixel_b = current_pixel_b;
                }
            }
    
          gegl_buffer_get (output, &current_rectangle, 1.0, babl_format ("R'G'B'A float"), image_buffer, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
    
          // Vertical Filter (Top-Down)
          for (j = 0; j < current_rectangle.width; ++j)
            {
              last_pixel_r_f = image_buffer[j * image_channels];
              last_pixel_g_f = image_buffer[j * image_channels + 1];
              last_pixel_b_f = image_buffer[j * image_channels + 2];
              last_pixel_a_f = image_buffer[j * image_channels + 3];
      
              for (k = 0; k < current_rectangle.height; ++k)
                {
                  d = transforms_buffer[k * current_rectangle.width + j];
                  w = rf_table[n][d];
                  
                  current_pixel_r_f = image_buffer[k * current_rectangle.width * image_channels + j * image_channels];
                  current_pixel_g_f = image_buffer[k * current_rectangle.width * image_channels + j * image_channels + 1];
                  current_pixel_b_f = image_buffer[k * current_rectangle.width * image_channels + j * image_channels + 2];
                  current_pixel_a_f = image_buffer[k * current_rectangle.width * image_channels + j * image_channels + 3];
        
                  last_pixel_r_f = ((1 - w) * current_pixel_r_f + w * last_pixel_r_f);
                  last_pixel_g_f = ((1 - w) * current_pixel_g_f + w * last_pixel_g_f);
                  last_pixel_b_f = ((1 - w) * current_pixel_b_f + w * last_pixel_b_f);
                  last_pixel_a_f = ((1 - w) * current_pixel_a_f + w * last_pixel_a_f);
        
                  image_buffer[k * current_rectangle.width * image_channels + j * image_channels] = last_pixel_r_f;
                  image_buffer[k * current_rectangle.width * image_channels + j * image_channels + 1] = last_pixel_g_f;
                  image_buffer[k * current_rectangle.width * image_channels + j * image_channels + 2] = last_pixel_b_f;
                  image_buffer[k * current_rectangle.width * image_channels + j * image_channels + 3] = last_pixel_a_f;
                }
            }
    
          // Vertical Filter (Bottom-Up)
          for (j = 0; j < current_rectangle.width; ++j)
            {
              last_pixel_r_f = image_buffer[(current_rectangle.height - 1) * current_rectangle.width * image_channels + j * image_channels];
              last_pixel_g_f = image_buffer[(current_rectangle.height - 1) * current_rectangle.width * image_channels + j * image_channels + 1];
              last_pixel_b_f = image_buffer[(current_rectangle.height - 1) * current_rectangle.width * image_channels + j * image_channels + 2];
              last_pixel_a_f = image_buffer[(current_rectangle.height - 1) * current_rectangle.width * image_channels + j * image_channels + 3];
              
              for (k = current_rectangle.height - 1; k >= 0; --k)
                {
                  d_y_position = (k < current_rectangle.height - 1) ? k + 1 : k;
                  d = transforms_buffer[d_y_position * current_rectangle.width + j];
                  w = rf_table[n][d];
        
                  current_pixel_r_f = image_buffer[k * current_rectangle.width * image_channels + j * image_channels];
                  current_pixel_g_f = image_buffer[k * current_rectangle.width * image_channels + j * image_channels + 1];
                  current_pixel_b_f = image_buffer[k * current_rectangle.width * image_channels + j * image_channels + 2];
                  current_pixel_a_f = image_buffer[k * current_rectangle.width * image_channels + j * image_channels + 3];
        
                  last_pixel_r_f = ((1 - w) * current_pixel_r_f + w * last_pixel_r_f);
                  last_pixel_g_f = ((1 - w) * current_pixel_g_f + w * last_pixel_g_f);
                  last_pixel_b_f = ((1 - w) * current_pixel_b_f + w * last_pixel_b_f);
                  last_pixel_a_f = ((1 - w) * current_pixel_a_f + w * last_pixel_a_f);
        
                  image_buffer[k * current_rectangle.width * image_channels + j * image_channels] = last_pixel_r_f;
                  image_buffer[k * current_rectangle.width * image_channels + j * image_channels + 1] = last_pixel_g_f;
                  image_buffer[k * current_rectangle.width * image_channels + j * image_channels + 2] = last_pixel_b_f;
                  image_buffer[k * current_rectangle.width * image_channels + j * image_channels + 3] = last_pixel_a_f;
                }
            }
    
          gegl_buffer_set (output, &current_rectangle, 0, babl_format ("R'G'B'A float"), image_buffer, GEGL_AUTO_ROWSTRIDE);
        }
  
      report_progress(operation, (2.0 * n + 2.0) / (2.0 * num_iterations), timer);
    }

  g_free(transforms_buffer);
  g_free(image_buffer);
  for (i = 0; i < num_iterations; ++i)
    g_free(rf_table[i]);
  g_free(rf_table);

  g_timer_destroy(timer);

  return 0;
}

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("R'G'B'A float"));
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B'A float"));
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  /* Don't request an infinite plane */
  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;

  return result;
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
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties         *o;
  gint image_height, image_width, image_channels;
  gfloat spatial_factor, edge_preservation, range_factor, num_iterations;

  o = GEGL_PROPERTIES (operation);
  image_height = result->height;
  image_width = result->width;
  image_channels = 4;

  spatial_factor = o->spatial_factor;
  edge_preservation = o->edge_preservation;
  num_iterations = o->num_iterations;

  if (edge_preservation != 1.0f)
    {
      if (edge_preservation != 0.0f)
        range_factor = (1.0f / edge_preservation) - 1.0f;
      else
        range_factor = G_MAXFLOAT;
  
      // Buffer is ready for domain transform
      domain_transform(operation,
        image_width,
        image_height,
        image_channels,
        spatial_factor,
        range_factor,
        num_iterations,
        input,
        output);
    }
  else
    {
      gegl_buffer_copy(input, result, GEGL_ABYSS_NONE, output, result);
    }

  return TRUE;
}

/* Pass-through when trying to perform a reduction on an infinite plane
 */
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
    gegl_operation_context_take_object (context, "output", g_object_ref (G_OBJECT (in)));
    return TRUE;
    }

  /* chain up, which will create the needed buffers for our actual
  * process function
  */
  return operation_class->process (operation, context, output_prop, result,
    gegl_operation_context_get_level (context));
}

/* This is called at the end of the gobject class_init function.
 *
 * Here we override the standard passthrough options for the rect
 * computations.
 */
static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare = prepare;
  operation_class->threaded = FALSE;
  operation_class->process = operation_process;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region = get_cached_region;
  operation_class->opencl_support = FALSE;

  gegl_operation_class_set_keys (operation_class,
      "name",        "gegl:domain-transform",
      "title",       _("Smooth by Domain Transform"),
      "categories" , "enhance:noise-reduction",
      "description", _("An edge-preserving smoothing filter implemented with the Domain Transform recursive technique. Similar to a bilateral filter, but faster to compute."),
      NULL);
}

#endif
