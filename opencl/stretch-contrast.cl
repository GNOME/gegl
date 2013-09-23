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

__kernel void two_stages_local_min_max_reduce (__global const float4 *in,
                                               __global       float  *out_min,
                                               __global       float  *out_max,
                                               __local        float  *aux_min,
                                               __local        float  *aux_max,
                                                              int    n_pixels)
{
  int    gid   = get_global_id(0);
  int    gsize = get_global_size(0);
  int    lid   = get_local_id(0);
  int    lsize = get_local_size(0);
  float4 min_v = (float4)(FLT_MAX);
  float4 max_v = (float4)(FLT_MIN);
  float4 in_v;
  float  aux0, aux1;
  int    it;
  /* Loop sequentially over chunks of input vector */
  while (gid < n_pixels)
    {
      in_v  =  in[gid];
      min_v =  fmin(min_v,in_v);
      max_v =  fmax(max_v,in_v);
      gid   += gsize;
    }

  /* Perform parallel reduction */
  aux_min[lid] = min(min(min_v.x,min_v.y),min_v.z);
  aux_max[lid] = max(max(max_v.x,max_v.y),max_v.z);
  barrier(CLK_LOCAL_MEM_FENCE);
  for(it = lsize / 2; it > 0; it >>= 1)
    {
      if (lid < it)
        {
          aux0         = aux_min[lid + it];
          aux1         = aux_min[lid];
          aux_min[lid] = fmin(aux0, aux1);

          aux0         = aux_max[lid + it];
          aux1         = aux_max[lid];
          aux_max[lid] = fmax(aux0, aux1);
        }
      barrier(CLK_LOCAL_MEM_FENCE);
  }
  if (lid == 0)
    {
      out_min[get_group_id(0)] = aux_min[0];
      out_max[get_group_id(0)] = aux_max[0];
    }
}

__kernel void init_to_float_max (__global float *in)
{
  int gid = get_global_id(0);
  in[gid] = FLT_MAX;
}

__kernel void init_to_float_min (__global float *in)
{
  int gid = get_global_id(0);
  in[gid] = FLT_MIN;
}

__kernel void global_min_max_reduce (__global float *in_min,
                                     __global float *in_max,
                                     __global float *out_min,
                                     __global float *out_max)
{
  int   gid   = get_global_id(0);
  int   lid   = get_local_id(0);
  int   lsize = get_local_size(0);
  float aux0, aux1;
  int   it;

  /* Perform parallel reduction */
  for(it = lsize / 2; it > 0; it >>= 1)
    {
      if (lid < it)
        {
          aux0        = in_min[gid + it];
          aux1        = in_min[gid];
          in_min[gid] = fmin(aux0, aux1);

          aux0        = in_max[gid + it];
          aux1        = in_max[gid];
          in_max[gid] = fmax(aux0, aux1);
        }
      barrier(CLK_LOCAL_MEM_FENCE);
  }
  if (lid == 0)
    {
      out_min[get_group_id(0)] = in_min[gid];
      out_max[get_group_id(0)] = in_max[gid];
    }
}

__kernel void cl_stretch_contrast (__global const float4 *in,
                                   __global       float4 *out,
                                                  float   min,
                                                  float   diff)
{
  int    gid  = get_global_id(0);
  float4 in_v = in[gid];

  in_v.xyz = (in_v.xyz - min) / diff;
  out[gid] = in_v;
}
