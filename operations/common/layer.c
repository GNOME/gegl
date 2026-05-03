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

/* FIXME: need to make this OpenRaster inspired layer integrate better
 * with the newer caching system
 */


#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_string(composite_op, _("Operation"), "gegl:over")
    description (_("Composite operation to use"))

property_double(opacity, _("Opacity"), 1.0)
    value_range (0.0, 1.0)

property_double(x, _("X"), 0.0)
    description (_("Horizontal position in pixels"))
    ui_meta     ("axis", "x")
    ui_meta     ("unit", "pixel-coordinate")

property_double(y, _("Y"), 0.0)
    description (_("Vertical position in pixels"))
    ui_meta     ("axis", "y")
    ui_meta     ("unit", "pixel-coordinate")

property_double(scale, _("Scale"), 1.0)
    description (_("Scale 1:1 size"))

property_file_path(src, _("Source"), "")
    description (_("Source image file path (png, jpg, raw, svg, bmp, tif, ...)"))

#else

#include <gegl.h>

typedef struct _LayerOpData
{
  GeglNode *input;
  GeglNode *aux;
  GeglNode *output;

  GeglNode *composite_op;
  GeglNode *translate;
  GeglNode *opacity;
  GeglNode *scale;
  GeglNode *load;

  gchar *cached_path;

  gdouble p_opacity;
  gdouble p_scale;
  gdouble p_x;
  gdouble p_y;
  gchar  *p_composite_op;
} LayerOpData;

#define GEGL_OP_META
#define GEGL_OP_NAME        layer
#define GEGL_OP_C_SOURCE    layer.c
#include "gegl-op.h"

#include <glib/gprintf.h>

static void
update_graph (GeglOperation *operation)
{
  GeglProperties *props = GEGL_PROPERTIES (operation);
  LayerOpData     *data  = (LayerOpData *) props->user_data;

  /* If the src is NULL, and we previously used a source, clear what we have
   * cached and directly link the input and output. We don't need a composite
   * operation if we don't have a source, so don't continue preparing.
   */
  if (props->src[0] == 0)
  {
    if (data->cached_path != NULL)
    {
      gegl_node_link (data->input, data->output);
      g_clear_pointer (&data->cached_path, g_free);
    }

    return;
  }

  /* Check if the composite operation we're using has changed from that which
   * is already in use.
   */
  if (!data->p_composite_op || strcmp (data->p_composite_op, props->composite_op))
  {
    gegl_node_set (data->composite_op,
                   "operation", props->composite_op,
                   NULL);
    g_free (data->p_composite_op);
    data->p_composite_op = g_strdup (props->composite_op);
  }

  /* Load a src image, and relink the input/composite/output chain, as it
   * will currently be set to an input/output chain without a composite
   * source.
   */

  if (data->cached_path == NULL || strcmp (props->src, data->cached_path))
  {
    gegl_node_set (data->load,
                   "operation", "gegl:load",
                   NULL);
    gegl_node_set (data->load,
                   "path", props->src,
                   NULL);

    /* Currently not using the composite op, reinsert it */
    if (!data->cached_path)
      gegl_node_link_many (data->input, data->composite_op, data->output, NULL);

    g_free (data->cached_path);
    data->cached_path = g_strdup (props->src);
  }

  if (props->scale != data->p_scale)
  {
    gegl_node_set (data->scale,
                   "x",  props->scale,
                   "y",  props->scale,
                   NULL);
    data->p_scale= props->scale;
  }

  if (props->opacity != data->p_opacity)
  {
    gegl_node_set (data->opacity,
                   "value",  props->opacity,
                   NULL);
    data->p_opacity = props->opacity;
  }

  if (props->x != data->p_x ||
    props->y != data->p_y)
  {
    gegl_node_set (data->translate,
                   "x",  props->x,
                   "y",  props->y,
                   NULL);
    data->p_x = props->x;
    data->p_y = props->y;
  }


}

static void attach (GeglOperation *operation)
{
  GeglProperties *props = GEGL_PROPERTIES (operation);
  GeglNode       *gegl  = GEGL_NODE (operation);
  LayerOpData    *data  = NULL;

  if (props->user_data == NULL)
    props->user_data = g_new0 (LayerOpData, 1);
  data = props->user_data;

  data->input  = gegl_node_get_input_proxy (gegl, "input");
  data->aux    = gegl_node_get_input_proxy (gegl, "aux");
  data->output = gegl_node_get_output_proxy (gegl, "output");

  data->composite_op = gegl_node_new_child (gegl,
                                            "operation", props->composite_op,
                                            NULL);

  data->translate = gegl_node_new_child (gegl, "operation", "gegl:translate", NULL);
  data->scale     = gegl_node_new_child (gegl, "operation", "gegl:scale-ratio", NULL);
  data->opacity   = gegl_node_new_child (gegl, "operation", "gegl:opacity", NULL);

  data->load = gegl_node_new_child (gegl,
                                    "operation", "gegl:text",
                                    "string", "Load operation placeholder",
                                    NULL);

  gegl_node_link_many (data->load, data->scale, data->opacity, data->translate,
                       NULL);
  gegl_node_link_many (data->input, data->composite_op, data->output, NULL);
  gegl_node_connect (data->composite_op, "aux", data->translate, "output");
}


static void
finalize (GObject *object)
{
  LayerOpData *data = (LayerOpData *) (GEGL_PROPERTIES (object)->user_data);

  if (data != NULL)
    {
      g_free (data->cached_path);
      g_free (data->p_composite_op);
    }
  g_free (data);

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass           *object_class;
  GeglOperationClass     *operation_class;
  GeglOperationMetaClass *operation_meta_class;
  gchar                  *composition =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "  <node operation='gegl:crop' width='200' height='200'/>"
    "  <node operation='gegl:over'>"
    "    <node operation='gegl:layer'>"
    "      <params>"
    "        <param name='opacity'>0.2</param>"
    "        <param name='x'>50</param>"
    "        <param name='y'>30</param>"
    "        <param name='scale'>0.5</param>"
    "        <param name='src'>standard-aux.png</param>"
    "      </params>"
    "    </node>"
    "    <node operation='gegl:load' path='standard-input.png'/>"
    "  </node>"
    "  <node operation='gegl:checkerboard'>"
    "    <params>"
    "      <param name='color1'>rgb(0.25,0.25,0.25)</param>"
    "      <param name='color2'>rgb(0.75,0.75,0.75)</param>"
    "    </params>"
    "  </node>"    
    "</gegl>";


  object_class         = G_OBJECT_CLASS (klass);
  operation_class      = GEGL_OPERATION_CLASS (klass);
  operation_meta_class = GEGL_OPERATION_META_CLASS (klass);

  object_class->finalize       = finalize;

  operation_meta_class->update = update_graph;
  operation_class->attach      = attach;

  gegl_operation_class_set_keys (operation_class,
    "name",                  "gegl:layer",
    "categories",            "meta",
    "title",                 _("Layer"),
    "reference-hash",        "44367aea166d43d6d55f8e11d0a654ee",
    "reference-composition", composition,
    "description", _("A layer in the traditional sense"),
    NULL);
}

#endif
