__kernel void cl_contrast_curve(__global const float2 *in,
                                __global       float2 *out,
                                __constant     float  *curve,
                                constant int           num_sampling_points)
{
  int gid = get_global_id(0);
  float2 in_v = in[gid];
  int x = in_v.x * num_sampling_points;
  float y;

  if(x < 0)
    y = curve[0];
  else if (x < num_sampling_points)
    y = curve[x];
  else
    y = curve[num_sampling_points - 1];

  out[gid] = float2(y,in_v.y);
}

