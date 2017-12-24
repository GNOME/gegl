float3 overlay(const float3 in_a,
               const float3 in_b,
               const float  opacity,
               const float  transform,
               const float  ccorrect,
               const float  low_approximation)
{
  /* a contains underlying image; b contains mask */

  const float3 scale = (float3)(100.0f, 128.0f, 128.0f);
  float3 a = in_a / scale;
  float3 b = in_b / scale;


  float opacity2 = opacity*opacity;

  while(opacity2 > 0.0f)
  {
    float la = a.x;
    float lb = (b.x - 0.5f) * sign(opacity)*sign(1.0f - la) + 0.5f;
    float lref = copysign(fabs(la) > low_approximation ? 1.0f/fabs(la) : 1.0f/low_approximation, la);
    float href = copysign(fabs(1.0f - la) > low_approximation ? 1.0f/fabs(1.0f - la) : 1.0f/low_approximation, 1.0f - la);

    float chunk = opacity2 > 1.0f ? 1.0f : opacity2;
    float optrans = chunk * transform;
    opacity2 -= 1.0f;

    a.x = la * (1.0f - optrans) + (la > 0.5f ? 1.0f - (1.0f - 2.0f * (la - 0.5f)) * (1.0f-lb) : 2.0f * la * lb) * optrans;
    a.y = a.y * (1.0f - optrans) + (a.y + b.y) * (a.x*lref * ccorrect + (1.0f - a.x)*href * (1.0f - ccorrect)) * optrans;
    a.z = a.z * (1.0f - optrans) + (a.z + b.z) * (a.x*lref * ccorrect + (1.0f - a.x)*href * (1.0f - ccorrect)) * optrans;
  }
  /* output scaled back pixel */
  return a * scale;
}


__kernel void shadows_highlights(__global const float4  *in,
                                 __global const float   *aux,
                                 __global       float4  *out,
                                          const float    shadows,
                                          const float    highlights,
                                          const float    compress,
                                          const float    shadows_ccorrect,
                                          const float    highlights_ccorrect,
                                          const float    whitepoint)
{
  int gid = get_global_id(0);
  const float low_approximation = 0.01f;

  float4 io = in[gid];
  float3 m = (float3)0.0f;
  float xform;

  if (! aux)
    {
      out[gid] = io;
      return;
    }

  /* blurred, inverted and desaturaed mask in m */
  m.x = 100.0f - aux[gid];

  /* white point adjustment */
  io.x = io.x > 0.0f ? io.x/whitepoint : io.x;
  m.x = m.x > 0.0f ? m.x/whitepoint : m.x;

  /* overlay highlights */
  xform = clamp(1.0f - 0.01f * m.x/(1.0f-compress), 0.0f, 1.0f);
  io.xyz = overlay(io.xyz, m, -highlights, xform, 1.0f - highlights_ccorrect, low_approximation);

  /* overlay shadows */
  xform = clamp(0.01f * m.x/(1.0f-compress) - compress/(1.0f-compress), 0.0f, 1.0f);
  io.xyz = overlay(io.xyz, m, shadows, xform, shadows_ccorrect, low_approximation);

  out[gid] = io;
}
