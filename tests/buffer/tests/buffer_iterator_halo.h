#pragma once

#include "babl/babl.h"

/*
 * Test for sub-iterators with ROIs of different sizes (not with just an
 * offset). The input buffer is read from using a (non)rectangular kernel and
 * the values are then averaged by the size. This should fill the output buffer
 * with a solid color with the exception of the borders which should be
 * "translucent" to various degree.
 */

#define KERNEL_SIZE        KERNEL_WIDTH *KERNEL_HEIGHT
#define KERNEL_WIDTH_SIDE  ((KERNEL_WIDTH - 1) / 2)
#define KERNEL_HEIGHT_SIDE ((KERNEL_HEIGHT - 1) / 2)

TEST ()
{
  const GeglRectangle out_full_roi = {
    0,
    0,
    OUT_ROI_WIDTH,
    OUT_ROI_HEIGHT
  };
  const GeglRectangle in_full_roi = {
    -IN_ROI_EXTRA_WIDTH,
    -IN_ROI_EXTRA_HEIGHT,
    out_full_roi.width + IN_ROI_EXTRA_WIDTH * 2,
    out_full_roi.height + IN_ROI_EXTRA_HEIGHT * 2,
  };

  test_start ();

  g_assert_cmpint (KERNEL_WIDTH % 2, ==, 1);
  g_assert_cmpint (KERNEL_WIDTH, <=, IN_ROI_EXTRA_WIDTH);
  g_assert_cmpint (KERNEL_HEIGHT % 2, ==, 1);
  g_assert_cmpint (KERNEL_HEIGHT, <=, IN_ROI_EXTRA_HEIGHT);

  GeglBuffer *out_buffer = gegl_buffer_new (&out_full_roi, babl_format ("Y float"));
  /* Uses out_full_roi on purpose. Relies on the abyss for the out-of-bounds halo
   * values. */
  GeglBuffer *in_buffer  = gegl_buffer_new (&out_full_roi, babl_format ("Y float"));
  gegl_buffer_set_color (in_buffer, NULL, gegl_color_new ("white"));

  GeglBufferIterator *iter = gegl_buffer_iterator_new (out_buffer, &out_full_roi, 0, NULL,
                                                       GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 2);
  gegl_buffer_iterator_add (iter, in_buffer, &in_full_roi, 0, NULL,
                            GEGL_ACCESS_READ, GEGL_ABYSS_BLACK);

  const gint components = babl_format_get_n_components (gegl_buffer_get_format (in_buffer));

  while (gegl_buffer_iterator_next (iter))
    {
      const GeglRectangle *out_roi = &iter->items[0].roi;
      const GeglRectangle *in_roi  = &iter->items[1].roi;
      const gfloat        *in      = iter->items[1].data;
      gfloat              *out     = iter->items[0].data;

      g_assert_true (gegl_rectangle_contains (in_roi,
                                              out_roi));
      g_assert_false (gegl_rectangle_equal (in_roi,
                                            out_roi));
      g_assert_true (in_roi->width - out_roi->width ==
                     in_full_roi.width - out_full_roi.width);
      g_assert_true (in_roi->height - out_roi->height ==
                     in_full_roi.height - out_full_roi.height);

      for (gint y = 0; y < out_roi->height; y++)
        {
          for (gint x = 0; x < out_roi->width; x++)
            {
#define KERNEL(DX, DY, C) in[(((IN_ROI_EXTRA_HEIGHT - KERNEL_HEIGHT_SIDE + y + DY) * in_roi->width) + (IN_ROI_EXTRA_WIDTH - KERNEL_WIDTH_SIDE + x + DX)) * components + C]
              gfloat val = 0;
              for (gint ky = 0; ky < KERNEL_HEIGHT; ky++)
                for (gint kx = 0; kx < KERNEL_WIDTH; kx++)
                  val += KERNEL (kx, ky, 0);
#undef KERNEL

              *out = val / (gfloat) (KERNEL_SIZE);
              out += components;
            }
        }
    }

  print_buffer (out_buffer);

  g_object_unref (out_buffer);
  g_object_unref (in_buffer);

  test_end ();
}
