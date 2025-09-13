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
 * Copyright 2016 Thomas Manni <thomas.manni@free.fr>
 */

 /* Propagate labels by wathershed transformation using hierarchical queues */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int (flag_component, _("Index of component flagging unlabelled pixels"), -1)
   description(_("Index of component flagging unlabelled pixels"))
   ui_range    (-1, 4)

property_format (flag, _("flag"), NULL)
   description (_("Pointer to flag value for unlabelled pixels"))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME      watershed_transform
#define GEGL_OP_C_SOURCE  watershed-transform.c

#include "gegl-op.h"

typedef struct _PixelCoords
{
  gint x;
  gint y;
} PixelCoords;

typedef struct _HQ
{
  GQueue  *queues[256];
  GQueue  *lowest_non_empty;
  gint     lowest_non_empty_level;
} HQ;

static void
HQ_init (HQ *hq)
{
  gint i;

  for (i = 0; i < 256; i++)
    hq->queues[i] = g_queue_new ();

  hq->lowest_non_empty       = NULL;
  hq->lowest_non_empty_level = 255;
}

static gboolean
HQ_is_empty (HQ *hq)
{
  if (hq->lowest_non_empty == NULL)
    return TRUE;

  return FALSE;
}

static inline void
HQ_push (HQ      *hq,
         guint8   level,
         gpointer data)
{
  g_queue_push_head (hq->queues[level], data);

  if (level <= hq->lowest_non_empty_level)
    {
      hq->lowest_non_empty_level = level;
      hq->lowest_non_empty       = hq->queues[level];
    }
}

static inline gpointer
HQ_pop (HQ *hq)
{
  gint i, level;
  gpointer data = NULL;

  if (hq->lowest_non_empty != NULL)
    {
      data = g_queue_pop_tail (hq->lowest_non_empty);

      if (g_queue_is_empty (hq->lowest_non_empty))
        {
          level = hq->lowest_non_empty_level;
          hq->lowest_non_empty_level = 255;
          hq->lowest_non_empty       = NULL;

          for (i = level + 1; i < 256; i++)
            if (!g_queue_is_empty (hq->queues[i]))
              {
                hq->lowest_non_empty_level = i;
                hq->lowest_non_empty       = hq->queues[i];
                break;
              }
        }
    }

  return data;
}

static void
HQ_clean (HQ *hq)
{
  gint i;

  for (i = 0; i < 256; i++)
    {
      if (!g_queue_is_empty (hq->queues[i]))
        g_printerr ("queue %u is not empty!\n", i);
      else
        g_queue_free (hq->queues[i]);
    }
}

static void
attach (GeglOperation *self)
{
  GeglOperation *operation = GEGL_OPERATION (self);
  GParamSpec    *pspec;

  pspec = g_param_spec_object ("output",
                               "Output",
                               "Output pad for generated image buffer.",
                               GEGL_TYPE_BUFFER,
                               G_PARAM_READABLE |
                               GEGL_PARAM_PAD_OUTPUT);
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);

  pspec = g_param_spec_object ("input",
                               "Input",
                               "Input pad, for image buffer input.",
                               GEGL_TYPE_BUFFER,
                               G_PARAM_READWRITE |
                               GEGL_PARAM_PAD_INPUT);
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);

  pspec = g_param_spec_object ("aux",
                               "Aux",
                               "Auxiliary image buffer input pad.",
                               GEGL_TYPE_BUFFER,
                               G_PARAM_READWRITE |
                               GEGL_PARAM_PAD_INPUT);
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);
}

static void
prepare (GeglOperation *operation)
{
  const Babl *labels_format   = gegl_operation_get_source_format (operation, "input");
  const Babl *gradient_format = babl_format ("Y u8");

  gegl_operation_set_format (operation, "output", labels_format);
  gegl_operation_set_format (operation, "aux",    gradient_format);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle *region;

  region = gegl_operation_source_get_bounding_box (operation, "input");

  if (region != NULL)
    return *region;
  else
    return *GEGL_RECTANGLE (0, 0, 0, 0);
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}

