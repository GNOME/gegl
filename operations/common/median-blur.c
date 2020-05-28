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
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <stdlib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_median_blur_neighborhood)
  enum_value (GEGL_MEDIAN_BLUR_NEIGHBORHOOD_SQUARE,  "square",  N_("Square"))
  enum_value (GEGL_MEDIAN_BLUR_NEIGHBORHOOD_CIRCLE,  "circle",  N_("Circle"))
  enum_value (GEGL_MEDIAN_BLUR_NEIGHBORHOOD_DIAMOND, "diamond", N_("Diamond"))
enum_end (GeglMedianBlurNeighborhood)

enum_start (gegl_median_blur_abyss_policy)
   enum_value (GEGL_MEDIAN_BLUR_ABYSS_NONE,  "none",  N_("None"))
   enum_value (GEGL_MEDIAN_BLUR_ABYSS_CLAMP, "clamp", N_("Clamp"))
enum_end (GeglMedianBlurAbyssPolicy)

property_enum (neighborhood, _("Neighborhood"),
               GeglMedianBlurNeighborhood, gegl_median_blur_neighborhood,
               GEGL_MEDIAN_BLUR_NEIGHBORHOOD_CIRCLE)
  description (_("Neighborhood type"))

property_int  (radius, _("Radius"), 3)
  value_range (-400, 400)
  ui_range    (0, 100)
  ui_meta     ("unit", "pixel-distance")
  description (_("Neighborhood radius, a negative value will calculate with inverted percentiles"))

property_double  (percentile, _("Percentile"), 50)
  value_range (0, 100)
  description (_("Neighborhood color percentile"))

property_double  (alpha_percentile, _("Alpha percentile"), 50)
  value_range (0, 100)
  description (_("Neighborhood alpha percentile"))

property_enum (abyss_policy, _("Abyss policy"), GeglMedianBlurAbyssPolicy,
               gegl_median_blur_abyss_policy, GEGL_MEDIAN_BLUR_ABYSS_CLAMP)
  description (_("How image edges are handled"))

property_boolean (high_precision, _("High precision"), FALSE)
  description (_("Avoid clipping and quantization (slower)"))

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME         median_blur
#define GEGL_OP_C_SOURCE     median-blur.c

#include "gegl-op.h"

#define DEFAULT_N_BINS   256
#define MAX_CHUNK_WIDTH  128
#define MAX_CHUNK_HEIGHT 128

#define SAFE_CLAMP(x, min, max) ((x) > (min) ? (x) < (max) ? (x) : (max) : (min))

static gfloat        default_bin_values[DEFAULT_N_BINS];
static gint          default_alpha_values[DEFAULT_N_BINS];
static volatile gint default_values_initialized = FALSE;

typedef struct
{
  gboolean  quantize;
  gint     *neighborhood_outline;
} UserData;

typedef struct
{
  gfloat value;
  gint   index;
} InputValue;

typedef struct
{
  gint   *bins;
  gfloat *bin_values;
  gint    last_median;
  gint    last_median_sum;
} HistogramComponent;

typedef struct
{
  HistogramComponent  components[4];
  gint               *alpha_values;
  gint                count;
  gint                size;
  gint                n_components;
  gint                n_color_components;
} Histogram;

typedef enum
{
  LEFT_TO_RIGHT,
  RIGHT_TO_LEFT,
  TOP_TO_BOTTOM
} Direction;

static inline gfloat
histogram_get_median (Histogram *hist,
                      gint       component,
                      gdouble    percentile)
{
  gint                count = hist->count;
  HistogramComponent *comp  = &hist->components[component];
  gint                i     = comp->last_median;
  gint                sum   = comp->last_median_sum;

  if (component == hist->n_color_components)
    count = hist->size;

  if (count == 0)
    return 0.0f;

  count = (gint) ceil (count * percentile);
  count = MAX (count, 1);

  if (sum < count)
    {
      while ((sum += comp->bins[++i]) < count);
    }
  else
    {
      while ((sum -= comp->bins[i--]) >= count);
      sum += comp->bins[++i];
    }

  comp->last_median     = i;
  comp->last_median_sum = sum;

  return comp->bin_values[i];
}

