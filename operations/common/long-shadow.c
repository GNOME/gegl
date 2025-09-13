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
 * Copyright (C) 2018 Ell
 */

#include "config.h"
#include <glib/gi18n-lib.h>

/* #define WITH_FADING_FIXED_RATE */

#ifdef GEGL_PROPERTIES

enum_start (gegl_long_shadow_style)
  enum_value      (GEGL_LONG_SHADOW_STYLE_FINITE,              "finite",              N_("Finite"))
  enum_value      (GEGL_LONG_SHADOW_STYLE_INFINITE,            "infinite",            N_("Infinite"))
  enum_value      (GEGL_LONG_SHADOW_STYLE_FADING,              "fading",              N_("Fading"))
  enum_value      (GEGL_LONG_SHADOW_STYLE_FADING_FIXED_LENGTH, "fading-fixed-length", N_("Fading (fixed length)"))
#ifdef WITH_FADING_FIXED_RATE
  enum_value      (GEGL_LONG_SHADOW_STYLE_FADING_FIXED_RATE,   "fading-fixed-rate",   N_("Fading (fixed rate)"))
#else
  enum_value_skip (GEGL_LONG_SHADOW_STYLE_FADING_FIXED_RATE)
#endif
enum_end (GeglLongShadowStyle)

enum_start (gegl_long_shadow_composition)
  enum_value (GEGL_LONG_SHADOW_COMPOSITION_SHADOW_PLUS_IMAGE,  "shadow-plus-image",  N_("Shadow plus image"))
  enum_value (GEGL_LONG_SHADOW_COMPOSITION_SHADOW_ONLY,        "shadow-only",        N_("Shadow only"))
  enum_value (GEGL_LONG_SHADOW_COMPOSITION_SHADOW_MINUS_IMAGE, "shadow-minus-image", N_("Shadow minus image"))
enum_end (GeglLongShadowComposition)

property_enum (style, _("Style"),
               GeglLongShadowStyle, gegl_long_shadow_style,
               GEGL_LONG_SHADOW_STYLE_FINITE)
  description (_("Shadow style"))

property_double (angle, _("Angle"), 45.0)
  description (_("Shadow angle"))
  value_range (-180.0, 180.0)
  ui_meta     ("unit", "degree")
  ui_meta     ("direction", "cw")

property_double (length, _("Length"), 100.0)
  description (_("Shadow length"))
  value_range (0.0, G_MAXDOUBLE)
  ui_range    (0.0, 1000.0)
  ui_meta     ("visible", "style {finite,            "
                          "       fading-fixed-length"
#ifdef WITH_FADING_FIXED_RATE
                          "       , fading-fixed-rate"
#endif
                          "      }")

property_double (midpoint, _("Midpoint"), 100.0)
  description (_("Shadow fade midpoint"))
  value_range (0.0, G_MAXDOUBLE)
  ui_range    (0.0, 1000.0)
  ui_meta     ("visible", "style {fading}")

property_double (midpoint_rel, _("Midpoint (relative)"), 0.5)
  description (_("Shadow fade midpoint, as a factor of the shadow length"))
  value_range (0.0, 1.0)
  ui_meta     ("visible", "style {fading-fixed-length"
#ifdef WITH_FADING_FIXED_RATE
                          "       , fading-fixed-rate"
#endif
                          "      }")
  ui_meta     ("label", "alt-label")
  ui_meta     ("alt-label", _("Midpoint"))

property_color (color, _("Color"), "black")
  description (_("Shadow color"))
  ui_meta     ("role", "color-primary")

property_enum (composition, _("Composition"),
               GeglLongShadowComposition, gegl_long_shadow_composition,
               GEGL_LONG_SHADOW_COMPOSITION_SHADOW_PLUS_IMAGE)
  description (_("Output composition"))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     long_shadow
#define GEGL_OP_C_SOURCE long-shadow.c

#include "gegl-op.h"

/* virtual screen resolution, as a factor of the image resolution.  must be an
 * integer.
 */
#define SCREEN_RESOLUTION 16
#define EPSILON           1e-6

#define SWAP(type, x, y) \
  G_STMT_START           \
    {                    \
      type t = (x);      \
      (x)    = (y);      \
      (y)    = t;        \
    }                    \
  G_STMT_END

typedef enum
{
  VARIANT_FINITE,
  VARIANT_FADING_FIXED_LENGTH_ACCELERATING,
  VARIANT_FADING_FIXED_LENGTH_DECELERATING,
  VARIANT_FADING_FIXED_RATE_NONLINEAR,
  VARIANT_FADING_FIXED_RATE_LINEAR,
  VARIANT_INFINITE,
  VARIANT_FADING
} Variant;

typedef struct
{
  gfloat *fade_lut;
  gint    fade_lut_size;
  gfloat  fade_lut_gamma;
} Data;

typedef struct
{
  gfloat value;
  gint   fy;
} Shadow;

typedef struct ShadowItem
{
  Shadow             shadow;

  struct ShadowItem *next;
  struct ShadowItem *prev;
} ShadowItem;

typedef struct
{
  Shadow      shadow;
  ShadowItem *queue;
} Pixel;

typedef struct
{
  Pixel  pixel;
  Shadow max1;
  gfloat max2;
} FFLPixel;

typedef struct
{
  gfloat value;
  gfloat fy;
  gint   last_fy;
} FFRPixel;

