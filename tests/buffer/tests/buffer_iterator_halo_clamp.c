#include "babl/babl.h"

#define KERNEL_WIDTH  3
#define KERNEL_HEIGHT 3
#define KERNEL_SIZE   KERNEL_WIDTH * KERNEL_HEIGHT

TEST ()
{
  GeglBuffer         *out_buffer = NULL;
  GeglBuffer         *in_buffer  = NULL;
  GeglBufferIterator *iter       = NULL;
  GeglRectangle       out_roi    = { 0, 0, 20, 20 };
  GeglRectangle       in_roi     = {-1, -1, 22, 22 };
  gint                components = 0;

  test_start();

  out_buffer = gegl_buffer_new (&out_roi, babl_format ("Y float"));
  in_buffer  = gegl_buffer_new (&out_roi, babl_format ("Y float"));
  gegl_buffer_set_color (in_buffer, NULL, gegl_color_new ("white"));

  iter = gegl_buffer_iterator_new (out_buffer, &out_roi, 0, NULL,
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 2);
  gegl_buffer_iterator_add (iter, in_buffer, &in_roi, 0, NULL,
                            GEGL_ACCESS_READ, GEGL_ABYSS_CLAMP);

  components = babl_format_get_n_components(gegl_buffer_get_format (in_buffer));

  while (gegl_buffer_iterator_next (iter))
    {
      g_assert_true (gegl_rectangle_contains (&iter->items[1].roi,
                                              &iter->items[0].roi));
      g_assert_false (gegl_rectangle_equal (&iter->items[1].roi,
                                            &iter->items[0].roi));
      g_assert_true (iter->items[0].roi.width  == out_roi.width);
      g_assert_true (iter->items[0].roi.height == out_roi.height);
      g_assert_true (iter->items[1].roi.width  == in_roi.width);
      g_assert_true (iter->items[1].roi.height == in_roi.height);

      gfloat *out                 = iter->items[0].data;
      gfloat *in                  = iter->items[1].data;
      gfloat  kernel[KERNEL_SIZE] = { 0 };

      for (gint i = 0; i < iter->length; i++)
        {
          gfloat val       = 0;
          gint   in_offset = iter->items[1].roi.width * (i / iter->items[0].roi.width) + i % iter->items[0].roi.width;

          for (int y = 0; y < KERNEL_HEIGHT; y++)
            {
              for (int x = 0; x < KERNEL_WIDTH; x++)
                {
                  kernel[y * KERNEL_WIDTH + x] = in[in_offset                                 +
                                                    y * iter->items[1].roi.width * components +
                                                    x * components];
                }
            }

          for (gint j = 0; j < KERNEL_SIZE; j++)
            val += kernel[j];

          out[i] = val / (gfloat)(KERNEL_SIZE);
        }
    }

  print_buffer (out_buffer);

  g_object_unref (out_buffer);
  g_object_unref (in_buffer);

  test_end ();
}
