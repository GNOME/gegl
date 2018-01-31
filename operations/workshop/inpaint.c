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
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2018 Øyvind Kolås <pippin@gimp.org>
 *
 */

#include <stdio.h>
#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int (seek_distance, "seek radius", 16)
  value_range (4, 512)

property_int (min_neigh, "min neigh", 7)
  value_range (1, 10)

property_int (min_iter, "min iter", 96)
  value_range (1, 512)

property_double (chance_try, "try chance", 0.66)
  value_range (0.0, 1.0)

property_double (chance_retry, "retry chance", 0.33)
  value_range (0.0, 1.0)

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME      inpaint
#define GEGL_OP_C_SOURCE  inpaint.c

#include "gegl-op.h"
#include <math.h>
#include <stdio.h>

#define POW2(x) ((x)*(x))

#define INITIAL_SCORE 1200000000
#define NEIGHBORHOOD 49
#if 1
int order_2d[][15]={
                 {  0,  0,  0,  0,  0,142,111,112,126,128,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,124,110, 86, 87, 88,113,129,143,  0,  0,  0},
                 {  0,  0,  0,125,109, 85, 58, 59, 60, 89, 90,114,  0,  0,  0},
                 {  0,  0,  0,108, 78, 57, 38, 39, 40, 61, 79,115,130,  0,  0},
                 {  0,  0,107, 84, 76, 37, 26, 21, 27, 41, 62, 91,116,  0,  0},
                 {  0,153,106, 77, 56, 25, 17,  9, 13, 28, 42, 62, 92,152,  0},
                 {  0,149,105, 55, 36, 16,  8,  1,  5, 18, 29, 43, 63,150,  0},
                 {  0,145, 75, 54, 24, 12,  4,  0,  2, 10, 22, 44, 64,147,  0},
                 {  0,146,104, 53, 35, 20,  7,  3,  6, 14, 30, 45, 65,148,  0},
                 {  0,154,103, 74, 52, 34, 15, 11, 19, 31, 46, 66, 93,152,  0},
                 {  0,156,102, 83, 73, 51, 33, 23, 32, 47, 67, 80,117,158,  0},
                 {  0,  0,123,101, 82, 72, 50, 49, 48, 68, 81, 94,131,159,  0},
                 {  0,  0,  0,122,100, 99, 71, 70, 69, 95,118,132,140,  0,  0},
                 {  0,  0,  0,  0,121,120, 98, 97, 96,119,133,139,  0,  0,  0},
                 {  0,  0,  0,  0,144,138,136,134,135,137,141,  0,  0,  0,  0},
               };
#else
int order_2d[][15]={
                 {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0, 21,  0,  0, 22,  0,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0,  0, 17,  0,  0,  0, 18,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0,  0,  0,  9,  0, 13,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0, 16,  8,  1,  5,  0,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0,  0,  4,  0,  2, 10,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0, 12,  7,  3,  6,  0,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0,  0,  0, 11,  0,  0, 23,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0,  0, 15,  0,  0, 14,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0, 20,  0,  0, 19,  0,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
               };
#endif

static int order[512][3];
static GHashTable *ht = NULL;
static GList *probes = NULL;

static void init_order(void)
{
  static int inited = 0;
  int i, x, y;
  if (inited)
    return;

  order[0][0] = 0;
  order[0][1] = 0;
  order[0][2] = 1;

  for (i = 1; i < 159; i++)
  for (y = -7; y <= 7; y ++)
    for (x = -7; x <= 7; x ++)
      {
        if (order_2d[x+7][y+7] == i)
        {
          order[i][0] = x;
          order[i][1] = y;
          order[i][2] = POW2(x)+POW2(y);
        }
      }
  inited = 1;
}


static void
prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("RGBA float");

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  /* Don't request an infinite plane */
  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;

  return result;
}


static void extract_site (GeglBuffer *input, int x, int y, guchar *dst)
{
  static const Babl *format = NULL;
  if (!format) format = babl_format ("R'G'B'A u8");
  for (int i = 0; i <= NEIGHBORHOOD; i++)
  {
    gegl_buffer_sample (input,  x + order[i][0], y + order[i][1], NULL,
      &dst[i*4], format, GEGL_SAMPLER_NEAREST, 0);
  }
}

static int u8_rgb_diff (guchar *a, guchar *b)
{
  return POW2(a[0]-b[0]) * 2 + POW2(a[1]-b[1]) * 4 + POW2(a[2]-b[2]) * 1;
}


