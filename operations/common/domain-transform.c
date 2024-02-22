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
 * Copyright 2018 Felipe Einsfeld Kersting <fekersting@inf.ufrgs.br>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int (n_iterations, _("Quality"), 3)
  description(_("Number of filtering iterations. "
                "A value between 2 and 4 is usually enough."))
  value_range (1, 5)

property_double (spatial_factor, _("Blur radius"),  30.0)
  description  (_("Spatial standard deviation of the blur kernel, "
                  "measured in pixels."))
  value_range (0, 1000.0)

property_double (edge_preservation, _("Edge preservation"),  0.8)
  description  (_("Amount of edge preservation. This quantity is inversely "
                  "proportional to the range standard deviation of the blur "
                  "kernel."))
  value_range (0, 1.0)

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     domain_transform
#define GEGL_OP_C_SOURCE domain-transform.c

#include "gegl-op.h"

/* This should be 768, since we have 2^8 possible options for each channel.
 * Since domain transform is given by:
 * 1 + (s_s / s_r) * (diff_channel_R + diff_channel_G + diff_channel_B)
 * We will have 3 * 2^8 different possibilities for the transform of each pixel.
 * 3 * 2^8 = 768
 */
#define RF_TABLE_SIZE 768
#define SQRT3 1.7320508075f
#define SQRT2 1.4142135623f
#define BLOCK_STRIDE 1
#define REPORT_PROGRESS_TIME 0.5  /* time to report gegl_operation_progress */

static gint16
absolute (gint16 x)
{
  return (x < 0) ? -x : x;
}

static void
report_progress (GeglOperation  *operation,
                 gdouble         progress,
                 GTimer         *timer)
{
  static gboolean reported = FALSE;

  if (progress == 0.0)
    reported = FALSE;
    
  if (g_timer_elapsed (timer, NULL) >= REPORT_PROGRESS_TIME && !reported)
    {
      reported = TRUE;
      gegl_operation_progress (operation, 0.0, "");
    }

  if (reported)
    gegl_operation_progress (operation, progress, "");
}

