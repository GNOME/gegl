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
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2013 Carlos Zubieta <czubieta.dev@gmail.com>
 */

static const char* posterize_cl_source =
"__kernel void cl_posterize(__global const float4 *in,                         \n"
"                           __global       float4 *out,                        \n"
"                                          float  levels)                      \n"
"{                                                                             \n"
"  int gid     = get_global_id(0);                                             \n"
"  float4 in_v = in[gid];                                                      \n"
"                                                                              \n"
"  in_v.xyz  = trunc(in_v.xyz*levels + 0.5) / levels;                          \n"
"  out[gid]  = in_v;                                                           \n"
"}                                                                             \n"
;
