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
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2006, 2007 Øyvind Kolås <pippin@gimp.org>
 */
#include "config.h"

#include <string.h>

#include <glib-object.h>
#include <glib/gstdio.h>

#include "gegl-buffer.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-source.h"
#include "gegl-tile-backend.h"
#include "gegl-buffer-config.h"

G_DEFINE_TYPE_WITH_PRIVATE (GeglTileBackend, gegl_tile_backend,
                            GEGL_TYPE_TILE_SOURCE)

#define parent_class gegl_tile_backend_parent_class

enum
{
  PROP_0,
  PROP_TILE_WIDTH,
  PROP_TILE_HEIGHT,
  PROP_PX_SIZE,
  PROP_TILE_SIZE,
  PROP_FORMAT,
  PROP_FLUSH_ON_DESTROY
};

static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglTileBackend *backend = GEGL_TILE_BACKEND (gobject);

  switch (property_id)
    {
      case PROP_TILE_WIDTH:
        g_value_set_int (value, backend->priv->tile_width);
        break;

      case PROP_TILE_HEIGHT:
        g_value_set_int (value, backend->priv->tile_height);
        break;

      case PROP_TILE_SIZE:
        g_value_set_int (value, backend->priv->tile_size);
        break;

      case PROP_PX_SIZE:
        g_value_set_int (value, backend->priv->px_size);
        break;

      case PROP_FORMAT:
        g_value_set_pointer (value, (void*)backend->priv->format);
        break;

      case PROP_FLUSH_ON_DESTROY:
        g_value_set_boolean (value, backend->priv->flush_on_destroy);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglTileBackend *backend = GEGL_TILE_BACKEND (gobject);

  switch (property_id)
    {
      case PROP_TILE_WIDTH:
        backend->priv->tile_width = g_value_get_int (value);
        return;

      case PROP_TILE_HEIGHT:
        backend->priv->tile_height = g_value_get_int (value);
        return;

      case PROP_FORMAT:
        backend->priv->format = g_value_get_pointer (value);
        break;

      case PROP_FLUSH_ON_DESTROY:
        backend->priv->flush_on_destroy = g_value_get_boolean (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

/* before 0.4.10, tile backends used to assert that
 * '0 <= command < GEGL_TILE_LAST_COMMAND' in their command handlers, which
 * prevented us from adding new tile commands without breaking the abi, since
 * GEGL_TILE_LAST_COMMAND is a compile-time constant.  tile backends are now
 * expected to forward unhandled commands to gegl_tile_backend_command()
 * instead.
 *
 * in order to keep supporting tile backends that were compiled against 0.4.8
 * or earlier, we replace the backend's command handler with a thunk
 * (tile_command_check_0_4_8()) upon construction, which tests whether the
 * backend forwards unhandled commands to gegl_tile_backend_command(), and
 * subsequently replaces the command handler with either the original command
 * handler if it does, or a compatibility shim (tile_command()) if it doesn't.
 */

/* this is the actual default command handler, called by
 * gegl_tile_backend_command().  currently, it simply validates the command
 * range, and returns NULL to any command.  we can add special behavior for
 * different commands as needed.
 */
static inline gpointer
_gegl_tile_backend_command (GeglTileBackend *backend,
                            GeglTileCommand  command,
                            gint             x,
                            gint             y,
                            gint             z,
                            gpointer         data)
{
  g_return_val_if_fail (command >= 0 && command < GEGL_TILE_LAST_COMMAND, NULL);

  return NULL;
}

/* this is a compatibility shim for backends compiled against 0.4.8 or earlier.
 * it forwards commands that were present in these versions to the original
 * handler, and newer commands to the default handler.
 */
static gpointer
tile_command (GeglTileSource  *source,
              GeglTileCommand  command,
              gint             x,
              gint             y,
              gint             z,
              gpointer         data)
{
  GeglTileBackend *backend = GEGL_TILE_BACKEND (source);

  if (G_LIKELY (command < _GEGL_TILE_LAST_0_4_8_COMMAND))
    return backend->priv->command (source, command, x, y, z, data);

  return _gegl_tile_backend_command (backend, command, x, y, z, data);
}

/* this is a thunk, testing whether the backend forwards unhandled commands to
 * gegl_tile_backend_command().  if it does, we replace the thunk by the
 * original handler; if it doesn't, we assume the backend can't handle post-
 * 0.4.8 commands, and replace the thunk with the compatibility shim.
 */
static gpointer
tile_command_check_0_4_8 (GeglTileSource  *source,
                          GeglTileCommand  command,
                          gint             x,
                          gint             y,
                          gint             z,
                          gpointer         data)
{
  GeglTileBackend *backend = GEGL_TILE_BACKEND (source);

  /* start by replacing the thunk by the compatibility shim */
  source->command = tile_command;

  /* pass the original handler a dummy command.  we use GEGL_TILE_IS_CACHED,
   * since backends shouldn't handle this command.  if the handler forwards the
   * command to gegl_tile_backend_command(), it will replace the shim by the
   * original handler.
   */
  backend->priv->command (source, GEGL_TILE_IS_CACHED, 0, 0, 0, NULL);

  /* forward the command to either the shim or the original handler */
  return source->command (source, command, x, y, z, data);
}

static void
constructed (GObject *object)
{
  GeglTileBackend *backend = GEGL_TILE_BACKEND (object);
  GeglTileSource  *source  = GEGL_TILE_SOURCE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (backend->priv->tile_width > 0 && backend->priv->tile_height > 0);
  g_assert (backend->priv->format);

  backend->priv->px_size = babl_format_get_bytes_per_pixel (backend->priv->format);
  backend->priv->tile_size = backend->priv->tile_width * backend->priv->tile_height * backend->priv->px_size;

  backend->priv->command = source->command;
  source->command        = tile_command_check_0_4_8;
}

static void
gegl_tile_backend_class_init (GeglTileBackendClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->constructed  = constructed;

  g_object_class_install_property (gobject_class, PROP_TILE_WIDTH,
                                   g_param_spec_int ("tile-width", "tile-width",
                                                     "Tile width in pixels",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY |
                                                     G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_TILE_HEIGHT,
                                   g_param_spec_int ("tile-height", "tile-height",
                                                     "Tile height in pixels",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY |
                                                     G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_TILE_SIZE,
                                   g_param_spec_int ("tile-size", "tile-size",
                                                     "Size of the tiles linear buffer in bytes",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READABLE |
                                                     G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PX_SIZE,
                                   g_param_spec_int ("px-size", "px-size",
                                                     "Size of a single pixel in bytes",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READABLE |
                                                     G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_FORMAT,
                                   g_param_spec_pointer ("format", "format",
                                                         "babl format",
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_FLUSH_ON_DESTROY,
                                   g_param_spec_boolean ("flush-on-destroy", "flush-on-destroy",
                                                         "Cache tiles will be flushed before the backend is destroyed",
                                                         TRUE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_STRINGS));
}

static void
gegl_tile_backend_init (GeglTileBackend *self)
{
  self->priv = gegl_tile_backend_get_instance_private (self);
  self->priv->shared = FALSE;
  self->priv->flush_on_destroy = TRUE;
}


gint
gegl_tile_backend_get_tile_size (GeglTileBackend *tile_backend)
{
  return tile_backend->priv->tile_size;
}

gint
gegl_tile_backend_get_tile_width (GeglTileBackend *tile_backend)
{
  return tile_backend->priv->tile_width;
}

gint
gegl_tile_backend_get_tile_height (GeglTileBackend *tile_backend)
{
  return tile_backend->priv->tile_height;
}

const Babl *
gegl_tile_backend_get_format (GeglTileBackend *tile_backend)
{
  return tile_backend->priv->format;
}


void
gegl_tile_backend_set_extent (GeglTileBackend     *tile_backend,
                              const GeglRectangle *rectangle)
{
  tile_backend->priv->extent = *rectangle;
}

GeglRectangle
gegl_tile_backend_get_extent (GeglTileBackend *tile_backend)
{
  return tile_backend->priv->extent;
}

GeglTileSource *
gegl_tile_backend_peek_storage (GeglTileBackend *backend)
{
  return backend->priv->storage;
}

void
gegl_tile_backend_set_flush_on_destroy (GeglTileBackend *tile_backend,
                                        gboolean         flush_on_destroy)
{
  tile_backend->priv->flush_on_destroy = flush_on_destroy;
}

gboolean
gegl_tile_backend_get_flush_on_destroy (GeglTileBackend *tile_backend)
{
  return tile_backend->priv->flush_on_destroy;
}

gpointer
gegl_tile_backend_command (GeglTileBackend *backend,
                           GeglTileCommand  command,
                           gint             x,
                           gint             y,
                           gint             z,
                           gpointer         data)
{
  /* we've been called during the tile_command() test, which means the backend
   * is post-0.4.8 compatible.  replace the shim with the original handler.
   */
  if (backend->priv->command)
    {
      GeglTileSource *source = GEGL_TILE_SOURCE (backend);

      source->command        = backend->priv->command;
      backend->priv->command = NULL;
    }

  return _gegl_tile_backend_command (backend, command, x, y, z, data);
}

void
gegl_tile_backend_unlink_swap (gchar *path)
{
  gchar *dirname = g_path_get_dirname (path);

  /* Ensure we delete only files in our known swap directory for safety. */
  if (g_file_test (path, G_FILE_TEST_EXISTS) &&
      g_strcmp0 (dirname, gegl_buffer_config()->swap) == 0)
    g_unlink (path);

  g_free (dirname);
}
