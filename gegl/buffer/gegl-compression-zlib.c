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

#include "gegl-compression.h"
#include "gegl-compression-zlib.h"


#ifdef HAVE_ZLIB


#include <zlib.h>


typedef struct
{
  GeglCompression compression;
  gint            level;
} GeglCompressionZlib;


/*  local function prototypes  */

static gboolean   gegl_compression_zlib_compress   (const GeglCompression *compression,
                                                    const Babl            *format,
                                                    gconstpointer          data,
                                                    gint                   n,
                                                    gpointer               compressed,
                                                    gint                  *compressed_size,
                                                    gint                   max_compressed_size);
static gboolean   gegl_compression_zlib_decompress (const GeglCompression *compression,
                                                    const Babl            *format,
                                                    gpointer               data,
                                                    gint                   n,
                                                    gconstpointer          compressed,
                                                    gint                   compressed_size);

static voidpf     gegl_compression_zlib_alloc      (voidpf                 opaque,
                                                    uInt                   items,
                                                    uInt                   size);
static void       gegl_compression_zlib_free       (voidpf                 opaque,
                                                    voidpf                 address);



/*  private functions  */

static gboolean
gegl_compression_zlib_compress (const GeglCompression *compression,
                                const Babl            *format,
                                gconstpointer          data,
                                gint                   n,
                                gpointer               compressed,
                                gint                  *compressed_size,
                                gint                   max_compressed_size)
{
  const GeglCompressionZlib *compression_zlib;
  z_stream                   stream;
  gint                       size;

  compression_zlib = (const GeglCompressionZlib *) compression;

  stream.zalloc = gegl_compression_zlib_alloc;
  stream.zfree  = gegl_compression_zlib_free;
  stream.opaque = NULL;

  if (deflateInit (&stream, compression_zlib->level) != Z_OK)
    return FALSE;

  size = n * babl_format_get_bytes_per_pixel (format);

  stream.next_in   = (gpointer) data;
  stream.avail_in  = size;
  stream.next_out  = compressed;
  stream.avail_out = max_compressed_size;

  if (deflate (&stream, Z_FINISH) != Z_STREAM_END)
    {
      deflateEnd (&stream);

      return FALSE;
    }

  *compressed_size = stream.total_out;

  deflateEnd (&stream);

  return TRUE;
}

static gboolean
gegl_compression_zlib_decompress (const GeglCompression *compression,
                                  const Babl            *format,
                                  gpointer               data,
                                  gint                   n,
                                  gconstpointer          compressed,
                                  gint                   compressed_size)
{
  z_stream stream;
  gint     size;

  stream.zalloc = gegl_compression_zlib_alloc;
  stream.zfree  = gegl_compression_zlib_free;
  stream.opaque = NULL;

  if (inflateInit (&stream) != Z_OK)
    return FALSE;

  size = n * babl_format_get_bytes_per_pixel (format);

  stream.next_in   = (gpointer) compressed;
  stream.avail_in  = compressed_size;
  stream.next_out  = data;
  stream.avail_out = size;

  if (inflate (&stream, Z_FINISH) != Z_STREAM_END ||
      stream.total_out != size)
    {
      inflateEnd (&stream);

      return FALSE;
    }

  inflateEnd (&stream);

  return TRUE;
}

static voidpf
gegl_compression_zlib_alloc (voidpf opaque,
                             uInt   items,
                             uInt   size)
{
  return g_try_malloc_n (items, size);
}

static void
gegl_compression_zlib_free (voidpf opaque,
                            voidpf address)
{
  g_free (address);
}


/*  public functions  */

void
gegl_compression_zlib_init (void)
{
  #define COMPRESSION_ZLIB(name, zlib_level)                \
    G_STMT_START                                            \
      {                                                     \
        static const GeglCompressionZlib compression_zlib = \
        {                                                   \
          .compression =                                    \
          {                                                 \
            .compress   = gegl_compression_zlib_compress,   \
            .decompress = gegl_compression_zlib_decompress  \
          },                                                \
          .level = (zlib_level)                             \
        };                                                  \
                                                            \
        gegl_compression_register (                         \
          name,                                             \
          (const GeglCompression *) &compression_zlib);     \
      }                                                     \
    G_STMT_END

  COMPRESSION_ZLIB ("zlib",  Z_DEFAULT_COMPRESSION);
  COMPRESSION_ZLIB ("zlib1", 1);
  COMPRESSION_ZLIB ("zlib2", 2);
  COMPRESSION_ZLIB ("zlib3", 3);
  COMPRESSION_ZLIB ("zlib4", 4);
  COMPRESSION_ZLIB ("zlib5", 5);
  COMPRESSION_ZLIB ("zlib6", 6);
  COMPRESSION_ZLIB ("zlib7", 7);
  COMPRESSION_ZLIB ("zlib8", 8);
  COMPRESSION_ZLIB ("zlib9", 9);
}


#else /* ! HAVE_ZLIB */


/*  public functions  */

void
gegl_compression_zlib_init (void)
{
}


#endif /* ! HAVE_ZLIB */
