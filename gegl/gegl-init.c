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
 *
 * Copyright 2003-2007 Calvin Williamson, Øyvind Kolås
 *           2013      Daniel Sabo
 */

#include "config.h"
#define __GEGL_INIT_C

#include <babl/babl.h>

#include <glib-object.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#include <locale.h>

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef G_OS_WIN32

#include <windows.h>

static HMODULE hLibGeglModule = NULL;

/* DllMain prototype */
BOOL WINAPI DllMain (HINSTANCE hinstDLL,
                     DWORD     fdwReason,
                     LPVOID    lpvReserved);

BOOL WINAPI
DllMain (HINSTANCE hinstDLL,
         DWORD     fdwReason,
         LPVOID    lpvReserved)
{
  hLibGeglModule = hinstDLL;
  return TRUE;
}

#endif

#ifdef __APPLE__
#import <Foundation/Foundation.h>
#endif

#include "gegl-debug.h"

guint gegl_debug_flags = 0;

#include "gegl-types.h"
#include "gegl-types-internal.h"
#include "gegl-instrument.h"
#include "gegl-init.h"
#include "gegl-init-private.h"
#include "module/geglmodule.h"
#include "module/geglmoduledb.h"
#include "buffer/gegl-buffer.h"
#include "operation/gegl-operation.h"
#include "operation/gegl-operations.h"
#include "operation/gegl-operation-handlers-private.h"
#include "buffer/gegl-buffer-private.h"
#include "buffer/gegl-buffer-iterator-private.h"
#include "buffer/gegl-buffer-swap-private.h"
#include "buffer/gegl-compression.h"
#include "buffer/gegl-tile-alloc.h"
#include "buffer/gegl-tile-backend-ram.h"
#include "buffer/gegl-tile-backend-file.h"
#include "gegl-config.h"
#include "gegl-stats.h"
#include "graph/gegl-node-private.h"
#include "gegl-random-private.h"
#include "gegl-parallel-private.h"
#include "gegl-cpuaccel.h"

static gboolean  gegl_post_parse_hook (GOptionContext *context,
                                       GOptionGroup   *group,
                                       gpointer        data,
                                       GError        **error);


static GeglConfig   *config = NULL;

static GeglStats    *stats = NULL;

static GeglModuleDB *module_db   = NULL;

static glong         global_time = 0;

static void load_module_path(gchar *path, GeglModuleDB *db);

static void
gegl_config_application_license_notify (GObject    *gobject,
                                        GParamSpec *pspec,
                                        gpointer    user_data)
{
  GeglConfig *cfg = GEGL_CONFIG (gobject);
  GSList *paths = gegl_get_default_module_paths ();

  gegl_operations_set_licenses_from_string (cfg->application_license);

  /* causes load of .so's that might have been skipped due to filename */
  g_slist_foreach(paths, (GFunc)load_module_path, module_db);
  g_slist_free_full (paths, g_free);
}


static void
gegl_config_use_opencl_notify (GObject    *gobject,
                               GParamSpec *pspec,
                               gpointer    user_data)
{
  GeglConfig *cfg = GEGL_CONFIG (gobject);

  g_signal_handlers_block_by_func (gobject,
                                   gegl_config_use_opencl_notify,
                                   NULL);

  if (cfg->use_opencl)
    {
       gegl_cl_init (NULL);
    }
  else
    {
      gegl_cl_disable ();
    }

  cfg->use_opencl = gegl_cl_is_accelerated();

  g_signal_handlers_unblock_by_func (gobject,
                                     gegl_config_use_opencl_notify,
                                     NULL);
}

#if defined (_WIN32) && !defined (__CYGWIN__)
extern IMAGE_DOS_HEADER __ImageBase;

static HMODULE
this_module (void)
{
  return (HMODULE) &__ImageBase;
}
#endif