typedef struct
{
  GeglProperties options;

  gboolean       is_finite;
  gboolean       is_fading;

  Variant        variant;

  /* image -> filter coordinate transformation */
  gboolean       flip_horizontally;
  gboolean       flip_vertically;
  gboolean       flip_diagonally;

  /* in filter coordinates */
  gdouble        tan_angle;

  gint           shadow_height;
  gfloat         shadow_proj;
  gfloat         shadow_remainder;

  gfloat         fade_rate;
  gfloat         fade_rate_inv;
  gfloat         fade_gamma;
  gfloat         fade_gamma_inv;

  const gfloat  *fade_lut;

  GeglRectangle  input_bounds;
  GeglRectangle  roi;
  GeglRectangle  area;

  /* in screen coordinates */
  gint           u0, u1;

  gpointer       screen;
  gint           pixel_size;
  gint           active_u0;
  gint           active_u1;

  GeglBuffer    *input;
  GeglBuffer    *output;

  const Babl    *format;

  gfloat        *input_row;
  gfloat        *output_row;

  gfloat        *input_row0;
  gfloat        *output_row0;
  gint           row_step;

  gint           row_fx0;
  gint           row_fx1;
  gint           row_u0;
  gint           row_input_pixel_offset0;
  gint           row_input_pixel_offset1;
  gint           row_output_pixel_span;
  gfloat         row_output_pixel_kernel[2 * SCREEN_RESOLUTION + 1];

  gfloat         color[4];

  gint           level;
  gdouble        scale;
  gdouble        scale_inv;
} Context;

static inline gboolean
is_finite (const GeglProperties *options)
{
  switch (options->style)
    {
    case GEGL_LONG_SHADOW_STYLE_FINITE:
    case GEGL_LONG_SHADOW_STYLE_FADING_FIXED_LENGTH:
    case GEGL_LONG_SHADOW_STYLE_FADING_FIXED_RATE:
      return TRUE;

    case GEGL_LONG_SHADOW_STYLE_INFINITE:
    case GEGL_LONG_SHADOW_STYLE_FADING:
      return FALSE;
    }

  g_return_val_if_reached (FALSE);
}

static inline gboolean
is_fading (const GeglProperties *options)
{
  switch (options->style)
    {
    case GEGL_LONG_SHADOW_STYLE_FADING:
    case GEGL_LONG_SHADOW_STYLE_FADING_FIXED_LENGTH:
    case GEGL_LONG_SHADOW_STYLE_FADING_FIXED_RATE:
      return TRUE;

    case GEGL_LONG_SHADOW_STYLE_FINITE:
    case GEGL_LONG_SHADOW_STYLE_INFINITE:
      return FALSE;
    }

  g_return_val_if_reached (FALSE);
}

static void
init_options (Context              *ctx,
              const GeglProperties *options,
              gint                  level)
{
  ctx->options = *options;

  ctx->is_finite = is_finite (options);
  ctx->is_fading = is_fading (options);

  if (ctx->is_finite && ctx->is_fading)
    {
      if (ctx->options.length       <= EPSILON ||
          ctx->options.midpoint_rel <= EPSILON ||
          ctx->options.midpoint_rel >= 1.0 - EPSILON)
        {
          if (ctx->options.midpoint_rel <= EPSILON ||
              ctx->options.style == GEGL_LONG_SHADOW_STYLE_FADING_FIXED_RATE)
            {
              ctx->options.length = 0.0;
            }

          ctx->options.style = GEGL_LONG_SHADOW_STYLE_FINITE;
          ctx->is_fading     = FALSE;
        }
    }

  switch (ctx->options.style)
    {
    case GEGL_LONG_SHADOW_STYLE_INFINITE:
      ctx->variant = VARIANT_INFINITE;
      break;

    case GEGL_LONG_SHADOW_STYLE_FINITE:
      ctx->variant = VARIANT_FINITE;
      break;

    case GEGL_LONG_SHADOW_STYLE_FADING:
      ctx->variant = VARIANT_FADING;
      break;

    case GEGL_LONG_SHADOW_STYLE_FADING_FIXED_LENGTH:
      if (ctx->options.midpoint_rel >= 0.5)
        ctx->variant = VARIANT_FADING_FIXED_LENGTH_ACCELERATING;
      else
        ctx->variant = VARIANT_FADING_FIXED_LENGTH_DECELERATING;
      break;

    case GEGL_LONG_SHADOW_STYLE_FADING_FIXED_RATE:
      if (fabs (ctx->options.midpoint_rel - 0.5) > EPSILON)
        ctx->variant = VARIANT_FADING_FIXED_RATE_NONLINEAR;
      else
        ctx->variant = VARIANT_FADING_FIXED_RATE_LINEAR;
      break;
    }

  ctx->level     = level;
  ctx->scale     = 1.0 / (1 << level);
  ctx->scale_inv = 1 << level;

  ctx->options.length   *= ctx->scale;
  ctx->options.midpoint *= ctx->scale;
}

static void
init_geometry (Context *ctx)
{
  ctx->flip_horizontally = FALSE;
  ctx->flip_vertically   = FALSE;
  ctx->flip_diagonally   = FALSE;

  /* set up the image -> filter coordinate transformation, such that the
   * shadow's angle is always inside the [0 deg., 45 deg.] range, relative to
   * the positive (filter-space) y-axis, counter-clockwise.
   */

  ctx->options.angle = 90.0 - ctx->options.angle;
  if (ctx->options.angle > 180.0)
    ctx->options.angle -= 360.0;

  if (ctx->options.angle < 0.0)
    {
      ctx->options.angle     = -ctx->options.angle;
      ctx->flip_horizontally = TRUE;
    }

  if (ctx->options.angle > 90.0)
    {
      ctx->options.angle   = 180.0 - ctx->options.angle;
      ctx->flip_vertically = TRUE;
    }

  if (ctx->options.angle > 45.0)
    {
      ctx->options.angle   = 90.0 - ctx->options.angle;
      ctx->flip_diagonally = TRUE;

      SWAP (gboolean, ctx->flip_horizontally, ctx->flip_vertically);
    }

  ctx->options.angle *= G_PI / 180.0;

  ctx->tan_angle = tan (ctx->options.angle);

  if (ctx->is_finite)
    {
      ctx->shadow_proj = cos (ctx->options.angle) * ctx->options.length;

      ctx->shadow_height    = ceilf (ctx->shadow_proj);
      ctx->shadow_remainder = 1.0 - (ctx->shadow_height - ctx->shadow_proj);
    }
}


