#define EPSILON 0.00001
#define NORMALIZE(d, tt, ot, c) ((d - tt) / (min (ot, c) - tt))

__kernel void cl_color_to_alpha(__global const float4 *in,
                                __global       float4 *out,
                                         const float4  color,
                                         const float   transparency_threshold,
                                         const float   opacity_threshold)
{
  const int    gid         = get_global_id (0);
  const float4 in_v        = in[gid];
        float4 out_v       = in_v;
  const float4 d           = fabs (out_v - color);
        float4 a;
  const float  input_alpha = out_v.w;
  const float  tt          = transparency_threshold;
  const float  ot          = opacity_threshold;
        float  alpha       = 0.0f;
        float  dist        = 0.0f;

  a = select (
        select (
          select (NORMALIZE (d, tt, ot, 1.0f - color),
                  NORMALIZE (d, tt, ot, color), out_v < color),
                1.0f, d > ot - (float4)EPSILON),
              0.0f, d < tt + (float4)EPSILON);

/* The above version could be hardware optimized and essentially does the below
   loop except for the if a[i] > alpha statements, it's vectorized code so there
   could be some performance benefits.

  for (int i = 0; i < 3; i++)
    {
      if (d[i] < tt + EPSILON)
        a[i] = 0.0f;
      else if (d[i] > ot - EPSILON)
        a[i] = 1.0f;
      else if (out_v[i] < color[i])
        a[i] = NORMALIZE (d[i], tt, ot, color[i]);
      else
        a[i] = NORMALIZE (d[i], tt, ot, 1.0f - color[i]);

      if (a[i] > alpha)
        {
          alpha = a[i];
          dist  = d[i];
        }
    }
*/

  if (a[2] > a[1] && a[2] > a[0])
    {
      alpha = a[2];
      dist  = d[2];
    }
  else if (a[1] > a[0])
    {
      alpha = a[1];
      dist  = d[1];
    }
  else
    {
      alpha = a[0];
      dist  = d[0];
    }

  if (alpha > EPSILON)
    {
      const float  ratio     = tt / dist;
      const float  alpha_inv = 1.0f / alpha;
      const float4 c         = color + (out_v - color) * ratio;
      out_v                  = c + (out_v - c) * alpha_inv;
    }

  out_v.w  = input_alpha * alpha;
  out[gid] = out_v;
}
