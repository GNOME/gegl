#define EPSILON     1e-4f

__kernel void gegl_hue_chroma (__global const float4 *in,
                               __global       float4 *out,
                                              float   hue,
                                              float   chroma,
                                              float   lightness)
{
  int gid = get_global_id(0);
  float4 in_v  = in [gid];
  float4 out_v;

  if (fabs (in_v.y) > EPSILON)
    {
      out_v.y = in_v.y + chroma;
      out_v.z = in_v.z + hue;
    }
  else
    {
      out_v.y = in_v.y;
      out_v.z = in_v.z;
    }

  out_v.x = in_v.x + lightness;
  out_v.y = clamp (out_v.y, 0.f, 300.f);
  out_v.w = in_v.w;
  out[gid]  =  out_v;
}