static inline void
histogram_modify_val (Histogram    *hist,
                      const gint32 *src,
                      gint          diff,
                      gint          n_color_components,
                      gboolean      has_alpha)
{
  gint alpha = diff;
  gint c;

  if (has_alpha)
    alpha *= hist->alpha_values[src[n_color_components]];

  for (c = 0; c < n_color_components; c++)
    {
      HistogramComponent *comp = &hist->components[c];
      gint                bin  = src[c];

      comp->bins[bin] += alpha;

      /* this is shorthand for:
       *
       *   if (bin <= comp->last_median)
       *     comp->last_median_sum += alpha;
       *
       * but with a notable speed boost.
       */
      comp->last_median_sum += (bin <= comp->last_median) * alpha;
    }

  if (has_alpha)
    {
      HistogramComponent *comp = &hist->components[n_color_components];
      gint                bin  = src[n_color_components];

      comp->bins[bin] += diff;

      comp->last_median_sum += (bin <= comp->last_median) * diff;
    }

  hist->count += alpha;
}

static inline void
histogram_modify_vals (Histogram    *hist,
                       const gint32 *src,
                       gint          stride,
                       gint          xmin,
                       gint          ymin,
                       gint          xmax,
                       gint          ymax,
                       gint          diff)
{
  gint     n_components       = hist->n_components;
  gint     n_color_components = hist->n_color_components;
  gboolean has_alpha          = n_color_components < n_components;
  gint x;
  gint y;

  if (xmin > xmax || ymin > ymax)
    return;

  src += ymin * stride + xmin * n_components;

  if (n_color_components == 3)
    {
      if (has_alpha)
        {
          for (y = ymin; y <= ymax; y++, src += stride)
            {
              const gint32 *pixel = src;

              for (x = xmin; x <= xmax; x++, pixel += n_components)
                {
                  histogram_modify_val (hist, pixel, diff, 3, TRUE);
                }
            }
        }
      else
        {
          for (y = ymin; y <= ymax; y++, src += stride)
            {
              const gint32 *pixel = src;

              for (x = xmin; x <= xmax; x++, pixel += n_components)
                {
                  histogram_modify_val (hist, pixel, diff, 3, FALSE);
                }
            }
        }
    }
  else
    {
      if (has_alpha)
        {
          for (y = ymin; y <= ymax; y++, src += stride)
            {
              const gint32 *pixel = src;

              for (x = xmin; x <= xmax; x++, pixel += n_components)
                {
                  histogram_modify_val (hist, pixel, diff, 1, TRUE);
                }
            }
        }
      else
        {
          for (y = ymin; y <= ymax; y++, src += stride)
            {
              const gint32 *pixel = src;

              for (x = xmin; x <= xmax; x++, pixel += n_components)
                {
                  histogram_modify_val (hist, pixel, diff, 1, FALSE);
                }
            }
        }
    }
}

