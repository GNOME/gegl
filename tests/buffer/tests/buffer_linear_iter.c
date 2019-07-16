
TEST ()
{
  GeglBuffer    *buffer, *buffer2;
  gchar         *buf1, *buf2;
  GeglRectangle  bound = {2, 2, 20, 20};
  GeglRectangle  source = {5, 3, 10, 10};
  GeglBufferIterator *iter;

  test_start ();

  buf1 = g_malloc0 (bound.width * bound.height * sizeof (float));
  buf2 = g_malloc0 (source.width * source.height * sizeof (float));

  buffer = gegl_buffer_linear_new_from_data (buf1, babl_format ("Y float"),
                                             &bound, GEGL_AUTO_ROWSTRIDE,
                                             NULL, NULL);
  buffer2 = gegl_buffer_linear_new_from_data (buf2, babl_format ("Y float"),
                                              &source, GEGL_AUTO_ROWSTRIDE,
                                              NULL, NULL);

  vgrad (buffer);
  fill (buffer2, 1.0);

  iter = gegl_buffer_iterator_new (buffer2, &source, 0, NULL,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 4);

  gegl_buffer_iterator_add (iter, buffer, &source, 0, NULL,
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *s = iter->items[0].data;
      gfloat *d = iter->items[1].data;
      gint length = iter->length;

      while (length--)
        *d++ = *s++;
    }

  print_buffer (buffer);
  g_object_unref (buffer2);
  g_object_unref (buffer);
  g_free (buf1);
  g_free (buf2);
  test_end ();
}
