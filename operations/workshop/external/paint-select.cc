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
  gfloat bins[N_BINS][N_BINS][N_BINS];
} Histogram;

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

  gfloat      *fg_samples;
  gint         n_fg_samples;
  Histogram    fg_hist;

  gfloat      *bg_samples;
  gint         n_bg_samples;
  Histogram    bg_hist;
} PaintSelect;


static inline gfloat
pixels_distance (gfloat  *p1,
                 gfloat  *p2)
{
  return sqrtf (POW2(p1[0] - p2[0]) +
                POW2(p1[1] - p2[1]) +
                POW2(p1[2] - p2[2]));
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

  ps->fg_samples = NULL;
  ps->bg_samples = NULL;

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
  g_free (ps->fg_samples);
  g_free (ps->bg_samples);
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
paint_select_get_local_region (PaintSelect  *ps)
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

static void
paint_select_init_local_samples (PaintSelect    *ps,
                                 GeglRectangle  *region)
{
  gint    *n_samples;
  gfloat **samples;
  gfloat   scribble_val;
  gfloat   mask_val;
  gint     x, y, i;

  if (ps->mode == GEGL_PAINT_SELECT_MODE_ADD)
    {
      n_samples    = &ps->n_fg_samples;
      samples      = &ps->fg_samples;
      scribble_val = FG_SCRIBBLE;
      mask_val     = FG_MASK;
      g_printerr ("local samples are foreground\n");
    }
  else
    {
      n_samples    = &ps->n_bg_samples;
      samples      = &ps->bg_samples;
      scribble_val = BG_SCRIBBLE;
      mask_val     = BG_MASK;
      g_printerr ("local samples are background\n");
    }

  *n_samples = 0;

  for (y = region->y; y < region->y + region->height; y++)
    {
      for (x = region->x; x < region->x + region->width; x++)
        {
          gint off = x + y * ps->width;

          if (ps->scribbles[off] == scribble_val ||
              ps->mask[off] == mask_val)
            {
              *n_samples += 1;
            }
        }
    }

  *samples = g_new (gfloat, *n_samples * 3);
  i = 0;

  for (y = region->y; y < region->y + region->height; y++)
    {
      for (x = region->x; x < region->x + region->width; x++)
        {
          gint off = x + y * ps->width;

          if (ps->scribbles[off] == scribble_val ||
              ps->mask[off] == mask_val)
            {
              gint coff = off * 3;
              (*samples)[i*3]   = ps->colors[coff];
              (*samples)[i*3+1] = ps->colors[coff+1];
              (*samples)[i*3+2] = ps->colors[coff+2];
              i++;
            }
        }
    }
}

static void
paint_select_init_global_samples (PaintSelect  *ps)
{
  gint    *n_samples;
  gfloat **samples;
  gfloat   mask_val;
  GRand   *gr;
  gint i = 0;

  gr = g_rand_new_with_seed (0);

  if (ps->mode == GEGL_PAINT_SELECT_MODE_SUBTRACT)
    {
      n_samples = &ps->n_fg_samples;
      samples   = &ps->fg_samples;
      mask_val  = FG_MASK;
      g_printerr ("global samples are foreground\n");
    }
  else
    {
      n_samples = &ps->n_bg_samples;
      samples   = &ps->bg_samples;
      mask_val  = BG_MASK;
      g_printerr ("global samples are background\n");
    }

  *n_samples = N_GLOBAL_SAMPLES;
  *samples = g_new (gfloat, N_GLOBAL_SAMPLES * 3);

  while (i < N_GLOBAL_SAMPLES)
    {
      gint x = g_rand_int_range (gr, 0, ps->width);
      gint y = g_rand_int_range (gr, 0, ps->height);
      gint off = x + y * ps->width;

      if (ps->mask[off] == mask_val)
        {
          gint coff = off * 3;
          (*samples)[i*3]   = ps->colors[coff];
          (*samples)[i*3+1] = ps->colors[coff+1];
          (*samples)[i*3+2] = ps->colors[coff+2];
          i++;
        }
    }
}

static void
paint_select_init_fg_bg_models (PaintSelect  *ps)
{
  gint n;
  gfloat increment = 1.f / (gfloat) ps->n_fg_samples;

  memset (&ps->fg_hist, 0.f, sizeof (Histogram));

  for (n = 0; n < ps->n_fg_samples; n++)
    {
      gfloat *sample = ps->fg_samples + n * 3;
      gint    b1 = (gint) (sample[0] * (N_BINS - 1));
      gint    b2 = (gint) (sample[1] * (N_BINS - 1));
      gint    b3 = (gint) (sample[2] * (N_BINS - 1));
      ps->fg_hist.bins[b1][b2][b3] += increment;
    }

  memset (&ps->bg_hist, 0.f, sizeof (Histogram));

  increment = 1.f / (gfloat) ps->n_bg_samples;

  for (n = 0; n < ps->n_bg_samples; n++)
    {
      gfloat *sample = ps->bg_samples + n * 3;
      gint    b1 = (gint) (sample[0] * (N_BINS - 1));
      gint    b2 = (gint) (sample[1] * (N_BINS - 1));
      gint    b3 = (gint) (sample[2] * (N_BINS - 1));
      ps->bg_hist.bins[b1][b2][b3] += increment;
    }
}

/* SOURCE terminal is foreground (selected pixels)
 * SINK terminal is background (unselected pixels)
 */

static void
paint_select_init_graph_nodes_and_tlinks (PaintSelect  *ps)
{
  gfloat          node_mask;
  ConstraintType  boundary;
  gint            x, y;

  if (ps->mode == GEGL_PAINT_SELECT_MODE_ADD)
    {
      node_mask  = BG_MASK;
      boundary   = HARD_SOURCE;
    }
  else
    {
      node_mask  = FG_MASK;
      boundary   = HARD_SINK;
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
                  gint b1 = (gint) (ps->colors[coff]   * (N_BINS - 1));
                  gint b2 = (gint) (ps->colors[coff+1] * (N_BINS - 1));
                  gint b3 = (gint) (ps->colors[coff+2] * (N_BINS - 1));

                  sink_weight   = - logf (ps->fg_hist.bins[b1][b2][b3] + 0.0001f);
                  source_weight = - logf (ps->bg_hist.bins[b1][b2][b3] + 0.0001f);
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

static void
paint_select_update_mask (PaintSelect  *ps)
{
  gint  i;

  for (i = 0; i < ps->width * ps->height; i++)
    {
      node_id id = ps->nodes[i];

      if (id != -1)
        {
          if (ps->graph->what_segment(id) == GraphType::SOURCE)
            ps->mask[i] = 1.f;
          else
            ps->mask[i] = 0.f;
        }
    }
}

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
  PaintSelect  ps;
  gfloat       flow;
  GeglRectangle region;

  if (! aux || ! aux2)
    {
      gegl_buffer_copy (input, NULL, GEGL_ABYSS_NONE, output, NULL);
      return TRUE;
    }

  /* memory allocations, pixels fetch */

  paint_select_init_buffers (&ps, input, aux, aux2, o->mode);

  /* find the overlap region where scribble value doesn't match mask value */

  region = paint_select_get_local_region (&ps);
  g_printerr ("local region: (%d,%d) %d x %d\n",
              region.x, region.y, region.width, region.height);

  if (! gegl_rectangle_is_empty (&region) &&
      ! gegl_rectangle_is_infinite_plane (&region))
    {
      /* retrieve colors samples and init histograms */

      paint_select_init_local_samples (&ps, &region);
      paint_select_init_global_samples (&ps);

      g_printerr ("n fg samples: %d\n", ps.n_fg_samples);
      g_printerr ("n bg samples: %d\n", ps.n_bg_samples);

      paint_select_init_fg_bg_models (&ps);

      /* init graph */

      paint_select_compute_adjacent_costs (&ps);
      g_printerr ("mean adjacent costs: %f\n", ps.mean_costs);
      paint_select_init_graph_nodes_and_tlinks (&ps);
      paint_select_init_graph_nlinks (&ps);

      /* compute flow and set result */

      flow = ps.graph->maxflow();
      g_printerr ("flow: %f\n", flow);

      paint_select_update_mask (&ps);

      gegl_buffer_set (output, NULL, 0, babl_format (SELECTION_FORMAT),
                       ps.mask, GEGL_AUTO_ROWSTRIDE);
    }

  paint_select_free_buffers (&ps);

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass          *operation_class;
  GeglOperationComposer3Class *composer_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  composer_class  = GEGL_OPERATION_COMPOSER3_CLASS (klass);

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
