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
 * Copyright 2011 Jan Rüegg <rggjan@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int (iterations, _("Iterations"), 10)
    value_range (1, 10000)
    ui_range (1, 200)

property_seed (seed, _("Random seed"), rand)

#else

#define GEGL_OP_COMPOSER
#define GEGL_OP_NAME     matting_global
#define GEGL_OP_C_SOURCE matting-global.c

#include "gegl-op.h"
#include "gegl-debug.h"

#define max(a,b) \
  ({ __typeof__ (a) _a = (a); \
  __typeof__ (b) _b = (b); \
  _a > _b ? _a : _b; })

#define min(a,b) \
  ({ __typeof__ (a) _a = (a); \
  __typeof__ (b) _b = (b); \
  _a > _b ? _b : _a; })

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)       __builtin_expect((x),0)

#define ASSERT(condition) \
  if(unlikely(!(condition))) { \
  printf("Error at line %i\n", __LINE__); \
  exit(1);\
  }

/* We don't use the babl_format_get_n_components function for these values,
 * as literal constants can be used for stack allocation of array sizes. They
 * are double checked in matting_process.
 */
#define COMPONENTS_AUX    1
#define COMPONENTS_INPUT  3
#define COMPONENTS_OUTPUT 1

static const gchar *FORMAT_AUX    = "Y u8";
static const gchar *FORMAT_INPUT  = "R'G'B' float";
static const gchar *FORMAT_OUTPUT = "Y float";

static void
matting_prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  const Babl *in_format  = babl_format_with_space (FORMAT_INPUT, space);
  const Babl *aux_format = babl_format_with_space (FORMAT_AUX, space);
  const Babl *out_format = babl_format_with_space (FORMAT_OUTPUT, space);

  gegl_operation_set_format (operation, "input",  in_format);
  gegl_operation_set_format (operation, "aux",    aux_format);
  gegl_operation_set_format (operation, "output", out_format);
}

static GeglRectangle
matting_get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result  = {};
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation,
                                                                   "input");

  if (in_rect)
    result = *in_rect;

  return result;
}

static GeglRectangle
matting_get_invalidated_by_change (GeglOperation       *operation,
                                   const gchar         *input_pad,
                                   const GeglRectangle *roi)
{
  return matting_get_bounding_box (operation);
}

static GeglRectangle
matting_get_required_for_output (GeglOperation       *operation,
                                 const gchar         *input_pad,
                                 const GeglRectangle *roi)
{
  return matting_get_bounding_box (operation);
}

static GeglRectangle
matting_get_cached_region (GeglOperation * operation,
                           const GeglRectangle * roi)
{
  return matting_get_bounding_box (operation);
}

typedef struct {
 gfloat fg_distance;
 gfloat bg_distance;
 guint  fg_index;
 guint  bg_index;
} BufferRecord;

typedef float Color[3];

typedef struct {
  int x;
  int y;
} Position;

typedef struct {
  Color color;
  Position pos;
} ColorSample;

#define SQUARE(x) ((x)*(x))

static inline gfloat
get_alpha (Color F,
           Color B,
           Color I)
{
  gint c;
  gfloat result = 0.f;
  gfloat div = 0.f;

  for (c = 0; c < 3; c++)
    {
      result += (I[c] - B[c]) * (F[c] - B[c]);
      div += SQUARE(F[c] - B[c]);
    }

  return min(max(result / div, 0), 1);
}

static inline float
get_color_cost (Color F,
                Color B,
                Color I,
                float alpha)
{
  int c;
  float result = 0;
  for (c = 0; c < 3; c++)
    {
      result += SQUARE(I[c] - (alpha * F[c] + (1 - alpha) * B[c]));
    }

  // TODO(rggjan): Remove sqrt to get faster code?
  // TODO(rggjan): Remove 255
  return sqrt(result) * 255;
}

static inline int
get_distance_squared (ColorSample s,
                      int         x,
                      int         y)
{
  return SQUARE(s.pos.x - x) + SQUARE(s.pos.y - y);
}

static inline float
get_distance (ColorSample s,
              int         x,
              int         y)
{
  // TODO(rggjan): Remove sqrt to get faster code?
  return sqrt (get_distance_squared (s, x, y));
}


