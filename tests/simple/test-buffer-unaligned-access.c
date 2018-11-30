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

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "gegl.h"


#define SUCCESS    0
#define FAILURE    -1


typedef GeglColor * (* FillFunc) (GeglBuffer          *buffer,
                                  const GeglRectangle *rect);


static GeglColor *
clear (GeglBuffer          *buffer,
       const GeglRectangle *rect)
{
  gegl_buffer_clear (buffer, rect);

  return gegl_color_new ("transparent");
}

static GeglColor *
set_color (GeglBuffer          *buffer,
           const GeglRectangle *rect)
{
  GeglColor *color = gegl_color_new ("red");

  gegl_buffer_set_color (buffer, rect, color);

  return color;
}

static GeglColor *
set_pattern (GeglBuffer          *buffer,
             const GeglRectangle *rect)
{
  GeglColor  *color = gegl_color_new ("green");
  GeglBuffer *pattern;

  pattern = gegl_buffer_new (GEGL_RECTANGLE (0, 0, 1, 1),
                             gegl_buffer_get_format (buffer));

  gegl_buffer_set_color (pattern, NULL, color);

  gegl_buffer_set_pattern (buffer, rect, pattern, 0, 0);

  g_object_unref (pattern);

  return color;
}

static GeglColor *
copy (GeglBuffer          *buffer,
      const GeglRectangle *rect)
{
  GeglColor  *color = gegl_color_new ("blue");
  GeglBuffer *src;

  src = gegl_buffer_new (gegl_buffer_get_extent (buffer),
                         gegl_buffer_get_format (buffer));

  gegl_buffer_set_color (src, NULL, color);

  gegl_buffer_copy (src, rect, GEGL_ABYSS_NONE, buffer, rect);

  g_object_unref (src);

  return color;
}


/* test that modifying a non-tile-grid-aligned area of a buffer using
 * fill_func() only affects the said area.
 */
static gint
test_unaligned_fill (FillFunc fill_func)
{
  gint                result = SUCCESS;
  const Babl         *format = babl_format ("RGBA float");
  gint                bpp    = babl_format_get_bytes_per_pixel (format);
  gint                tile_width, tile_height;
  GeglRectangle       rect;
  GeglBuffer         *buffer;
  GeglBufferIterator *iter;
  GeglColor          *color;
  guchar              pixel1[bpp];
  guchar              pixel2[bpp];

  buffer = gegl_buffer_new (NULL, format);

  g_object_get (buffer,
                "tile-width",  &tile_width,
                "tile-height", &tile_height,
                NULL);

  rect.x      = tile_width  / 4;
  rect.y      = tile_height / 4;
  rect.width  = tile_width  / 2;
  rect.height = tile_height / 2;

  gegl_buffer_set_extent (buffer, &rect);

  color = gegl_color_new ("white");
  gegl_buffer_set_color (buffer, NULL, color);
  gegl_color_get_pixel (color, format, pixel1);
  g_object_unref (color);

  color = fill_func (buffer, &rect);
  gegl_color_get_pixel (color, format, pixel2);
  g_object_unref (color);

  iter = gegl_buffer_iterator_new (buffer, NULL, 0, format,
                                   GEGL_BUFFER_READ, GEGL_ABYSS_NONE, 1);

  while (gegl_buffer_iterator_next (iter))
    {
      const guchar        *src = iter->items[0].data;
      const GeglRectangle *roi = &iter->items[0].roi;
      gint                 x;
      gint                 y;

      for (y = roi->y; y < roi->y + roi->height; y++)
        {
          for (x = roi->x; x < roi->x + roi->width; x++)
            {
              const guchar *pixel = pixel1;

              if (x >= rect.x && x < rect.x + rect.width &&
                  y >= rect.y && y < rect.y + rect.height)
                {
                  pixel = pixel2;
                }

              if (memcmp (src, pixel, bpp))
                result = FAILURE;

              src += bpp;
            }
        }
    }

  g_object_unref (buffer);

  return result;
}

#define RUN_TEST(test, ...) \
  do \
  { \
    printf (#test " (" #__VA_ARGS__ ")..."); \
    fflush (stdout); \
    \
    if (test_##test (__VA_ARGS__) == SUCCESS) \
      printf (" passed\n"); \
    else \
      { \
        printf (" FAILED\n"); \
        result = FAILURE; \
      } \
  } while (FALSE)

int main (int argc, char *argv[])
{
  gint result = SUCCESS;

  gegl_init (&argc, &argv);

  RUN_TEST (unaligned_fill, clear);
  RUN_TEST (unaligned_fill, set_color);
  RUN_TEST (unaligned_fill, set_pattern);
  RUN_TEST (unaligned_fill, copy);

  gegl_exit ();

  return result;
}