static gint
domain_transform (GeglOperation  *operation,
                  gint            width,
                  gint            height,
                  gint            n_chan,
                  gfloat          spatial_factor,
                  gfloat          range_factor,
                  gint            n_iterations,
                  GeglBuffer     *input,
                  GeglBuffer     *output)
{
  const Babl *space    = gegl_operation_get_source_space (operation, "input");
  const Babl *formatu8 = babl_format_with_space ("R'G'B' u8", space);
  const Babl *format   = babl_format_with_space ("R'G'B'A float", space);
  gfloat  **rf_table;
  guint16  *transforms_buffer;
  gfloat   *buffer;

  gint16    last[3];
  gint16    current[3];
  gfloat    lastf[4];
  gfloat    currentf[4];
  gfloat    sum_channels_difference, a, w, sdt_dev;
  gint      i, j, k, n;
  gint      real_stride, d_x_position, d_y_position, biggest_dimension;
  guint16   d;
  GeglRectangle rect;
  GTimer  *timer;

  timer = g_timer_new ();

  /* PRE-ALLOC MEMORY */
  biggest_dimension = (width > height) ? width : height;
  buffer            = g_new (gfloat, BLOCK_STRIDE * biggest_dimension * n_chan);
  transforms_buffer = g_new (guint16, BLOCK_STRIDE * biggest_dimension);

  rf_table = g_new (gfloat *, n_iterations);

  for (i = 0; i < n_iterations; ++i)
    rf_table[i] = g_new (gfloat, RF_TABLE_SIZE);

  /* ************************ */

  report_progress (operation, 0.0, timer);

  /* Pre-calculate RF table */
  for (i = 0; i < n_iterations; ++i)
    {
      /* calculating RF feedback coefficient 'a' from desired variance
       * 'a' will change each iteration while the domain transform will
       * remain constant
       */
      sdt_dev = spatial_factor * SQRT3 *
                          (powf (2.0f, (gfloat)(n_iterations - (i + 1))) /
                             sqrtf (powf (4.0f, (gfloat) n_iterations) - 1));

      a = expf (-SQRT2 / sdt_dev);

      for (j = 0; j < RF_TABLE_SIZE; ++j)
        {
          rf_table[i][j] = powf (a, 1.0f + (spatial_factor / range_factor) *
                             ((gfloat)j / 255.0f));
        }
    }

  /* Filter Iterations */
  for (n = 0; n < n_iterations; ++n)
    {
      /* Horizontal Pass */
      for (i = 0; i < height; i += BLOCK_STRIDE)
        {
          real_stride = (i + BLOCK_STRIDE > height) ? height - i : BLOCK_STRIDE;
    
          rect.x = 0;
          rect.y = i;
          rect.width = width;
          rect.height = real_stride;
    
          gegl_buffer_get (input, &rect, 1.0, formatu8, buffer,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
    
          /* Domain Transform */
          for (j = 0; j < rect.height; ++j)
            {
              last[0] = ((guint8*) buffer)[j * rect.width * 3];
              last[1] = ((guint8*) buffer)[j * rect.width * 3 + 1];
              last[2] = ((guint8*) buffer)[j * rect.width * 3 + 2];
      
              for (k = 0; k < rect.width; ++k)
                {
                  current[0] = ((guint8*) buffer)[j * rect.width * 3 + k * 3];
                  current[1] = ((guint8*) buffer)[j * rect.width * 3 + k * 3 + 1];
                  current[2] = ((guint8*) buffer)[j * rect.width * 3 + k * 3 + 2];
        
                  sum_channels_difference = absolute (current[0] - last[0]) +
                                            absolute (current[1] - last[1]) +
                                            absolute (current[2] - last[2]);
        
                  /* @NOTE: 'd' should be 1.0f + s_s / s_r * sum_diff
                   * However, we will store just sum_diff.
                   * 1.0f + s_s / s_r will be calculated later when calculating
                   * the RF table. This is done this way because the sum_diff is
                   * perfect to be used as the index of the RF table.
                   * d = 1.0f + (vdt_information->spatial_factor /
                   *   vdt_information->range_factor) * sum_channels_difference;
                   */
                  transforms_buffer[j * rect.width + k] = sum_channels_difference;
        
                  last[0] = current[0];
                  last[1] = current[1];
                  last[2] = current[2];
                }
            }
    
          if (n == 0)
            gegl_buffer_get (input, &rect, 1.0, format, buffer,
                             GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
          else
            gegl_buffer_get (output, &rect, 1.0, format, buffer,
                             GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
    
          /* Horizontal Filter (Left-Right) */
          for (j = 0; j < rect.height; ++j)
            {
              lastf[0] = buffer[j * rect.width * n_chan];
              lastf[1] = buffer[j * rect.width * n_chan + 1];
              lastf[2] = buffer[j * rect.width * n_chan + 2];
              lastf[3] = buffer[j * rect.width * n_chan + 3];
      
              for (k = 0; k < rect.width; ++k)
                {
                  d = transforms_buffer[j * rect.width + k];
                  w = rf_table[n][d];
        
                  currentf[0] = buffer[j * rect.width * n_chan + k * n_chan];
                  currentf[1] = buffer[j * rect.width * n_chan + k * n_chan + 1];
                  currentf[2] = buffer[j * rect.width * n_chan + k * n_chan + 2];
                  currentf[3] = buffer[j * rect.width * n_chan + k * n_chan + 3];
        
                  lastf[0] = ((1 - w) * currentf[0] + w * lastf[0]);
                  lastf[1] = ((1 - w) * currentf[1] + w * lastf[1]);
                  lastf[2] = ((1 - w) * currentf[2] + w * lastf[2]);
                  lastf[3] = ((1 - w) * currentf[3] + w * lastf[3]);
        
                  buffer[j * rect.width * n_chan + k * n_chan]     = lastf[0];
                  buffer[j * rect.width * n_chan + k * n_chan + 1] = lastf[1];
                  buffer[j * rect.width * n_chan + k * n_chan + 2] = lastf[2];
                  buffer[j * rect.width * n_chan + k * n_chan + 3] = lastf[3];
                }
            }
    
          /* Horizontal Filter (Right-Left) */
          for (j = 0; j < rect.height; ++j)
            {
              lastf[0] = buffer[j * rect.width * n_chan + (rect.width - 1) * n_chan];
              lastf[1] = buffer[j * rect.width * n_chan + (rect.width - 1) * n_chan + 1];
              lastf[2] = buffer[j * rect.width * n_chan + (rect.width - 1) * n_chan + 2];
              lastf[3] = buffer[j * rect.width * n_chan + (rect.width - 1) * n_chan + 3];
              
              for (k = rect.width - 1; k >= 0; --k)
                {
                  d_x_position = (k < rect.width - 1) ? k + 1 : k;
                  d = transforms_buffer[j * rect.width + d_x_position];
                  w = rf_table[n][d];
        
                  currentf[0] = buffer[j * rect.width * n_chan + k * n_chan];
                  currentf[1] = buffer[j * rect.width * n_chan + k * n_chan + 1];
                  currentf[2] = buffer[j * rect.width * n_chan + k * n_chan + 2];
                  currentf[3] = buffer[j * rect.width * n_chan + k * n_chan + 3];
        
                  lastf[0] = ((1 - w) * currentf[0] + w * lastf[0]);
                  lastf[1] = ((1 - w) * currentf[1] + w * lastf[1]);
                  lastf[2] = ((1 - w) * currentf[2] + w * lastf[2]);
                  lastf[3] = ((1 - w) * currentf[3] + w * lastf[3]);
        
                  buffer[j * rect.width * n_chan + k * n_chan] = lastf[0];
                  buffer[j * rect.width * n_chan + k * n_chan + 1] = lastf[1];
                  buffer[j * rect.width * n_chan + k * n_chan + 2] = lastf[2];
                  buffer[j * rect.width * n_chan + k * n_chan + 3] = lastf[3];
                }
            }
    
          gegl_buffer_set (output, &rect, 0, format, buffer,
                           GEGL_AUTO_ROWSTRIDE);
        }
  
      report_progress (operation, (2.0 * n + 1.0) / (2.0 * n_iterations), timer);
  
      /* Vertical Pass */
      for (i = 0; i < width; i += BLOCK_STRIDE)
        {
          real_stride = (i + BLOCK_STRIDE > width) ? width - i : BLOCK_STRIDE;
    
          rect.x = i;
          rect.y = 0;
          rect.width = real_stride;
          rect.height = height;
    
          gegl_buffer_get (input, &rect, 1.0, formatu8, buffer,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
    
          /* Domain Transform */
          for (j = 0; j < rect.width; ++j)
            {
              last[0] = ((guint8*) buffer)[j * 3];
              last[1] = ((guint8*) buffer)[j * 3 + 1];
              last[2] = ((guint8*) buffer)[j * 3 + 2];
      
              for (k = 0; k < rect.height; ++k)
                {
                  current[0] = ((guint8*) buffer)[k * rect.width * 3 + j * 3];
                  current[1] = ((guint8*) buffer)[k * rect.width * 3 + j * 3 + 1];
                  current[2] = ((guint8*) buffer)[k * rect.width * 3 + j * 3 + 2];
        
                  sum_channels_difference = absolute (current[0] - last[0]) +
                                            absolute (current[1] - last[1]) +
                                            absolute (current[2] - last[2]);
        
                  /* @NOTE: 'd' should be 1.0f + s_s / s_r * sum_diff
                   * However, we will store just sum_diff.
                   * 1.0f + s_s / s_r will be calculated later when calculating
                   * the RF table. This is done this way because the sum_diff is
                   * perfect to be used as the index of the RF table.
                   * d = 1.0f + (vdt_information->spatial_factor /
                   *   vdt_information->range_factor) * sum_channels_difference;
                   */
                  transforms_buffer[k * rect.width + j] = sum_channels_difference;
        
                  last[0] = current[0];
                  last[1] = current[1];
                  last[2] = current[2];
                }
            }
    
          gegl_buffer_get (output, &rect, 1.0, format, buffer,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
    
          /* Vertical Filter (Top-Down) */
          for (j = 0; j < rect.width; ++j)
            {
              lastf[0] = buffer[j * n_chan];
              lastf[1] = buffer[j * n_chan + 1];
              lastf[2] = buffer[j * n_chan + 2];
              lastf[3] = buffer[j * n_chan + 3];
      
              for (k = 0; k < rect.height; ++k)
                {
                  d = transforms_buffer[k * rect.width + j];
                  w = rf_table[n][d];
                  
                  currentf[0] = buffer[k * rect.width * n_chan + j * n_chan];
                  currentf[1] = buffer[k * rect.width * n_chan + j * n_chan + 1];
                  currentf[2] = buffer[k * rect.width * n_chan + j * n_chan + 2];
                  currentf[3] = buffer[k * rect.width * n_chan + j * n_chan + 3];
        
                  lastf[0] = ((1 - w) * currentf[0] + w * lastf[0]);
                  lastf[1] = ((1 - w) * currentf[1] + w * lastf[1]);
                  lastf[2] = ((1 - w) * currentf[2] + w * lastf[2]);
                  lastf[3] = ((1 - w) * currentf[3] + w * lastf[3]);
        
                  buffer[k * rect.width * n_chan + j * n_chan]     = lastf[0];
                  buffer[k * rect.width * n_chan + j * n_chan + 1] = lastf[1];
                  buffer[k * rect.width * n_chan + j * n_chan + 2] = lastf[2];
                  buffer[k * rect.width * n_chan + j * n_chan + 3] = lastf[3];
                }
            }
    
          /* Vertical Filter (Bottom-Up) */
          for (j = 0; j < rect.width; ++j)
            {
              lastf[0] = buffer[(rect.height - 1) * rect.width * n_chan + j * n_chan];
              lastf[1] = buffer[(rect.height - 1) * rect.width * n_chan + j * n_chan + 1];
              lastf[2] = buffer[(rect.height - 1) * rect.width * n_chan + j * n_chan + 2];
              lastf[3] = buffer[(rect.height - 1) * rect.width * n_chan + j * n_chan + 3];
              
              for (k = rect.height - 1; k >= 0; --k)
                {
                  d_y_position = (k < rect.height - 1) ? k + 1 : k;
                  d = transforms_buffer[d_y_position * rect.width + j];
                  w = rf_table[n][d];
        
                  currentf[0] = buffer[k * rect.width * n_chan + j * n_chan];
                  currentf[1] = buffer[k * rect.width * n_chan + j * n_chan + 1];
                  currentf[2] = buffer[k * rect.width * n_chan + j * n_chan + 2];
                  currentf[3] = buffer[k * rect.width * n_chan + j * n_chan + 3];
        
                  lastf[0] = ((1 - w) * currentf[0] + w * lastf[0]);
                  lastf[1] = ((1 - w) * currentf[1] + w * lastf[1]);
                  lastf[2] = ((1 - w) * currentf[2] + w * lastf[2]);
                  lastf[3] = ((1 - w) * currentf[3] + w * lastf[3]);
        
                  buffer[k * rect.width * n_chan + j * n_chan]     = lastf[0];
                  buffer[k * rect.width * n_chan + j * n_chan + 1] = lastf[1];
                  buffer[k * rect.width * n_chan + j * n_chan + 2] = lastf[2];
                  buffer[k * rect.width * n_chan + j * n_chan + 3] = lastf[3];
                }
            }
    
          gegl_buffer_set (output, &rect, 0, format, buffer,
                           GEGL_AUTO_ROWSTRIDE);
        }
  
      report_progress (operation, (2.0 * n + 2.0) / (2.0 * n_iterations), timer);
    }

  g_free (transforms_buffer);
  g_free (buffer);

  for (i = 0; i < n_iterations; ++i)
    g_free (rf_table[i]);

  g_free (rf_table);
  g_timer_destroy (timer);

  return 0;
}

static void
prepare (GeglOperation *operation)
{
  const Babl *space  = gegl_operation_get_source_space (operation, "input");
  const Babl *format = babl_format_with_space ("R'G'B'A float", space);

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle *result =
      gegl_operation_source_get_bounding_box (operation, "input");

  /* Don't request an infinite plane */
  if (result && gegl_rectangle_is_infinite_plane (result))
    return *roi;

  return *result;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  GeglRectangle *result =
       gegl_operation_source_get_bounding_box (operation, "input");

  if (result && gegl_rectangle_is_infinite_plane (result))
    return *roi;

  return *result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties  *o = GEGL_PROPERTIES (operation);
  gfloat range_factor;

  if (o->edge_preservation != 0.0)
    range_factor = (1.0 / o->edge_preservation) - 1.0f;
  else
    range_factor = G_MAXFLOAT;

  /* Buffer is ready for domain transform */
  domain_transform (operation,
                    result->width,
                    result->height,
                    4,
                    o->spatial_factor,
                    range_factor,
                    o->n_iterations,
                    input,
                    output);

  return TRUE;
}

/* Pass-through when trying to perform a reduction on an infinite plane
 * or when edge preservation is one.
 */
static gboolean
operation_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglProperties      *o = GEGL_PROPERTIES (operation);
  GeglOperationClass  *operation_class;

  const GeglRectangle *in_rect =
    gegl_operation_source_get_bounding_box (operation, "input");

  operation_class = GEGL_OPERATION_CLASS (gegl_op_parent_class);

  if ((in_rect && gegl_rectangle_is_infinite_plane (in_rect)) ||
       o->edge_preservation == 1.0)
    {
      gpointer in = gegl_operation_context_get_object (context, "input");
      gegl_operation_context_take_object (context, "output",
                                          g_object_ref (G_OBJECT (in)));
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

  filter_class->process                    = process;
  operation_class->prepare                 = prepare;
  operation_class->threaded                = FALSE;
  operation_class->process                 = operation_process;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region       = get_cached_region;
  operation_class->opencl_support          = FALSE;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:domain-transform",
    "title",       _("Smooth by Domain Transform"),
    "categories" , "enhance:noise-reduction",
    "reference-hash", "8755fd14807dbd5ac1d7a31c02865a63",
    "description", _("An edge-preserving smoothing filter implemented with the "
                     "Domain Transform recursive technique. Similar to a "
                     "bilateral filter, but faster to compute."),
    NULL);
}

#endif
