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

#define SUCCESS  0
#define FAILURE -1
#define SKIP     77

#define RUN_TEST(test_name) \
{ \
  gint retval = test_name (); \
  if (retval == SUCCESS) \
    { \
      printf ("" #test_name " ... PASS\n"); \
      tests_passed++; \
    } \
  else if (retval == SKIP) \
    { \
      printf ("" #test_name " ... SKIP\n"); \
      tests_skipped++; \
    } \
  else \
    { \
      printf ("" #test_name " ... FAIL\n"); \
      tests_failed++; \
    } \
  tests_run++; \
}

#define MORE_INFO(expected_message, expected_domain, expected_code) \
  if (status != SUCCESS) \
    { \
      fprintf (stderr, \
               "- %s: Expected error (domain: %d - code: %d): %s\n", \
               G_STRFUNC, expected_domain, expected_code, expected_message); \
      if (error) \
        fprintf (stderr, "- %s: Actual error (domain: %d - code: %d): %s\n", \
                 G_STRFUNC, error->domain, error->code, error->message); \
      else \
        fprintf (stderr, "- %s: No error message!\n", G_STRFUNC); \
    }

static gboolean save_denied      (void);
static gboolean load_incomplete  (void);
static gboolean load_zero_blit   (void);
static gboolean save_invalid_mp4 (void);

/* Trying to save in a non-writable file with gegl_node_process(). */
static gint
save_denied (void)
{
  GeglNode  *graph;
  GeglNode  *color;
  GeglNode  *crop;
  GeglNode  *save;
  GeglColor *red;
  GError    *error  = NULL;
  gchar     *path;
  gint       status = FAILURE;
  gint       fd;

  red = gegl_color_new ("rgb(1.0, 0.0, 0.0)");

  /* Create a new empty file and forbid writing. */
  fd = g_file_open_tmp (NULL, &path, NULL);
  close (fd);
  if (g_chmod (path, S_IRUSR) == -1)
    /* We cannot change file permission, thus the test cannot be done. */
    return SKIP;
  if (g_access (path, W_OK) == 0)
    /* It seems even if the chmod succeeds, on some systems, we may still have
     * write access. Check and skip if we do.
     */
    return SKIP;

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
      /* We test against error domain and code programmatically (no i18n
       * issue, string change problems, etc.).
       */
      if (error                       &&
          error->domain == G_IO_ERROR &&
          error->code == G_IO_ERROR_PERMISSION_DENIED)
        status = SUCCESS;
    }
  MORE_INFO ("Error opening file “/some/tmp/path”: Permission denied", G_IO_ERROR, G_IO_ERROR_PERMISSION_DENIED);

  g_object_unref (graph);
  g_clear_error (&error);

  /* Delete the temp file. */
  g_unlink (path);
  g_free (path);

  return status;
}

/* Trying to load a non-readable file with gegl_node_process(). */
static gint
load_incomplete (void)
{
  GeglNode   *graph;
  GeglNode   *source;
  GeglNode   *sink;
  GeglBuffer *buffer = NULL;
  GError     *error  = NULL;
  gchar      *path;
  gint        status = FAILURE;
  gint        fd;

  /* Create a file with only the PNG header.
   * It is not a valid PNG image.
   */
  fd = g_file_open_tmp (NULL, &path, NULL);
  write (fd, "\x89\x50\x4e\x47\xd\xa\x1a\xa", 8);
  close (fd);

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
      if (error                                                &&
          error->domain == g_quark_from_static_string ("gegl:load-png-error-quark") &&
          error->code == 0)
        status = SUCCESS;
    }
  MORE_INFO ("[gegl:png-load] failed to read file '/some/tmp/path': [86][7F][00][00]: invalid chunk type",
             g_quark_from_static_string ("gegl:load-png-error-quark"), 0);

  g_object_unref (graph);
  g_clear_error (&error);
  if (buffer)
    g_object_unref (buffer);

  /* Delete the temp file. */
  g_unlink (path);
  g_free (path);

  return status;
}

/* Trying to load an empty file (i.e. not valid PNG) with
 * gegl_node_blit_buffer(). */
static gint
load_zero_blit (void)
{
  GeglNode   *graph;
  GeglNode   *source;
  GeglNode   *scale;
  GeglBuffer *buffer = NULL;
  GError     *error  = NULL;
  gchar      *path;
  gint        status = FAILURE;
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
      if (error &&
          error->domain == g_quark_from_static_string ("gegl:load-png-error-quark") &&
          error->code == 0)
        status = SUCCESS;
    }
  MORE_INFO ("too short for a png file, only 0 bytes.", g_quark_from_static_string ("gegl:load-png-error-quark"), 0);

  g_object_unref (graph);
  g_clear_error (&error);
  if (buffer)
    g_object_unref (buffer);

  /* Delete the temp file. */
  g_unlink (path);
  g_free (path);

  return status;
}

/* Trying to save a mp4 video with impossible dimensions. */
static gint
save_invalid_mp4 (void)
{
  GeglNode  *graph;
  GeglNode  *color;
  GeglNode  *crop;
  GeglNode  *save;
  GeglColor *red;
  GError    *error  = NULL;
  gchar     *path;
  gint       status = FAILURE;
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
      if (error &&
          error->domain == g_quark_from_static_string ("gegl:ff-save") &&
          error->code == 0)
        status = SUCCESS;
    }
  MORE_INFO ("[libx264 @ 0x0123456] width not divisible by 2 (101x101)", g_quark_from_static_string ("gegl:ff-save"), 0);

  g_object_unref (graph);
  g_clear_error (&error);

  /* Delete the temp file. */
  g_unlink (path);
  g_free (path);

  return status;
}

int
main (int argc, char **argv)
{
  gint tests_run     = 0;
  gint tests_passed  = 0;
  gint tests_failed  = 0;
  gint tests_skipped = 0;

  gegl_init (0, NULL);
  g_object_set (G_OBJECT (gegl_config()),
                "swap",       "RAM",
                "use-opencl", FALSE,
                NULL);

  RUN_TEST (save_denied);
  RUN_TEST (load_incomplete);
  RUN_TEST (load_zero_blit);
  RUN_TEST (save_invalid_mp4);

  gegl_exit ();

  if (tests_failed > 0)
    return FAILURE;
  else if (tests_passed > 0)
    return SUCCESS;
  else
    return SKIP;
}
