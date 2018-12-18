/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2018 Ell
 */

#include "config.h"

#include <stdio.h>
#include <string.h>

#include "gegl.h"
#include "buffer/gegl-compression.h"

#define SUCCESS  0
#define FAILURE -1

static gpointer
load_png (const gchar *path,
          const Babl  *format,
          gint        *n)
{
  GeglNode   *node;
  GeglNode   *node_source;
  GeglNode   *node_sink;
  GeglBuffer *buffer = NULL;
  gpointer    data;

  node = gegl_node_new ();

  node_source = gegl_node_new_child (node,
                                     "operation", "gegl:load",
                                     "path",      path,
                                     NULL);
  node_sink   = gegl_node_new_child (node,
                                     "operation", "gegl:buffer-sink",
                                     "buffer",    &buffer,
                                     NULL);

  gegl_node_link (node_source, node_sink);

  gegl_node_process (node_sink);

  g_object_unref (node);

  *n   = gegl_buffer_get_width (buffer) * gegl_buffer_get_height (buffer);
  data = g_malloc (*n * babl_format_get_bytes_per_pixel (format));

  gegl_buffer_get (buffer, NULL, 1.0, format, data,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  g_object_unref (buffer);

  return data;
}

gint
main (gint    argc,
      gchar **argv)
{
  const Babl   *format;
  gint          bpp;
  gchar        *path;
  gpointer      data;
  gint          n;
  gint          size;
  guint8       *compressed;
  gint          max_compressed_size;
  guint8       *decompressed;
  const gchar   signature[] = "test-gegl-compression";
  const gchar **algorithms;
  gint          i;
  gint          result = SUCCESS;

  gegl_init (&argc, &argv);

  format = babl_format ("R'G'B'A u8");
  bpp    = babl_format_get_bytes_per_pixel (format);

  path = g_build_filename (g_getenv ("ABS_TOP_SRCDIR"),
                           "tests", "compositions", "data", "car-stack.png",
                           NULL);

  data = load_png (path, format, &n);
  size = n * bpp;

  g_free (path);

  max_compressed_size = 2 * n * bpp;
  compressed          = g_malloc (max_compressed_size + sizeof (signature));
  decompressed        = g_malloc (size);

  algorithms = gegl_compression_list ();

  for (i = 0; algorithms[i]; i++)
    {
      const GeglCompression *compression = gegl_compression (algorithms[i]);
      gint                   compressed_size;
      gint                   trunc_size;

      printf ("%s: ", algorithms[i]);
      fflush (stdout);

      memset (compressed,   0, max_compressed_size);
      memset (decompressed, 0, size);

      if (! gegl_compression_compress (compression, format,
                                       data, n,
                                       compressed, &compressed_size,
                                       max_compressed_size))
        {
          goto fail;
        }

      if (! gegl_compression_decompress (compression, format,
                                         decompressed, n,
                                         compressed, compressed_size))
        {
          goto fail;
        }

      if (memcmp (data, decompressed, size))
        goto fail;

      printf ("pass (%d%%)\n", (100 * compressed_size + size / 2) / size);

      printf ("%s (trunc.): ", algorithms[i]);
      fflush (stdout);

      trunc_size = compressed_size / 2;

      memcpy (compressed + trunc_size, signature, sizeof (signature));

      if (gegl_compression_compress (compression, format,
                                     data, n,
                                     compressed, &compressed_size,
                                     trunc_size))
        {
          goto fail;
        }

      if (memcmp (compressed + trunc_size, signature, sizeof (signature)))
        goto fail;

      printf ("pass\n");

      continue;

fail:
      printf ("FAIL\n");

      result = FAILURE;
    }

  g_free (algorithms);

  g_free (compressed);
  g_free (decompressed);

  g_free (data);

  gegl_exit ();

  return result;
}
