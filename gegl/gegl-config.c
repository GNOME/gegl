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
#include <glib/gprintf.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-config.h"
#include "gegl-buffer-config.h"

#include "opencl/gegl-cl.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef G_OS_WIN32
#include <windows.h>
#endif

G_DEFINE_TYPE (GeglConfig, gegl_config, G_TYPE_OBJECT)

static GObjectClass * parent_class = NULL;

enum
{
  PROP_0,
  PROP_QUALITY,
  PROP_TILE_CACHE_SIZE,
  PROP_CHUNK_SIZE,
  PROP_SWAP,
  PROP_SWAP_COMPRESSION,
  PROP_TILE_WIDTH,
  PROP_TILE_HEIGHT,
  PROP_THREADS,
  PROP_USE_OPENCL,
  PROP_QUEUE_SIZE,
  PROP_APPLICATION_LICENSE,
  PROP_MIPMAP_RENDERING
};

gint _gegl_threads = 1;

static void
gegl_config_get_property (GObject    *gobject,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GeglConfig *config = GEGL_CONFIG (gobject);

  switch (property_id)
    {
      case PROP_TILE_CACHE_SIZE:
        g_value_set_uint64 (value, config->tile_cache_size);
        break;

      case PROP_CHUNK_SIZE:
        g_value_set_int (value, config->chunk_size);
        break;

      case PROP_TILE_WIDTH:
        g_value_set_int (value, config->tile_width);
        break;

      case PROP_TILE_HEIGHT:
        g_value_set_int (value, config->tile_height);
        break;

      case PROP_QUALITY:
        g_value_set_double (value, config->quality);
        break;

      case PROP_SWAP:
        g_value_set_string (value, config->swap);
        break;

      case PROP_SWAP_COMPRESSION:
        g_value_set_string (value, config->swap_compression);
        break;

      case PROP_THREADS:
        g_value_set_int (value, _gegl_threads);
        break;

      case PROP_USE_OPENCL:
        g_value_set_boolean (value, gegl_cl_is_accelerated());
        break;

      case PROP_QUEUE_SIZE:
        g_value_set_int (value, config->queue_size);
        break;

      case PROP_APPLICATION_LICENSE:
        g_value_set_string (value, config->application_license);
        break;

      case PROP_MIPMAP_RENDERING:
        g_value_set_boolean (value, config->mipmap_rendering);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
gegl_config_set_property (GObject      *gobject,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GeglConfig *config = GEGL_CONFIG (gobject);

  switch (property_id)
    {
      case PROP_TILE_CACHE_SIZE:
        config->tile_cache_size = g_value_get_uint64 (value);
        break;
      case PROP_CHUNK_SIZE:
        config->chunk_size = g_value_get_int (value);
        break;
      case PROP_TILE_WIDTH:
        config->tile_width = g_value_get_int (value);
        break;
      case PROP_TILE_HEIGHT:
        config->tile_height = g_value_get_int (value);
        break;
      case PROP_QUALITY:
        config->quality = g_value_get_double (value);
        return;
      case PROP_SWAP:
        g_free (config->swap);
        config->swap = g_value_dup_string (value);
        break;
      case PROP_SWAP_COMPRESSION:
        g_free (config->swap_compression);
        config->swap_compression = g_value_dup_string (value);
        break;
      case PROP_THREADS:
        _gegl_threads = g_value_get_int (value);
        return;
      case PROP_USE_OPENCL:
        config->use_opencl = g_value_get_boolean (value);
        break;
      case PROP_MIPMAP_RENDERING:
        config->mipmap_rendering = g_value_get_boolean (value);
        break;
      case PROP_QUEUE_SIZE:
        config->queue_size = g_value_get_int (value);
        break;
      case PROP_APPLICATION_LICENSE:
        g_free (config->application_license);
        config->application_license = g_value_dup_string (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
gegl_config_finalize (GObject *gobject)
{
  GeglConfig *config = GEGL_CONFIG (gobject);

  g_free (config->swap);
  g_free (config->swap_compression);
  g_free (config->application_license);

  G_OBJECT_CLASS (gegl_config_parent_class)->finalize (gobject);
}

static void
gegl_config_class_init (GeglConfigClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class  = g_type_class_peek_parent (klass);

  gobject_class->set_property = gegl_config_set_property;
  gobject_class->get_property = gegl_config_get_property;
  gobject_class->finalize     = gegl_config_finalize;


  g_object_class_install_property (gobject_class, PROP_TILE_WIDTH,
                                   g_param_spec_int ("tile-width",
                                                     "Tile width",
                                                     "default tile width for created buffers.",
                                                     0, G_MAXINT, 128,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_TILE_HEIGHT,
                                   g_param_spec_int ("tile-height",
                                                     "Tile height",
                                                     "default tile height for created buffers.",
                                                     0, G_MAXINT, 128,
                                                     G_PARAM_READWRITE));

  {
    long default_tile_cache_size = 1024l * 1024 * 1024;
    long mem_total = default_tile_cache_size;
    long mem_min = 512 << 20; // 512mb
    long mem_available = mem_min;

#ifdef G_OS_WIN32
# if defined(_MSC_VER) && (_MSC_VER <= 1200)
  MEMORYSTATUS memory_status;
  memory_status.dwLength = sizeof (memory_status);

  GlobalMemoryStatus (&memory_status);
  mem_total = memory_status.dwTotalPhys;
  mem_available = memory_status.dwAvailPhys;
# else
  /* requires w2k and newer SDK than provided with msvc6 */
  MEMORYSTATUSEX memory_status;

  memory_status.dwLength = sizeof (memory_status);

  if (GlobalMemoryStatusEx (&memory_status))
  {
    mem_total = memory_status.ullTotalPhys;
    mem_available = memory_status.ullAvailPhys;
  }
# endif

#else
    mem_total = sysconf (_SC_PHYS_PAGES) * sysconf (_SC_PAGESIZE);
    mem_available = sysconf (_SC_AVPHYS_PAGES) * sysconf (_SC_PAGESIZE);
#endif

    default_tile_cache_size = mem_total;
    if (default_tile_cache_size > mem_available)
    {
      default_tile_cache_size = mem_available;
    }
    if (default_tile_cache_size < mem_min)
      default_tile_cache_size = mem_min;

  g_object_class_install_property (gobject_class, PROP_TILE_CACHE_SIZE,
                                   g_param_spec_uint64 ("tile-cache-size",
                                                        "Tile Cache size",
                                                        "size of tile cache in bytes",
                                                        0, G_MAXUINT64, default_tile_cache_size,
                                                        G_PARAM_READWRITE));
  }

  g_object_class_install_property (gobject_class, PROP_CHUNK_SIZE,
                                   g_param_spec_int ("chunk-size",
                                                     "Chunk size",
                                                     "the number of pixels processed simultaneously by GEGL.",
                                                     1, G_MAXINT, 1024 * 1024,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_QUALITY,
                                   g_param_spec_double ("quality",
                                                        "Quality",
                                                        "quality/speed trade off 1.0 = full quality, 0.0 = full speed",
                                                        0.0, 1.0, 1.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_SWAP,
                                   g_param_spec_string ("swap",
                                                        "Swap",
                                                        "where gegl stores it's swap files",
                                                        NULL,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SWAP_COMPRESSION,
                                   g_param_spec_string ("swap-compression",
                                                        "Swap compression",
                                                        "compression algorithm used for data stored in the swap",
                                                        NULL,
                                                        G_PARAM_READWRITE));

  _gegl_threads = g_get_num_processors ();
  _gegl_threads = MIN (_gegl_threads, GEGL_MAX_THREADS);
  g_object_class_install_property (gobject_class, PROP_THREADS,
                                   g_param_spec_int ("threads",
                                                     "Number of threads",
                                                     "Number of concurrent evaluation threads",
                                                     0, GEGL_MAX_THREADS,
                                                     _gegl_threads,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_MIPMAP_RENDERING,
                                   g_param_spec_boolean ("mipmap-rendering",
                                                         "mipmap rendering",
                                                         "Enable code paths for mipmap preview rendering, uses approximations for 50% 25% etc zoom factors to reduce processing.",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_USE_OPENCL,
                                   g_param_spec_boolean ("use-opencl",
                                                         "Use OpenCL",
                                                         "Try to use OpenCL",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_QUEUE_SIZE,
                                   g_param_spec_int ("queue-size",
                                                     "Queue size",
                                                     "Maximum size of a file backend's writer thread queue (in bytes)",
                                                     2, G_MAXINT, 50 * 1024 *1024,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_APPLICATION_LICENSE,
                                   g_param_spec_string ("application-license",
                                                        "Application license",
                                                        "A list of additional licenses to allow for operations",
                                                        "",
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
gegl_config_init (GeglConfig *self)
{
  char *forward_props[]={"swap",
                         "swap-compression",
                         "queue-size",
                         "tile-width",
                         "tile-height",
                         "tile-cache-size",
                         NULL};
  GeglBufferConfig *bconf = gegl_buffer_config ();
  for (int i = 0; forward_props[i]; i++)
    g_object_bind_property (bconf, forward_props[i],
                            self, forward_props[i],
                            G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
}

#undef  gegl_config_threads
int gegl_config_threads(void);
int gegl_config_threads(void)
{
  return _gegl_threads;
}
