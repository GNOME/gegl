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
 * Copyright 2020 Thomas Manni <thomas.manni@free.fr>
 *
 * expected input buffers:
 *   - input : current selection
 *   - aux   : color image
 *   - aux2  : scribbles
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_paint_select_mode_type)
  enum_value (GEGL_PAINT_SELECT_MODE_ADD,      "add",      N_("Add"))
  enum_value (GEGL_PAINT_SELECT_MODE_SUBTRACT, "subtract", N_("Subtract"))
enum_end (GeglPaintSelectModeType)

property_enum (mode, _("Mode"),
               GeglPaintSelectModeType, gegl_paint_select_mode_type,
               GEGL_PAINT_SELECT_MODE_ADD)
    description (_("Either to add to or subtract from the mask"))

#else

#define GEGL_OP_COMPOSER3
#define GEGL_OP_NAME      paint_select
#define GEGL_OP_C_SOURCE  paint-select.cc

#include <maxflow/graph.h>
#include <math.h>
#include "gegl-op.h"

#define SELECTION_FORMAT "Y float"
#define SCRIBBLES_FORMAT "Y float"
#define COLORS_FORMAT    "R'G'B' float"

#define POW2(x) ((x)*(x))
#define BIG_CAPACITY 100.f
#define EPSILON 0.05f
#define N_GLOBAL_SAMPLES 1200
#define N_BINS 64
#define LOCAL_REGION_DILATE 40

#define FG_MASK      1.f
#define BG_MASK      0.f
#define FG_SCRIBBLE  1.f
#define BG_SCRIBBLE  0.f

typedef maxflow::Graph<gfloat,gfloat,gfloat> GraphType;
typedef int node_id;

typedef struct
{
  gfloat  rgb[3];
  gint    x;
  gint    y;
} ColorsSample;

typedef struct
{
  GArray  *samples;
  gfloat   bins[N_BINS][N_BINS][N_BINS];
} ColorsModel;

typedef enum
{
  HARD_SOURCE,
  HARD_SINK,
  SOFT
} ConstraintType;

typedef struct
{
  GeglPaintSelectModeType  mode;
  gint         width;
  gint         height;
  gint         n_pixels;

  gfloat      *mask;        /* selection mask */
  gfloat      *colors;      /* input image    */
  gfloat      *scribbles;   /* user scribbles */
  gfloat      *h_costs;     /* horizontal edges costs */
  gfloat      *v_costs;     /* vertical edges costs   */
  gfloat       mean_costs;

  node_id     *nodes;       /* nodes map */
  GraphType   *graph;       /* maxflow::graph */
} PaintSelect;

typedef struct
{
  ColorsModel  *fg_colors;
  ColorsModel  *bg_colors;
} PaintSelectPrivate;


/* colors model */

static ColorsModel *
colors_model_new_global (gfloat  *pixels,
                         gfloat  *mask,
                         gint     width,
                         gint     height,
                         gfloat   mask_value)
{
  ColorsModel *model = g_slice_new0 (ColorsModel);
  GRand  *gr = g_rand_new_with_seed (0);
  gint    i = 0;

  model->samples = g_array_sized_new (FALSE, FALSE, sizeof (ColorsSample), N_GLOBAL_SAMPLES);

  while (i < N_GLOBAL_SAMPLES)
    {
      ColorsSample  sample;
      gint          m_offset;

      sample.x = g_rand_int_range (gr, 0, width);
      sample.y = g_rand_int_range (gr, 0, height);
      m_offset = sample.x + sample.y * width;

      if (mask[m_offset] == mask_value)
        {
          gint p_offset = m_offset * 3;
          gint b1, b2, b3;

          sample.rgb[0] = CLAMP (pixels[p_offset], 0.f, 1.f);
          sample.rgb[1] = CLAMP (pixels[p_offset+1], 0.f, 1.f);
          sample.rgb[2] = CLAMP (pixels[p_offset+2], 0.f, 1.f);

          b1 = (gint) (sample.rgb[0] * (N_BINS - 1));
          b2 = (gint) (sample.rgb[1] * (N_BINS - 1));
          b3 = (gint) (sample.rgb[2] * (N_BINS - 1));

          model->bins[b1][b2][b3]++;
          g_array_append_val (model->samples, sample);
          i++;
        }
    }

  g_rand_free (gr);
  return model;
}

