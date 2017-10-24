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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

property_double (shadows, _("Shadows"), 50.0)
    value_range (-100.0, 100.0)

property_double (highlights, _("Highlights"), -50.0)
    value_range (-100.0, 100.0)

property_double (whitepoint, _("White point adjustment"), 0.0)
    value_range (-10.0, 10.0)

property_double (radius, _("Radius"), 100.0)
    value_range (0.1, 1500.0)
    ui_range    (0.1, 200.0)

property_double (compress, _("Compress"), 50.0)
    value_range (0.0, 100.0)

property_double (shadows_ccorrect, _("Shadows color adjustment"), 100.0)
    value_range (0.0, 100.0)

property_double (highlights_ccorrect, _("Highlights color adjustment"), 50.0)
    value_range (0.0, 100.0)

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     shadows_highlights
#define GEGL_OP_C_SOURCE shadows-highlights.c

#include "gegl-op.h"

static void
attach (GeglOperation *operation)
{
  GeglNode *gegl;
  GeglNode *input;
  GeglNode *output;
  GeglNode *blur;
  GeglNode *shprocess;

  gegl   = operation->node;
  input  = gegl_node_get_input_proxy (gegl, "input");
  output = gegl_node_get_output_proxy (gegl, "output");

  blur = gegl_node_new_child (gegl,
                              "operation",    "gegl:gaussian-blur",
                              "abyss-policy", 1,
                               NULL);


  shprocess = gegl_node_new_child (gegl,
                                   "operation", "gegl:shadows-highlights-correction",
                                   NULL);

  gegl_node_link (input, blur);
  gegl_node_link_many (input, shprocess, output, NULL);

  gegl_node_connect_to (blur, "output", shprocess, "aux");

  gegl_operation_meta_redirect (operation, "radius", blur, "std-dev-x");
  gegl_operation_meta_redirect (operation, "radius", blur, "std-dev-y");
  gegl_operation_meta_redirect (operation, "shadows", shprocess, "shadows");
  gegl_operation_meta_redirect (operation, "highlights", shprocess, "highlights");
  gegl_operation_meta_redirect (operation, "whitepoint", shprocess, "whitepoint");
  gegl_operation_meta_redirect (operation, "compress", shprocess, "compress");
  gegl_operation_meta_redirect (operation, "shadows-ccorrect", shprocess, "shadows-ccorrect");
  gegl_operation_meta_redirect (operation, "highlights-ccorrect", shprocess, "highlights-ccorrect");

  gegl_operation_meta_watch_nodes (operation, blur, shprocess, NULL);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:shadows-highlights",
    "title",       _("Shadows-Highlights"),
    "categories",  "light",
    "license",     "GPL3+",
    "description", _("Perform shadows and highlights correction"),
    NULL);
}

#endif
