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

__kernel void cl_noise_pick(__global const float4 *src,
                            __global       float4 *dst,
                            __global const int    *random_data,
                            __global const long   *random_primes,
                                           int     x_offset,
                                           int     y_offset,
                                           int     src_width,
                                           int     whole_region_width,
                                           int     radius,
                                           int     seed,
                                           float   pct_random,
                                           int     offset)
{
  int gidx = get_global_id(0);
  int gidy = get_global_id(1);

  int x = gidx + x_offset;
  int y = gidy + y_offset;
  int n = 2 * (x + whole_region_width * y + offset);

  int src_idx = (gidx + radius) + src_width * (gidy + radius);
  int dst_idx = gidx + get_global_size(0) * gidy;

  float4 src_v[9] = { src[src_idx - src_width - 1],
                      src[src_idx - src_width],
                      src[src_idx - src_width + 1],
                      src[src_idx - 1],
                      src[src_idx],
                      src[src_idx + 1],
                      src[src_idx + src_width - 1],
                      src[src_idx + src_width],
                      src[src_idx + src_width + 1]
                    };

  float pct = gegl_cl_random_float_range (random_data, random_primes,
                                          seed, x, y, 0, n, 0.0, 100.0);
  int   k   = gegl_cl_random_int_range (random_data, random_primes,
                                        seed, x, y, 0, n+1, 0, 9);

  float4 tmp = src_v[4];
  if (pct <= pct_random)
      tmp = src_v[k];
  dst[offset] = tmp;
}

__kernel void copy_out_to_aux(__global const float4 *src,
                              __global       float4 *dst,
                                             int     src_width,
                                             int     radius)
{
  int gidx    = get_global_id(0);
  int gidy    = get_global_id(1);
  int dst_idx = (gidx + radius) + src_width * (gidy + radius);
  int src_idx = gidx + get_global_size(0) * gidy;

  float4 tmp   = src[src_idx];
  dst[dst_idx] = tmp;
}