static inline void
histogram_update (Histogram                  *hist,
                  const gint32               *src,
                  gint                        stride,
                  GeglMedianBlurNeighborhood  neighborhood,
                  gint                        radius,
                  const gint                 *neighborhood_outline,
                  Direction                   dir)
{
  gint i;

  switch (neighborhood)
    {
    case GEGL_MEDIAN_BLUR_NEIGHBORHOOD_SQUARE:
      switch (dir)
        {
          case LEFT_TO_RIGHT:
            histogram_modify_vals (hist, src, stride,
                                   -radius - 1, -radius,
                                   -radius - 1, +radius,
                                   -1);

            histogram_modify_vals (hist, src, stride,
                                   +radius, -radius,
                                   +radius, +radius,
                                   +1);
            break;

          case RIGHT_TO_LEFT:
            histogram_modify_vals (hist, src, stride,
                                   +radius + 1, -radius,
                                   +radius + 1, +radius,
                                   -1);

            histogram_modify_vals (hist, src, stride,
                                   -radius, -radius,
                                   -radius, +radius,
                                   +1);
            break;

          case TOP_TO_BOTTOM:
            histogram_modify_vals (hist, src, stride,
                                   -radius, -radius - 1,
                                   +radius, -radius - 1,
                                   -1);

            histogram_modify_vals (hist, src, stride,
                                   -radius, +radius,
                                   +radius, +radius,
                                   +1);
            break;
        }
      break;

    default:
      switch (dir)
        {
          case LEFT_TO_RIGHT:
            for (i = 0; i < radius; i++)
              {
                histogram_modify_vals (hist, src, stride,
                                       -i - 1, -neighborhood_outline[i],
                                       -i - 1, -neighborhood_outline[i + 1] - 1,
                                       -1);
                histogram_modify_vals (hist, src, stride,
                                       -i - 1, +neighborhood_outline[i + 1] + 1,
                                       -i - 1, +neighborhood_outline[i],
                                       -1);

                histogram_modify_vals (hist, src, stride,
                                       +i, -neighborhood_outline[i],
                                       +i, -neighborhood_outline[i + 1] - 1,
                                       +1);
                histogram_modify_vals (hist, src, stride,
                                       +i, +neighborhood_outline[i + 1] + 1,
                                       +i, +neighborhood_outline[i],
                                       +1);
              }

            histogram_modify_vals (hist, src, stride,
                                   -i - 1, -neighborhood_outline[i],
                                   -i - 1, +neighborhood_outline[i],
                                   -1);

            histogram_modify_vals (hist, src, stride,
                                   +i, -neighborhood_outline[i],
                                   +i, +neighborhood_outline[i],
                                   +1);

            break;

          case RIGHT_TO_LEFT:
            for (i = 0; i < radius; i++)
              {
                histogram_modify_vals (hist, src, stride,
                                       +i + 1, -neighborhood_outline[i],
                                       +i + 1, -neighborhood_outline[i + 1] - 1,
                                       -1);
                histogram_modify_vals (hist, src, stride,
                                       +i + 1, +neighborhood_outline[i + 1] + 1,
                                       +i + 1, +neighborhood_outline[i],
                                       -1);

                histogram_modify_vals (hist, src, stride,
                                       -i, -neighborhood_outline[i],
                                       -i, -neighborhood_outline[i + 1] - 1,
                                       +1);
                histogram_modify_vals (hist, src, stride,
                                       -i, +neighborhood_outline[i + 1] + 1,
                                       -i, +neighborhood_outline[i],
                                       +1);
              }

            histogram_modify_vals (hist, src, stride,
                                   +i + 1, -neighborhood_outline[i],
                                   +i + 1, +neighborhood_outline[i],
                                   -1);

            histogram_modify_vals (hist, src, stride,
                                   -i, -neighborhood_outline[i],
                                   -i, +neighborhood_outline[i],
                                   +1);

            break;

          case TOP_TO_BOTTOM:
            for (i = 0; i < radius; i++)
              {
                histogram_modify_vals (hist, src, stride,
                                       -neighborhood_outline[i],         -i - 1,
                                       -neighborhood_outline[i + 1] - 1, -i - 1,
                                       -1);
                histogram_modify_vals (hist, src, stride,
                                       +neighborhood_outline[i + 1] + 1, -i - 1,
                                       +neighborhood_outline[i],         -i - 1,
                                       -1);

                histogram_modify_vals (hist, src, stride,
                                       -neighborhood_outline[i],         +i,
                                       -neighborhood_outline[i + 1] - 1, +i,
                                       +1);
                histogram_modify_vals (hist, src, stride,
                                       +neighborhood_outline[i + 1] + 1, +i,
                                       +neighborhood_outline[i],         +i,
                                       +1);
              }

            histogram_modify_vals (hist, src, stride,
                                   -neighborhood_outline[i], -i - 1,
                                   +neighborhood_outline[i], -i - 1,
                                   -1);

            histogram_modify_vals (hist, src, stride,
                                   -neighborhood_outline[i], +i,
                                   +neighborhood_outline[i], +i,
                                   +1);

            break;
        }
      break;
    }
}

