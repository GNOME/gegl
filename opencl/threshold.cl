__kernel void gegl_threshold (__global const float2     *in,
                              __global const float      *aux,
                              __global       float2     *out,
                                       const float       value,
                                       const float       high)
{
  int gid = get_global_id(0);
  float2 in_v  = in [gid];
  float2 out_v;

  if (! aux)
    {
      out_v.x  = (in_v.x >= value && in_v.x <= high)? 1.0 : 0.0;
      out_v.y  = in_v.y;
      out[gid] = out_v;
    }
  else
    {
      float level_low  = aux[gid];
      float level_high = level_low;

      if (value <= 0.5)
        {
          float t = 1.0-((value)/0.5);
          level_low = 0.0 * t + level_low * (1.0-t);
        }
      else
        {
          float t = 1.0-((1.0-value)/0.5);
          level_low = 1.0* t + level_low * (1.0-t);
        }
      if (high <= 0.5)
        {
          float t = 1.0-((high)/0.5);
          level_high = 0.0 * t + level_high * (1.0-t);
        }
      else
        {
          float t = 1.0-((1.0-high)/0.5);
          level_high = 1.0* t + level_high * (1.0-t);
        }

      out_v.x = (in_v.x >= level_low && in_v.x <= level_high)? 1.0 : 0.0;
      out_v.y  = in_v.y;
      out[gid] = out_v;
    }
}