static int inline score_site (guchar *needle, guchar *hay, int bail)
{
  int i;
  int score = 0;
  /* bail early with really bad score - the target site doesnt have opacity */

  if (hay[3] < 2)
  {
    return INITIAL_SCORE;
  }

  for (i = 1; i < NEIGHBORHOOD && score < bail; i++)
  {
    if (needle[i*4 + 3] && hay[i*4 + 3])
    {
      score += u8_rgb_diff (&needle[i*4 + 0], &hay[i*4 + 0]) * 10 / order[i][2];
    }
    else
    {
      /* we score missing cells as if it is a big diff */
      score += ((POW2(36))*3) * 70 / order[i][2];
    }
  }
  return score;
}

typedef struct Probe {
  int target_x;
  int target_y;
  int age;
  int score;
  int source_x;
  int source_y;
} Probe;


static void add_probe (GeglOperation *operation, int target_x, int target_y)
{
  Probe *probe    = g_malloc0 (sizeof (Probe));
  probe->target_x = target_x;
  probe->target_y = target_y;
  probe->source_x = target_x;
  probe->source_y = target_y;
  probe->score    = INITIAL_SCORE;
  probes          = g_list_prepend (probes, probe);
}

static int probe_rel_is_set (GeglBuffer *output, Probe *probe, int rel_x, int rel_y)
{
  static const Babl *format = NULL;
  guchar pix[4];
  if (!format) format = babl_format ("R'G'B'A u8");
  gegl_buffer_sample (output, probe->target_x + rel_x, probe->target_y + rel_y, NULL, &pix[0], format, GEGL_SAMPLER_NEAREST, 0);
  return pix[3] > 5;
}


static int probe_neighbors (GeglBuffer *output, Probe *probe)
{
  return probe_rel_is_set (output, probe, -1, 0) +
         probe_rel_is_set (output, probe,  1, 0) +
         probe_rel_is_set (output, probe,  0, 1) +
         probe_rel_is_set (output, probe,  0, -1)
#if 1
         + probe_rel_is_set (output, probe,  1, 1)
         + probe_rel_is_set (output, probe,  2, 0)
         + probe_rel_is_set (output, probe,  0, 2)
         + probe_rel_is_set (output, probe, -2, 0)
         + probe_rel_is_set (output, probe,  0, -2)
         + probe_rel_is_set (output, probe, -1,-1)
         + probe_rel_is_set (output, probe,  1,-1)
         + probe_rel_is_set (output, probe, -1, 1)
         + probe_rel_is_set (output, probe, -3, 0)
         + probe_rel_is_set (output, probe,  3, 0)
         + probe_rel_is_set (output, probe,  0, 3)
         + probe_rel_is_set (output, probe,  0, -3)
#endif
;
}

static int probe_improve (GeglOperation       *operation,
                          GeglBuffer          *input,
                          GeglBuffer          *output,
                          const GeglRectangle *result,
                          Probe               *probe,
                          int                  min_neighbours)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  guchar needle[4 * NEIGHBORHOOD];
  gint  dst_x  = probe->target_x;
  gint  dst_y  = probe->target_y;
  gint *best_x = &probe->source_x;
  gint *best_y = &probe->source_y;
  gint start_x = probe->source_x;
  gint start_y = probe->source_y;
  int old_score = probe->score;
  static const Babl *format = NULL;
  if (!format)
    format = babl_format ("RGBA float");
  extract_site (output, dst_x, dst_y, &needle[0]);

#if 0
  if (probe->score < 10000)
    return 0;
#endif

#if 0
  for (int dy = -o->seek_distance; dy < o->seek_distance; dy += (dy*dy < 64 ? 1 : 3))
    for (int dx = -o->seek_distance ; dx < o->seek_distance; dx += (dx*dx < 64 ? 1 : 3))
#else
  for (int dy = -o->seek_distance; dy < o->seek_distance; dy ++)
    for (int dx = -o->seek_distance ; dx < o->seek_distance; dx ++)
