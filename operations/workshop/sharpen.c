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

#define KERNEL_SIZE 1

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  const Babl *space  = gegl_operation_get_source_space (operation, "input");
  const Babl *format = babl_format_with_space ("R'G'B'A float", space);

  gegl_operation_set_format (operation, "output", format);
  gegl_operation_set_format (operation, "input", format);
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle ret = *roi;
  ret.x -= KERNEL_SIZE;
  ret.y -= KERNEL_SIZE;
  ret.width  += KERNEL_SIZE * 2;
  ret.height += KERNEL_SIZE * 2;
  return ret;
}

static gboolean
process (GeglOperation       *op,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties     *o          = GEGL_PROPERTIES (op);
  // The input and output pads use the same format.
  const Babl         *format  = gegl_operation_get_format (op, "input");
  const GeglRectangle in_roi     = {
    roi->x - KERNEL_SIZE,
    roi->y - KERNEL_SIZE,
    roi->width  + KERNEL_SIZE * 2,
    roi->height + KERNEL_SIZE * 2,
  };
  const gint          channels_num = babl_format_get_n_components (format);
  gfloat              percentage   = o->amount;
  gfloat              fact         = 1.0f - (percentage / 100.0f);

  /* In the legacy version, the center was (800/fact) and neighbors were
   * (pos - i*8)/8. In normalized float math, we set the center weight and
   * distribute the negative remainder among the 8 neighbors to preserve
   * overall brightness.
   */
  gfloat center_weight   = 1.0f / fact;
  gfloat neighbor_weight = (1.0f - center_weight) / 8.0f;

  GeglBufferIterator *iter = gegl_buffer_iterator_new (output, roi, level, format,
                                                       GEGL_ACCESS_WRITE,
                                                       GEGL_ABYSS_NONE, 10);
  gegl_buffer_iterator_add (iter, input, &in_roi, level, format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_CLAMP);

  while (gegl_buffer_iterator_next (iter))
    {
      const GeglRectangle *out_roi = &iter->items[0].roi;
      const GeglRectangle *in_roi  = &iter->items[1].roi;
      const gfloat        *in      = iter->items[1].data;
      gfloat              *out     = iter->items[0].data;

      for (gint y = 0; y < out_roi->height; y++)
        {
          for (gint x = 0; x < out_roi->width; x++)
            {
#define KERNEL(dx, dy, c) in[(((KERNEL_SIZE + y + dy) * in_roi->width) + (KERNEL_SIZE + x + dx)) * channels_num + c]
              for (gint c = 0; c < channels_num - 1; c++)
                {
                  gfloat neighbor_sum;

                  neighbor_sum = KERNEL(-1, -1, c) + KERNEL(0, -1, c) + KERNEL(1, -1, c) +
                                 KERNEL(-1,  0, c) +                    KERNEL(1,  0, c) +
                                 KERNEL(-1,  1, c) + KERNEL(0 , 1, c) + KERNEL(1,  1, c);

                  out[c] = CLAMP ((KERNEL(0, 0, c) * center_weight) +
                                  (neighbor_sum * neighbor_weight),
                                  0.0f, 1.0f);
                }

              /* copy alpha channel without modification */
              out[3] = KERNEL(0, 0, 3);
#undef KERNEL
              /* update to the next pixel location */
              out += channels_num;
            }
        }
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationFilterClass *filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->get_required_for_output   = get_required_for_output;
  operation_class->get_invalidated_by_change = get_required_for_output;
  operation_class->prepare        = prepare;
  filter_class->process           = process;

  gegl_operation_class_set_keys (operation_class,
                                 "name",            "gegl:sharpen",
                                 "title",           _("Sharpen"),
                                 "categories",      "enhance",
                                 "description",     _("Sharpens the image by enhancing "
                                                      "edge contrast using a standard "
                                                      "3x3 convolution kernel."),
                                 "reference-hash",  "3a12de8755d7385a0e89bbf6a4258cfe",
                                 "gimp:menu-path",  "<Image>/Filters/Enhance",
                                 "gimp:menu-label", _("Sharpen"),
                                 NULL);
}
#endif

