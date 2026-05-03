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
 * Copyright 2010 Danny Robson <danny@blubinc.net>
 */

#include "config.h"

#include <stdio.h>

#include <glib/gi18n-lib.h>
#include <gegl.h>
#include <gegl-metadata.h>

#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
    description(_("Path of file to save."))
property_object (metadata, _("Metadata"), GEGL_TYPE_METADATA)
    description (_("Object providing image metadata"))

#else

typedef struct _SaveOpData
{
  GeglNode *input;
  GeglNode *save;
  gchar    *cached_path;
} SaveOpData;

#define GEGL_OP_SINK
#define GEGL_OP_NAME        save
#define GEGL_OP_C_SOURCE    save.c
#include "gegl-op.h"

static void
gegl_save_set_saver (GeglOperation *operation)
{
  const gchar    *extension = NULL;
  const gchar    *handler   = NULL;
  GeglProperties *props     = GEGL_PROPERTIES (operation);
  SaveOpData     *data      = (SaveOpData *) (props->user_data);

  /* If prepare has already been invoked, bail out early */
  if (data->cached_path && props->path && !strcmp (props->path, data->cached_path))
      return;
  if (props->path == NULL || *props->path == '\0')
    return;
  g_free (data->cached_path);

  /* Find an extension handler and put it into the graph. The path can be
   * empty, which indicates an error, but not null. Ideally we'd like to
   * report an error here, but there's no way to pass it back to GEGL or the
   * user in prepare()...
   */
  g_assert (props->path);
  extension = strrchr (props->path, '.');
  handler   = extension ? gegl_operation_handlers_get_saver (extension) : NULL;

  if (handler)
    {
      gegl_node_set (data->save,
                     "operation", handler,
                     "path",      props->path,
                     NULL);

      if (props->metadata &&
          gegl_operation_find_property (handler, "metadata") != NULL)
        gegl_node_set (data->save, "metadata", props->metadata, NULL);

    }
  else
    {
      g_warning ("Unable to find suitable save handler for path '%s'", props->path);
      gegl_node_set (data->save,
                     "operation", "gegl:nop",
                     NULL);
    }

  data->cached_path = g_strdup (props->path);
}


/* Create an input proxy, and temporary save operation, and link together.
 * These will be passed control when gegl_save_process is called later.
 */
static void
gegl_save_attach (GeglOperation *operation)
{
  GeglProperties *props = GEGL_PROPERTIES (operation);
  SaveOpData     *data  = (SaveOpData *) (props->user_data);

  /* const gchar *nodename; */
  /* gchar       *childname; */

  g_assert (!data->input);
  g_assert (!data->save);
  g_assert (!data->cached_path);

  /* Initialise and attach child nodes */
  data->input  = gegl_node_get_input_proxy (operation->node, "input");
  data->save   = gegl_node_new_child (operation->node,
                                      "operation", "gegl:nop",
                                      NULL);

  /* Set some debug names for the child nodes */
  /*  nodename = gegl_node_get_debug_name (operation->node);
  g_assert (nodename);

  childname = g_strconcat (nodename, "-save", NULL);
  gegl_node_set_name (self->input, childname);
  g_free (childname);

  childname = g_strconcat (nodename, "-input", NULL);
  gegl_node_set_name (self->input, childname);
  g_free (childname);*/

  /* Link the saving node and attempt to set an appropriate save operation,
   * might as well at least try to do this before prepare.
   */
  gegl_node_link (data->input, data->save);
  gegl_save_set_saver (operation);
}

static void
gegl_save_set_property (GObject      *gobject,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GeglOperation  *operation = GEGL_OPERATION (gobject);
  GeglProperties *props     = GEGL_PROPERTIES (operation);
  SaveOpData     *data      = NULL;

  if (props->user_data == NULL)
    props->user_data = g_new0 (SaveOpData, 1);
  data = props->user_data;

  /* The set_property provided by the chant system does the
   * storing and reffing/unreffing of the input properties */
  set_property (gobject, property_id, value, pspec);

  if (data->save)
    gegl_save_set_saver (operation);
}

static gboolean
gegl_save_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_pad,
                   const GeglRectangle  *roi,
                   gint                  level)
{
  GeglProperties *props = GEGL_PROPERTIES (operation);
  SaveOpData     *data  = (SaveOpData *) (props->user_data);

  return gegl_operation_process (gegl_node_get_gegl_operation (data->save),
                                 context,
                                 output_pad,
                                 roi,
                                 level);
}

static void
gegl_save_dispose (GObject *object)
{
  GeglProperties *props = GEGL_PROPERTIES (GEGL_OPERATION (object));
  SaveOpData     *data  = (SaveOpData *) (props->user_data);

  g_clear_pointer (&data->cached_path, g_free);

  G_OBJECT_CLASS (gegl_op_parent_class)->dispose (object);
}

static void
gegl_save_finalize (GObject *object)
{
  GeglProperties *props = GEGL_PROPERTIES (GEGL_OPERATION (object));
  SaveOpData     *data  = (SaveOpData *) (props->user_data);

  g_free (data);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass           *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationSinkClass *sink_class      = GEGL_OPERATION_SINK_CLASS (klass);
  GeglOperationClass     *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->dispose      = gegl_save_dispose;
  object_class->finalize     = gegl_save_finalize;
  object_class->set_property = gegl_save_set_property;

  operation_class->attach  = gegl_save_attach;
  operation_class->process = gegl_save_process;

  sink_class->needs_full = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:save",
    "title",       _("Save"),
    "categories" , "meta:output",
    "description",
        _("Multipurpose file saver, that uses other native save handlers depending on extension, use the format specific save ops to specify additional parameters."),
        NULL);
}

#endif

