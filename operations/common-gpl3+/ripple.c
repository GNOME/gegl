/* This file is an image processing operation for GEGL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2018 Simon Budig <simon@gimp.org>
 * Copyright 2011 Michael Muré <batolettre@gmail.com>
 * Copyright 2011 Robert Sasu <sasu.robert@gmail.com>
 * Copyright 2011 Hans Lo <hansshulo@gmail.com>
 * Copyright 1997 Brian Degenhardt <bdegenha@ucsd.edu>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_ripple_wave_type)
  enum_value (GEGL_RIPPLE_WAVE_TYPE_SINE,     "sine",     N_("Sine"))
  enum_value (GEGL_RIPPLE_WAVE_TYPE_TRIANGLE, "triangle", N_("Triangle"))
  enum_value (GEGL_RIPPLE_WAVE_TYPE_SAWTOOTH, "sawtooth", N_("Sawtooth"))
enum_end (GeglRippleWaveType)

property_double (amplitude, _("Amplitude"), 25.0)
    value_range (0.0, 1000.0)
    ui_gamma    (2.0)

property_double (period, _("Period"), 200.0)
    value_range (0.0, 1000.0)
    ui_gamma    (1.5)

property_double (phi, _("Phase shift"), 0.0)
    value_range (-1.0, 1.0)

property_double (angle, _("Angle"), 0.0)
    value_range (-180, 180)
    ui_meta     ("unit", "degree")
    ui_meta     ("direction", "ccw")

property_enum  (sampler_type, _("Resampling method"),
    GeglSamplerType, gegl_sampler_type, GEGL_SAMPLER_CUBIC)

property_enum (wave_type, _("Wave type"),
    GeglRippleWaveType, gegl_ripple_wave_type, GEGL_RIPPLE_WAVE_TYPE_SINE)

property_enum (abyss_policy, _("Abyss policy"),
               GeglAbyssPolicy, gegl_abyss_policy,
               GEGL_ABYSS_NONE)
    description (_("How image edges are handled"))

property_boolean (tileable, _("Tileable"), FALSE)
    description(_("Retain tilebility"))

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     ripple
#define GEGL_OP_C_SOURCE ripple.c

#include "gegl-op.h"
#include <stdio.h>
#include <stdlib.h>

static void
prepare (GeglOperation *operation)
{
  GeglProperties              *o;
  GeglOperationAreaFilter *op_area;
  const Babl              *space = gegl_operation_get_source_space (operation, "input");

  op_area = GEGL_OPERATION_AREA_FILTER (operation);
  o       = GEGL_PROPERTIES (operation);

  op_area->left   = o->amplitude;
  op_area->right  = o->amplitude;
  op_area->top    = o->amplitude;
  op_area->bottom = o->amplitude;

  gegl_operation_set_format (operation, "input",
                             babl_format_with_space ("RGBA float", space));
  gegl_operation_set_format (operation, "output",
                             babl_format_with_space ("RGBA float", space));
}

static GeglAbyssPolicy
get_abyss_policy (GeglOperation *operation,
                  const gchar   *input_pad)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  return o->abyss_policy;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties     *o       = GEGL_PROPERTIES (operation);
  const Babl         *format  = gegl_operation_get_format (operation, "output");
  GeglSampler        *sampler = gegl_buffer_sampler_new_at_level (input,
                                                         format,
                                                         o->sampler_type,
                                                         level);
  GeglBufferIterator *iter;
  gdouble angle_rad, period, amplitude, phi;

  angle_rad = o->angle / 180.0 * G_PI;
  period    = o->period;
  amplitude = o->amplitude;
  phi       = o->phi;

  if (period < 0.0001)
    {
      period = 1.0;
      amplitude = 0.0;
    }

  if (o->tileable)
    {
      gint w, h;
      gdouble n, m;
      GeglRectangle *bbox;

      bbox = gegl_operation_source_get_bounding_box (operation, "input");
      w = bbox->width;
      h = bbox->height;

      n = cos (angle_rad) * w / period;
      m = sin (angle_rad) * h / period;

      /* round away from zero */
      n = round (n);
      m = round (m);
      if (n == 0 && m == 0)
        {
          n = 1;
          amplitude = 0;
        }

      /* magic! */
      angle_rad = atan2 (m * w, h * n);
      period = sqrt ((gdouble) h*h * w*w / (n*n * h*h + m*m * w*w));

      /* ok, not actually.
       *
       * For the result of the ripple op being tileable you need
       * to have the period/angle select in a way, so that the top left
       * corner has an integer * period distance along the angle to
       * the top right corner as well as the bottom left corner.
       *
       * I.e.:
       *
       * cos (angle) * width = n * period
       * sin (angle) * height = m * period
       *
       * with n, m being integers.
       *
       * We determine n, m by rounding the results optained by the
       * user specified angle/period and then calculate a hopefully only
       * slightly modified new angle/period that meets these criteria.
       *
       * We determine the angle by computing tan(), thereby eliminating
       * the period, then determining the period via a formula
       * derived from the  sin²(a)+cos²(a) = 1  identity.
       */
    }

  iter = gegl_buffer_iterator_new (output, result, 0, format,
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      gint x = result->x;
      gint y = result->y;
      gfloat *out_pixel = iter->items[0].data;

      for (y = iter->items[0].roi.y; y < iter->items[0].roi.y + iter->items[0].roi.height; ++y)
        for (x = iter->items[0].roi.x; x < iter->items[0].roi.x + iter->items[0].roi.width; ++x)
          {
            gdouble shift;
            gdouble coordsx;
            gdouble coordsy;
            gdouble lambda;

            gdouble nx = x * cos (angle_rad) - y * sin (angle_rad);

            switch (o->wave_type)
              {
                case GEGL_RIPPLE_WAVE_TYPE_SAWTOOTH:
                  lambda = div (nx + period / 2 - phi * period, period).rem;
                  if (lambda < 0)
                    lambda += period;
                  shift = amplitude * (((lambda / period) * 2) - 1);
                  break;

                case GEGL_RIPPLE_WAVE_TYPE_TRIANGLE:
                  lambda = div (nx + period * 3 / 4 - phi * period, period).rem;
                  if (lambda < 0)
                    lambda += period;
                  shift = amplitude * (fabs (((lambda / period) * 4) - 2) - 1);
                  break;

                case GEGL_RIPPLE_WAVE_TYPE_SINE:
                default:
                  shift = amplitude * sin (2.0 * G_PI * nx / period + 2.0 * G_PI * phi);
                  break;
              }

            coordsx = x + shift * sin (angle_rad);
            coordsy = y + shift * cos (angle_rad);

            gegl_sampler_get (sampler,
                              coordsx,
                              coordsy,
                              NULL,
                              out_pixel,
                              o->abyss_policy);

            out_pixel += 4;
          }
    }

  g_object_unref (sampler);

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

  operation_class->prepare     = prepare;
  filter_class->process        = process;
  area_class->get_abyss_policy = get_abyss_policy;

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:ripple",
    "title",              _("Ripple"),
    "categories",         "distort",
    "position-dependent", "true",
    "license",            "GPL3+",
    "reference-hash",     "7f291e2dfcc59d6832be21c839e58963",
    "reference-hashB",    "194e6648043a63616a2f498084edbe92",
    "description", _("Displace pixels in a ripple pattern"),
    NULL);
}

#endif