static void
init_neighborhood_outline (GeglMedianBlurNeighborhood  neighborhood,
                           gint                        radius,
                           gint                       *neighborhood_outline)
{
  gint i;

  for (i = 0; i <= radius; i++)
    {
      switch (neighborhood)
        {
        case GEGL_MEDIAN_BLUR_NEIGHBORHOOD_SQUARE:
          neighborhood_outline[i] = radius;
          break;

        case GEGL_MEDIAN_BLUR_NEIGHBORHOOD_CIRCLE:
          neighborhood_outline[i] =
            (gint) sqrt ((radius + .5) * (radius + .5) - i * i);
          break;

        case GEGL_MEDIAN_BLUR_NEIGHBORHOOD_DIAMOND:
          neighborhood_outline[i] = radius - i;
          break;
        }
    }
}

static void
sort_input_values (InputValue **values,
                   InputValue **scratch,
                   gint         n_values)
{
  const InputValue *in     = *values;
  InputValue       *out    = *scratch;
  gint              size;

  for (size = 1; size < n_values; size *= 2)
    {
      InputValue *temp;
      gint        i;

      for (i = 0; i < n_values; )
        {
          gint l     = i;
          gint l_end = MIN (l + size, n_values);
          gint r     = l_end;
          gint r_end = MIN (r + size, n_values);

          while (l < l_end && r < r_end)
            {
              if (in[r].value < in[l].value)
                out[i] = in[r++];
              else
                out[i] = in[l++];

              i++;
            }

          memcpy (&out[i], &in[l], (l_end - l) * sizeof (InputValue));
          i += l_end - l;

          memcpy (&out[i], &in[r], (r_end - r) * sizeof (InputValue));
          i += r_end - r;
        }

      temp = (InputValue *) in;
      in   = out;
      out  = temp;
    }

  *values  = (InputValue *) in;
  *scratch = out;
}

