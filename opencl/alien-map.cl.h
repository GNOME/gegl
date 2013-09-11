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

static const char* alien_map_cl_source =
"__kernel void cl_alien_map(__global const float4 *in,                         \n"
"                           __global       float4 *out,                        \n"
"                                          float3 freq,                        \n"
"                                          float3 phaseshift,                  \n"
"                                          int3   keep)                        \n"
"{                                                                             \n"
"  int gid     = get_global_id(0);                                             \n"
"  float4 in_v = in[gid];                                                      \n"
"  float3 tmp  = 0.5*(1.0                                                      \n"
"                     + sin((2 * in_v.xyz - 1.0) * freq.xyz + phaseshift.xyz));\n"
"  float4 out_v;                                                               \n"
"                                                                              \n"
"  out_v.xyz = keep.xyz ? in_v.xyz : tmp;                                      \n"
"  out_v.w   = in_v.w;                                                         \n"
"  out[gid]  = out_v;                                                          \n"
"}                                                                             \n"
;
