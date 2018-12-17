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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "gegl-compression.h"
#include "gegl-compression-nop.h"
#include "gegl-compression-rle.h"
#include "gegl-compression-zlib.h"


/*  local function prototypes  */

void   gegl_compression_register_alias (const gchar *name,
                                        ...) G_GNUC_NULL_TERMINATED;


/*  local variables  */

GHashTable *algorithms;


/*  private functions  */

void
gegl_compression_register_alias (const gchar *name,
                                 ...)
{
  va_list      args;
  const gchar *algorithm;

  va_start (args, name);

  while ((algorithm = va_arg (args, const gchar *)))
    {
      const GeglCompression *compression = gegl_compression (algorithm);

      if (compression)
        {
          gegl_compression_register (name, compression);

          break;
        }
    }

  va_end (args);
}


/*  public functions  */

void
gegl_compression_init (void)
{
  g_return_if_fail (algorithms == NULL);

  algorithms = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  gegl_compression_nop_init ();
  gegl_compression_rle_init ();
  gegl_compression_zlib_init ();

  gegl_compression_register_alias ("fast",
                                   /* in order of precedence: */
                                   "rle8",
                                   "zlib1",
                                   "nop",
                                   NULL);

  gegl_compression_register_alias ("balanced",
                                   /* in order of precedence: */
                                   "rle4",
                                   "zlib",
                                   "nop",
                                   NULL);

  gegl_compression_register_alias ("best",
                                   /* in order of precedence: */
                                   "zlib9",
                                   "rle1",
                                   "nop",
                                   NULL);
}

void
gegl_compression_cleanup (void)
{
  g_clear_pointer (&algorithms, g_hash_table_unref);
}

void
gegl_compression_register (const gchar           *name,
                           const GeglCompression *compression)
{
  g_return_if_fail (name != NULL);
  g_return_if_fail (compression != NULL);
  g_return_if_fail (compression->compress != NULL);
  g_return_if_fail (compression->decompress != NULL);

  g_hash_table_insert (algorithms, g_strdup (name), (gpointer) compression);
}

static gint
gegl_compression_list_compare (gconstpointer x,
                               gconstpointer y)
{
  return strcmp (*(const gchar **) x, *(const gchar **) y);
}

const gchar **
gegl_compression_list (void)
{
  const gchar    **names;
  GHashTableIter   iter;
  gint             i;

  names = g_new (const gchar *, g_hash_table_size (algorithms) + 1);

  g_hash_table_iter_init (&iter, algorithms);

  i = 0;

  while (g_hash_table_iter_next (&iter, (gpointer *) &names[i], NULL))
    i++;

  names[i] = NULL;

  qsort (names, i, sizeof (names[0]), gegl_compression_list_compare);

  return names;
}

const GeglCompression *
gegl_compression (const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);

  return g_hash_table_lookup (algorithms, name);
}

gboolean
gegl_compression_compress (const GeglCompression *compression,
                           const Babl            *format,
                           gconstpointer          data,
                           gint                   n,
                           gpointer               compressed,
                           gint                  *compressed_size,
                           gint                   max_compressed_size)
{
  g_return_val_if_fail (compression != NULL, FALSE);
  g_return_val_if_fail (format != NULL, FALSE);
  g_return_val_if_fail (data != NULL || n == 0, FALSE);
  g_return_val_if_fail (n >= 0, FALSE);
  g_return_val_if_fail (compressed != NULL || max_compressed_size == 0, FALSE);
  g_return_val_if_fail (compressed_size != NULL, FALSE);
  g_return_val_if_fail (max_compressed_size >= 0, FALSE);

  return compression->compress (compression,
                                format, data, n,
                                compressed, compressed_size,
                                max_compressed_size);
}

gboolean
gegl_compression_decompress (const GeglCompression *compression,
                             const Babl            *format,
                             gpointer               data,
                             gint                   n,
                             gconstpointer          compressed,
                             gint                   compressed_size)
{
  g_return_val_if_fail (compression != NULL, FALSE);
  g_return_val_if_fail (format != NULL, FALSE);
  g_return_val_if_fail (data != NULL || n == 0, FALSE);
  g_return_val_if_fail (n >= 0, FALSE);
  g_return_val_if_fail (compressed != NULL || compressed_size == 0, FALSE);
  g_return_val_if_fail (compressed_size >= 0, FALSE);

  return compression->decompress (compression,
                                  format, data, n,
                                  compressed, compressed_size);
}