static void
convert_values_to_bins (Histogram *hist,
                        gint32    *src,
                        gint       n_pixels,
                        gboolean   quantize)
{
  gint     n_components       = hist->n_components;
  gint     n_color_components = hist->n_color_components;
  gboolean has_alpha          = n_color_components < n_components;
  gint     i;
  gint     c;

  if (quantize)
    {
      for (c = 0; c < n_components; c++)
        {
          hist->components[c].bins       = g_new0 (gint, DEFAULT_N_BINS);
          hist->components[c].bin_values = default_bin_values;
        }

      while (n_pixels--)
        {
          for (c = 0; c < n_components; c++)
            {
              gfloat value = ((gfloat *) src)[c];
              gint   bin;

              bin = floorf (SAFE_CLAMP (value, 0.0f, 1.0f) * (DEFAULT_N_BINS - 1) + 0.5f);

              src[c] = bin;
            }

          src += n_components;
        }

      hist->alpha_values = default_alpha_values;
    }
  else
    {
      InputValue *values       = g_new (InputValue, n_pixels);
      InputValue *scratch      = g_new (InputValue, n_pixels);
      gint       *alpha_values = NULL;

      if (has_alpha)
        {
          alpha_values = g_new (gint, n_pixels);

          hist->alpha_values = alpha_values;
        }

      for (i = 0; i < n_pixels; i++)
        {
          values[i].value = ((gfloat *) src)[i * n_components];
          values[i].index = i;
        }

      for (c = 0; c < n_components; c++)
        {
          gint    bin = 0;
          gfloat  prev_value;
          gfloat *bin_values;

          sort_input_values (&values, &scratch, n_pixels);

          prev_value = values[0].value;

          bin_values    = g_new (gfloat, n_pixels);
          bin_values[0] = prev_value;
          if (c == n_color_components)
            {
              alpha_values[0] = floorf (SAFE_CLAMP (prev_value, 0.0f, 1.0f) *
                                        (gfloat) (1 << 10) + 0.5f);
            }

          for (i = 0; i < n_pixels; i++)
            {
              gint32 *p = &src[values[i].index * n_components + c];

              if (values[i].value != prev_value)
                {
                  bin++;
                  prev_value      = values[i].value;
                  bin_values[bin] = prev_value;
                  if (c == n_color_components)
                    {
                      alpha_values[bin] = floorf (SAFE_CLAMP (prev_value, 0.0f, 1.0f) *
                                                  (gfloat) (1 << 10) + 0.5f);
                    }
                }

              *p = bin;
              if (c < n_components - 1)
                values[i].value = ((gfloat *) p)[1];
            }

          hist->components[c].bins       = g_new0 (gint, bin + 1);
          hist->components[c].bin_values = bin_values;
        }

      g_free (scratch);
      g_free (values);
    }
}

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area      = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties          *o         = GEGL_PROPERTIES (operation);
  const Babl              *in_format = gegl_operation_get_source_format (operation, "input");
  const Babl              *format    = NULL;
  UserData                *data;
  gint                     radius    = abs (o->radius);

  area->left   =
  area->right  =
  area->top    =
  area->bottom = radius;

  if (! o->user_data)
    o->user_data = g_slice_new0 (UserData);

  data                       = o->user_data;
  data->quantize             = ! o->high_precision;
  data->neighborhood_outline = g_renew (gint, data->neighborhood_outline,
                                        radius + 1);
  init_neighborhood_outline (o->neighborhood, radius,
                             data->neighborhood_outline);

  if (in_format)
    {
      const Babl *model = babl_format_get_model (in_format);

      if (o->high_precision)
        {
          if (babl_model_is (model, "Y"))
            format = babl_format_with_space ("Y float", in_format);
          else if (babl_model_is (model, "Y'"))
            format = babl_format_with_space ("Y' float", in_format);
          else if (babl_model_is (model, "YA") || babl_model_is (model, "YaA"))
            format = babl_format_with_space ("YA float", in_format);
          else if (babl_model_is (model, "Y'A") || babl_model_is (model, "Y'aA"))
            format = babl_format_with_space ("Y'A float", in_format);
          else if (babl_model_is (model, "RGB"))
            format = babl_format_with_space ("RGB float", in_format);
          else if (babl_model_is (model, "R'G'B'"))
            format = babl_format_with_space ("R'G'B' float", in_format);
          else if (babl_model_is (model, "RGBA") || babl_model_is (model, "RaGaBaA"))
            format = babl_format_with_space ("RGBA float", in_format);
          else if (babl_model_is (model, "R'G'B'A") || babl_model_is (model, "R'aG'aB'aA"))
            format = babl_format_with_space ("R'G'B'A float", in_format);

          if (format)
            {
              gint n_components = babl_format_get_n_components (in_format);
              gint i;

              data->quantize = TRUE;

              for (i = 0; i < n_components; i++)
                {
                  if (babl_format_get_type (in_format, i) != babl_type ("u8"))
                    {
                      data->quantize = FALSE;
                      break;
                    }
                }
            }
        }
      else
        {
          if (babl_model_is (model, "Y") || babl_model_is (model, "Y'"))
            format = babl_format_with_space ("Y' float", in_format);
          else if (babl_model_is (model, "YA")  || babl_model_is (model, "YaA") ||
                   babl_model_is (model, "Y'A") || babl_model_is (model, "Y'aA"))
            format = babl_format_with_space ("Y'A float", in_format);
          else if (babl_model_is (model, "RGB") || babl_model_is (model, "R'G'B'"))
            format = babl_format_with_space ("R'G'B' float", in_format);
          else if (babl_model_is (model, "RGBA")    || babl_model_is (model, "RaGaBaA") ||
                   babl_model_is (model, "R'G'B'A") || babl_model_is (model, "R'aG'aB'aA"))
            format = babl_format_with_space ("R'G'B'A float", in_format);
        }

      if (format == NULL)
        {
          if (babl_format_has_alpha (in_format))
            format = babl_format_with_space ("R'G'B'A float", in_format);
          else
            format = babl_format_with_space ("R'G'B' float", in_format);
        }
    }
  else
    {
      if (o->high_precision)
        format = babl_format_with_space ("RGBA float", in_format);
      else
        format = babl_format_with_space ("R'G'B'A float", in_format);
    }

  if (data->quantize && ! g_atomic_int_get (&default_values_initialized))
    {
      gint i;

      for (i = 0; i < DEFAULT_N_BINS; i++)
        {
          default_bin_values[i]   = (gfloat) i / (gfloat) (DEFAULT_N_BINS - 1);
          default_alpha_values[i] = i;
        }

      g_atomic_int_set (&default_values_initialized, TRUE);
    }

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  if (o->abyss_policy != GEGL_MEDIAN_BLUR_ABYSS_NONE)
    {
      GeglRectangle  result = { 0, 0, 0, 0 };
      GeglRectangle *in_rect;

      in_rect = gegl_operation_source_get_bounding_box (operation, "input");

      if (in_rect)
        result = *in_rect;

      return result;
    }

  return GEGL_OPERATION_CLASS (gegl_op_parent_class)->get_bounding_box (operation);
}