static GeglRectangle
get_invalidated_by_change (GeglOperation       *operation,
                           const gchar         *input_pad,
                           const GeglRectangle *input_region)
{
  return get_bounding_box (operation);
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *aux,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level,
         guint8              *flag,
         gint                 flag_idx)
{
  HQ      hq;
  guint8  square3x3[72];
  gint    i;
  gint    j;
  gint    x, y;
  GeglBufferIterator  *iter;
  GeglSampler         *gradient_sampler = NULL;
  const GeglRectangle *extent = gegl_buffer_get_extent (input);

  const Babl  *gradient_format = babl_format ("Y u8");
  const Babl  *labels_format   = gegl_buffer_get_format (input);
  gint         bpp             = babl_format_get_bytes_per_pixel (labels_format);
  gint         bpc             = bpp / babl_format_get_n_components (labels_format);

  gint neighbors_coords[8][2] = {{-1, -1},{0, -1},{1, -1},
                                 {-1, 0},         {1, 0},
                                 {-1, 1}, {0, 1}, {1, 1}};

  /* initialize hierarchical queues */

  HQ_init (&hq);

  iter = gegl_buffer_iterator_new (input, extent, 0, labels_format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE,
                                   aux ? 11 : 10);

  gegl_buffer_iterator_add (iter, output, extent, 0, labels_format,
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  /* Add 8 neighbours. */
  gegl_buffer_iterator_add (iter, input,
                            GEGL_RECTANGLE (-1, -1, extent->width, extent->height),
                            0, labels_format, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  gegl_buffer_iterator_add (iter, input,
                            GEGL_RECTANGLE (0, -1, extent->width, extent->height),
                            0, labels_format, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  gegl_buffer_iterator_add (iter, input,
                            GEGL_RECTANGLE (1, -1, extent->width, extent->height),
                            0, labels_format, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  gegl_buffer_iterator_add (iter, input,
                            GEGL_RECTANGLE (-1, 0, extent->width, extent->height),
                            0, labels_format, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  gegl_buffer_iterator_add (iter, input,
                            GEGL_RECTANGLE (1, 0, extent->width, extent->height),
                            0, labels_format, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  gegl_buffer_iterator_add (iter, input,
                            GEGL_RECTANGLE (-1, 1, extent->width, extent->height),
                            0, labels_format, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  gegl_buffer_iterator_add (iter, input,
                            GEGL_RECTANGLE (0, 1, extent->width, extent->height),
                            0, labels_format, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  gegl_buffer_iterator_add (iter, input,
                            GEGL_RECTANGLE (1, 1, extent->width, extent->height),
                            0, labels_format, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  /* Priority map: lower is higher priority. */
  if (aux)
    gegl_buffer_iterator_add (iter, aux, extent, 0, gradient_format,
                              GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      GeglRectangle *roi      = &iter->items[0].roi;
      guint8        *label    = iter->items[0].data;
      guint8        *outlabel = iter->items[1].data;
      guint8        *n[8]     =
        {
          iter->items[2].data,
          iter->items[3].data,
          iter->items[4].data,
          iter->items[5].data,
          iter->items[6].data,
          iter->items[7].data,
          iter->items[8].data,
          iter->items[9].data
        };
      guint8        *prio    = aux ? iter->items[10].data : NULL;

      for (y = roi->y; y < roi->y + roi->height; y++)
        for (x = roi->x; x < roi->x + roi->width; x++)
          {
            gboolean flagged = TRUE;

            for (i = 0; i < bpc; i++)
              if (label[flag_idx * bpc + i] != (flag ? flag[i] : 0))
                {
                  flagged = FALSE;
                  break;
                }
            if (! flagged)
              {
                for (j = 0; j < 8; j++)
                  {
                    if (x == 0 && (j == 0 || j == 3 || j == 5))
                      continue;
                    if (y == 0 && (j == 0 || j == 1 || j == 2))
                      continue;
                    if (x == extent->width - 1 && (j == 2 || j == 4 || j == 7))
                      continue;
                    if (y == extent->height - 1 && (j == 5 || j == 6 || j == 7))
                      continue;

                    flagged = TRUE;
                    for (i = 0; i < bpc; i++)
                      if (n[j][flag_idx * bpc + i] != (flag ? flag[i] : 0))
                        {
                          flagged = FALSE;
                          break;
                        }
                    if (flagged)
                      break;
                  }
                if (flagged)
                  {
                    /* This pixel is not flagged and has at least one flagged
                     * neighbour.
                     */
                    PixelCoords *p = g_new (PixelCoords, 1);
                    p->x = x;
                    p->y = y;

                    HQ_push (&hq, prio ? *prio : 0, p);
                  }
              }

            for (i = 0; i < bpp; i++)
              outlabel[i] = label[i];

            if (prio)
              prio++;
            label    += bpp;
            outlabel += bpp;
            for (j = 0; j < 8; j++)
              n[j] += bpp;
          }
    }

  if (aux)
    gradient_sampler = gegl_buffer_sampler_new_at_level (aux,
                                                         gradient_format,
                                                         GEGL_SAMPLER_NEAREST,
                                                         level);
  while (!HQ_is_empty (&hq))
    {
      PixelCoords *p = (PixelCoords *) HQ_pop (&hq);
      guint8       label[bpp];

      GeglRectangle square_rect = {p->x - 1, p->y - 1, 3, 3};

      gegl_buffer_get (output, &square_rect, 1.0, labels_format,
                       square3x3,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      for (i = 0; i < bpp; i++)
        label[i] = square3x3[4 * bpp + i];

      /* compute neighbors coordinate */
      for (j = 0; j < 8; j++)
        {
          guint8   *neighbor_label;
          gint      nx = p->x + neighbors_coords[j][0];
          gint      ny = p->y + neighbors_coords[j][1];
          gboolean  flagged = TRUE;

          if (nx < 0 || nx >= extent->width || ny < 0 || ny >= extent->height)
            continue;

          neighbor_label = square3x3 + ((neighbors_coords[j][0] + 1) + (neighbors_coords[j][1] + 1) * 3) * bpp;

          for (i = 0; i < bpc; i++)
            if (neighbor_label[flag_idx * bpc + i] != (flag ? flag[i] : 0))
              {
                flagged = FALSE;
                break;
              }
          if (flagged)
            {
              guint8 gradient_value = 0;
              GeglRectangle n_rect = {nx, ny, 1, 1};
              PixelCoords *n = g_new (PixelCoords, 1);
              n->x = nx;
              n->y = ny;

              if (gradient_sampler)
                gegl_sampler_get (gradient_sampler,
                                  (gdouble) nx,
                                  (gdouble) ny,
                                  NULL, &gradient_value, GEGL_ABYSS_NONE);

              HQ_push (&hq, gradient_value, n);

              for (i = 0; i < bpp; i++)
                neighbor_label[i] = label[i];

              gegl_buffer_set (output, &n_rect,
                               0, labels_format,
                               neighbor_label, GEGL_AUTO_ROWSTRIDE);
            }
        }

      g_free (p);
    }
  if (gradient_sampler)
    g_object_unref (gradient_sampler);

  HQ_clean (&hq);
  return  TRUE;
}

static gboolean
operation_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglBuffer     *input = NULL;
  GeglBuffer     *aux;
  GeglBuffer     *output;
  gboolean        success;
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gint            n_comp;

  aux    = (GeglBuffer*) gegl_operation_context_dup_object (context, "aux");
  input  = (GeglBuffer*) gegl_operation_context_dup_object (context, "input");
  n_comp = babl_format_get_n_components (gegl_buffer_get_format (input));

  if (o->flag_component >= n_comp || o->flag_component < -n_comp)
    {
      g_warning ("The input buffer has %d components. Invalid flag component: %d",
                 n_comp, o->flag_component);
      success = FALSE;
    }
  else
    {
      gint flag_idx = o->flag_component < 0 ? n_comp + o->flag_component : o->flag_component;

      output = gegl_operation_context_get_target (context, "output");

      success = process (operation, input, aux, output, result, level,
                         (guint8 *) o->flag, flag_idx);
    }

  g_clear_object (&input);
  g_clear_object (&aux);

  return success;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach                    = attach;
  operation_class->prepare                   = prepare;
  operation_class->process                   = operation_process;
  operation_class->get_bounding_box          = get_bounding_box;
  operation_class->get_required_for_output   = get_required_for_output;
  operation_class->get_invalidated_by_change = get_invalidated_by_change;
  operation_class->get_cached_region         = get_cached_region;
  operation_class->opencl_support            = FALSE;
  operation_class->threaded                  = FALSE;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:watershed-transform",
    "title",       _("Watershed Transform"),
    "categories",  "hidden",
    "reference-hash", "983ef24a840ad8e46698ffd7cd11f5b8",
    "description", _("Labels propagation by watershed transformation. "
                     "Output buffer will keep the input format. "
                     "Unlabelled pixels are marked with a given flag value "
                     "(by default: last component with NULL value). "
                     "The aux buffer is a \"Y u8\" image representing the priority levels "
                     "(lower value is higher priority). If aux is absent, "
                     "all labellized pixels have the same priority "
                     "and propagated labels have a lower priority."),
    NULL);
}

#endif