#endif
      {
#define xy2offset(x,y)   ((y) * result->width + (x))
        int offset = xy2offset(start_x + dx, start_y + dy);
        int score;
        guchar *hay = NULL;

        if (start_x + dx == dst_x &&
            start_y + dy == dst_y)
          continue;

        if (offset < 0 || offset >= result->width * result->height)
          continue;
        if (start_x + dx < 5 || start_y + dy < 5 ||
            start_x + dx > result->width - 5 ||
            start_y + dy > result->height - 5)
          continue;

        hay = g_hash_table_lookup (ht, GINT_TO_POINTER(offset));
        if (!hay)
        {
          hay = g_malloc (4 * NEIGHBORHOOD);
          extract_site (input, start_x + dx, start_y + dy, hay);
          g_hash_table_insert (ht, GINT_TO_POINTER(offset), hay);
        }

        score = score_site (&needle[0], hay, probe->score);

        if (score < probe->score)
          {
            *best_x = start_x + dx;
            *best_y = start_y + dy;
            probe->score = score;
          }
      }

  if (old_score != probe->score && old_score != INITIAL_SCORE) 
    {
      /* spread our resulting neighborhodo to unset neighbors */
      if (!probe_rel_is_set (output, probe, -1, 0))
        {
          for (GList *l = probes; l; l = l->next)
          {
            Probe *neighbor_probe = l->data;
            if ( (probe->target_y      == neighbor_probe->target_y) &&
                ((probe->target_x - 1) == neighbor_probe->target_x))
            {
              gfloat rgba[4];
              gegl_buffer_sample (input, probe->source_x - 1, probe->source_y,  NULL, &rgba[0], format, GEGL_SAMPLER_NEAREST, 0);
              if (rgba[3] > 0.001)
               {
                 neighbor_probe->source_x = probe->source_x - 1;
                 neighbor_probe->source_y = probe->source_y;
                 neighbor_probe->score --;
                 gegl_buffer_set (output, GEGL_RECTANGLE(neighbor_probe->target_x, neighbor_probe->target_y, 1, 1), 0, format, &rgba[0], 0);
                 neighbor_probe->age++;
               }
            }
          }
        }

      if (!probe_rel_is_set (output, probe, 1, 0))
        {
          for (GList *l = probes; l; l = l->next)
          {
            Probe *neighbor_probe = l->data;
            if ( (probe->target_y      == neighbor_probe->target_y) &&
                ((probe->target_x + 1) == neighbor_probe->target_x))
            {
              gfloat rgba[4];
              gegl_buffer_sample (input, probe->source_x + 1, probe->source_y,  NULL, &rgba[0], format, GEGL_SAMPLER_NEAREST, 0);
              if (rgba[3] > 0.001)
               {
                 neighbor_probe->source_x = probe->source_x + 1;
                 neighbor_probe->source_y = probe->source_y;
                 neighbor_probe->score --;
                 gegl_buffer_set (output, GEGL_RECTANGLE(neighbor_probe->target_x, neighbor_probe->target_y, 1, 1), 0, format, &rgba[0], 0);
                 neighbor_probe->age++;
               }
            }
          }
        }

      if (!probe_rel_is_set (output, probe, 0, -1))
        {
          for (GList *l = probes; l; l = l->next)
          {
            Probe *neighbor_probe = l->data;
            if ( (probe->target_y - 1  == neighbor_probe->target_y) &&
                ((probe->target_x    ) == neighbor_probe->target_x))
            {
              gfloat rgba[4];
              gegl_buffer_sample (input, probe->source_x, probe->source_y - 1,  NULL, &rgba[0], format, GEGL_SAMPLER_NEAREST, 0);
              if (rgba[3] > 0.001)
               {
                 neighbor_probe->source_x = probe->source_x;
                 neighbor_probe->source_y = probe->source_y - 1;
                 neighbor_probe->score --;
                 gegl_buffer_set (output, GEGL_RECTANGLE(neighbor_probe->target_x, neighbor_probe->target_y, 1, 1), 0, format, &rgba[0], 0);
                 neighbor_probe->age++;
               }
            }
          }
        }

      if (!probe_rel_is_set (output, probe, 0, 1))
        {
          for (GList *l = probes; l; l = l->next)
          {
            Probe *neighbor_probe = l->data;
            if ( (probe->target_y + 1  == neighbor_probe->target_y) &&
                ((probe->target_x    ) == neighbor_probe->target_x))
            {
              gfloat rgba[4];
              gegl_buffer_sample (input, probe->source_x, probe->source_y + 1,  NULL, &rgba[0], format, GEGL_SAMPLER_NEAREST, 0);
              if (rgba[3] > 0.001)
               {
                 neighbor_probe->source_x = probe->source_x;
                 neighbor_probe->source_y = probe->source_y + 1;
                 neighbor_probe->score --;
                 gegl_buffer_set (output, GEGL_RECTANGLE(neighbor_probe->target_x, neighbor_probe->target_y, 1, 1), 0, format, &rgba[0], 0);
                 neighbor_probe->age++;
               }
            }
          }
        }


    }
  if (probe->score == old_score)
    return -1;
  return 0;
}