static GeglAbyssPolicy
get_abyss_policy (GeglOperation *operation,
                  const gchar   *input_pad)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  switch (o->abyss_policy)
    {
    case GEGL_MEDIAN_BLUR_ABYSS_NONE:  return GEGL_ABYSS_NONE;
    case GEGL_MEDIAN_BLUR_ABYSS_CLAMP: return GEGL_ABYSS_CLAMP;
    }

  g_return_val_if_reached (GEGL_ABYSS_NONE);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o    = GEGL_PROPERTIES (operation);
  UserData       *data = o->user_data;

  gint            radius               = abs (o->radius);
  gdouble         percentile           = o->percentile       / 100.0;
  gdouble         alpha_percentile     = o->alpha_percentile / 100.0;
  const gint     *neighborhood_outline = data->neighborhood_outline;

  const Babl     *format               = gegl_operation_get_format (operation, "input");
  gint            n_components         = babl_format_get_n_components (format);
  gint            n_color_components   = n_components;
  gboolean        has_alpha            = babl_format_has_alpha (format);

  G_STATIC_ASSERT (sizeof (gint32) == sizeof (gfloat));
  gint32         *src_buf;
  gfloat         *dst_buf;
  GeglRectangle   src_rect;
  gint            src_stride;
  gint            dst_stride;
  gint            n_src_pixels;
  gint            n_dst_pixels;

  Histogram      *hist;

  const gint32   *src;
  gfloat         *dst;
  gint            dst_x, dst_y;
  Direction       dir;

  gint            i;
  gint            c;

  if (o->radius < 0)
    {
      percentile       = 1.0 - percentile;
      alpha_percentile = 1.0 - alpha_percentile;
    }

  if (! data->quantize &&
      (roi->width > MAX_CHUNK_WIDTH || roi->height > MAX_CHUNK_HEIGHT))
    {
      gint n_x = (roi->width  + MAX_CHUNK_WIDTH  - 1) / MAX_CHUNK_WIDTH;
      gint n_y = (roi->height + MAX_CHUNK_HEIGHT - 1) / MAX_CHUNK_HEIGHT;
      gint x;
      gint y;

      for (y = 0; y < n_y; y++)
        {
          for (x = 0; x < n_x; x++)
            {
              GeglRectangle chunk;

              chunk.x      = roi->x + roi->width  * x       / n_x;
              chunk.y      = roi->y + roi->height * y       / n_y;
              chunk.width  = roi->x + roi->width  * (x + 1) / n_x - chunk.x;
              chunk.height = roi->y + roi->height * (y + 1) / n_y - chunk.y;

              if (! process (operation, input, output, &chunk, level))
                return FALSE;
            }
        }

      return TRUE;
    }

  if (has_alpha)
    n_color_components--;

  g_return_val_if_fail (n_color_components == 1 || n_color_components == 3, FALSE);

  hist = g_slice_new0 (Histogram);

  hist->n_components       = n_components;
  hist->n_color_components = n_color_components;

  src_rect     = gegl_operation_get_required_for_output (operation, "input", roi);
  src_stride   = src_rect.width * n_components;
  dst_stride   = roi->width * n_components;
  n_src_pixels = src_rect.width * src_rect.height;
  n_dst_pixels = roi->width * roi->height;
  src_buf = g_new (gint32, n_src_pixels * n_components);
  dst_buf = g_new (gfloat, n_dst_pixels * n_components);

  gegl_buffer_get (input, &src_rect, 1.0, format, src_buf,
                   GEGL_AUTO_ROWSTRIDE, get_abyss_policy (operation, "input"));
  convert_values_to_bins (hist, src_buf, n_src_pixels, data->quantize);

  src = src_buf + radius * (src_rect.width + 1) * n_components;
  dst = dst_buf;

  /* compute the first window */

  for (i = -radius; i <= radius; i++)
    {
      histogram_modify_vals (hist, src, src_stride,
                             i, -neighborhood_outline[abs (i)],
                             i, +neighborhood_outline[abs (i)],
                             +1);

      hist->size += 2 * neighborhood_outline[abs (i)] + 1;
    }

  for (c = 0; c < n_color_components; c++)
    dst[c] = histogram_get_median (hist, c, percentile);
  if (has_alpha)
    dst[c] = histogram_get_median (hist, c, alpha_percentile);

  dst_x = 0;
  dst_y = 0;

  n_dst_pixels--;
  dir = LEFT_TO_RIGHT;

  while (n_dst_pixels--)
    {
      /* move the src coords based on current direction and positions */
      if (dir == LEFT_TO_RIGHT)
        {
          if (dst_x != roi->width - 1)
            {
              dst_x++;
              src += n_components;
              dst += n_components;
            }
          else
            {
              dst_y++;
              src += src_stride;
              dst += dst_stride;
              dir = TOP_TO_BOTTOM;
            }
        }
      else if (dir == TOP_TO_BOTTOM)
        {
          if (dst_x == 0)
            {
              dst_x++;
              src += n_components;
              dst += n_components;
              dir = LEFT_TO_RIGHT;
            }
          else
            {
              dst_x--;
              src -= n_components;
              dst -= n_components;
              dir = RIGHT_TO_LEFT;
            }
        }
      else if (dir == RIGHT_TO_LEFT)
        {
          if (dst_x != 0)
            {
              dst_x--;
              src -= n_components;
              dst -= n_components;
            }
          else
            {
              dst_y++;
              src += src_stride;
              dst += dst_stride;
              dir = TOP_TO_BOTTOM;
            }
        }

      histogram_update (hist, src, src_stride,
                        o->neighborhood, radius, neighborhood_outline,
                        dir);

      for (c = 0; c < n_color_components; c++)
        dst[c] = histogram_get_median (hist, c, percentile);
      if (has_alpha)
        dst[c] = histogram_get_median (hist, c, alpha_percentile);
    }

  gegl_buffer_set (output, roi, 0, format, dst_buf, GEGL_AUTO_ROWSTRIDE);

  for (c = 0; c < n_components; c++)
    {
      g_free (hist->components[c].bins);

      if (! data->quantize)
        g_free (hist->components[c].bin_values);
    }

  if (! data->quantize && has_alpha)
    g_free (hist->alpha_values);

  g_slice_free (Histogram, hist);
  g_free (dst_buf);
  g_free (src_buf);

  return TRUE;
}

static void
finalize (GObject *object)
{
  GeglOperation  *op = (void*) object;
  GeglProperties *o  = GEGL_PROPERTIES (op);

  if (o->user_data)
    {
      UserData *data = o->user_data;

      g_free (data->neighborhood_outline);
      g_slice_free (UserData, data);
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass                 *object_class;
  GeglOperationClass           *operation_class;
  GeglOperationFilterClass     *filter_class;
  GeglOperationAreaFilterClass *area_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);
  area_class      = GEGL_OPERATION_AREA_FILTER_CLASS (klass);

  object_class->finalize            = finalize;
  filter_class->process             = process;
  operation_class->prepare          = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  area_class->get_abyss_policy      = get_abyss_policy;

  gegl_operation_class_set_keys (operation_class,
    "name",           "gegl:median-blur",
    "title",          _("Median Blur"),
    "categories",     "blur",
    "reference-hash", "1865918d2f3b95690359534bbd58b513",
    "description",    _("Blur resulting from computing the median "
                        "color in the neighborhood of each pixel."),
    NULL);
}

#endif
