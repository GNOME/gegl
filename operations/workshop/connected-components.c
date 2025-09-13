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

property_color (separator, _("Separator"), "black")
  description (_("Component separator color"))

property_boolean (invert, _("Invert"), FALSE)
  description (_("Invert the separator region"))

property_double (base, _("Base"), 0.0)
  description (_("Base index"))
  value_range (-G_MAXDOUBLE, G_MAXDOUBLE)
  ui_range    (0.0, 1.0)

property_double (step, _("Step"), 1.0)
  description (_("Index step"))
  value_range (-G_MAXDOUBLE, G_MAXDOUBLE)
  ui_range    (0.0, 1.0)

property_boolean (normalize, _("Normalize"), TRUE)
  description (_("Normalize output to the range [base,base + step]"))

property_boolean (linear, _("Linear"), FALSE)
  description (_("Linear output"))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     connected_components
#define GEGL_OP_C_SOURCE connected-components.c

#include "gegl-op.h"

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return gegl_operation_get_bounding_box (operation);
}

static GeglRectangle
get_invalidated_by_change (GeglOperation       *operation,
                           const gchar         *input_pad,
                           const GeglRectangle *roi)
{
  return gegl_operation_get_bounding_box (operation);
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  return gegl_operation_get_bounding_box (operation);
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  gegl_operation_set_format (operation, "output",
                             o->linear ? babl_format ("Y float") :
                                         babl_format ("Y' float"));
}

static gint
get_target_index (GArray *indices,
                  gint    index)
{
  gint target = g_array_index (indices, gint32, index);

  if (target != index)
    {
      target = get_target_index (indices, target);

      g_array_index (indices, gint32, index) = target;
    }

  return target;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties     *o             = GEGL_PROPERTIES (operation);
  gboolean            invert        = o->invert;
  const Babl         *input_format  = gegl_buffer_get_format (input);
  const Babl         *output_format = gegl_buffer_get_format (output);
  gint                input_bpp     = babl_format_get_bytes_per_pixel (
                                        input_format);
  guint8             *in_row;
  gint32             *out_rows[2];
  guint8              separator[64];
  GArray             *indices;
  gint                n_indices;
  gint32              index;
  GeglBufferIterator *iter;
  gint                y;
  gint                i;

  G_STATIC_ASSERT (sizeof (gint32) == sizeof (gfloat));

  if (input_bpp > sizeof (separator))
    return FALSE;

  gegl_color_get_pixel (o->separator, input_format, separator);

  indices = g_array_new (FALSE, FALSE, sizeof (gint32));

  in_row      = g_malloc (input_bpp * roi->width);
  out_rows[0] = g_new (gint32, roi->width);
  out_rows[1] = g_new (gint32, roi->width);

  g_array_append_val (indices, (gint32) {0});

  n_indices = 1;

  for (y = 0; y < roi->height; y++)
    {
      guint8       *in   = in_row;
      const gint32 *out0 = out_rows[y       % 2];
      gint32       *out1 = out_rows[(y + 1) % 2];
      gint          x;

      gegl_buffer_get (input,
                       GEGL_RECTANGLE (roi->x, roi->y + y, roi->width, 1), 1.0,
                       input_format, in, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      for (x = 0; x < roi->width; x++)
        {
          index = 0;

          if ((! memcmp (in, separator, input_bpp)) == invert)
            {
              gint index1 = 0;
              gint index2 = 0;

              if (x > 0)
                index1 = out1[-1];

              if (y > 0)
                index2 = out0[0];

              if (index1 && index2 && index1 != index2)
                {
                  index1 = get_target_index (indices, index1);
                  index2 = get_target_index (indices, index2);

                  index = MIN (index1, index2);

                  if (index1 != index2)
                    {
                      g_array_index (indices, gint32,
                                     MAX (index1, index2)) = index;

                      n_indices--;
                    }
                }
              else
                {
                  index = MAX (index1, index2);

                  if (! index)
                    {
                      index = indices->len;

                      g_array_append_val (indices, index);

                      n_indices++;
                    }
                }
            }

          *out1 = index;

          in += input_bpp;

          out0++;
          out1++;
        }

      gegl_buffer_set (output,
                       GEGL_RECTANGLE (roi->x, roi->y + y, roi->width, 1), 0,
                       output_format, out1 - roi->width, GEGL_AUTO_ROWSTRIDE);
    }

  g_free (in_row);
  g_free (out_rows[0]);
  g_free (out_rows[1]);

  n_indices = MAX (n_indices - 1, 1);

  index = 0;

  for (i = 0; i < indices->len; i++)
    {
      gint   j = g_array_index (indices, gint32, i);
      gfloat v;

      if (j == i)
        {
          if (o->normalize)
            v = o->base + o->step * index++ / n_indices;
          else
            v = o->base + o->step * index++;
        }
      else
        {
          v = g_array_index (indices, gfloat, j);
        }

      g_array_index (indices, gfloat, i) = v;
    }

  iter = gegl_buffer_iterator_new (output, roi, 0, output_format,
                                   GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      gint32 *data = iter->items[0].data;

      for (i = 0; i < iter->length; i++)
        {
          *(gfloat *) data = g_array_index (indices, gfloat, *data);

          data++;
        }
    }

  g_array_unref (indices);

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
  operation_class->get_cached_region         = get_cached_region;
  operation_class->prepare                   = prepare;

  operation_class->threaded                  = FALSE;
  operation_class->want_in_place             = TRUE;

  filter_class->process                      = process;

  gegl_operation_class_set_keys (operation_class,
    "name",           "gegl:connected-components",
    "title",          _("Connected Components"),
    "categories",     "map",
    "description",    _("Fills each connected region of the input, separated "
                        "from the rest of the input by a given color, with a "
                        "unique color."),
    NULL);
}

#endif