static void
colors_model_update_global (ColorsModel  *model,
                            gfloat       *pixels,
                            gfloat       *mask,
                            gint          width,
                            gint          height,
                            gfloat        mask_value)
{
  GRand  *gr = g_rand_new_with_seed (0);
  guint   i;

  for (i = 0; i < model->samples->len; i++)
    {
      ColorsSample  *sample = &g_array_index (model->samples, ColorsSample, i);
      gint           m_offset = sample->x + sample->y * width;

      if (mask[m_offset] != mask_value)
        {
          gboolean changed = FALSE;
          gint b1, b2, b3;

          b1 = (gint) (sample->rgb[0] * (N_BINS - 1));
          b2 = (gint) (sample->rgb[1] * (N_BINS - 1));
          b3 = (gint) (sample->rgb[2] * (N_BINS - 1));

          model->bins[b1][b2][b3]--;

          while (! changed)
            {
              sample->x = g_rand_int_range (gr, 0, width);
              sample->y = g_rand_int_range (gr, 0, height);
              m_offset = sample->x + sample->y * width;

              if (mask[m_offset] == mask_value)
                {
                  gint p_offset = m_offset * 3;

                  sample->rgb[0] = pixels[p_offset];
                  sample->rgb[1] = pixels[p_offset+1];
                  sample->rgb[2] = pixels[p_offset+2];

                  b1 = (gint) (CLAMP(sample->rgb[0], 0.f, 1.f) * (N_BINS - 1));
                  b2 = (gint) (CLAMP(sample->rgb[1], 0.f, 1.f) * (N_BINS - 1));
                  b3 = (gint) (CLAMP(sample->rgb[2], 0.f, 1.f) * (N_BINS - 1));

                  model->bins[b1][b2][b3]++;
                  changed = TRUE;
                }
            }
        }
    }

  g_rand_free (gr);
}

static ColorsModel *
colors_model_new_local (gfloat         *pixels,
                        gfloat         *mask,
                        gfloat         *scribbles,
                        gint            width,
                        gint            height,
                        GeglRectangle  *region,
                        gfloat          mask_value,
                        gfloat          scribble_value)
{
  gint  x, y;
  ColorsModel *model = g_slice_new0 (ColorsModel);
  model->samples = g_array_sized_new (FALSE, FALSE, sizeof (ColorsSample), N_GLOBAL_SAMPLES);

  for (y = region->y; y < region->y + region->height; y++)
    {
      for (x = region->x; x < region->x + region->width; x++)
        {
          gint offset = x + y * width;

          if (scribbles[offset] == scribble_value ||
              mask[offset] == mask_value)
            {
              ColorsSample  sample;
              gint b1, b2, b3;
              gint p_offset = offset * 3;

              sample.x = x;
              sample.y = y;
              sample.rgb[0] = CLAMP (pixels[p_offset], 0.f, 1.f);
              sample.rgb[1] = CLAMP (pixels[p_offset+1], 0.f, 1.f);
              sample.rgb[2] = CLAMP (pixels[p_offset+2], 0.f, 1.f);

              b1 = (gint) (sample.rgb[0] * (N_BINS - 1));
              b2 = (gint) (sample.rgb[1] * (N_BINS - 1));
              b3 = (gint) (sample.rgb[2] * (N_BINS - 1));

              model->bins[b1][b2][b3]++;
              g_array_append_val (model->samples, sample);
            }
        }
    }

  return model;
}

static void
colors_model_free (ColorsModel  *model)
{
  if (model->samples)
    g_array_free (model->samples, TRUE);

  g_slice_free (ColorsModel, model);
}

static gfloat
colors_model_get_likelyhood (ColorsModel  *model,
                             gfloat       *color)
{
  gint b1 = (gint) (CLAMP(color[0], 0.f, 1.f) * (N_BINS - 1));
  gint b2 = (gint) (CLAMP(color[1], 0.f, 1.f) * (N_BINS - 1));
  gint b3 = (gint) (CLAMP(color[2], 0.f, 1.f) * (N_BINS - 1));

  return model->bins[b1][b2][b3] / (gfloat) model->samples->len;
}