static inline float
get_distance_cost (ColorSample s,
                   int         x,
                   int         y,
                   float      *best_distance)
{
  float new_distance = get_distance (s, x, y);

  if (new_distance < *best_distance)
    *best_distance = new_distance;

  return new_distance / *best_distance;
}

static inline float
get_cost (ColorSample fg,
          ColorSample bg,
          Color       I,
          int         x,
          int         y,
          float      *best_fg_distance,
          float      *best_bg_distance)
{
  float cost = get_color_cost (fg.color, bg.color, I,
                               get_alpha (fg.color, bg.color, I));
  cost += get_distance_cost (fg, x, y, best_fg_distance);
  cost += get_distance_cost (bg, x, y, best_bg_distance);
  return cost;
}

static inline void
do_propagate (GArray        *fg_samples,
              GArray        *bg_samples,
              gfloat        *input,
              BufferRecord  *buffer,
              guchar        *trimap,
              int            x,
              int            y,
              int            w,
              int            h)
{
  int index_orig = y * w + x;
  int index_new;

  if (!(trimap[index_orig] == 0 || trimap[index_orig] == 255))
    {
      int    xdiff, ydiff;
      float  best_cost        = FLT_MAX;
      float *best_fg_distance = &buffer[index_orig].fg_distance;
      float *best_bg_distance = &buffer[index_orig].bg_distance;

      for (ydiff = -1; ydiff <= 1; ydiff++)
        {
          // Borders
          if (y+ydiff < 0 || y+ydiff >= h)
            continue;
          for (xdiff = -1; xdiff <= 1; xdiff++)
            {
              // Borders
              if (x+xdiff < 0 || x+xdiff >= w)
                continue;

              index_new = (y + ydiff) * w + (x + xdiff);

              if (! (trimap[index_new] == 0 || trimap[index_new] == 255))
                {
                  guint fi = buffer[index_new].fg_index;
                  guint bi = buffer[index_new].bg_index;

                  ColorSample fg = g_array_index (fg_samples, ColorSample, fi);
                  ColorSample bg = g_array_index (bg_samples, ColorSample, bi);

                  float cost = get_cost (fg, bg, &input[index_orig * 3], x, y,
                                         best_fg_distance, best_bg_distance);
                  if (cost < best_cost)
                    {
                      buffer[index_orig].fg_index = fi;
                      buffer[index_orig].bg_index = bi;
                      best_cost = cost;
                    }
                }
            }
        }
    }
}

static inline void
do_random_search (GArray        *fg_samples,
                  GArray        *bg_samples,
                  gfloat        *input,
                  BufferRecord  *buffer,
                  int            x,
                  int            y,
                  int            w,
                  GeglRandom    *gr)
{
  guint dist_f = fg_samples->len;
  guint dist_b = bg_samples->len;
  guint fl     = fg_samples->len;
  guint bl     = bg_samples->len;

  int index = y * w + x;

  guint best_fi = buffer[index].fg_index;
  guint best_bi = buffer[index].bg_index;

  guint start_fi = best_fi;
  guint start_bi = best_bi;

  // Get current best result
  float *best_fg_distance = &buffer[index].fg_distance;
  float *best_bg_distance = &buffer[index].bg_distance;

  ColorSample fg = g_array_index (fg_samples, ColorSample, best_fi);
  ColorSample bg = g_array_index (bg_samples, ColorSample, best_bi);

  // Get cost
  float best_cost = get_cost (fg, bg, &input[index * 3], x, y,
                              best_fg_distance, best_bg_distance);

  while (dist_f > 0 || dist_b > 0)
    {
      // Get new indices to check
      guint fgi = gegl_random_int (gr, x, y, 0, 0);
      guint bgi = gegl_random_int (gr, x, y, 0, 1);
      guint fi  = (start_fi + (fgi % (dist_f * 2 + 1)) + fl - dist_f) % fl;
      guint bi  = (start_bi + (bgi % (dist_b * 2 + 1)) + bl - dist_b) % bl;

      ColorSample fg = g_array_index (fg_samples, ColorSample, fi);
      ColorSample bg = g_array_index (bg_samples, ColorSample, bi);

      float cost = get_cost (fg, bg, &input[index * 3], x, y,
                             best_fg_distance, best_bg_distance);

      if (cost < best_cost)
        {
          best_cost = cost;
          best_fi   = fi;
          best_bi   = bi;
        }

      dist_f /= 2;
      dist_b /= 2;
    }

  buffer[index].fg_index = best_fi;
  buffer[index].bg_index = best_bi;
}

