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

#ifdef GEGL_PROPERTIES

enum_start (gegl_long_shadow_style)
  enum_value (GEGL_LONG_SHADOW_STYLE_FINITE,   "finite",   N_("Finite"))
  enum_value (GEGL_LONG_SHADOW_STYLE_INFINITE, "infinite", N_("Infinite"))
  enum_value (GEGL_LONG_SHADOW_STYLE_FADING,   "fading",   N_("Fading"))
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
  ui_meta     ("visible", "style {finite}")

property_double (midpoint, _("Midpoint"), 100.0)
  description (_("Shadow fade midpoint"))
  value_range (0.0, G_MAXDOUBLE)
  ui_range    (0.0, 1000.0)
  ui_meta     ("visible", "style {fading}")

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
#include <math.h>

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
  GeglProperties     options;

  /* image -> filter coordinate transformation */
  gboolean           flip_horizontally;
  gboolean           flip_vertically;
  gboolean           flip_diagonally;

  /* in filter coordinates */
  gdouble            tan_angle;

  gdouble            sample_offset_x;
  gdouble            sample_offset_y;

  gint               shadow_height;

  gfloat             fade_rate;

  GeglRectangle      input_bounds;
  GeglRectangle      roi;
  GeglRectangle      area;

  /* in screen coordinates */
  gint               u0, u1;

  gpointer           screen;
  gint               pixel_size;
  gint               n_active_pixels;
  gint               prev_n_active_pixels;

  gdouble            filter_pixel_shadow_width;

  GeglBuffer        *input;
  GeglBuffer        *output;

  const Babl        *format;

  gfloat            *input_row;
  gfloat            *output_row;

  gfloat            *input_row0;
  gfloat            *output_row0;
  gint               row_step;

  GeglSampler       *sampler;
  GeglSamplerGetFun  sampler_get_fun;

  gfloat             color[4];

  gint               level;
  gdouble            scale;
} Context;

static void
init_options (Context              *ctx,
              const GeglProperties *options,
              gint                  level)
{
  ctx->options = *options;

  ctx->level = level;
  ctx->scale = 1.0 / (1 << level);

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

  ctx->sample_offset_x = -sin (ctx->options.angle) * ctx->options.length;
  ctx->sample_offset_y = -cos (ctx->options.angle) * ctx->options.length;

  ctx->shadow_height = floor (-ctx->sample_offset_y);

  if (ctx->options.midpoint > EPSILON)
    {
      ctx->fade_rate = pow (0.5, 1.0 / (cos (ctx->options.angle) *
                                        ctx->options.midpoint));
    }
  else
    {
      ctx->fade_rate = 0.0f;
    }
}

static inline void
transform_coords_to_filter (Context *ctx,
                            gdouble  ix,
                            gdouble  iy,
                            gdouble *fx,
                            gdouble *fy)
{
  if (ctx->flip_diagonally)
    SWAP (gdouble, ix, iy);

  if (ctx->flip_horizontally)
    ix = -ix;

  if (ctx->flip_vertically)
    iy = -iy;

  *fx = ix;
  *fy = iy;
}

static inline void
transform_coords_to_image (Context *ctx,
                           gdouble  fx,
                           gdouble  fy,
                           gdouble *ix,
                           gdouble *iy)
{
  if (ctx->flip_vertically)
    fy = -fy;

  if (ctx->flip_horizontally)
    fx = -fx;

  if (ctx->flip_diagonally)
    SWAP (gdouble, fx, fy);

  *ix = fx;
  *iy = fy;
}

static inline void
transform_rect_to_filter (Context             *ctx,
                          const GeglRectangle *irect,
                          GeglRectangle       *frect)
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
}

static inline void
transform_rect_to_image (Context             *ctx,
                         const GeglRectangle *frect,
                         GeglRectangle       *irect)
{
  *irect = *frect;

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
  if (fx0) *fx0 = floor (project_to_filter (ctx, u0, fy));
  if (fx1) *fx1 = ceil  (project_to_filter (ctx, u1, fy));
}

