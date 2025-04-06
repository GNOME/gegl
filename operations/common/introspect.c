/* This file is an image processing operation for GEGL
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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <stdlib.h>
#include <glib/gi18n-lib.h>
#include <unistd.h>


#ifdef GEGL_PROPERTIES

property_object(node, _("Node"), GEGL_TYPE_NODE)

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_NAME     introspect
#define GEGL_OP_C_SOURCE introspect.c

#include "gegl-op.h"
gchar *gegl_to_dot                       (GeglNode       *node);
#include <stdio.h>

static void
gegl_introspect_load_cache (GeglProperties *op_introspect)
{
  gchar  *argv[6]      = {"dot", "-o", NULL, "-Tpng", NULL, NULL};
  GError *error        = NULL;
  gchar  *dot_string   = NULL;
  gchar  *png_filename = NULL;
  gchar  *dot_filename = NULL;
  gchar  *dot_cmd      = NULL;
  gchar  *dot;
  gint    fd;

  dot = g_find_program_in_path ("dot");

  if (! dot || op_introspect->user_data || op_introspect->node == NULL)
    return;

  /* Construct temp filenames */
  dot_filename = g_build_filename (g_get_tmp_dir (), "gegl-introspect-XXXXXX.dot", NULL);
  png_filename = g_build_filename (g_get_tmp_dir (), "gegl-introspect-XXXXXX.png", NULL);

  /* Construct the .dot source */
  fd = g_mkstemp (dot_filename);
  dot_string = gegl_to_dot (GEGL_NODE (op_introspect->node));
  write (fd, dot_string, strlen (dot_string));
  close (fd);

  /* The only point of using g_mkstemp() here is creating a new file and making
   * sure we don't override a file which existed before.
   * Also png_filename will be modified in-place to the actual path name
   * generated as being unique.
   */
  fd = g_mkstemp (png_filename);
  close (fd);

  /* Process the .dot to a .png */
  argv[2] = png_filename;
  argv[4] = dot_filename;
  if (! g_spawn_sync (NULL, argv, NULL, G_SPAWN_SEARCH_PATH,
                      NULL, NULL, NULL, NULL, NULL, &error))
    {
      g_warning ("Error executing GraphViz dot program: %s", error->message);
      g_clear_error (&error);
    }
  else
    {
      GeglBuffer *new_buffer   = NULL;
      GeglNode   *png_load     = NULL;
      GeglNode   *buffer_sink  = NULL;

      /* Create a graph that loads the png into a GeglBuffer and process
       * it
       */
      png_load = gegl_node_new_child (NULL,
                                      "operation", "gegl:png-load",
                                      "path",      png_filename,
                                      NULL);
      buffer_sink = gegl_node_new_child (NULL,
                                         "operation", "gegl:buffer-sink",
                                         "buffer",    &new_buffer,
                                         NULL);
      gegl_node_link_many (png_load, buffer_sink, NULL);
      gegl_node_process (buffer_sink);

      op_introspect->user_data= new_buffer;

      g_object_unref (buffer_sink);
      g_object_unref (png_load);
    }

  /* Do not keep the files around. */
  unlink (dot_filename);
  unlink (png_filename);

  /* Cleanup */
  g_free (dot);
  g_free (dot_string);
  g_free (dot_cmd);
  g_free (dot_filename);
  g_free (png_filename);
}

static void
gegl_introspect_dispose (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);

  g_clear_object (&o->user_data);

  G_OBJECT_CLASS (gegl_op_parent_class)->dispose (object);
}

static GeglRectangle
gegl_introspect_get_bounding_box (GeglOperation *operation)
{
  GeglRectangle   result = {0,0,0,0};
  GeglProperties *o = GEGL_PROPERTIES (operation);

  gegl_introspect_load_cache (o);

  if (o->user_data)
    {
      gint width, height;

      g_object_get (o->user_data,
                    "width",  &width,
                    "height", &height,
                    NULL);

      result.width  = width;
      result.height = height;
    }

  return result;
}

static gboolean
gegl_introspect_process (GeglOperation        *operation,
                         GeglOperationContext *context,
                         const gchar          *output_pad,
                         const GeglRectangle  *result,
                         gint                  level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  gegl_introspect_load_cache (o);

  if (!o->user_data)
    return FALSE;

  /* gegl_operation_context_take_object() takes the reference we have,
   * so we must increase it since we want to keep the object
   */
  g_object_ref (o->user_data);

  gegl_operation_context_take_object (context, output_pad, G_OBJECT (o->user_data));

  return  TRUE;
}

static gboolean
gegl_introspect_is_available (void)
{
  gchar    *dot;
  gboolean  found = FALSE;

  dot = g_find_program_in_path ("dot");
  found = (dot != NULL);
  g_free (dot);

  return found;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass             *object_class;
  GeglOperationClass       *operation_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->dispose             = gegl_introspect_dispose;

  operation_class->process          = gegl_introspect_process;
  operation_class->get_bounding_box = gegl_introspect_get_bounding_box;
  operation_class->is_available     = gegl_introspect_is_available;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:introspect",
    "categories" , "render",
    "description", _("GEGL graph visualizer."),
    NULL);
}

#endif