static gchar *
gegl_init_get_prefix (void)
{
  gchar *prefix = NULL;

#if defined (_WIN32) && !defined (__CYGWIN__)

  prefix = g_win32_get_package_installation_directory_of_module (this_module ());

#elif defined (__APPLE__)

  NSAutoreleasePool *pool;
  NSString          *resource_path;
  gchar             *basename;
  gchar             *basepath;
  gchar             *dirname;

  pool = [[NSAutoreleasePool alloc] init];

  resource_path = [[NSBundle mainBundle] resourcePath];

  basename = g_path_get_basename ([resource_path UTF8String]);
  basepath = g_path_get_dirname ([resource_path UTF8String]);
  dirname  = g_path_get_basename (basepath);

  if (! strcmp (basename, ".libs"))
    {
      /*  we are running from the source dir, do normal unix things  */

      prefix = g_strdup (GEGL_PREFIX);
    }
  else if (! strcmp (basename, "bin"))
    {
      /*  we are running the main app, but not from a bundle, the resource
       *  path is the directory which contains the executable
       */

      prefix = g_strdup (basepath);
    }
  else if (strstr (basepath, "/Cellar/"))
    {
      /*  we are running from a Python.framework bundle built in homebrew
       *  during the build phase
       */

      gchar *fulldir = g_strdup (basepath);
      gchar *lastdir = g_path_get_basename (fulldir);
      gchar *tmp_fulldir;

      while (strcmp (lastdir, "Cellar"))
        {
          tmp_fulldir = g_path_get_dirname (fulldir);

          g_free (lastdir);
          g_free (fulldir);

          fulldir = tmp_fulldir;
          lastdir = g_path_get_basename (fulldir);
        }
      prefix = g_path_get_dirname (fulldir);

      g_free (fulldir);
      g_free (lastdir);
    }
  else
    {
      /*  if none of the above match, we assume that we are really in a bundle  */

      prefix = g_strdup ([resource_path UTF8String]);
    }

  g_free (basename);
  g_free (basepath);
  g_free (dirname);

  [pool drain];
#endif

  if (prefix == NULL)
    prefix = g_strdup (GEGL_PREFIX);

  return prefix;
}

static void
gegl_init_i18n (void)
{
  static gboolean i18n_initialized = FALSE;

  if (! i18n_initialized)
    {
      gchar *localedir = NULL;

      if (g_path_is_absolute (GEGL_LOCALEDIR))
        localedir = g_strdup (GEGL_LOCALEDIR);

#if defined (_WIN32) && !defined (__CYGWIN__)
      wchar_t *dir_name_utf16;

      if (localedir == NULL)
        {
          gchar *prefix;

          prefix = gegl_init_get_prefix ();
          localedir = g_build_filename (prefix, GEGL_LOCALEDIR, NULL);
          g_free (prefix);
        }

      dir_name_utf16 = g_utf8_to_utf16 (localedir, -1, NULL, NULL, NULL);
      if G_UNLIKELY (! dir_name_utf16)
        g_printerr ("%s: Cannot translate the catalog directory to UTF-16\n", G_STRFUNC);
      else
        wbindtextdomain (GETTEXT_PACKAGE, dir_name_utf16);

      g_free (dir_name_utf16);
#else
      if (localedir == NULL)
        {
          gchar *prefix;

          prefix = gegl_init_get_prefix ();
          localedir = g_build_filename (prefix, GEGL_LOCALEDIR, NULL);
          g_free (prefix);
        }

      bindtextdomain (GETTEXT_PACKAGE, localedir);
#endif
      bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

      i18n_initialized = TRUE;
      g_free (localedir);
    }
}

static GThread *main_thread = NULL;

gboolean gegl_is_main_thread (void)
{
  return g_thread_self () == main_thread;
}

#if BABL_MINOR_VERSION>1 || (BABL_MINOR_VERSION==1 && BABL_MICRO_VERSION >= 90)
static gboolean gegl_idle_gc (gpointer user_data)
{
  babl_gc ();
  return TRUE;
}
#endif

void _gegl_init_buffer (int x86_64_version);