static inline gfloat
pixels_distance (gfloat  *p1,
                 gfloat  *p2)
{
  return sqrtf (POW2(p1[0] - p2[0]) +
                POW2(p1[1] - p2[1]) +
                POW2(p1[2] - p2[2]));
}

/* fluctuations removal */

static void
push_segment (GQueue *segment_queue,
              gint    y,
              gint    old_y,
              gint    start,
              gint    end,
              gint    new_y,
              gint    new_start,
              gint    new_end)
{
  /* To avoid excessive memory allocation (y, old_y, start, end) tuples are
   * stored in interleaved format:
   *
   * [y1] [old_y1] [start1] [end1] [y2] [old_y2] [start2] [end2]
   */

  if (new_y != old_y)
    {
      /* If the new segment's y-coordinate is different than the old (source)
       * segment's y-coordinate, push the entire segment.
       */
      g_queue_push_tail (segment_queue, GINT_TO_POINTER (new_y));
      g_queue_push_tail (segment_queue, GINT_TO_POINTER (y));
      g_queue_push_tail (segment_queue, GINT_TO_POINTER (new_start));
      g_queue_push_tail (segment_queue, GINT_TO_POINTER (new_end));
    }
  else
    {
      /* Otherwise, only push the set-difference between the new segment and
       * the source segment (since we've already scanned the source segment.)
       */
      if (new_start < start)
        {
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (new_y));
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (y));
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (new_start));
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (start));
        }

      if (new_end > end)
        {
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (new_y));
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (y));
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (end));
          g_queue_push_tail (segment_queue, GINT_TO_POINTER (new_end));
        }
    }
}

static void
pop_segment (GQueue *segment_queue,
             gint   *y,
             gint   *old_y,
             gint   *start,
             gint   *end)
{
  *y     = GPOINTER_TO_INT (g_queue_pop_head (segment_queue));
  *old_y = GPOINTER_TO_INT (g_queue_pop_head (segment_queue));
  *start = GPOINTER_TO_INT (g_queue_pop_head (segment_queue));
  *end   = GPOINTER_TO_INT (g_queue_pop_head (segment_queue));
}

static gboolean
find_contiguous_segment (PaintSelect  *ps,
                         gfloat       *diff_mask,
                         gint          initial_x,
                         gint          initial_y,
                         gint         *start,
                         gint         *end)
{
  gint  offset = initial_x + initial_y * ps->width;

  /* check the starting pixel */
  if (diff_mask[offset] == 0.f)
    return FALSE;

  ps->mask[offset] = 1.f;

  *start = initial_x - 1;

  while (*start >= 0)
    {
      offset = *start + initial_y * ps->width;

      if (diff_mask[offset] == 0.f)
        break;

      ps->mask[offset] = 1.f;

      (*start)--;
    }

  *end = initial_x + 1;

  while (*end < ps->width)
    {
      offset = *end + initial_y * ps->width;

      if (diff_mask[offset] == 0.f)
        break;

      ps->mask[offset] = 1.f;

      (*end)++;
    }

  return TRUE;
}

static void
paint_select_remove_fluctuations (PaintSelect  *ps,
                                  gfloat       *diff_mask,
                                  gint          x,
                                  gint          y)
{
  gint                 old_y;
  gint                 start, end;
  gint                 new_start, new_end;
  GQueue              *segment_queue;

  /* mask buffer will hold the result and need to be clean first */
  memset (ps->mask, 0.f, sizeof (gfloat) * ps->n_pixels);

  segment_queue = g_queue_new ();

  push_segment (segment_queue,
                y, /* dummy values: */ -1, 0, 0,
                y, x - 1, x + 1);

  do
    {
      pop_segment (segment_queue,
                   &y, &old_y, &start, &end);

      for (x = start + 1; x < end; x++)
        {
          gfloat val = ps->mask[x + y * ps->width];

          if (val != 0.f)
            {
              /* If the current pixel is selected, then we've already visited
               * the next pixel.  (Note that we assume that the maximal image
               * width is sufficiently low that `x` won't overflow.)
               */
              x++;
              continue;
            }

          if (! find_contiguous_segment (ps, diff_mask,
                                         x, y,
                                         &new_start, &new_end))
            continue;

          /* We can skip directly to `new_end + 1` on the next iteration, since
           * we've just selected all pixels in the range `[x, new_end)`, and
           * the pixel at `new_end` is above threshold.  (Note that we assume
           * that the maximal image width is sufficiently low that `x` won't
           * overflow.)
           */
          x = new_end;

          if (y + 1 < ps->height)
            {
              push_segment (segment_queue,
                            y, old_y, start, end,
                            y + 1, new_start, new_end);
            }

          if (y - 1 >= 0)
            {
              push_segment (segment_queue,
                            y, old_y, start, end,
                            y - 1, new_start, new_end);
            }

        }
    }
  while (! g_queue_is_empty (segment_queue));

  g_queue_free (segment_queue);
}

