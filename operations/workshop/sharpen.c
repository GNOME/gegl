#include "config.h"

#include <math.h>

#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (amount, _("Amount"), 10.0)
  description (_("Controls the amount of sharpness applied to an image"))
  value_range (1.0, 99.0)
  ui_range    (1.0, 99.0)

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     sharpen
#define GEGL_OP_C_SOURCE sharpen.c

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");

  gegl_operation_set_format (operation, "input",
                             babl_format_with_space ("R'G'B'A float", space));
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
  GeglProperties     *o            = GEGL_PROPERTIES (op);
  const Babl         *format       = babl_format_with_space ("R'G'B'A float",
                                                             gegl_operation_get_source_space (op, "input"));
  
  gfloat              percentage   = o->amount;
  gfloat              fact         = 1.0f - (percentage / 100.0f);
  gint                channels_num = 4;
  
  /* Clamping fact to avoid division by zero */ 
  if (fact < 0.01f) fact = 0.01f;

  /* In the legacy version, the center was (800/fact) and neighbors were (pos - i*8)/8.
   * In normalized float math, we set the center weight and distribute the 
   * negative remainder among the 8 neighbors to preserve overall brightness.
   */
  gfloat center_weight = 1.0f / fact;
  gfloat neighbour_weight   = (1.0f - center_weight) / 8.0f;

  GeglBufferIterator *iter;
  iter = gegl_buffer_iterator_new (output, roi, level, format,
                                   GEGL_ACCESS_WRITE,
                                   GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      GeglRectangle  out_roi = iter->items[0].roi;
      gfloat        *output_data = iter->items[0].data;

      /* Expand ROI by 1 pixel in all directions to get the neighbors */ 
      GeglRectangle input_roi;
      input_roi.x      = out_roi.x - 1;
      input_roi.y      = out_roi.y - 1;
      input_roi.width  = out_roi.width  + 2;
      input_roi.height = out_roi.height + 2;

      gint    input_width = input_roi.width;
      gfloat *input_buf   = g_new (gfloat, input_roi.width * input_roi.height * channels_num);

      gegl_buffer_get (input, &input_roi, 1.0, format, input_buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

      for (gint y = 0; y < out_roi.height; y++)
        {
          for (gint x = 0; x < out_roi.width; x++)
            {
              /* since input_buf starts at (out_roi.x - 1), our local 
               * center in the buffer is at (x + 1, y + 1) */
              gint in_x = x + 1;
              gint in_y = y + 1;

              for (gint c = 0; c < 3; c++) /* Process R, G, B */
                {
                  gfloat res = 0.0f;

                  /* 3x3 convolution loop */ 
                  for (gint ky = -1; ky <= 1; ky++)
                    {
                      for (gint kx = -1; kx <= 1; kx++)
                        {
                          gint in_idx = ((in_y + ky) * input_width + (in_x + kx)) * channels_num + c;
                          gfloat pixel_val = input_buf[in_idx];

                          if (kx == 0 && ky == 0)
                            res += pixel_val * center_weight; /* for center pixel */
                          else
                            res += pixel_val * neighbour_weight; /* for neighbours */
                        }
                    }
                  
                  gint out_idx = (y * out_roi.width + x) * channels_num + c;
                  output_data[out_idx] = CLAMP (res, 0.0f, 1.0f);
                }

              /* copy Alpha channel without modification */ 
              gint alpha_in  = (in_y * input_width + in_x) * channels_num + 3;
              gint alpha_out = (y * out_roi.width + x) * channels_num + 3;
              output_data[alpha_out] = input_buf[alpha_in];
            }
        }
      g_free (input_buf);
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationFilterClass *filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare        = prepare;
  operation_class->opencl_support = FALSE;
  filter_class->process           = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:sharpen",
    "title",       _("Sharpen (Legacy)"),
    "categories",  "enhance",
    "description", _("A port of the legacy GIMP sharpen filter to GEGL float math."),
    NULL);
}

#endif