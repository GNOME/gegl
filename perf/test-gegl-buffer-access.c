#include "test-common.h"

#define BPP 16

gint
main (gint    argc,
      gchar **argv)
{
  GeglBuffer    *buffer;
  GeglRectangle  bound = {0, 0, 1024, 1024};
  GeglRectangle  bound2 = {0, 0, 300, 300};
  const Babl *format;
  guchar *sbuf;
  guchar *buf;
  gint i;

  gegl_init (NULL, NULL);
  format = babl_format ("RGBA float");
  sbuf = g_malloc0 (bound.width * bound.height * BPP);
  buf = g_malloc0 (bound.width * bound.height * BPP);

  for (i = 0; i < bound.width * bound.height * BPP;i++)
    sbuf[i] = rand() & 0xff;

  buffer = gegl_buffer_new (&bound, format);

  /* pre-initialize */
  gegl_buffer_set (buffer, &bound, 0, NULL, sbuf, GEGL_AUTO_ROWSTRIDE);
#if 1
  test_start ();
  for (i=0;i<ITERATIONS && converged < BAIL_COUNT;i++)
    {
      test_start_iter ();
      gegl_buffer_get (buffer, &bound, 1.0, NULL, buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
      test_end_iter ();
     }
  test_end ("gegl_buffer_get", 1.0 * bound.width * bound.height * ITERATIONS * BPP);

  test_start ();
  for (i=0;i<ITERATIONS && converged < BAIL_COUNT;i++)
    {
      GeglBuffer *buffer = gegl_buffer_new (&bound, format);
  /* pre-initialize */
      gegl_buffer_set (buffer, &bound, 0, NULL, sbuf, GEGL_AUTO_ROWSTRIDE);
      test_start_iter ();
      gegl_buffer_get (buffer, &bound, 0.333, NULL, buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
      test_end_iter ();
      g_object_unref (buffer);
     }
  test_end ("gegl_buffer_get 0.333", 1.0 * bound.width * bound.height * ITERATIONS * BPP);
#endif
  {

  test_start ();
  for (i=0;i<ITERATIONS && converged < BAIL_COUNT;i++)
    {
      const Babl *format = babl_format ("R'G'B'A u8");
      GeglBuffer *buffer = gegl_buffer_new (&bound, format);
      gegl_buffer_set (buffer, &bound, 0, NULL, sbuf, GEGL_AUTO_ROWSTRIDE);
  /* pre-initialize */
      test_start_iter ();
      gegl_buffer_get (buffer, &bound2, 0.333, NULL, buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
      test_end_iter ();
      g_object_unref (buffer);
     }
  }
  test_end ("buffer_get 8bit 0.333", 1.0 * bound2.width * bound2.height * ITERATIONS * 4);

  {

      const Babl *format = babl_format ("R'G'B'A u8");
      GeglBuffer *buffer = gegl_buffer_new (&bound, format);
      gegl_buffer_set (buffer, &bound, 0, NULL, sbuf, GEGL_AUTO_ROWSTRIDE);
  test_start ();
  for (i=0;i<ITERATIONS && converged < BAIL_COUNT;i++)
    {
  /* pre-initialize */
      test_start_iter ();
      gegl_buffer_get (buffer, &bound2, 0.333, NULL, buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE|GEGL_BUFFER_FILTER_BOX);
      test_end_iter ();
     }
      g_object_unref (buffer);
  }
  test_end ("box 0.333", 1.0 * bound2.width * bound2.height * ITERATIONS * 4);


  {
      const Babl *format = babl_format ("R'G'B'A u8");
      GeglBuffer *buffer = gegl_buffer_new (&bound, format);
      gegl_buffer_set (buffer, &bound, 0, NULL, sbuf, GEGL_AUTO_ROWSTRIDE);
  test_start ();
  for (i=0;i<ITERATIONS && converged < BAIL_COUNT;i++)
    {
  /* pre-initialize */
      test_start_iter ();
      gegl_buffer_get (buffer, &bound2, 0.333, NULL, buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE|GEGL_BUFFER_FILTER_NEAREST);
      test_end_iter ();
     }
      g_object_unref (buffer);
  }
  test_end ("nearest 0.333", 1.0 * bound2.width * bound2.height * ITERATIONS * 4);



  {
      const Babl *format = babl_format ("R'G'B'A u8");
      GeglBuffer *buffer = gegl_buffer_new (&bound, format);
      gegl_buffer_set (buffer, &bound, 0, NULL, sbuf, GEGL_AUTO_ROWSTRIDE);
  test_start ();
  for (i=0;i<ITERATIONS && converged < BAIL_COUNT;i++)
    {
  /* pre-initialize */
      test_start_iter ();
      gegl_buffer_get (buffer, &bound2, 0.333, NULL, buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE|GEGL_BUFFER_FILTER_BILINEAR);
      test_end_iter ();
     }
      g_object_unref (buffer);
  }
  test_end ("bilinear 0.333", 1.0 * bound2.width * bound2.height * ITERATIONS * 4);

  test_start ();
  for (i=0;i<ITERATIONS && converged < BAIL_COUNT;i++)
    {
      test_start_iter ();
      gegl_buffer_set (buffer, &bound, 0, NULL, sbuf, GEGL_AUTO_ROWSTRIDE);
      test_end_iter ();
     }
  test_end ("gegl_buffer_set", 1.0 * bound.width * bound.height * ITERATIONS * BPP);


  format = babl_format ("RGBA float");


  {
#define SAMPLES 150000
    gint rands[SAMPLES*2];

  for (i = 0; i < SAMPLES; i ++)
  {
    rands[i*2]   = rand()%1000;
    rands[i*2+1] = rand()%1000;
    rands[i*2]   = i / 1000;
    rands[i*2+1] = i % 1000;
  }

  test_start ();
  for (i=0;i<ITERATIONS && converged < BAIL_COUNT;i++)
  {
    int j;
    test_start_iter ();
    for (j = 0; j < SAMPLES; j ++)
    {
      int x = rands[j*2];
      int y = rands[j*2+1];
      float px[4] = {0.2, 0.4, 0.1, 0.5};
      GeglRectangle rect = {x, y, 1, 1};
      gegl_buffer_set (buffer, &rect, 0, format, (void*)&px[0], GEGL_AUTO_ROWSTRIDE);
    }
    test_end_iter ();
  }
  test_end ("gegl_buffer_set 1x1", 1.0 * SAMPLES * ITERATIONS * BPP);

  test_start ();
  for (i=0;i<ITERATIONS && converged < BAIL_COUNT;i++)
  {
    int j;
    float px[4] = {0.2, 0.4, 0.1, 0.5};
    test_start_iter ();
    for (j = 0; j < SAMPLES; j ++)
    {
      int x = rands[j*2];
      int y = rands[j*2+1];
      GeglRectangle rect = {x, y, 1, 1};
      gegl_buffer_get (buffer, &rect, 1.0, format, (void*)&px[0],
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
    }
    test_end_iter ();
  }
  test_end ("gegl_buffer_get 1x1", 1.0 * SAMPLES * ITERATIONS * BPP);

  test_start ();
  for (i=0;i<ITERATIONS && converged < BAIL_COUNT;i++)
  {
    int j;
    float px[4] = {0.2, 0.4, 0.1, 0.5};
    test_start_iter ();
    for (j = 0; j < SAMPLES; j ++)
    {
      int x = rands[j*2];
      int y = rands[j*2+1];
      gegl_buffer_sample (buffer, x, y, NULL, (void*)&px[0], format,
                          GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
    }
    test_end_iter ();
  }
  test_end ("gegl_buffer_sample nearest", 1.0 * SAMPLES * ITERATIONS * BPP);

  }

  g_free (buf);
  g_object_unref (buffer);

  return 0;
}
