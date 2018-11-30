/* This file is a test-case for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2018 Ell
 */

#include "config.h"

#include <stdio.h>

#include "gegl.h"


#define SUCCESS    0
#define FAILURE    -1

#define MAX_TEST_TIME (10 * G_TIME_SPAN_SECOND)


static GMutex   mutex;
static GCond    cond;
static gboolean finished;


static gint
test_aligned_read_write (void)
{
  GeglBuffer         *buffer1;
  GeglBuffer         *buffer2;
  GeglColor          *color;
  GeglBufferIterator *iter;
  gint                tile_width;
  gint                tile_height;

  buffer1 = gegl_buffer_new (NULL, babl_format ("RGBA float"));

  g_object_get (buffer1,
                "tile-width",  &tile_width,
                "tile-height", &tile_height,
                NULL);

  gegl_buffer_set_extent (buffer1,
                          GEGL_RECTANGLE (0, 0, tile_width, tile_height));

  color = gegl_color_new ("white");

  gegl_buffer_set_color (buffer1, NULL, color);

  g_object_unref (color);

  buffer2 = gegl_buffer_dup (buffer1);

  iter = gegl_buffer_iterator_new (buffer2, NULL, 0, NULL, GEGL_ACCESS_READ,
                                   GEGL_ABYSS_NONE, 2);
  gegl_buffer_iterator_add        (iter,
                                   buffer2, NULL, 0, NULL, GEGL_ACCESS_WRITE,
                                   GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter));

  g_object_unref (buffer2);
  g_object_unref (buffer1);

  return SUCCESS;
}

static gint
test_unaligned_read_write_read (void)
{
  GeglBuffer         *buffer1;
  GeglBuffer         *buffer2;
  GeglColor          *color;
  GeglBufferIterator *iter;
  gint                tile_width;
  gint                tile_height;

  buffer1 = gegl_buffer_new (NULL, babl_format ("RGBA float"));

  g_object_get (buffer1,
                "tile-width",  &tile_width,
                "tile-height", &tile_height,
                NULL);

  gegl_buffer_set_extent (buffer1,
                          GEGL_RECTANGLE (0,
                                          0,
                                          tile_width  + 1,
                                          tile_height + 1));

  color = gegl_color_new ("white");

  gegl_buffer_set_color (buffer1, NULL, color);

  g_object_unref (color);

  buffer2 = gegl_buffer_dup (buffer1);

  iter = gegl_buffer_iterator_new (buffer2,
                                   GEGL_RECTANGLE (0,
                                                   0,
                                                   tile_width  / 2,
                                                   tile_height / 2),
                                   0, NULL, GEGL_ACCESS_READ, GEGL_ABYSS_NONE,
                                   3);
  gegl_buffer_iterator_add        (iter,
                                   buffer2,
                                   GEGL_RECTANGLE (tile_width  / 2,
                                                   tile_height / 2,
                                                   tile_width  / 2,
                                                   tile_height / 2),
                                   0, NULL, GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);
  gegl_buffer_iterator_add        (iter,
                                   buffer2,
                                   GEGL_RECTANGLE (0,
                                                   0,
                                                   tile_width  / 2,
                                                   tile_height / 2),
                                   0, NULL, GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter));

  g_object_unref (buffer2);
  g_object_unref (buffer1);

  return SUCCESS;
}

static gpointer
run_test_thread_func (gint (* func) (void))
{
  gint result;

  result = func ();

  g_mutex_lock (&mutex);

  finished = TRUE;
  g_cond_signal (&cond);

  g_mutex_unlock (&mutex);

  return GINT_TO_POINTER (result);
}

static gint
run_test (const gchar   *name,
          gint        (* func) (void))
{
  GThread *thread;
  gint64   end_time;
  gint     result = SUCCESS;

  printf ("%s ... ", name);
  fflush (stdout);

  g_mutex_lock (&mutex);

  finished = FALSE;

  thread = g_thread_new ("test", (GThreadFunc) run_test_thread_func, func);

  end_time = g_get_monotonic_time () + MAX_TEST_TIME;

  while (! finished)
    {
      if (! g_cond_wait_until (&cond, &mutex, end_time))
        {
          result = FAILURE;

          break;
        }
    }

  if (result == SUCCESS)
    result = GPOINTER_TO_INT (g_thread_join (thread));

  g_mutex_unlock (&mutex);

  printf ("%s\n", result == SUCCESS ? "pass" : "FAIL");

  return result;
}

#define RUN_TEST(test)                          \
  G_STMT_START                                  \
    {                                           \
      if (result == SUCCESS)                    \
        result = run_test (#test, test_##test); \
    }                                           \
  G_STMT_END

gint
main (gint    argc,
      gchar **argv)
{
  gint result = SUCCESS;

  gegl_init (&argc, &argv);

  RUN_TEST (aligned_read_write);
  RUN_TEST (unaligned_read_write_read);

  if (result == SUCCESS)
    gegl_exit ();

  return result;
}