static inline void
get_affecting_screen_range (Context *ctx,
                            gint     fx0,
                            gint     fx1,
                            gint     fy,
                            gint    *u0,
                            gint    *u1)
{
  if (u0) *u0 = floor (project_to_screen (ctx, fx0, fy));
  if (u1) *u1 = ceil  (project_to_screen (ctx, fx1, fy));
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

static void
init_area (Context             *ctx,
           GeglOperation       *operation,
           const GeglRectangle *roi)
{
  GeglRectangle *input_bounds;

  input_bounds = gegl_operation_source_get_bounding_box (operation, "input");

  if (input_bounds)
    transform_rect_to_filter (ctx, input_bounds, &ctx->input_bounds);
  else
    ctx->input_bounds = *GEGL_RECTANGLE (0, 0, 0, 0);

  transform_rect_to_filter (ctx, roi, &ctx->roi);

  get_affecting_screen_range (ctx,
                              ctx->roi.x, 0,
                              ctx->roi.y + ctx->roi.height - 1,
                              &ctx->u0, NULL);
  get_affecting_screen_range (ctx,
                              0, ctx->roi.x + ctx->roi.width,
                              ctx->roi.y,
                              NULL, &ctx->u1);

  ctx->area = ctx->roi;

  if (ctx->options.style == GEGL_LONG_SHADOW_STYLE_FINITE)
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
  if (ctx->options.style == GEGL_LONG_SHADOW_STYLE_FINITE)
    ctx->pixel_size = sizeof (Pixel);
  else
    ctx->pixel_size = sizeof (gfloat);

  ctx->screen = g_malloc0 (ctx->pixel_size * (ctx->u1 - ctx->u0));
  ctx->screen = (guchar *) ctx->screen - ctx->pixel_size * ctx->u0;

  ctx->n_active_pixels      = 0;
  ctx->prev_n_active_pixels = 1;

  ctx->filter_pixel_shadow_width = SCREEN_RESOLUTION * (1.0 + ctx->tan_angle);
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
  if (ctx->options.style == GEGL_LONG_SHADOW_STYLE_FINITE)
    {
      Pixel *p = ctx->screen;
      gint   u;

      for (u = ctx->u0; u < ctx->u1; u++)
        clear_pixel_queue (ctx, &p[u]);
    }

  ctx->screen = (guchar *) ctx->screen + ctx->pixel_size * ctx->u0;

  g_free (ctx->screen);
}

static inline void
shift_pixel (Context *ctx,
             Pixel   *pixel)
{
  ShadowItem *item = pixel->queue;

  if (! item)
    {
      pixel->shadow.value = 0.0f;

      ctx->n_active_pixels--;

      return;
    }

  pixel->shadow        = item->shadow;
  pixel->queue         = item->next;
  if (pixel->queue)
    pixel->queue->prev = item->prev;

  g_slice_free (ShadowItem, item);
}

static void
trim_shadow (Context *ctx,
             gint     fy)
{
  ctx->prev_n_active_pixels = ctx->n_active_pixels;

  if (! ctx->n_active_pixels)
    return;

  switch (ctx->options.style)
    {
    case GEGL_LONG_SHADOW_STYLE_FINITE:
      {
        Pixel *p = ctx->screen;
        gint   u;

        fy -= ctx->shadow_height;

        for (u = ctx->u0; u < ctx->u1; u++)
          {
            Pixel *pixel = &p[u];

            while (pixel->shadow.value && pixel->shadow.fy < fy)
              shift_pixel (ctx, pixel);
          }
      }
      break;

    case GEGL_LONG_SHADOW_STYLE_FADING:
      {
        gfloat   *p             = ctx->screen;
        gboolean  active_pixels = FALSE;
        gint      u;

        for (u = ctx->u0; u < ctx->u1; u++)
          {
            p[u] *= ctx->fade_rate;

            if (p[u] < EPSILON)
              p[u] = 0.0f;
            else
              active_pixels = TRUE;
          }

        ctx->n_active_pixels = active_pixels;
      }
      break;

    default:
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

  if (ctx->options.style == GEGL_LONG_SHADOW_STYLE_FINITE)
    {
      Pixel *p = ctx->screen;

      for (u = u0; u < u1; u++)
        {
          Pixel *pixel = &p[u];

          if (value >= pixel->shadow.value)
            {
              ctx->n_active_pixels += ! pixel->shadow.value;

              pixel->shadow.value = value;
              pixel->shadow.fy    = fy;

              clear_pixel_queue (ctx, pixel);
            }
          else if (! pixel->queue)
            {
              pixel->queue = g_slice_new (ShadowItem);

              pixel->queue->shadow.value = value;
              pixel->queue->shadow.fy    = fy;
              pixel->queue->next         = NULL;
              pixel->queue->prev         = pixel->queue;
            }
          else
            {
              ShadowItem *item      = pixel->queue->prev;
              ShadowItem *last_item = item;
              ShadowItem *new_item  = NULL;

              do
                {
                  if (value < item->shadow.value)
                    break;

                  g_slice_free (ShadowItem, new_item);
                  new_item = item;

                  item = item->prev;
                }
              while (item != last_item);

              if (! new_item)
                {
                  new_item = g_slice_new (ShadowItem);

                  new_item->prev  = last_item;
                  last_item->next = new_item;
                }

              new_item->shadow.value = value;
              new_item->shadow.fy    = fy;
              new_item->next         = NULL;

              pixel->queue->prev     = new_item;
            }
        }
    }
  else
    {
      gfloat *p = ctx->screen;

      for (u = u0; u < u1; u++)
        p[u] = MAX (p[u], value);

      ctx->n_active_pixels = 1;
    }
}

static inline void
add_shadow_at (Context *ctx,
               gdouble  u0,
               gint     fy,
               gfloat   value)
{
  add_shadow (ctx,
              floor (u0 + 0.5),
              floor (u0 + ctx->filter_pixel_shadow_width + 0.5),
              fy,
              value);
}

static inline gfloat
get_pixel_shadow (Context       *ctx,
                  gconstpointer  pixel,
                  gint           fy)
{
  if (ctx->options.style == GEGL_LONG_SHADOW_STYLE_FINITE)
    {
      const Pixel *p = pixel;

      return p->shadow.value;
    }
  else
    {
      const gfloat *p = pixel;

      return *p;
    }
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

  if (ctx->options.style == GEGL_LONG_SHADOW_STYLE_FINITE)
    {
      ctx->sampler = gegl_buffer_sampler_new_at_level (input, ctx->format,
                                                       GEGL_SAMPLER_LINEAR,
                                                       ctx->level);

      ctx->sampler_get_fun = gegl_sampler_get_fun (ctx->sampler);
    }
  else
    {
      ctx->sampler = NULL;
    }
}

static void
cleanup_buffers (Context *ctx)
{
  g_clear_object (&ctx->sampler);

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

  transform_rect_to_image (ctx, &row, &row);

  gegl_buffer_get (ctx->input,
                   &row, ctx->scale, ctx->format, ctx->input_row,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
}

static void
set_row (Context *ctx,
         gint     fy)
{
  GeglRectangle row = {ctx->roi.x, fy, ctx->roi.width, 1};

  transform_rect_to_image (ctx, &row, &row);

  gegl_buffer_set (ctx->output,
                   &row, ctx->level, ctx->format, ctx->output_row,
                   GEGL_AUTO_ROWSTRIDE);
}

static gfloat
get_shadow (Context *ctx,
            gdouble  u0,
            gdouble  u1,
            gint     fx,
            gint     fy)
{
  gfloat result = 0.0f;

  if (ctx->n_active_pixels)
    {
      gdouble       u0i, u0f;
      gdouble       u1i, u1f;
      gint          a, b;
      const guchar *pixel;
      gint          u;

      u0 = MAX (u0, ctx->u0);
      u1 = MIN (u1, ctx->u1);

      u0i = ceil (u0);
      u0f = u0i - u0;

      u1i = floor (u1);
      u1f = u1 - u1i;

      a = u0i;
      b = u1i;

      pixel  = ctx->screen;
      pixel += (a - 1) * ctx->pixel_size;

      if (u0f)
        result += get_pixel_shadow (ctx, pixel, fy) * u0f;
      pixel += ctx->pixel_size;

      for (u = a; u < b; u++)
        {
          result += get_pixel_shadow (ctx, pixel, fy);
          pixel  += ctx->pixel_size;
        }

      if (u1f)
        result += get_pixel_shadow (ctx, pixel, fy) * u1f;

      result /= SCREEN_RESOLUTION;
    }

  if (ctx->sampler && (ctx->n_active_pixels || ctx->prev_n_active_pixels))
    {
      gdouble ix, iy;
      gfloat  sample[4];

      transform_coords_to_image (ctx,
                                 fx + 0.5 + ctx->sample_offset_x,
                                 fy + 0.5 + ctx->sample_offset_y,
                                 &ix, &iy);

      ctx->sampler_get_fun (ctx->sampler,
                            ix, iy, NULL, sample, GEGL_ABYSS_NONE);

      result = MAX (result, sample[3]);
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

  if (o->style == GEGL_LONG_SHADOW_STYLE_FINITE)
    {
      Context ctx;
      gdouble sample_x;
      gdouble sample_y;

      init_options  (&ctx, o, 0);
      init_geometry (&ctx);
      init_area     (&ctx, operation, roi);

      sample_x = ctx.roi.x + ctx.sample_offset_x;
      sample_y = ctx.roi.y + ctx.sample_offset_y;

      result.x      = floor (sample_x);
      result.y      = floor (sample_y);
      result.width  = roi->width  + (result.x < sample_x);
      result.height = roi->height + (result.y < sample_y);

      gegl_rectangle_bounding_box (&result, &result, &ctx.area);

      gegl_rectangle_intersect (&result, &result, &ctx.input_bounds);

      transform_rect_to_image (&ctx, &result, &result);
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

  if (o->style == GEGL_LONG_SHADOW_STYLE_FINITE)
    {
      Context ctx;
      gint    u1;
      gint    fx1;

      init_options  (&ctx, o, 0);
      init_geometry (&ctx);

      transform_rect_to_filter (&ctx, roi, &result);

      get_affected_screen_range (&ctx,
                                 0, result.x + result.width,
                                 result.y,
                                 NULL, &u1);
      get_affected_filter_range (&ctx,
                                 0, u1,
                                 result.y + ctx.shadow_height,
                                 NULL, &fx1);

      fx1++;

      result.width  += ceil (-ctx.sample_offset_x);
      result.height += ceil (-ctx.sample_offset_y);

      result.width = MAX (result.width, fx1 - result.x);

      transform_rect_to_image (&ctx, &result, &result);
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

      if (o->style == GEGL_LONG_SHADOW_STYLE_FINITE)
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

  if (o->style == GEGL_LONG_SHADOW_STYLE_FINITE)
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
  Context ctx;
  gint    fx, fy;

  init_options  (&ctx, GEGL_PROPERTIES (operation), level);
  init_geometry (&ctx);
  init_area     (&ctx, operation, roi);
  init_screen   (&ctx);
  init_buffers  (&ctx, input, output);

  for (fy = ctx.area.y; fy < ctx.area.y + ctx.area.height; fy++)
    {
      const gfloat *input_pixel;
      gfloat       *output_pixel;
      gdouble       u0;
      gdouble       shadow_offset;
      gint          fx0, fx1;

      get_row (&ctx, fy);

      if (fy > ctx.roi.y)
        trim_shadow (&ctx, fy);

      get_affecting_filter_range (&ctx,
                                  ctx.u0, ctx.u1,
                                  fy,
                                  &fx0, &fx1);

      fx0 = MAX (fx0, ctx.area.x);
      fx1 = MIN (fx1, ctx.area.x + ctx.area.width);

      u0 = project_to_screen (&ctx, fx0, fy);

      shadow_offset = project_to_screen (&ctx, fx0, fy + 0.5) - u0;

      input_pixel  = ctx.input_row0 + (fx0 - ctx.area.x) * ctx.row_step;
      output_pixel = ctx.output_row0;

      for (fx = fx0; fx < fx1; fx++)
        {
          add_shadow_at (&ctx, u0 + shadow_offset, fy, input_pixel[3]);

          if (fy >= ctx.roi.y && fx >= ctx.roi.x)
            {
              gfloat shadow_value;

              shadow_value = get_shadow (&ctx,
                                         u0, u0 + SCREEN_RESOLUTION,
                                         fx, fy);

              set_output_pixel (&ctx,
                                input_pixel, output_pixel,
                                shadow_value);

              output_pixel += ctx.row_step;
            }

          u0 += SCREEN_RESOLUTION;

          input_pixel += ctx.row_step;
        }

      if (fy >= ctx.roi.y)
        set_row (&ctx, fy);
    }

  cleanup_buffers (&ctx);
  cleanup_screen  (&ctx);

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->get_required_for_output   = get_required_for_output;
  operation_class->get_invalidated_by_change = get_invalidated_by_change;
  operation_class->get_bounding_box          = get_bounding_box;
  operation_class->get_cached_region         = get_cached_region;

  /* FIXME:  we want 'threaded == TRUE, want_in_place == FALSE' for finite
   * shadows, and 'threaded == FALSE, want_in_place == TRUE' for infinite and
   * fading shadows.  right now, since there's no way to control these
   * attributes dynamically, we settle for the lowest common denominator.
   */
  operation_class->threaded                  = FALSE;
  operation_class->want_in_place             = FALSE;

  filter_class->process                      = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:long-shadow",
    "title",       _("Long Shadow"),
    "categories",  "light",
    "needs-alpha", "true",
    "description", _("Creates a long-shadow effect"),
    NULL);
}

#endif
