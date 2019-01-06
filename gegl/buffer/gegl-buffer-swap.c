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

#include <stdlib.h>
#include <string.h>

#include <glib/gstdio.h>

#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef G_OS_WIN32
#include <windows.h>
#include <process.h>
#define getpid() _getpid()
#else
#include <signal.h>
#endif

#include "gegl-buffer-config.h"
#include "gegl-buffer-swap.h"
#include "gegl-buffer-swap-private.h"


#define SWAP_PREFIX        "gegl-swap-"

/* this used to be the suffix for swap files before commit
 * b61f9015bf19611225df9832db3cfd9ee2558fc9.  let's keep
 * cleaning files that match this suffix on startup, at least
 * for a while.
 */
#define SWAP_LEGACY_SUFFIX "-shared.swap"


/*  local function prototypes  */

static void       gegl_buffer_swap_notify_swap    (GeglBufferConfig *config);

static void       gegl_buffer_swap_clean_dir      (void);
static gboolean   gegl_buffer_swap_pid_is_running (gint              pid);


/*  local variables  */

static GMutex      swap_mutex;
static gchar      *swap_dir;
static GHashTable *swap_files;
static guint       swap_file_counter;


/*  public functions  */

void
gegl_buffer_swap_init (void)
{
  swap_files = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  g_signal_connect (gegl_buffer_config (), "notify::swap",
                    G_CALLBACK (gegl_buffer_swap_notify_swap),
                    NULL);

  gegl_buffer_swap_notify_swap (gegl_buffer_config ());
}

void
gegl_buffer_swap_cleanup (void)
{
  GHashTableIter  iter;
  const gchar    *path;

  g_signal_handlers_disconnect_by_func (gegl_buffer_config (),
                                        gegl_buffer_swap_notify_swap,
                                        NULL);

  g_mutex_lock (&swap_mutex);

  g_hash_table_iter_init (&iter, swap_files);

  while (g_hash_table_iter_next (&iter, (gpointer) &path, NULL))
    g_unlink (path);

  g_clear_pointer (&swap_files, g_hash_table_destroy);

  g_clear_pointer (&swap_dir, g_free);

  g_mutex_unlock (&swap_mutex);
}

gchar *
gegl_buffer_swap_create_file (const gchar *suffix)
{
  gchar    *basename;
  gchar    *path;
  gboolean  added;

  if (! swap_dir)
    return NULL;

  g_mutex_lock (&swap_mutex);

  if (! swap_dir)
    {
      g_mutex_unlock (&swap_mutex);

      return NULL;
    }

  if (suffix)
    {
      basename = g_strdup_printf (SWAP_PREFIX "%d-%u-%s",
                                  (gint) getpid (),
                                  swap_file_counter++,
                                  suffix);
    }
  else
    {
      basename = g_strdup_printf (SWAP_PREFIX "%d-%u",
                                  (gint) getpid (),
                                  swap_file_counter++);
    }

  path = g_build_filename (swap_dir, basename, NULL);

  added = g_hash_table_add (swap_files, path);

  g_mutex_unlock (&swap_mutex);

  g_free (basename);

  if (! added)
    {
      g_warning ("swap file collision '%s'", path);

      g_free (path);

      return NULL;
    }

  return g_strdup (path);
}

void
gegl_buffer_swap_remove_file (const gchar *path)
{
  gboolean removed;

  g_return_if_fail (path != NULL);

  g_mutex_lock (&swap_mutex);

  removed = g_hash_table_remove (swap_files, path);

  g_mutex_unlock (&swap_mutex);

  if (removed)
    g_unlink (path);
  else
    g_warning ("attempt to remove unregistered swap file '%s'", path);
}

gboolean
gegl_buffer_swap_has_file (const gchar *path)
{
  gboolean found;

  g_return_val_if_fail (path != NULL, FALSE);

  g_mutex_lock (&swap_mutex);

  found = (g_hash_table_lookup (swap_files, path) != NULL);

  g_mutex_unlock (&swap_mutex);

  return found;
}


/*  private functions  */

static void
gegl_buffer_swap_notify_swap (GeglBufferConfig *config)
{
  gchar *dir = config->swap;

  if (dir)
    {
      dir = g_strstrip (g_strdup (dir));

      /* Remove any trailing separator, unless the path is only made of a
       * leading separator.
       */
      while (strlen (dir) > strlen (G_DIR_SEPARATOR_S) &&
             g_str_has_suffix (dir, G_DIR_SEPARATOR_S))
        {
          dir[strlen (dir) - strlen (G_DIR_SEPARATOR_S)] = '\0';
        }
    }

  g_mutex_lock (&swap_mutex);

  if (! g_strcmp0 (dir, swap_dir))
    {
      g_mutex_unlock (&swap_mutex);

      g_free (dir);

      return;
    }

  g_clear_pointer (&swap_dir, g_free);

  if (dir                                     &&
      ! g_file_test (dir, G_FILE_TEST_IS_DIR) &&
      g_mkdir_with_parents (dir, S_IRUSR | S_IWUSR | S_IXUSR) != 0)
    {
      g_mutex_unlock (&swap_mutex);

      g_free (dir);

      return;
    }

  swap_dir = dir;

  gegl_buffer_swap_clean_dir ();

  g_mutex_unlock (&swap_mutex);

  return;
}

static void
gegl_buffer_swap_clean_dir (void)
{
  GDir *dir;

  if (! swap_dir)
    return;

  dir = g_dir_open (swap_dir, 0, NULL);

  if (dir != NULL)
    {
      const gchar *basename;

      while ((basename = g_dir_read_name (dir)) != NULL)
        {
          gint pid = 0;

          if (g_str_has_prefix (basename, SWAP_PREFIX))
            pid = atoi (basename + strlen (SWAP_PREFIX));
          else if (g_str_has_suffix (basename, SWAP_LEGACY_SUFFIX))
            pid = atoi (basename);

          if (pid && ! gegl_buffer_swap_pid_is_running (pid))
            {
              gchar *path = g_build_filename (swap_dir, basename, NULL);

              g_unlink (path);

              g_free (path);
            }
         }

      g_dir_close (dir);
    }
}

#ifdef G_OS_WIN32

static gboolean
gegl_buffer_swap_pid_is_running (gint pid)
{
  HANDLE h;
  DWORD exitcode = 0;

  h = OpenProcess (PROCESS_QUERY_INFORMATION, FALSE, pid);
  GetExitCodeProcess (h, &exitcode);
  CloseHandle (h);

  return exitcode == STILL_ACTIVE;
}

#else

static gboolean
gegl_buffer_swap_pid_is_running (gint pid)
{
  return kill (pid, 0) == 0;
}

#endif