static void copy_buf (GeglOperation *operation,
                      GeglBuffer    *input,
                      GeglBuffer    *output,
                      const GeglRectangle *result)
{
  const Babl *format = babl_format ("RGBA float");
  GeglBufferIterator *i = gegl_buffer_iterator_new (output,
                                                    result,
                                                    0,
                                                    format,
                                                    GEGL_ACCESS_WRITE,
                                                    GEGL_ABYSS_NONE);
  gegl_buffer_iterator_add (i, input, result, 0, format, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  while (gegl_buffer_iterator_next (i))
  {
    gint x = i->roi[0].x;
    gint y = i->roi[0].y;
    gint n_pixels  = i->roi[0].width * i->roi[0].height;
    float *in_pix  = i->data[1];
    float *out_pix = i->data[0];

    while (n_pixels--)
    {
      if (in_pix[3] <= 0.001)
      {
        out_pix[0] = 0;
        out_pix[1] = 0;
        out_pix[2] = 0;
        out_pix[3] = 0;
        add_probe (operation, x, y);
      }
      else
      {
        out_pix[0] = in_pix[0];
        out_pix[1] = in_pix[1];
        out_pix[2] = in_pix[2];
        out_pix[3] = in_pix[3];
      }

      in_pix += 4;
      out_pix += 4;

      x++;
      if (x >= i->roi[0].x + i->roi[0].width)
      {
        x = i->roi[0].x;
        y++;
      }
    }
  }
}

static void do_inpaint (GeglOperation *operation,
                        GeglBuffer *input,
                        GeglBuffer *output,
                        const GeglRectangle *result)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *format = babl_format ("RGBA float");
  gint missing = 1;
  gint old_missing = 3;
  gint total = 0;
  gint runs = 0;

  while (missing != old_missing || runs < o->min_iter)
  { runs++;
    total = 0;
    old_missing = missing;
    missing = 0;
  for (GList *p= probes; p; p= p->next)
  {
    Probe *probe = p->data;
    gint try_replace;

    if (probe->score == INITIAL_SCORE)
    {
      missing ++;
      try_replace =  0;
    }
    else
    {
      try_replace = (probe->age < 8 &&
                     ((rand()%100)/100.0) < o->chance_retry);
    }
    total ++;

    if ((probe->source_x == probe->target_x &&
        probe->source_y == probe->target_y) || try_replace)
    {
       if ((probe->source_x == probe->target_x &&
            probe->source_y == probe->target_y))
         try_replace = 0;

      if ((rand()%100)/100.0 < o->chance_try && probe_neighbors (output, probe) >= o->min_neigh)
      {
        if(try_replace)
         probe->score = INITIAL_SCORE;
        if (probe_improve (operation, input, output, result, probe, 1) == 0)
        {
          gfloat rgba[4];
          gegl_buffer_sample (input,  probe->source_x, probe->source_y,  NULL, &rgba[0], format, GEGL_SAMPLER_NEAREST, 0);
          if (rgba[3] <= 0.01)
            fprintf (stderr, "eek %i,%i %f %f %f %f\n", probe->source_x, probe->source_y, rgba[0], rgba[1], rgba[2], rgba[3]);
          gegl_buffer_set (output, GEGL_RECTANGLE(probe->target_x, probe->target_y, 1, 1), 0, format, &rgba[0], 0);
          probe->age++;
         }
      }
    }
  }
  gegl_operation_progress (operation, (total-missing) * 1.0 / total,
                           "finding suitable pixels");
#if 0
  fprintf (stderr, "\r%i/%i %2.2f run#:%i  ", total-missing, total, (total-missing) * 100.0 / total, runs);
#endif
  }
  fprintf (stderr, "\n");
}


static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  init_order ();
  ht = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);

  copy_buf (operation, input, output, result);
  do_inpaint (operation, input, output, result);

  while (probes)
  {
    g_free (probes->data);
    probes = g_list_remove (probes, probes->data);
  }

  g_hash_table_destroy (ht);

  return TRUE;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;

  return result;
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

  operation_class = GEGL_OPERATION_CLASS (gegl_op_parent_class);

  if (in_rect && gegl_rectangle_is_infinite_plane (in_rect))
    {
      gpointer in = gegl_operation_context_get_object (context, "input");
      gegl_operation_context_take_object (context, "output",
                                          g_object_ref (G_OBJECT (in)));
      return TRUE;
    }

  return operation_class->process (operation, context, output_prop, result,
                                   gegl_operation_context_get_level (context));
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process                    = process;
  operation_class->prepare                 = prepare;
  operation_class->process                 = operation_process;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region = get_cached_region;
  operation_class->opencl_support          = FALSE;
  operation_class->threaded                = FALSE;

  gegl_operation_class_set_keys (operation_class,
      "name",        "gegl:alpha-inpaint",
      "title",       "Heal transparent",
      "categories",  "heal",
      "description", "Replaces fully transparent pixels with good candidate pixels found in the neighbourhood of the hole",
      NULL);
}

#endif