static inline void
transform_rect_to_filter (Context             *ctx,
                          const GeglRectangle *irect,
                          GeglRectangle       *frect,
                          gboolean             scale)
{
  *frect = *irect;

  if (ctx->flip_diagonally)
    {
      SWAP (gint, frect->x,     frect->y);
      SWAP (gint, frect->width, frect->height);
    }

  if (ctx->flip_horizontally)
    frect->x = -frect->x - frect->width;

  if (ctx->flip_vertically)
    frect->y = -frect->y - frect->height;

  if (scale)
    {
      frect->width  += frect->x;
      frect->height += frect->y;

      frect->x      = (frect->x      + 0) >> ctx->level;
      frect->y      = (frect->y      + 0) >> ctx->level;
      frect->width  = (frect->width  + 1) >> ctx->level;
      frect->height = (frect->height + 1) >> ctx->level;

      frect->width  -= frect->x;
      frect->height -= frect->y;
    }
}

static inline void
transform_rect_to_image (Context             *ctx,
                         const GeglRectangle *frect,
                         GeglRectangle       *irect,
                         gboolean             scale)
{
  *irect = *frect;

  if (scale)
    {
      irect->x      <<= ctx->level;
      irect->y      <<= ctx->level;
      irect->width  <<= ctx->level;
      irect->height <<= ctx->level;
    }

  if (ctx->flip_vertically)
    irect->y = -irect->y - irect->height;

  if (ctx->flip_horizontally)
    irect->x = -irect->x - irect->width;

  if (ctx->flip_diagonally)
    {
      SWAP (gint, irect->x,     irect->y);
      SWAP (gint, irect->width, irect->height);
    }
}

static inline gdouble
project_to_screen (Context *ctx,
                   gdouble  fx,
                   gdouble  fy)
{
  return SCREEN_RESOLUTION * (fx - ctx->tan_angle * fy);
}

static inline gdouble
project_to_filter (Context *ctx,
                   gdouble  u,
                   gdouble  fy)
{
  return u / SCREEN_RESOLUTION + ctx->tan_angle * fy;
}

static inline void
get_affected_screen_range (Context *ctx,
                           gint     fx0,
                           gint     fx1,
                           gint     fy,
                           gint    *u0,
                           gint    *u1)
{
  if (u0) *u0 = floor (project_to_screen (ctx, fx0, fy + 0.5) + 0.5);
  if (u1) *u1 = floor (project_to_screen (ctx, fx1, fy - 0.5) + 0.5);
}

static inline void
get_affected_filter_range (Context *ctx,
                           gint     u0,
                           gint     u1,
                           gint     fy,
                           gint    *fx0,
                           gint    *fx1)
{
  if (fx0) *fx0 = floor (project_to_filter (ctx, u0, fy - 0.5));
  if (fx1) *fx1 = ceil  (project_to_filter (ctx, u1, fy + 0.5));
}

static inline void
get_affecting_screen_range (Context *ctx,
                            gint     fx0,
                            gint     fx1,
                            gint     fy,
                            gint    *u0,
                            gint    *u1)
{
  if (u0) *u0 = floor (project_to_screen (ctx, fx0, fy + 0.5));
  if (u1) *u1 = ceil  (project_to_screen (ctx, fx1, fy - 0.5));
}

static inline void
get_affecting_filter_range (Context *ctx,
                            gint     u0,
                            gint     u1,
                            gint     fy,
                            gint    *fx0,
                            gint    *fx1)
{
  if (fx0) *fx0 = floor (project_to_filter (ctx, u0 + 0.5, fy - 0.5));
  if (fx1) *fx1 = ceil  (project_to_filter (ctx, u1 - 0.5, fy + 0.5));
}

static inline gfloat
fade_value (Context *ctx,
            gfloat   fy)
{
  return 1.0f - powf (fy * ctx->fade_rate, ctx->fade_gamma);
}

static void
init_fade (Context *ctx)
{
  switch (ctx->options.style)
    {
    case GEGL_LONG_SHADOW_STYLE_FADING:
      if (ctx->options.midpoint > EPSILON)
        {
          ctx->fade_rate = pow (0.5, 1.0 / (cos (ctx->options.angle) *
                                            ctx->options.midpoint));
        }
      else
        {
          ctx->fade_rate = 0.0f;
        }
      break;

    case GEGL_LONG_SHADOW_STYLE_FADING_FIXED_LENGTH:
    case GEGL_LONG_SHADOW_STYLE_FADING_FIXED_RATE:
      ctx->fade_rate      = 1.0 / (ctx->shadow_proj + 1.0);
      ctx->fade_gamma     = -G_LN2 / log (ctx->options.midpoint_rel);

      ctx->fade_rate_inv  = 1.0 / ctx->fade_rate;
      ctx->fade_gamma_inv = 1.0 / ctx->fade_gamma;

      if (ctx->options.style == GEGL_LONG_SHADOW_STYLE_FADING_FIXED_LENGTH)
        {
          Data *data = ctx->options.user_data;

          if (! data)
            {
              data = g_slice_new0 (Data);

              ctx->options.user_data = data;
            }

          if (ctx->shadow_height + 1 != data->fade_lut_size ||
              ctx->fade_gamma        != data->fade_lut_gamma)
            {
              gint fy;

              data->fade_lut       = g_renew (gfloat, data->fade_lut,
                                              ctx->shadow_height + 1);
              data->fade_lut_size  = ctx->shadow_height + 1;
              data->fade_lut_gamma = ctx->fade_gamma;

              for (fy = 0; fy <= ctx->shadow_height; fy++)
                data->fade_lut[fy] = fade_value (ctx, fy);
            }

          ctx->fade_lut = data->fade_lut;
        }
      break;

    case GEGL_LONG_SHADOW_STYLE_FINITE:
    case GEGL_LONG_SHADOW_STYLE_INFINITE:
      break;
    }
}

