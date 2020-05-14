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
 * Copyright (C) 2020 Ell
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>
#include <stdio.h>

#define N_AUX_INPUTS 16

#ifdef GEGL_PROPERTIES

property_int (levels, _("Levels"), 0)
    description (_("Number of blend levels"))
    value_range (0, N_AUX_INPUTS)

property_boolean (normalized_mask, _("Normalized mask"), TRUE)
    description (_("Mask values are in the [0,1] range"))

property_boolean (linear_mask, _("Linear mask"), TRUE)
    description (_("Use linear mask values"))

#else

#define GEGL_OP_BASE
#define GEGL_OP_NAME     piecewise_blend
#define GEGL_OP_C_SOURCE piecewise-blend.cc

#include "gegl-op.h"

static void
attach (GeglOperation *operation)
{
  GParamSpec *pspec;
  gint        i;

  pspec = g_param_spec_object ("output",
                               "Output",
                               "Output pad for generated image buffer.",
                               GEGL_TYPE_BUFFER,
                               (GParamFlags) (G_PARAM_READABLE |
                                              GEGL_PARAM_PAD_OUTPUT));
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);

  pspec = g_param_spec_object ("input",
                               "Input",
                               "Input pad, for image buffer input.",
                               GEGL_TYPE_BUFFER,
                               (GParamFlags) (G_PARAM_READABLE |
                                              GEGL_PARAM_PAD_INPUT));
  gegl_operation_create_pad (operation, pspec);
  g_param_spec_sink (pspec);

  for (i = 1; i <= N_AUX_INPUTS; i++)
    {
      gchar aux_name[32];
      gchar aux_desc[32];

      sprintf (aux_name, "aux%d",  i);
      sprintf (aux_desc, "Aux %d", i);

      pspec = g_param_spec_object (aux_name,
                                   aux_desc,
                                   "Auxiliary image buffer input pad.",
                                   GEGL_TYPE_BUFFER,
                                   (GParamFlags) (G_PARAM_READABLE |
                                                  GEGL_PARAM_PAD_INPUT));
      gegl_operation_create_pad (operation, pspec);
      g_param_spec_sink (pspec);
    }
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle *in_rect;
  GeglRectangle  result = {};

  in_rect = gegl_operation_source_get_bounding_box (operation, "input");

  if (in_rect)
    result = *in_rect;

  return result;
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl      *space;
  const Babl      *input_space;
  const Babl      *format;
  const Babl      *input_format;
  gint             i;

  input_space  = gegl_operation_get_source_space (operation, "input");
  input_format = babl_format_with_space (o->linear_mask ? "Y float" :
                                                          "Y' float",
                                         input_space);

  space  = gegl_operation_get_source_space (operation, "aux1");
  format = babl_format_with_space ("RaGaBaA float", space);

  gegl_operation_set_format (operation, "input",  input_format);
  gegl_operation_set_format (operation, "output", format);

  for (i = 1; i <= N_AUX_INPUTS; i++)
    {
      gchar aux_name[32];

      sprintf (aux_name, "aux%d", i);

      gegl_operation_set_format (operation, aux_name, format);
    }
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  GeglRectangle   result = {};

  if (! strcmp (input_pad, "input"))
    {
      result = *roi;
    }
  else if (g_str_has_prefix (input_pad, "aux"))
    {
      gint i = atoi (input_pad + 3);

      if (i <= o->levels)
        result = *roi;
    }

  return result;
}

static gboolean
process (GeglOperation        *operation,
         GeglOperationContext *context,
         const gchar          *output_prop,
         const GeglRectangle  *result,
         gint                  level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglBuffer     *input;
  GeglBuffer     *output;
  const Babl     *format;
  const Babl     *input_format;
  gfloat          scale;
  gint            levels;

  levels = o->levels;

  if (o->normalized_mask)
    scale = levels - 1.0f;
  else
    scale = 1.0f;

  if (levels == 0)
    {
      return TRUE;
    }
  else if (levels == 1)
    {
      gegl_operation_context_set_object (
        context, "output",
        gegl_operation_context_get_object (context, "aux1"));

      return TRUE;
    }

  format       = gegl_operation_get_format (operation, "output");
  input_format = gegl_operation_get_format (operation, "input");

  input  = GEGL_BUFFER (gegl_operation_context_get_object (context, "input"));
  output = gegl_operation_context_get_output_maybe_in_place (operation,
                                                             context,
                                                             input,
                                                             result);

  gegl_parallel_distribute_area (
    result, gegl_operation_get_pixels_per_thread (operation),
    [=] (const GeglRectangle *area)
    {
      GeglBuffer         *empty_buffer = NULL;
      GeglBufferIterator *iter;
      gint                i;

      iter = gegl_buffer_iterator_new (output, area, level, format,
                                       GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE,
                                       2 + levels);

      gegl_buffer_iterator_add (iter, input, area, level, input_format,
                                GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

      for (i = 1; i <= levels; i++)
        {
          GeglBuffer *aux;
          gchar       aux_name[32];

          sprintf (aux_name, "aux%d", i);

          aux = GEGL_BUFFER (gegl_operation_context_get_object (context,
                                                                aux_name));

          if (! aux)
            {
              if (! empty_buffer)
                {
                  empty_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, 0, 0),
                                                  format);
                }

              aux = empty_buffer;
            }

          gegl_buffer_iterator_add (iter, aux, area, level, format,
                                    GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
        }

      while (gegl_buffer_iterator_next (iter))
        {
          gfloat       *out = (      gfloat *) iter->items[0].data;
          const gfloat *in  = (const gfloat *) iter->items[1].data;
          gint          i;

          for (i = 0; i < iter->length; i++)
            {
              const gfloat *aux1;
              const gfloat *aux2;
              gfloat        v;
              gint          j;
              gint          c;

              v = *in * scale;

              v = v > 0.0f ? v < scale ? v : scale : 0.0f;

              j = (gint) v;
              j = MIN (j, levels - 2);

              v -= j;

              aux1 = (const gfloat *) iter->items[2 + j    ].data + 4 * i;
              aux2 = (const gfloat *) iter->items[2 + j + 1].data + 4 * i;

              for (c = 0; c < 4; c++)
                out[c] = aux1[c] + v * (aux2[c] - aux1[c]);

              out += 4;
              in++;
            }
        }

      g_clear_object (&empty_buffer);
    });

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach                    = attach;
  operation_class->get_bounding_box          = get_bounding_box;
  operation_class->prepare                   = prepare;
  operation_class->get_required_for_output   = get_required_for_output;
  operation_class->get_invalidated_by_change = get_required_for_output;
  operation_class->process                   = process;

  operation_class->threaded      = TRUE;
  operation_class->want_in_place = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:piecewise-blend",
    "title",       _("Piecewise Blend"),
    "categories",  "blend",
    "description", _("Blend a chain of inputs using a mask"),
    NULL);
}

#endif
