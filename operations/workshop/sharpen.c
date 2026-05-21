#include "config.h"

#include <math.h>

#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (amount, _("Amount"), 10.0)
  description (_("Controls the amount of sharpness applied to an image"))
  value_range (1.0, 99.0)
  ui_range    (1.0, 99.0)

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     sharpen
#define GEGL_OP_C_SOURCE sharpen.c

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");

  gegl_operation_set_format (operation, "output",
                             babl_format_with_space ("R'G'B'A float", space));
}

static gboolean
process (GeglOperation       *op,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties     *o               = GEGL_PROPERTIES (op);
  const Babl         *format          = babl_format_with_space ("R'G'B'A float",
                                                                gegl_operation_get_source_space (op, "input"));
  GeglRectangle       neighbor_rect   = *roi;
  gfloat              percentage      = o->amount;
  gfloat              fact            = 1.0f - (percentage / 100.0f);
  gint                channels_num    = babl_format_get_n_components (format);
  GeglBufferIterator *iter;
  gfloat              center_weight;
  gfloat              neighbor_weight;

  /* In the legacy version, the center was (800/fact) and neighbors were
   * (pos - i*8)/8. In normalized float math, we set the center weight and
   * distribute the negative remainder among the 8 neighbors to preserve
   * overall brightness.
   */
  center_weight   = 1.0f / fact;
  neighbor_weight = (1.0f - center_weight) / 8.0f;

  iter = gegl_buffer_iterator_new (output, roi, level, format,
                                   GEGL_ACCESS_WRITE,
                                   GEGL_ABYSS_NONE, 10);

  gegl_rectangle_inset (&neighbor_rect, &neighbor_rect, -1, -1);
  gegl_buffer_iterator_add (iter, input, &neighbor_rect, level, format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_CLAMP);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *out_p = iter->items[0].data;
      gfloat *in = iter->items[1].data;

      for (gint i = 0; i < iter->length; i++)
        {
          gint c;
          gint in_offset = (iter->items[1].roi.width * (i / iter->items[0].roi.width) + i % iter->items[0].roi.width) * channels_num;

          #define K(x, y, c) in[in_offset + y * iter->items[1].roi.width * channels_num + x * channels_num + c]

          for (c = 0; c < channels_num - 1; c++)
            {
              gfloat neighbor_sum;

              neighbor_sum = K(0, 0, c) + K(1, 0, c) + K(2, 0, c) +
                             K(0, 1, c) +              K(2, 1, c) +
                             K(0, 2, c) + K(2, 2, c) + K(2, 2, c);

              out_p[c] = CLAMP ((K(1, 1, c) * center_weight) +
                                (neighbor_sum * neighbor_weight),
                                0.0f, 1.0f);
            }

          /* copy alpha channel without modification */
          out_p[3] = K(1, 1, 3);

          #undef K

          /* update to the next pixel location */
          out_p += 4;
        }
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationFilterClass *filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare        = prepare;
  filter_class->process           = process;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gegl:sharpen",
                                 "title",       _("Sharpen"),
                                 "categories",  "enhance",
                                 "description", _("Sharpens the image by enhancing "
                                                  "edge contrast using a standard "
                                                  "3x3 convolution kernel."),
                                 NULL);
}

#endif