static void
init_area (Context             *ctx,
           GeglOperation       *operation,
           const GeglRectangle *roi)
{
  GeglRectangle *input_bounds;

  input_bounds = gegl_operation_source_get_bounding_box (operation, "input");

  if (input_bounds)
    transform_rect_to_filter (ctx, input_bounds, &ctx->input_bounds, TRUE);
  else
    ctx->input_bounds = *GEGL_RECTANGLE (0, 0, 0, 0);

  transform_rect_to_filter (ctx, roi, &ctx->roi, TRUE);

  get_affecting_screen_range (ctx,
                              ctx->roi.x, 0,
                              ctx->roi.y + ctx->roi.height - 1,
                              &ctx->u0, NULL);
  get_affecting_screen_range (ctx,
                              0, ctx->roi.x + ctx->roi.width,
                              ctx->roi.y,
                              NULL, &ctx->u1);

  ctx->area = ctx->roi;

  if (ctx->is_finite)
    {
      gint u0;

      ctx->area.y -= ctx->shadow_height;

      get_affecting_screen_range (ctx,
                                  ctx->roi.x, 0,
                                  ctx->roi.y,
                                  &u0, NULL);
      get_affecting_filter_range (ctx,
                                  u0, 0,
                                  ctx->area.y,
                                  &ctx->area.x, NULL);

      ctx->area.x--;

      ctx->area.x = MAX (ctx->area.x, ctx->input_bounds.x);
      ctx->area.y = MAX (ctx->area.y, ctx->input_bounds.y);

      ctx->area.width  += ctx->roi.x - ctx->area.x;
      ctx->area.height += ctx->roi.y - ctx->area.y;
    }
}

static void
init_screen (Context *ctx)
{
  switch (ctx->variant)
    {
    case VARIANT_FINITE:
    case VARIANT_FADING_FIXED_LENGTH_ACCELERATING:
      ctx->pixel_size = sizeof (Pixel);
      break;

    case VARIANT_FADING_FIXED_LENGTH_DECELERATING:
      ctx->pixel_size = sizeof (FFLPixel);
      break;

    case VARIANT_FADING_FIXED_RATE_NONLINEAR:
      ctx->pixel_size = sizeof (FFRPixel);
      break;

    case VARIANT_FADING_FIXED_RATE_LINEAR:
    case VARIANT_INFINITE:
    case VARIANT_FADING:
      ctx->pixel_size = sizeof (gfloat);
      break;
    }

  ctx->screen = g_malloc0 (ctx->pixel_size * (ctx->u1 - ctx->u0));
  ctx->screen = (guchar *) ctx->screen - ctx->pixel_size * ctx->u0;

  ctx->active_u0 = ctx->u1;
  ctx->active_u1 = ctx->u0;
}

static inline void
clear_pixel_queue (Context *ctx,
                   Pixel   *pixel)
{
  ShadowItem *item = pixel->queue;

  while (item)
    {
      ShadowItem *next = item->next;

      g_slice_free (ShadowItem, item);

      item = next;
    }

  pixel->queue = NULL;
}

static void
cleanup_screen (Context *ctx)
{
  switch (ctx->options.style)
    {
    case GEGL_LONG_SHADOW_STYLE_FINITE:
    case GEGL_LONG_SHADOW_STYLE_FADING_FIXED_LENGTH:
      {
        guchar *p = ctx->screen;
        gint    u;

        p += ctx->u0 * ctx->pixel_size;

        for (u = ctx->u0; u < ctx->u1; u++, p += ctx->pixel_size)
          clear_pixel_queue (ctx, (Pixel *) p);
      }
      break;

    case GEGL_LONG_SHADOW_STYLE_INFINITE:
    case GEGL_LONG_SHADOW_STYLE_FADING:
    case GEGL_LONG_SHADOW_STYLE_FADING_FIXED_RATE:
      break;
    }

  ctx->screen = (guchar *) ctx->screen + ctx->pixel_size * ctx->u0;

  g_free (ctx->screen);
}

