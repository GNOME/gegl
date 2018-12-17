/* This file is part of GEGL
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
 */

#include "config.h"

#include <string.h>

#include "gegl-compression.h"
#include "gegl-compression-nop.h"


/*  local function prototypes  */

static gboolean   gegl_compression_nop_compress   (const GeglCompression *compression,
                                                   const Babl            *format,
                                                   gconstpointer          data,
                                                   gint                   n,
                                                   gpointer               compressed,
                                                   gint                  *compressed_size,
                                                   gint                   max_compressed_size);
static gboolean   gegl_compression_nop_decompress (const GeglCompression *compression,
                                                   const Babl            *format,
                                                   gpointer               data,
                                                   gint                   n,
                                                   gconstpointer          compressed,
                                                   gint                   compressed_size);


/*  private functions  */

static gboolean
gegl_compression_nop_compress (const GeglCompression *compression,
                               const Babl            *format,
                               gconstpointer          data,
                               gint                   n,
                               gpointer               compressed,
                               gint                  *compressed_size,
                               gint                   max_compressed_size)
{
  gint size = n * babl_format_get_bytes_per_pixel (format);

  if (max_compressed_size < size)
    return FALSE;

  memcpy (compressed, data, size);

  *compressed_size = size;

  return TRUE;
}

static gboolean
gegl_compression_nop_decompress (const GeglCompression *compression,
                                 const Babl            *format,
                                 gpointer               data,
                                 gint                   n,
                                 gconstpointer          compressed,
                                 gint                   compressed_size)
{
  gint size = n * babl_format_get_bytes_per_pixel (format);

  if (compressed_size != size)
    return FALSE;

  memcpy (data, compressed, size);

  return TRUE;
}


/*  public functions  */

void
gegl_compression_nop_init (void)
{
  static const GeglCompression compression_nop =
  {
    .compress   = gegl_compression_nop_compress,
    .decompress = gegl_compression_nop_decompress
  };

  gegl_compression_register ("nop", &compression_nop);
}
