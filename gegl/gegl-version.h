/* This file is the public GEGL API
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
 * 2000-2008 © Calvin Williamson, Øyvind Kolås.
 */

#ifndef __GEGL_VERSION_H__
#define __GEGL_VERSION_H__

/**
 * gegl_get_version:
 * @major: (out caller-allocates): a pointer to a int where the major version number will be stored
 * @minor: (out caller-allocates): ditto for the minor version number
 * @micro: (out caller-allocates): ditto for the micro version number
 *
 * This function fetches the version of the GEGL library being used by
 * the running process.
 */
void           gegl_get_version          (int *major,
                                          int *minor,
                                          int *micro);


#endif