static void
init_row (Context *ctx,
          gint     fy)
{
  gdouble u0, u1;
  gdouble v0, v1;
  gdouble b;
  gint    i;

  get_affecting_filter_range (ctx,
                              ctx->u0, ctx->u1,
                              fy,
                              &ctx->row_fx0, &ctx->row_fx1);

  ctx->row_fx0 = MAX (ctx->row_fx0, ctx->area.x);
  ctx->row_fx1 = MIN (ctx->row_fx1, ctx->area.x + ctx->area.width);

  u0 = project_to_screen (ctx, ctx->row_fx0,       fy + 0.5);
  u1 = project_to_screen (ctx, ctx->row_fx0 + 1.0, fy - 0.5);

  ctx->row_u0                  = floor (u0);

  ctx->row_input_pixel_offset0 = floor (u0 + 0.5) - ctx->row_u0;
  ctx->row_input_pixel_offset1 = floor (u1 + 0.5) - ctx->row_u0;

  ctx->row_output_pixel_span  = ceil (u1) - floor (u0);

  b = ctx->tan_angle * SCREEN_RESOLUTION;

  v0 = u0 + b;
  v1 = u1 - b;

  for (i = 0; i < ctx->row_output_pixel_span; i++)
    {
      gdouble w0, w1;
      gdouble value = 0.0;

      if (b > EPSILON)
        {
          w0     = CLAMP (ctx->row_u0 + i,       u0, v0);
          w1     = CLAMP (ctx->row_u0 + i + 1.0, u0, v0);
          value += (w1 - w0) * (w0 + w1 - 2.0 * u0) / (2.0 * b);
        }

      w0     = CLAMP (ctx->row_u0 + i,       v0, v1);
      w1     = CLAMP (ctx->row_u0 + i + 1.0, v0, v1);
      value += w1 - w0;

      if (b > EPSILON)
        {
          w0     = CLAMP (ctx->row_u0 + i,       v1, u1);
          w1     = CLAMP (ctx->row_u0 + i + 1.0, v1, u1);
          value += (w1 - w0) * (2.0 * u1 - w0 - w1) / (2.0 * b);
        }

      value /= SCREEN_RESOLUTION;

      ctx->row_output_pixel_kernel[i] = value;
    }
}

static inline gfloat
shadow_value (Context      *ctx,
              const Shadow *shadow,
              gint          fy)
{
  if (! ctx->is_fading || ! shadow->value)
    return shadow->value;
  else
    return shadow->value * ctx->fade_lut[fy - shadow->fy];
}

static inline gfloat
shadow_collation_value (Context      *ctx,
                        const Shadow *shadow,
                        gint          fy)
{
  if (ctx->variant == VARIANT_FINITE                           ||
      ctx->variant == VARIANT_FADING_FIXED_LENGTH_DECELERATING ||
      ! shadow->value)
    {
      return shadow->value;
    }
  else
    {
      return shadow->value * ctx->fade_lut[fy - shadow->fy];
    }
}

static inline gboolean
shift_pixel (Context *ctx,
             Pixel   *pixel)
{
  ShadowItem *item = pixel->queue;

  if (! item)
    {
      pixel->shadow.value = 0.0f;

      return FALSE;
    }

  pixel->shadow        = item->shadow;
  pixel->queue         = item->next;
  if (pixel->queue)
    pixel->queue->prev = item->prev;

  g_slice_free (ShadowItem, item);

  return TRUE;
}

static void
trim_shadow (Context *ctx,
             gint     fy)
{
  if (fy <= ctx->roi.y && ! ctx->is_fading)
    return;

  if (ctx->active_u0 >= ctx->active_u1)
    return;

  switch (ctx->variant)
    {
    case VARIANT_FINITE:
    case VARIANT_FADING_FIXED_LENGTH_ACCELERATING:
    case VARIANT_FADING_FIXED_LENGTH_DECELERATING:
      {
        guchar *p                = ctx->screen;
        gint    active_u0        = ctx->u1;
        gint    active_u1        = ctx->u0;
        gint    fy_shadow_height = fy - ctx->shadow_height;
        gint    u;

        p += ctx->active_u0 * ctx->pixel_size;

        for (u = ctx->active_u0; u < ctx->active_u1; u++, p += ctx->pixel_size)
          {
            Pixel    *pixel  = (Pixel *) p;
            gboolean  active = (pixel->shadow.value != 0.0);

            if (active && pixel->shadow.fy < fy_shadow_height)
              active = shift_pixel (ctx, pixel);

            if (active)
              {
                if (ctx->variant ==
                    VARIANT_FADING_FIXED_LENGTH_DECELERATING)
                  {
                    FFLPixel   *ffl_pixel = (FFLPixel *) p;
                    ShadowItem *item;

                    ffl_pixel->max1.value = shadow_value (ctx,
                                                          &pixel->shadow, fy);
                    ffl_pixel->max1.fy    = pixel->shadow.fy;
                    ffl_pixel->max2       = 0.0f;

                    for (item = pixel->queue; item; item = item->next)
                      {
                        gfloat value = shadow_value (ctx, &item->shadow, fy);

                        if (value > ffl_pixel->max1.value)
                          {
                            ffl_pixel->max2       = ffl_pixel->max1.value;

                            ffl_pixel->max1.value = value;
                            ffl_pixel->max1.fy    = item->shadow.fy;
                          }
                      }
                  }
                else if (ctx->variant ==
                         VARIANT_FADING_FIXED_LENGTH_ACCELERATING &&
                         pixel->queue)
                  {
                    ShadowItem *item      = pixel->queue->prev;
                    ShadowItem *last_item = item;
                    gfloat      prev_value;

                    prev_value = shadow_value (ctx, &item->shadow, fy);

                    item = item->prev;

                    while (item != last_item)
                      {
                        ShadowItem *prev = item->prev;
                        gfloat      value;

                        value = shadow_value (ctx, &item->shadow, fy);

                        if (value <= prev_value)
                          {
                            if (item->prev != last_item)
                              item->prev->next = item->next;
                            else
                              pixel->queue = item->next;

                            item->next->prev = item->prev;

                            g_slice_free (ShadowItem, item);
                          }
                        else
                          {
                            prev_value = value;
                          }

                        item = prev;
                      }

                    if (shadow_value (ctx, &pixel->shadow, fy) <= prev_value)
                      shift_pixel (ctx, pixel);
                  }

                active_u0 = MIN (active_u0, u);
                active_u1 = u + 1;
              }
          }

        ctx->active_u0 = active_u0;
        ctx->active_u1 = active_u1;
      }
      break;

    case VARIANT_FADING_FIXED_RATE_NONLINEAR:
      {
        FFRPixel *p         = ctx->screen;
        gint      active_u0 = ctx->u1;
        gint      active_u1 = ctx->u0;
        gint      u;

        for (u = ctx->active_u0; u < ctx->active_u1; u++)
          {
            FFRPixel *pixel = &p[u];

            if (! pixel->value)
              continue;

            if (pixel->last_fy < fy)
              {
                pixel->value = 0.0f;
              }
            else
              {
                pixel->value = fade_value (ctx, fy - pixel->fy);

                active_u0 = MIN (active_u0, u);
                active_u1 = u + 1;
              }
          }

        ctx->active_u0 = active_u0;
        ctx->active_u1 = active_u1;
      }
      break;

    case VARIANT_FADING_FIXED_RATE_LINEAR:
      {
        gfloat *p         = ctx->screen;
        gint    active_u0 = ctx->u1;
        gint    active_u1 = ctx->u0;
        gint    u;

        for (u = ctx->active_u0; u < ctx->active_u1; u++)
          {
            if (! p[u])
              continue;

            p[u] -= ctx->fade_rate;

            if (p[u] <= EPSILON)
              {
                p[u] = 0.0f;
              }
            else
              {
                active_u0 = MIN (active_u0, u);
                active_u1 = u + 1;
              }
          }

        ctx->active_u0 = active_u0;
        ctx->active_u1 = active_u1;
      }
      break;

    case VARIANT_FADING:
      {
        gfloat *p         = ctx->screen;
        gint    active_u0 = ctx->u1;
        gint    active_u1 = ctx->u0;
        gint    u;

        for (u = ctx->active_u0; u < ctx->active_u1; u++)
          {
            if (! p[u])
              continue;

            p[u] *= ctx->fade_rate;

            if (p[u] <= EPSILON)
              {
                p[u] = 0.0f;
              }
            else
              {
                active_u0 = MIN (active_u0, u);
                active_u1 = u + 1;
              }
          }

        ctx->active_u0 = active_u0;
        ctx->active_u1 = active_u1;
      }
      break;

    case VARIANT_INFINITE:
      break;
    }
}

