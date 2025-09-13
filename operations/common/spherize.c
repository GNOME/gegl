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

enum_start (gegl_spherize_mode)
  enum_value (GEGL_SPHERIZE_MODE_RADIAL,     "radial",     N_("Radial"))
  enum_value (GEGL_SPHERIZE_MODE_HORIZONTAL, "horizontal", N_("Horizontal"))
  enum_value (GEGL_SPHERIZE_MODE_VERTICAL,   "vertical",   N_("Vertical"))
enum_end (GeglSpherizeMode)

property_enum (mode, _("Mode"),
               GeglSpherizeMode, gegl_spherize_mode,
               GEGL_SPHERIZE_MODE_RADIAL)
  description (_("Displacement mode"))

property_double (angle_of_view, _("Angle of view"), 0.0)
  description (_("Camera angle of view"))
  value_range (0.0, 180.0)
  ui_meta     ("unit", "degree")

property_double (curvature, _("Curvature"), 1.0)
  description (_("Spherical cap apex angle, as a fraction of the co-angle of view"))
  value_range (0.0, 1.0) /* note that the code can handle negative curvatures
                          * (in the [-1, 0) range), in which case the image is
                          * wrapped around the back face, rather than the front
                          * face, of the spherical cap.  we disable negative
                          * curvatures atm, in particular, since they produce
                          * the same result when the angle of view is 0, and
                          * since their upper-bound, wrt the angle of view, is
                          * arbitrary.
                          */

property_double (amount, _("Amount"), 1.0)
  description (_("Displacement scaling factor (negative values refer to the inverse displacement)"))
  value_range (-1.0, 1.0)

property_enum (sampler_type, _("Resampling method"),
  GeglSamplerType, gegl_sampler_type, GEGL_SAMPLER_LINEAR)
  description (_("Mathematical method for reconstructing pixel values"))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     spherize
#define GEGL_OP_C_SOURCE spherize.c

#include "gegl-op.h"

#define EPSILON 1e-10

static gboolean
is_nop (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle  *in_rect;

  if (fabs (o->curvature) < EPSILON || fabs (o->amount) < EPSILON)
    return TRUE;

  in_rect = gegl_operation_source_get_bounding_box (operation, "input");

  if (in_rect && gegl_rectangle_is_infinite_plane (in_rect))
    return TRUE;

  switch (o->mode)
    {
    case GEGL_SPHERIZE_MODE_RADIAL:
      return in_rect->width < 1 || in_rect->height < 1;

    case GEGL_SPHERIZE_MODE_HORIZONTAL:
      return in_rect->width < 1;

    case GEGL_SPHERIZE_MODE_VERTICAL:
      return in_rect->height < 1;
    }

  g_return_val_if_reached (TRUE);
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle result = *roi;

  if (! is_nop (operation))
    {
      GeglProperties *o       = GEGL_PROPERTIES (operation);
      GeglRectangle  *in_rect = gegl_operation_source_get_bounding_box (operation, "input");

      if (in_rect)
        {
          switch (o->mode)
            {
            case GEGL_SPHERIZE_MODE_RADIAL:
              result = *in_rect;
              break;

            case GEGL_SPHERIZE_MODE_HORIZONTAL:
              result.x     = in_rect->x;
              result.width = in_rect->width;
              break;

            case GEGL_SPHERIZE_MODE_VERTICAL:
              result.y      = in_rect->y;
              result.height = in_rect->height;
              break;
            }
        }
    }

  return result;
}

