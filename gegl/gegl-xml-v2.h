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
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006 Øyvind Kolås
 * Copyright 2012 Michael Muré <batolettre@gmail.com>
 */

#ifndef GEGL_XML_V2_H
#define GEGL_XML_V2_H

GeglNode * gegl_node_new_from_xml_v2  (const gchar *xmldata,
                                       const gchar *path_root);
GeglNode * gegl_node_new_from_file_v2 (const gchar *path);

#endif
