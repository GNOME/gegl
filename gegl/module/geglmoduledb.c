/* This file is part of GEGL
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
 */

#include "config.h"
#include <string.h>

#include <glib-object.h>
#include "gegl-plugin.h"
#include "geglmodule.h"
#include "geglmoduledb.h"
#include "gegldatafiles.h"
#include "gegl-cpuaccel.h"
#include "gegl-config.h"


#ifdef ARCH_X86_64
#define ARCH_SIMD
#endif
#ifdef ARCH_ARM
#define ARCH_SIMD
#endif

#ifdef __APPLE__ /* G_MODULE_SUFFIX is defined to .so instead of .dylib */
#define MODULE_SUFFIX "dylib"
#else
#define MODULE_SUFFIX G_MODULE_SUFFIX
#endif

enum
{
  ADD,
  REMOVE,
  MODULE_MODIFIED,
  LAST_SIGNAL
};


static void         gegl_module_db_finalize            (GObject      *object);

static void         gegl_module_db_module_search       (const GeglDatafileData *file_data,
                                                        gpointer                user_data);

static void         gegl_module_db_module_modified     (GeglModule   *module,
                                                        GeglModuleDB *db);


G_DEFINE_TYPE (GeglModuleDB, gegl_module_db, G_TYPE_OBJECT)

#define parent_class gegl_module_db_parent_class

static guint db_signals[LAST_SIGNAL] = { 0 };


static void
gegl_module_db_class_init (GeglModuleDBClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  db_signals[ADD] =
    g_signal_new ("add",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GeglModuleDBClass, add),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_MODULE);

  db_signals[REMOVE] =
    g_signal_new ("remove",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GeglModuleDBClass, remove),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_MODULE);

  db_signals[MODULE_MODIFIED] =
    g_signal_new ("module-modified",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GeglModuleDBClass, module_modified),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_MODULE);

  object_class->finalize = gegl_module_db_finalize;

  klass->add             = NULL;
  klass->remove          = NULL;
}

static void
gegl_module_db_init (GeglModuleDB *db)
{
  db->modules      = NULL;
  db->load_inhibit = NULL;
  db->verbose      = FALSE;
}