static void
add_shadow (Context *ctx,
            gint     u0,
            gint     u1,
            gint     fy,
            gfloat   value)
{
  gint u;

  if (! value)
    return;

  u0 = MAX (u0, ctx->u0);
  u1 = MIN (u1, ctx->u1);

  switch (ctx->variant)
    {
    case VARIANT_FINITE:
    case VARIANT_FADING_FIXED_LENGTH_ACCELERATING:
    case VARIANT_FADING_FIXED_LENGTH_DECELERATING:
      {
        guchar *p = ctx->screen;

        p += u0 * ctx->pixel_size;

        for (u = u0; u < u1; u++, p += ctx->pixel_size)
          {
            Pixel *pixel = (Pixel *) p;

            if (value >= shadow_collation_value (ctx, &pixel->shadow, fy))
              {
                pixel->shadow.value = value;
                pixel->shadow.fy    = fy;

                clear_pixel_queue (ctx, pixel);
              }
            else if (! pixel->queue)
              {
                if (fy != pixel->shadow.fy)
                  {
                    pixel->queue = g_slice_new (ShadowItem);

                    pixel->queue->shadow.value = value;
                    pixel->queue->shadow.fy    = fy;
                    pixel->queue->next         = NULL;
                    pixel->queue->prev         = pixel->queue;
                  }
              }
            else
              {
                ShadowItem *item      = pixel->queue->prev;
                ShadowItem *last_item = item;
                ShadowItem *new_item  = NULL;

                do
                  {
                    if (value < shadow_collation_value (ctx, &item->shadow, fy))
                      break;

                    g_slice_free (ShadowItem, new_item);
                    new_item = item;

                    item = item->prev;
                  }
                while (item != last_item);

                if (! new_item)
                  {
                    if (fy == last_item->shadow.fy)
                      continue;

                    new_item = g_slice_new (ShadowItem);

                    new_item->prev  = last_item;
                    last_item->next = new_item;
                  }

                new_item->shadow.value = value;
                new_item->shadow.fy    = fy;
                new_item->next         = NULL;

                pixel->queue->prev     = new_item;
              }

            if (ctx->variant == VARIANT_FADING_FIXED_LENGTH_DECELERATING)
              {
                FFLPixel *ffl_pixel = (FFLPixel *) p;

                if (value >= ffl_pixel->max1.value)
                  {
                    ffl_pixel->max1.value = value;
                    ffl_pixel->max1.fy    = fy;
                  }
              }
          }
      }
      break;

    case VARIANT_FADING_FIXED_RATE_NONLINEAR:
      {
        FFRPixel *p = ctx->screen;

        for (u = u0; u < u1; u++)
          {
            FFRPixel *pixel = &p[u];

            if (value >= pixel->value)
              {
                pixel->value   = value;
                pixel->fy      = fy - powf (1.0f - value,
                                            ctx->fade_gamma_inv) *
                                      ctx->fade_rate_inv;
                pixel->last_fy = ceilf (pixel->fy + ctx->shadow_proj);
              }
          }
      }
      break;

    case VARIANT_FADING_FIXED_RATE_LINEAR:
    case VARIANT_INFINITE:
    case VARIANT_FADING:
      {
        gfloat *p = ctx->screen;

        for (u = u0; u < u1; u++)
          p[u] = MAX (p[u], value);
      }
      break;
    }

  ctx->active_u0 = MIN (ctx->active_u0, u0);
  ctx->active_u1 = MAX (ctx->active_u1, u1);
}

static inline void
add_shadow_at (Context *ctx,
               gint     u,
               gint     fy,
               gfloat   value)
{
  add_shadow (ctx,
              u + ctx->row_input_pixel_offset0,
              u + ctx->row_input_pixel_offset1,
              fy,
              value);
}

