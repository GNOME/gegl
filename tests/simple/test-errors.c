/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"
#include <stdio.h>
#include <unistd.h>

#include <glib/gstdio.h>
#include <gio/gio.h>

#include "gegl.h"

static gboolean save_denied    (void);
static gboolean load_denied    (void);
static gboolean load_zero_blit (void);

/* Trying to save in a non-writable file with gegl_node_process(). */
static gboolean
save_denied (void)
{
  GeglNode  *graph;
  GeglNode  *color;
  GeglNode  *crop;
  GeglNode  *save;
  GeglColor *red;
  GError    *error   = NULL;
  gchar     *path;
  gboolean   success = FALSE;
  gint       fd;

  red = gegl_color_new ("rgb(1.0, 0.0, 0.0)");

  /* Create a new empty file and forbid writing. */
  fd = g_file_open_tmp (NULL, &path, NULL);
  close (fd);
  g_chmod (path, S_IRUSR);

  /* Try to save. */
  graph = gegl_node_new ();
  color = gegl_node_new_child (graph,
                               "operation", "gegl:color",
                               "value",     red,
                               NULL);
  crop = gegl_node_new_child (graph,
                              "operation", "gegl:crop",
                              "width", 100.0,
                              "height", 100.0,
                              NULL);
  save = gegl_node_new_child (graph,
                              "operation", "gegl:png-save",
                              "path",      path,
                              NULL);
  gegl_node_link_many (color, crop, save, NULL);

  gegl_node_process (save);
  if (! gegl_node_process_success (save, &error))
    {
      /* Expected error is "Error opening file “/tmp/.ZBD4YZ”: Permission denied"
       * We test against error domain and code programmatically (no i18n
       * issue, or string change problems, etc.
       */
      success = (error                       &&
                 error->domain == G_IO_ERROR &&
                 error->code == G_IO_ERROR_PERMISSION_DENIED);
    }

  g_object_unref (graph);
  g_clear_error (&error);

  /* Delete the temp file. */
  g_unlink (path);
  g_free (path);

  return success;
}

/* Trying to load a non-readable file with gegl_node_process(). */
static gboolean
load_denied (void)
{
  GeglNode   *graph;
  GeglNode   *source;
  GeglNode   *sink;
  GeglBuffer *buffer  = NULL;
  GError     *error   = NULL;
  gchar      *path;
  gboolean    success = FALSE;
  gint        fd;

  /* Create a new empty file. It is not a valid image but we don't care as we
   * are going to make it non-readable anyway.
   */
  fd = g_file_open_tmp (NULL, &path, NULL);
  close (fd);
  g_chmod (path, 0);

  /* Try to load it in a buffer. */
  graph = gegl_node_new ();
  source = gegl_node_new_child (graph,
                                "operation", "gegl:png-load",
                                "path",      path,
                                NULL);
  sink = gegl_node_new_child (graph,
                              "operation", "gegl:buffer-sink",
                              "buffer",    &buffer,
                              NULL);
  gegl_node_link (source, sink);

  gegl_node_process (sink);
  if (! gegl_node_process_success (sink, &error))
    {
      /* Expected error is "Error opening file “/tmp/.ZBD4YZ”: Permission denied" */
      success = (error                       &&
                 error->domain == G_IO_ERROR &&
                 error->code == G_IO_ERROR_PERMISSION_DENIED);
    }

  g_object_unref (graph);
  g_clear_error (&error);
  if (buffer)
    g_object_unref (buffer);

  /* Delete the temp file. */
  g_unlink (path);
  g_free (path);

  return success;
}

/* Trying to load an empty file (i.e. not valid PNG) with
 * gegl_node_blit_buffer(). */
static gboolean
load_zero_blit (void)
{
  GeglNode   *graph;
  GeglNode   *source;
  GeglNode   *scale;
  GeglBuffer *buffer  = NULL;
  GError     *error   = NULL;
  gchar      *path;
  gboolean    success = FALSE;
  gint        fd;

  /* Create a new empty file. It is not a valid PNG image. */
  fd = g_file_open_tmp (NULL, &path, NULL);
  close (fd);

  /* Try to load it in a buffer. */
  graph = gegl_node_new ();
  source = gegl_node_new_child (graph,
                                "operation", "gegl:png-load",
                                "path",      path,
                                NULL);
  scale = gegl_node_new_child (graph,
                               "operation", "gegl:scale-ratio",
                               "x",    2.0,
                               "y",    2.0,
                               NULL);
  gegl_node_link (source, scale);
  gegl_node_blit_buffer (scale,
                         buffer,
                         NULL,
                         0,
                         GEGL_ABYSS_NONE);

  if (! gegl_node_process_success (scale, &error))
    {
      /* Expected error: "too short for a png file, only 0 bytes." */
      success = (error &&
                 error->domain == g_quark_from_static_string ("gegl:load-png-error-quark") &&
                 error->code == 0);
    }

  g_object_unref (graph);
  g_clear_error (&error);
  if (buffer)
    g_object_unref (buffer);

  /* Delete the temp file. */
  g_unlink (path);
  g_free (path);

  return success;
}

/* Trying to save a mp4 video with impossible dimensions. */
static gboolean
save_invalid_mp4 (void)
{
  GeglNode  *graph;
  GeglNode  *color;
  GeglNode  *crop;
  GeglNode  *save;
  GeglColor *red;
  GError    *error   = NULL;
  gchar     *path;
  gboolean   success = FALSE;
  gint       fd;

  red = gegl_color_new ("rgb(1.0, 0.0, 0.0)");

  /* Create a new empty file. */
  fd = g_file_open_tmp ("XXXXXX.mp4", &path, NULL);
  close (fd);

  /* Try to save. */
  graph = gegl_node_new ();
  color = gegl_node_new_child (graph,
                               "operation", "gegl:color",
                               "value",     red,
                               NULL);
  crop = gegl_node_new_child (graph,
                              "operation", "gegl:crop",
                              "width", 101.0,
                              "height", 101.0,
                              NULL);
  save = gegl_node_new_child (graph,
                              "operation", "gegl:ff-save",
                              "path",      path,
                              NULL);
  gegl_node_link_many (color, crop, save, NULL);

  gegl_node_process (save);
  if (! gegl_node_process_success (save, &error))
    {
      /* Expected error: [libx264 @ 0x1e94680] width not divisible by 2 (101x101)
       * libx264 does not allow odd dimensions for MP4 format and therefore the
       * export to video should fail.
       */
      success = (error &&
                 error->domain == g_quark_from_static_string ("gegl:ff-save") &&
                 error->code == 0);
    }

  g_object_unref (graph);
  g_clear_error (&error);

  /* Delete the temp file. */
  g_unlink (path);
  g_free (path);

  return success;
}

int
main (int argc, char **argv)
{
  int success;

  gegl_init (0, NULL);
  g_object_set (G_OBJECT (gegl_config()),
                "swap",       "RAM",
                "use-opencl", FALSE,
                NULL);

  if (save_denied ()    &&
      load_denied ()    &&
      load_zero_blit () &&
      save_invalid_mp4 ())
    success = 0;
  else
    success = -1;

  gegl_exit ();

  return success;
}