void
gegl_init (gint    *argc,
           gchar ***argv)
{
  GOptionContext *context;
  GError         *error = NULL;
  static gboolean initialized = FALSE;

  if (initialized)
    return;



  initialized = TRUE;

  context = g_option_context_new (NULL);
  g_option_context_set_ignore_unknown_options (context, TRUE);
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_set_main_group (context, gegl_get_option_group ());

  if (!g_option_context_parse (context, argc, argv, &error))
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }

  g_option_context_free (context);

#if BABL_MINOR_VERSION>1 || (BABL_MINOR_VERSION==1 && BABL_MICRO_VERSION >= 97)
  g_timeout_add_seconds (10, gegl_idle_gc, NULL);
#endif
}

static gchar    *cmd_gegl_swap             = NULL;
static gchar    *cmd_gegl_swap_compression = NULL;
static gchar    *cmd_gegl_cache_size       = NULL;
static gchar    *cmd_gegl_chunk_size       = NULL;
static gchar    *cmd_gegl_quality          = NULL;
static gchar    *cmd_gegl_tile_size        = NULL;
static gchar    *cmd_gegl_threads          = NULL;
static gboolean *cmd_gegl_disable_opencl   = NULL;

static const GOptionEntry cmd_entries[]=
{
    {
     "gegl-swap", 0, 0,
     G_OPTION_ARG_STRING, &cmd_gegl_swap,
     N_("Where GEGL stores its swap"), "<uri>"
    },
    {
     "gegl-swap-compression", 0, 0,
     G_OPTION_ARG_STRING, &cmd_gegl_swap_compression,
     N_("Compression algorithm used for data stored in the swap"), "<algorithm>"
    },
    {
     "gegl-cache-size", 0, 0,
     G_OPTION_ARG_STRING, &cmd_gegl_cache_size,
     N_("How much memory to (approximately) use for caching imagery"), "<megabytes>"
    },
    {
     "gegl-tile-size", 0, 0,
     G_OPTION_ARG_STRING, &cmd_gegl_tile_size,
     N_("Default size of tiles in GeglBuffers"), "<widthxheight>"
    },
    {
     "gegl-chunk-size", 0, 0,
     G_OPTION_ARG_STRING, &cmd_gegl_chunk_size,
     N_("The count of pixels to compute simultaneously"), "pixel count"
    },
    {
     "gegl-quality", 0, 0,
     G_OPTION_ARG_STRING, &cmd_gegl_quality,
     N_("The quality of rendering, a value between 0.0 (fast) and 1.0 (reference)"), "<quality>"
    },
    {
     "gegl-threads", 0, 0,
     G_OPTION_ARG_STRING, &cmd_gegl_threads,
     N_("The number of concurrent processing threads to use"), "<threads>"
    },
    {
      "gegl-disable-opencl", 0, 0,
      G_OPTION_ARG_NONE, &cmd_gegl_disable_opencl,
      N_("Disable OpenCL"), NULL
    },
    { NULL }
};

GOptionGroup *
gegl_get_option_group (void)
{
  GOptionGroup *group;

  gegl_init_i18n ();

  group = g_option_group_new ("gegl", "GEGL Options", _("Show GEGL Options"),
                              NULL, NULL);
  g_option_group_add_entries (group, cmd_entries);

  g_option_group_set_parse_hooks (group, NULL, gegl_post_parse_hook);

  return group;
}

