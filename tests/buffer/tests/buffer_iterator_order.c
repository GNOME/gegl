TEST ()
{
  GeglBuffer         *in_buffer  = NULL;
  GeglBuffer         *out_buffer = NULL;
  GeglBufferIterator *iter       = NULL;
  GeglRectangle       out_extent = { 0, 0, 20, 20 };
  GeglRectangle       in_extent  = { 
    0, 0, out_extent.width / 2, out_extent.height / 2
  };

  test_start();

  in_buffer  = gegl_buffer_new (&in_extent, babl_format ("Y float"));
  out_buffer = gegl_buffer_new (&out_extent, babl_format ("Y float"));

  gegl_buffer_set_color (in_buffer, &in_extent, gegl_color_new ("white"));
  gegl_buffer_set_color (out_buffer, &out_extent, gegl_color_new ("black"));

  iter = gegl_buffer_iterator_new (in_buffer, &in_extent, 0,
                                   gegl_buffer_get_format (in_buffer),
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);
  gegl_buffer_iterator_add (iter, out_buffer, &out_extent, 0,
                            gegl_buffer_get_format (out_buffer),
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *in  = iter->items[0].data;
      gfloat *out = iter->items[1].data;

      for (int i = 0; i < iter->length; i++)
        out[i] = in[i];
    }

  print_buffer (out_buffer);

  g_object_unref (in_buffer);
  g_object_unref (out_buffer);

  test_end ();
}
