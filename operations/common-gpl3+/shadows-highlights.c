/* This file is an image processing operation for GEGL
 *
 * This program is free software: you can redistribute it and/or modify
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This operation is a port of the Darktable Shadows Highlights filter
 * copyright (c) 2012--2015 Ulrich Pegelow.
 *
 * GEGL port: Thomas Manni <thomas.manni@free.fr>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (shadows, _("Shadows"), 0.0)
    description (_("Adjust exposure of shadows"))
    value_range (-100.0, 100.0)

property_double (highlights, _("Highlights"), 0.0)
    description (_("Adjust exposure of highlights"))
    value_range (-100.0, 100.0)

property_double (whitepoint, _("White point adjustment"), 0.0)
    description (_("Shift white point"))
    value_range (-10.0, 10.0)

property_double (radius, _("Radius"), 100.0)
    description (_("Spatial extent"))
    value_range (0.1, 1500.0)
    ui_range    (0.1, 200.0)

property_double (compress, _("Compress"), 50.0)
    description (_("Compress the effect on shadows/highlights and preserve midtones"))
    value_range (0.0, 100.0)

property_double (shadows_ccorrect, _("Shadows color adjustment"), 100.0)
    description (_("Adjust saturation of shadows"))
    value_range (0.0, 100.0)

property_double (highlights_ccorrect, _("Highlights color adjustment"), 50.0)
    description (_("Adjust saturation of highlights"))
    value_range (0.0, 100.0)

#else

#define GEGL_OP_NAME     shadows_highlights
#define GEGL_OP_C_SOURCE shadows-highlights.c

#include <gegl-plugin.h>

struct _GeglOp
{
  GeglOperationMeta parent_instance;
  gpointer          properties;

  const Babl *blur_format;
  GeglNode *blur_convert;
  GeglNode *input;
  GeglNode *output;
};

typedef struct
{
  GeglOperationMetaClass parent_class;
} GeglOpClass;

#include "gegl-op.h"
GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_TYPE_OPERATION_META)

static gboolean
is_operation_a_nop (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  return GEGL_FLOAT_EQUAL (o->shadows, 0.0) &&
         GEGL_FLOAT_EQUAL (o->highlights, 0.0) &&
         GEGL_FLOAT_EQUAL (o->whitepoint, 0.0);
}

static void
do_setup (GeglOperation *operation)
{
  GeglOp *self = GEGL_OP (operation);
  GSList *children = NULL;
  GSList *l;

  g_return_if_fail (GEGL_IS_NODE (operation->node));
  g_return_if_fail (GEGL_IS_NODE (self->input));
  g_return_if_fail (GEGL_IS_NODE (self->output));

  self->blur_convert = NULL;

  children = gegl_node_get_children (operation->node);
  for (l = children; l != NULL; l = l->next)
    {
      GeglNode *node = GEGL_NODE (l->data);

      if (node == self->input || node == self->output)
        continue;

      g_object_unref (node);
    }

  if (is_operation_a_nop (operation))
    {
      gegl_node_link (self->input, self->output);
    }
  else
    {
      GeglNode *blur;
      GeglNode *shprocess;

      blur = gegl_node_new_child (operation->node,
                                  "operation",    "gegl:gaussian-blur",
                                  "abyss-policy", 1,
                                  NULL);

      if (self->blur_format == NULL)
        self->blur_format = babl_format ("YaA float");

      self->blur_convert = gegl_node_new_child (operation->node,
                                                "operation", "gegl:convert-format",
                                                "format",    self->blur_format,
                                                NULL);

      shprocess = gegl_node_new_child (operation->node,
                                       "operation", "gegl:shadows-highlights-correction",
                                       NULL);

      gegl_node_link_many (self->input, self->blur_convert, blur, NULL);
      gegl_node_link_many (self->input, shprocess, self->output, NULL);

      gegl_node_connect_to (blur, "output", shprocess, "aux");

      gegl_operation_meta_redirect (operation, "radius", blur, "std-dev-x");
      gegl_operation_meta_redirect (operation, "radius", blur, "std-dev-y");
      gegl_operation_meta_redirect (operation, "shadows", shprocess, "shadows");
      gegl_operation_meta_redirect (operation, "highlights", shprocess, "highlights");
      gegl_operation_meta_redirect (operation, "whitepoint", shprocess, "whitepoint");
      gegl_operation_meta_redirect (operation, "compress", shprocess, "compress");
      gegl_operation_meta_redirect (operation, "shadows-ccorrect", shprocess, "shadows-ccorrect");
      gegl_operation_meta_redirect (operation, "highlights-ccorrect", shprocess, "highlights-ccorrect");
    }

  g_slist_free (children);
}

static void
attach (GeglOperation *operation)
{
  GeglOp *self = GEGL_OP (operation);
  GeglNode *gegl;

  gegl   = operation->node;
  self->input  = gegl_node_get_input_proxy (gegl, "input");
  self->output = gegl_node_get_output_proxy (gegl, "output");

  do_setup (operation);
}

static void
prepare (GeglOperation *operation)
{
  GeglOp *self = GEGL_OP (operation);
  const Babl *blur_format = NULL;
  const Babl *input_format;

  input_format = gegl_operation_get_source_format (operation, "input");
  if (input_format == NULL)
    {
      blur_format = babl_format ("YaA float");
      goto out;
    }

  if (babl_format_has_alpha (input_format))
    blur_format = babl_format_with_space ("YaA float", input_format);
  else
    blur_format = babl_format_with_space ("Y float", input_format);

 out:
  g_return_if_fail (blur_format != NULL);

  if (self->blur_format != blur_format)
    {
      self->blur_format = blur_format;
      if (self->blur_convert != NULL)
        gegl_node_set (self->blur_convert, "format", self->blur_format, NULL);
    }
}

static void
my_set_property (GObject      *gobject,
                 guint         property_id,
                 const GValue *value,
                 GParamSpec   *pspec)
{
  GeglOperation *operation = GEGL_OPERATION (gobject);
  gboolean       is_nop;
  gboolean       was_nop;

  was_nop = is_operation_a_nop (operation);

  /* The set_property provided by the chant system does the
   * storing and reffing/unreffing of the input properties
   */
  set_property (gobject, property_id, value, pspec);

  is_nop = is_operation_a_nop (operation);
  if (operation->node != NULL && is_nop != was_nop)
    do_setup (operation);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass *object_class;
  GeglOperationClass *operation_class;

  object_class = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->set_property = my_set_property;

  operation_class->attach = attach;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:shadows-highlights",
    "title",       _("Shadows-Highlights"),
    "categories",  "light",
    "license",     "GPL3+",
    "description", _("Perform shadows and highlights correction"),
    NULL);
}

#endif
