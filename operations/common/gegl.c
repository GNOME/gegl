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
 * Copyright 2016 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#define TUTORIAL \
"id=input\n"\
"# uncomment a set of lines below to do test an example, use\n#use ctrl+a before typing to select all, if you want a blank slate.\n"\
"\n"\
"# adaptive threshold\n"\
"#threshold aux=[\n"\
"#  ref=input\n"\
"#  gaussian-blur\n"\
"#    std-dev-x=0.25rel  # rel suffix means relative to\n"\
"#    std-dev-y=0.25rel  # input dimensions rather than in pixels\n"\
"#]\n"\
"\n"\
"# antialias vignette proportion=0 radius=0.7\n"\
"\n"\
"# text overlay\n"\
"#over aux=[\n"\
"#  text wrap=1.0rel  color=rgb(0.1,0.1,.3) size=.1rel\n"\
"#    string=\"ipsum dolor, sic amet foo bar baz qux amelqur fax\"\n"\
"#  dropshadow\n"\
"#     radius=.01rel  grow-radius=0.0065rel color=white x=0 y=0\n"\
"#  rotate degrees=3.1415\n"\
"#  translate y=.1rel x=.1rel\n"\
"#]\n"\
"\n"\
"#over aux=[\n"\
"#  ref=input\n"\
"#  opacity aux=[\n"\
"#    color value=black over aux=[\n"\
"#        text string=\"original\" color=white  size=.3rel\n"\
"#        translate y=.7rel\n"\
"#      ]\n"\
"#    ]\n"\
"#]\n"\
"\n"\
"#over aux=[\n"\
"#  ref=input\n"\
"#  scale-ratio x=0.20 y=0.20\n"\
"#  newsprint period=0.01rel period2=0.01rel period3=0.01rel period4=0.01rel\n"\
"#    color-model=cmyk\n"\
"#    aa-samples=64\n"\
"#    pattern=pssquare\n"\
"#    pattern2=pssquare\n"\
"#    pattern3=pssquare\n"\
"#    pattern4=pssquare\n"\
"#    translate x=0.1rel y=0.5rel\n"\
"#]\n"\
"\n"\
"#over aux=[\n"\
"#  ref=input\n"\
"#  scale-ratio x=0.20 y=0.20\n"\
"#  newsprint period=0.01rel period2=0.01rel period3=0.01rel period4=00.01rel\n"\
"#  color-model=rgb aa-samples=64\n"\
"#  translate x=0.4rel y=0.5rel\n"\
"#]\n"\
"\n"\
"#over aux=[\n"\
"#  ref=input\n"\
"#  scale-ratio x=0.20 y=0.20\n"\
"#  snn-mean snn-mean\n"\
"#  translate x=0.7rel y=0.5rel\n"\
"#]\n"\
"\n"\
"#over aux=[\n"\
"#  ref=input\n"\
"#  scale-ratio x=0.20 y=0.20\n"\
"#  mosaic tile-size=0.03rel\n"\
"#  translate x=1.0rel y=0.5rel\n"\
"#]\n"\
"\n"



#ifdef GEGL_PROPERTIES

property_string (string, _("pipeline"), TUTORIAL)
    description(_("[op [property=value] [property=value]] [[op] [property=value]"))
    ui_meta ("multiline", "true")

property_string (error, _("Eeeeeek"), "")
    description (_("There is a problem in the syntax or in the application of parsed property values. Things might mostly work nevertheless."))
    ui_meta ("error", "true")

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     gegl
#define GEGL_OP_C_SOURCE gegl.c

#include "gegl-op.h"
#include <unistd.h>


#include <stdio.h>
#include <stdlib.h>

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglNode *gegl, *input, *output;
  GError *error = NULL;

  gegl = operation->node;

  if (!o->user_data || !g_str_equal (o->user_data, o->string))
  {
    if (o->user_data)
      g_free (o->user_data);
    o->user_data = g_strdup (o->string);

  input  = gegl_node_get_input_proxy (gegl,  "input");
  output = gegl_node_get_output_proxy (gegl, "output");

  gegl_node_link_many (input, output, NULL);
  {
     gchar cwd[81920]; // XXX: should do better
     getcwd (cwd, sizeof(cwd));
  gegl_create_chain (o->string, input, output, 0.0,
                     gegl_node_get_bounding_box (input).height, cwd,
                     &error);
  }

  if (error)
  {
    gegl_node_set (gegl, "error", error->message, NULL);
    g_clear_error (&error);
  }
  else
  {
    g_object_set (operation, "error", "", NULL);
  }
  }
}

static void
attach (GeglOperation *operation)
{
  GeglNode *gegl, *input, *output;

  gegl    = operation->node;

  input  = gegl_node_get_input_proxy (gegl, "input");
  output = gegl_node_get_output_proxy (gegl, "output");

  gegl_node_link_many (input, output, NULL);
  prepare (operation);
}

static void
dispose (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);
  if (o->user_data)
  {
    g_free (o->user_data);
    o->user_data = NULL;
  }
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass       *object_class;
  GeglOperationClass *operation_class;

  object_class = G_OBJECT_CLASS  (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->dispose = dispose;
  operation_class->attach = attach;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:gegl",
    "title",       _("GEGL graph"),
    "categories",  "generic",
    "reference-hash", "29bf5654242f069e2867ba9cb41d8d4e",
    "description", _("Do a chain of operations, with key=value pairs after each operation name to set properties. And aux=[ source filter ] for specifying a chain with a source as something connected to an aux pad."),
    NULL);
}

#endif
