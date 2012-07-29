/* This file is part of GEGL editor -- a gtk frontend for GEGL
 *
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2012 Michael Muré <batolettre@gmail.com>
 */

#include "config.h"

#include <stdlib.h>
#include <gegl.h>
#include <gegl-xml-v2.h>

static G_GNUC_NORETURN void
usage (char *application_name)
{
    g_print ( "usage: %s file\n"
              "\n"
              "Convert a xml v1 grah into a xml v2 graph.\n"
              , application_name);
    exit (0);
}

gint
main (gint    argc,
      gchar **argv)
{
  GeglNode    *gegl      = NULL;

  gegl_init (&argc, &argv);

  if (argc==1)
      usage (argv[0]);

  gegl = gegl_node_new_from_file(argv[1]);

  g_print (gegl_node_to_xml_v2 (gegl, g_get_current_dir ()));

  gegl_exit ();
  return 0;
}
