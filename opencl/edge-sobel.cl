#define SOBEL_RADIUS 1
/* FIXME: There are issues while horizontal and vertical are enabled together or
          if keep_sign is disabled and either of horizontal or vertical is enabled. */
#define FIXME 1

/* Boundary check function which clamps pixels similar to the
   non OpenCL version */
static inline float4 clamp_pixel (global float4 *in,
                                         int     x,
                                         int     y,
                                         int     width,
                                         int     height) {
    x = (x < 0) ? 0 : ((x < width)  ? x : (width - 1));
    y = (y < 0) ? 0 : ((y < height) ? y : (height - 1));
    return in[x + y * width];
}

kernel void kernel_edgesobel (global float4 *in,
                              global float4 *out,
                              const int horizontal,
                              const int vertical,
                              const int keep_sign,
                              const int has_alpha)
{
    const int gidx = get_global_id (0);
    const int gidy = get_global_id (1);

    float4 hor_grad = 0.0f;
    float4 ver_grad = 0.0f;
    float4 gradient = 0.0f;

    const int dst_width  = get_global_size (0);
    const int dst_height = get_global_size (1);
    const int src_width  = dst_width  + (SOBEL_RADIUS << 1);
    const int src_ht     = dst_height + (SOBEL_RADIUS << 1);

    const float4 pix_fl = clamp_pixel (in, gidx - 1, gidy - 1, src_width, src_ht);
    const float4 pix_fm = clamp_pixel (in, gidx,     gidy - 1, src_width, src_ht);
    const float4 pix_fr = clamp_pixel (in, gidx + 1, gidy - 1, src_width, src_ht);
    const float4 pix_ml = clamp_pixel (in, gidx - 1, gidy,     src_width, src_ht);
    const float4 pix_mm = clamp_pixel (in, gidx,     gidy,     src_width, src_ht);
    const float4 pix_mr = clamp_pixel (in, gidx + 1, gidy,     src_width, src_ht);
    const float4 pix_bl = clamp_pixel (in, gidx - 1, gidy + 1, src_width, src_ht);
    const float4 pix_bm = clamp_pixel (in, gidx,     gidy + 1, src_width, src_ht);
    const float4 pix_br = clamp_pixel (in, gidx + 1, gidy + 1, src_width, src_ht);

    if (horizontal)
      {
        hor_grad +=
            (-1.0f * pix_fl + 1.0f * pix_fr) +
            (-2.0f * pix_ml + 2.0f * pix_mr) +
            (-1.0f * pix_bl + 1.0f * pix_br);
      }
    if (vertical)
      {
        ver_grad +=
            ( 1.0f * pix_fl) + ( 2.0f * pix_fm) + ( 1.0f * pix_fr) +
            (-1.0f * pix_bl) + (-2.0f * pix_bm) + (-1.0f * pix_br);
      }

#ifndef FIXME
    if (horizontal && vertical)
      {
        /* sqrt(32.0) = 5.656854 */
        gradient = hypot (hor_grad, ver_grad) / (float4) 5.656854f;
      }
    else
      {
        if (keep_sign)
#endif
          gradient = (float4) 0.5f + (hor_grad + ver_grad) / (float4) 8.0f;
#ifndef FIXME
        else
          gradient = fabs (hor_grad + ver_grad) / (float4) 4.0f;
      }
#endif
    if (has_alpha)
      gradient.w = pix_mm.w;
    else
      gradient.w = 1.0f;

    out[gidx + gidy * dst_width] = gradient;
}