// Compare color intensities
static gint
color_compare (gconstpointer p1, gconstpointer p2)
{
  ColorSample *s1 = (ColorSample *) p1;
  ColorSample *s2 = (ColorSample *) p2;

  float sum1 = s1->color[0] + s1->color[1] + s1->color[2];
  float sum2 = s2->color[0] + s2->color[1] + s2->color[2];

  return ((sum1 > sum2) - (sum2 > sum1));
}

static void
fill_result (GeglBuffer   *output,
             const Babl   *format,
             guchar       *trimap,
             gfloat       *input,
             BufferRecord *buffer,
             GArray       *fg_samples,
             GArray       *bg_samples)
{
  GeglBufferIterator  *iter;

  iter = gegl_buffer_iterator_new (output, NULL, 0, format, GEGL_ACCESS_WRITE,
                                   GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat  *out = iter->items[0].data;
      gint     x   = iter->items[0].roi.x;
      gint     y   = iter->items[0].roi.y;
      glong    n_pixels = iter->length;

      while (n_pixels--)
        {
          gint index = x + y * gegl_buffer_get_width (output);

          if (trimap[index] == 0 || trimap[index] == 255)
            {

              if (trimap[index] == 0)
                {
                  *out = 0.f;
                }
              else if (trimap[index] == 255)
                {
                  *out = 1.f;
                }
            }
          else
            {
              ColorSample fg;
              ColorSample bg;
              fg = g_array_index (fg_samples, ColorSample, buffer[index].fg_index);
              bg = g_array_index (bg_samples, ColorSample, buffer[index].bg_index);

              *out = get_alpha (fg.color, bg.color, &input[index * 3]);
            }

          out++;
          x++;

          if (x >= iter->items[0].roi.x + iter->items[0].roi.width)
            {
              x = iter->items[0].roi.x;
              y++;
            }
        }
    }
}