static void
paint_select_init_buffers (PaintSelect  *ps,
                           GeglBuffer   *mask,
                           GeglBuffer   *colors,
                           GeglBuffer   *scribbles,
                           GeglPaintSelectModeType  mode)
{
  ps->mode     = mode;
  ps->width    = gegl_buffer_get_width (mask);
  ps->height   = gegl_buffer_get_height (mask);
  ps->n_pixels = ps->width * ps->height;

  ps->graph = new GraphType (ps->n_pixels,
                            (ps->width - 1) * ps->height + ps->width * (ps->height - 1));

  ps->mask      = (gfloat *)  gegl_malloc (sizeof (gfloat) * ps->n_pixels);
  ps->colors    = (gfloat *)  gegl_malloc (sizeof (gfloat) * ps->n_pixels * 3);
  ps->scribbles = (gfloat *)  gegl_malloc (sizeof (gfloat) * ps->n_pixels);
  ps->h_costs   = (gfloat *)  gegl_malloc (sizeof (gfloat) * (ps->width - 1) * ps->height);
  ps->v_costs   = (gfloat *)  gegl_malloc (sizeof (gfloat) * ps->width * (ps->height - 1));
  ps->nodes     = (node_id *) gegl_malloc (sizeof (node_id) * ps->n_pixels);

  gegl_buffer_get (mask, NULL, 1.0, babl_format (SELECTION_FORMAT),
                   ps->mask, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  gegl_buffer_get (colors, NULL, 1.0, babl_format (COLORS_FORMAT),
                   ps->colors, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  gegl_buffer_get (scribbles, NULL, 1.0, babl_format (SCRIBBLES_FORMAT),
                   ps->scribbles, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
}

static void
paint_select_free_buffers (PaintSelect  *ps)
{
  delete ps->graph;
  gegl_free (ps->nodes);
  gegl_free (ps->mask);
  gegl_free (ps->scribbles);
  gegl_free (ps->colors);
  gegl_free (ps->h_costs);
  gegl_free (ps->v_costs);
}

static void
paint_select_compute_adjacent_costs (PaintSelect  *ps)
{
  gint  x, y, n = 0;
  gfloat sum = 0.f;

  /* horizontal */

  for (y = 0; y < ps->height; y++)
    {
      for (x = 0; x < ps->width - 1; x++)
        {
          gint w_off  = x + y * (ps->width - 1);
          gint c1_off = (x + y * ps->width) * 3;
          gint c2_off = c1_off + 3;

          gfloat d = pixels_distance (ps->colors + c1_off, ps->colors + c2_off);
          ps->h_costs[w_off] = d;
          sum += d;
          n++;
        }
    }

  /* vertical */

  for (x = 0; x < ps->width; x++)
    {
      for (y = 0; y < ps->height - 1; y++)
        {
          gint w_off = x + y * ps->width;
          gint c1_off = (x + y * ps->width) * 3;
          gint c2_off = c1_off + ps->width * 3;

          gfloat d = pixels_distance (ps->colors + c1_off, ps->colors + c2_off);
          ps->v_costs[w_off] = d;
          sum += d;
          n++;
        }
    }

  /* compute mean costs */

  ps->mean_costs = sum / (gfloat) n;
}

static inline gboolean
paint_select_mask_is_interior_boundary (PaintSelect  *ps,
                                        gint          x,
                                        gint          y)
{
  gboolean is_boundary = FALSE;
  gint offset = x + y * ps->width;
  gint x2, y2, n;
  gint neighbors[4] = {0, };

  x2 = x - 1;
  if (x2 >= 0)
    {
      neighbors[0] = x2 + y * ps->width;
    }

  x2 = x + 1;
  if (x2 < ps->width)
    {
      neighbors[1] = x2 + y * ps->width;
    }

  y2 = y - 1;
  if (y2 >= 0)
    {
      neighbors[2] = x + y2 * ps->width;
    }

  y2 = y + 1;
  if (y2 < ps->height)
    {
      neighbors[3] = x + y2 * ps->width;
    }

  for (n = 0; n < 4; n++)
    {
      gint neighbor_offset = neighbors[n];

      if (neighbor_offset && ps->mask[neighbor_offset] != ps->mask[offset])
        {
          is_boundary = TRUE;
          break;
        }
    }

 return is_boundary;
}

static GeglRectangle
paint_select_get_local_region (PaintSelect  *ps,
                               gint         *pix_x,
                               gint         *pix_y)
{
  GeglRectangle extent = {0, 0, ps->width, ps->height};
  GeglRectangle region;
  gfloat scribble_val;
  gfloat mask_val;
  gint minx, miny;
  gint maxx, maxy;
  gint x, y;

  minx = ps->width;
  miny = ps->height;
  maxx = maxy = 0;

  if (ps->mode == GEGL_PAINT_SELECT_MODE_ADD)
    {
      scribble_val = FG_SCRIBBLE;
      mask_val     = BG_MASK;
    }
  else
    {
      scribble_val = BG_SCRIBBLE;
      mask_val     = FG_MASK;
    }

  for (y = 0; y < ps->height; y++)
    {
      for (x = 0; x < ps->width; x++)
        {
          gint off = x + y * ps->width;
          if (ps->scribbles[off] == scribble_val &&
              ps->mask[off] == mask_val)
            {
              /* keep track of one pixel position located in
               * local region ; it will be used later as a seed
               * point for fluctuations removal.
               */

              *pix_x = x;
              *pix_y = y;

              if (x < minx)
                minx = x;
              else if (x > maxx)
                maxx = x;

              if (y < miny)
                miny = y;
              else if (y > maxy)
                maxy = y;
            }
        }
    }

  region.x = minx;
  region.y = miny;
  region.width = maxx - minx + 1;
  region.height = maxy - miny + 1;

  if (gegl_rectangle_is_empty (&region) ||
      gegl_rectangle_is_infinite_plane (&region))
    return region;

  region.x -= LOCAL_REGION_DILATE;
  region.y -= LOCAL_REGION_DILATE;
  region.width += 2 * LOCAL_REGION_DILATE;
  region.height += 2 * LOCAL_REGION_DILATE;

  gegl_rectangle_intersect (&region, &region, &extent);
  return region;
}

/* SOURCE terminal is foreground (selected pixels)
 * SINK terminal is background (unselected pixels)
 */

static void
paint_select_init_graph_nodes_and_tlinks (PaintSelect  *ps,
                                          ColorsModel  *local_colors,
                                          ColorsModel  *global_colors)
{
  gfloat          node_mask;
  ConstraintType  boundary;
  ColorsModel    *source_model;
  ColorsModel    *sink_model;
  gint            x, y;

  if (ps->mode == GEGL_PAINT_SELECT_MODE_ADD)
    {
      node_mask    = BG_MASK;
      boundary     = HARD_SOURCE;
      source_model = global_colors;
      sink_model   = local_colors;
    }
  else
    {
      node_mask  = FG_MASK;
      boundary   = HARD_SINK;
      source_model = local_colors;
      sink_model   = global_colors;
    }

  for (y = 0; y < ps->height; y++)
    {
      for (x = 0; x < ps->width; x++)
        {
          gboolean is_boundary = FALSE;
          node_id id = -1;
          gint off = x + y * ps->width;

          /* determine if a node is needed for this pixel */

          if (ps->mask[off] == node_mask)
            {
              id = ps->graph->add_node();
            }
          else
            {
              is_boundary = paint_select_mask_is_interior_boundary (ps, x, y);
              if (is_boundary)
                id = ps->graph->add_node();
            }

          ps->nodes[off] = id;

          /* determine the constraint type and set weights accordingly */

          if (id != -1)
            {
              gfloat source_weight;
              gfloat sink_weight;

              if (is_boundary)
                {
                  if (boundary == HARD_SOURCE)
                    {
                      source_weight = BIG_CAPACITY;
                      sink_weight   = 0.f;
                    }
                  else
                    {
                      source_weight = 0.f;
                      sink_weight   = BIG_CAPACITY;
                    }
                }
              else if (ps->scribbles[off] == FG_SCRIBBLE)
                {
                  source_weight = BIG_CAPACITY;
                  sink_weight   = 0.f;
                }
              else if (ps->scribbles[off] == BG_SCRIBBLE)
                {
                  source_weight = 0.f;
                  sink_weight   = BIG_CAPACITY;
                }
              else
                {
                  gint coff = off * 3;

                  sink_weight   = - logf (colors_model_get_likelyhood (sink_model, ps->colors + coff) + 0.0001f);
                  source_weight = - logf (colors_model_get_likelyhood (source_model, ps->colors + coff) + 0.0001f);
                }

              if (source_weight < 0)
                source_weight = 0.f;

              if (sink_weight < 0)
                sink_weight = 0.f;

              ps->graph->add_tweights (id, source_weight, sink_weight);
            }
        }
    }
}

static void
paint_select_init_graph_nlinks (PaintSelect  *ps)
{
  gint x, y;

  /* horizontal */

  for (y = 0; y < ps->height; y++)
    {
      for (x = 0; x < ps->width - 1; x++)
        {
          node_id id1 = ps->nodes[x + y * ps->width];
          node_id id2 = ps->nodes[x + 1 + y * ps->width];

          if (id1 != -1 && id2 != -1)
            {
              gint costs_off = x + y * (ps->width - 1);
              gfloat weight  = 60.f * ps->mean_costs / (ps->h_costs[costs_off] + EPSILON);
              g_assert (weight >= 0.f);
              ps->graph->add_edge (id1, id2, weight, weight);
            }
        }
    }

  /* vertical */

  for (x = 0; x < ps->width; x++)
    {
      for (y = 0; y < ps->height - 1; y++)
        {
          node_id id1 = ps->nodes[x + y * ps->width];
          node_id id2 = ps->nodes[x + (y+1) * ps->width];

          if (id1 != -1 && id2 != -1)
            {
              gint costs_off = x + y * ps->width;
              gfloat weight  = 60.f * ps->mean_costs / (ps->v_costs[costs_off] + EPSILON);
              g_assert (weight >= 0.f);
              ps->graph->add_edge (id1, id2, weight, weight);
            }
        }
    }
}

static gfloat *
paint_select_compute_diff_mask (PaintSelect  *ps)
{
  gfloat  *diff = (gfloat *) gegl_malloc (sizeof (gfloat) * ps->n_pixels);
  gfloat   node_value;
  gint     i;

  for (i = 0; i < ps->width * ps->height; i++)
    {
      node_id id = ps->nodes[i];

      if (id != -1)
        {
          if (ps->graph->what_segment(id) == GraphType::SOURCE)
            node_value = 1.f;
          else
            node_value = 0.f;


          if (ps->mask[i] != node_value)
            diff[i] = 1.f;
          else
            diff[i] = 0.f;
        }
    }

  return diff;
}

/* GEGL operation */

static void
prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "aux");
  const Babl *selection  = babl_format (SELECTION_FORMAT);
  const Babl *scribbles  = babl_format (SCRIBBLES_FORMAT);
  const Babl *colors     = babl_format_with_space (COLORS_FORMAT, space);

  gegl_operation_set_format (operation, "input",  selection);
  gegl_operation_set_format (operation, "aux",    colors);
  gegl_operation_set_format (operation, "aux2",   scribbles);
  gegl_operation_set_format (operation, "output", selection);
}

static GeglRectangle
get_cached_region (GeglOperation        *operation,
                   const GeglRectangle  *roi)
{
  GeglRectangle  empty_r  = {0, };
  GeglRectangle *in_r = gegl_operation_source_get_bounding_box (operation,
                                                                "input");
  if (in_r)
    return *in_r;

  return empty_r;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *aux,
         GeglBuffer          *aux2,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties  *o = GEGL_PROPERTIES (operation);
  PaintSelect   ps;
  gfloat        flow;
  GeglRectangle region;
  gint          x, y;

  if (! aux || ! aux2)
    {
      gegl_buffer_copy (input, NULL, GEGL_ABYSS_NONE, output, NULL);
      return TRUE;
    }

  /* memory allocations, pixels fetch */

  paint_select_init_buffers (&ps, input, aux, aux2, o->mode);

  /* find the overlap region where scribble value doesn't match mask value */

  region = paint_select_get_local_region (&ps, &x, &y);

  g_printerr ("local region: (%d,%d) %d x %d\n",
              region.x, region.y, region.width, region.height);

  if (! gegl_rectangle_is_empty (&region) &&
      ! gegl_rectangle_is_infinite_plane (&region))
    {
      PaintSelectPrivate *priv = (PaintSelectPrivate *) o->user_data;
      ColorsModel        *local_colors;
      ColorsModel        *global_colors;
      gfloat             *diff;

      if (! priv)
        {
          priv = g_slice_new (PaintSelectPrivate);
          priv->fg_colors = NULL;
          priv->bg_colors = NULL;
          o->user_data    = priv;
        }

      if (o->mode == GEGL_PAINT_SELECT_MODE_ADD)
        {
          if (! priv->bg_colors)
            {
              priv->bg_colors = colors_model_new_global (ps.colors, ps.mask,
                                                         ps.width, ps.height,
                                                         BG_MASK);
            }
          else
            {
              colors_model_update_global (priv->bg_colors,
                                          ps.colors, ps.mask,
                                          ps.width, ps.height,
                                          BG_MASK);
            }

          global_colors = priv->bg_colors;
          local_colors = colors_model_new_local (ps.colors, ps.mask, ps.scribbles,
                                                 ps.width, ps.height,
                                                 &region,
                                                 FG_MASK, FG_SCRIBBLE);
        }
      else
        {
          if (! priv->fg_colors)
            {
              priv->fg_colors = colors_model_new_global (ps.colors, ps.mask,
                                                         ps.width, ps.height,
                                                         FG_MASK);
            }
          else
            {
              colors_model_update_global (priv->fg_colors,
                                          ps.colors, ps.mask,
                                          ps.width, ps.height,
                                          FG_MASK);
            }

          global_colors = priv->fg_colors;
          local_colors = colors_model_new_local (ps.colors, ps.mask, ps.scribbles,
                                                 ps.width, ps.height,
                                                 &region,
                                                 BG_MASK, BG_SCRIBBLE);
        }

      /* init graph */

      paint_select_compute_adjacent_costs (&ps);
      paint_select_init_graph_nodes_and_tlinks (&ps, local_colors, global_colors);
      paint_select_init_graph_nlinks (&ps);

      /* compute maxflow/mincut */

      flow = ps.graph->maxflow();

      /* compute difference between original mask and graphcut result.
       * then remove fluctuations */

      diff = paint_select_compute_diff_mask (&ps);

      paint_select_remove_fluctuations (&ps, diff, x, y);

      gegl_buffer_set (output, NULL, 0, babl_format (SELECTION_FORMAT),
                       ps.mask, GEGL_AUTO_ROWSTRIDE);

      gegl_free (diff);
      colors_model_free (local_colors);
    }

  paint_select_free_buffers (&ps);

  return TRUE;
}

static void
finalize (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);

  if (o->user_data)
    {
      PaintSelectPrivate *priv = (PaintSelectPrivate *) o->user_data;

      if (priv->fg_colors)
        colors_model_free (priv->fg_colors);

      if (priv->bg_colors)
        colors_model_free (priv->bg_colors);

      g_slice_free (PaintSelectPrivate, o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass                *object_class;
  GeglOperationClass          *operation_class;
  GeglOperationComposer3Class *composer_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  composer_class  = GEGL_OPERATION_COMPOSER3_CLASS (klass);

  object_class->finalize                     = finalize;
  operation_class->get_cached_region         = get_cached_region;
  operation_class->prepare                   = prepare;
  operation_class->threaded                  = FALSE;
  composer_class->process                    = process;

  gegl_operation_class_set_keys (operation_class,
  "name",         "gegl:paint-select",
  "title",        _("Paint Select"),
  NULL);
}

#endif
