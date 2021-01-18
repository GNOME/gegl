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

property_boolean (use_local_region, _("Use local region"), FALSE)
    description (_("Perform graphcut in a local region"))

property_int (region_x, _("region-x"), 0)
    value_range (0, G_MAXINT)

property_int (region_y, _("region-y"), 0)
    value_range (0, G_MAXINT)

property_int (region_width, _("region-width"), 0)
    value_range (0, G_MAXINT)

property_int (region_height, _("region-height"), 0)
    value_range (0, G_MAXINT)

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
  SEED_NONE,
  SEED_SOURCE,
  SEED_SINK,
} SeedType;

typedef struct
{
  GeglPaintSelectModeType  mode;
  GeglRectangle            roi;
  GeglRectangle            extent;

  gfloat      *mask;        /* selection mask */
  gfloat      *colors;      /* input image    */
  gfloat      *scribbles;   /* user scribbles */
} PaintSelect;

typedef struct
{
  ColorsModel  *fg_colors;
  ColorsModel  *bg_colors;
} PaintSelectPrivate;

typedef struct
{
  gfloat        mask_value_seed; /* Value of the mask where seeds are needed. */
  SeedType      mask_seed_type;  /* Corresponding seed type.                  */
  ColorsModel  *source_model;    /* Colors model used to compute terminal     */
  ColorsModel  *sink_model;      /*  links weights.                           */
  ColorsModel  *local_colors;

  gboolean      boundary_top;    /* Seed type added to the local region       */
  gboolean      boundary_left;   /*  boundaries to avoid the selection to     */
  gboolean      boundary_right;  /*  align with them.                         */
  gboolean      boundary_bottom;
  SeedType      boundary_seed_type;
} PaintSelectContext;


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
find_contiguous_segment (gfloat  *mask,
                         gfloat  *diff,
                         gint     width,
                         gint     initial_x,
                         gint     initial_y,
                         gint    *start,
                         gint    *end)
{
  gint  offset = initial_x + initial_y * width;

  /* check the starting pixel */
  if (diff[offset] == 0.f)
    return FALSE;

  mask[offset] = 1.f;

  *start = initial_x - 1;

  while (*start >= 0)
    {
      offset = *start + initial_y * width;

      if (diff[offset] == 0.f)
        break;

      mask[offset] = 1.f;

      (*start)--;
    }

  *end = initial_x + 1;

  while (*end < width)
    {
      offset = *end + initial_y * width;

      if (diff[offset] == 0.f)
        break;

      mask[offset] = 1.f;

      (*end)++;
    }

  return TRUE;
}

