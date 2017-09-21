__kernel void gegl_hue_chroma (__global const float4 *in,
                               __global       float4 *out,
                                              float   hue,
                                              float   chroma,
                                              float   lightness)
{
  int gid = get_global_id(0);
  float4 in_v  = in [gid];
  float4 out_v;
  out_v.x = in_v.x + lightness;
  out_v.y = clamp(in_v.y + chroma, 0.f, 200.f);
  out_v.z = in_v.z + hue;
  out_v.w = in_v.w;
  out[gid]  =  out_v;
}
