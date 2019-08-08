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
 * Copyright 2006, 2007, 2018 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>
#include <glib/gprintf.h>

#include "gegl-buffer.h"
#include "gegl-buffer-config.h"


G_DEFINE_TYPE (GeglBufferConfig, gegl_buffer_config, G_TYPE_OBJECT)

static GObjectClass * parent_class = NULL;

enum
{
  PROP_0,
  PROP_TILE_CACHE_SIZE,
  PROP_SWAP,
  PROP_SWAP_COMPRESSION,
  PROP_TILE_WIDTH,
  PROP_TILE_HEIGHT,
  PROP_QUEUE_SIZE,
};

static void
gegl_buffer_config_get_property (GObject    *gobject,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GeglBufferConfig *config = GEGL_BUFFER_CONFIG (gobject);

  switch (property_id)
    {
      case PROP_TILE_CACHE_SIZE:
        g_value_set_uint64 (value, config->tile_cache_size);
        break;

      case PROP_TILE_WIDTH:
        g_value_set_int (value, config->tile_width);
        break;

      case PROP_TILE_HEIGHT:
        g_value_set_int (value, config->tile_height);
        break;

      case PROP_SWAP:
        g_value_set_string (value, config->swap);
        break;

      case PROP_SWAP_COMPRESSION:
        g_value_set_string (value, config->swap_compression);
        break;

      case PROP_QUEUE_SIZE:
        g_value_set_int (value, config->queue_size);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
gegl_buffer_config_set_property (GObject      *gobject,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GeglBufferConfig *config = GEGL_BUFFER_CONFIG (gobject);

  switch (property_id)
    {
      case PROP_TILE_CACHE_SIZE:
        config->tile_cache_size = g_value_get_uint64 (value);
        break;
      case PROP_TILE_WIDTH:
        config->tile_width = g_value_get_int (value);
        break;
      case PROP_TILE_HEIGHT:
        config->tile_height = g_value_get_int (value);
        break;
      case PROP_QUEUE_SIZE:
        config->queue_size = g_value_get_int (value);
        break;
      case PROP_SWAP:
        g_free (config->swap);
        config->swap = g_value_dup_string (value);
        break;
      case PROP_SWAP_COMPRESSION:
        g_free (config->swap_compression);
        config->swap_compression = g_value_dup_string (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
gegl_buffer_config_finalize (GObject *gobject)
{
  GeglBufferConfig *config = GEGL_BUFFER_CONFIG (gobject);

  g_free (config->swap);
  g_free (config->swap_compression);

  G_OBJECT_CLASS (gegl_buffer_config_parent_class)->finalize (gobject);
}

static void
gegl_buffer_config_class_init (GeglBufferConfigClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class  = g_type_class_peek_parent (klass);

  gobject_class->set_property = gegl_buffer_config_set_property;
  gobject_class->get_property = gegl_buffer_config_get_property;
  gobject_class->finalize     = gegl_buffer_config_finalize;


  g_object_class_install_property (gobject_class, PROP_TILE_WIDTH,
                                   g_param_spec_int ("tile-width",
                                                     "Tile width",
                                                     "default tile width for created buffers.",
                                                     0, G_MAXINT, 128,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT |
                                                     G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TILE_HEIGHT,
                                   g_param_spec_int ("tile-height",
                                                     "Tile height",
                                                     "default tile height for created buffers.",
                                                     0, G_MAXINT, 128,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT |
                                                     G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TILE_CACHE_SIZE,
                                   g_param_spec_uint64 ("tile-cache-size",
                                                        "Tile Cache size",
                                                        "size of tile cache in bytes",
                                                        0, G_MAXUINT64, 512 * 1024 * 1024,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SWAP,
                                   g_param_spec_string ("swap",
                                                        "Swap",
                                                        "where gegl stores it's swap files",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_SWAP_COMPRESSION,
                                   g_param_spec_string ("swap-compression",
                                                        "Swap compression",
                                                        "compression algorithm used for data stored in the swap",
                                                        "fast",
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_QUEUE_SIZE,
                                   g_param_spec_int ("queue-size",
                                                     "Queue size",
                                                     "Maximum size of a file backend's writer thread queue (in bytes)",
                                                     2, G_MAXINT, 50 * 1024 *1024,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT |
                                                     G_PARAM_STATIC_STRINGS));
}

static void
gegl_buffer_config_init (GeglBufferConfig *self)
{
}


static GeglBufferConfig *config = NULL;


static void gegl_buffer_config_set_defaults (GeglBufferConfig *config)
{
  gchar *swapdir = g_build_filename (g_get_user_cache_dir(),
                                     GEGL_LIBRARY,
                                     "swap",
                                     NULL);
  g_object_set (config,
                "swap", swapdir,
                NULL);

  g_free (swapdir);
}

GeglBufferConfig *gegl_buffer_config (void)
{
  if (!config)
    {
      config = g_object_new (GEGL_TYPE_BUFFER_CONFIG, NULL);
      gegl_buffer_config_set_defaults (config);
    }
  return config;
}