static void
paint_select_remove_fluctuations (gfloat  *mask,
                                  gfloat  *diff,
                                  gint     width,
                                  gint     height,
                                  gint     x,
                                  gint     y)
{
  gint                 old_y;
  gint                 start, end;
  gint                 new_start, new_end;
  GQueue              *segment_queue;

  /* mask buffer will hold the result and need to be clean first */
  memset (mask, 0.f, sizeof (gfloat) * (width * height));

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
          gfloat val = mask[x + y * width];

          if (val != 0.f)
            {
              /* If the current pixel is selected, then we've already visited
               * the next pixel.  (Note that we assume that the maximal image
               * width is sufficiently low that `x` won't overflow.)
               */
              x++;
              continue;
            }

          if (! find_contiguous_segment (mask, diff, width,
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

          if (y + 1 < height)
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

/* graphcut
 *
 * SOURCE terminal always represents selected region */

static inline gfloat
pixels_distance (gfloat  *p1,
                 gfloat  *p2)
{
  return sqrtf (POW2(p1[0] - p2[0]) +
                POW2(p1[1] - p2[1]) +
                POW2(p1[2] - p2[2]));
}

static void
paint_select_compute_adjacent_costs (gfloat  *pixels,
                                     gint     width,
                                     gint     height,
                                     gfloat  *h_costs,
                                     gfloat  *v_costs,
                                     gfloat  *mean)
{
  gint  x, y;
  gint  n_h_costs = (width - 1) * height;
  gint  n_v_costs = width * (height - 1);
  gfloat sum      = 0.f;

  /* horizontal */

  for (y = 0; y < height; y++)
    {
      for (x = 0; x < width - 1; x++)
        {
          gint cost_offset = x + y * (width - 1);
          gint p1_offset   = (x + y * width) * 3;
          gint p2_offset   = p1_offset + 3;

          gfloat d = pixels_distance (pixels + p1_offset, pixels + p2_offset);
          h_costs[cost_offset] = d;
          sum += d;
        }
    }

  /* vertical */

  for (x = 0; x < width; x++)
    {
      for (y = 0; y < height - 1; y++)
        {
          gint cost_offset = x + y * width;
          gint p1_offset    = (x + y * width) * 3;
          gint p2_offset    = p1_offset + width * 3;

          gfloat d = pixels_distance (pixels + p1_offset, pixels + p2_offset);
          v_costs[cost_offset] = d;
          sum += d;
        }
    }

  /* compute mean costs */

  *mean = sum / (gfloat) (n_h_costs + n_v_costs);
}

static guint8 *
paint_select_compute_seeds_map (gfloat              *mask,
                                gfloat              *scribbles,
                                gint                 width,
                                gint                 height,
                                PaintSelectContext  *context)
{
  guint8  *seeds;
  gint     x, y;

  seeds = g_new (guint8, width * height);

  for (y = 0; y < height; y++)
    {
      for (x = 0; x < width; x++)
        {
          gint offset = x + y * width;

          gfloat *p_mask      = mask + offset;
          gfloat *p_scribbles = scribbles + offset;
          guint8 *p_seeds     = seeds + offset;

          *p_seeds = SEED_NONE;

          if (*p_mask == context->mask_value_seed)
            {
              *p_seeds = context->mask_seed_type;
            }
          else if (*p_scribbles == FG_SCRIBBLE)
            {
              *p_seeds = SEED_SOURCE;
            }

          else if (*p_scribbles == BG_SCRIBBLE)
            {
              *p_seeds = SEED_SINK;
            }
        }
    }

  /* put boundary seeds if needed */

  if (context->boundary_top)
    {
      y = 0;
      for (x = 0; x < width; x++)
        {
          gint offset = x + y * width;
          if (seeds[offset] == SEED_NONE)
            seeds[offset] = context->boundary_seed_type;
        }
    }

  if (context->boundary_left)
    {
      x = 0;
      for (y = 0; y < height; y++)
        {
          gint offset = x + y * width;
          if (seeds[offset] == SEED_NONE)
            seeds[offset] = context->boundary_seed_type;
        }
    }

  if (context->boundary_right)
    {
      x = width - 1;
      for (y = 0; y < height; y++)
        {
          gint offset = x + y * width;
          if (seeds[offset] == SEED_NONE)
            seeds[offset] = context->boundary_seed_type;
        }
    }

  if (context->boundary_bottom)
    {
      y = height - 1;
      for (x = 0; x < width; x++)
        {
          gint offset = x + y * width;
          if (seeds[offset] == SEED_NONE)
            seeds[offset] = context->boundary_seed_type;
        }
    }

  return seeds;
}

static inline gboolean
paint_select_seed_is_boundary (guint8  *seeds,
                               gint     width,
                               gint     height,
                               gint     x,
                               gint     y)
{
  gint offset = x + y * width;
  gint neighbor_offset;
  gint x2, y2;

  x2 = x - 1;
  if (x2 >= 0)
    {
      neighbor_offset = offset - 1;
      if (seeds[neighbor_offset] != seeds[offset])
        return TRUE;
    }

  x2 = x + 1;
  if (x2 < width)
    {
      neighbor_offset = offset + 1;
      if (seeds[neighbor_offset] != seeds[offset])
        return TRUE;
    }

  y2 = y - 1;
  if (y2 >= 0)
    {
      neighbor_offset = offset - width;
      if (seeds[neighbor_offset] != seeds[offset])
        return TRUE;
    }

  y2 = y + 1;
  if (y2 < height)
    {
      neighbor_offset = offset + width;
      if (seeds[neighbor_offset] != seeds[offset])
        return TRUE;
    }

  return FALSE;
}

static void
paint_select_graph_init_nodes_and_tlinks (GraphType           *graph,
                                          gfloat              *pixels,
                                          guint8              *seeds,
                                          gint                *nodes,
                                          gint                 width,
                                          gint                 height,
                                          PaintSelectContext  *context)
{
  gint  x, y;

  for (y = 0; y < height; y++)
    {
      for (x = 0; x < width; x++)
        {
          node_id  id      = -1;
          gint     offset  = x + y * width;
          guint8  *p_seeds = seeds + offset;
          gint    *p_nodes = nodes + offset;

          if (*p_seeds == SEED_NONE)
            {
              gfloat  source_weight;
              gfloat  sink_weight;
              gfloat *color = pixels + offset * 3;

              id = graph->add_node();

              sink_weight   = - logf (colors_model_get_likelyhood (context->sink_model, color) + 0.0001f);
              source_weight = - logf (colors_model_get_likelyhood (context->source_model, color) + 0.0001f);

              graph->add_tweights (id, source_weight, sink_weight);
            }
          else if (paint_select_seed_is_boundary (seeds, width, height, x, y))
            {
              id = graph->add_node();

              if (*p_seeds == SEED_SOURCE)
                {
                  graph->add_tweights (id, BIG_CAPACITY, 0.f);
                }
              else
                {
                  graph->add_tweights (id, 0.f, BIG_CAPACITY);
                }
            }

          *p_nodes = id;
        }
    }
}

static void
paint_select_graph_init_nlinks (GraphType  *graph,
                                gint       *nodes,
                                gfloat     *h_costs,
                                gfloat     *v_costs,
                                gfloat      mean_costs,
                                gint        width,
                                gint        height)
{
  gint x, y;

  /* horizontal */

  for (y = 0; y < height; y++)
    {
      for (x = 0; x < width - 1; x++)
        {
          node_id id1 = nodes[x + y * width];
          node_id id2 = nodes[x + 1 + y * width];

          if (id1 != -1 && id2 != -1)
            {
              gint costs_off = x + y * (width - 1);
              gfloat weight  = 60.f * mean_costs / (h_costs[costs_off] + EPSILON);
              g_assert (weight >= 0.f);
              graph->add_edge (id1, id2, weight, weight);
            }
        }
    }

  /* vertical */

  for (x = 0; x < width; x++)
    {
      for (y = 0; y < height - 1; y++)
        {
          node_id id1 = nodes[x + y * width];
          node_id id2 = nodes[x + (y+1) * width];

          if (id1 != -1 && id2 != -1)
            {
              gint costs_off = x + y * width;
              gfloat weight  = 60.f * mean_costs / (v_costs[costs_off] + EPSILON);
              g_assert (weight >= 0.f);
              graph->add_edge (id1, id2, weight, weight);
            }
        }
    }
}

static gfloat *
paint_select_graph_get_segmentation (GraphType  *graph,
                                     gint       *nodes,
                                     guint8     *seeds,
                                     gint        width,
                                     gint        height)
{
  gint     size = width * height;
  gfloat  *segmentation;
  gint     i;

  segmentation = g_new (gfloat, size);

  for (i = 0; i < size; i++)
    {
      node_id id = nodes[i];

      if (id != -1)
        {
          if (graph->what_segment(id) == GraphType::SOURCE)
            segmentation[i] = 1.f;
          else
            segmentation[i] = 0.f;
        }
      else if (seeds[i] == SEED_SOURCE)
        {
          segmentation[i] = 1.f;
        }
      else if (seeds[i] == SEED_SINK)
        {
          segmentation[i] = 0.f;
        }
    }

  return segmentation;
}

static gfloat *
paint_select_graphcut (gfloat              *pixels,
                       guint8              *seeds,
                       gint                 width,
                       gint                 height,
                       PaintSelectContext  *context)
{
  GraphType  *graph;
  gint       *nodes;
  gfloat     *result;
  gfloat     *h_costs;
  gfloat     *v_costs;
  gfloat      mean_costs;
  gint        n_nodes;
  gint        n_edges;
  gint        n_h_costs;
  gint        n_v_costs;

  n_h_costs = (width - 1) * height;
  n_v_costs = (height - 1) * width;

  n_nodes = width * height;
  n_edges = n_h_costs + n_v_costs;

  h_costs = g_new (gfloat, n_h_costs);
  v_costs = g_new (gfloat, n_v_costs);

  paint_select_compute_adjacent_costs (pixels, width, height,
                                       h_costs, v_costs, &mean_costs);

  graph = new GraphType (n_nodes, n_edges);
  nodes = g_new (gint, n_nodes);

  paint_select_graph_init_nodes_and_tlinks (graph, pixels, seeds, nodes,
                                            width, height, context);

  paint_select_graph_init_nlinks (graph, nodes, h_costs, v_costs, mean_costs,
                                  width, height);

  graph->maxflow();

  g_free (h_costs);
  g_free (v_costs);

  result = paint_select_graph_get_segmentation (graph, nodes, seeds, width, height);

  g_free (nodes);
  delete graph;

  return result;
}

/* high level functions */

static PaintSelectContext *
paint_select_context_new (GeglProperties      *o,
                          gfloat              *pixels,
                          gfloat              *mask,
                          gfloat              *scribbles,
                          GeglRectangle       *roi,
                          GeglRectangle       *extent,
                          GeglRectangle       *region)
{
  PaintSelectPrivate *priv = (PaintSelectPrivate *) o->user_data;
  PaintSelectContext *context = g_slice_new (PaintSelectContext);

  if (! priv)
    {
      priv = g_slice_new (PaintSelectPrivate);
      priv->fg_colors = NULL;
      priv->bg_colors = NULL;
      o->user_data    = priv;
    }

  if (o->use_local_region)
    {
      context->boundary_top    = (roi->y > 0);
      context->boundary_left   = (roi->x > 0);
      context->boundary_right  = (roi->x + roi->width != extent->width);
      context->boundary_bottom = (roi->y + roi->height != extent->height);
    }

  if (o->mode == GEGL_PAINT_SELECT_MODE_ADD)
    {
      if (! priv->bg_colors)
        {
          priv->bg_colors = colors_model_new_global (pixels, mask,
                                                     roi->width,
                                                     roi->height,
                                                     BG_MASK);
        }
      else
        {
          colors_model_update_global (priv->bg_colors, pixels, mask,
                                      roi->width, roi->height, BG_MASK);
        }

      context->local_colors = colors_model_new_local (pixels, mask, scribbles,
                                                      roi->width, roi->height, region,
                                                      FG_MASK, FG_SCRIBBLE);
      context->mask_value_seed = FG_MASK;
      context->mask_seed_type  = SEED_SOURCE;
      context->source_model    = priv->bg_colors;
      context->sink_model      = context->local_colors;
      context->boundary_seed_type = SEED_SINK;
    }
  else
    {
      if (! priv->fg_colors)
        {
          priv->fg_colors = colors_model_new_global (pixels, mask,
                                                     roi->width, roi->height, FG_MASK);
        }
      else
        {
          colors_model_update_global (priv->fg_colors, pixels, mask,
                                      roi->width, roi->height, FG_MASK);
        }

      context->local_colors = colors_model_new_local (pixels, mask, scribbles,
                                                      roi->width, roi->height, region,
                                                      BG_MASK, BG_SCRIBBLE);
      context->mask_value_seed = BG_MASK;
      context->mask_seed_type  = SEED_SINK;
      context->source_model    = context->local_colors;
      context->sink_model      = priv->fg_colors;
      context->boundary_seed_type = SEED_SOURCE;
    }

  return context;
}

static void
paint_select_context_free (PaintSelectContext  *context)
{
  colors_model_free (context->local_colors);
  g_slice_free (PaintSelectContext, context);
}

static void
paint_select_init_buffers (PaintSelect  *ps,
                           GeglBuffer   *mask,
                           GeglBuffer   *colors,
                           GeglBuffer   *scribbles,
                           GeglProperties  *o)
{
  gint n_pixels;

  ps->extent = *gegl_buffer_get_extent (mask);

  if (o->use_local_region)
    {
      ps->roi.x      = o->region_x;
      ps->roi.y      = o->region_y;
      ps->roi.width  = o->region_width;
      ps->roi.height = o->region_height;
    }
  else
    {
      gegl_rectangle_copy (&ps->roi, &ps->extent);
    }

  ps->mode   = o->mode;
  n_pixels   = ps->roi.width * ps->roi.height;

  ps->mask      = (gfloat *)  gegl_malloc (sizeof (gfloat) * n_pixels);
  ps->colors    = (gfloat *)  gegl_malloc (sizeof (gfloat) * n_pixels * 3);
  ps->scribbles = (gfloat *)  gegl_malloc (sizeof (gfloat) * n_pixels);

  gegl_buffer_get (mask, &ps->roi, 1.0, babl_format (SELECTION_FORMAT),
                   ps->mask, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  gegl_buffer_get (colors, &ps->roi, 1.0, babl_format (COLORS_FORMAT),
                   ps->colors, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  gegl_buffer_get (scribbles, &ps->roi, 1.0, babl_format (SCRIBBLES_FORMAT),
                   ps->scribbles, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
}

static void
paint_select_free_buffers (PaintSelect  *ps)
{
  gegl_free (ps->mask);
  gegl_free (ps->scribbles);
  gegl_free (ps->colors);
}

static gboolean
paint_select_get_scribble_region (gfloat                  *mask,
                                  gfloat                  *scribbles,
                                  gint                     width,
                                  gint                     height,
                                  GeglRectangle           *region,
                                  gint                    *pix_x,
                                  gint                    *pix_y,
                                  GeglPaintSelectModeType  mode)
{
  GeglRectangle extent = {0, 0, width, height};
  gfloat scribble_val;
  gfloat mask_val;
  gint minx, miny;
  gint maxx, maxy;
  gint x, y;

  minx = width;
  miny = height;
  maxx = maxy = 0;

  if (mode == GEGL_PAINT_SELECT_MODE_ADD)
    {
      scribble_val = FG_SCRIBBLE;
      mask_val     = BG_MASK;
    }
  else
    {
      scribble_val = BG_SCRIBBLE;
      mask_val     = FG_MASK;
    }

  for (y = 0; y < height; y++)
    {
      for (x = 0; x < width; x++)
        {
          gint off = x + y * width;
          if (scribbles[off] == scribble_val &&
              mask[off] == mask_val)
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

  region->x = minx;
  region->y = miny;
  region->width = maxx - minx + 1;
  region->height = maxy - miny + 1;

  if (gegl_rectangle_is_empty (region) ||
      gegl_rectangle_is_infinite_plane (region))
    return FALSE;

  region->x -= LOCAL_REGION_DILATE;
  region->y -= LOCAL_REGION_DILATE;
  region->width += 2 * LOCAL_REGION_DILATE;
  region->height += 2 * LOCAL_REGION_DILATE;

  gegl_rectangle_intersect (region, region, &extent);
  return TRUE;
}

static void
paint_select_compute_diff_mask (gfloat  *mask,
                                gfloat  *result,
                                gint     width,
                                gint     height)
{
  gint  i;

  for (i = 0; i < width * height; i++)
    {
      result[i] = result[i] != mask[i] ? 1.f : 0.f;
    }
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
get_bounding_box (GeglOperation  *operation)
{
  GeglProperties  *o = GEGL_PROPERTIES (operation);
  GeglRectangle    result = {0, 0, 0, 0};
  GeglRectangle   *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (operation, "input");

  if (o->use_local_region)
    {
      result.x = o->region_x;
      result.y = o->region_y;
      result.width  = o->region_width;
      result.height = o->region_height;
    }
  else if (in_rect)
    {
      result = *in_rect;
    }

  return result;
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
  GeglRectangle region;
  gint          x = 0;
  gint          y = 0;

  if (! aux || ! aux2)
    {
      gegl_buffer_copy (input, NULL, GEGL_ABYSS_NONE, output, NULL);
      return TRUE;
    }

  /* memory allocations, pixels fetch */

  paint_select_init_buffers (&ps, input, aux, aux2, o);

  /* find the overlap region where scribble value doesn't match mask value */

  if (paint_select_get_scribble_region (ps.mask, ps.scribbles,
                                        ps.roi.width, ps.roi.height,
                                        &region, &x, &y, ps.mode))
    {
      PaintSelectContext *context;
      guint8             *seeds;
      gfloat             *result;

      g_printerr ("scribble region: (%d,%d) %d x %d\n",
                  region.x, region.y, region.width, region.height);

      context = paint_select_context_new (o,
                                          ps.colors,
                                          ps.mask,
                                          ps.scribbles,
                                          &ps.roi,
                                          &ps.extent,
                                          &region);

      seeds = paint_select_compute_seeds_map (ps.mask,
                                              ps.scribbles,
                                              ps.roi.width,
                                              ps.roi.height,
                                              context);

      result = paint_select_graphcut (ps.colors, seeds,
                                      ps.roi.width, ps.roi.height,
                                      context);

      /* compute difference between original mask and graphcut result.
       * then remove fluctuations */

      paint_select_compute_diff_mask (ps.mask, result, ps.roi.width, ps.roi.height);
      paint_select_remove_fluctuations (ps.mask, result, ps.roi.width, ps.roi.height, x, y);

      if (o->use_local_region)
        {
          gegl_buffer_set (output, &ps.roi, 0, babl_format (SELECTION_FORMAT),
                       ps.mask, GEGL_AUTO_ROWSTRIDE);
        }
      else
        {
          gegl_buffer_set (output, NULL, 0, babl_format (SELECTION_FORMAT),
                       ps.mask, GEGL_AUTO_ROWSTRIDE);
        }

      g_free (seeds);
      g_free (result);
      paint_select_context_free (context);
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
  operation_class->get_bounding_box          = get_bounding_box;
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
