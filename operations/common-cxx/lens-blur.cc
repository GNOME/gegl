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
 * Copyright (C) 2017 Ell
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (radius, _("Radius"), 10.0)
  description (_("Blur radius"))
  value_range (0.0, G_MAXDOUBLE)
  ui_range    (0.0, 100.0)
  ui_gamma    (2.0)
  ui_meta     ("unit", "pixel-distance")

property_double (highlight_factor, _("Highlight factor"), 0.0)
  description (_("Relative highlight strength"))
  value_range (0.0, 1.0)

property_double (highlight_threshold_low, _("Highlight threshold (low)"), 0.0)
  ui_range    (0.0, 1.0)
  ui_meta     ("role", "range-start")
  ui_meta     ("unit", "luminance")
  ui_meta     ("range-label", _("Highlight threshold"))

property_double (highlight_threshold_high, _("Highlight threshold (high)"), 1.0)
  ui_range    (0.0, 1.0)
  ui_meta     ("role", "range-end")
  ui_meta     ("unit", "luminance")

property_boolean (clip, _("Clip to input extents"), TRUE)
  description (_("Clip output to the input extents"))

property_boolean (linear_mask, _("Linear mask"), FALSE)
  description (_("Use linear mask values"))

#else

#define GEGL_OP_COMPOSER
#define GEGL_OP_NAME     lens_blur
#define GEGL_OP_C_SOURCE lens-blur.cc

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl     *space;
  const Babl     *format;

  space  = gegl_operation_get_source_space (operation, "input");
  format = babl_format_with_space ("RGBA float", space);

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);

  gegl_operation_set_format (operation, "aux",
                             babl_format_with_space (
                               o->linear_mask ? "Y float" : "Y' float",
                               gegl_operation_get_source_space (operation,
                                                                "aux")));

  o->user_data = (gpointer) babl_fish (format,
                                       babl_format_with_space ("Y float",
                                                               space));
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglProperties      *o      = GEGL_PROPERTIES (operation);
  const GeglRectangle *in_rect;
  GeglRectangle        result = {};

  in_rect = gegl_operation_source_get_bounding_box (operation, "input");

  if (in_rect)
    {
      result = *in_rect;

      if (! o->clip)
        {
          gint iradius = floor (o->radius + 0.5);

          result.x      -= iradius;
          result.y      -= iradius;
          result.width  += 2 * iradius;
          result.height += 2 * iradius;
        }
    }

  return result;
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglProperties *o       = GEGL_PROPERTIES (operation);
  GeglRectangle   result  = *roi;
  gint            iradius = floor (o->radius + 0.5);

  result.x      -= iradius;
  result.y      -= iradius;
  result.width  += 2 * iradius;
  result.height += 2 * iradius;

  return result;
}

