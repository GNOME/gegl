void
BILINEAR_FUNCNAME (guchar                    *dest_buf,
                   const guchar              *source_buf,
                   const GeglRectangle *dst_rect,
                   const GeglRectangle *src_rect,
                   const gint                 s_rowstride,
                   const gdouble              scale,
                   const gint                 bpp,
                   const gint                 d_rowstride)
{
  const gint components = bpp / sizeof(BILINEAR_TYPE);
  const gint ver  = s_rowstride/(bpp/components);
  const gint diag = ver + components;

  gfloat dx[dst_rect->width];
  gint   jj[dst_rect->width];

  for (gint x = 0; x < dst_rect->width; x++)
  {
    gfloat sx  = (dst_rect->x + x + 0.5f) / scale - src_rect->x - 0.5f;
    jj[x]  = int_floorf (sx);
    dx[x]  = sx - jj[x];
    jj[x] *= components;
  }
#define IMPL(...) do{ \
  for (gint y = 0; y < dst_rect->height; y++)\
    {\
      const gfloat sy = (dst_rect->y + y + 0.5f) / scale - src_rect->y - 0.5f;\
      const gint   ii = int_floorf (sy);\
      const gfloat dy = (sy - ii);\
      const gfloat rdy = 1.0 - dy;\
      BILINEAR_TYPE *dst = (BILINEAR_TYPE*)(dest_buf + y * d_rowstride);\
      const guchar  *src_base = source_buf + ii * s_rowstride;\
\
      for (gint x = 0; x < dst_rect->width; x++)\
      {\
        const gfloat ldx = dx[x];\
        const gfloat rdx = 1.0f-ldx;\
        const BILINEAR_TYPE *src[4];\
        src[0] = src[1] = src[2] = src[3] = \
            (const BILINEAR_TYPE*)(src_base) + jj[x];\
        src[1] += components;\
        src[2] += ver;\
        src[3] += diag;\
        __VA_ARGS__;\
        dst += components;\
      }\
    }\
}while(0)
   switch (components)
   {
     default:
       IMPL(
         for (gint i = 0; i < components; ++i)
          {
            dst[i] =
               BILINEAR_ROUND(
               (src[0][i] * (rdx) + src[1][i] * (ldx)) * (rdy) +
               (src[2][i] * (rdx) + src[3][i] * (ldx)) * (dy));
          });
       break;
     case 1:
       IMPL( dst[0] =
               BILINEAR_ROUND(
               (src[0][0] * (rdx) + src[1][0] * (ldx)) * (rdy) +
               (src[2][0] * (rdx) + src[3][0] * (ldx)) * (dy));
           );
       break;
     case 2:
       IMPL(
         dst[0] = BILINEAR_ROUND(
               (src[0][0] * (rdx) + src[1][0] * (ldx)) * (rdy) +
               (src[2][0] * (rdx) + src[3][0] * (ldx)) * (dy));
         dst[1] = BILINEAR_ROUND(
               (src[0][1] * (rdx) + src[1][1] * (ldx)) * (rdy) +
               (src[2][1] * (rdx) + src[3][1] * (ldx)) * (dy));
           );
       break;
     case 3:
       IMPL(
         dst[0] = BILINEAR_ROUND(
               (src[0][0] * (rdx) + src[1][0] * (ldx)) * (rdy) +
               (src[2][0] * (rdx) + src[3][0] * (ldx)) * (dy));
         dst[1] = BILINEAR_ROUND(
               (src[0][1] * (rdx) + src[1][1] * (ldx)) * (rdy) +
               (src[2][1] * (rdx) + src[3][1] * (ldx)) * (dy));
         dst[2] = BILINEAR_ROUND(
               (src[0][2] * (rdx) + src[1][2] * (ldx)) * (rdy) +
               (src[2][2] * (rdx) + src[3][2] * (ldx)) * (dy));
           );
       break;
     case 4:
       IMPL(
         dst[0] = BILINEAR_ROUND(
               (src[0][0] * (rdx) + src[1][0] * (ldx)) * (rdy) +
               (src[2][0] * (rdx) + src[3][0] * (ldx)) * (dy));
         dst[1] = BILINEAR_ROUND(
               (src[0][1] * (rdx) + src[1][1] * (ldx)) * (rdy) +
               (src[2][1] * (rdx) + src[3][1] * (ldx)) * (dy));
         dst[2] = BILINEAR_ROUND(
               (src[0][2] * (rdx) + src[1][2] * (ldx)) * (rdy) +
               (src[2][2] * (rdx) + src[3][2] * (ldx)) * (dy));
         dst[3] = BILINEAR_ROUND(
               (src[0][3] * (rdx) + src[1][3] * (ldx)) * (rdy) +
               (src[2][3] * (rdx) + src[3][3] * (ldx)) * (dy));
           );
       break;
     case 5:
       IMPL(
         dst[0] = BILINEAR_ROUND(
               (src[0][0] * (rdx) + src[1][0] * (ldx)) * (rdy) +
               (src[2][0] * (rdx) + src[3][0] * (ldx)) * (dy));
         dst[1] = BILINEAR_ROUND(
               (src[0][1] * (rdx) + src[1][1] * (ldx)) * (rdy) +
               (src[2][1] * (rdx) + src[3][1] * (ldx)) * (dy));
         dst[2] = BILINEAR_ROUND(
               (src[0][2] * (rdx) + src[1][2] * (ldx)) * (rdy) +
               (src[2][2] * (rdx) + src[3][2] * (ldx)) * (dy));
         dst[3] = BILINEAR_ROUND(
               (src[0][3] * (rdx) + src[1][3] * (ldx)) * (rdy) +
               (src[2][3] * (rdx) + src[3][3] * (ldx)) * (dy));
         dst[4] = BILINEAR_ROUND(
               (src[0][4] * (rdx) + src[1][4] * (ldx)) * (rdy) +
               (src[2][4] * (rdx) + src[3][4] * (ldx)) * (dy));
           );
       break;
   }
#undef IMPL
}
