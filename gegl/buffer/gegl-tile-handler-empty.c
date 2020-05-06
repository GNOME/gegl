/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2006,2007 Øyvind Kolås <pippin@gimp.org>
 */
#include "config.h"
#include <glib.h>
#include <glib-object.h>
#include <string.h>

#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-handler-empty.h"

G_DEFINE_TYPE (GeglTileHandlerEmpty, gegl_tile_handler_empty,
               GEGL_TYPE_TILE_HANDLER)

static void
finalize (GObject *object)
{
  GeglTileHandlerEmpty *empty = GEGL_TILE_HANDLER_EMPTY (object);

  if (empty->tile)
    gegl_tile_unref (empty->tile);

  G_OBJECT_CLASS (gegl_tile_handler_empty_parent_class)->finalize (object);
}

static GeglTile *
get_tile (GeglTileSource *gegl_tile_source,
          gint            x,
          gint            y,
          gint            z)
{
  GeglTileSource       *source = ((GeglTileHandler *) gegl_tile_source)->source;
  GeglTileHandlerEmpty *empty  = (GeglTileHandlerEmpty *) gegl_tile_source;
  GeglTile             *tile   = NULL;

  if (source)
    tile = gegl_tile_source_get_tile (source, x, y, z);
  if (tile)
    return tile;

  if (!empty->tile)
    {
      gint tile_size = gegl_tile_backend_get_tile_size (empty->backend);
      empty->tile    = gegl_tile_handler_empty_new_tile (tile_size);
    }

  tile = gegl_tile_handler_dup_tile (GEGL_TILE_HANDLER (empty),
                                     empty->tile, x, y, z);

  /* if empty tiles don't have to be zero-initialized, mark them as fully
   * damaged, so that their data is not unnecessarily initialized when
   * uncloned.
   *
   * we currently only do this for level-0 tiles, since it simplifies the rest
   * of the code.
   */
  if (z == 0 && ! empty->initialized)
    tile->damage = ~0ull;

  /* no need to store the tile, since we'll just create another empty tile on-
   * demand if it's dropped.
   */
  gegl_tile_mark_as_stored (tile);

  return tile;
}

static gpointer
gegl_tile_handler_empty_command (GeglTileSource  *buffer,
                                 GeglTileCommand  command,
                                 gint             x,
                                 gint             y,
                                 gint             z,
                                 gpointer         data)
{
  if (command == GEGL_TILE_GET)
    return get_tile (buffer, x, y, z);

  return gegl_tile_handler_source_command (buffer, command, x, y, z, data);
}

static void
gegl_tile_handler_empty_class_init (GeglTileHandlerEmptyClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = finalize;
}

static void
gegl_tile_handler_empty_init (GeglTileHandlerEmpty *self)
{
  ((GeglTileSource *) self)->command = gegl_tile_handler_empty_command;
}

GeglTileHandler *
gegl_tile_handler_empty_new (GeglTileBackend *backend,
                             gboolean         initialized)
{
  GeglTileHandlerEmpty *empty = g_object_new (GEGL_TYPE_TILE_HANDLER_EMPTY, NULL);

  empty->backend     = backend;
  empty->tile        = NULL;
  empty->initialized = initialized;

  return (void*)empty;
}

GeglTile *
gegl_tile_handler_empty_new_tile (gint tile_size)
{
  static GeglTile   *common_tile = NULL;
  static const gint  common_empty_size = sizeof (gdouble) * 4 * 128 * 128;

  GeglTile *tile;

  if (tile_size > common_empty_size)
    {
      /* The tile size is too big to use the shared buffer */
      tile = gegl_tile_new (tile_size);

      memset (gegl_tile_get_data (tile), 0x00, tile_size);
      tile->is_zero_tile = TRUE;
    }
  else
    {
      if (!g_atomic_pointer_get (&common_tile) &&
          g_once_init_enter (&common_tile))
        {
          GeglTile *allocated_tile = gegl_tile_new_bare ();
          guchar *allocated_buffer = gegl_malloc (common_empty_size);
          memset (allocated_buffer, 0x00, common_empty_size);

          allocated_tile->data           = allocated_buffer;
          allocated_tile->destroy_notify = NULL;
          allocated_tile->size           = common_empty_size;
          allocated_tile->is_zero_tile   = TRUE;
          allocated_tile->is_global_tile = TRUE;

          /* avoid counting duplicates of the empty tile towards the total
           * cache size, both since this is unnecessary, and since they may
           * have different sizes, which is inconsistent with the duplicate-
           * tracking cache logic.
           */
          (*gegl_tile_n_cached_clones (allocated_tile))++;

          g_once_init_leave (&common_tile, allocated_tile);
        }

      tile = gegl_tile_dup (common_tile);

      tile->size = tile_size;
    }

  return tile;
}
