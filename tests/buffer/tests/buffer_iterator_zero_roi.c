TEST ()
{
  GeglRectangle extent = { 0, 0, 20, 20 };

  test_start ();

  GeglBuffer *in_buffer  = gegl_buffer_new (&extent, babl_format ("Y float"));
  GeglBuffer *out_buffer = gegl_buffer_new (&extent, babl_format ("Y float"));

  gegl_buffer_set_color (in_buffer, &extent, gegl_color_new ("white"));
  gegl_buffer_set_color (out_buffer, &extent, gegl_color_new ("black"));

  GeglBufferIterator *iter = gegl_buffer_iterator_new (in_buffer, &extent, 0,
                                                       gegl_buffer_get_format (in_buffer),
                                                       GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 3);
  gegl_buffer_iterator_add (iter, in_buffer, GEGL_RECTANGLE (0, 0, 0, 0), 0,
                            gegl_buffer_get_format (in_buffer),
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
  gegl_buffer_iterator_add (iter, out_buffer, &extent, 0,
                            gegl_buffer_get_format (out_buffer),
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      g_assert_cmpint (iter->items[0].roi.width, ==, iter->items[1].roi.width);
      g_assert_cmpint (iter->items[0].roi.height, ==, iter->items[1].roi.height);

      gfloat *in  = iter->items[1].data;
      gfloat *out = iter->items[2].data;

      for (int i = 0; i < iter->length; i++)
        out[i] = in[i];
    }

  print_buffer (out_buffer);

  g_object_unref (in_buffer);
  g_object_unref (out_buffer);

  test_end ();
}