static gboolean
matting_process (GeglOperation       *operation,
                 GeglBuffer          *input_buf,
                 GeglBuffer          *aux_buf,
                 GeglBuffer          *output,
                 const GeglRectangle *result,
                 int                  level)
{
  const GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *space      = gegl_operation_get_source_space (operation, "input");
  const Babl *in_format  = babl_format_with_space (FORMAT_INPUT, space);
  const Babl *aux_format = babl_format_with_space (FORMAT_AUX, space);
  const Babl *out_format = babl_format_with_space (FORMAT_OUTPUT, space);

  gfloat        *input   = NULL;
  guchar        *trimap  = NULL;
  BufferRecord  *buffer  = NULL;

  gboolean       success = FALSE;
  GeglRandom    *gr      = gegl_random_new_with_seed (o->seed);
  int            w, h, i, x, y, xdiff, ydiff, neighbour_mask;

  GArray        *fg_samples;
  GArray        *bg_samples;
  GArray        *unknown_positions;


  w = result->width;
  h = result->height;

  input  = g_new (gfloat, w * h * COMPONENTS_INPUT);
  trimap = g_new (guchar, w * h * COMPONENTS_AUX);
  buffer = g_new0 (BufferRecord, w * h);

  gegl_buffer_get (input_buf, result, 1.0, in_format, input,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  gegl_buffer_get (aux_buf, result, 1.0, aux_format, trimap,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  fg_samples = g_array_new (FALSE, FALSE, sizeof (ColorSample));
  bg_samples = g_array_new (FALSE, FALSE, sizeof (ColorSample));
  unknown_positions  = g_array_new (FALSE, FALSE, sizeof (Position));

  // Get mask
  for (y = 0; y < h; y++)
    {
      for (x = 0; x < w; x++)
        {
          int mask = trimap[y * w + x];
          for (ydiff = -1; ydiff <= 1; ydiff++)
            {
              // Borders
              if (y+ydiff < 0 || y+ydiff >= h)
                continue;

              for (xdiff = -1; xdiff <= 1; xdiff++)
                {
                  // Borders
                  if (x+xdiff < 0 || x+xdiff >= w)
                    continue;

                  neighbour_mask = trimap[(y + ydiff) * w + x + xdiff];
                  if (neighbour_mask != mask && (mask == 0 || mask == 255))
                    {
                      int index = y*w+x;
                      ColorSample s;
                      s.pos.x = x;
                      s.pos.y = y;
                      s.color[0] = input[index * 3];
                      s.color[1] = input[index * 3 + 1];
                      s.color[2] = input[index * 3 + 2];

                      if (mask == 255)
                        {
                          g_array_append_val (fg_samples, s);
                          buffer[index].fg_distance = 0;
                          buffer[index].bg_distance = FLT_MAX;
                        }
                      else
                        {
                          g_array_append_val (bg_samples, s);
                          buffer[index].fg_distance = 0;
                          buffer[index].bg_distance = FLT_MAX;
                        }

                      // Go to next pixel
                      xdiff = 1;
                      ydiff = 1;
                    }
                }
            }
        }
    }

  /* If we have no information to work with, there is nothing to process. */
  if (fg_samples->len == 0 || bg_samples->len == 0)
    {
      success = FALSE;
      goto cleanup;
    }

  // Initialize unknowns
  for (y = 0; y < h; y++)
    {
      for (x = 0; x < w; x++)
        {
          int index = y * w + x;

          if (trimap[index] != 0 && trimap[index] != 255)
            {
              Position p;
              guint fgi = gegl_random_int (gr, x, y, 0, 0);
              guint bgi = gegl_random_int (gr, x, y, 0, 1);
              p.x = x;
              p.y = y;
              g_array_append_val (unknown_positions, p);
              buffer[index].fg_distance = FLT_MAX;
              buffer[index].bg_distance = FLT_MAX;
              buffer[index].fg_index = fgi % fg_samples->len;
              buffer[index].bg_index = bgi % bg_samples->len;
            }
        }
    }

  g_array_sort (fg_samples, color_compare);
  g_array_sort (bg_samples, color_compare);

  // Do real iterations
  for (i = 0; i < o->iterations; i++)
    {
      guint j;

      GEGL_NOTE (GEGL_DEBUG_PROCESS, "Iteration %i", i);

      for (j=0; j<unknown_positions->len; j++)
        {
          Position p = g_array_index (unknown_positions, Position, j);
          do_random_search (fg_samples, bg_samples, input, buffer, p.x, p.y, w, gr);
        }

      for (j=0; j<unknown_positions->len; j++)
        {
          Position p = g_array_index (unknown_positions, Position, j);
          do_propagate (fg_samples, bg_samples, input, buffer, trimap, p.x, p.y, w, h);
        }
    }

  fill_result (output, out_format, trimap, input, buffer, fg_samples, bg_samples);

  success = TRUE;

cleanup:
  g_free (input);
  g_free (trimap);
  g_free (buffer);
  g_array_free (fg_samples, TRUE);
  g_array_free (bg_samples, TRUE);
  g_array_free (unknown_positions, TRUE);
  gegl_random_free (gr);

  return success;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass         *operation_class;
  GeglOperationComposerClass *composer_class;

  composer_class  = GEGL_OPERATION_COMPOSER_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);

  composer_class->process                    = matting_process;
  operation_class->prepare                   = matting_prepare;
  operation_class->get_bounding_box          = matting_get_bounding_box;
  operation_class->get_invalidated_by_change = matting_get_invalidated_by_change;
  operation_class->get_required_for_output   = matting_get_required_for_output;
  operation_class->get_cached_region         = matting_get_cached_region;
  operation_class->threaded                  = FALSE;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:matting-global",
    "categories" , "matting",
    "title"      , _("Matting Global"),
    "description",
   _("Given a sparse user supplied tri-map and an input image, create a "
     "foreground alpha matte. Set white as foreground, black as background "
     "for the tri-map. Everything else will be treated as unknown and filled in."),
    NULL);
}

#endif