static gboolean
parent_process (GeglOperation        *operation,
                GeglOperationContext *context,
                const gchar          *output_prop,
                const GeglRectangle  *result,
                gint                  level)
{
  if (is_nop (operation))
    {
      GObject *input;

      input = gegl_operation_context_get_object (context, "input");

      gegl_operation_context_set_object (context, "output", input);

      return TRUE;
    }

  return GEGL_OPERATION_CLASS (gegl_op_parent_class)->process (operation,
                                                               context,
                                                               output_prop,
                                                               result, level);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties      *o      = GEGL_PROPERTIES (operation);
  const Babl          *format = gegl_operation_get_format (operation, "output");
  GeglSampler         *sampler;
  GeglBufferIterator  *iter;
  const GeglRectangle *in_extent;
  gdouble              cx, cy;
  gdouble              dx = 0.0, dy = 0.0;
  gdouble              coangle_of_view_2;
  gdouble              focal_length;
  gdouble              curvature_sign;
  gdouble              cap_angle_2;
  gdouble              cap_radius;
  gdouble              cap_depth;
  gdouble              factor;
  gdouble              f, f2, r, r_inv, r2, p, f_p, f_p2, f_pf, a, a_inv, sgn;
  gboolean             perspective;
  gboolean             inverse;
  gint                 i, j;

  sampler = gegl_buffer_sampler_new_at_level (input, format,
                                              o->sampler_type, level);

  iter = gegl_buffer_iterator_new (output, roi, level, format,
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 2);

  gegl_buffer_iterator_add (iter, input, roi, level, format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  in_extent = gegl_operation_source_get_bounding_box (operation, "input");

  cx = in_extent->x + in_extent->width  / 2.0;
  cy = in_extent->y + in_extent->height / 2.0;

  if (o->mode == GEGL_SPHERIZE_MODE_RADIAL ||
      o->mode == GEGL_SPHERIZE_MODE_HORIZONTAL)
    {
      dx = 2.0 / (in_extent->width - 1);
    }
  if (o->mode == GEGL_SPHERIZE_MODE_RADIAL ||
      o->mode == GEGL_SPHERIZE_MODE_VERTICAL)
    {
      dy = 2.0 / (in_extent->height - 1);
    }

  coangle_of_view_2 = MAX (180.0 - o->angle_of_view, 0.01) * G_PI / 360.0;
  focal_length      = tan (coangle_of_view_2);
  curvature_sign    = o->curvature > 0.0 ? +1.0 : -1.0;
  cap_angle_2       = fabs (o->curvature) * coangle_of_view_2;
  cap_radius        = 1.0 / sin (cap_angle_2);
  cap_depth         = curvature_sign * cap_radius * cos (cap_angle_2);
  factor            = fabs (o->amount);

  f     = focal_length;
  f2    = f * f;
  r     = cap_radius;
  r_inv = 1 / r;
  r2    = r * r;
  p     = cap_depth;
  f_p   = f + p;
  f_p2  = f_p * f_p;
  f_pf  = f_p * f;
  a     = cap_angle_2;
  a_inv = 1 / a;
  sgn   = curvature_sign;

  perspective = o->angle_of_view > EPSILON;
  inverse     = o->amount < 0.0;

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat       *out_pixel = iter->items[0].data;
      const gfloat *in_pixel  = iter->items[1].data;
      GeglRectangle *roi = &iter->items[0].roi;
      gfloat        x,  y;

      y = dy * (roi->y + 0.5 - cy);

      for (j = roi->y; j < roi->y + roi->height; j++, y += dy)
        {
          x = dx * (roi->x + 0.5 - cx);

          for (i = roi->x; i < roi->x + roi->width; i++, x += dx)
            {
              gfloat d2;

              d2 = x * x + y * y;

              if (d2 > EPSILON && d2 < 1.0 - EPSILON)
                {
                  gdouble d     = sqrt (d2);
                  gdouble src_d = d;
                  gdouble src_x, src_y;

                  if (! inverse)
                    {
                      gdouble d2_f2 = d2 + f2;

                      if (perspective)
                        src_d = (f_pf - sgn * sqrt (d2_f2 * r2 - f_p2 * d2)) * d / d2_f2;

                      src_d = (G_PI_2 - acos (src_d * r_inv)) * a_inv;
                    }
                  else
                    {
                      src_d = r * cos (G_PI_2 - src_d * a);

                      if (perspective)
                        src_d = f * src_d / (f_p - sgn * sqrt (r2 - src_d * src_d));
                    }

                  if (factor < 1.0)
                    src_d = d + (src_d - d) * factor;

                  src_x = dx ? cx + src_d * x / (dx * d) :
                               i + 0.5;
                  src_y = dy ? cy + src_d * y / (dy * d) :
                               j + 0.5;

                  gegl_sampler_get (sampler, src_x, src_y,
                                    NULL, out_pixel, GEGL_ABYSS_NONE);
                }
              else
                {
                  memcpy (out_pixel, in_pixel, sizeof (gfloat) * 4);
                }

              out_pixel += 4;
              in_pixel  += 4;
            }
        }
    }

  g_object_unref (sampler);

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->get_invalidated_by_change = get_required_for_output;
  operation_class->get_required_for_output   = get_required_for_output;
  operation_class->process                   = parent_process;

  filter_class->process                      = process;

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:spherize",
    "title",              _("Spherize"),
    "categories",         "distort:map",
    "position-dependent", "true",
    "reference-hash", "3c5a521a9a82d02943654df85c39eba0",
    "description",        _("Wrap image around a spherical cap"),
    NULL);
}

#endif
