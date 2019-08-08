__kernel void box_blur_hor (__global const float4     *in,
                            __global       float4     *out,
                                     const int         radius)
{
  const int size          = 2 * radius + 1;
  const int gidx          = get_global_id (0);
  const int gidy          = get_global_id (1);
  const int src_rowstride = get_global_size (0) + size - 1;
  const int dst_rowstride = get_global_size (0);

  const int src_offset    = gidx + gidy * src_rowstride + radius;
  const int dst_offset    = gidx + gidy * dst_rowstride;

  const int src_start_ind = src_offset - radius;

  float4 mean = 0.0f;

  for (int i = 0; i < size; i++)
    {
      mean += in[src_start_ind + i];
    }

  out[dst_offset] = mean / (float)(size);
}

__kernel void box_blur_ver (__global const float4     *in,
                            __global       float4     *out,
                                     const int         radius)
{
  const int size          = 2 * radius + 1;
  const int gidx          = get_global_id (0);
  const int gidy          = get_global_id (1);
  const int src_rowstride = get_global_size (0);
  const int dst_rowstride = get_global_size (0);

  const int src_offset    = gidx + (gidy + radius) * src_rowstride;
  const int dst_offset    = gidx +  gidy           * dst_rowstride;

  const int src_start_ind = src_offset - radius * src_rowstride;

  float4 mean = 0.0f;

  for (int i = 0; i < size; i++)
    {
      mean += in[src_start_ind + i * src_rowstride];
    }

  out[dst_offset] = mean / (float)(size);
}

