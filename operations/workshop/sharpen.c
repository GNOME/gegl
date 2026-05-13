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

  /* Clamping fact to avoid division by zero */
  if (fact < 0.01f)
    fact = 0.01f;

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

  /* top left */
  neighbor_rect.x = roi->x - 1;
  neighbor_rect.y = roi->y - 1;
  gegl_buffer_iterator_add (iter, input, &neighbor_rect, level, format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_CLAMP);

  /* top center */
  neighbor_rect.x = roi->x;
  neighbor_rect.y = roi->y - 1;
  gegl_buffer_iterator_add (iter, input, &neighbor_rect, level, format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_CLAMP);

  /* top right */
  neighbor_rect.x = roi->x + 1;
  neighbor_rect.y = roi->y - 1;
  gegl_buffer_iterator_add (iter, input, &neighbor_rect, level, format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_CLAMP);

  /* center left */
  neighbor_rect.x = roi->x - 1;
  neighbor_rect.y = roi->y;
  gegl_buffer_iterator_add (iter, input, &neighbor_rect, level, format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_CLAMP);

  /* center */
  gegl_buffer_iterator_add (iter, input, roi, level, format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_CLAMP);

  /* center right */
  neighbor_rect.x = roi->x + 1;
  neighbor_rect.y = roi->y;
  gegl_buffer_iterator_add (iter, input, &neighbor_rect, level, format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_CLAMP);

  /* bottom left */
  neighbor_rect.x = roi->x - 1;
  neighbor_rect.y = roi->y + 1;
  gegl_buffer_iterator_add (iter, input, &neighbor_rect, level, format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_CLAMP);

  /* bottom center */
  neighbor_rect.x = roi->x;
  neighbor_rect.y = roi->y + 1;
  gegl_buffer_iterator_add (iter, input, &neighbor_rect, level, format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_CLAMP);

  /* bottom right */
  neighbor_rect.x = roi->x + 1;
  neighbor_rect.y = roi->y + 1;
  gegl_buffer_iterator_add (iter, input, &neighbor_rect, level, format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_CLAMP);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *out_p = iter->items[0].data;
      gfloat *tl    = iter->items[1].data;
      gfloat *tc    = iter->items[2].data;
      gfloat *tr    = iter->items[3].data;
      gfloat *cl    = iter->items[4].data;
      gfloat *cp    = iter->items[5].data;
      gfloat *cr    = iter->items[6].data;
      gfloat *bl    = iter->items[7].data;
      gfloat *bc    = iter->items[8].data;
      gfloat *br    = iter->items[9].data;
      gint    i;

      for (i = 0; i < iter->length; i++)
        {
          gint c;

          for (c = 0; c < channels_num - 1; c++)
            {
              gfloat neighbor_sum;

              neighbor_sum = tl[c] + tc[c] + tr[c] +
                             cl[c] +         cr[c] +
                             bl[c] + bc[c] + br[c];

              out_p[c] = CLAMP ((cp[c] * center_weight) +
                                (neighbor_sum * neighbor_weight),
                                0.0f, 1.0f);
            }

          /* copy alpha channel without modification */
          out_p[3] = cp[3];

          /* update to the next pixel location */
          out_p += 4;
          tl    += 4;
          tc    += 4;
          tr    += 4;
          cl    += 4;
          cp    += 4;
          cr    += 4;
          bl    += 4;
          bc    += 4;
          br    += 4;
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