static void
gegl_module_db_finalize (GObject *object)
{
  GeglModuleDB *db = GEGL_MODULE_DB (object);

  g_list_free (db->modules);
  g_free (db->load_inhibit);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * gegl_module_db_new:
 * @verbose: Pass %TRUE to enable debugging output.
 *
 * Creates a new #GeglModuleDB instance. The @verbose parameter will be
 * passed to the created #GeglModule instances using gegl_module_new().
 *
 * Return value: The new #GeglModuleDB instance.
 **/
GeglModuleDB *
gegl_module_db_new (gboolean verbose)
{
  GeglModuleDB *db;

  db = g_object_new (GEGL_TYPE_MODULE_DB, NULL);

  db->verbose = verbose ? TRUE : FALSE;

  return db;
}

static gboolean
is_in_inhibit_list (const gchar *filename,
                    const gchar *inhibit_list)
{
  gchar       *p;
  gint         pathlen;
  const gchar *start;
  const gchar *end;

  if (! inhibit_list || ! strlen (inhibit_list))
    return FALSE;

  p = strstr (inhibit_list, filename);
  if (!p)
    return FALSE;

  /* we have a substring, but check for colons either side */
  start = p;
  while (start != inhibit_list && *start != G_SEARCHPATH_SEPARATOR)
    start--;

  if (*start == G_SEARCHPATH_SEPARATOR)
    start++;

  end = strchr (p, G_SEARCHPATH_SEPARATOR);
  if (! end)
    end = inhibit_list + strlen (inhibit_list);

  pathlen = strlen (filename);

  if ((end - start) == pathlen)
    return TRUE;

  return FALSE;
}

#ifdef ARCH_SIMD

static gboolean
gegl_str_has_one_of_suffixes (const char *str,
                              char **suffixes)
{
  for (int i = 0; suffixes[i]; i++)
  {
    if (g_str_has_suffix (str, suffixes[i]))
      return TRUE;
  }
  return FALSE;
}

static void
gegl_module_db_remove_duplicates (GeglModuleDB *db)
{
#ifdef ARCH_X86_64

  char *suffix_list[] = {"-x86_64-v2." MODULE_SUFFIX,"-x86_64-v3." MODULE_SUFFIX, NULL};

  GList *suffix_entries = NULL;
  int preferred = -1;
  GeglCpuAccelFlags cpu_accel = gegl_cpu_accel_get_support ();
  if (cpu_accel & GEGL_CPU_ACCEL_X86_64_V3) preferred = 1;
  else if (cpu_accel & GEGL_CPU_ACCEL_X86_64_V2) preferred = 0;

#endif
#ifdef ARCH_ARM
  char *suffix_list[] = {"-arm-neon." MODULE_SUFFIX, NULL};

  GList *suffix_entries = NULL;
  int preferred = -1;

  GeglCpuAccelFlags cpu_accel = gegl_cpu_accel_get_support ();
  if (cpu_accel & GEGL_CPU_ACCEL_ARM_NEON) preferred = 0;
#endif

  for (GList *l = db->to_load; l; l = l->next)
  {
    char *filename = l->data;
    if (gegl_str_has_one_of_suffixes (filename, suffix_list))
    {
       suffix_entries = g_list_prepend (suffix_entries, filename);
    }
  }

  for (GList *l = suffix_entries; l; l = l->next)
  {
    db->to_load = g_list_remove (db->to_load, l->data);
  }

  if (preferred>-1)
  {
  
  for (GList *l = suffix_entries; l; l = l->next)
  {
    char *filename = l->data;
    if (g_str_has_suffix (filename, suffix_list[preferred]))
    {
       char *expected = g_strdup (filename);
       char *e = strrchr (expected, '.');
       char *p = e;
       while (p && p>expected && *p != 'x' ) p--;
       if (p && *p == 'x' && p[-1] == '-'){
         p--;
         strcpy (p, e);
       }
       for (GList *l2 = db->to_load; l2; l2=l2->next)
       {
         char *filename2 = l2->data;
         if (!strcmp (filename2, expected))
         {
           g_free (l2->data);
           l2->data = g_strdup (filename);
         }
       }
       g_free (expected);
    }
  }

  }

  g_list_free_full(suffix_entries, g_free);
}
#endif

/**
 * gegl_module_db_load:
 * @db:          A #GeglModuleDB.
 * @module_path: A #G_SEARCHPATH_SEPARATOR delimited list of directories
 *               to load modules from.
 *
 * Scans the directories contained in @module_path using
 * gegl_datafiles_read_directories() and creates a #GeglModule
 * instance for every loadable module contained in the directories.
 **/
void
gegl_module_db_load (GeglModuleDB *db,
                     const gchar  *module_path)
{
  g_return_if_fail (GEGL_IS_MODULE_DB (db));
  g_return_if_fail (module_path != NULL);

  if (g_module_supported ())
  {
    GeglModule   *module;
    gboolean load_inhibit;

    gegl_datafiles_read_directories (module_path,
                                     G_FILE_TEST_EXISTS,
                                     gegl_module_db_module_search,
                                     db);
#ifdef ARCH_SIMD
    gegl_module_db_remove_duplicates (db);
#endif
    while (db->to_load)
    {
      char *filename = db->to_load->data;
      load_inhibit = is_in_inhibit_list (filename,
                                         db->load_inhibit);

      module = gegl_module_new (filename,
                                load_inhibit,
                                db->verbose);

      g_signal_connect (module, "modified",
                        G_CALLBACK (gegl_module_db_module_modified),
                        db);

      db->modules = g_list_append (db->modules, module);
      g_signal_emit (db, db_signals[ADD], 0, module);
      db->to_load = g_list_remove (db->to_load, filename);
      g_free (filename);
    }
  }

}

/* name must be of the form lib*.so (Unix) or *.dll (Win32) */
static gboolean
valid_module_name (const gchar *filename)
{
  gchar *basename = g_path_get_basename (filename);

  if (gegl_config()->application_license == NULL             ||
      (strcmp (gegl_config ()->application_license, "GPL3") &&
       strcmp (gegl_config ()->application_license, "GPL3+")))
    {
      if (strstr (basename, "-gpl3"))
        {
          g_free (basename);
          return FALSE;
        }
    }
#ifdef __APPLE__ /* G_MODULE_SUFFIX is defined to .so instead of .dylib */
  if (! g_str_has_suffix (basename, ".dylib" ) || strstr (filename, ".dSYM"))
#else
  if (! g_str_has_suffix (basename, "." G_MODULE_SUFFIX))
#endif
    {
      g_free (basename);

      return FALSE;
    }

  g_free (basename);

  return TRUE;
}



static void
gegl_module_db_module_search (const GeglDatafileData *file_data,
                              gpointer                user_data)
{
  GeglModuleDB *db = GEGL_MODULE_DB (user_data);

  if (! valid_module_name (file_data->filename))
    return;

  db->to_load = g_list_prepend (db->to_load, g_strdup (file_data->filename));
}

static void
gegl_module_db_module_modified (GeglModule   *module,
                                GeglModuleDB *db)
{
  g_signal_emit (db, db_signals[MODULE_MODIFIED], 0, module);
}