static void 
gegl_config_parse_env (GeglConfig *config)
{
  if (g_getenv ("GEGL_MIPMAP_RENDERING"))
    {
      const gchar *value = g_getenv ("GEGL_MIPMAP_RENDERING");
      if (!strcmp (value, "1")||
          !strcmp (value, "true")||
          !strcmp (value, "yes"))
        g_object_set (config, "mipmap-rendering", TRUE, NULL);
      else
        g_object_set (config, "mipmap-rendering", FALSE, NULL);
    }


  if (g_getenv ("GEGL_QUALITY"))
    {
      const gchar *quality = g_getenv ("GEGL_QUALITY");

      if (g_str_equal (quality, "fast"))
        g_object_set (config, "quality", 0.0, NULL);
      else if (g_str_equal (quality, "good"))
        g_object_set (config, "quality", 0.5, NULL);
      else if (g_str_equal (quality, "best"))
        g_object_set (config, "quality", 1.0, NULL);
      else
        g_object_set (config, "quality", atof (quality), NULL);
    }

  if (g_getenv ("GEGL_CACHE_SIZE"))
    {
      g_object_set (config,
                    "tile-cache-size",
                    (guint64) atoll(g_getenv("GEGL_CACHE_SIZE")) * 1024 * 1024,
                    NULL);
    }

  if (g_getenv ("GEGL_CHUNK_SIZE"))
    config->chunk_size = atoi(g_getenv("GEGL_CHUNK_SIZE"));

  if (g_getenv ("GEGL_TILE_SIZE"))
    {
      const gchar *str = g_getenv ("GEGL_TILE_SIZE");
      gint         width;
      gint         height;
      width = height = atoi(str);
      str = strchr (str, 'x');
      if (str)
        height = atoi(str+1);
      g_object_set (config,
                    "tile-width",  width,
                    "tile-height", height,
                    NULL);
    }

  if (g_getenv ("GEGL_THREADS"))
    {
      _gegl_threads = atoi(g_getenv("GEGL_THREADS"));

      if (_gegl_threads > GEGL_MAX_THREADS)
        {
          g_warning ("Tried to use %i threads, max is %i",
                     _gegl_threads, GEGL_MAX_THREADS);
          _gegl_threads = GEGL_MAX_THREADS;
        }
    }

  if (g_getenv ("GEGL_USE_OPENCL"))
    {
      const char *opencl_env = g_getenv ("GEGL_USE_OPENCL");

      if (g_ascii_strcasecmp (opencl_env, "yes") == 0)
        g_object_set (config, "use-opencl", TRUE, NULL);
      else if (g_ascii_strcasecmp (opencl_env, "no") == 0)
        gegl_cl_hard_disable ();
      else if (g_ascii_strcasecmp (opencl_env, "cpu") == 0) {
        gegl_cl_set_default_device_type (CL_DEVICE_TYPE_CPU);
        g_object_set (config, "use-opencl", TRUE, NULL);
      } else if (g_ascii_strcasecmp (opencl_env, "gpu") == 0) {
        gegl_cl_set_default_device_type (CL_DEVICE_TYPE_GPU);
        g_object_set (config, "use-opencl", TRUE, NULL);
      } else if (g_ascii_strcasecmp (opencl_env, "accelerator") == 0) {
        gegl_cl_set_default_device_type (CL_DEVICE_TYPE_ACCELERATOR);
        g_object_set (config, "use-opencl", TRUE, NULL);
      } else
        g_warning ("Unknown value for GEGL_USE_OPENCL: %s", opencl_env);
    }

  if (g_getenv ("GEGL_SWAP"))
    g_object_set (config, "swap", g_getenv ("GEGL_SWAP"), NULL);

  if (g_getenv ("GEGL_SWAP_COMPRESSION"))
    {
      g_object_set (config,
                    "swap-compression", g_getenv ("GEGL_SWAP_COMPRESSION"),
                    NULL);
    }
}

GeglConfig *
gegl_config (void)
{
  if (!config)
    config = g_object_new (GEGL_TYPE_CONFIG, NULL);

  return config;
}

GeglStats *
gegl_stats (void)
{
  if (! stats)
    stats = g_object_new (GEGL_TYPE_STATS, NULL);

  return stats;
}

void 
gegl_reset_stats (void)
{
  gegl_stats_reset (gegl_stats ());
}

void 
gegl_temp_buffer_free (void);

