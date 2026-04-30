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

#ifndef __GEGL_COMPRESSION_H__
#define __GEGL_COMPRESSION_H__


#include <glib.h>
#include <babl/babl.h>

G_BEGIN_DECLS

typedef struct _GeglCompression GeglCompression;

typedef gboolean (* GeglCompressionCompressFunc)   (const GeglCompression *compression,
                                                    const Babl            *format,
                                                    gconstpointer          data,
                                                    gint                   n,
                                                    gpointer               compressed,
                                                    gint                  *compressed_size,
                                                    gint                   max_compressed_size);
typedef gboolean (* GeglCompressionDecompressFunc) (const GeglCompression *compression,
                                                    const Babl            *format,
                                                    gpointer               data,
                                                    gint                   n,
                                                    gconstpointer          compressed,
                                                    gint                   compressed_size);

struct _GeglCompression
{
  GeglCompressionCompressFunc   compress;
  GeglCompressionDecompressFunc decompress;
};

void                     gegl_compression_init       (void);
void                     gegl_compression_cleanup    (void);

void                     gegl_compression_register   (const gchar           *name,
                                                      const GeglCompression *compression);
const gchar           ** gegl_compression_list       (void);

const GeglCompression  * gegl_compression            (const gchar           *name);

gboolean                 gegl_compression_compress   (const GeglCompression *compression,
                                                      const Babl            *format,
                                                      gconstpointer          data,
                                                      gint                   n,
                                                      gpointer               compressed,
                                                      gint                  *compressed_size,
                                                      gint                   max_compressed_size);
gboolean                 gegl_compression_decompress (const GeglCompression *compression,
                                                      const Babl            *format,
                                                      gpointer               data,
                                                      gint                   n,
                                                      gconstpointer          compressed,
                                                      gint                   compressed_size);

G_END_DECLS

#endif