static inline gfloat
get_pixel_shadow (Context       *ctx,
                  gconstpointer  pixel,
                  gint           fy)
{
  switch (ctx->variant)
    {
    case VARIANT_FINITE:
    case VARIANT_FADING_FIXED_LENGTH_ACCELERATING:
      {
        const Pixel *p     = pixel;
        gfloat       value = shadow_value (ctx, &p->shadow, fy);

        if (value && p->shadow.fy + ctx->shadow_height == fy)
          {
            value *= ctx->shadow_remainder;

            if (p->queue)
              value += (1.0 - ctx->shadow_remainder) *
                       shadow_value (ctx, &p->queue->shadow, fy);
          }

        return value;
      }

    case VARIANT_FADING_FIXED_LENGTH_DECELERATING:
      {
        const FFLPixel *p     = pixel;
        gfloat          value = p->max1.value;

        if (value && p->max1.fy + ctx->shadow_height == fy)
          {
            value *= ctx->shadow_remainder;
            value += (1.0 - ctx->shadow_remainder) * p->max2;
          }

        return value;
      }

    case VARIANT_FADING_FIXED_RATE_NONLINEAR:
      {
        const FFRPixel *p     = pixel;
        gfloat          value = p->value;

        if (fy == p->last_fy)
          {
            gfloat remainder = p->fy + ctx->shadow_proj + 1.0f - fy;

            value *= remainder;
          }

        return value;
      }

    case VARIANT_FADING_FIXED_RATE_LINEAR:
    case VARIANT_INFINITE:
    case VARIANT_FADING:
      {
        const gfloat *p = pixel;

        return *p;
      }
    }

  g_return_val_if_reached (0.0f);
}

static void
init_buffers (Context    *ctx,
              GeglBuffer *input,
              GeglBuffer *output)
{
  ctx->input  = input;
  ctx->output = output;

  ctx->format = gegl_buffer_get_format (output);

  gegl_color_get_pixel (ctx->options.color, ctx->format, ctx->color);

  ctx->input_row  = g_new (gfloat, 4 * ctx->area.width);
  ctx->output_row = ctx->input_row;

  if (ctx->options.composition !=
      GEGL_LONG_SHADOW_COMPOSITION_SHADOW_PLUS_IMAGE)
    {
      ctx->output_row = g_new (gfloat, 4 * ctx->roi.width);

      gegl_memset_pattern (ctx->output_row,
                           ctx->color, 4 * sizeof (float),
                           ctx->roi.width);
    }

  if (! ctx->flip_horizontally)
    {
      if (ctx->output_row == ctx->input_row)
        ctx->output_row += 4 * (ctx->roi.x - ctx->area.x);

      ctx->input_row0  = ctx->input_row;
      ctx->output_row0 = ctx->output_row;

      ctx->row_step    = +4;
    }
  else
    {
      ctx->input_row0  = ctx->input_row  + 4 * (ctx->area.width - 1);
      ctx->output_row0 = ctx->output_row + 4 * (ctx->roi.width  - 1);

      ctx->row_step    = -4;
    }
}

static void
cleanup_buffers (Context *ctx)
{
  if (ctx->options.composition !=
      GEGL_LONG_SHADOW_COMPOSITION_SHADOW_PLUS_IMAGE)
    {
      g_free (ctx->output_row);
    }

  g_free (ctx->input_row);
}