void
gegl_exit (void)
{
  if (!config)
    {
      g_warning("gegl_exit() called without matching call to gegl_init()");
      return;
    }

  GEGL_INSTRUMENT_START()

  gegl_tile_backend_swap_cleanup ();
  gegl_tile_cache_destroy ();
  gegl_operation_gtype_cleanup ();
  gegl_operation_handlers_cleanup ();
  gegl_compression_cleanup ();
  gegl_random_cleanup ();
  gegl_parallel_cleanup ();
  gegl_buffer_swap_cleanup ();
  gegl_tile_alloc_cleanup ();
  gegl_cl_cleanup ();

  gegl_temp_buffer_free ();

  g_clear_object (&module_db);

  babl_exit ();

  GEGL_INSTRUMENT_END ("gegl", "gegl_exit")

  /* used when tracking buffer and tile leaks */
  if (g_getenv ("GEGL_DEBUG_BUFS") != NULL)
    {
      gegl_buffer_stats ();
      gegl_tile_backend_ram_stats ();
      gegl_tile_backend_file_stats ();
    }
  global_time = gegl_ticks () - global_time;
  gegl_instrument ("gegl", "gegl", global_time);

  if (gegl_instrument_enabled)
    {
      g_printf ("\n%s", gegl_instrument_utf8 ());
    }

  if (gegl_buffer_leaks ())
    {
      g_printf ("EEEEeEeek! %i GeglBuffers leaked\n", gegl_buffer_leaks ());
#ifdef GEGL_ENABLE_DEBUG
      if (!(gegl_debug_flags & GEGL_DEBUG_BUFFER_ALLOC))
        g_printerr ("To debug GeglBuffer leaks, set the environment "
                    "variable GEGL_DEBUG to \"buffer-alloc\"\n");
#endif
    }

  g_clear_object (&config);
  global_time = 0;
}

void
gegl_get_version (int *major,
                  int *minor,
                  int *micro)
{
  if (major != NULL)
    *major = GEGL_MAJOR_VERSION;

  if (minor != NULL)
    *minor = GEGL_MINOR_VERSION;

  if (micro != NULL)
    *micro = GEGL_MICRO_VERSION;
}

void
gegl_load_module_directory (const gchar *path)
{
  g_return_if_fail (g_file_test (path, G_FILE_TEST_IS_DIR));

  gegl_module_db_load (module_db, path);
}


GSList *
gegl_get_default_module_paths(void)
{
  GSList *list = NULL;
  gchar *module_path = NULL;

  // GEGL_PATH
  const gchar *gegl_path = g_getenv ("GEGL_PATH");
  if (gegl_path)
    {
      list = g_slist_append (list, g_strdup (gegl_path));
      return list;
    }

  // System library dir
#ifdef G_OS_WIN32
  {
    gchar *prefix;
    prefix = g_win32_get_package_installation_directory_of_module ( hLibGeglModule );
    module_path = g_build_filename (prefix, "lib", GEGL_LIBRARY, NULL);
    g_free(prefix);
  }
#else
  module_path = g_build_filename (LIBDIR, GEGL_LIBRARY, NULL);
#endif
  list = g_slist_append (list, module_path);

  /* User data dir
   * ~/.local/share/gegl-x.y/plug-ins */
  module_path = g_build_filename (g_get_user_data_dir (),
                                  GEGL_LIBRARY,
                                  "plug-ins",
                                  NULL);
  g_mkdir_with_parents (module_path, S_IRUSR | S_IWUSR | S_IXUSR);
  list = g_slist_append (list, module_path);

  return list;
}

static void
load_module_path(gchar *path, GeglModuleDB *db)
{
  gegl_module_db_load (db, path);
}

