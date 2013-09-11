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

static const char* noise_pick_cl_source =
"__kernel void cl_noise_pick(__global const float4 *src,                       \n"
"                            __global       float4 *dst,                       \n"
"                            __global const int    *random_data,               \n"
"                            __global const long   *random_primes,             \n"
"                                           int     x_offset,                  \n"
"                                           int     y_offset,                  \n"
"                                           int     src_width,                 \n"
"                                           int     whole_region_width,        \n"
"                                           int     radius,                    \n"
"                                           int     seed,                      \n"
"                                           float   pct_random,                \n"
"                                           int     offset)                    \n"
"{                                                                             \n"
"  int gidx = get_global_id(0);                                                \n"
"  int gidy = get_global_id(1);                                                \n"
"                                                                              \n"
"  int x = gidx + x_offset;                                                    \n"
"  int y = gidy + y_offset;                                                    \n"
"  int n = 2 * (x + whole_region_width * y + offset);                          \n"
"                                                                              \n"
"  int src_idx = (gidx + radius) + src_width * (gidy + radius);                \n"
"  int dst_idx = gidx + get_global_size(0) * gidy;                             \n"
"                                                                              \n"
"  float4 src_v[9] = { src[src_idx - src_width - 1],                           \n"
"                      src[src_idx - src_width],                               \n"
"                      src[src_idx - src_width + 1],                           \n"
"                      src[src_idx - 1],                                       \n"
"                      src[src_idx],                                           \n"
"                      src[src_idx + 1],                                       \n"
"                      src[src_idx + src_width - 1],                           \n"
"                      src[src_idx + src_width],                               \n"
"                      src[src_idx + src_width + 1]                            \n"
"                    };                                                        \n"
"                                                                              \n"
"  float pct = gegl_cl_random_float_range (random_data, random_primes,         \n"
"                                          seed, x, y, 0, n, 0.0, 100.0);      \n"
"  int   k   = gegl_cl_random_int_range (random_data, random_primes,           \n"
"                                        seed, x, y, 0, n+1, 0, 9);            \n"
"                                                                              \n"
"  float4 tmp = src_v[4];                                                      \n"
"  if (pct <= pct_random)                                                      \n"
"      tmp = src_v[k];                                                         \n"
"  dst[dst_idx] = tmp;                                                         \n"
"}                                                                             \n"
"                                                                              \n"
"                                                                              \n"
"__kernel void copy_out_to_aux(__global const float4 *src,                     \n"
"                              __global       float4 *dst,                     \n"
"                                             int     src_width,               \n"
"                                             int     radius)                  \n"
"{                                                                             \n"
"  int gidx    = get_global_id(0);                                             \n"
"  int gidy    = get_global_id(1);                                             \n"
"  int dst_idx = (gidx + radius) + src_width * (gidy + radius);                \n"
"  int src_idx = gidx + get_global_size(0) * gidy;                             \n"
"                                                                              \n"
"  float4 tmp   = src[src_idx];                                                \n"
"  dst[dst_idx] = tmp;                                                         \n"
"}                                                                             \n"
;