static void
get_row (Context *ctx,
         gint     fy)
{
  GeglRectangle row = {ctx->area.x, fy, ctx->area.width, 1};

  transform_rect_to_image (ctx, &row, &row, FALSE);

  gegl_buffer_get (ctx->input,
                   &row, ctx->scale, ctx->format, ctx->input_row,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
}

static void
set_row (Context *ctx,
         gint     fy)
{
  GeglRectangle row = {ctx->roi.x, fy, ctx->roi.width, 1};

  transform_rect_to_image (ctx, &row, &row, FALSE);

  gegl_buffer_set (ctx->output,
                   &row, ctx->level, ctx->format, ctx->output_row,
                   GEGL_AUTO_ROWSTRIDE);
}

static gfloat
get_shadow_at (Context *ctx,
               gint     u,
               gint     fy)
{
  gfloat result = 0.0f;

  if (ctx->active_u0 < ctx->active_u1)
    {
      const guchar *pixel = ctx->screen;
      gint          u0, u1;
      gint          i;

      u0 = MAX (u,                              ctx->active_u0);
      u1 = MIN (u + ctx->row_output_pixel_span, ctx->active_u1);

      pixel += u0 * ctx->pixel_size;

      for (i = u0 - u; i < u1 - u; i++)
        {
          result += ctx->row_output_pixel_kernel[i] *
                    get_pixel_shadow (ctx, pixel, fy);

          pixel += ctx->pixel_size;
        }
    }

  return result;
}

static void
set_output_pixel (Context      *ctx,
                  const gfloat *input_pixel,
                  gfloat       *output_pixel,
                  gfloat        shadow_value)
{
  shadow_value = MAX (shadow_value, input_pixel[3]);

  switch (ctx->options.composition)
    {
    case GEGL_LONG_SHADOW_COMPOSITION_SHADOW_PLUS_IMAGE:
      shadow_value = (shadow_value - input_pixel[3]) * ctx->color[3];

      if (shadow_value > 0.0f)
        {
          gfloat alpha     = input_pixel[3] + shadow_value;
          gfloat alpha_inv = 1.0f / alpha;
          gint   i;

          for (i = 0; i < 3; i++)
            {
              output_pixel[i] = (input_pixel[3] * input_pixel[i] +
                                 shadow_value   * ctx->color[i]) * alpha_inv;
            }
          output_pixel[3] = alpha;
        }
      break;

    case GEGL_LONG_SHADOW_COMPOSITION_SHADOW_ONLY:
      output_pixel[3] = shadow_value * ctx->color[3];
      break;

    case GEGL_LONG_SHADOW_COMPOSITION_SHADOW_MINUS_IMAGE:
      output_pixel[3] = MAX (shadow_value - input_pixel[3], 0.0f) *
                        ctx->color[3];
      break;
    }
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  GeglRectangle   result = {};

  if (is_finite (o))
    {
      Context ctx;

      init_options  (&ctx, o, 0);
      init_geometry (&ctx);
      init_area     (&ctx, operation, roi);

      gegl_rectangle_intersect (&result, &ctx.area, &ctx.input_bounds);

      transform_rect_to_image (&ctx, &result, &result, TRUE);
    }
  else
    {
      GeglRectangle *in_rect =
        gegl_operation_source_get_bounding_box (operation, "input");

      if (in_rect)
        result = *in_rect;
    }

  return result;
}

static GeglRectangle
get_invalidated_by_change (GeglOperation       *operation,
                           const gchar         *input_pad,
                           const GeglRectangle *roi)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  GeglRectangle   result = {};

  if (is_finite (o))
    {
      Context ctx;
      gint    u1;
      gint    fx1;

      init_options  (&ctx, o, 0);
      init_geometry (&ctx);

      transform_rect_to_filter (&ctx, roi, &result, TRUE);

      get_affected_screen_range (&ctx,
                                 0, result.x + result.width,
                                 result.y,
                                 NULL, &u1);
      get_affected_filter_range (&ctx,
                                 0, u1,
                                 result.y + ctx.shadow_height,
                                 NULL, &fx1);

      fx1++;

      result.width   = fx1 - result.x;
      result.height += ctx.shadow_height;

      transform_rect_to_image (&ctx, &result, &result, TRUE);
    }
  else
    {
      GeglRectangle *in_rect =
        gegl_operation_source_get_bounding_box (operation, "input");

      if (in_rect)
        result = *in_rect;
    }

  return result;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result  = {};
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation,
                                                                   "input");

  if (in_rect)
    {
      GeglProperties *o = GEGL_PROPERTIES (operation);

      if (is_finite (o) && ! gegl_rectangle_is_infinite_plane (in_rect))
        result = get_invalidated_by_change (operation, "input", in_rect);
      else
        result = *in_rect;
    }

  return result;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  if (is_finite (o))
    return *roi;
  else
    return get_bounding_box (operation);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Context         ctx;
  gint            fx, fy;

  init_options  (&ctx, o, level);
  init_geometry (&ctx);
  init_fade     (&ctx);
  init_area     (&ctx, operation, roi);
  init_screen   (&ctx);
  init_buffers  (&ctx, input, output);

  for (fy = ctx.area.y; fy < ctx.area.y + ctx.area.height; fy++)
    {
      const gfloat *input_pixel;
      gfloat       *output_pixel;
      gint          u;

      init_row (&ctx, fy);

      get_row (&ctx, fy);

      trim_shadow (&ctx, fy);

      input_pixel  = ctx.input_row0 +
                     (ctx.row_fx0 - ctx.area.x) * ctx.row_step;
      output_pixel = ctx.output_row0;

      u = ctx.row_u0;

      for (fx = ctx.row_fx0; fx < ctx.row_fx1; fx++)
        {
          add_shadow_at (&ctx, u, fy, input_pixel[3]);

          if (fy >= ctx.roi.y && fx >= ctx.roi.x)
            {
              set_output_pixel (&ctx,
                                input_pixel, output_pixel,
                                get_shadow_at (&ctx, u, fy));

              output_pixel += ctx.row_step;
            }

          u += SCREEN_RESOLUTION;

          input_pixel += ctx.row_step;
        }

      if (fy >= ctx.roi.y)
        set_row (&ctx, fy);
    }

  o->user_data = ctx.options.user_data;

  cleanup_buffers (&ctx);
  cleanup_screen  (&ctx);

  return TRUE;
}

static void
dispose (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);

  if (o->user_data)
    {
      Data *data = o->user_data;

      g_free (data->fade_lut);

      g_slice_free (Data, data);

      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->dispose (object);
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

  if (in_rect && gegl_rectangle_is_infinite_plane (in_rect))
    {
      gpointer in = gegl_operation_context_get_object (context, "input");
      gegl_operation_context_take_object (context, "output",
                                          g_object_ref (G_OBJECT (in)));
      return TRUE;
    }

  operation_class = GEGL_OPERATION_CLASS (gegl_op_parent_class);

  return operation_class->process (operation, context, output_prop, result,
                                   gegl_operation_context_get_level (context));
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass             *object_class;
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  object_class->dispose                      = dispose;

  operation_class->get_required_for_output   = get_required_for_output;
  operation_class->get_invalidated_by_change = get_invalidated_by_change;
  operation_class->get_bounding_box          = get_bounding_box;
  operation_class->get_cached_region         = get_cached_region;
  operation_class->process                   = operation_process;

  /* FIXME:  we want 'threaded == TRUE' for finite shadows, and
   * 'threaded == FALSE' for infinite and fading shadows.  right now, there's
   * no way to control this dynamically, so we settle for the latter.
   */
  operation_class->threaded                  = FALSE;
  operation_class->want_in_place             = TRUE;

  filter_class->process                      = process;

  gegl_operation_class_set_keys (operation_class,
    "name",           "gegl:long-shadow",
    "title",          _("Long Shadow"),
    "categories",     "light",
    "needs-alpha",    "true",
    "reference-hash", "7e3c16678d971e1ecb3c204770659bfd",
    "description",    _("Creates a long-shadow effect"),
    NULL);
}

#endif