static gboolean
operation_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *roi,
                   gint                  level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  if (o->radius < 0.5)
    {
      gegl_operation_context_set_object (
        context, "output",
        gegl_operation_context_get_object (context, "input"));

      return TRUE;
    }

  return GEGL_OPERATION_CLASS (gegl_op_parent_class)->process (
    operation, context, output_prop, roi, level);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *aux,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o           = GEGL_PROPERTIES (operation);
  const Babl     *format      = gegl_operation_get_format (operation, "input");
  const Babl     *aux_format  = gegl_operation_get_format (operation, "aux");
  const Babl     *weight_fish = (const Babl *) o->user_data;
  GeglRectangle   rect;
  gfloat         *in;
  gfloat         *in_w;
  gfloat         *out;
  gfloat         *out_w;
  gfloat         *mask        = NULL;
  gfloat          highlight_threshold_low;
  gfloat          highlight_threshold_high;
  gfloat          highlight_factor;
  gfloat          highlight_max;
  gfloat          radius      = o->radius;
  gint            iradius     = floorf (radius + 0.5f);
  gint            size        = 2 * iradius + 1;
  gint            y;

  highlight_threshold_low  = o->highlight_threshold_low;
  highlight_threshold_high = o->highlight_threshold_high;
  highlight_factor         = 10.0 * o->highlight_factor * G_LN2;
  highlight_max            = expf (highlight_factor);

  rect.x      = roi->x      -     iradius;
  rect.y      = roi->y      -     iradius;
  rect.width  = roi->width  + 2 * iradius;
  rect.height = roi->height + 2 * iradius;

  if (o->clip)
    {
      gegl_rectangle_intersect (
        &rect,
        &rect,
        gegl_operation_source_get_bounding_box (operation, "input"));
    }

  size = MIN (size, rect.height);

  in    = (gfloat *) gegl_malloc (4 * sizeof (gfloat) * rect.width * size);
  in_w  = (gfloat *) gegl_malloc (    sizeof (gfloat) * rect.width * size);
  out   = (gfloat *) gegl_malloc (4 * sizeof (gfloat) * roi->width);
  out_w = (gfloat *) gegl_malloc (    sizeof (gfloat) * roi->width);

  if (aux)
    mask = (gfloat *) gegl_malloc (sizeof (gfloat) * rect.width * size);

  auto row_index = [&] (gint y)
  {
    return (y - rect.y) % size;
  };

  auto weight = [&] (gfloat v)
  {
    v = (v                        - highlight_threshold_low) /
        (highlight_threshold_high - highlight_threshold_low);

    if (v <= 0.0f)
      return 1.0f;
    else if (v >= 1.0f)
      return highlight_max;

    return expf (v * highlight_factor);
  };

  auto read = [&] (gint y,
                   gint height)
  {
    gfloat *row;
    gfloat *row_w;
    gfloat *row_m = NULL;
    gint    n = rect.width * height;
    gint    r_i;
    gint    i;

    r_i = row_index (y);

    row   = in   + 4 * rect.width * r_i;
    row_w = in_w +     rect.width * r_i;

    gegl_buffer_get (input,
                     GEGL_RECTANGLE (rect.x, y, rect.width, height),
                     1.0,
                     format,
                     row,
                     GEGL_AUTO_ROWSTRIDE,
                     GEGL_ABYSS_NONE);

    if (mask)
      {
        row_m = mask + rect.width * r_i;

        gegl_buffer_get (aux,
                         GEGL_RECTANGLE (rect.x, y, rect.width, height),
                         1.0,
                         aux_format,
                         row_m,
                         GEGL_AUTO_ROWSTRIDE,
                         GEGL_ABYSS_NONE);
      }

    if (highlight_factor)
      {
        babl_process (weight_fish, row, row_w, n);
      }
    else
      {
        gfloat w = 1.0f;

        gegl_memset_pattern (row_w, &w, sizeof (gfloat), n);
      }

    for (i = 0; i < n; i++)
      {
        gint c;

        if (highlight_factor)
          row_w[i] = weight (row_w[i]);

        if (mask)
          {
            row_m[i]  = row_m[i] * radius + 0.5f;
            row_m[i] *= row_m[i];

            row_w[i] /= G_PI * row_m[i];
          }

        row[4 * i + 3] *= row_w[i];

        for (c = 0; c < 3; c++)
          row[4 * i + c] *= row[4 * i + 3];
      }
  };

  read (rect.y, MIN (roi->y + iradius + 1 - rect.y, rect.height));

  for (y = roi->y; y < roi->y + roi->height; y++)
    {
      gint r1, r2;
      gint x;

      memset (out,   0, 4 * sizeof (gfloat) * roi->width);
      memset (out_w, 0,     sizeof (gfloat) * roi->width);

      r1 = MAX (-iradius,                   - (y - rect.y));
      r2 = MIN (+iradius, (rect.height - 1) - (y - rect.y));

      if (! mask)
        {
          gint r;

          for (r = r1; r <= r2; r++)
            {
              gfloat       *o        = out;
              gfloat       *o_w      = out_w;
              const gfloat *row;
              const gfloat *row_w;
              gfloat        accum[4] = {};
              gfloat        accum_w  = 0.0f;
              gint          r_i;
              gint          s;
              gint          x1, x2;

              r_i = row_index (y + r);

              row   = in   + 4 * (rect.width * r_i + (roi->x - rect.x));
              row_w = in_w +     (rect.width * r_i + (roi->x - rect.x));

              s = sqrtf ((radius + 0.5f) * (radius + 0.5f) - r * r);

              x1 = MAX (-s,                  - (roi->x - rect.x));
              x2 = MIN (+s, (rect.width - 1) - (roi->x - rect.x));

              for (x = x1; x <= x2; x++)
                {
                  gint c;

                  for (c = 0; c < 4; c++)
                    accum[c] += row[4 * x + c];

                  accum_w += row_w[x];
                }

              x1 =                  - (roi->x - rect.x) + s;
              x2 = (rect.width - 1) - (roi->x - rect.x) - s;

              for (x = 0; x < roi->width; x++)
                {
                  gint c;

                  for (c = 0; c < 4; c++)
                    o[c] += accum[c];

                  *o_w += accum_w;

                  o += 4;
                  o_w++;

                  if (x >= x1)
                    {
                      for (c = 0; c < 4; c++)
                        accum[c] -= row[4 * (-s) + c];

                      accum_w -= row_w[-s];
                    }

                  row += 4;
                  row_w++;

                  if (x + 1 <= x2)
                    {
                      for (c = 0; c < 4; c++)
                        accum[c] += row[4 * (+s) + c];

                      accum_w += row_w[+s];
                    }
                }
            }
        }
      else
        {
          gint r;

          for (r = r1; r <= r2; r++)
            {
              const gfloat *row;
              const gfloat *row_w;
              const gfloat *row_m;
              gfloat        r2 = r * r;
              gint          r_i;
              gint          s;
              gint          x1, x2;

              r_i = row_index (y + r);

              row   = in   + 4 * (rect.width * r_i + (roi->x - rect.x));
              row_w = in_w +     (rect.width * r_i + (roi->x - rect.x));
              row_m = mask +     (rect.width * r_i + (roi->x - rect.x));

              s = sqrtf ((radius + 0.5f) * (radius + 0.5f) - r * r);

              x1 = MAX (                 - s,
                                         - (roi->x - rect.x));
              x2 = MIN ((roi->width - 1) + s,
                        (rect.width - 1) - (roi->x - rect.x));

              row   += 4 * x1;
              row_w +=     x1;
              row_m +=     x1;

              for (x = x1; x <= x2; x++, row += 4, row_w++, row_m++)
                {
                  gint xl, xr;
                  gint c;

                  if (*row_m < r2)
                    continue;

                  s = sqrtf (*row_m - r2);

                  xl = x - s;
                  xr = x + s + 1;

                  xl = MAX (xl, 0);

                  if (xl >= xr || xl >= roi->width)
                    continue;

                  for (c = 0; c < 4; c++)
                    out[4 * xl + c] += row[c];

                  out_w[xl] += *row_w;

                  if (xr < roi->width)
                    {
                      for (c = 0; c < 4; c++)
                        out[4 * xr + c] -= row[c];

                      out_w[xr] -= *row_w;
                    }
                }
            }

          for (x = 1; x < roi->width; x++)
            {
              gint c;

              for (c = 0; c < 4; c++)
                out[4 * x + c] += out[4 * (x - 1) + c];

              out_w[x] += out_w[x - 1];
            }
        }

      for (x = 0; x < roi->width; x++)
        {
          gint c;

          for (c = 0; c < 3; c++)
            out[4 * x + c] /= out[4 * x + 3];

          out[4 * x + 3] /= out_w[x];
        }

      gegl_buffer_set (output,
                       GEGL_RECTANGLE (roi->x,
                                       y,
                                       roi->width,
                                       1),
                       0,
                       format,
                       out,
                       GEGL_AUTO_ROWSTRIDE);

      if ((y + 1) + iradius < rect.y + rect.height)
        read ((y + 1) + iradius, 1);
    }

  gegl_free (mask);
  gegl_free (out_w);
  gegl_free (out);
  gegl_free (in_w);
  gegl_free (in);

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass         *operation_class;
  GeglOperationComposerClass *composer_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  composer_class  = GEGL_OPERATION_COMPOSER_CLASS (klass);

  operation_class->prepare                   = prepare;
  operation_class->get_bounding_box          = get_bounding_box;
  operation_class->get_required_for_output   = get_required_for_output;
  operation_class->get_invalidated_by_change = get_required_for_output;
  operation_class->process                   = operation_process;

  composer_class->process                    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:lens-blur",
    "title",       _("Lens Blur"),
    "categories",  "blur",
    "reference-hash", "c5dc4c97b0dacbe3fee41cefca1e6f42",
    "description", _("Simulate out-of-focus lens blur"),
    NULL);
}

#endif