static gboolean
gegl_post_parse_hook (GOptionContext *context,
                      GOptionGroup   *group,
                      gpointer        data,
                      GError        **error)
{
  GeglConfig *config;

  g_assert (global_time == 0);
  global_time = gegl_ticks ();

  if (g_getenv ("GEGL_DEBUG_TIME") != NULL)
    gegl_instrument_enable ();

  gegl_instrument ("gegl", "gegl_init", 0);

  config = gegl_config ();

  gegl_config_parse_env (config);

  babl_init ();

#if ARCH_ARM
  GeglCpuAccelFlags cpu_accel = gegl_cpu_accel_get_support ();
  _gegl_init_buffer ((cpu_accel & GEGL_CPU_ACCEL_ARM_NEON) != 0);
#else
  GeglCpuAccelFlags cpu_accel = gegl_cpu_accel_get_support ();
  int x86_64_version = 0;
  if ((cpu_accel & GEGL_CPU_ACCEL_X86_64_V2) == GEGL_CPU_ACCEL_X86_64_V2)
    x86_64_version = 2;
  if ((cpu_accel & GEGL_CPU_ACCEL_X86_64_V3) == GEGL_CPU_ACCEL_X86_64_V3)
    x86_64_version = 3;

  _gegl_init_buffer (x86_64_version);
#endif

#ifdef GEGL_ENABLE_DEBUG
  {
    const char *env_string;
    env_string = g_getenv ("GEGL_DEBUG");
    if (env_string != NULL)
      {
        gegl_debug_flags =
          g_parse_debug_string (env_string,
                                gegl_debug_keys,
                                G_N_ELEMENTS (gegl_debug_keys));
        env_string = NULL;
      }
  }
#endif /* GEGL_ENABLE_DEBUG */

  if (cmd_gegl_swap)
    g_object_set (config, "swap", cmd_gegl_swap, NULL);
  if (cmd_gegl_swap_compression)
    g_object_set (config, "swap-compression", cmd_gegl_swap_compression, NULL);
  if (cmd_gegl_quality)
    config->quality = atof (cmd_gegl_quality);
  if (cmd_gegl_cache_size)
    {
      g_object_set (config,
                    "tile-cache-size",
                    (guint64) atoll (cmd_gegl_cache_size) * 1024 * 1024,
                    NULL);
    }
  if (cmd_gegl_chunk_size)
    config->chunk_size = atoi (cmd_gegl_chunk_size);
  if (cmd_gegl_tile_size)
    {
      const gchar *str = cmd_gegl_tile_size;
      gint         width;
      gint         height;
      width = height = atoi(str);
      str = strchr (str, 'x');
      if (str)
        height = atoi(str+1);
      g_object_set (config,
                    "tile-width",  width,
                    "tile-height", height,
                    NULL);
    }
  if (cmd_gegl_threads)
    {
      _gegl_threads = atoi (cmd_gegl_threads);
      if (_gegl_threads > GEGL_MAX_THREADS)
        {
          g_warning ("Tried to use %i threads, max is %i",
                     _gegl_threads, GEGL_MAX_THREADS);
          _gegl_threads = GEGL_MAX_THREADS;
        }
    }
  if (cmd_gegl_disable_opencl)
    gegl_cl_hard_disable ();

  GEGL_INSTRUMENT_START();

  gegl_tile_alloc_init ();
  gegl_buffer_swap_init ();
  gegl_parallel_init ();
  gegl_compression_init ();
  gegl_operation_gtype_init ();
  gegl_tile_cache_init ();

  if (!module_db)
    {
      GSList *paths = gegl_get_default_module_paths ();
      module_db = gegl_module_db_new (FALSE);
      g_slist_foreach(paths, (GFunc)load_module_path, module_db);
      g_slist_free_full (paths, g_free);
    }

  GEGL_INSTRUMENT_END ("gegl_init", "load modules");

  gegl_instrument ("gegl", "gegl_init", gegl_ticks () - global_time);

  g_signal_connect (G_OBJECT (config),
                   "notify::use-opencl",
                   G_CALLBACK (gegl_config_use_opencl_notify),
                   NULL);
  g_object_set (config, "use-opencl", config->use_opencl, NULL);

  g_signal_connect (G_OBJECT (config),
                   "notify::application-license",
                   G_CALLBACK (gegl_config_application_license_notify),
                   NULL);
  gegl_operations_set_licenses_from_string (config->application_license);

  main_thread = g_thread_self ();

  return TRUE;
}

gboolean
gegl_get_debug_enabled (void)
{
#ifdef GEGL_ENABLE_DEBUG
  return gegl_debug_flags != 0;
#else
  return FALSE;
#endif
}
